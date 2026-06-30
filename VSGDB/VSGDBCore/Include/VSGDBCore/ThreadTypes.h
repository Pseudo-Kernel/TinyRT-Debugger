#pragma once

#include "Types.h"

#include <string>

namespace VSGDBCore
{
    struct DebugThreadInfo
    {
        U32 CpuId = 0;

        //
        // Raw GDB remote thread ID.
        // Examples:
        //   "p01.01"
        //   "p01.02"
        //
        std::string RemoteThreadId;

        std::wstring DisplayName;
    };
}
