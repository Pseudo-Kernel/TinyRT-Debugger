#include <VSGDBCore/NullComponents.h>

#include <unordered_map>

namespace VSGDBCore
{
    class SimpleExpressionEvaluator final : public IExpressionEvaluator
    {
    public:
        Result<ExpressionValue> Evaluate(
            const std::wstring& Expression) override
        {
            if (Expression.empty())
            {
                return Result<ExpressionValue>::Failure(
                    DebugError::Failure(
                        ErrorCode::InvalidArgument,
                        L"Expression is empty."));
            }

            auto Variable = Variables.find(Expression);
            if (Variable != Variables.end())
            {
                return Result<ExpressionValue>::Success(Variable->second);
            }

            return Result<ExpressionValue>::Failure(
                DebugError::Failure(
                    ErrorCode::NotSupported,
                    L"Expression evaluation is not implemented yet."));
        }

        DebugError DeclareVariable(
            const std::wstring& Name,
            const ExpressionValue& Value) override
        {
            if (Name.empty())
            {
                return DebugError::Failure(
                    ErrorCode::InvalidArgument,
                    L"Variable name is empty.");
            }

            if (Variables.find(Name) != Variables.end())
            {
                return DebugError::Failure(
                    ErrorCode::InvalidArgument,
                    L"Variable already exists.");
            }

            Variables.emplace(Name, Value);
            return DebugError::Success();
        }

        DebugError UndeclareVariable(
            const std::wstring& Name) override
        {
            const size_t Removed = Variables.erase(Name);
            if (Removed == 0)
            {
                return DebugError::Failure(
                    ErrorCode::InvalidArgument,
                    L"Variable does not exist.");
            }

            return DebugError::Success();
        }

        DebugError SetVariable(
            const std::wstring& Name,
            const ExpressionValue& Value) override
        {
            auto Variable = Variables.find(Name);
            if (Variable == Variables.end())
            {
                return DebugError::Failure(
                    ErrorCode::InvalidArgument,
                    L"Variable does not exist.");
            }

            Variable->second = Value;
            return DebugError::Success();
        }

        Result<ExpressionValue> GetVariable(
            const std::wstring& Name) const override
        {
            auto Variable = Variables.find(Name);
            if (Variable == Variables.end())
            {
                return Result<ExpressionValue>::Failure(
                    DebugError::Failure(
                        ErrorCode::InvalidArgument,
                        L"Variable does not exist."));
            }

            return Result<ExpressionValue>::Success(Variable->second);
        }

    private:
        std::unordered_map<std::wstring, ExpressionValue> Variables;
    };

    std::unique_ptr<IExpressionEvaluator>
        CreateSimpleExpressionEvaluator()
    {
        return std::make_unique<SimpleExpressionEvaluator>();
    }
}