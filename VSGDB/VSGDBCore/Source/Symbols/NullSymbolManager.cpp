#include <VSGDBCore/NullComponents.h>

namespace VSGDBCore
{
    class NullSymbolManager final : public ISymbolManager
    {
    public:
        DebugError LoadSymbols(
            const ModuleInfo& Module) override
        {
            (void)Module;

            return DebugError::Failure(
                ErrorCode::NotSupported,
                L"Symbol manager is not available.");
        }

        DebugError UnloadSymbols(
            const std::wstring& ModuleName) override
        {
            (void)ModuleName;

            return DebugError::Failure(
                ErrorCode::NotSupported,
                L"Symbol manager is not available.");
        }

        Result<ResolvedSymbol> GetNearestSymbol(
            U64 Address) const override
        {
            (void)Address;

            return Result<ResolvedSymbol>::Failure(
                DebugError::Failure(
                    ErrorCode::NotSupported,
                    L"Symbol manager is not available."));
        }

        Result<ResolvedSymbol> AddressToSymbol(
            U64 Address) const override
        {
            (void)Address;

            return Result<ResolvedSymbol>::Failure(
                DebugError::Failure(
                    ErrorCode::NotSupported,
                    L"Symbol manager is not available."));
        }

        Result<SourceLocation> AddressToSourceLine(
            U64 Address) const override
        {
            (void)Address;

            return Result<SourceLocation>::Failure(
                DebugError::Failure(
                    ErrorCode::NotSupported,
                    L"Symbol manager is not available."));
        }

        Result<std::vector<U64>> SourceLineToAddress(
            const std::wstring& FilePath,
            U32 Line) const override
        {
            (void)FilePath;
            (void)Line;

            return Result<std::vector<U64>>::Failure(
                DebugError::Failure(
                    ErrorCode::NotSupported,
                    L"Symbol manager is not available."));
        }
    };

    std::unique_ptr<ISymbolManager>
        CreateNullSymbolManager()
    {
        return std::make_unique<NullSymbolManager>();
    }
}