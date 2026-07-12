#include <Windows.h>

#include <cstdio>
#include <cstdlib>
#include <cwchar>

using VsgdbDebugEngineSmokeTestProc =
int(__cdecl*)(
    const wchar_t* Host,
    unsigned short Port);

int
wmain(
    int ArgumentCount,
    wchar_t** Arguments)
{
    const wchar_t* DllPath =
        L"VSGDBDebugEngine.dll";

    const wchar_t* Host =
        L"localhost";

    unsigned short Port =
        1234;

    if (ArgumentCount >= 2)
    {
        DllPath = Arguments[1];
    }

    if (ArgumentCount >= 3)
    {
        Host = Arguments[2];
    }

    if (ArgumentCount >= 4)
    {
        Port = static_cast<unsigned short>(
            std::wcstoul(
                Arguments[3],
                nullptr,
                10));
    }

    HMODULE Module =
        LoadLibraryW(DllPath);

    if (Module == nullptr)
    {
        std::wprintf(
            L"LoadLibraryW failed: %lu\n",
            GetLastError());

        return 1;
    }

    auto SmokeTest =
        reinterpret_cast<VsgdbDebugEngineSmokeTestProc>(
            GetProcAddress(
                Module,
                "VsgdbDebugEngineSmokeTest"));

    if (SmokeTest == nullptr)
    {
        std::wprintf(
            L"GetProcAddress failed: %lu\n",
            GetLastError());

        FreeLibrary(Module);
        return 1;
    }

    std::wprintf(
        L"Running VSGDBDebugEngine smoke test: %s:%u\n",
        Host,
        Port);

    int Result =
        SmokeTest(
            Host,
            Port);

    FreeLibrary(Module);

    if (Result != 0)
    {
        std::wprintf(
            L"Smoke test failed: %d\n",
            Result);

        return Result;
    }

    std::wprintf(
        L"Smoke test succeeded.\n");

    return 0;
}