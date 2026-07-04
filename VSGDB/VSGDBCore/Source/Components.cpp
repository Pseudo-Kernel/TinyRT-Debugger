#include <VSGDBCore/Components.h>

#include "Disassembly/ZydisDisassembler.h"
#include "Modules/ModuleManager.h"

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

}