#pragma once

#include "IDebugTarget.h"
#include "IDisassembler.h"
#include "IModuleManager.h"
#include "ISymbolManager.h"
#include "IStackWalker.h"
#include "IDebugSession.h"

#include <memory>

namespace VSGDBCore
{
    std::unique_ptr<IDebugSession> CreateDefaultDebugSession();
    std::unique_ptr<IDebugTarget> CreateGdbRemoteTarget();

    std::unique_ptr<IDisassembler> CreateDefaultDisassembler();
    std::unique_ptr<IDisassembler> CreateNullDisassembler();
    std::unique_ptr<ISymbolManager> CreateNullSymbolManager();

    std::unique_ptr<IModuleManager> CreateModuleManager();
    std::unique_ptr<ISymbolManager> CreateDefaultSymbolManager();

    std::unique_ptr<IStackWalker> CreateX64FramePointerStackWalker();
    std::unique_ptr<IStackWalker>
        CreateX64PeUnwindStackWalker(
            const IModuleManager* ModuleManager);
}
