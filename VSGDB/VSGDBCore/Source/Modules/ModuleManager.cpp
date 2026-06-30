#include <VSGDBCore/NullComponents.h>

#include <algorithm>

namespace VSGDBCore
{
    class ModuleManager final : public IModuleManager
    {
    public:
        DebugError LoadImage(
            const ModuleInfo& Module) override
        {
            if (Module.Name.empty())
            {
                return DebugError::Failure(
                    ErrorCode::InvalidArgument,
                    L"Module name is empty.");
            }

            auto Existing = std::find_if(
                Modules.begin(),
                Modules.end(),
                [&](const ModuleInfo& Entry)
                {
                    return Entry.Name == Module.Name;
                });

            if (Existing != Modules.end())
            {
                *Existing = Module;
            }
            else
            {
                Modules.push_back(Module);
            }

            return DebugError::Success();
        }

        DebugError UnloadImage(
            const std::wstring& ModuleName) override
        {
            auto NewEnd = std::remove_if(
                Modules.begin(),
                Modules.end(),
                [&](const ModuleInfo& Entry)
                {
                    return Entry.Name == ModuleName;
                });

            if (NewEnd == Modules.end())
            {
                return DebugError::Failure(
                    ErrorCode::InvalidArgument,
                    L"Module is not loaded.");
            }

            Modules.erase(NewEnd, Modules.end());
            return DebugError::Success();
        }

        Result<ModuleInfo> GetModuleByAddress(
            U64 Address) const override
        {
            for (const ModuleInfo& Module : Modules)
            {
                if (Address >= Module.ImageBase &&
                    Address < Module.ImageBase + Module.ImageSize)
                {
                    return Result<ModuleInfo>::Success(Module);
                }
            }

            return Result<ModuleInfo>::Failure(
                DebugError::Failure(
                    ErrorCode::InvalidArgument,
                    L"No module contains the specified address."));
        }

        std::vector<ModuleInfo> EnumerateModules() const override
        {
            return Modules;
        }

    private:
        std::vector<ModuleInfo> Modules;
    };

    std::unique_ptr<IModuleManager>
    CreateModuleManager()
    {
        return std::make_unique<ModuleManager>();
    }
}