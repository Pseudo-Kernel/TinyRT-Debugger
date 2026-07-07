#include <VSGDBCore/Components.h>

#include "Disassembly/ZydisDisassembler.h"
#include "Modules/ModuleManager.h"
#include "Symbols/DiaSymbolManager.h"
#include "Symbols/NullSymbolManager.h"
#include "Stack/X64FramePointerStackWalker.h"
#include "Stack/X64PeUnwindStackWalker.h"

namespace VSGDBCore
{
    std::unique_ptr<IDisassembler>
        CreateDefaultDisassembler()
    {
        return std::make_unique<ZydisDisassembler>();
    }

    std::unique_ptr<IModuleManager>
        CreateModuleManager()
    {
        return std::make_unique<ModuleManager>();
    }

    std::unique_ptr<ISymbolManager>
        CreateDefaultSymbolManager()
    {
        return std::make_unique<DiaSymbolManager>();
    }

    std::unique_ptr<ISymbolManager>
        CreateNullSymbolManager()
    {
        return std::make_unique<NullSymbolManager>();
    }

    std::unique_ptr<IStackWalker>
        CreateX64FramePointerStackWalker()
    {
        return std::make_unique<X64FramePointerStackWalker>();
    }

    std::unique_ptr<IStackWalker>
        CreateX64PeUnwindStackWalker(
            const IModuleManager* ModuleManager)
    {
        return std::make_unique<X64PeUnwindStackWalker>(
            ModuleManager);
    }
}
