#include <VSGDBCore/X64Paging.h>

namespace VSGDBCore
{
    static constexpr U64 Cr0PagingEnabled = 1ull << 31;
    static constexpr U64 Cr4PaeEnabled = 1ull << 5;
    static constexpr U64 EferLongModeActive = 1ull << 10;

    static constexpr U64 PageOffsetMask4K = 0xfffull;
    static constexpr U64 PageOffsetMask2M = 0x1fffffull;
    static constexpr U64 PageOffsetMask1G = 0x3fffffffull;

    static constexpr U64 PageBaseMask4K =
        0x000ffffffffff000ull;

    static constexpr U64 PageBaseMask2M =
        0x000fffffffe00000ull;

    static constexpr U64 PageBaseMask1G =
        0x000fffffc0000000ull;

    static Result<U64>
        ReadPhysicalU64(
            IDebugTarget& Target,
            U64 PhysicalAddress)
    {
        auto Bytes = Target.ReadPhysicalMemory(
            PhysicalAddress,
            sizeof(U64));

        if (!Bytes.Succeeded())
        {
            return Result<U64>::Failure(Bytes.Error);
        }

        if (Bytes.Value.size() != sizeof(U64))
        {
            return Result<U64>::Failure(
                DebugError::Failure(
                    ErrorCode::BackendFailure,
                    L"Physical memory read returned unexpected size."));
        }

        U64 Value = 0;

        for (size_t Index = 0; Index < sizeof(U64); ++Index)
        {
            Value |=
                static_cast<U64>(Bytes.Value[Index])
                << (Index * 8);
        }

        return Result<U64>::Success(Value);
    }

    X64VirtualAddressParts
        DecodeX64VirtualAddress(
            U64 VirtualAddress)
    {
        X64VirtualAddressParts Parts{};

        Parts.Pml4Index = static_cast<U16>(
            (VirtualAddress >> 39) & 0x1ff);

        Parts.PdptIndex = static_cast<U16>(
            (VirtualAddress >> 30) & 0x1ff);

        Parts.PdIndex = static_cast<U16>(
            (VirtualAddress >> 21) & 0x1ff);

        Parts.PtIndex = static_cast<U16>(
            (VirtualAddress >> 12) & 0x1ff);

        Parts.PageOffset = static_cast<U16>(
            VirtualAddress & 0xfff);

        return Parts;
    }

    bool
        IsCanonicalX64VirtualAddress(
            U64 VirtualAddress)
    {
        const U64 Sign = (VirtualAddress >> 47) & 1;
        const U64 Upper = VirtualAddress >> 48;

        if (Sign)
        {
            return Upper == 0xffff;
        }

        return Upper == 0;
    }

    X64PageEntry
        DecodeX64PageEntry(
            X64PagingLevel Level,
            U64 EntryPhysicalAddress,
            U64 EntryValue)
    {
        X64PageEntry Entry{};

        Entry.Level = Level;
        Entry.EntryPhysicalAddress = EntryPhysicalAddress;
        Entry.Value = EntryValue;

        Entry.Present = (EntryValue & (1ull << 0)) != 0;
        Entry.Write = (EntryValue & (1ull << 1)) != 0;
        Entry.User = (EntryValue & (1ull << 2)) != 0;
        Entry.PageWriteThrough = (EntryValue & (1ull << 3)) != 0;
        Entry.PageCacheDisable = (EntryValue & (1ull << 4)) != 0;
        Entry.Accessed = (EntryValue & (1ull << 5)) != 0;
        Entry.Dirty = (EntryValue & (1ull << 6)) != 0;
        Entry.LargePage = (EntryValue & (1ull << 7)) != 0;
        Entry.Global = (EntryValue & (1ull << 8)) != 0;
        Entry.NoExecute = (EntryValue & (1ull << 63)) != 0;

        Entry.NextTablePhysicalBase =
            EntryValue & PageBaseMask4K;

        switch (Level)
        {
        case X64PagingLevel::Pdpt:
            if (Entry.LargePage)
            {
                Entry.PagePhysicalBase =
                    EntryValue & PageBaseMask1G;
            }
            else
            {
                Entry.PagePhysicalBase =
                    EntryValue & PageBaseMask4K;
            }
            break;

        case X64PagingLevel::Pd:
            if (Entry.LargePage)
            {
                Entry.PagePhysicalBase =
                    EntryValue & PageBaseMask2M;
            }
            else
            {
                Entry.PagePhysicalBase =
                    EntryValue & PageBaseMask4K;
            }
            break;

        default:
            Entry.PagePhysicalBase =
                EntryValue & PageBaseMask4K;
            break;
        }

        return Entry;
    }

    static X64AddressTranslationResult
        MakeFailure(
            U64 VirtualAddress,
            const X64PagingContext& Context,
            const X64VirtualAddressParts& Parts,
            std::vector<X64PageEntry>&& Entries,
            X64PagingFailureReason Reason,
            const std::wstring& Message)
    {
        X64AddressTranslationResult Result{};

        Result.Success = false;
        Result.VirtualAddress = VirtualAddress;
        Result.Cr3 = Context.Cr3;
        Result.Parts = Parts;
        Result.Entries = std::move(Entries);
        Result.FailureReason = Reason;
        Result.FailureMessage = Message;

        return Result;
    }

    Result<X64AddressTranslationResult>
        TranslateX64VirtualAddress(
            IDebugTarget& Target,
            const X64PagingContext& Context,
            U64 VirtualAddress)
    {
        X64VirtualAddressParts Parts =
            DecodeX64VirtualAddress(VirtualAddress);

        std::vector<X64PageEntry> Entries;

        if (!IsCanonicalX64VirtualAddress(VirtualAddress))
        {
            return Result<X64AddressTranslationResult>::Success(
                MakeFailure(
                    VirtualAddress,
                    Context,
                    Parts,
                    std::move(Entries),
                    X64PagingFailureReason::NonCanonicalAddress,
                    L"Virtual address is not canonical."));
        }

        if ((Context.Cr0 & Cr0PagingEnabled) == 0 ||
            (Context.Cr4 & Cr4PaeEnabled) == 0 ||
            (Context.Efer & EferLongModeActive) == 0)
        {
            return Result<X64AddressTranslationResult>::Success(
                MakeFailure(
                    VirtualAddress,
                    Context,
                    Parts,
                    std::move(Entries),
                    X64PagingFailureReason::UnsupportedPagingMode,
                    L"4-level long-mode paging is not active."));
        }

        const U64 Pml4Base =
            Context.Cr3 & PageBaseMask4K;

        const U64 Pml4eAddress =
            Pml4Base + static_cast<U64>(Parts.Pml4Index) * sizeof(U64);

        auto Pml4eValue = ReadPhysicalU64(Target, Pml4eAddress);
        if (!Pml4eValue.Succeeded())
        {
            return Result<X64AddressTranslationResult>::Failure(
                Pml4eValue.Error);
        }

        X64PageEntry Pml4e = DecodeX64PageEntry(
            X64PagingLevel::Pml4,
            Pml4eAddress,
            Pml4eValue.Value);

        Entries.push_back(Pml4e);

        if (!Pml4e.Present)
        {
            return Result<X64AddressTranslationResult>::Success(
                MakeFailure(
                    VirtualAddress,
                    Context,
                    Parts,
                    std::move(Entries),
                    X64PagingFailureReason::Pml4eNotPresent,
                    L"PML4E is not present."));
        }

        const U64 PdpteAddress =
            Pml4e.NextTablePhysicalBase +
            static_cast<U64>(Parts.PdptIndex) * sizeof(U64);

        auto PdpteValue = ReadPhysicalU64(Target, PdpteAddress);
        if (!PdpteValue.Succeeded())
        {
            return Result<X64AddressTranslationResult>::Failure(
                PdpteValue.Error);
        }

        X64PageEntry Pdpte = DecodeX64PageEntry(
            X64PagingLevel::Pdpt,
            PdpteAddress,
            PdpteValue.Value);

        Entries.push_back(Pdpte);

        if (!Pdpte.Present)
        {
            return Result<X64AddressTranslationResult>::Success(
                MakeFailure(
                    VirtualAddress,
                    Context,
                    Parts,
                    std::move(Entries),
                    X64PagingFailureReason::PdpteNotPresent,
                    L"PDPTE is not present."));
        }

        if (Pdpte.LargePage)
        {
            X64AddressTranslationResult Translation{};

            Translation.Success = true;
            Translation.VirtualAddress = VirtualAddress;
            Translation.Cr3 = Context.Cr3;
            Translation.Parts = Parts;
            Translation.Entries = std::move(Entries);
            Translation.PageSize = 0x40000000ull;
            Translation.PhysicalAddress =
                Pdpte.PagePhysicalBase +
                (VirtualAddress & PageOffsetMask1G);

            return Result<X64AddressTranslationResult>::Success(
                Translation);
        }

        const U64 PdeAddress =
            Pdpte.NextTablePhysicalBase +
            static_cast<U64>(Parts.PdIndex) * sizeof(U64);

        auto PdeValue = ReadPhysicalU64(Target, PdeAddress);
        if (!PdeValue.Succeeded())
        {
            return Result<X64AddressTranslationResult>::Failure(
                PdeValue.Error);
        }

        X64PageEntry Pde = DecodeX64PageEntry(
            X64PagingLevel::Pd,
            PdeAddress,
            PdeValue.Value);

        Entries.push_back(Pde);

        if (!Pde.Present)
        {
            return Result<X64AddressTranslationResult>::Success(
                MakeFailure(
                    VirtualAddress,
                    Context,
                    Parts,
                    std::move(Entries),
                    X64PagingFailureReason::PdeNotPresent,
                    L"PDE is not present."));
        }

        if (Pde.LargePage)
        {
            X64AddressTranslationResult Translation{};

            Translation.Success = true;
            Translation.VirtualAddress = VirtualAddress;
            Translation.Cr3 = Context.Cr3;
            Translation.Parts = Parts;
            Translation.Entries = std::move(Entries);
            Translation.PageSize = 0x200000ull;
            Translation.PhysicalAddress =
                Pde.PagePhysicalBase +
                (VirtualAddress & PageOffsetMask2M);

            return Result<X64AddressTranslationResult>::Success(
                Translation);
        }

        const U64 PteAddress =
            Pde.NextTablePhysicalBase +
            static_cast<U64>(Parts.PtIndex) * sizeof(U64);

        auto PteValue = ReadPhysicalU64(Target, PteAddress);
        if (!PteValue.Succeeded())
        {
            return Result<X64AddressTranslationResult>::Failure(
                PteValue.Error);
        }

        X64PageEntry Pte = DecodeX64PageEntry(
            X64PagingLevel::Pt,
            PteAddress,
            PteValue.Value);

        Entries.push_back(Pte);

        if (!Pte.Present)
        {
            return Result<X64AddressTranslationResult>::Success(
                MakeFailure(
                    VirtualAddress,
                    Context,
                    Parts,
                    std::move(Entries),
                    X64PagingFailureReason::PteNotPresent,
                    L"PTE is not present."));
        }

        X64AddressTranslationResult Translation{};

        Translation.Success = true;
        Translation.VirtualAddress = VirtualAddress;
        Translation.Cr3 = Context.Cr3;
        Translation.Parts = Parts;
        Translation.Entries = std::move(Entries);
        Translation.PageSize = 0x1000ull;
        Translation.PhysicalAddress =
            Pte.PagePhysicalBase +
            (VirtualAddress & PageOffsetMask4K);

        return Result<X64AddressTranslationResult>::Success(
            Translation);
    }
}