#include "ModuleManager.h"

#include <algorithm>
#include <cwctype>

namespace VSGDBCore
{
    static std::wstring
        ToLowerInvariant(
            const std::wstring& Value)
    {
        std::wstring Lower = Value;

        std::transform(
            Lower.begin(),
            Lower.end(),
            Lower.begin(),
            [](wchar_t Ch)
            {
                return static_cast<wchar_t>(std::towlower(Ch));
            });

        return Lower;
    }

    bool
        ModuleManager::IsAddressInModule(
            const ModuleInfo& Module,
            U64 Address)
    {
        if (Module.Size == 0)
        {
            return Address == Module.BaseAddress;
        }

        const U64 EndAddress = Module.BaseAddress + Module.Size;

        if (EndAddress < Module.BaseAddress)
        {
            return false;
        }

        return Address >= Module.BaseAddress &&
            Address < EndAddress;
    }

    bool
        ModuleManager::AreNamesEqual(
            const std::wstring& Left,
            const std::wstring& Right)
    {
        return ToLowerInvariant(Left) == ToLowerInvariant(Right);
    }

    Expected<ModuleId>
        ModuleManager::AddModule(
            const ModuleInfo& Module)
    {
        if (Module.BaseAddress == 0)
        {
            return Expected<ModuleId>::Failure(
                DebugError::Failure(
                    ErrorCode::InvalidArgument,
                    L"Module base address is zero."));
        }

        if (Module.Name.empty() && Module.ImagePath.empty())
        {
            return Expected<ModuleId>::Failure(
                DebugError::Failure(
                    ErrorCode::InvalidArgument,
                    L"Module name and image path are both empty."));
        }

        if (Module.Size != 0)
        {
            const U64 EndAddress = Module.BaseAddress + Module.Size;

            if (EndAddress < Module.BaseAddress)
            {
                return Expected<ModuleId>::Failure(
                    DebugError::Failure(
                        ErrorCode::InvalidArgument,
                        L"Module address range overflows."));
            }

            for (const ModuleInfo& ExistingModule : Modules)
            {
                if (ExistingModule.Size == 0)
                {
                    continue;
                }

                const U64 ExistingEndAddress =
                    ExistingModule.BaseAddress + ExistingModule.Size;

                const bool Overlaps =
                    Module.BaseAddress < ExistingEndAddress &&
                    EndAddress > ExistingModule.BaseAddress;

                if (Overlaps)
                {
                    return Expected<ModuleId>::Failure(
                        DebugError::Failure(
                            ErrorCode::InvalidArgument,
                            L"Module address range overlaps an existing module."));
                }
            }
        }

        if (NextModuleId == 0)
        {
            return Expected<ModuleId>::Failure(
                DebugError::Failure(
                    ErrorCode::InvalidState,
                    L"Module ID space exhausted."));
        }

        const ModuleId Id = NextModuleId++;

        ModuleInfo StoredModule = Module;
        StoredModule.Id = Id;

        Modules.push_back(std::move(StoredModule));

        return Expected<ModuleId>::Success(Id);
    }

    DebugError
        ModuleManager::RemoveModule(
            ModuleId Id)
    {
        const auto Iterator =
            std::find_if(
                Modules.begin(),
                Modules.end(),
                [Id](const ModuleInfo& Module)
                {
                    return Module.Id == Id;
                });

        if (Iterator == Modules.end())
        {
            return DebugError::Failure(
                ErrorCode::InvalidArgument,
                L"Module ID was not found.");
        }

        Modules.erase(Iterator);

        return DebugError::Success();
    }

    Expected<ModuleInfo>
        ModuleManager::GetModuleById(
            ModuleId Id) const
    {
        for (const ModuleInfo& Module : Modules)
        {
            if (Module.Id == Id)
            {
                return Expected<ModuleInfo>::Success(Module);
            }
        }

        return Expected<ModuleInfo>::Failure(
            DebugError::Failure(
                ErrorCode::InvalidArgument,
                L"Module ID was not found."));
    }

    Expected<ModuleInfo>
        ModuleManager::GetModuleByAddress(
            U64 Address) const
    {
        for (const ModuleInfo& Module : Modules)
        {
            if (IsAddressInModule(Module, Address))
            {
                return Expected<ModuleInfo>::Success(Module);
            }
        }

        return Expected<ModuleInfo>::Failure(
            DebugError::Failure(
                ErrorCode::InvalidArgument,
                L"No module contains the specified address."));
    }

    Expected<ModuleInfo>
        ModuleManager::GetModuleByName(
            const std::wstring& Name) const
    {
        for (const ModuleInfo& Module : Modules)
        {
            if (AreNamesEqual(Module.Name, Name))
            {
                return Expected<ModuleInfo>::Success(Module);
            }
        }

        return Expected<ModuleInfo>::Failure(
            DebugError::Failure(
                ErrorCode::InvalidArgument,
                L"Module name was not found."));
    }

    std::vector<ModuleInfo>
        ModuleManager::EnumerateModules() const
    {
        return Modules;
    }

    void
        ModuleManager::Clear()
    {
        Modules.clear();
        NextModuleId = 1;
    }
}