#pragma once

extern constinit std::array<std::string_view, 18> tms;
extern constinit std::array<std::string_view, 10> types;

namespace roblox::shuffles {
	void __fastcall init();
	void __fastcall init_vm_shuffle5(std::uintptr_t global);
	void __fastcall init_vm_shuffle6(std::uintptr_t ls, std::uint8_t capacity);
	void __fastcall init_vm_shuffle8(std::uintptr_t proto);
	void __fastcall deobfuscate(std::vector<std::uint16_t*> offsets);

	FORCEINLINE std::vector<uint8_t> get_tt(std::string_view type) {
		std::vector<uint8_t> results{};

		for (uint8_t i = 1; i < 10; i++) {
			if (types[i] == type)
				results.emplace_back(i);
		}

		return results;
	}

	FORCEINLINE uint8_t get_tms(std::string_view type) {
		int i = 0;
		for (const auto & tm : tms) {
			if (tm == type)
				return i;

			i++;
		}
	}
}