#pragma once

#include "objects.hpp"

#pragma optimize( "", off)

namespace roblox::lua::api {
	void __fastcall execute(lua_state* ls, const std::string_view& code);

	void __fastcall luau_load(lua_state* state, const std::string& bytecode, const char* chunkname, int env = 0);
	int __fastcall pcall(lua_state* state, int args, int results);
	tvalue* __fastcall index2adr(lua_state* state, int idx);
    unsigned int __fastcall luaS_hash(std::string_view string);
	void __fastcall pop(lua_state* ls);
}

#pragma optimize( "", on)