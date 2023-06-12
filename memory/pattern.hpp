#pragma once

namespace memory {
	std::uintptr_t __fastcall scan(void* module, const std::vector<int>& signature);
	std::uintptr_t __fastcall scan_range(void* module, const std::vector<int>& signature, const std::uintptr_t& start, const std::uintptr_t& end, bool exact = false);
	std::uintptr_t __fastcall scan_range_int(void* module, int signature, std::uintptr_t start, std::uintptr_t end);
	std::uintptr_t __fastcall pattern_scan(const char* signature);
	std::vector<std::uintptr_t> __fastcall safe_ptr_scan_range(const std::uintptr_t& ptr, const std::uintptr_t& start, const std::uintptr_t& end);
	std::uintptr_t __fastcall str_xref_scan(const char* str, const std::uintptr_t& rdata_start, const std::uintptr_t& rdata_end, bool exact = false);
}