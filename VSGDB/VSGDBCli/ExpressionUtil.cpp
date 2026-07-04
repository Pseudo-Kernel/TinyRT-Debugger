#include "ExpressionUtil.h"

#include <cwctype>
#include <cstdlib>

static std::wstring
ToLowerString(
    std::wstring Text)
{
    for (wchar_t& Character : Text)
    {
        Character = static_cast<wchar_t>(
            std::towlower(Character));
    }

    return Text;
}

bool
ParseU64(
    const std::wstring& Text,
    VSGDBCore::U64& OutValue)
{
    if (Text.empty())
    {
        return false;
    }

    wchar_t* End = nullptr;

    const unsigned long long Value = std::wcstoull(
        Text.c_str(),
        &End,
        0);

    if (End == Text.c_str() || *End != L'\0')
    {
        return false;
    }

    OutValue = static_cast<VSGDBCore::U64>(Value);
    return true;
}

static VSGDBCore::Expected<VSGDBCore::U64>
EvaluateRegisterName(
    VSGDBCore::GdbRemoteTarget& Target,
    VSGDBCore::U32 CpuId,
    const std::wstring& Name)
{
    const std::wstring LowerName = ToLowerString(Name);

    auto Registers = Target.GetRegisters(CpuId);
    if (!Registers.HasValue())
    {
        return VSGDBCore::Expected<VSGDBCore::U64>::Failure(
            Registers.Error);
    }

    const auto& R = Registers.Value;

    if (LowerName == L"rax") return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.Rax);
    if (LowerName == L"rbx") return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.Rbx);
    if (LowerName == L"rcx") return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.Rcx);
    if (LowerName == L"rdx") return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.Rdx);
    if (LowerName == L"rsi") return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.Rsi);
    if (LowerName == L"rdi") return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.Rdi);
    if (LowerName == L"rbp") return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.Rbp);
    if (LowerName == L"rsp") return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.Rsp);

    if (LowerName == L"r8")  return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.R8);
    if (LowerName == L"r9")  return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.R9);
    if (LowerName == L"r10") return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.R10);
    if (LowerName == L"r11") return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.R11);
    if (LowerName == L"r12") return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.R12);
    if (LowerName == L"r13") return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.R13);
    if (LowerName == L"r14") return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.R14);
    if (LowerName == L"r15") return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.R15);

    if (LowerName == L"rip")    return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.Rip);
    if (LowerName == L"rflags") return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.Rflags);
    if (LowerName == L"eflags") return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.Rflags);

    if (LowerName == L"cs") return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.Cs);
    if (LowerName == L"ss") return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.Ss);
    if (LowerName == L"ds") return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.Ds);
    if (LowerName == L"es") return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.Es);
    if (LowerName == L"fs") return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.Fs);
    if (LowerName == L"gs") return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.Gs);

    if (LowerName == L"fsbase")  return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.FsBase);
    if (LowerName == L"gsbase")  return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.GsBase);
    if (LowerName == L"kgsbase") return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.KernelGsBase);

    if (LowerName == L"cr0")  return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.Cr0);
    if (LowerName == L"cr2")  return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.Cr2);
    if (LowerName == L"cr3")  return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.Cr3);
    if (LowerName == L"cr4")  return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.Cr4);
    if (LowerName == L"cr8")  return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.Cr8);
    if (LowerName == L"efer") return VSGDBCore::Expected<VSGDBCore::U64>::Success(R.Efer);

    return VSGDBCore::Expected<VSGDBCore::U64>::Failure(
        VSGDBCore::DebugError::Failure(
            VSGDBCore::ErrorCode::InvalidArgument,
            L"Unknown register name."));
}

static VSGDBCore::Expected<VSGDBCore::U64>
EvaluateAtom(
    VSGDBCore::GdbRemoteTarget& Target,
    VSGDBCore::U32 CpuId,
    const std::wstring& Atom)
{
    VSGDBCore::U64 Value = 0;

    if (ParseU64(Atom, Value))
    {
        return VSGDBCore::Expected<VSGDBCore::U64>::Success(Value);
    }

    return EvaluateRegisterName(
        Target,
        CpuId,
        Atom);
}

VSGDBCore::Expected<VSGDBCore::U64>
EvaluateSimpleExpression(
    VSGDBCore::GdbRemoteTarget& Target,
    VSGDBCore::U32 CpuId,
    const std::wstring& Expression)
{
    if (Expression.empty())
    {
        return VSGDBCore::Expected<VSGDBCore::U64>::Failure(
            VSGDBCore::DebugError::Failure(
                VSGDBCore::ErrorCode::InvalidArgument,
                L"Expression is empty."));
    }

    size_t OperatorPosition = std::wstring::npos;
    wchar_t Operator = 0;

    //
    // Find the first + or - that is not the leading sign of a numeric literal.
    //
    for (size_t Index = 1; Index < Expression.size(); ++Index)
    {
        if (Expression[Index] == L'+' ||
            Expression[Index] == L'-')
        {
            OperatorPosition = Index;
            Operator = Expression[Index];
            break;
        }
    }

    if (OperatorPosition == std::wstring::npos)
    {
        return EvaluateAtom(
            Target,
            CpuId,
            Expression);
    }

    const std::wstring LeftText =
        Expression.substr(0, OperatorPosition);

    const std::wstring RightText =
        Expression.substr(OperatorPosition + 1);

    if (LeftText.empty() || RightText.empty())
    {
        return VSGDBCore::Expected<VSGDBCore::U64>::Failure(
            VSGDBCore::DebugError::Failure(
                VSGDBCore::ErrorCode::InvalidArgument,
                L"Malformed expression."));
    }

    auto Left = EvaluateAtom(
        Target,
        CpuId,
        LeftText);

    if (!Left.HasValue())
    {
        return Left;
    }

    auto Right = EvaluateAtom(
        Target,
        CpuId,
        RightText);

    if (!Right.HasValue())
    {
        return Right;
    }

    if (Operator == L'+')
    {
        return VSGDBCore::Expected<VSGDBCore::U64>::Success(
            Left.Value + Right.Value);
    }

    return VSGDBCore::Expected<VSGDBCore::U64>::Success(
        Left.Value - Right.Value);
}