#include "../pch/pch.h"
#include "capstone_engine.hpp"

bool capstone_engine::is_function(const std::uintptr_t& address) {
	if (address % 0x10 != 0) /* compilers will never have "0x1532455" as a function, "0x1532450" could be though */
		return false;

	constinit static std::array<std::uint8_t, 3> bytes{};

	if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(address), bytes.data(), 3, nullptr)) {
		return false;
	}

	// run though common stack frames
	if (bytes[0] == 0x8B && bytes[1] == 0xFF && bytes[2] == 0x55)
		return true;

	if (bytes[0] == 0x53 && bytes[1] == 0x8B && bytes[2] == 0xDC)
		return true;

	if (bytes[0] == 0x55 && bytes[1] == 0x8B && bytes[2] == 0xEC)
		return true;

	if (bytes[0] == 0x56 && bytes[1] == 0x8B && bytes[2] == 0xF4)
		return true;

	if (bytes[0] == 0x56 && bytes[1] == 0x8B && bytes[2] == 0xF1)
		return true;

	if (bytes[0] == 0x57 && bytes[1] == 0x8B && bytes[2] == 0xFC)
		return true;

	return false;
}

std::vector<std::uint8_t> capstone_engine::get_func_bytes(const std::uintptr_t& address) {
	std::vector<std::uint8_t> bytes{};

	const uintptr_t& end = get_prologue<next>(address);

	const auto func_sz = end - address;

	char* buffer = new char[func_sz];

	ReadProcessMemory(GetCurrentProcess(), (LPCVOID)address, buffer, func_sz, nullptr);

	bytes.reserve(func_sz);

	for (int i = 0; i < func_sz; i++) {
		bytes.push_back(buffer[i]);
	}

	return bytes;
}
