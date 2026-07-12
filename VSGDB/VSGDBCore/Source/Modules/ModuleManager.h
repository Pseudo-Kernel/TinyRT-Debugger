#pragma once

#include <VSGDBCore/IModuleManager.h>

#include <vector>

namespace VSGDBCore
{
    class ModuleManager final : public IModuleManager
    {
    public:
        Expected<ModuleId> AddModule(
            const ModuleInfo& Module) override;

        DebugError RemoveModule(
            ModuleId Id) override;

        Expected<ModuleInfo> GetModuleById(
            ModuleId Id) const override;

        Expected<ModuleInfo> GetModuleByAddress(
            U64 Address) const override;

        Expected<ModuleInfo> GetModuleByName(
            const std::wstring& Name) const override;

        std::vector<ModuleInfo> EnumerateModules() const override;

        void Clear() override;

    private:
        static bool IsAddressInModule(
            const ModuleInfo& Module,
            U64 Address);

        static bool AreNamesEqual(
            const std::wstring& Left,
            const std::wstring& Right);

        std::vector<ModuleInfo> Modules_;
        ModuleId NextModuleId_ = 1;
    };
}