#include "../../../pch/pch.h"

#include "objects.hpp"
#include "../../shuffles/structs.hpp"

roblox::shuffles::new_structs::gch* _gch;
roblox::shuffles::new_structs::ls* _ls;
roblox::shuffles::new_structs::ci* _ci;
roblox::shuffles::new_structs::tbl* _tbl;

void gc_header::init(roblox::shuffles::new_structs::gch* gcheader) {
	_gch = static_cast<roblox::shuffles::new_structs::gch*>(malloc(sizeof(roblox::shuffles::new_structs::gch)));
	memcpy(_gch, gcheader, sizeof(roblox::shuffles::new_structs::gch));
}

uint8_t& gc_header::type() {
	return *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(this) + _gch->type);
}

uint8_t& gc_header::marked() {
	return *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(this) + _gch->marked);
}

uint8_t& gc_header::memcat() {
	return *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(this) + _gch->modkey);
}

void table::init(roblox::shuffles::new_structs::tbl* tbl) {
	_tbl = static_cast<roblox::shuffles::new_structs::tbl*>(malloc(sizeof(roblox::shuffles::new_structs::tbl)));
	memcpy(_tbl, tbl, sizeof(roblox::shuffles::new_structs::tbl));
}

struct str_pair {
	const char* item1;
	const char* item2;
};


FORCEINLINE auto get_tbl_type(std::string_view string) {
	static std::array tbl_type_declarations{
	    str_pair{ xor("std::uint8_t"), xor("flags") },
	    str_pair{ xor("std::uint8_t"), xor("readonly") },
	    str_pair{ xor("std::uint8_t"), xor("safeenv") },
	    str_pair{ xor("std::uint8_t"), xor("lsizenode") },
	    str_pair{ xor("std::uint8_t"), xor("nodemask8") },
	    str_pair{ xor("struct Table*"), xor("metatable") },
	    str_pair{ xor("TValue*"), xor("array") },
	    str_pair{ xor("LuaNode*"), xor("node") },
	    str_pair{ xor("GCObject*"), xor("gclist") },
	    str_pair{ xor("int"), xor("sizearray") },
	    str_pair{ xor("int"), xor("lastfree") },
	    str_pair{ xor("int"), xor("aboundary") },
	};
	for (const auto & [type, name] : tbl_type_declarations) {
		if (strcmp(string.data(), name) == 0)
			return type;
	}
}

// this was meant to sort a shuffled struct, i think it works however it was never optimized or finished.
void table::dump_function(const std::string_view& path, roblox::shuffles::new_structs::tbl* tbl) {
	std::vector<std::pair<std::string_view, std::uint16_t>> offsets{
		{ xor("flags"), tbl->flags },
		{ xor("readonly"), tbl->readonly },
		{ xor("safeenv"), tbl->safeenv },
		{ xor("lsizenode"), tbl->lsizenode },
		{ xor("nodemask8"), tbl->nodemask8 },
		{ xor("sizearray"), tbl->sizearray },
		{ xor("lastfree"), tbl->lastfree },
		{ xor("metatable"), tbl->metatable },
		{ xor("array"), tbl->metatable },
		{ xor("node"), tbl->node },
		{ xor("gclist"), tbl->gclist }
	};

	std::ranges::sort(offsets, [](std::pair<std::string_view, std::uint16_t> a, std::pair<std::string_view, std::uint16_t> b) { return a.second < b.second; });

	std::cout << xor("struct Table {\n     CommonHeader\n");

	for (const auto & offset : offsets) {
		std::cout << "     ";

		if (offset.first == xor("lastfree")) {
			std::cout << xor("union\n     {\n     ");
		}

		std::cout << get_tbl_type(offset.first) << " " << offset.first << ";\n";

		if (offset.first == xor("lastfree")) {
			std::cout << xor("     int aboundary;\n     };\n");
		}
	}

	std::cout << xor("}\n");
}

uint8_t& table::flags() {
	return *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(this) + _tbl->flags);
}

uint8_t& table::readonly() {
	return *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(this) + _tbl->readonly);
}

uint8_t& table::safeenv() {
	return *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(this) + _tbl->safeenv);
}

uint8_t& table::lsizenode() {
	return *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(this) + _tbl->lsizenode);
}

uint8_t& table::nodemask8() {
	return *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(this) + _tbl->nodemask8);
}

int& table::sizearray() {
	return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + _tbl->sizearray);
}

int& table::lastfree() {
	return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + _tbl->lastfree);
}

int& table::aboundary() {
	return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + _tbl->lastfree);
}

table*& table::metatable() {
	return *reinterpret_cast<table**>(reinterpret_cast<uintptr_t>(this) + _tbl->metatable);
}

tvalue*& table::array() {
	return *reinterpret_cast<tvalue**>(reinterpret_cast<uintptr_t>(this) + _tbl->array);
}

node*& table::node() {
	return *reinterpret_cast<struct node**>(reinterpret_cast<uintptr_t>(this) + _tbl->node);
}

gc_object*& table::gclist() {
	return *reinterpret_cast<gc_object**>(reinterpret_cast<uintptr_t>(this) + _tbl->gclist);
}

void call_info::init(roblox::shuffles::new_structs::ci* callinfo) {
	_ci = static_cast<roblox::shuffles::new_structs::ci*>(malloc(sizeof(roblox::shuffles::new_structs::ci)));
	memcpy(_ci, callinfo, sizeof(roblox::shuffles::new_structs::ci));
}

tvalue*& call_info::base() {
	return *reinterpret_cast<tvalue**>(reinterpret_cast<uintptr_t>(this) + _ci->base);
}

tvalue*& call_info::func() {
	return *reinterpret_cast<tvalue**>(reinterpret_cast<uintptr_t>(this) + _ci->func);
}

tvalue*& call_info::top() {
	return *reinterpret_cast<tvalue**>(reinterpret_cast<uintptr_t>(this) + _ci->top);
}

unsigned*& call_info::savedpc() {
	return *reinterpret_cast<unsigned**>(reinterpret_cast<uintptr_t>(this) + _ci->savedpc);
}

int& call_info::nresults() {
	return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + _ci->nresults);
}

/*unsigned*& call_info::savedpc() {
	return *reinterpret_cast<unsigned**>(reinterpret_cast<uintptr_t>(this) + _ci->savedpc);
}*/

/*int& call_info::nresults() {
	return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + _ci->nresults);
}*/

unsigned& call_info::flags() {
	return *reinterpret_cast<unsigned*>(reinterpret_cast<uintptr_t>(this) + _ci->flags);
}

void lua_state::init(roblox::shuffles::new_structs::ls* ls) {
	_ls = static_cast<roblox::shuffles::new_structs::ls*>(malloc(sizeof(roblox::shuffles::new_structs::ls)));
	memcpy(_ls, ls, sizeof(roblox::shuffles::new_structs::ls));
}

tvalue*& lua_state::top() {
	return *reinterpret_cast<tvalue**>(reinterpret_cast<uintptr_t>(this) + _ls->top);
}

tvalue*& lua_state::base() {
	return *reinterpret_cast<tvalue**>(reinterpret_cast<uintptr_t>(this) + _ls->base);
}

uintptr_t& lua_state::global() {
	return *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(this) + _ls->global);
}

call_info*& lua_state::ci() {
	return *reinterpret_cast<call_info**>(reinterpret_cast<uintptr_t>(this) + _ls->ci);
}

tvalue*& lua_state::stack_last() {
	return *reinterpret_cast<tvalue**>(reinterpret_cast<uintptr_t>(this) + _ls->stack_last);
}

tvalue*& lua_state::stack() {
	return *reinterpret_cast<tvalue**>(reinterpret_cast<uintptr_t>(this) + _ls->stack);
}

int& lua_state::stacksize() {
	return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + _ls->stacksize);
}

int& lua_state::size_ci() {
	return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + _ls->size_ci);
}

unsigned short& lua_state::n_ccalls() {
	return *reinterpret_cast<unsigned short*>(reinterpret_cast<uintptr_t>(this) + _ls->nCcalls);
}

unsigned short& lua_state::base_n_ccalls() {
	return *reinterpret_cast<unsigned short*>(reinterpret_cast<uintptr_t>(this) + _ls->baseCcalls);
}

int& lua_state::cached_slot() {
	return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + _ls->cachedslot);
}
