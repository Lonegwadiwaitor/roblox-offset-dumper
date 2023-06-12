#pragma once

#include "util.hpp"

#include "deobfuscate.hpp"
#include "obfuscate.hpp"

namespace roblox::obfuscations {
	std::pair<obf_obfuscation, obf_direction> __fastcall get_opposite(obf_obfuscation obf, obf_direction dir);
	obfuscation_t get_ptr_obfuscation(std::uintptr_t obfuscated_object_addr, std::uint16_t disp, std::uint8_t size, std::vector<int> expected_values);

	template<typename T>
	DECLSPEC_NOINLINE obfuscation_t get_obfuscation(std::uintptr_t obfuscated_value_addr, std::uint16_t disp, std::vector<T> expected_values) {
        obfuscation_t result{};

        using namespace obfuscations;

        auto&& obf = SUB;
    auto&& dir = RIGHT;

    int deobf_ptr2 = deobfuscate<T>(obfuscated_value_addr, disp, obf, dir);

    for (auto expected_value : expected_values)
            if (deobf_ptr2 == expected_value) {
	            // got it. wew

                result.deobf_op = obf;
                result.deobf_dir = dir;

                auto [obf2, dir2] = get_opposite(obf, dir);

                result.obf_op = obf2;
                result.obf_dir = dir2;

                return result;
            }

    dir = LEFT;

        deobf_ptr2 = deobfuscate<T>(obfuscated_value_addr, disp, obf, dir);

        for (auto expected_value : expected_values)
            if (deobf_ptr2 == expected_value) {
	            // got it. wew

                result.deobf_op = obf;
                result.deobf_dir = dir;

                auto [obf2, dir2] = get_opposite(obf, dir);

                result.obf_op = obf2;
                result.obf_dir = dir2;

                return result;
            }

        for (int i1 = 3-1; i1 >= 0; --i1) {
            auto&& obfuscation = static_cast<obf_obfuscation>(i1);

            for (int i2 = 2-1; i2 >= 0; --i2) {
                auto&& direction = static_cast<obf_direction>(i2);
                T rbx_val = deobfuscate<T>(obfuscated_value_addr, disp, obfuscation, direction);

                for (auto expected_value : expected_values) {
                    if (expected_value == rbx_val) {
	                    // gotem

                        result.deobf_op = (obf_obfuscation)i1;
                        result.deobf_dir = (obf_direction)i2;

                        auto [obf2, dir2] = get_opposite(result.deobf_op, result.deobf_dir);

                        result.obf_op = obf2;
                        result.obf_dir = dir2;

                        return result;

                        for (int i3 = 0; i3 < 3; i3++) {
                            auto&& obfuscation2 = static_cast<obf_obfuscation>(i3);

                            for (int i4 = 0; i4 < 2; i4++) {
                                auto&& direction2 = static_cast<obf_direction>(i4);

                                auto obfuscated = obfuscate<uintptr_t>(obfuscated_value_addr, disp, rbx_val, obfuscation2, direction2);
                                auto rbx_obfuscated = *reinterpret_cast<uintptr_t*>(obfuscated_value_addr + disp);

                                if (obfuscated == rbx_obfuscated) {
                                    result.obf_op = obfuscation2;
                                    result.obf_dir = direction2;

                                    return result;
                                }
                            }
                        }
                    }
                }
            }
        }

        smh::println(xor("The bruteforce below could not be completed successfully."));

        return result;
	}


    using callback_t = bool(*)(obf_obfuscation, obf_direction);


    template<typename callback_t>
    obfuscation_t __fastcall iterate_obfuscations(callback_t callback) {
		for (int i1 = 3-1; i1 >= 0; --i1) {
            auto&& obfuscation = static_cast<obf_obfuscation>(i1);

            for (int i2 = 2-1; i2 >= 0; --i2) {
                auto&& direction = static_cast<obf_direction>(i2);

                if (callback(obfuscation, direction))
                    return [obfuscation, direction] {
                        auto [obf_op, obf_dir] = get_opposite(obfuscation, direction);

                        return obfuscation_t{ obf_op, obf_dir, obfuscation, direction };
                    }();
            }
        }

        smh::println(xor("The bruteforce below could not be completed successfully."));

		return {};
	}


	obfuscation_t __fastcall deobfuscate_ptr(std::uintptr_t addr, std::uint16_t disp);

}