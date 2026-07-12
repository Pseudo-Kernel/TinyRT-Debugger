
#include <Windows.h>
#include <atomic>

#include "SessionHost.h"
#include "LogUtils.h"

#pragma comment(lib, "VSGDBCore.lib")
#pragma comment(lib, "diaguids.lib")
#pragma comment(lib, "ws2_32.lib")

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "uuid.lib")


std::atomic<ULONG> g_DllObjectCount = 0;
std::atomic<ULONG> g_DllLockCount = 0;


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        VsgdbLog(L"DLL_PROCESS_ATTACH");
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        VsgdbLog(L"DLL_PROCESS_DETACH");
        break;
    }

    return TRUE;
}

