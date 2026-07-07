#include "X64PeUnwindData.h"

#include <algorithm>
#include <fstream>

#pragma push_macro("NOMINMAX")
#define NOMINMAX
#include <windows.h>
#pragma pop_macro("NOMINMAX")

namespace VSGDBCore
{
    static U32
        ReadU32(
            const U8* Data)
    {
        return static_cast<U32>(Data[0]) |
            (static_cast<U32>(Data[1]) << 8) |
            (static_cast<U32>(Data[2]) << 16) |
            (static_cast<U32>(Data[3]) << 24);
    }

    static Expected<std::vector<U8>>
        ReadWholeFile(
            const std::wstring& Path)
    {
        std::ifstream Stream(
            Path,
            std::ios::binary);

        if (!Stream.is_open())
        {
            return Expected<std::vector<U8>>::Failure(
                DebugError::Failure(
                    ErrorCode::SymbolFailure,
                    L"Failed to open module image"));
        }

        Stream.seekg(0, std::ios::end);
        const std::streamoff Size = Stream.tellg();
        Stream.seekg(0, std::ios::beg);

        if (Size <= 0)
        {
            return Expected<std::vector<U8>>::Failure(
                DebugError::Failure(
                    ErrorCode::SymbolFailure,
                    L"Module image is empty"));
        }

        std::vector<U8> Bytes(
            static_cast<size_t>(Size));

        Stream.read(
            reinterpret_cast<char*>(Bytes.data()),
            Size);

        if (!Stream)
        {
            return Expected<std::vector<U8>>::Failure(
                DebugError::Failure(
                    ErrorCode::SymbolFailure,
                    L"Failed to read module image"));
        }

        return Expected<std::vector<U8>>::Success(
            std::move(Bytes));
    }

    static const IMAGE_NT_HEADERS64*
        GetNtHeaders64(
            const std::vector<U8>& Image)
    {
        if (Image.size() < sizeof(IMAGE_DOS_HEADER))
        {
            return nullptr;
        }

        const auto* Dos =
            reinterpret_cast<const IMAGE_DOS_HEADER*>(Image.data());

        if (Dos->e_magic != IMAGE_DOS_SIGNATURE)
        {
            return nullptr;
        }

        if (Dos->e_lfanew < 0 ||
            static_cast<size_t>(Dos->e_lfanew) + sizeof(IMAGE_NT_HEADERS64) >
            Image.size())
        {
            return nullptr;
        }

        const auto* Nt =
            reinterpret_cast<const IMAGE_NT_HEADERS64*>(
                Image.data() + Dos->e_lfanew);

        if (Nt->Signature != IMAGE_NT_SIGNATURE)
        {
            return nullptr;
        }

        if (Nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        {
            return nullptr;
        }

        return Nt;
    }

    static const IMAGE_SECTION_HEADER*
        GetFirstSection(
            const IMAGE_NT_HEADERS64* Nt)
    {
        return IMAGE_FIRST_SECTION(
            const_cast<IMAGE_NT_HEADERS64*>(Nt));
    }

    static const IMAGE_SECTION_HEADER*
        FindSectionByRva(
            const IMAGE_NT_HEADERS64* Nt,
            U32 Rva)
    {
        const IMAGE_SECTION_HEADER* Section =
            GetFirstSection(Nt);

        for (U16 Index = 0;
            Index < Nt->FileHeader.NumberOfSections;
            ++Index)
        {
            const U32 Begin = Section[Index].VirtualAddress;
            const U32 RawSize = Section[Index].SizeOfRawData;
            const U32 VirtualSize = Section[Index].Misc.VirtualSize;
            const U32 Size = std::max(RawSize, VirtualSize);

            if (Rva >= Begin && Rva < Begin + Size)
            {
                return &Section[Index];
            }
        }

        return nullptr;
    }

    static bool
        RvaToFileOffset(
            const std::vector<U8>& Image,
            const IMAGE_NT_HEADERS64* Nt,
            U32 Rva,
            U32 Size,
            U32* OutOffset)
    {
        const IMAGE_SECTION_HEADER* Section =
            FindSectionByRva(Nt, Rva);

        if (Section == nullptr)
        {
            return false;
        }

        const U32 Delta =
            Rva - Section->VirtualAddress;

        if (Delta > Section->SizeOfRawData)
        {
            return false;
        }

        const U32 Offset =
            Section->PointerToRawData + Delta;

        if (static_cast<U64>(Offset) + Size > Image.size())
        {
            return false;
        }

        *OutOffset = Offset;
        return true;
    }
}

namespace VSGDBCore
{
    Expected<X64UnwindModuleData>
        X64PeUnwindData::LoadModule(
            const ModuleInfo& Module) const
    {
        if (Module.ImagePath.empty())
        {
            return Expected<X64UnwindModuleData>::Failure(
                DebugError::Failure(
                    ErrorCode::InvalidArgument,
                    L"Module has no image path"));
        }

        auto ImageBytes = ReadWholeFile(Module.ImagePath);

        if (!ImageBytes.HasValue())
        {
            return Expected<X64UnwindModuleData>::Failure(
                ImageBytes.Error);
        }

        const IMAGE_NT_HEADERS64* Nt =
            GetNtHeaders64(ImageBytes.Value);

        if (Nt == nullptr)
        {
            return Expected<X64UnwindModuleData>::Failure(
                DebugError::Failure(
                    ErrorCode::SymbolFailure,
                    L"Invalid x64 PE image"));
        }

        const IMAGE_DATA_DIRECTORY& ExceptionDirectory =
            Nt->OptionalHeader.DataDirectory[
                IMAGE_DIRECTORY_ENTRY_EXCEPTION];

        if (ExceptionDirectory.VirtualAddress == 0 ||
            ExceptionDirectory.Size == 0)
        {
            return Expected<X64UnwindModuleData>::Failure(
                DebugError::Failure(
                    ErrorCode::SymbolFailure,
                    L"Module has no x64 exception directory"));
        }

        U32 ExceptionOffset = 0;

        if (!RvaToFileOffset(
            ImageBytes.Value,
            Nt,
            ExceptionDirectory.VirtualAddress,
            ExceptionDirectory.Size,
            &ExceptionOffset))
        {
            return Expected<X64UnwindModuleData>::Failure(
                DebugError::Failure(
                    ErrorCode::SymbolFailure,
                    L"Failed to map exception directory"));
        }

        constexpr U32 SizeOfRuntimeFunctionEntry = sizeof(X64RuntimeFunction);

        if ((ExceptionDirectory.Size % SizeOfRuntimeFunctionEntry) != 0)
        {
            return Expected<X64UnwindModuleData>::Failure(
                DebugError::Failure(
                    ErrorCode::SymbolFailure,
                    L"Invalid exception directory size"));
        }

        std::vector<X64RuntimeFunction> RuntimeFunctions;

        const U32 Count =
            ExceptionDirectory.Size / SizeOfRuntimeFunctionEntry;

        RuntimeFunctions.reserve(Count);

        const U8* Pdata =
            ImageBytes.Value.data() + ExceptionOffset;

        for (U32 Index = 0; Index < Count; ++Index)
        {
            const U8* Entry =
                Pdata + Index * SizeOfRuntimeFunctionEntry;

            X64RuntimeFunction Function{};
            Function.BeginAddress = ReadU32(Entry + 0);
            Function.EndAddress = ReadU32(Entry + 4);
            Function.UnwindInfoAddress = ReadU32(Entry + 8);

            if (Function.BeginAddress < Function.EndAddress)
            {
                RuntimeFunctions.push_back(Function);
            }
        }

        std::sort(
            RuntimeFunctions.begin(),
            RuntimeFunctions.end(),
            [](const X64RuntimeFunction& Left,
                const X64RuntimeFunction& Right)
            {
                return Left.BeginAddress < Right.BeginAddress;
            });

        X64UnwindModuleData Data{};
        Data.Id = Module.Id;
        Data.BaseAddress = Module.BaseAddress;
        Data.Size = Module.Size;
        Data.ImagePath = Module.ImagePath;
        Data.ImageBytes = std::move(ImageBytes.Value);
        Data.RuntimeFunctions = std::move(RuntimeFunctions);

        return Expected<X64UnwindModuleData>::Success(
            std::move(Data));
    }

    Expected<const X64UnwindModuleData*>
        X64PeUnwindData::GetOrLoadModule(
            const ModuleInfo& Module)
    {
        const auto Existing =
            Modules.find(Module.Id);

        if (Existing != Modules.end())
        {
            return Expected<const X64UnwindModuleData*>::Success(
                &Existing->second);
        }

        auto Loaded = LoadModule(Module);

        if (!Loaded.HasValue())
        {
            return Expected<const X64UnwindModuleData*>::Failure(
                Loaded.Error);
        }

        auto Inserted = Modules.emplace(
            Module.Id,
            std::move(Loaded.Value));

        return Expected<const X64UnwindModuleData*>::Success(
            &Inserted.first->second);
    }
}

namespace VSGDBCore
{
    Expected<X64RuntimeFunction>
        X64PeUnwindData::LookupRuntimeFunction(
            const ModuleInfo& Module,
            U64 ControlPc)
    {
        auto ModuleData = GetOrLoadModule(Module);

        if (!ModuleData.HasValue())
        {
            return Expected<X64RuntimeFunction>::Failure(
                ModuleData.Error);
        }

        if (ControlPc < Module.BaseAddress)
        {
            return Expected<X64RuntimeFunction>::Failure(
                DebugError::Failure(
                    ErrorCode::InvalidArgument,
                    L"ControlPc is below module base"));
        }

        const U64 Rva64 =
            ControlPc - Module.BaseAddress;

        if (Rva64 > 0xffffffffull)
        {
            return Expected<X64RuntimeFunction>::Failure(
                DebugError::Failure(
                    ErrorCode::InvalidArgument,
                    L"ControlPc RVA is too large"));
        }

        const U32 Rva =
            static_cast<U32>(Rva64);

        const auto& Functions =
            ModuleData.Value->RuntimeFunctions;

        auto It = std::upper_bound(
            Functions.begin(),
            Functions.end(),
            Rva,
            [](U32 Value, const X64RuntimeFunction& Function)
            {
                return Value < Function.BeginAddress;
            });

        if (It == Functions.begin())
        {
            return Expected<X64RuntimeFunction>::Failure(
                DebugError::Failure(
                    ErrorCode::SymbolFailure,
                    L"No runtime function found"));
        }

        --It;

        if (Rva >= It->BeginAddress &&
            Rva < It->EndAddress)
        {
            return Expected<X64RuntimeFunction>::Success(
                *It);
        }

        return Expected<X64RuntimeFunction>::Failure(
            DebugError::Failure(
                ErrorCode::SymbolFailure,
                L"No runtime function found"));
    }
}

namespace VSGDBCore
{
    const U8*
        X64PeUnwindData::RvaToPtr(
            const X64UnwindModuleData& ModuleData,
            U32 Rva,
            U32 Size) const
    {
        const IMAGE_NT_HEADERS64* Nt =
            GetNtHeaders64(ModuleData.ImageBytes);

        if (Nt == nullptr)
        {
            return nullptr;
        }

        U32 Offset = 0;

        if (!RvaToFileOffset(
            ModuleData.ImageBytes,
            Nt,
            Rva,
            Size,
            &Offset))
        {
            return nullptr;
        }

        return ModuleData.ImageBytes.data() + Offset;
    }

    Expected<X64UnwindInfo>
        X64PeUnwindData::ReadUnwindInfo(
            const X64UnwindModuleData& ModuleData,
            U32 UnwindInfoRva) const
    {
        const U8* Header =
            RvaToPtr(ModuleData, UnwindInfoRva, 4);

        if (Header == nullptr)
        {
            return Expected<X64UnwindInfo>::Failure(
                DebugError::Failure(
                    ErrorCode::SymbolFailure,
                    L"Failed to map unwind info"));
        }

        X64UnwindInfo Info{};
        Info.Version = Header[0] & 0x7;
        Info.Flags = Header[0] >> 3;
        Info.SizeOfProlog = Header[1];
        Info.CountOfCodes = Header[2];
        Info.FrameRegister = Header[3] & 0x0f;
        Info.FrameOffset = Header[3] >> 4;

        if (Info.Version != 1)
        {
            return Expected<X64UnwindInfo>::Failure(
                DebugError::Failure(
                    ErrorCode::SymbolFailure,
                    L"Unsupported unwind info version"));
        }

        const U32 CodesSize =
            static_cast<U32>(Info.CountOfCodes) * 2;

        const U8* Codes =
            RvaToPtr(ModuleData, UnwindInfoRva + 4, CodesSize);

        if (Codes == nullptr)
        {
            return Expected<X64UnwindInfo>::Failure(
                DebugError::Failure(
                    ErrorCode::SymbolFailure,
                    L"Failed to map unwind codes"));
        }

        for (U8 Index = 0; Index < Info.CountOfCodes; ++Index)
        {
            const U8* Code = Codes + Index * 2;

            X64UnwindCode Item{};
            Item.Raw0 = Code[0];
            Item.Raw1 = Code[1];
            Item.CodeOffset = Code[0];
            Item.UnwindOp = Code[1] & 0x0f;
            Item.OpInfo = Code[1] >> 4;

            Info.Codes.push_back(Item);
        }

        return Expected<X64UnwindInfo>::Success(
            std::move(Info));
    }
}