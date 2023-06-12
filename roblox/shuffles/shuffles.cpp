#include "../../pch/pch.h"

#include "shuffles.hpp"

#include <cryptopp/rng.h>

#include "structs.hpp"
#include "../../memory/routine_mgr.hpp"  

#include "../../memory/pattern.hpp"

#include "../obfuscations/bruteforce.hpp"

#include "../lua/compiler/Compilerator.h"
#include "../lua/mini_api/api.hpp"

constinit std::array<std::string_view, 18> tms{};  // NOLINT(clang-diagnostic-microsoft-redeclare-static)
constinit std::array<std::string_view, 10> types{};  // NOLINT(clang-diagnostic-microsoft-redeclare-static)

static constinit std::array<std::uint8_t, 2> vm_shuffle_2{};
static constinit std::array<std::uint8_t, 3> vm_shuffle_3{};
static constinit std::array<std::uint8_t, 4> vm_shuffle_4{};
static constinit std::array<std::uint8_t, 5> vm_shuffle_5{};
static constinit std::array<std::uint8_t, 6> vm_shuffle_6{};
static constinit std::array<std::uint8_t, 7> vm_shuffle_7{};
static constinit std::array<std::uint8_t, 8> vm_shuffle_8{};

template<int array_count>
static constexpr std::array<std::uint8_t, array_count> vm_shuffle_ord_str(const std::array<std::string_view, array_count>& expected_order, const std::array<std::string_view, array_count>
	& gotten_order) {
	static constinit std::array<std::uint8_t, array_count> vms{};

	for (int iterator = 0; iterator < gotten_order.size(); iterator++) {
		const auto& a = gotten_order[iterator];

		for (auto i = 0u; i < expected_order.size(); ++i)
			if (a == expected_order[i])
				vms[iterator] = i;
	}

	return vms;
}

template<int array_count>
static constexpr std::array<std::uint8_t, array_count> vm_shuffle_ord_enum(const std::array<std::uint8_t, array_count>& expected_order, const std::array<std::uint8_t, array_count>
	& gotten_order) {
	static constinit std::array<std::uint8_t, array_count> vms{};

	for (int iterator = 0; iterator < gotten_order.size(); iterator++) {
		const auto& a = gotten_order[iterator];

		for (auto i = 0u; i < expected_order.size(); ++i)
			if (a == expected_order[i])
				vms[iterator] = i;
	}

	return vms;
}


FORCEINLINE std::vector<std::uint16_t*> make_vec_copy(const std::vector<std::uint16_t*>& offsets) {
	std::vector<std::uint16_t*> copy{};

	for (auto&& offset : offsets) {
		auto* epic = new std::uint16_t;

		memcpy(epic, offset, 2);

		copy.emplace_back(epic);
	}

	return copy;
}

std::uintptr_t lua_typename_addr = 0;
__declspec(naked) const char* __cdecl lua_typename(int a1, int a2)
{
	__asm {
		push ebp
		mov ebp, esp
		push[ebp-0xc]
		push[ebp-0x8]
		call lua_typename_addr
		add esp, 8
		pop ebp
		ret
	}
}

void __fastcall roblox::shuffles::init() {
	lua_typename_addr = memory::pattern_scan(xor("B8 ? ? ? ? 83 FA FF 74 07"));

	for (int i = 0; i < 10; i++)
		types[i] = lua_typename(0u, i);

	for (int i = 0; i < 19; i++)
		tms[i] = lua_typename(0u, i + 10);

	vm_shuffle_3 = vm_shuffle_ord_str<3>({
		std::string_view(xor("userdata")),
		std::string_view(xor("number")),
		std::string_view(xor("vector"))
		}, {
			types[2],
			types[3],
			types[4]
		});

	std::cout << xor("#define VM_SHUFFLE3(sep, a0, a1, a2) ");

	for (unsigned i = 0 ; i < vm_shuffle_3.size(); i++) {
		std::cout << "a" << static_cast<int>(vm_shuffle_3[i]) << (i == vm_shuffle_3.size()-1 ? xor(" ") : xor(" sep "));
	}

	std::cout << '\n';

	vm_shuffle_4 = vm_shuffle_ord_str<4>({
		std::string_view(xor("table")),
		std::string_view(xor("function")),
		std::string_view(xor("userdata")),
		std::string_view(xor("thread"))
		}, {
			types[6],
			types[7],
			types[8],
			types[9]
		});

	std::cout << xor("#define VM_SHUFFLE4(sep, a0, a1, a2, a3) ");

	for (unsigned i = 0 ; i < vm_shuffle_4.size(); i++) {
		std::cout << "a" << static_cast<int>(vm_shuffle_4[i]) << (i == vm_shuffle_4.size()-1 ? xor(" ") : xor(" sep "));
	}

	std::cout << '\n';

	vm_shuffle_7 = vm_shuffle_ord_str<7>({
		std::string_view(xor("__add")),
		std::string_view(xor("__sub")),
		std::string_view(xor("__mul")),
		std::string_view(xor("__div")),
		std::string_view(xor("__mod")),
		std::string_view(xor("__pow")),
		std::string_view(xor("__unm"))
		}, {
			tms[8],
			tms[9],
			tms[10],
			tms[11],
			tms[12],
			tms[13],
			tms[14],
		});

	std::cout << xor("#define VM_SHUFFLE7(sep, a0, a1, a2, a3, a4, a5, a6) ");

	for (unsigned i = 0 ; i < vm_shuffle_7.size(); i++) {
		std::cout << "a" << static_cast<int>(vm_shuffle_7[i]) << (i == vm_shuffle_7.size()-1 ? xor(" ") : xor(" sep "));
	}

	std::cout << '\n';
}

void __fastcall roblox::shuffles::init_vm_shuffle5(std::uintptr_t global) {
		/* Needed for shuffle5. */

	    // GCThreshold: 1
	    // totalbytes: 2
	    // goal: 3
	    // gcstepmul: 4
	    // gcstepsize: 5

	    enum vm_shuffle_5_t {
			None,
		    GCthreshold = 1,
			totalbytes = 2,
			gcgoal = 3,
			gcstepmul = 4,
			gcstepsize = 5
	    };

		static constexpr std::array<std::uint8_t, 5> expected_order{ GCthreshold, totalbytes, gcgoal, gcstepmul, gcstepsize };

		static constinit std::array<std::uint8_t, 5> gotten_order;

		vm_shuffle_5_t temp_enum = None;

		for (unsigned a = 0; a < 5u; ++a) {

			/* 52 is after strbufgc segment. */
			/* strbufgc removed as of version-b117c4c276664452 */
			constexpr int offset = offsetof(roblox::shuffles::original_structs::global_State, GCthreshold);
			const auto value = *reinterpret_cast<std::int32_t*>(global + (offset + (a * 4u)));

			std::cout << std::to_string((40u + (a * 4u))) << ": " << std::to_string(value) << '\n';


			switch (value) {
			case 1024:
			case 2048:
			case 4096:
			case 8192: {
				gotten_order[a] = gcstepsize;
				break;
			}

			case 250: {
				gotten_order[a] = gcstepmul;
				break;
			}

			case 125:
			case 150: {
				gotten_order[a] = gcgoal;
				break;
			}

			default: {
				if (!temp_enum) {

					for (auto b = 0; b < 5u; ++b) {
						const auto v = *reinterpret_cast<std::int32_t*>(global + (40u + (b * 4u)));
						if (value < v && v > 1100 && v != value) {
							temp_enum = GCthreshold;
							gotten_order[a] = totalbytes;
							break;
						}
						if (v > 1100 && v != value) {
							temp_enum = totalbytes;
							gotten_order[a] = GCthreshold;
							break;
						}
					}

				}
				else
					gotten_order[a] = temp_enum;

				break;
			}
			}
		}

	vm_shuffle_5 = vm_shuffle_ord_enum<5>(expected_order, gotten_order);

	std::cout << xor("#define VM_SHUFFLE5(sep, a0, a1, a2, a3, a4) ");

	for (unsigned i = 0; i < vm_shuffle_5.size(); i++) {
		std::cout << "a" << static_cast<int>(vm_shuffle_5[i]) << (i == vm_shuffle_5.size()-1 ? " " : " sep ");
	}
	std::cout << '\n';
}

template<typename vec_t>
FORCEINLINE auto remove_value(std::vector<vec_t>* vec, vec_t value) {
	for (int i = 0; i < vec->size(); i++) {
		if (vec->operator[](i) == value) {
			vec->erase(vec->begin() + i);
		}
	}
}


void __fastcall roblox::shuffles::init_vm_shuffle6(std::uintptr_t ls, std::uint8_t capacity) {
	enum vm_shuffle_6_t {
		None,
		top,
		base,
		global,
		ci,
		stack_last,
		stack
	};

	static constexpr std::array<std::uint8_t, 6> expected_order = {
		top, base, global, ci, stack_last, stack };

	std::uint8_t stack_offset = 0, stack_last_offset = 0, top_offset = 0, base_offset = 0, global_offset = 0, ci_offset = 0;

	for (std::uint8_t a = 0; a < 6u; ++a) { /* stack_offset & stack_last_offset */
		const auto address = *reinterpret_cast<std::uintptr_t*>(ls + (8u + a * 4u));

		if (!stack_offset || !stack_last_offset) {
			for (std::uint8_t a2 = 0; a2 < 6u; ++a2) {
				constexpr size_t stack_size = 640;
				const auto addr2 = *reinterpret_cast<std::uintptr_t*>(ls + (8u + a2 * 4u));
				if (address + stack_size == addr2) {
					stack_offset = a;
					stack_last_offset = a2;
					std::cout << xor("found two\n");
					break;
				}

				if (address == addr2 + stack_size) {
					stack_offset = a2;
					stack_last_offset = a;
					std::cout << xor("found two\n");
					break;
				}
			}
		}
	}


	if (!top_offset && !base_offset) {
		for (std::uint8_t a = 0; a < 6u; ++a) {
			const auto address = *reinterpret_cast<std::uintptr_t*>(ls + (8u + a * 4u));

			for (std::uint8_t a2 = 0; a2 < 6u; ++a2) {
				const auto addr2 = *reinterpret_cast<std::uintptr_t*>(ls + (8u + a2 * 4u));

				if (address == addr2 && a != a2) {
					//smh::println(std::to_string(a), " and ", std::to_string(a2), " are the same");
					// now to figure out which one is top and which is base!

					// dont worry about getting this wrong, shouldn't matter in the short term.
					auto bytecode = Compiler::compile(xor("return \"hi\""), nullptr);
					lua::api::luau_load((lua_state*)ls, bytecode, xor("hi"));

					const auto addr3 = *reinterpret_cast<std::uintptr_t*>(ls + (8u + a2 * 4u));
					const auto address2 = *reinterpret_cast<std::uintptr_t*>(ls + (8u + a * 4u));

					//smh::println("addr3: 0x", std::hex, addr3, std::dec, '\n');
					//smh::println("addr2: 0x", std::hex, address2, std::dec, '\n');

					if (address2 - 16 == addr3) {
						top_offset = a;
						base_offset = a2;
						*reinterpret_cast<std::uintptr_t*>(ls + (8u + a * 4u)) -= 16;
						//smh::println(xor("gotem"));
						break;
					}

					if (address2 == addr3 - 16) {
						top_offset = a2;
						base_offset = a;
						*reinterpret_cast<std::uintptr_t*>(ls + (8u + a2 * 4u)) -= 16;
						//smh::println(xor("gotem"));
						break;
					}
				}
			}
		}
	}


	for (std::uint8_t a = 0; a < 6u; ++a) { /* global */
		const auto address = ls + (8u + a * 4u);

		const auto& [obf_op, obf_dir, deobf_op, deobf_dir] = obfuscations::get_ptr_obfuscation(address, capacity, 4, { 256, 512, 1024, 2048, 4096, 8192, 16384, 32768 });

		if (deobf_dir || deobf_op || obf_dir || obf_op) {
			//std::cout << "d\n";
			global_offset = a;
		}
	}

	

	std::vector nums = { 0, 1, 2, 3, 4, 5 };


	remove_value<int>(&nums, top_offset);
	remove_value<int>(&nums, base_offset);
	remove_value<int>(&nums, stack_offset);
	remove_value<int>(&nums, stack_last_offset);
	remove_value<int>(&nums, global_offset);
	ci_offset = nums.front();

	std::map<std::uint8_t, std::uint8_t> map{
		{ top_offset, top }, // top
		{ base_offset, base }, // base
		{ global_offset, global }, // global
		{ ci_offset, ci }, // ci
		{ stack_last_offset, stack_last}, // stack_last
		{ stack_offset, stack }, // stack
	};

	static const std::array gotten_order{
		map[0],
		map[1],
		map[2],
		map[3],
		map[4],
		map[5],
	};

	vm_shuffle_6 = vm_shuffle_ord_enum<6>(expected_order, gotten_order);

	std::cout << xor("#define VM_SHUFFLE6(sep, a0, a1, a2, a3, a4, a5) ");

	for (unsigned i = 0; i < vm_shuffle_6.size(); i++) {
		std::cout << "a" << static_cast<int>(vm_shuffle_6[i]) << (i == vm_shuffle_6.size() - 1 ? "\n" : " sep ");
	}
}

#pragma optimize( "", off)
void __fastcall roblox::shuffles::init_vm_shuffle8(std::uintptr_t proto) {
	enum vm_shuffle_8_t {
		None,
		sizecode,
		sizep,
		sizelocvals,
		sizeupvalues,
		sizek,
		sizelineinfo,
		linegaplog2,
		linedefined
	};

	static constexpr std::array <std::uint8_t, 8> expected_order = { sizecode, sizep, sizelocvals, sizeupvalues, sizek, sizelineinfo, linegaplog2, linedefined };

	static constinit std::array <std::uint8_t, 8> gotten_order{};

		for (auto a = 0; a < 8u; ++a) {

			switch (const auto value = *reinterpret_cast<std::int32_t*>(proto + (48u + (a * 4u)))) {
			case 25: // sizecode
				gotten_order[a] = sizecode;
				break;
			case 4: // sizep
				gotten_order[a] = sizep;
				break;
			case 3: // sizelocvals
				gotten_order[a] = sizelocvals;
				break;
			case 0: // sizeupvalues
				gotten_order[a] = sizeupvalues;
				break;
			case 5: // sizek
				gotten_order[a] = sizek;
				break;
			case 32: // sizelineinfo
				gotten_order[a] = sizelineinfo;
				break;
			case 24: // linegaplog2
				gotten_order[a] = linegaplog2;
				break;
			case 2: // linedefined
				gotten_order[a] = linedefined;
				break;
			}
		}

	vm_shuffle_8 = vm_shuffle_ord_enum<8>(expected_order, gotten_order);

	std::cout << xor("#define VM_SHUFFLE8(sep, a0, a1, a2, a3, a4, a5, a6, a7) ");

	for (unsigned i = 0 ; i < vm_shuffle_8.size(); i++) {
		std::cout << "a" << static_cast<int>(vm_shuffle_8[i]) << (i == vm_shuffle_8.size()-1 ? xor(" ") : xor(" sep "));
	}

	std::cout << '\n';
}

#pragma optimize( "", on)
void __fastcall roblox::shuffles::deobfuscate(std::vector<std::uint16_t*> offsets)
{
	const auto copy = make_vec_copy(offsets);

	switch (offsets.size()) {
	case 2:
		for (std::uint8_t i = 0; i < copy.size(); i++) {
			*offsets[vm_shuffle_2.at(i)] = *copy[i];
		}
		return;
	case 3:
		for (std::uint8_t i = 0; i < copy.size(); i++) {
			*offsets[vm_shuffle_3.at(i)] = *copy[i];
		}
		return;
	case 4:
		for (std::uint8_t i = 0; i < copy.size(); i++) {
			*offsets[vm_shuffle_4.at(i)] = *copy[i];
		}
		return;
	case 5:
		for (std::uint8_t i = 0; i < copy.size(); i++) {
			*offsets[vm_shuffle_5.at(i)] = *copy[i];
		}
		return;
	case 6:
		for (std::uint8_t i = 0; i < copy.size(); i++) {
			*offsets[vm_shuffle_6.at(i)] = *copy[i];
		}
		return;
	case 7:
		for (std::uint8_t i = 0; i < copy.size(); i++) {
			*offsets[vm_shuffle_7.at(i)] = *copy[i];
		}
		return;
	case 8:
		for (std::uint8_t i = 0; i < copy.size(); i++) {
			*offsets[vm_shuffle_8.at(i)] = *copy[i];
		}
		return;
	}
}
