#pragma once

#include "util.hpp"

namespace roblox::obfuscations {
	template<typename T>
	__forceinline T deobfuscate(uintptr_t address, uint16_t offset, obf_obfuscation obf, obf_direction dir) {
		if (dir == LEFT) {
			switch (obf) {
			case ADD: {
				return (T)(*reinterpret_cast<uintptr_t*>(address + offset) + (address + offset));
			}
			case SUB: {
				return (T)(*reinterpret_cast<uintptr_t*>(address + offset) - (address + offset));
			}
			case XOR: {
				return (T)(*reinterpret_cast<uintptr_t*>(address + offset) ^ (address + offset));
			}
			}
		}
		else {
			switch (obf) {
			case ADD: {
				return (T)((address + offset) + *reinterpret_cast<uintptr_t*>(address + offset));
			}
			case SUB: {
				return (T)((address + offset) - *reinterpret_cast<uintptr_t*>(address + offset));
			}
			case XOR: {
				return (T)((address + offset) ^ *reinterpret_cast<uintptr_t*>(address + offset));
			}
			}
		}
		return T();
	}
}
