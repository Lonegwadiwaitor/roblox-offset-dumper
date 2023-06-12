#include "../pch/pch.h"

#include "offset_cache.hpp"

#include <map>

#include "../memory/rebase.hpp"

std::vector<std::pair<std::string, std::uintptr_t>> offset_cache;

void __fastcall util::insert(const std::string_view name, const std::uintptr_t& offset) {
	smh::println("\"", name, xor("\": 0x"), std::hex, memory::unbase(offset), std::dec);

	offset_cache.emplace_back(name, offset);
}

std::uintptr_t __fastcall util::find(const char* name) {
	for (int i = 0; i < offset_cache.size(); i++) {
		if (std::string(name) == offset_cache[i].first)
			return offset_cache[i].second;
	}

	return 0;
}