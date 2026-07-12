#include <Windows.h>

#include <cstdio>
#include <cstdlib>
#include <cwchar>

#include "DynamicLibrary.h"
#include "DllFunction.h"


int
wmain(
    int ArgumentCount,
    wchar_t** Arguments)
{
    try
    {
        DynamicLibrary EngineDll(
            L"VSGDBDebugEngine.dll");

        DllFunction<int __cdecl(const wchar_t*, unsigned short)>
            EngineSmokeTest(
                EngineDll,
                "VsgdbDebugEngineSmokeTest");

        DllFunction<int __cdecl()>
            ProgramNodeSmokeTest(
                EngineDll,
                "VsgdbDebugProgramNodeSmokeTest");

        int Result =
            EngineSmokeTest(
                L"127.0.0.1",
                1234);

        if (Result != 0)
        {
            return Result;
        }

        Result = ProgramNodeSmokeTest();

        if (Result != 0)
        {
            return Result;
        }

        std::wprintf(
            L"Smoke test succeeded.\n");
    }
    catch (const std::exception& Exception)
    {
        std::printf(
            "Smoke test failed: %s\n",
            Exception.what());

        return 1;
    }

    return 0;
}

