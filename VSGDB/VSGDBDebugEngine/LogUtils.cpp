
#include <Windows.h>
#include "LogUtils.h"

void
VsgdbLog(
    const wchar_t* Message)
{
    OutputDebugStringW(L"[VSGDBDebugEngine] ");
    OutputDebugStringW(Message);
    OutputDebugStringW(L"\n");
}

