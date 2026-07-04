#pragma once

#include "Types.h"
#include "Result.h"
#include "ModuleTypes.h"

#include <string>
#include <vector>

namespace VSGDBCore
{
    class IModuleManager
    {
    public:
        virtual ~IModuleManager() = default;

        virtual Expected<ModuleId> AddModule(
            const ModuleInfo& Module) = 0;

        virtual DebugError RemoveModule(
            ModuleId Id) = 0;

        virtual Expected<ModuleInfo> GetModuleById(
            ModuleId Id) const = 0;

        virtual Expected<ModuleInfo> GetModuleByAddress(
            U64 Address) const = 0;

        virtual Expected<ModuleInfo> GetModuleByName(
            const std::wstring& Name) const = 0;

        virtual std::vector<ModuleInfo> EnumerateModules() const = 0;

        virtual void Clear() = 0;
    };
}