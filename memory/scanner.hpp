#pragma once

namespace memory {
	std::uintptr_t get_str_xref(const char* string);
	std::vector<std::uintptr_t> __fastcall scan_string(const char* string, bool exact = false);
}