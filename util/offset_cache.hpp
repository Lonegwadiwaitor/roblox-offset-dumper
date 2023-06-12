#pragma once

class spoof;

namespace util {
	void __fastcall insert(std::string_view name, const std::uintptr_t& offset);
	std::uintptr_t __fastcall find(const char* name);
}