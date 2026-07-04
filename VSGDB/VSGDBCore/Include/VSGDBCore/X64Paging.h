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

    Expected<X64AddressTranslationResult> TranslateX64VirtualAddress(
        IDebugTarget& Target,
        const X64PagingContext& Context,
        U64 VirtualAddress);


    enum class X64PageFaultAccessKind
    {
        Read,
        Write,
        Execute
    };

    struct X64PageFaultErrorCode
    {
        U64 Value = 0;

        bool Present = false;              // bit 0: 0 = not-present, 1 = protection violation
        bool Write = false;                // bit 1: 1 = write, 0 = read
        bool User = false;                 // bit 2: 1 = user, 0 = supervisor
        bool ReservedBit = false;          // bit 3
        bool InstructionFetch = false;     // bit 4
        bool ProtectionKey = false;        // bit 5
        bool ShadowStack = false;          // bit 6
        bool Sgx = false;                  // bit 15
    };

    struct X64PageFaultAnalysis
    {
        X64PageFaultErrorCode ErrorCode{};

        X64PageFaultAccessKind AccessKind =
            X64PageFaultAccessKind::Read;

        bool TranslationSucceeded = false;
        bool NotPresentFault = false;
        bool ProtectionViolation = false;

        bool CausedByNonCanonicalAddress = false;
        bool CausedByNotPresentEntry = false;
        bool CausedByWriteToReadOnlyPage = false;
        bool CausedByUserAccessToSupervisorPage = false;
        bool CausedByExecuteOnNxPage = false;
        bool CausedByReservedBit = false;

        std::wstring Summary;
    };

    X64PageFaultErrorCode DecodeX64PageFaultErrorCode(
        U64 ErrorCode);

    X64PageFaultAnalysis AnalyzeX64PageFault(
        const X64PagingContext& Context,
        const X64AddressTranslationResult& Translation,
        U64 ErrorCode);

    bool IsX64SupervisorWriteProtectEnabled(
        const X64PagingContext& Context);
}