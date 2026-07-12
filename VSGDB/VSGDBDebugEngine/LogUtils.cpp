// LogUtils.cpp
#include "LogUtils.h"

#include <Windows.h>
#include <strsafe.h>
#include <stdio.h>

static constexpr wchar_t VsgdbLogPath[] =
L"C:\\VSGDBVsix\\VSGDBDebugEngine.log";

void
VsgdbLog(
    const wchar_t* Message)
{
    if (Message == nullptr)
    {
        Message = L"(null)";
    }

    OutputDebugStringW(L"[VSGDBDebugEngine] ");
    OutputDebugStringW(Message);
    OutputDebugStringW(L"\n");

    FILE* File = nullptr;
    errno_t Error =
        _wfopen_s(
            &File,
            VsgdbLogPath,
            L"a, ccs=UTF-8");

    if (Error != 0 || File == nullptr)
    {
        return;
    }

    SYSTEMTIME Time;
    GetLocalTime(&Time);

    fwprintf(
        File,
        L"%04u-%02u-%02u %02u:%02u:%02u.%03u [TID:%lu] %s\n",
        Time.wYear,
        Time.wMonth,
        Time.wDay,
        Time.wHour,
        Time.wMinute,
        Time.wSecond,
        Time.wMilliseconds,
        GetCurrentThreadId(),
        Message);

    fclose(File);
}

void
VsgdbLogFormat(
    const wchar_t* Format,
    ...)
{
    wchar_t Buffer[1024];

    va_list Arguments;
    va_start(Arguments, Format);

    HRESULT Hr =
        StringCchVPrintfW(
            Buffer,
            _countof(Buffer),
            Format,
            Arguments);

    va_end(Arguments);

    if (FAILED(Hr))
    {
        VsgdbLog(L"VsgdbLogFormat formatting failed.");
        return;
    }

    VsgdbLog(Buffer);
}
