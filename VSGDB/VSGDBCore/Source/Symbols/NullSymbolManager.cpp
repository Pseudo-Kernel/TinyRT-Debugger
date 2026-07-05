#include <VSGDBCore/NullComponents.h>
#include "NullSymbolManager.h"

namespace VSGDBCore
{
    DebugError
        NullSymbolManager::LoadSymbolsForModule(
            const ModuleInfo & Module)
    {
        return DebugError::Failure(
            ErrorCode::NotSupported,
            L"Symbol manager is not available.");
    }

    DebugError
        NullSymbolManager::UnloadSymbolsForModule(
            ModuleId ModuleId)
    {
        return DebugError::Failure(
            ErrorCode::NotSupported,
            L"Symbol manager is not available.");
    }

    Expected<SymbolInfo>
        NullSymbolManager::GetSymbolByAddress(
            U64 Address) const
    {
        return Expected<SymbolInfo>::Failure(
            DebugError::Failure(
                ErrorCode::NotSupported,
                L"No symbol manager is available."));
    }
    
    Expected<SymbolInfo>
        NullSymbolManager::GetSymbolByName(
            const std::wstring & Name) const
    {
        return Expected<SymbolInfo>::Failure(
            DebugError::Failure(
                ErrorCode::NotSupported,
                L"No symbol manager is available."));
    }

    Expected<SourceLocation>
        NullSymbolManager::GetSourceLocationByAddress(
            U64 Address) const
    {
        return Expected<SourceLocation>::Failure(
            DebugError::Failure(
                ErrorCode::NotSupported,
                L"No symbol manager is available."));
    }

    Expected<std::vector<SymbolInfo>>
        NullSymbolManager::EnumerateSymbols(
            ModuleId ModuleId) const
    {
        return Expected<std::vector<SymbolInfo>>::Failure(
            DebugError::Failure(
                ErrorCode::NotSupported,
                L"No symbol manager is available."));
    }
}
