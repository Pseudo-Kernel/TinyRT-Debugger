
#include <VSGDBCore/IDebugSession.h>
#include <VSGDBCore/IDisassembler.h>
#include <VSGDBCore/IDebugTarget.h>
#include <VSGDBCore/IExpressionEvaluator.h>
#include <VSGDBCore/IModuleManager.h>
#include <VSGDBCore/ISymbolManager.h>
#include <VSGDBCore/IStackWalker.h>

#include <memory>
#include <mutex>

namespace VSGDBCore
{
    class DebugSession final : public IDebugSession
    {
    public:
        DebugSession();

        DebugSessionState GetState() const override;

        DebugError Connect(
            const DebugSessionConnectOptions& Options) override;

        DebugError Disconnect() override;

        DebugError BreakExecution() override;
        DebugError Continue(U32 CpuId) override;
        DebugError ContinueAll() override;
        DebugError StepInto(U32 CpuId) override;

        Expected<DebugEvent> WaitForEvent(
            U32 TimeoutMilliseconds) override;

        Expected<std::vector<DebugThreadInfo>> EnumerateThreads() override;

        Expected<RegisterContext> GetRegisters(U32 CpuId) override;

        Expected<ByteVector> ReadVirtualMemory(
            U32 CpuId,
            U64 Address,
            U32 Size) override;

        Expected<ByteVector> ReadPhysicalMemory(
            U64 Address,
            U32 Size) override;


        // Breakpoints

        Expected<DebugEvent> GetLastEvent() const override;

        Expected<BreakpointInfo> SetBreakpoint(
            BreakpointKind Kind,
            U64 Address,
            U32 Size) override;

        DebugError DeleteBreakpoint(
            BreakpointId Id) override;

        DebugError DeleteBreakpointByAddress(
            BreakpointKind Kind,
            U64 Address,
            U32 Size) override;

        Expected<std::vector<BreakpointInfo>>
            EnumerateBreakpoints() const override;

        DebugError DeleteAllBreakpoints() override;

    private:
        Expected<bool> StepOverCurrentSoftwareBreakpointIfNeeded(U32 CpuId);

    private:
        DebugError RequireConnected() const;
        DebugError RequireStopped() const;
        DebugError RequireCanWaitForEvent() const;
        DebugError RequireStoppedLocked() const;
        DebugError RequireCanWaitForEventLocked() const;

        mutable std::mutex Lock;
        DebugSessionState State = DebugSessionState::Disconnected;

        std::unique_ptr<IDebugTarget> Target;
        std::unique_ptr<IDisassembler> Disassembler;
        std::unique_ptr<IModuleManager> ModuleManager;
        std::unique_ptr<ISymbolManager> SymbolManager;
        std::unique_ptr<IStackWalker> StackWalker;
    };
}