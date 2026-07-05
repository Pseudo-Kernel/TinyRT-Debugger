#include "SymbolQuery.h"

#include <cwctype>

static wchar_t
ToLowerWide(
    wchar_t Character)
{
    return static_cast<wchar_t>(
        std::towlower(Character));
}

static bool
EqualsIgnoreCase(
    const std::wstring& Left,
    const std::wstring& Right)
{
    if (Left.size() != Right.size())
    {
        return false;
    }

    for (size_t Index = 0; Index < Left.size(); ++Index)
    {
        if (ToLowerWide(Left[Index]) !=
            ToLowerWide(Right[Index]))
        {
            return false;
        }
    }

    return true;
}

static bool
StartsWithIgnoreCase(
    const std::wstring& Text,
    const std::wstring& Prefix)
{
    if (Prefix.size() > Text.size())
    {
        return false;
    }

    for (size_t Index = 0; Index < Prefix.size(); ++Index)
    {
        if (ToLowerWide(Text[Index]) !=
            ToLowerWide(Prefix[Index]))
        {
            return false;
        }
    }

    return true;
}

static bool
EndsWithIgnoreCase(
    const std::wstring& Text,
    const std::wstring& Suffix)
{
    if (Suffix.size() > Text.size())
    {
        return false;
    }

    const size_t Offset =
        Text.size() - Suffix.size();

    for (size_t Index = 0; Index < Suffix.size(); ++Index)
    {
        if (ToLowerWide(Text[Offset + Index]) !=
            ToLowerWide(Suffix[Index]))
        {
            return false;
        }
    }

    return true;
}

static bool
ContainsIgnoreCase(
    const std::wstring& Text,
    const std::wstring& Pattern)
{
    if (Pattern.empty())
    {
        return true;
    }

    if (Pattern.size() > Text.size())
    {
        return false;
    }

    for (size_t Start = 0;
        Start <= Text.size() - Pattern.size();
        ++Start)
    {
        bool Matched = true;

        for (size_t Index = 0; Index < Pattern.size(); ++Index)
        {
            if (ToLowerWide(Text[Start + Index]) !=
                ToLowerWide(Pattern[Index]))
            {
                Matched = false;
                break;
            }
        }

        if (Matched)
        {
            return true;
        }
    }

    return false;
}

SymbolQuery
ParseSymbolQuery(
    const std::wstring& Query)
{
    if (Query == L"*" || Query == L"**")
    {
        return { SymbolMatchKind::All, L"" };
    }

    const bool HasLeadingStar =
        !Query.empty() &&
        Query.front() == L'*';

    const bool HasTrailingStar =
        !Query.empty() &&
        Query.back() == L'*';

    if (HasLeadingStar &&
        HasTrailingStar &&
        Query.size() >= 2)
    {
        return {
            SymbolMatchKind::Contains,
            Query.substr(1, Query.size() - 2)
        };
    }

    if (HasTrailingStar)
    {
        return {
            SymbolMatchKind::StartsWith,
            Query.substr(0, Query.size() - 1)
        };
    }

    if (HasLeadingStar)
    {
        return {
            SymbolMatchKind::EndsWith,
            Query.substr(1)
        };
    }

    return { SymbolMatchKind::Exact, Query };
}

bool
MatchesSymbolQuery(
    const std::wstring& SymbolName,
    const SymbolQuery& Query)
{
    switch (Query.Kind)
    {
    case SymbolMatchKind::Exact:
        return EqualsIgnoreCase(
            SymbolName,
            Query.Pattern);

    case SymbolMatchKind::StartsWith:
        return StartsWithIgnoreCase(
            SymbolName,
            Query.Pattern);

    case SymbolMatchKind::EndsWith:
        return EndsWithIgnoreCase(
            SymbolName,
            Query.Pattern);

    case SymbolMatchKind::Contains:
        return ContainsIgnoreCase(
            SymbolName,
            Query.Pattern);

    case SymbolMatchKind::All:
        return true;

    default:
        return false;
    }
}

bool
IsSymbolEnumerationQuery(
    const SymbolQuery& Query)
{
    return Query.Kind != SymbolMatchKind::Exact;
}