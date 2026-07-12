#pragma once

#include "DynamicLibrary.h"

#include <utility>

template <typename TSignature>
class DllFunction;

template <typename TRet, typename... TArgs>
class DllFunction<TRet __cdecl(TArgs...)> final
{
public:
    using FunctionPointer = TRet (__cdecl*)(TArgs...);

    DllFunction(
        const DynamicLibrary& Library,
        const char* Name)
    {
        Function_ =
            reinterpret_cast<FunctionPointer>(
                Library.GetProcAddressChecked(Name));
    }

    TRet
    operator()(
        TArgs... Arguments) const
    {
        return Function_(
            std::forward<TArgs>(Arguments)...);
    }

private:
    FunctionPointer Function_ = nullptr;
};