#pragma once

#include <string>

enum class SymbolMatchKind
{
    Exact,
    StartsWith,
    EndsWith,
    Contains,
    All
};

struct SymbolQuery
{
    SymbolMatchKind Kind = SymbolMatchKind::Exact;
    std::wstring Pattern;
};

SymbolQuery
ParseSymbolQuery(
    const std::wstring& Query);

bool
MatchesSymbolQuery(
    const std::wstring& SymbolName,
    const SymbolQuery& Query);

bool
IsSymbolEnumerationQuery(
    const SymbolQuery& Query);