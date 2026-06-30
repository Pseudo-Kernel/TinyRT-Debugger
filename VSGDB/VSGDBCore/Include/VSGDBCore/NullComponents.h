#pragma once

#include <memory>

#include "IDebugTarget.h"
#include "IDisassembler.h"
#include "IAssembler.h"
#include "ISymbolManager.h"
#include "IExpressionEvaluator.h"
#include "IModuleManager.h"

namespace VSGDBCore
{
    std::unique_ptr<IDebugTarget> CreateNullDebugTarget();
    std::unique_ptr<IDisassembler> CreateNullDisassembler();
    std::unique_ptr<IAssembler> CreateNullAssembler();
    std::unique_ptr<ISymbolManager> CreateNullSymbolManager();
    std::unique_ptr<IExpressionEvaluator> CreateSimpleExpressionEvaluator();
    std::unique_ptr<IModuleManager> CreateModuleManager();
}