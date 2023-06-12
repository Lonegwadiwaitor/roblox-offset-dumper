#pragma once

#include "util.hpp"

namespace roblox::obfuscations {
	/* 0: ADD, 1: SUB, 2, XOR */
	/* 0: LEFT, 1: RIGHT */
	template<typename T, typename T2>
	__forceinline T obfuscate(uintptr_t address, uint16_t offset, T2 data, obf_obfuscation obf, obf_direction dir) {
		if (dir == LEFT) {
			switch (obf) {
			case ADD: {
				return (T)((uintptr_t)(data)+(address + offset));
			}
			case SUB: {
				return (T)((uintptr_t)(data)-(address + offset));
			}
			case XOR: {
				return (T)((uintptr_t)(data) ^ (address + offset));
			}
			}
		}
		else {
			switch (obf) {
			case ADD: {
				return (T)((address + offset) + (uintptr_t)(data));
			}
			case SUB: {
				return (T)((address + offset) - (uintptr_t)(data));
			}
			case XOR: {
				return (T)((address + offset) ^ (uintptr_t)(data));
			}
			}
		}
	}
}
