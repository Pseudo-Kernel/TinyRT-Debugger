#pragma once

#include <VSGDBCore/ISymbolManager.h>

#include <dia2.h>
#include <comdef.h>

#include <vector>

// Our simple ComPtr<T> to replace CComPtr<T>
template<typename T>
class ComPtr final
{
public:
    ComPtr() = default;

    ComPtr(const ComPtr&) = delete;
    ComPtr& operator=(const ComPtr&) = delete;

    ComPtr(ComPtr&& Other) noexcept
        : Pointer_(Other.Pointer_)
    {
        Other.Pointer_ = nullptr;
    }

    ComPtr& operator=(ComPtr&& Other) noexcept
    {
        if (this != &Other)
        {
            Reset();
            Pointer_ = Other.Pointer_;
            Other.Pointer_ = nullptr;
        }

        return *this;
    }

    operator T*() const
    {
        return Pointer_;
    }

    ~ComPtr()
    {
        Reset();
    }

    T* Get() const
    {
        return Pointer_;
    }

    T** Put()
    {
        Reset();
        return &Pointer_;
    }

    T* operator->() const
    {
        return Pointer_;
    }

    void Reset()
    {
        if (Pointer_ != nullptr)
        {
            Pointer_->Release();
            Pointer_ = nullptr;
        }
    }

private:
    T* Pointer_ = nullptr;
};

namespace VSGDBCore
{
    class DiaSymbolManager final : public ISymbolManager
    {
    public:
        DiaSymbolManager();
        ~DiaSymbolManager() override;

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

    private:
        struct LoadedModule
        {
            ModuleInfo Module;

            ComPtr<IDiaDataSource> DataSource;
            ComPtr<IDiaSession> Session;
            ComPtr<IDiaSymbol> GlobalScope;
        };

        static DebugError HResultToDebugError(
            HRESULT Hr,
            ErrorCode Code,
            const wchar_t* Message);

        static Expected<std::wstring> GetSymbolName(
            IDiaSymbol* Symbol);

        Expected<U32> GetSymbolRva(
            IDiaSymbol* Symbol) const;

        Expected<U64>
            GetSymbolRuntimeAddress(
                const LoadedModule& Loaded,
                IDiaSymbol* Symbol) const;

        static Expected<U64> GetSymbolLength(
            IDiaSymbol* Symbol);

        const LoadedModule* FindLoadedModuleByAddress(
            U64 Address) const;

        const LoadedModule* FindLoadedModuleById(
            ModuleId ModuleId) const;

        Expected<SymbolInfo>
            BuildSymbolInfo(
                const LoadedModule& Loaded,
                IDiaSymbol* DiaSymbol,
                SymbolKind Kind,
                enum SymTagEnum NativeTag) const;


        std::vector<LoadedModule> LoadedModules_;
    };
}