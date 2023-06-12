#pragma once

#include <cstdint>

namespace roblox::shuffles::new_structs {
	struct tbl;
	struct gch;
	struct ls;
	struct ci;
}

typedef union
{
	double u;
	void* s;
	long l;
} L_Umaxalign;

constexpr auto LUA_VECTOR_SIZE = 3;	/* must be 3 or 4 */

constexpr auto LUA_EXTRA_SIZE = LUA_VECTOR_SIZE - 2;

union gc_object;

typedef union
{
	gc_object* gc;
	void* p;
	double n;
	int b;
	float v[2]; // v[0], v[1] live here; v[2] lives in TValue::extra
} value;

/*
** Tagged Values
*/

typedef struct tvalue
{
	value value;
	int extra[LUA_EXTRA_SIZE];
	int tt;
} tvalue;

static_assert(sizeof(tvalue) == 16, "TValue wrong size");

class gc_header {
public:
	static void init(roblox::shuffles::new_structs::gch* gcheader);

	uint8_t& type();
	uint8_t& marked();
	uint8_t& memcat();
};

class table {
public:
	static void init(roblox::shuffles::new_structs::tbl* tbl);
	static void dump_function(const std::string_view& path, roblox::shuffles::new_structs::tbl* tbl);

	uint8_t& flags();
	uint8_t& readonly();
	uint8_t& safeenv();
	uint8_t& lsizenode();
	uint8_t& nodemask8();

	int& sizearray();

	/* union */
	int& lastfree();
	int& aboundary();
	/* */

	table*& metatable();
	tvalue*& array();
	struct node*& node();
	gc_object*& gclist();
};

class call_info {
private:
	uint8_t _[28]; // 28;
public:
	static void init(roblox::shuffles::new_structs::ci* callinfo);

	tvalue*& base();
	tvalue*& func();
	tvalue*& top();
	unsigned*& savedpc();

	int& nresults();
	unsigned int& flags();
};

static_assert(sizeof(call_info) == 28, "Invalid CI size");

typedef struct Closure
{
	gc_header gch;

	uint8_t isC;
	uint8_t nupvalues;
	uint8_t stacksize;
	uint8_t preload;

	gc_object* gclist;
	struct Table* env;

	union
	{
		struct
		{
			uintptr_t f;
			uintptr_t cont;
			const char* debugname;
			tvalue upvals[1];
		} c;

		struct
		{
			struct Proto* p;
			tvalue uprefs[1];
		} l;
	};
} Closure;

typedef struct TString
{
	gc_header gch;

	int16_t atom;

	TString* next; // next string in the hash table bucket or the string buffer linked list

	unsigned int hash;
	unsigned int len;

	char data[1]; // string data is allocated right after the header
} TString;

typedef struct key
{
	value value;
	int extra[LUA_EXTRA_SIZE];
	unsigned tt : 4;
	int next : 28; /* for chaining */
} key;

typedef struct node
{
	tvalue val;
	key key;
} node;

typedef struct Udata
{
	gc_header gch;

	uint8_t tag;

	int len;

	table* metatable;

	union
	{
		char data[1];      // userdata is allocated right after the header
		L_Umaxalign dummy; // ensures maximum alignment for data
	};
} Udata;

typedef struct Proto {

};

union gc_object {
	gc_header gch;
	table t;
	Closure cl;
	TString ts;
	Udata u;
	Proto p;
};

class lua_state {
public:
	static constexpr std::uintptr_t script_context_offset = 476;
private:
	std::uintptr_t ls;

public:
	static void init(roblox::shuffles::new_structs::ls* ls);

	tvalue*& top();
	tvalue*& base();
	uintptr_t& global();
	call_info*& ci();
	tvalue*& stack_last();
	tvalue*& stack();

	int& stacksize();
	int& size_ci();

	unsigned short& n_ccalls();
	unsigned short& base_n_ccalls();

	int& cached_slot();
};