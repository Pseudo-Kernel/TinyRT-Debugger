#pragma once

#include <Windows.h>

#include <stdexcept>
#include <string>

class DynamicLibrary final
{
public:
    explicit DynamicLibrary(
        const wchar_t* Path)
    {
        Module_ = LoadLibraryW(Path);

        if (Module_ == nullptr)
        {
            throw std::runtime_error(
                "LoadLibraryW failed.");
        }
    }

    ~DynamicLibrary()
    {
        if (Module_ != nullptr)
        {
            FreeLibrary(Module_);
            Module_ = nullptr;
        }
    }

    DynamicLibrary(
        const DynamicLibrary&) = delete;

    DynamicLibrary&
    operator=(
        const DynamicLibrary&) = delete;

    DynamicLibrary(
        DynamicLibrary&& Other) noexcept
    {
        Module_ = Other.Module_;
        Other.Module_ = nullptr;
    }

    DynamicLibrary&
    operator=(
        DynamicLibrary&& Other) noexcept
    {
        if (this != &Other)
        {
            if (Module_ != nullptr)
            {
                FreeLibrary(Module_);
            }

            Module_ = Other.Module_;
            Other.Module_ = nullptr;
        }

        return *this;
    }

    FARPROC
    GetProcAddressChecked(
        const char* Name) const
    {
        FARPROC Procedure =
            GetProcAddress(
                Module_,
                Name);

        if (Procedure == nullptr)
        {
            throw std::runtime_error(
                "GetProcAddress failed.");
        }

        return Procedure;
    }

private:
    HMODULE Module_ = nullptr;
};