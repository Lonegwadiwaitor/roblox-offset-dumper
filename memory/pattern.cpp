#include "../pch/pch.h"

#include "internal/segment.hpp"

#include "pattern.hpp"

std::uintptr_t __fastcall memory::scan(void* module, const std::vector<int>& signature) {
    const auto dos_header = static_cast<PIMAGE_DOS_HEADER>(module);
    const auto nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(static_cast<std::uint8_t*>(module) + dos_header->e_lfanew);

    const auto size_of_image = nt_headers->OptionalHeader.SizeOfImage;
    const auto pattern_bytes = signature;
    auto scan_bytes = static_cast<std::uint8_t*>(module);

    const auto s = pattern_bytes.size();
    const auto d = pattern_bytes.data();

    for (auto i = 0ul; i < size_of_image - s; ++i) {
        bool found = true;
        for (auto j = 0ul; j < s; ++j) {
            if (scan_bytes[i + j] != d[j] && d[j] != -1) {
                found = false;
                break;
            }
        }
        if (found) {
            return reinterpret_cast<std::uintptr_t>(&scan_bytes[i]);
        }
    }
    return 0;
}

__forceinline auto reconstruct_char(std::vector<int> vec) {
	const std::string str(vec.begin(), vec.end());

    const char* c_string = str.data();

    return c_string;
} 

std::uintptr_t __fastcall memory::scan_range(void* module, const std::vector<int>& signature, const std::uintptr_t& start,
                                             const std::uintptr_t& end, bool exact) {
    const auto size_of_image = end;
    const auto& pattern_bytes = signature;
    auto scan_bytes = static_cast<std::uint8_t*>(module);

    const auto s = pattern_bytes.size();
    const auto d = pattern_bytes.data();

    for (auto i = start; i < size_of_image - s; ++i) {
        bool found = true;
        for (auto j = 0ul; j < s; ++j) {
            if (scan_bytes[i + j] != d[j] && d[j] != -1) {
                found = false;
                break;
            }
        }
        if (found) {
            if (exact) {
                while (reinterpret_cast<char*>(scan_bytes[i-1]) != NULL) {
                    i--;
                }
                auto str1 = reinterpret_cast<const char*>(&scan_bytes[i]);
                std::string str2(signature.begin(), signature.end());
                if (strcmp(str1, str2.c_str()) == 0)
                    return reinterpret_cast<std::uintptr_t>(&scan_bytes[i]);
                i += str2.size();
            } else return reinterpret_cast<std::uintptr_t>(&scan_bytes[i]);
        }
    }
    return 0;
}

constexpr char c_ref1[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
constexpr int c_ref2[16] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15 };

FORCEINLINE UCHAR to_byte(char* x) {
    if (lstrlenA(x) < 2) return 0;
    if (x[0] == '?' && x[1] == '?') return 0;
    UCHAR b = 0;
    for (int i = 0; i < 16; i++) {
        if (x[0] == c_ref1[i]) b += c_ref2[i] * 16;
        if (x[1] == c_ref1[i]) b += i;
    }
    return b;
}

FORCEINLINE std::string to_str(auto b) {
    std::string x = "";
    x += c_ref1[b / 16];
    x += c_ref1[b % 16];
}

struct cbyte {
    std::vector<UCHAR>bytes;

    cbyte(std::string saob) {
        bytes = std::vector<UCHAR>();
        std::string newstr;
        for (char c : saob) if (c != 0x20) newstr += c;
        if (newstr.length() / 2 > 0 && newstr.length() % 2 == 0) {
            for (int i = 0; i < newstr.length(); i += 2) {
                char s[3];
                s[0] = newstr[i];
                s[1] = newstr[i + 1];
                s[2] = '\0';
                add(to_byte(s));
            }
        }
    }

    void add(UCHAR b) {
        bytes.push_back(b);
    }

    size_t size() const {
        return bytes.size();
    }

    std::string to_string() const {
        std::string str;
        for (int i = 0; i < bytes.size(); i++) {
            str += to_str(bytes[i]);
            if (i != (size() - 1))
                str += ", ";
            else
                str += ".";
        }
        return str;
    }
};

std::uintptr_t __fastcall memory::scan_range_int(void* module, int signature, std::uintptr_t start,
    std::uintptr_t end) {
    const auto size_of_image = end;
    const auto pattern_bytes = signature;
    auto scan_bytes = static_cast<std::uint8_t*>(module);

    const auto s = 1;

    for (auto i = start; i < size_of_image - s; ++i) {
        bool found = true;
        for (auto j = 0ul; j < s; ++j) {
            if (reinterpret_cast<int*>(scan_bytes[i + j]) != reinterpret_cast<int*>(signature)) {
                found = false;
                break;
            }
        }
        if (found) {
            return reinterpret_cast<std::uintptr_t>(&scan_bytes[i]);
        }
    }
    return 0;
}

std::uintptr_t __fastcall memory::pattern_scan(const char* signature) {
    static auto pattern_to_byte = [](const char* pattern) {
        auto bytes = std::vector<int>{};
        const auto start = const_cast<char*>(pattern);
        const auto end = const_cast<char*>(pattern) + strlen(pattern);

        for (auto current = start; current < end; ++current) {
            if (*current == '?') {
                ++current;
                if (*current == '?')
                    ++current;
                bytes.push_back(-1);
            }
            else {
                bytes.push_back(strtoul(current, &current, 16));
            }
        }
        return bytes;
    };

    return scan(GetModuleHandle(nullptr), pattern_to_byte(signature));
}

std::vector<std::uintptr_t> __fastcall memory::safe_ptr_scan_range(const std::uintptr_t& ptr, const std::uintptr_t& start,
                                                                   const std::uintptr_t& end) {
    std::vector<std::uintptr_t> results{};

    union epic {
        int yea;
        unsigned char no[4];
    };

    static HANDLE process_handle = GetCurrentProcess();

    const auto& _ptr = static_cast<std::array<BYTE, 2048>*>(malloc(2048));

    for (int i = start; i < end-4; i+= 2048) {
        if (!ReadProcessMemory(process_handle, reinterpret_cast<LPCVOID>(i), _ptr->data(), 2048, nullptr))
            continue;

        epic conversion(ptr);

        for (int x = 0; x < _ptr->size()-4; x++) {
            if (_ptr->at(x) == conversion.no[0] && _ptr->at(x + 1) == conversion.no[1] && _ptr->at(x + 2) == conversion.no[2] && _ptr->at(x + 3) == conversion.no[3])
                results.emplace_back(i+x);
        }
    }

    delete[] _ptr;

    return results;
}

std::uintptr_t __fastcall memory::str_xref_scan(const char* str, const std::uintptr_t& rdata_start, const std::uintptr_t& rdata_end, bool exact) {
    std::string string = str;
    const std::vector<int> bytes(string.begin(), string.end());

    return scan_range(GetModuleHandle(nullptr), bytes, rdata_start, rdata_end, exact);
}
