#include "PageFaultPrinter.h"

#include <cstdio>

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

void
PrintPageFaultAnalysis(
    const VSGDBCore::X64PageFaultAnalysis& Analysis)
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
