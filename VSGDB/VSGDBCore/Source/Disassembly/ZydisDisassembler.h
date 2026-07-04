#pragma once

#include <VSGDBCore/IDisassembler.h>

#include <zydis.h>

namespace VSGDBCore
{
    class ZydisDisassembler final : public IDisassembler
    {
    public:
        ZydisDisassembler();

        ProcessorArchitecture GetArchitecture() const override;

        DebugError SetArchitecture(
            ProcessorArchitecture Architecture) override;

        Expected<std::vector<DisassembledInstruction>> Disassemble(
            U64 Address,
            const ByteVector& Bytes,
            U32 MaxInstructionCount) override;

    private:
        ProcessorArchitecture Architecture =
            ProcessorArchitecture::Unknown;

        ZydisDecoder Decoder{};
        ZydisFormatter Formatter{};
    };
}