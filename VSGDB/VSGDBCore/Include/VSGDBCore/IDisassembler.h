#pragma once

#include "Types.h"
#include "Result.h"

namespace VSGDBCore
{
    enum class ProcessorArchitecture
    {
        Unknown,
        X64
    };

    struct DisassembledInstruction
    {
        U64 Address = 0;
        U32 Size = 0;

        ByteVector Bytes;

        std::wstring Mnemonic;
        std::wstring Operands;
        std::wstring Text;
    };

    class IDisassembler
    {
    public:
        virtual ~IDisassembler() = default;

        virtual ProcessorArchitecture GetArchitecture() const = 0;

        virtual DebugError SetArchitecture(
            ProcessorArchitecture Architecture) = 0;

        virtual Expected<std::vector<DisassembledInstruction>> Disassemble(
            U64 Address,
            const ByteVector& Bytes,
            U32 MaxInstructionCount) = 0;
    };
}
