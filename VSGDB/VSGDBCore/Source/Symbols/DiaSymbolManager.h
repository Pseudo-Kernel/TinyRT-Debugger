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
        : Pointer(Other.Pointer)
    {
        Other.Pointer = nullptr;
    }

    ComPtr& operator=(ComPtr&& Other) noexcept
    {
        if (this != &Other)
        {
            Reset();
            Pointer = Other.Pointer;
            Other.Pointer = nullptr;
        }

        return *this;
    }

    operator T*() const
    {
        return Pointer;
    }

    ~ComPtr()
    {
        Reset();
    }

    T* Get() const
    {
        return Pointer;
    }

    T** Put()
    {
        Reset();
        return &Pointer;
    }

    T* operator->() const
    {
        return Pointer;
    }

    void Reset()
    {
        if (Pointer != nullptr)
        {
            Pointer->Release();
            Pointer = nullptr;
        }
    }

private:
    T* Pointer = nullptr;
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

        static Expected<U32> GetSymbolRva(
            IDiaSymbol* Symbol);

        static Expected<U64> GetSymbolLength(
            IDiaSymbol* Symbol);

        const LoadedModule* FindLoadedModuleByAddress(
            U64 Address) const;

        const LoadedModule* FindLoadedModuleById(
            ModuleId ModuleId) const;

        std::vector<LoadedModule> LoadedModules;
    };
}