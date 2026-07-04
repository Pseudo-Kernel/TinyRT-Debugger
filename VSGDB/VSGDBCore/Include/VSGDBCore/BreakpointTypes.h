#pragma once

#include "Types.h"

namespace VSGDBCore
{
    using BreakpointId = U32;

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

        //
        // User-visible logical state.
        // If false, the breakpoint should not trigger and should appear disabled.
        //
        bool Enabled = true;

        //
        // Backend insertion state.
        // If true, the breakpoint is currently installed in the remote target.
        //
        bool Inserted = true;
    };
}
