#pragma once

namespace memory {
	std::uintptr_t rebase(const std::uintptr_t& address, const std::uintptr_t& base = 0x400000);

	std::uintptr_t unbase(const std::uintptr_t& address, const std::uintptr_t& base = 0x400000);
}