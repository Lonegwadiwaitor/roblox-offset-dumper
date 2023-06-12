#include "../../pch/pch.h"
#include "bruteforce.hpp"

#include "obfuscate.hpp"
#include "util.hpp"
#include "../../util/capstone_engine.hpp"


std::pair<roblox::obfuscations::obf_obfuscation, roblox::obfuscations::obf_direction> __fastcall roblox::obfuscations::
get_opposite(obf_obfuscation obfuscation, obf_direction direction) {

    using namespace obfuscations;

    std::pair<obf_obfuscation, obf_direction> opposite = {};

    switch (obfuscation)
    {
    case ADD:
        opposite.first = SUB;
        break;

    case SUB:
        opposite.first = ADD;
        break;

    case XOR:
        opposite.first = XOR;
        break;
    }

    switch (direction)
    {
    case LEFT:
        opposite.second = RIGHT;
        break;

    case RIGHT:
        opposite.second = LEFT;
        break;
    }

    if (obfuscation == SUB && direction == RIGHT)
        return opposite = { SUB, RIGHT };

    if (obfuscation == XOR)
        opposite.second = direction; /* if XOR LEFT then stay XOR LEFT, if XOR RIGHT then stay XOR RIGHT */

    return opposite;
}


DECLSPEC_NOINLINE roblox::obfuscations::obfuscation_t roblox::obfuscations::get_ptr_obfuscation(std::uintptr_t obfuscated_object,
                                                                          std::uint16_t disp, std::uint8_t size, std::vector<int> expected_values) {

    obfuscation_t result{};

    using namespace obfuscations;

    auto&& obf = SUB;
    auto&& dir = RIGHT;

    int deobf_ptr2 = deobfuscate<int>(obfuscated_object, 0, obf, dir);

    int value = 0;

	if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(static_cast<uintptr_t>(deobf_ptr2) + disp), &value, size, nullptr))

    for (auto expected_value : expected_values)
            if (value == expected_value) {
	            // got it. wew

                result.deobf_op = obf;
                result.deobf_dir = dir;

                auto [obf2, dir2] = get_opposite(obf, dir);

                result.obf_op = obf2;
                result.obf_dir = dir2;

                return result;
            }

    dir = LEFT;

    if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(static_cast<uintptr_t>(deobf_ptr2) + disp), &value, size, nullptr))

    for (auto expected_value : expected_values)
            if (value == expected_value) {
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

            /*__try {
            }
            __except ([](int, PEXCEPTION_POINTERS) {
                return EXCEPTION_EXECUTE_HANDLER;
                }(GetExceptionCode(), GetExceptionInformation())) {}*/

            int deobf_ptr = deobfuscate<int>(obfuscated_object, 0, obfuscation, direction);


                if (!deobf_ptr)
                    continue;

            int value = 0;

            if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(static_cast<uintptr_t>(deobf_ptr) + disp), &value, size, nullptr))
                continue;

            
            for (auto expected_value : expected_values)
            if (value == expected_value) {
	            // got it. wew

                result.deobf_op = obfuscation;
                result.deobf_dir = direction;

                auto [obf2, dir2] = get_opposite(obfuscation, direction);

                result.obf_op = obf2;
                result.obf_dir = dir2;

                return result;

                for (int i3 = 0; i3 < 3; i3++) {
                    auto&& obfuscation2 = static_cast<obf_obfuscation>(i3);

                    for (int i4 = 0; i4 < 2; i4++) {
                        auto&& direction2 = static_cast<obf_direction>(i4);

                        auto obfuscated = obfuscate<uintptr_t>(obfuscated_object, 0, deobf_ptr, obfuscation2, direction2);
                        auto rbx_obfuscated = *reinterpret_cast<uintptr_t*>(obfuscated_object);

                        if (obfuscated == rbx_obfuscated) {
                            result.obf_op = obfuscation2;
                            result.obf_dir = direction2;

                            
                        }
                    }
                }
            }
        }
    }

    smh::println(xor("The bruteforce below could not be completed successfully."));

    return result;
}


roblox::obfuscations::obfuscation_t __fastcall roblox::obfuscations::deobfuscate_ptr(std::uintptr_t addr,
	std::uint16_t disp) {
	obfuscation_t result{};

    using namespace obfuscations;

    int deobf_ptr = deobfuscate<int>(addr, disp, SUB, RIGHT);

    if (capstone_engine::is_function(deobf_ptr)) {
	    result.deobf_op = SUB;
                result.deobf_dir = RIGHT;

                auto [obf2, dir2] = get_opposite(SUB, RIGHT);

                result.obf_op = obf2;
                result.obf_dir = dir2;

                return result;
    }

    deobf_ptr = deobfuscate<int>(addr, disp, SUB, LEFT);

    if (capstone_engine::is_function(deobf_ptr)) {
	    result.deobf_op = SUB;
                result.deobf_dir = LEFT;

                auto [obf2, dir2] = get_opposite(SUB, LEFT);

                result.obf_op = obf2;
                result.obf_dir = dir2;

                return result;
    }

    for (int i1 = 3-1; i1 >= 0; --i1) {
        auto&& obfuscation = static_cast<obf_obfuscation>(i1);

        for (int i2 = 2-1; i2 >= 0; --i2) {
            auto&& direction = static_cast<obf_direction>(i2);


            int deobf_ptr = deobfuscate<int>(addr, disp, obfuscation, direction);

            if (capstone_engine::is_function(deobf_ptr)) {
                result.deobf_op = obfuscation;
                result.deobf_dir = direction;

                auto [obf2, dir2] = get_opposite(obfuscation, direction);

                result.obf_op = obf2;
                result.obf_dir = dir2;

                return result;

                for (int i3 = 0; i3 < 3; i3++) {
                    auto&& obfuscation2 = static_cast<obf_obfuscation>(i3);

                    for (int i4 = 0; i4 < 2; i4++) {
                        auto&& direction2 = static_cast<obf_direction>(i4);

                        auto obfuscated = obfuscate<uintptr_t>(addr, 0, deobf_ptr, obfuscation2, direction2);
                        auto rbx_obfuscated = *reinterpret_cast<uintptr_t*>(addr);

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

    smh::println(xor("The bruteforce below could not be completed successfully."));
}

