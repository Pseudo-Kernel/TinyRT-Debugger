#include <VSGDBCore/NullComponents.h>

namespace VSGDBCore
{
    class NullDisassembler final : public IDisassembler
    {
    public:
        ProcessorArchitecture GetArchitecture() const override
        {
            return ProcessorArchitecture::Unknown;
        }

        DebugError SetArchitecture(
            ProcessorArchitecture Architecture) override
        {
            (void)Architecture;

            return DebugError::Failure(
                ErrorCode::NotSupported,
                L"Disassembler is not available.");
        }

        Expected<std::vector<DisassembledInstruction>> Disassemble(
            U64 Address,
            const ByteVector& Bytes,
            U32 MaxInstructionCount) override
        {
            (void)Address;
            (void)Bytes;
            (void)MaxInstructionCount;

            return Expected<std::vector<DisassembledInstruction>>::Failure(
                DebugError::Failure(
                    ErrorCode::NotSupported,
                    L"Disassembler is not available."));
        }
    };

    std::unique_ptr<IDisassembler>
        CreateNullDisassembler()
    {
        return std::make_unique<NullDisassembler>();
    }
}