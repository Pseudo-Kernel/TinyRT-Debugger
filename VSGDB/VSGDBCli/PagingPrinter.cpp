#include "PagingPrinter.h"

#include <cstdio>

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

void
PrintTranslation(
    const VSGDBCore::X64AddressTranslationResult& Translation)
{
    std::printf(
        "VA  = 0x%016llx\n",
        Translation.VirtualAddress);

    std::printf(
        "CR3 = 0x%016llx\n",
        Translation.Cr3);

    std::printf(
        "IDX = PML4[%03x] PDPT[%03x] PD[%03x] PT[%03x] OFF[%03x]\n",
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