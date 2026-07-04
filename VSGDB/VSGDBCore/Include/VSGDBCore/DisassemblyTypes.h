#pragma once

#include "Types.h"

#include <string>
#include <vector>

namespace VSGDBCore
{
    struct InstructionInfo
    {
        U64 Address = 0;
        ByteVector Bytes;
        std::string Text;
        U8 Length = 0;
        bool Decoded = false;
    };

    using InstructionList = std::vector<InstructionInfo>;
}
