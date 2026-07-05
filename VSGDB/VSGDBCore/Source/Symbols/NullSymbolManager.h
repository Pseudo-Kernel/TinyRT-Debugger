#include <VSGDBCore/NullComponents.h>

namespace VSGDBCore
{
    class NullSymbolManager final : public ISymbolManager
    {
    public:
        DebugError LoadSymbolsForModule(
            const ModuleInfo& Module) override;

        DebugError UnloadSymbolsForModule(
            ModuleId ModuleId) override;

        Expected<SymbolInfo> GetSymbolByAddress(
            U64 Address) const override;

        Expected<SymbolInfo> GetSymbolByName(
            const std::wstring& Name) const override;

        Expected<SourceLocation> GetSourceLocationByAddress(
            U64 Address) const override;

        Expected<std::vector<SymbolInfo>> EnumerateSymbols(
            ModuleId ModuleId) const override;

    };
}