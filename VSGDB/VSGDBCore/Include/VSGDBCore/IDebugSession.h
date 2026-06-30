#pragma once

#include "Types.h"
#include "Result.h"

#include "IDebugTarget.h"
#include "IModuleManager.h"
#include "ISymbolManager.h"
#include "IExpressionEvaluator.h"
#include "IDisassembler.h"
#include "IAssembler.h"

namespace VSGDBCore
{
    class IDebugSession
    {
    public:
        virtual ~IDebugSession() = default;

        virtual IDebugTarget& GetTarget() = 0;
        virtual IModuleManager& GetModuleManager() = 0;
        virtual ISymbolManager& GetSymbolManager() = 0;
        virtual IExpressionEvaluator& GetExpressionEvaluator() = 0;
        virtual IDisassembler& GetDisassembler() = 0;
        virtual IAssembler& GetAssembler() = 0;

        virtual DebugError Connect(
            const DebugTargetConfig& Config) = 0;

        virtual DebugError Disconnect() = 0;
    };
}
