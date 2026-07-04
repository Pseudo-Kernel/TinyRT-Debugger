#include <VSGDBCore/NullComponents.h>

namespace VSGDBCore
{
    class NullAssembler final : public IAssembler
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
                L"Assembler is not available.");
        }

        Expected<ByteVector> Assemble(
            U64 Address,
            const std::wstring& Text) override
        {
            (void)Address;
            (void)Text;

            return Expected<ByteVector>::Failure(
                DebugError::Failure(
                    ErrorCode::NotSupported,
                    L"Assembler is not available."));
        }
    };

    std::unique_ptr<IAssembler>
        CreateNullAssembler()
    {
        return std::make_unique<NullAssembler>();
    }
}