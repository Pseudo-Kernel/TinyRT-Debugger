#pragma once

#include "Types.h"

namespace VSGDBCore
{
    using BreakpointId = U64;

    enum class BreakpointKind
    {
        Software,
        HardwareExecute,
        HardwareWrite,
        HardwareAccess
    };

    struct BreakpointInfo
    {
        BreakpointId Id = 0;
        BreakpointKind Kind = BreakpointKind::Software;
        U64 Address = 0;
        U32 Size = 1;
        bool Enabled = true;
    };
}
