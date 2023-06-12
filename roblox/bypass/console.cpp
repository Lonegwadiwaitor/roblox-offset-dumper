#include "../../pch/pch.h"

#include "console.hpp"

void __fastcall roblox::bypass::console(LPCWSTR console_name) {
    AllocConsole();
    
    SetConsoleTitleW(console_name);
    freopen_s(&safe_handle_stream, xor("CONIN$"), "r", stdin);
    freopen_s(&safe_handle_stream, xor("CONOUT$"), "w", stdout);
    freopen_s(&safe_handle_stream, xor("CONOUT$"), "w", stderr);
}
