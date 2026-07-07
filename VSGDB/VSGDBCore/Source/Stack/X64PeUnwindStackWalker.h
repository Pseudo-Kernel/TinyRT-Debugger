#pragma once

#include <VSGDBCore/Result.h>
#include <VSGDBCore/RegisterContext.h>
#include <VSGDBCore/StackTypes.h>
#include <VSGDBCore/X64UnwindTypes.h>
#include <VSGDBCore/IStackWalker.h>
#include <VSGDBCore/IDebugTarget.h>
#include <VSGDBCore/IModuleManager.h>

#include "X64PeUnwindData.h"

namespace VSGDBCore
{
    class X64PeUnwindStackWalker final : public IStackWalker
    {
    public:
        X64PeUnwindStackWalker(
            const IModuleManager* ModuleManager);

        Expected<X64UnwindResult>
            UnwindLeafFrame(
                IDebugTarget& Target,
                U32 CpuId,
                const RegisterContext& Context) const;

        Expected<X64UnwindResult>
            UnwindFrame(
                IDebugTarget& Target,
                U32 CpuId,
                const RegisterContext& Context);

        Expected<X64UnwindResult>
            ApplyUnwindInfo(
                IDebugTarget& Target,
                U32 CpuId,
                const RegisterContext& Context,
                const X64UnwindInfo& UnwindInfo) const;

        Expected<std::vector<StackFrame>>
            WalkStack(
                IDebugTarget& Target,
                U32 CpuId,
                const StackWalkOptions& Options) override;

    private:
        bool IsPlausibleCodeAddress(
            U64 Address);

    private:
        const IModuleManager* ModuleManager = nullptr;
        X64PeUnwindData UnwindData;
    };
}
