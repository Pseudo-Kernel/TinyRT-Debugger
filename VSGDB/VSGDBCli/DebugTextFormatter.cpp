#include "DebugTextFormatter.h"

#include <cwctype>

static const char*
AccessKindName(
    VSGDBCore::X64PageFaultAccessKind Kind)
{
    switch (Kind)
    {
    case VSGDBCore::X64PageFaultAccessKind::Read:
        return "read";

    case VSGDBCore::X64PageFaultAccessKind::Write:
        return "write";

    case VSGDBCore::X64PageFaultAccessKind::Execute:
        return "execute";

    default:
        return "unknown";
    }
}

static const char*
PagingLevelName(
    VSGDBCore::X64PagingLevel Level)
{
    switch (Level)
    {
    case VSGDBCore::X64PagingLevel::Pml4:
        return "PML4";

    case VSGDBCore::X64PagingLevel::Pdpt:
        return "PDPT";

    case VSGDBCore::X64PagingLevel::Pd:
        return "PD  ";

    case VSGDBCore::X64PagingLevel::Pt:
        return "PT  ";

    default:
        return "????";
    }
}

static void
PrintPageEntry(
    const VSGDBCore::X64PageEntry& Entry)
{
    std::printf(
        "%s @ 0x%016llx = 0x%016llx  %c %c %c %c NX=%u LP=%u\n",
        PagingLevelName(Entry.Level),
        Entry.EntryPhysicalAddress,
        Entry.Value,
        Entry.Present ? 'P' : '-',
        Entry.Write ? 'W' : 'R',
        Entry.User ? 'U' : 'S',
        Entry.Global ? 'G' : '-',
        Entry.NoExecute ? 1 : 0,
        Entry.LargePage ? 1 : 0);
}


DebugTextFormatter::DebugTextFormatter(
    const VSGDBCore::IModuleManager* ModuleManager, 
    const VSGDBCore::ISymbolManager* SymbolManager) :
    ModuleManager_(ModuleManager),
    SymbolManager_(SymbolManager)
{
}

AddressLabel
DebugTextFormatter::FormatAddressLabel(
    VSGDBCore::U64 Address) const
{
    AddressLabel SymbolLabel =
        FormatAddressWithSymbol(Address);

    if (!SymbolLabel.Text.empty())
    {
        return SymbolLabel;
    }

    return FormatAddressWithModule(Address);
}

std::wstring
DebugTextFormatter::FormatAddressInline(
    VSGDBCore::U64 Address) const
{
    wchar_t AddressBuffer[32] = {};
    std::swprintf(
        AddressBuffer,
        std::size(AddressBuffer),
        L"0x%016llx",
        Address);

    AddressLabel Label = FormatAddressLabel(Address);

    if (Label.Text.empty())
    {
        return AddressBuffer;
    }

    std::wstring Text = AddressBuffer;
    Text += L" <";
    Text += Label.Text;
    Text += L">";

    return Text;
}

void
DebugTextFormatter::PrintRegister(
    const wchar_t* Name,
    VSGDBCore::U64 Value) const
{
    if (IsSymbolAwareRegister(Name))
    {
        std::wprintf(
            L"%-12s = %s\n",
            Name,
            FormatAddressInline(Value).c_str());
    }
    else
    {
        PrintRegisterRaw(Name, Value);
    }
}

void
DebugTextFormatter::PrintRegisterRaw(
    const wchar_t* Name,
    VSGDBCore::U64 Value) const
{
    std::wprintf(
        L"%-12s = 0x%016llx\n",
        Name,
        Value);
}

void
DebugTextFormatter::PrintRegisters(
    const VSGDBCore::RegisterContext& Registers) const
{
    PrintRegister(L"RAX", Registers.Rax);
    PrintRegister(L"RBX", Registers.Rbx);
    PrintRegister(L"RCX", Registers.Rcx);
    PrintRegister(L"RDX", Registers.Rdx);
    PrintRegister(L"RSI", Registers.Rsi);
    PrintRegister(L"RDI", Registers.Rdi);
    PrintRegister(L"RBP", Registers.Rbp);
    PrintRegister(L"RSP", Registers.Rsp);

    PrintRegister(L"R8", Registers.R8);
    PrintRegister(L"R9", Registers.R9);
    PrintRegister(L"R10", Registers.R10);
    PrintRegister(L"R11", Registers.R11);
    PrintRegister(L"R12", Registers.R12);
    PrintRegister(L"R13", Registers.R13);
    PrintRegister(L"R14", Registers.R14);
    PrintRegister(L"R15", Registers.R15);

    PrintRegister(L"RIP", Registers.Rip);

    PrintRegisterRaw(L"RFLAGS", Registers.Rflags);

    PrintRegisterRaw(L"CS", Registers.Cs);
    PrintRegisterRaw(L"SS", Registers.Ss);
    PrintRegisterRaw(L"DS", Registers.Ds);
    PrintRegisterRaw(L"ES", Registers.Es);
    PrintRegisterRaw(L"FS", Registers.Fs);
    PrintRegisterRaw(L"GS", Registers.Gs);

    PrintRegisterRaw(L"FS_BASE", Registers.FsBase);
    PrintRegisterRaw(L"GS_BASE", Registers.GsBase);
    PrintRegisterRaw(L"KGS_BASE", Registers.KernelGsBase);

    PrintRegisterRaw(L"CR0", Registers.Cr0);
    PrintRegister(L"CR2", Registers.Cr2);
    PrintRegisterRaw(L"CR3", Registers.Cr3);
    PrintRegisterRaw(L"CR4", Registers.Cr4);
    PrintRegisterRaw(L"CR8", Registers.Cr8);
    PrintRegisterRaw(L"EFER", Registers.Efer);
}

bool
DebugTextFormatter::PrintRegisterByName(
    const VSGDBCore::RegisterContext& Registers,
    const std::wstring& Name) const
{
    const std::wstring Lower = NormalizeName(Name);

    if (Lower == L"rax")
    {
        PrintRegister(L"RAX", Registers.Rax);
        return true;
    }

    if (Lower == L"rbx")
    {
        PrintRegister(L"RBX", Registers.Rbx);
        return true;
    }

    if (Lower == L"rcx")
    {
        PrintRegister(L"RCX", Registers.Rcx);
        return true;
    }

    if (Lower == L"rdx")
    {
        PrintRegister(L"RDX", Registers.Rdx);
        return true;
    }

    if (Lower == L"rsi")
    {
        PrintRegister(L"RSI", Registers.Rsi);
        return true;
    }

    if (Lower == L"rdi")
    {
        PrintRegister(L"RDI", Registers.Rdi);
        return true;
    }

    if (Lower == L"rbp")
    {
        PrintRegister(L"RBP", Registers.Rbp);
        return true;
    }

    if (Lower == L"rsp")
    {
        PrintRegister(L"RSP", Registers.Rsp);
        return true;
    }

    if (Lower == L"r8")
    {
        PrintRegister(L"R8", Registers.R8);
        return true;
    }

    if (Lower == L"r9")
    {
        PrintRegister(L"R9", Registers.R9);
        return true;
    }

    if (Lower == L"r10")
    {
        PrintRegister(L"R10", Registers.R10);
        return true;
    }

    if (Lower == L"r11")
    {
        PrintRegister(L"R11", Registers.R11);
        return true;
    }

    if (Lower == L"r12")
    {
        PrintRegister(L"R12", Registers.R12);
        return true;
    }

    if (Lower == L"r13")
    {
        PrintRegister(L"R13", Registers.R13);
        return true;
    }

    if (Lower == L"r14")
    {
        PrintRegister(L"R14", Registers.R14);
        return true;
    }

    if (Lower == L"r15")
    {
        PrintRegister(L"R15", Registers.R15);
        return true;
    }

    if (Lower == L"rip")
    {
        PrintRegister(L"RIP", Registers.Rip);
        return true;
    }

    if (Lower == L"rflags")
    {
        PrintRegisterRaw(L"RFLAGS", Registers.Rflags);
        return true;
    }

    if (Lower == L"cs")
    {
        PrintRegisterRaw(L"CS", Registers.Cs);
        return true;
    }

    if (Lower == L"ss")
    {
        PrintRegisterRaw(L"SS", Registers.Ss);
        return true;
    }

    if (Lower == L"ds")
    {
        PrintRegisterRaw(L"DS", Registers.Ds);
        return true;
    }

    if (Lower == L"es")
    {
        PrintRegisterRaw(L"ES", Registers.Es);
        return true;
    }

    if (Lower == L"fs")
    {
        PrintRegisterRaw(L"FS", Registers.Fs);
        return true;
    }

    if (Lower == L"gs")
    {
        PrintRegisterRaw(L"GS", Registers.Gs);
        return true;
    }

    if (Lower == L"fs_base")
    {
        PrintRegisterRaw(L"FS_BASE", Registers.FsBase);
        return true;
    }

    if (Lower == L"gs_base")
    {
        PrintRegisterRaw(L"GS_BASE", Registers.GsBase);
        return true;
    }

    if (Lower == L"kgs_base")
    {
        PrintRegisterRaw(L"KGS_BASE", Registers.KernelGsBase);
        return true;
    }

    if (Lower == L"cr0")
    {
        PrintRegisterRaw(L"CR0", Registers.Cr0);
        return true;
    }

    if (Lower == L"cr2")
    {
        PrintRegister(L"CR2", Registers.Cr2);
        return true;
    }

    if (Lower == L"cr3")
    {
        PrintRegisterRaw(L"CR3", Registers.Cr3);
        return true;
    }

    if (Lower == L"cr4")
    {
        PrintRegisterRaw(L"CR4", Registers.Cr4);
        return true;
    }

    if (Lower == L"cr8")
    {
        PrintRegisterRaw(L"CR8", Registers.Cr8);
        return true;
    }

    if (Lower == L"efer")
    {
        PrintRegisterRaw(L"EFER", Registers.Efer);
        return true;
    }

    return false;
}

std::wstring
DebugTextFormatter::NormalizeName(
    const std::wstring& Name)
{
    std::wstring LowerName = Name;

    for (wchar_t& Character : LowerName)
    {
        Character = static_cast<wchar_t>(
            std::towlower(Character));
    }

    return LowerName;
}

bool
DebugTextFormatter::IsSymbolAwareRegister(
    const std::wstring& Name)
{
    const std::wstring Lower = NormalizeName(Name);

    return Lower == L"rip" ||
        Lower == L"rsp" ||
        Lower == L"rbp" ||
        Lower == L"rax" ||
        Lower == L"rbx" ||
        Lower == L"rcx" ||
        Lower == L"rdx" ||
        Lower == L"rsi" ||
        Lower == L"rdi" ||
        Lower == L"r8"  ||
        Lower == L"r9"  ||
        Lower == L"r10" ||
        Lower == L"r11" ||
        Lower == L"r12" ||
        Lower == L"r13" ||
        Lower == L"r14" ||
        Lower == L"r15" ||
        Lower == L"cr2";
}

AddressLabel
DebugTextFormatter::FormatAddressWithSymbol(
    VSGDBCore::U64 Address) const
{
    AddressLabel Label{};

    if (SymbolManager_ == nullptr)
    {
        return Label;
    }

    auto Symbol = SymbolManager_->GetSymbolByAddress(Address);

    if (!Symbol.HasValue())
    {
        return Label;
    }

    const VSGDBCore::U64 Offset =
        Address - Symbol.Value.Address;

    wchar_t KeyBuffer[128] = {};
    std::swprintf(
        KeyBuffer,
        std::size(KeyBuffer),
        L"symbol:%u:%016llx",
        Symbol.Value.ModuleId,
        Symbol.Value.Address);

    wchar_t TextBuffer[512] = {};

    if (Offset == 0)
    {
        std::swprintf(
            TextBuffer,
            std::size(TextBuffer),
            L"%s",
            Symbol.Value.Name.c_str());
    }
    else
    {
        std::swprintf(
            TextBuffer,
            std::size(TextBuffer),
            L"%s+0x%llx",
            Symbol.Value.Name.c_str(),
            Offset);
    }

    Label.Key = KeyBuffer;
    Label.Text = TextBuffer;

    return Label;
}

AddressLabel
DebugTextFormatter::FormatAddressWithModule(
    VSGDBCore::U64 Address) const
{
    AddressLabel Label{};

    if (ModuleManager_ == nullptr)
    {
        return Label;
    }

    auto Module = ModuleManager_->GetModuleByAddress(Address);

    if (!Module.HasValue())
    {
        return Label;
    }

    const VSGDBCore::U64 Offset =
        Address - Module.Value.BaseAddress;

    wchar_t KeyBuffer[128] = {};
    std::swprintf(
        KeyBuffer,
        std::size(KeyBuffer),
        L"module:%u",
        Module.Value.Id);

    wchar_t TextBuffer[512] = {};
    std::swprintf(
        TextBuffer,
        std::size(TextBuffer),
        L"%s+0x%llx",
        Module.Value.Name.c_str(),
        Offset);

    Label.Key = KeyBuffer;
    Label.Text = TextBuffer;

    return Label;
}

void
DebugTextFormatter::PrintTranslation(
    const VSGDBCore::X64AddressTranslationResult& Translation) const
{
    std::wprintf(
        L"VA  = %s\n",
        FormatAddressInline(Translation.VirtualAddress).c_str());

    std::wprintf(
        L"CR3 = 0x%016llx\n",
        Translation.Cr3);

    std::wprintf(
        L"IDX = PML4[%03x] PDPT[%03x] PD[%03x] PT[%03x] OFF[%03x]\n",
        Translation.Parts.Pml4Index,
        Translation.Parts.PdptIndex,
        Translation.Parts.PdIndex,
        Translation.Parts.PtIndex,
        Translation.Parts.PageOffset);

    for (const auto& Entry : Translation.Entries)
    {
        PrintPageEntry(Entry);
    }

    if (Translation.Success)
    {
        std::printf(
            "PA  = 0x%016llx\n",
            Translation.PhysicalAddress);

        std::printf(
            "PageSize = 0x%llx\n",
            Translation.PageSize);
    }
    else
    {
        std::wprintf(
            L"Translation failed: %s\n",
            Translation.FailureMessage.c_str());
    }
}

void
DebugTextFormatter::PrintPageFaultAnalysis(
    const VSGDBCore::X64PageFaultAnalysis& Analysis) const
{
    const auto& Ec = Analysis.ErrorCode;

    std::printf(
        "PFEC = 0x%016llx  P=%u W=%u U=%u RSVD=%u IFetch=%u PK=%u SS=%u SGX=%u\n",
        Ec.Value,
        Ec.Present ? 1 : 0,
        Ec.Write ? 1 : 0,
        Ec.User ? 1 : 0,
        Ec.ReservedBit ? 1 : 0,
        Ec.InstructionFetch ? 1 : 0,
        Ec.ProtectionKey ? 1 : 0,
        Ec.ShadowStack ? 1 : 0,
        Ec.Sgx ? 1 : 0);

    std::printf(
        "Access = %s\n",
        AccessKindName(Analysis.AccessKind));

    std::wprintf(
        L"Analysis = %s\n",
        Analysis.Summary.c_str());
}
