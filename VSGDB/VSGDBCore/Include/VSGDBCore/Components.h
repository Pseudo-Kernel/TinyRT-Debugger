#pragma once

#include "IDisassembler.h"
#include "IModuleManager.h"
#include "ISymbolManager.h"

#include <memory>

namespace VSGDBCore
{
    std::unique_ptr<IDisassembler> CreateDefaultDisassembler();
    std::unique_ptr<IDisassembler> CreateNullDisassembler();
    std::unique_ptr<ISymbolManager> CreateNullSymbolManager();

    std::unique_ptr<IModuleManager> CreateModuleManager();
    std::unique_ptr<ISymbolManager> CreateDefaultSymbolManager();
}
