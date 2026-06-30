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
        InternalError
    };

    struct DebugError
    {
        ErrorCode Code = ErrorCode::Success;
        std::wstring Message;
        U32 NativeCode = 0;

        bool Succeeded() const
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
    struct Result
    {
        TValue Value {};
        DebugError Error;

        bool Succeeded() const
        {
            return Error.Succeeded();
        }

        static Result<TValue> Success(TValue Value)
        {
            Result<TValue> ResultValue;
            ResultValue.Value = std::move(Value);
            ResultValue.Error = DebugError::Success();
            return ResultValue;
        }

        static Result<TValue> Failure(DebugError Error)
        {
            Result<TValue> ResultValue;
            ResultValue.Error = std::move(Error);
            return ResultValue;
        }
    };
}