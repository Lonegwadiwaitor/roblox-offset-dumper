#include "../pch/pch.h"

#include "scanner.hpp"

#include "internal/segment.hpp"

#include "pattern.hpp"

#include "../memory/rebase.hpp"

#include <boost/iterator/filter_iterator.hpp>

union byte_conversion {
	int num;
	char bytes[4];
};

std::uintptr_t memory::get_str_xref(const char* string) {
	const auto& segments = internal::get_segments();

	auto&& string_segment = boost::make_filter_iterator([](const segment_info& info) {
		return info.name == ".rdata";
		}, segments.begin(), segments.end());

	return str_xref_scan(string, string_segment->start_addr, string_segment->end_addr, true);;
}

std::vector<std::uintptr_t> __fastcall memory::scan_string(const char* string, bool exact) {
	const auto& segments = internal::get_segments();

	auto&& string_segment = boost::make_filter_iterator([](const segment_info& info) {
		return info.name == ".rdata";
	}, segments.begin(), segments.end());

	auto&& text_segment = boost::make_filter_iterator([](const segment_info& info) {
		return info.name == ".text";
	}, segments.begin(), segments.end());

	auto epic = str_xref_scan(string, string_segment->start_addr, string_segment->end_addr, exact);

	auto start2 = clock();

	//smh::println(std::hex, std::format("Data segment start: 0x{} && end: 0x{}", string_segment->start_addr, string_segment->end_addr), std::dec);
	//smh::println(std::hex, std::format("Text segment start: 0x{} && end: 0x{}", text_segment->start_addr, text_segment->end_addr), std::dec);

	auto addr = safe_ptr_scan_range(epic, text_segment->start_addr, text_segment->end_addr);

	auto end2 = clock();

	//smh::print("Found ", addr.size(), " item(s). (took ", end2-start2, " ms!)", "\n");

	return addr;
}
