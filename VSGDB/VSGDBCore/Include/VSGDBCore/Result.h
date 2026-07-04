#pragma once

#include "Types.h"

namespace VSGDBCore
{
    enum class ErrorCode
    {
        Success,
        InvalidArgument,
        NotConnected,
        BackendFailure,
        SymbolFailure,
        MemoryReadFailure,
        MemoryWriteFailure,
        RegisterReadFailure,
        RegisterWriteFailure,
        BreakpointFailure,
        NotSupported,
        InvalidState,
        InternalError
    };

    struct DebugError
    {
        ErrorCode Code = ErrorCode::Success;
        std::wstring Message;
        U32 NativeCode = 0;

        bool IsSuccess() const
        {
            return Code == ErrorCode::Success;
        }

        static DebugError Success()
        {
            return {};
        }

        static DebugError Failure(
            ErrorCode Code,
            std::wstring Message,
            U32 NativeCode = 0)
        {
            DebugError Error;
            Error.Code = Code;
            Error.Message = std::move(Message);
            Error.NativeCode = NativeCode;
            return Error;
        }
    };

    template <typename TValue>
    struct Expected
    {
        TValue Value {};
        DebugError Error;

        bool HasValue() const
        {
            return Error.IsSuccess();
        }

        static Expected<TValue> Success(TValue Value)
        {
            Expected<TValue> ResultValue;
            ResultValue.Value = std::move(Value);
            ResultValue.Error = DebugError::Success();
            return ResultValue;
        }

        static Expected<TValue> Failure(DebugError Error)
        {
            Expected<TValue> ResultValue;
            ResultValue.Error = std::move(Error);
            return ResultValue;
        }
    };
}