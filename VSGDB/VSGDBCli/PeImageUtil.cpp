#include "PeImageUtil.h"

#include <windows.h>

#include <cstdio>
#include <vector>

using namespace VSGDBCore;

static Expected<std::vector<U8>>
    ReadWholeFileBytes(
        const std::wstring& Path)
{
    FILE* File = nullptr;

    if (_wfopen_s(&File, Path.c_str(), L"rb") != 0 || File == nullptr)
    {
        return Expected<std::vector<U8>>::Failure(
            DebugError::Failure(
                ErrorCode::InvalidArgument,
                L"Failed to open image file"));
    }

    std::vector<U8> Bytes;

    if (std::fseek(File, 0, SEEK_END) != 0)
    {
        std::fclose(File);
        return Expected<std::vector<U8>>::Failure(
            DebugError::Failure(
                ErrorCode::InvalidArgument,
                L"Failed to seek image file"));
    }

    const long FileSize = std::ftell(File);

    if (FileSize <= 0)
    {
        std::fclose(File);
        return Expected<std::vector<U8>>::Failure(
            DebugError::Failure(
                ErrorCode::InvalidArgument,
                L"Invalid image file size"));
    }

    std::rewind(File);

    Bytes.resize(static_cast<size_t>(FileSize));

    const size_t ReadCount =
        std::fread(Bytes.data(), 1, Bytes.size(), File);

    std::fclose(File);

    if (ReadCount != Bytes.size())
    {
        return Expected<std::vector<U8>>::Failure(
            DebugError::Failure(
                ErrorCode::InvalidArgument,
                L"Failed to read image file"));
    }

    return Expected<std::vector<U8>>::Success(std::move(Bytes));
}

Expected<U64>
    ReadPeSizeOfImage(
        const std::wstring& ImagePath)
{
    auto FileBytes = ReadWholeFileBytes(ImagePath);

    if (!FileBytes.HasValue())
    {
        return Expected<U64>::Failure(FileBytes.Error);
    }

    const std::vector<U8>& Bytes = FileBytes.Value;

    if (Bytes.size() < sizeof(IMAGE_DOS_HEADER))
    {
        return Expected<U64>::Failure(
            DebugError::Failure(
                ErrorCode::InvalidArgument,
                L"Image is too small for DOS header"));
    }

    const auto* DosHeader =
        reinterpret_cast<const IMAGE_DOS_HEADER*>(Bytes.data());

    if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        return Expected<U64>::Failure(
            DebugError::Failure(
                ErrorCode::InvalidArgument,
                L"Invalid DOS signature"));
    }

    if (DosHeader->e_lfanew < 0)
    {
        return Expected<U64>::Failure(
            DebugError::Failure(
                ErrorCode::InvalidArgument,
                L"Invalid PE header offset"));
    }

    const size_t NtOffset =
        static_cast<size_t>(DosHeader->e_lfanew);

    if (NtOffset > Bytes.size() ||
        Bytes.size() - NtOffset < sizeof(IMAGE_NT_HEADERS64))
    {
        return Expected<U64>::Failure(
            DebugError::Failure(
                ErrorCode::InvalidArgument,
                L"Image is too small for NT header"));
    }

    const auto* NtHeader =
        reinterpret_cast<const IMAGE_NT_HEADERS64*>(
            Bytes.data() + NtOffset);

    if (NtHeader->Signature != IMAGE_NT_SIGNATURE)
    {
        return Expected<U64>::Failure(
            DebugError::Failure(
                ErrorCode::InvalidArgument,
                L"Invalid NT signature"));
    }

    if (NtHeader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        return Expected<U64>::Failure(
            DebugError::Failure(
                ErrorCode::InvalidArgument,
                L"Only PE32+ images are supported"));
    }

    const U32 SizeOfImage =
        NtHeader->OptionalHeader.SizeOfImage;

    if (SizeOfImage == 0)
    {
        return Expected<U64>::Failure(
            DebugError::Failure(
                ErrorCode::InvalidArgument,
                L"PE SizeOfImage is zero"));
    }

    return Expected<U64>::Success(
        static_cast<U64>(SizeOfImage));
}
