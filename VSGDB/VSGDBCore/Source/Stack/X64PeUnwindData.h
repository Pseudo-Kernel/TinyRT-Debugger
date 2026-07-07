#pragma once

#include <VSGDBCore/ModuleTypes.h>
#include <VSGDBCore/Result.h>
#include <VSGDBCore/X64UnwindTypes.h>

#include <optional>
#include <unordered_map>

namespace VSGDBCore
{
    class X64PeUnwindData final
    {
    public:
        Expected<const X64UnwindModuleData*> GetOrLoadModule(
            const ModuleInfo& Module);

        Expected<X64RuntimeFunction> LookupRuntimeFunction(
            const ModuleInfo& Module,
            U64 ControlPc);

        Expected<X64UnwindInfo> ReadUnwindInfo(
            const X64UnwindModuleData& ModuleData,
            U32 UnwindInfoRva) const;

        const U8* RvaToPtr(
            const X64UnwindModuleData& ModuleData,
            U32 Rva,
            U32 Size) const;

    private:
        Expected<X64UnwindModuleData> LoadModule(
            const ModuleInfo& Module) const;

        std::unordered_map<ModuleId, X64UnwindModuleData> Modules;
    };
}