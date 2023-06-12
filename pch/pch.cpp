// pch.cpp: source file corresponding to the pre-compiled header

#include "pch.h"

#include "../src/main.hpp"

// When you are using pre-compiled headers, this source file is necessary for compilation to succeed.

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(nullptr, NULL, [](LPVOID)->DWORD { return main(); }, nullptr, NULL, nullptr);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}