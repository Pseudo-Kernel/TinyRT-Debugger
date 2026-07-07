#pragma once

#include "Types.h"
#include "ModuleTypes.h"

#include <string>

namespace VSGDBCore
{
    using SymbolId = U32;

    enum class SymbolKind
    {
        Unknown,
        Function,
        PublicSymbol,
        Data,
        Label
    };

    struct SymbolInfo
    {
        SymbolId Id = 0;
        ModuleId ModuleId = 0;

        std::wstring Name;

        //
        // Runtime virtual address.
        //
        U64 Address = 0;

        //
        // Size can be zero if unknown.
        //
        U32 Size = 0;

        SymbolKind Kind = SymbolKind::Unknown;
        U32 NativeTag = 0;
    };

    struct SourceLocation
    {
        ModuleId ModuleId = 0;

        std::wstring FilePath;

        U32 Line = 0;
        U32 Column = 0;

        U64 Address = 0;
    };
}