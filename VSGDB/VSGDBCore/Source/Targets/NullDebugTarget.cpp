#include <VSGDBCore/NullComponents.h>

namespace VSGDBCore
{
    class NullDebugTarget final : public IDebugTarget
    {
    public:
        DebugError Connect(
            const DebugTargetConfig& Config) override
        {
            (void)Config;

            return DebugError::Failure(
                ErrorCode::NotSupported,
                L"Debug target is not available.");
        }

        DebugError Disconnect() override
        {
            return DebugError::Success();
        }

        DebugError Break() override
        {
            return DebugError::Failure(
                ErrorCode::NotSupported,
                L"Debug target is not available.");
        }

        DebugError Continue(U32 CpuId) override
        {
            return DebugError::Failure(
                ErrorCode::NotSupported,
                L"Debug target is not available.");
        }

        DebugError ContinueAll() override
        {
            return DebugError::Failure(
                ErrorCode::NotSupported,
                L"Debug target is not available.");
        }

        DebugError StepInto(U32 CpuId) override
        {
            return DebugError::Failure(
                ErrorCode::NotSupported,
                L"Debug target is not available.");
        }

        Result<DebugEvent> WaitForEvent(
            U32 TimeoutMilliseconds) override
        {
            (void)TimeoutMilliseconds;

            return Result<DebugEvent>::Failure(
                DebugError::Failure(
                    ErrorCode::NotSupported,
                    L"Debug target is not available."));
        }

        Result<RegisterContext> GetRegisters(
            U32 CpuId) override
        {
            (void)CpuId;

            return Result<RegisterContext>::Failure(
                DebugError::Failure(
                    ErrorCode::NotSupported,
                    L"Debug target is not available."));
        }

        DebugError SetRegister(
            U32 CpuId,
            const std::wstring& Name,
            U64 Value) override
        {
            (void)CpuId;
            (void)Name;
            (void)Value;

            return DebugError::Failure(
                ErrorCode::NotSupported,
                L"Debug target is not available.");
        }

        Result<ByteVector> ReadVirtualMemory(
            U32 CpuId,
            U64 Address,
            U32 Size) override
        {
            (void)CpuId;
            (void)Address;
            (void)Size;

            return Result<ByteVector>::Failure(
                DebugError::Failure(
                    ErrorCode::NotSupported,
                    L"Debug target is not available."));
        }

        DebugError WriteVirtualMemory(
            U32 CpuId,
            U64 Address,
            const ByteVector& Bytes) override
        {
            (void)CpuId;
            (void)Address;
            (void)Bytes;

            return DebugError::Failure(
                ErrorCode::NotSupported,
                L"Debug target is not available.");
        }

        Result<ByteVector> ReadPhysicalMemory(
            U64 Address,
            U32 Size) override
        {
            (void)Address;
            (void)Size;

            return Result<ByteVector>::Failure(
                DebugError::Failure(
                    ErrorCode::NotSupported,
                    L"Debug target is not available."));
        }

        DebugError WritePhysicalMemory(
            U64 Address,
            const ByteVector& Bytes) override
        {
            (void)Address;
            (void)Bytes;

            return DebugError::Failure(
                ErrorCode::NotSupported,
                L"Debug target is not available.");
        }

        Result<BreakpointId> SetBreakpoint(
            BreakpointKind Kind,
            U64 Address,
            U32 Size) override
        {
            (void)Kind;
            (void)Address;
            (void)Size;

            return Result<BreakpointId>::Failure(
                DebugError::Failure(
                    ErrorCode::NotSupported,
                    L"Debug target is not available."));
        }

        DebugError DeleteBreakpoint(
            BreakpointId Id) override
        {
            (void)Id;

            return DebugError::Failure(
                ErrorCode::NotSupported,
                L"Debug target is not available.");
        }

        Result<std::vector<DebugThreadInfo>>
            EnumerateThreads() override
        {
            return Result<std::vector<DebugThreadInfo>>::Failure(
                DebugError::Failure(
                    ErrorCode::NotSupported,
                    L"Debug target is not available."));
        }
    };

    std::unique_ptr<IDebugTarget>
    CreateNullDebugTarget()
    {
        return std::make_unique<NullDebugTarget>();
    }
}