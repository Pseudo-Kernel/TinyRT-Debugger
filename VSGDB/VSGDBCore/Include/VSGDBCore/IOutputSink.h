#pragma once

#include "Types.h"

namespace VSGDBCore
{
    class IOutputSink
    {
    public:
        virtual ~IOutputSink() = default;

        virtual void Write(
            const std::wstring& Text) = 0;

        virtual void WriteLine(
            const std::wstring& Text) = 0;

        virtual void WriteError(
            const std::wstring& Text) = 0;
    };
}
