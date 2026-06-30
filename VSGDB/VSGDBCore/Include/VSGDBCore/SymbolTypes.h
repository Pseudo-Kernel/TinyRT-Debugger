#pragma once

#include "Types.h"

namespace VSGDBCore
{
    struct ResolvedSymbol
    {
        bool Found = false;

        std::wstring ModuleName;
        std::wstring SymbolName;

        U64 Address = 0;
        U64 SymbolAddress = 0;
        U64 Offset = 0;
    };

    struct SourceLocation
    {
        bool Found = false;

        std::wstring FilePath;
        U32 Line = 0;
        U32 Column = 0;
        U64 Address = 0;
    };
}
