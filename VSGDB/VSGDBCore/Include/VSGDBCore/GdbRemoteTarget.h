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

        Expected<DebugEvent> WaitForEvent(
            U32 TimeoutMilliseconds) override;

        Expected<RegisterContext> GetRegisters(
            U32 CpuId) override;

        DebugError SetRegister(
            U32 CpuId,
            const std::wstring& Name,
            U64 Value) override;

        Expected<ByteVector> ReadVirtualMemory(
            U32 CpuId,
            U64 Address,
            U32 Size) override;

        Expected<ByteVector> ReadVirtualMemorySingle(
            U32 CpuId,
            U64 Address,
            U32 Size);

        DebugError WriteVirtualMemory(
            U32 CpuId,
            U64 Address,
            const ByteVector& Bytes) override;

        Expected<ByteVector> ReadPhysicalMemory(
            U64 Address,
            U32 Size) override;

        DebugError WritePhysicalMemory(
            U64 Address,
            const ByteVector& Bytes) override;

        Expected<BreakpointInfo> SetBreakpoint(
            BreakpointKind Kind,
            U64 Address,
            U32 Size) override;

        DebugError DeleteBreakpoint(
            BreakpointId Id) override;

        Expected<std::vector<BreakpointInfo>>
            EnumerateBreakpoints() const override;

        DebugError DisableBreakpointInTarget(
            BreakpointId Id) override;

        DebugError EnableBreakpointInTarget(
            BreakpointId Id) override;

    private:
        DebugError DeleteBreakpointByAddress(
            BreakpointKind Kind,
            U64 Address,
            U32 Size);

        DebugError InsertRemoteBreakpoint(
            BreakpointKind Kind,
            U64 Address,
            U32 Size);

        DebugError DeleteRemoteBreakpoint(
            BreakpointKind Kind,
            U64 Address,
            U32 Size);

        DebugError DeleteAllBreakpoints();

        Expected<std::vector<DebugThreadInfo>> EnumerateThreads() override;

        Expected<DebugEvent> GetLastEvent() const;

        DebugError BreakExecution();

    private:
        Expected<DebugEvent> DecodeStopReply(
            const std::string& Reply) const;

        DebugError SelectRegisterThread(
            const std::string& ThreadId);

        DebugError SelectContinueThread(
            const std::string& ThreadId);

        Expected<std::string> GetRemoteThreadIdFromCpuId(
            U32 CpuId);

    private:
        Expected<std::string> ReadTargetXml();

        Expected<std::string> ReadTargetDescriptionFile(
            const std::string& FileName);

        Expected<RegisterDescriptorSet> ReadTargetDescriptionTree(
            const std::string& FileName,
            std::set<std::string>& VisitedFiles);

        DebugError EnsureTargetDescriptionLoaded();

        Expected<U64> ReadRegisterByName(
            U32 CpuId,
            const std::string& Name);

    private:
        DebugError SetQemuPhysicalMemoryMode(
            bool Enabled);

        Expected<ByteVector> ReadMemoryUsingGdbM(
            U64 Address,
            U32 Size);

    private:
        GdbRemoteConnection Connection_;
        DebugTargetConfig Config_{};

        DebugEvent LastEvent_{};
        bool HasLastEvent_ = false;

        BreakpointId NextBreakpointId_ = 1;
        std::unordered_map<BreakpointId, BreakpointInfo> Breakpoints_;

        std::vector<DebugThreadInfo> Threads_;

        RegisterDescriptorSet RegisterDescriptors_{};
        bool RegisterDescriptorsLoaded_ = false;
    };
}
