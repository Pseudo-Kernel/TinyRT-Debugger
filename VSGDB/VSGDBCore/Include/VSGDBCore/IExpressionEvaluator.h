#pragma once

#include "Types.h"
#include "Result.h"

namespace VSGDBCore
{
    enum class ExpressionValueKind
    {
        Integer,
        Bytes,
        String
    };

    struct ExpressionValue
    {
        ExpressionValueKind Kind = ExpressionValueKind::Integer;

        U64 Integer = 0;
        ByteVector Bytes;
        std::wstring String;
    };

    class IExpressionEvaluator
    {
    public:
        virtual ~IExpressionEvaluator() = default;

        virtual Result<ExpressionValue> Evaluate(
            const std::wstring& Expression) = 0;

        virtual DebugError DeclareVariable(
            const std::wstring& Name,
            const ExpressionValue& Value) = 0;

        virtual DebugError UndeclareVariable(
            const std::wstring& Name) = 0;

        virtual DebugError SetVariable(
            const std::wstring& Name,
            const ExpressionValue& Value) = 0;

        virtual Result<ExpressionValue> GetVariable(
            const std::wstring& Name) const = 0;
    };
}
