#include "DiaSymbolManager.h"

#include <algorithm>

namespace VSGDBCore
{
    struct DiaSymbolTagPolicy
    {
        enum SymTagEnum Tag;
        SymbolKind Kind;
    };

    static constexpr DiaSymbolTagPolicy g_DiaSymbolSearchTags[] =
    {
        { SymTagFunction,     SymbolKind::Function },
        { SymTagData,         SymbolKind::Data },
        { SymTagPublicSymbol, SymbolKind::PublicSymbol },
        { SymTagLabel,        SymbolKind::Label },
    };

    static int
        GetSymbolKindPriority(
            SymbolKind Kind)
    {
        switch (Kind)
        {
        case SymbolKind::Function: return 0;
        case SymbolKind::Data: return 1;
        case SymbolKind::Label: return 2;
        case SymbolKind::PublicSymbol: return 3;
        default: return 4;
        }
    }

    static void
        DeduplicateSymbols(
            std::vector<SymbolInfo>& Symbols)
    {
        std::sort(
            Symbols.begin(),
            Symbols.end(),
            [](const SymbolInfo& Left, const SymbolInfo& Right)
            {
                if (Left.ModuleId != Right.ModuleId)
                {
                    return Left.ModuleId < Right.ModuleId;
                }

                if (Left.Address != Right.Address)
                {
                    return Left.Address < Right.Address;
                }

                if (Left.Name != Right.Name)
                {
                    return Left.Name < Right.Name;
                }

                return GetSymbolKindPriority(Left.Kind) <
                    GetSymbolKindPriority(Right.Kind);
            });

        std::vector<SymbolInfo> Unique;
        Unique.reserve(Symbols.size());

        for (const SymbolInfo& Symbol : Symbols)
        {
            if (!Unique.empty() &&
                Unique.back().ModuleId == Symbol.ModuleId &&
                Unique.back().Address == Symbol.Address &&
                Unique.back().Name == Symbol.Name)
            {
                //
                // Existing one has better or equal priority because of sort.
                //
                continue;
            }

            Unique.push_back(Symbol);
        }

        Symbols.swap(Unique);
    }

    DiaSymbolManager::DiaSymbolManager()
    {
        (void)::CoInitializeEx(
            nullptr,
            COINIT_MULTITHREADED);
    }

    DiaSymbolManager::~DiaSymbolManager()
    {
        LoadedModules_.clear();
        ::CoUninitialize();
    }

    DebugError
        DiaSymbolManager::LoadSymbolsForModule(
            const ModuleInfo& Module)
    {
        for (const LoadedModule& Existing : LoadedModules_)
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

        LoadedModules_.push_back(std::move(Loaded));

        return DebugError::Success();
    }

    DebugError
        DiaSymbolManager::UnloadSymbolsForModule(
            ModuleId ModuleId)
    {
        const auto Iterator =
            std::find_if(
                LoadedModules_.begin(),
                LoadedModules_.end(),
                [ModuleId](const LoadedModule& Module)
                {
                    return Module.Module.Id == ModuleId;
                });

        if (Iterator == LoadedModules_.end())
        {
            return DebugError::Failure(
                ErrorCode::InvalidArgument,
                L"Module symbols were not loaded.");
        }

        LoadedModules_.erase(Iterator);

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

        //
        // 1. Function containing address.
        //
        {
            ComPtr<IDiaSymbol> DiaSymbol;

            HRESULT Hr = Loaded->Session->findSymbolByRVA(
                Rva,
                SymTagFunction,
                DiaSymbol.Put());

            if (SUCCEEDED(Hr) && DiaSymbol != nullptr)
            {
                auto Info = BuildSymbolInfo(
                    *Loaded,
                    DiaSymbol,
                    SymbolKind::Function,
                    SymTagFunction);

                if (Info.HasValue())
                {
                    const U64 Begin = Info.Value.Address;
                    const U64 End =
                        Info.Value.Size != 0 ?
                        Begin + Info.Value.Size :
                        Begin;

                    if (Info.Value.Size == 0 ||
                        (Address >= Begin && Address < End))
                    {
                        return Info;
                    }
                }
            }
        }

        //
        // 2. Exact data/public/label match.
        //
        for (const DiaSymbolTagPolicy& Policy : g_DiaSymbolSearchTags)
        {
            if (Policy.Tag == SymTagFunction)
            {
                continue;
            }

            ComPtr<IDiaSymbol> DiaSymbol;

            HRESULT Hr = Loaded->Session->findSymbolByRVA(
                Rva,
                Policy.Tag,
                DiaSymbol.Put());

            if (FAILED(Hr) || DiaSymbol == nullptr)
            {
                continue;
            }

            auto Info = BuildSymbolInfo(
                *Loaded,
                DiaSymbol,
                Policy.Kind,
                Policy.Tag);

            if (!Info.HasValue())
            {
                continue;
            }

            if (Info.Value.Address == Address ||
                (Info.Value.Size != 0 &&
                    Address >= Info.Value.Address &&
                    Address < Info.Value.Address + Info.Value.Size))
            {
                return Info;
            }
        }

        return Expected<SymbolInfo>::Failure(
            DebugError::Failure(
                ErrorCode::SymbolFailure,
                L"No symbol found for address."));
    }

    Expected<SymbolInfo>
        DiaSymbolManager::GetSymbolByName(
            const std::wstring& Name) const
    {
        for (const LoadedModule& Loaded : LoadedModules_)
        {
            for (const DiaSymbolTagPolicy& Policy : g_DiaSymbolSearchTags)
            {
                ComPtr<IDiaEnumSymbols> EnumSymbols;

                HRESULT Hr = Loaded.GlobalScope->findChildren(
                    Policy.Tag,
                    Name.c_str(),
                    nsCaseInsensitive,
                    EnumSymbols.Put());

                if (FAILED(Hr) || EnumSymbols == nullptr)
                {
                    continue;
                }

                for (;;)
                {
                    ComPtr<IDiaSymbol> DiaSymbol;
                    ULONG Fetched = 0;

                    Hr = EnumSymbols->Next(
                        1,
                        DiaSymbol.Put(),
                        &Fetched);

                    if (FAILED(Hr) || Fetched == 0 || DiaSymbol == nullptr)
                    {
                        break;
                    }

                    auto Info = BuildSymbolInfo(
                        Loaded,
                        DiaSymbol,
                        Policy.Kind,
                        Policy.Tag);

                    if (Info.HasValue())
                    {
                        return Info;
                    }
                }
            }
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
            IDiaSymbol* Symbol) const
    {
        DWORD Rva = 0;

        HRESULT Hr = Symbol->get_relativeVirtualAddress(&Rva);

        if (SUCCEEDED(Hr) && Rva != 0)
        {
            return Expected<U32>::Success(
                static_cast<U32>(Rva));
        }

        ULONGLONG Va = 0;

        Hr = Symbol->get_virtualAddress(&Va);

        if (SUCCEEDED(Hr) && Va != 0)
        {
            //
            // We cannot convert VA to RVA here unless we know module base.
            // Prefer handling this in BuildSymbolInfo if needed.
            //
        }

        DWORD LocationType = 0;
        Hr = Symbol->get_locationType(&LocationType);

        if (SUCCEEDED(Hr))
        {
            //
            // LocIsStatic usually means an addressable global/static.
            // Other location types may be register-relative/local/etc.
            // Those are not valid expression atoms without a frame context.
            //
        }

        return Expected<U32>::Failure(
            DebugError::Failure(
                ErrorCode::SymbolFailure,
                L"Symbol has no RVA"));
    }

    Expected<U64>
        DiaSymbolManager::GetSymbolRuntimeAddress(
            const LoadedModule& Loaded,
            IDiaSymbol* Symbol) const
    {
        DWORD Rva = 0;

        HRESULT Hr = Symbol->get_relativeVirtualAddress(&Rva);

        if (SUCCEEDED(Hr) && Rva != 0)
        {
            return Expected<U64>::Success(
                Loaded.Module.BaseAddress + static_cast<U64>(Rva));
        }

        ULONGLONG Va = 0;

        Hr = Symbol->get_virtualAddress(&Va);

        if (SUCCEEDED(Hr) && Va != 0)
        {
            //
            // DIA VA may already include load address if put_loadAddress()
            // was used. Accept it if it falls inside the loaded module.
            //
            const U64 RuntimeVa = static_cast<U64>(Va);

            if (RuntimeVa >= Loaded.Module.BaseAddress &&
                (Loaded.Module.Size == 0 ||
                    RuntimeVa < Loaded.Module.BaseAddress + Loaded.Module.Size))
            {
                return Expected<U64>::Success(RuntimeVa);
            }

            //
            // Some DIA data may report image-base-relative VA. If needed,
            // handle that later.
            //
        }

        return Expected<U64>::Failure(
            DebugError::Failure(
                ErrorCode::SymbolFailure,
                L"Symbol has no runtime address"));
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
        for (const LoadedModule& Loaded : LoadedModules_)
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
        for (const LoadedModule& Loaded : LoadedModules_)
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

        std::vector<SymbolInfo> Symbols;

        for (const DiaSymbolTagPolicy& Policy : g_DiaSymbolSearchTags)
        {
            ComPtr<IDiaEnumSymbols> EnumSymbols;

            HRESULT Hr = Loaded->GlobalScope->findChildren(
                Policy.Tag,
                nullptr,
                nsNone,
                EnumSymbols.Put());

            if (FAILED(Hr) || EnumSymbols == nullptr)
            {
                continue;
            }

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
                            L"Failed to enumerate DIA symbols"));
                }

                if (Fetched == 0 || DiaSymbol == nullptr)
                {
                    break;
                }

                auto Info = BuildSymbolInfo(
                    *Loaded,
                    DiaSymbol,
                    Policy.Kind,
                    Policy.Tag);

                if (!Info.HasValue())
                {
                    continue;
                }

                Symbols.push_back(std::move(Info.Value));
            }
        }

        DeduplicateSymbols(Symbols);

        std::sort(
            Symbols.begin(),
            Symbols.end(),
            [](const SymbolInfo& Left, const SymbolInfo& Right)
            {
                if (Left.Address != Right.Address)
                {
                    return Left.Address < Right.Address;
                }

                if (Left.Kind != Right.Kind)
                {
                    return static_cast<U32>(Left.Kind) <
                        static_cast<U32>(Right.Kind);
                }

                return Left.Name < Right.Name;
            });

        return Expected<std::vector<SymbolInfo>>::Success(
            std::move(Symbols));
    }

    Expected<SymbolInfo>
        DiaSymbolManager::BuildSymbolInfo(
            const LoadedModule& Loaded,
            IDiaSymbol* DiaSymbol,
            SymbolKind Kind,
            enum SymTagEnum NativeTag) const
    {
        auto SymbolName = GetSymbolName(DiaSymbol);
        auto Address = GetSymbolRuntimeAddress(Loaded, DiaSymbol);
        auto Length = GetSymbolLength(DiaSymbol);

        if (!SymbolName.HasValue() || !Address.HasValue())
        {
            return Expected<SymbolInfo>::Failure(
                DebugError::Failure(
                    ErrorCode::SymbolFailure,
                    L"DIA symbol has no usable name or address"));
        }

        SymbolInfo Info{};
        Info.ModuleId = Loaded.Module.Id;
        Info.Name = std::move(SymbolName.Value);
        Info.Address = Address.Value;
        Info.Kind = Kind;
        Info.NativeTag = static_cast<U32>(NativeTag);

        if (Length.HasValue() &&
            Length.Value <= 0xffffffffull)
        {
            Info.Size = static_cast<U32>(Length.Value);
        }

        return Expected<SymbolInfo>::Success(std::move(Info));
    }
}

