#pragma once

#include "Types.h"
#include "Result.h"
#include "IDisassembler.h"

namespace VSGDBCore
{
    class IAssembler
    {
    public:
        virtual ~IAssembler() = default;

        virtual ProcessorArchitecture GetArchitecture() const = 0;

        virtual DebugError SetArchitecture(
            ProcessorArchitecture Architecture) = 0;

        virtual Expected<ByteVector> Assemble(
            U64 Address,
            const std::wstring& Text) = 0;
    };
}
