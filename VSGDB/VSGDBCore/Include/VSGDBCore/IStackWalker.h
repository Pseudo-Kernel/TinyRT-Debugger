#pragma once

#include "Result.h"
#include "StackTypes.h"

#include <vector>

namespace VSGDBCore
{
    class IDebugTarget;

    class IStackWalker
    {
    public:
        virtual ~IStackWalker() = default;

        virtual Expected<std::vector<StackFrame>> WalkStack(
            IDebugTarget& Target,
            U32 CpuId,
            const StackWalkOptions& Options) = 0;
    };
}