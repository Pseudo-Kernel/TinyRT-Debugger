#pragma once

#include "Types.h"

namespace VSGDBCore
{
    struct ModuleInfo
    {
        std::wstring Name;
        std::wstring ImagePath;
        std::wstring SymbolPath;

        U64 ImageBase = 0;
        U64 ImageSize = 0;
    };
}
