#include "../../pch/pch.h"

#include "segment.hpp"

#include "process.hpp"

std::vector<segment_info> memory::internal::get_segments() {
    static std::vector<segment_info> segments;

    if (!segments.empty())
        return segments;

    DWORD bytesRead = NULL;
    // open file
    const HANDLE file = CreateFileA(get_roblox_path().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) printf("Could not read file");

    // allocate heap
    DWORD fileSize = GetFileSize(file, NULL);
    LPVOID fileData = HeapAlloc(GetProcessHeap(), 0, fileSize);


    // read file bytes to memory
    ReadFile(file, fileData, fileSize, &bytesRead, NULL);

    const auto dos_header = static_cast<PIMAGE_DOS_HEADER>(fileData);
    auto image_nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<DWORD>(fileData) + dos_header->e_lfanew);
    auto section_location = reinterpret_cast<DWORD>(image_nt_headers) + sizeof(DWORD) + static_cast<DWORD>(sizeof(IMAGE_FILE_HEADER)) + static_cast<DWORD>(image_nt_headers->FileHeader.SizeOfOptionalHeader);

    for (int i = 0; i < image_nt_headers->FileHeader.NumberOfSections; i++) {
	    constexpr auto section_size = sizeof(IMAGE_SECTION_HEADER);
	    const auto section_header = reinterpret_cast<PIMAGE_SECTION_HEADER>(section_location);
        segments.emplace_back(segment_info{
                reinterpret_cast<const char* const>(section_header->Name),
                section_header->VirtualAddress,
                section_header->VirtualAddress + section_header->Misc.VirtualSize
            });

        section_location += section_size;
    }

    return segments;
}
