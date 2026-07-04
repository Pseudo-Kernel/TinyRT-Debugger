#pragma once

#include "Types.h"
#include "Result.h"
#include "ModuleTypes.h"
#include "SymbolTypes.h"

namespace VSGDBCore
{
    class ISymbolManager
    {
    public:
        virtual ~ISymbolManager() = default;

        virtual DebugError LoadSymbols(
            const ModuleInfo& Module) = 0;

        virtual DebugError UnloadSymbols(
            const std::wstring& ModuleName) = 0;

        virtual Expected<ResolvedSymbol> GetNearestSymbol(
            U64 Address) const = 0;

        virtual Expected<ResolvedSymbol> AddressToSymbol(
            U64 Address) const = 0;

        virtual Expected<SourceLocation> AddressToSourceLine(
            U64 Address) const = 0;

        virtual Expected<std::vector<U64>> SourceLineToAddress(
            const std::wstring& FilePath,
            U32 Line) const = 0;
    };
}
