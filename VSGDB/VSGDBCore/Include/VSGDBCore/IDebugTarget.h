#pragma once

#include "Types.h"
#include "Result.h"
#include "RegisterContext.h"
#include "MemoryTypes.h"
#include "BreakpointTypes.h"
#include "ThreadTypes.h"

namespace VSGDBCore
{
    enum class DebugStopReason
    {
        None,
        Breakpoint,
        SingleStep,
        Signal,
        ManualBreak,
        Exited,
        Unknown
    };

    struct DebugEvent
    {
        DebugStopReason StopReason = DebugStopReason::None;
        U32 CpuId = 0;
        U64 Address = 0;
        U32 Signal = 0;

        std::string RemoteThreadId;     // e.g. "p01.02"
        std::wstring Description;
    };

    struct DebugTargetConfig
    {
        std::wstring Host = L"127.0.0.1";
        U16 Port = 1234;
        U32 DefaultCpuId = 0;
        bool UseQemuPhysicalMemoryMode = false;
    };

    class IDebugTarget
    {
    public:
        virtual ~IDebugTarget() = default;

        virtual DebugError Connect(
            const DebugTargetConfig& Config) = 0;

        virtual DebugError Disconnect() = 0;

        virtual DebugError Break() = 0;

        virtual DebugError Continue(
            U32 CpuId) = 0;

        virtual DebugError ContinueAll() = 0;

        virtual DebugError StepInto(
            U32 CpuId) = 0;

        virtual DebugError StepOver()
        {
            return DebugError::Failure(
                ErrorCode::NotSupported,
                L"StepOver is not implemented.");
        }

        virtual DebugError StepOut()
        {
            return DebugError::Failure(
                ErrorCode::NotSupported,
                L"StepOut is not implemented.");
        }

        virtual Result<DebugEvent> WaitForEvent(
            U32 TimeoutMilliseconds) = 0;

        virtual Result<RegisterContext> GetRegisters(
            U32 CpuId) = 0;

        virtual DebugError SetRegister(
            U32 CpuId,
            const std::wstring& Name,
            U64 Value) = 0;

        virtual Result<ByteVector> ReadVirtualMemory(
            U32 CpuId,
            U64 Address,
            U32 Size) = 0;

        virtual DebugError WriteVirtualMemory(
            U32 CpuId,
            U64 Address,
            const ByteVector& Bytes) = 0;

        virtual Result<ByteVector> ReadPhysicalMemory(
            U64 Address,
            U32 Size) = 0;

        virtual DebugError WritePhysicalMemory(
            U64 Address,
            const ByteVector& Bytes) = 0;

        virtual Result<BreakpointId> SetBreakpoint(
            BreakpointKind Kind,
            U64 Address,
            U32 Size) = 0;

        virtual DebugError DeleteBreakpoint(
            BreakpointId Id) = 0;

        virtual Result<std::vector<DebugThreadInfo>> EnumerateThreads() = 0;
    };
}