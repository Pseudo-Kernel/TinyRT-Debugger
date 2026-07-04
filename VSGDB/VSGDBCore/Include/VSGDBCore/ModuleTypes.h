#pragma once

#include "Types.h"

#include <string>
#include <vector>

namespace VSGDBCore
{
    using ModuleId = U32;

    struct ModuleInfo
    {
        ModuleId Id = 0;

        std::wstring Name;
        std::wstring ImagePath;
        std::wstring SymbolPath;

        U64 BaseAddress = 0;
        U64 Size = 0;

        bool SymbolsLoaded = false;
    };
}