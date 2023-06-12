#include "../pch/pch.h"

#include "rebase.hpp"

std::uintptr_t memory::rebase(const std::uintptr_t& address, const std::uintptr_t& base) {
	return address - base + reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
}

std::uintptr_t memory::unbase(const std::uintptr_t& address, const std::uintptr_t& base) {
	return (address - reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr))) + base;
}
