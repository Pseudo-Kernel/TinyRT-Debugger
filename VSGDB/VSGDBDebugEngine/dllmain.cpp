
#include <Windows.h>

#pragma comment(lib, "VSGDBCore.lib")
#pragma comment(lib, "diaguids.lib")
#pragma comment(lib, "ws2_32.lib")

#include "SessionHost.h"

extern "C"
__declspec(dllexport)
int
VsgdbDebugEngineSmokeTest(
    const wchar_t* Host,
    unsigned short Port)
{
    if (Host == nullptr || Host[0] == L'\0')
    {
        Host = L"localhost";
    }

    SessionHost HostObject;

    VSGDBCore::DebugError Error =
        HostObject.Connect(
            Host,
            static_cast<VSGDBCore::U16>(Port ? Port : 1234));

    if (!Error.IsSuccess())
    {
        return static_cast<int>(Error.Code);
    }

    Error = HostObject.Disconnect();
    if (!Error.IsSuccess())
    {
        return static_cast<int>(Error.Code);
    }

    return 0;
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

