#pragma once

#include "Types.h"
#include "Result.h"
#include "ModuleTypes.h"
#include "SymbolTypes.h"

#include <string>
#include <vector>

namespace VSGDBCore
{
    class ISymbolManager
    {
    public:
        virtual ~ISymbolManager() = default;

        virtual DebugError LoadSymbolsForModule(
            const ModuleInfo& Module) = 0;

        virtual DebugError UnloadSymbolsForModule(
            ModuleId ModuleId) = 0;

        virtual Expected<SymbolInfo> GetSymbolByAddress(
            U64 Address) const = 0;

        virtual Expected<SymbolInfo> GetSymbolByName(
            const std::wstring& Name) const = 0;

        virtual Expected<SourceLocation> GetSourceLocationByAddress(
            U64 Address) const = 0;

        virtual Expected<std::vector<SymbolInfo>> EnumerateSymbols(
            ModuleId ModuleId) const = 0;
    };
}