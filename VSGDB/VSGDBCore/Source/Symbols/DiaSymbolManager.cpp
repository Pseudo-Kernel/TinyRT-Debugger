#include "DiaSymbolManager.h"

#include <algorithm>

namespace VSGDBCore
{
    DiaSymbolManager::DiaSymbolManager()
    {
        (void)::CoInitializeEx(
            nullptr,
            COINIT_MULTITHREADED);
    }

    DiaSymbolManager::~DiaSymbolManager()
    {
        LoadedModules.clear();
        ::CoUninitialize();
    }

    DebugError
        DiaSymbolManager::LoadSymbolsForModule(
            const ModuleInfo& Module)
    {
        for (const LoadedModule& Existing : LoadedModules)
        {
            if (Existing.Module.Id == Module.Id)
            {
                return DebugError::Success();
            }
        }

        if (Module.ImagePath.empty())
        {
            return DebugError::Failure(
                ErrorCode::InvalidArgument,
                L"Module image path is empty.");
        }

        ComPtr<IDiaDataSource> DataSource;

        HRESULT Hr = ::CoCreateInstance(
            CLSID_DiaSource,
            nullptr,
            CLSCTX_INPROC_SERVER,
            __uuidof(IDiaDataSource),
            reinterpret_cast<void**>(DataSource.Put()));

        if (FAILED(Hr))
        {
            return HResultToDebugError(
                Hr,
                ErrorCode::SymbolFailure,
                L"Failed to create DIA data source.");
        }

        Hr = DataSource->loadDataForExe(
            Module.ImagePath.c_str(),
            nullptr,
            nullptr);

        if (FAILED(Hr))
        {
            return HResultToDebugError(
                Hr,
                ErrorCode::SymbolFailure,
                L"Failed to load DIA symbols for image.");
        }

        ComPtr<IDiaSession> Session;

        Hr = DataSource->openSession(Session.Put());

        if (FAILED(Hr))
        {
            return HResultToDebugError(
                Hr,
                ErrorCode::SymbolFailure,
                L"Failed to open DIA session.");
        }

        Hr = Session->put_loadAddress(Module.BaseAddress);

        if (FAILED(Hr))
        {
            return HResultToDebugError(
                Hr,
                ErrorCode::SymbolFailure,
                L"Failed to set DIA module load address.");
        }

        ComPtr<IDiaSymbol> GlobalScope;

        Hr = Session->get_globalScope(GlobalScope.Put());

        if (FAILED(Hr))
        {
            return HResultToDebugError(
                Hr,
                ErrorCode::SymbolFailure,
                L"Failed to get DIA global scope.");
        }

        LoadedModule Loaded{};
        Loaded.Module = Module;
        Loaded.DataSource = std::move(DataSource);
        Loaded.Session = std::move(Session);
        Loaded.GlobalScope = std::move(GlobalScope);

        LoadedModules.push_back(std::move(Loaded));

        return DebugError::Success();
    }

    DebugError
        DiaSymbolManager::UnloadSymbolsForModule(
            ModuleId ModuleId)
    {
        const auto Iterator =
            std::find_if(
                LoadedModules.begin(),
                LoadedModules.end(),
                [ModuleId](const LoadedModule& Module)
                {
                    return Module.Module.Id == ModuleId;
                });

        if (Iterator == LoadedModules.end())
        {
            return DebugError::Failure(
                ErrorCode::InvalidArgument,
                L"Module symbols were not loaded.");
        }

        LoadedModules.erase(Iterator);

        return DebugError::Success();
    }

    Expected<SymbolInfo>
        DiaSymbolManager::GetSymbolByAddress(
            U64 Address) const
    {
        const LoadedModule* Loaded = FindLoadedModuleByAddress(Address);

        if (Loaded == nullptr)
        {
            return Expected<SymbolInfo>::Failure(
                DebugError::Failure(
                    ErrorCode::SymbolFailure,
                    L"No loaded symbols contain the specified address."));
        }

        const U32 Rva =
            static_cast<U32>(Address - Loaded->Module.BaseAddress);

        ComPtr<IDiaSymbol> DiaSymbol;

        HRESULT Hr = Loaded->Session->findSymbolByRVA(
            Rva,
            SymTagFunction,
            DiaSymbol.Put());

        if (FAILED(Hr) || DiaSymbol == nullptr)
        {
            Hr = Loaded->Session->findSymbolByRVA(
                Rva,
                SymTagPublicSymbol,
                DiaSymbol.Put());
        }

        if (FAILED(Hr) || DiaSymbol == nullptr)
        {
            return Expected<SymbolInfo>::Failure(
                DebugError::Failure(
                    ErrorCode::SymbolFailure,
                    L"No symbol found for address."));
        }

        auto Name = GetSymbolName(DiaSymbol);
        if (!Name.HasValue())
        {
            return Expected<SymbolInfo>::Failure(Name.Error);
        }

        auto SymbolRva = GetSymbolRva(DiaSymbol);
        if (!SymbolRva.HasValue())
        {
            return Expected<SymbolInfo>::Failure(SymbolRva.Error);
        }

        auto Length = GetSymbolLength(DiaSymbol);

        SymbolInfo Info{};
        Info.ModuleId = Loaded->Module.Id;
        Info.Name = Name.Value;
        Info.Address = Loaded->Module.BaseAddress + SymbolRva.Value;

        if (Length.HasValue() && Length.Value <= 0xffffffffull)
        {
            Info.Size = static_cast<U32>(Length.Value);
        }

        return Expected<SymbolInfo>::Success(std::move(Info));
    }

    Expected<SymbolInfo>
        DiaSymbolManager::GetSymbolByName(
            const std::wstring& Name) const
    {
        for (const LoadedModule& Loaded : LoadedModules)
        {
            ComPtr<IDiaEnumSymbols> EnumSymbols;

            HRESULT Hr = Loaded.GlobalScope->findChildren(
                SymTagFunction,
                Name.c_str(),
                nsCaseInsensitive,
                EnumSymbols.Put());

            if (FAILED(Hr) || EnumSymbols == nullptr)
            {
                continue;
            }

            ComPtr<IDiaSymbol> DiaSymbol;
            ULONG Fetched = 0;

            Hr = EnumSymbols->Next(
                1,
                DiaSymbol.Put(),
                &Fetched);

            if (FAILED(Hr) || Fetched == 0 || DiaSymbol == nullptr)
            {
                continue;
            }

            auto SymbolName = GetSymbolName(DiaSymbol);
            auto SymbolRva = GetSymbolRva(DiaSymbol);
            auto Length = GetSymbolLength(DiaSymbol);

            if (!SymbolName.HasValue() || !SymbolRva.HasValue())
            {
                continue;
            }

            SymbolInfo Info{};
            Info.ModuleId = Loaded.Module.Id;
            Info.Name = SymbolName.Value;
            Info.Address = Loaded.Module.BaseAddress + SymbolRva.Value;

            if (Length.HasValue() && Length.Value <= 0xffffffffull)
            {
                Info.Size = static_cast<U32>(Length.Value);
            }

            return Expected<SymbolInfo>::Success(std::move(Info));
        }

        return Expected<SymbolInfo>::Failure(
            DebugError::Failure(
                ErrorCode::SymbolFailure,
                L"Symbol name was not found."));
    }

    DebugError
        DiaSymbolManager::HResultToDebugError(
            HRESULT Hr,
            ErrorCode Code,
            const wchar_t* Message)
    {
        return DebugError::Failure(
            Code,
            Message,
            static_cast<U32>(Hr));
    }

    Expected<std::wstring>
        DiaSymbolManager::GetSymbolName(
            IDiaSymbol* Symbol)
    {
        BSTR Name = nullptr;

        HRESULT Hr = Symbol->get_name(&Name);

        if (FAILED(Hr) || Name == nullptr)
        {
            if (Name != nullptr)
            {
                ::SysFreeString(Name);
            }

            return Expected<std::wstring>::Failure(
                HResultToDebugError(
                    Hr,
                    ErrorCode::SymbolFailure,
                    L"Failed to get symbol name."));
        }

        std::wstring Text(
            Name,
            ::SysStringLen(Name));

        ::SysFreeString(Name);

        if (Text.empty())
        {
            return Expected<std::wstring>::Failure(
                DebugError::Failure(
                    ErrorCode::SymbolFailure,
                    L"Symbol name is empty."));
        }

        return Expected<std::wstring>::Success(std::move(Text));
    }

    Expected<U32>
        DiaSymbolManager::GetSymbolRva(
            IDiaSymbol* Symbol)
    {
        DWORD Rva = 0;

        HRESULT Hr = Symbol->get_relativeVirtualAddress(&Rva);

        if (FAILED(Hr))
        {
            return Expected<U32>::Failure(
                HResultToDebugError(
                    Hr,
                    ErrorCode::SymbolFailure,
                    L"Failed to get symbol RVA."));
        }

        return Expected<U32>::Success(static_cast<U32>(Rva));
    }

    Expected<U64>
        DiaSymbolManager::GetSymbolLength(
            IDiaSymbol* Symbol)
    {
        ULONGLONG Length = 0;

        HRESULT Hr = Symbol->get_length(&Length);

        if (FAILED(Hr))
        {
            return Expected<U64>::Failure(
                HResultToDebugError(
                    Hr,
                    ErrorCode::SymbolFailure,
                    L"Failed to get symbol length."));
        }

        return Expected<U64>::Success(static_cast<U64>(Length));
    }

    const DiaSymbolManager::LoadedModule*
        DiaSymbolManager::FindLoadedModuleByAddress(
            U64 Address) const
    {
        for (const LoadedModule& Loaded : LoadedModules)
        {
            if (Loaded.Module.Size == 0)
            {
                if (Address >= Loaded.Module.BaseAddress)
                {
                    return &Loaded;
                }

                continue;
            }

            const U64 EndAddress =
                Loaded.Module.BaseAddress + Loaded.Module.Size;

            if (Address >= Loaded.Module.BaseAddress &&
                Address < EndAddress)
            {
                return &Loaded;
            }
        }

        return nullptr;
    }

    const DiaSymbolManager::LoadedModule*
        DiaSymbolManager::FindLoadedModuleById(
            ModuleId ModuleId) const
    {
        for (const LoadedModule& Loaded : LoadedModules)
        {
            if (Loaded.Module.Id == ModuleId)
            {
                return &Loaded;
            }
        }

        return nullptr;
    }

    Expected<SourceLocation>
        DiaSymbolManager::GetSourceLocationByAddress(
            U64 Address) const
    {
        const LoadedModule* Loaded =
            FindLoadedModuleByAddress(Address);

        if (Loaded == nullptr)
        {
            return Expected<SourceLocation>::Failure(
                DebugError::Failure(
                    ErrorCode::SymbolFailure,
                    L"No loaded symbols contain the specified address"));
        }

        const U64 Rva64 =
            Address - Loaded->Module.BaseAddress;

        if (Rva64 > 0xffffffffull)
        {
            return Expected<SourceLocation>::Failure(
                DebugError::Failure(
                    ErrorCode::SymbolFailure,
                    L"Address RVA is larger than 32 bits"));
        }

        const DWORD Rva =
            static_cast<DWORD>(Rva64);

        ComPtr<IDiaEnumLineNumbers> Lines;

        HRESULT Hr = Loaded->Session->findLinesByRVA(
            Rva,
            1,
            Lines.Put());

        if (FAILED(Hr) || Lines == nullptr)
        {
            return Expected<SourceLocation>::Failure(
                HResultToDebugError(
                    Hr,
                    ErrorCode::SymbolFailure,
                    L"Failed to find source line for address"));
        }

        ComPtr<IDiaLineNumber> Line;
        ULONG Fetched = 0;

        Hr = Lines->Next(
            1,
            Line.Put(),
            &Fetched);

        if (FAILED(Hr) || Fetched == 0 || Line == nullptr)
        {
            return Expected<SourceLocation>::Failure(
                DebugError::Failure(
                    ErrorCode::SymbolFailure,
                    L"No source line found for address"));
        }

        DWORD LineNumber = 0;
        DWORD ColumnNumber = 0;
        DWORD LineRva = 0;

        Hr = Line->get_lineNumber(&LineNumber);

        if (FAILED(Hr))
        {
            return Expected<SourceLocation>::Failure(
                HResultToDebugError(
                    Hr,
                    ErrorCode::SymbolFailure,
                    L"Failed to get source line number"));
        }

        //
        // Column can be unavailable. Treat failure as column 0.
        //
        (void)Line->get_columnNumber(&ColumnNumber);
        (void)Line->get_relativeVirtualAddress(&LineRva);

        ComPtr<IDiaSourceFile> SourceFile;

        Hr = Line->get_sourceFile(SourceFile.Put());

        if (FAILED(Hr) || SourceFile == nullptr)
        {
            return Expected<SourceLocation>::Failure(
                HResultToDebugError(
                    Hr,
                    ErrorCode::SymbolFailure,
                    L"Failed to get DIA source file"));
        }

        BSTR FileName = nullptr;

        Hr = SourceFile->get_fileName(&FileName);

        if (FAILED(Hr) || FileName == nullptr)
        {
            if (FileName != nullptr)
            {
                ::SysFreeString(FileName);
            }

            return Expected<SourceLocation>::Failure(
                HResultToDebugError(
                    Hr,
                    ErrorCode::SymbolFailure,
                    L"Failed to get source file path"));
        }

        std::wstring FilePath(
            FileName,
            ::SysStringLen(FileName));

        ::SysFreeString(FileName);

        SourceLocation Location{};
        Location.ModuleId = Loaded->Module.Id;
        Location.FilePath = std::move(FilePath);
        Location.Line = static_cast<U32>(LineNumber);
        Location.Column = static_cast<U32>(ColumnNumber);
        Location.Address = Loaded->Module.BaseAddress + LineRva;

        return Expected<SourceLocation>::Success(std::move(Location));
    }

    Expected<std::vector<SymbolInfo>>
        DiaSymbolManager::EnumerateSymbols(
            ModuleId ModuleId) const
    {
        const LoadedModule* Loaded =
            FindLoadedModuleById(ModuleId);

        if (Loaded == nullptr)
        {
            return Expected<std::vector<SymbolInfo>>::Failure(
                DebugError::Failure(
                    ErrorCode::InvalidArgument,
                    L"Module symbols were not loaded"));
        }

        ComPtr<IDiaEnumSymbols> EnumSymbols;

        HRESULT Hr = Loaded->GlobalScope->findChildren(
            SymTagFunction,
            nullptr,
            nsNone,
            EnumSymbols.Put());

        if (FAILED(Hr) || EnumSymbols == nullptr)
        {
            return Expected<std::vector<SymbolInfo>>::Failure(
                HResultToDebugError(
                    Hr,
                    ErrorCode::SymbolFailure,
                    L"Failed to enumerate function symbols"));
        }

        std::vector<SymbolInfo> Symbols;

        for (;;)
        {
            ComPtr<IDiaSymbol> DiaSymbol;
            ULONG Fetched = 0;

            Hr = EnumSymbols->Next(
                1,
                DiaSymbol.Put(),
                &Fetched);

            if (FAILED(Hr))
            {
                return Expected<std::vector<SymbolInfo>>::Failure(
                    HResultToDebugError(
                        Hr,
                        ErrorCode::SymbolFailure,
                        L"Failed to enumerate next symbol"));
            }

            if (Fetched == 0 || DiaSymbol == nullptr)
            {
                break;
            }

            auto Name = GetSymbolName(DiaSymbol);
            auto Rva = GetSymbolRva(DiaSymbol);
            auto Length = GetSymbolLength(DiaSymbol);

            if (!Name.HasValue() || !Rva.HasValue())
            {
                continue;
            }

            SymbolInfo Info{};
            Info.ModuleId = Loaded->Module.Id;
            Info.Name = std::move(Name.Value);
            Info.Address = Loaded->Module.BaseAddress + Rva.Value;

            if (Length.HasValue() &&
                Length.Value <= 0xffffffffull)
            {
                Info.Size = static_cast<U32>(Length.Value);
            }

            Symbols.push_back(std::move(Info));
        }

        std::sort(
            Symbols.begin(),
            Symbols.end(),
            [](const SymbolInfo& Left, const SymbolInfo& Right)
            {
                if (Left.Address != Right.Address)
                {
                    return Left.Address < Right.Address;
                }

                return Left.Name < Right.Name;
            });

        return Expected<std::vector<SymbolInfo>>::Success(
            std::move(Symbols));
    }
}

