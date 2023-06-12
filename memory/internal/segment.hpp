#pragma once

struct segment_info {
    std::string name;
    std::uintptr_t start_addr;
    std::uintptr_t end_addr;
};

namespace memory::internal {
	std::vector<segment_info> get_segments();
}