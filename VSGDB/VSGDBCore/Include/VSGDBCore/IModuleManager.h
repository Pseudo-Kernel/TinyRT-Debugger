#pragma once

#include "Types.h"
#include "Result.h"
#include "ModuleTypes.h"

namespace VSGDBCore
{
    class IModuleManager
    {
    public:
        virtual ~IModuleManager() = default;

        virtual DebugError LoadImage(
            const ModuleInfo& Module) = 0;

        virtual DebugError UnloadImage(
            const std::wstring& ModuleName) = 0;

        virtual Result<ModuleInfo> GetModuleByAddress(
            U64 Address) const = 0;

        virtual std::vector<ModuleInfo> EnumerateModules() const = 0;
    };
}
