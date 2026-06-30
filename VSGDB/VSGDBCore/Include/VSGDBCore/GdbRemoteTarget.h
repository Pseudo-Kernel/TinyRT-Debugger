#pragma once

#include <unordered_map>
#include <set>

#include "IDebugTarget.h"
#include "GdbRemoteConnection.h"
#include "RegisterTypes.h"

namespace VSGDBCore
{
    class GdbRemoteTarget final : public IDebugTarget
    {
    public:
        DebugError Connect(
            const DebugTargetConfig& Config) override;

        DebugError Disconnect() override;

        DebugError Break() override;

        DebugError Continue(U32 CpuId) override;

        DebugError ContinueAll() override;

        DebugError StepInto(U32 CpuId) override;

        Result<DebugEvent> WaitForEvent(
            U32 TimeoutMilliseconds) override;

        Result<RegisterContext> GetRegisters(
            U32 CpuId) override;

        DebugError SetRegister(
            U32 CpuId,
            const std::wstring& Name,
            U64 Value) override;

        Result<ByteVector> ReadVirtualMemory(
            U32 CpuId,
            U64 Address,
            U32 Size) override;

        DebugError WriteVirtualMemory(
            U32 CpuId,
            U64 Address,
            const ByteVector& Bytes) override;

        Result<ByteVector> ReadPhysicalMemory(
            U64 Address,
            U32 Size) override;

        DebugError WritePhysicalMemory(
            U64 Address,
            const ByteVector& Bytes) override;

        Result<BreakpointId> SetBreakpoint(
            BreakpointKind Kind,
            U64 Address,
            U32 Size) override;

        DebugError DeleteBreakpoint(
            BreakpointId Id) override;

        Result<std::vector<DebugThreadInfo>> EnumerateThreads() override;

    public:
        DebugError
            SelectRegisterThreadForTest(
                const std::string& ThreadId);

    private:
        Result<DebugEvent> DecodeStopReply(
            const std::string& Reply) const;

        DebugError SelectRegisterThread(
            const std::string& ThreadId);

        DebugError SelectContinueThread(
            const std::string& ThreadId);

        Result<std::string> GetRemoteThreadIdFromCpuId(
            U32 CpuId);

    private:
        Result<std::string> ReadTargetXml();

        Result<std::string> ReadTargetDescriptionFile(
            const std::string& FileName);

        Result<RegisterDescriptorSet> ReadTargetDescriptionTree(
            const std::string& FileName,
            std::set<std::string>& VisitedFiles);

        DebugError EnsureTargetDescriptionLoaded();

        Result<U64> ReadRegisterByName(
            U32 CpuId,
            const std::string& Name);

    private:
        GdbRemoteConnection Connection;
        DebugTargetConfig Config{};

        DebugEvent LastEvent{};
        bool HasLastEvent = false;

        BreakpointId NextBreakpointId = 1;
        std::unordered_map<BreakpointId, BreakpointInfo> Breakpoints;

        std::vector<DebugThreadInfo> Threads;

        RegisterDescriptorSet RegisterDescriptors{};
        bool RegisterDescriptorsLoaded = false;
    };
}
