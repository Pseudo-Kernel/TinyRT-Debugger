#pragma once

#include "Types.h"
#include "Result.h"
#include "IDebugTarget.h"

#include <string>
#include <vector>

namespace VSGDBCore
{
    enum class X64PagingLevel
    {
        Pml4,
        Pdpt,
        Pd,
        Pt
    };

    enum class X64PagingFailureReason
    {
        None,

        NonCanonicalAddress,
        UnsupportedPagingMode,

        Pml4eNotPresent,
        PdpteNotPresent,
        PdeNotPresent,
        PteNotPresent,

        PhysicalReadFailed
    };

    struct X64VirtualAddressParts
    {
        U16 Pml4Index = 0;
        U16 PdptIndex = 0;
        U16 PdIndex = 0;
        U16 PtIndex = 0;
        U16 PageOffset = 0;
    };

    struct X64PageEntry
    {
        X64PagingLevel Level = X64PagingLevel::Pml4;

        U64 EntryPhysicalAddress = 0;
        U64 Value = 0;

        bool Present = false;
        bool Write = false;
        bool User = false;
        bool PageWriteThrough = false;
        bool PageCacheDisable = false;
        bool Accessed = false;
        bool Dirty = false;
        bool LargePage = false;
        bool Global = false;
        bool NoExecute = false;

        U64 NextTablePhysicalBase = 0;
        U64 PagePhysicalBase = 0;
    };

    struct X64PagingContext
    {
        U64 Cr0 = 0;
        U64 Cr3 = 0;
        U64 Cr4 = 0;
        U64 Efer = 0;
    };

    struct X64AddressTranslationResult
    {
        bool Success = false;

        U64 VirtualAddress = 0;
        U64 PhysicalAddress = 0;
        U64 Cr3 = 0;
        U64 PageSize = 0;

        X64VirtualAddressParts Parts{};
        std::vector<X64PageEntry> Entries;

        X64PagingFailureReason FailureReason =
            X64PagingFailureReason::None;

        std::wstring FailureMessage;
    };

    X64VirtualAddressParts DecodeX64VirtualAddress(
        U64 VirtualAddress);

    bool IsCanonicalX64VirtualAddress(
        U64 VirtualAddress);

    X64PageEntry DecodeX64PageEntry(
        X64PagingLevel Level,
        U64 EntryPhysicalAddress,
        U64 EntryValue);

    Result<X64AddressTranslationResult> TranslateX64VirtualAddress(
        IDebugTarget& Target,
        const X64PagingContext& Context,
        U64 VirtualAddress);
}