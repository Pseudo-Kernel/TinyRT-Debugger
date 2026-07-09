#pragma once

#include "Types.h"
#include "Result.h"

#include "DebugSessionTypes.h"
#include "IDebugTarget.h"

namespace VSGDBCore
{
    struct DebugSessionConnectOptions
    {
        std::wstring Host = L"localhost";
        U16 Port = 1234;
    };

    class IDebugSession
    {
    public:
        virtual ~IDebugSession() = default;

        virtual DebugSessionState GetState() const = 0;

        virtual DebugError Connect(
            const DebugSessionConnectOptions& Options) = 0;

        virtual DebugError Disconnect() = 0;

        virtual DebugError BreakExecution() = 0;
        virtual DebugError Continue(U32 CpuId) = 0;
        virtual DebugError ContinueAll() = 0;
        virtual DebugError StepInto(U32 CpuId) = 0;

        virtual Expected<DebugEvent> WaitForEvent(
            U32 TimeoutMilliseconds) = 0;

        virtual Expected<std::vector<DebugThreadInfo>> EnumerateThreads() = 0;

        virtual Expected<RegisterContext> GetRegisters(U32 CpuId) = 0;

        virtual Expected<ByteVector> ReadVirtualMemory(
            U32 CpuId,
            U64 Address,
            U32 Size) = 0;

        virtual Expected<ByteVector> ReadPhysicalMemory(
            U64 Address,
            U32 Size) = 0;

        // Breakpoints

        virtual Expected<DebugEvent> GetLastEvent() const = 0;

        virtual Expected<BreakpointId> SetBreakpoint(
            BreakpointKind Kind,
            U64 Address,
            U32 Size) = 0;

        virtual DebugError DeleteBreakpoint(
            BreakpointId Id) = 0;

        virtual DebugError DeleteBreakpointByAddress(
            BreakpointKind Kind,
            U64 Address,
            U32 Size) = 0;

        virtual DebugError RemoveBreakpointFromTarget(
            BreakpointId Id) = 0;

        virtual DebugError InsertBreakpointIntoTarget(
            BreakpointId Id) = 0;

        virtual DebugError DeleteAllBreakpoints() = 0;
    };
}
