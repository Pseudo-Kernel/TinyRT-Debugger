#pragma once

#include "Types.h"

namespace VSGDBCore
{
    enum class MemorySpace
    {
        Virtual,
        Physical
    };

    struct MemoryRange
    {
        MemorySpace Space = MemorySpace::Virtual;
        U32 CpuId = 0;
        U64 Address = 0;
        U32 Size = 0;
    };
}
