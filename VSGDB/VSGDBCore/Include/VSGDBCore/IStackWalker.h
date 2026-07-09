#pragma once

#include "Result.h"
#include "StackTypes.h"

#include <vector>

namespace VSGDBCore
{
    class IDebugSession;

    class IStackWalker
    {
    public:
        virtual ~IStackWalker() = default;

        virtual Expected<std::vector<StackFrame>> WalkStack(
            IDebugSession& Session,
            U32 CpuId,
            const StackWalkOptions& Options) = 0;
    };
}