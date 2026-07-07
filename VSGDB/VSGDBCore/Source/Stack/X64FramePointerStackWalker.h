#pragma once

#include <VSGDBCore/IStackWalker.h>

namespace VSGDBCore
{
    class X64FramePointerStackWalker final : public IStackWalker
    {
    public:
        Expected<std::vector<StackFrame>> WalkStack(
            IDebugTarget& Target,
            U32 CpuId,
            const StackWalkOptions& Options) override;

    private:
        static bool IsCanonicalAddress(U64 Address);
        static bool IsReasonableNextFramePointer(
            U64 CurrentFramePointer,
            U64 NextFramePointer,
            U64 MaxFrameDistance);
    };
}