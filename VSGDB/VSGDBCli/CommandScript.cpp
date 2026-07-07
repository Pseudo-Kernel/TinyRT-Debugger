#include "CommandScript.h"
#include "CommandProcessor.h"

#include <cstdio>
#include <fstream>
#include <string>


static std::wstring
TrimWhitespace(
    const std::wstring& Text)
{
    size_t Begin = 0;

    while (Begin < Text.size() &&
        iswspace(Text[Begin]))
    {
        ++Begin;
    }

    size_t End = Text.size();

    while (End > Begin &&
        iswspace(Text[End - 1]))
    {
        --End;
    }

    return Text.substr(Begin, End - Begin);
}

static bool
IsCommentOrEmpty(
    const std::wstring& Line)
{
    const std::wstring Trimmed =
        TrimWhitespace(Line);

    return Trimmed.empty() ||
        Trimmed[0] == L'#' ||
        Trimmed[0] == L';';
}

bool
ExecuteCommandScript(
    CommandProcessor& Processor,
    const std::wstring& ScriptPath)
{
    std::wifstream Stream(ScriptPath);

    if (!Stream.is_open())
    {
        std::wprintf(
            L"Failed to open command script: %s\n",
            ScriptPath.c_str());

        return false;
    }

    std::wstring Line;
    VSGDBCore::U32 LineNumber = 0;

    while (std::getline(Stream, Line))
    {
        ++LineNumber;

        //
        // Remove UTF-8/UTF-16 BOM if present in first line.
        //
        if (LineNumber == 1 &&
            !Line.empty() &&
            Line[0] == 0xfeff)
        {
            Line.erase(Line.begin());
        }

        const std::wstring Trimmed =
            TrimWhitespace(Line);

        if (IsCommentOrEmpty(Trimmed))
        {
            continue;
        }

        std::wprintf(
            L"%s:%u> %s\n",
            ScriptPath.c_str(),
            LineNumber,
            Trimmed.c_str());

        const bool Succeeded =
            Processor.ProcessLine(Trimmed);

        if (!Succeeded)
        {
            std::wprintf(
                L"Command script failed at %s:%u\n",
                ScriptPath.c_str(),
                LineNumber);

            return false;
        }
    }

    return true;
}
