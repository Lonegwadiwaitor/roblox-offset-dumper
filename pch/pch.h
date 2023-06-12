// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include <Windows.h>
#include <iostream>
#include <array>
#include <vector>
#include <filesystem>
#include <cstdint>
#include <string>
#include <cmath>
#include <Psapi.h>
#include <TlHelp32.h>
#include <MinHook.h>

#include <functional>


static FILE* safe_handle_stream; /* our std handle */

#include "../smh/cout.hpp" // output
#include "../security/xor.hpp" // security
#include <nlohmann/json.hpp>

#endif //PCH_H
