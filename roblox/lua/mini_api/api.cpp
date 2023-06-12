#include "../../../pch/pch.h"
#include "api.hpp"

#include "../../../util/offset_cache.hpp"

#include "../compiler/Compilerator.h"

#include "../../../security/spoof.hpp"

#include <zstd.h>
#include <xxhash.h>

constexpr auto clvalue(auto o) {
    return &o->value.gc->cl;
}

constexpr auto curr_func(auto L) {
    return clvalue(L->ci()->func());
}

constexpr auto adjust_results(auto L, auto nres) {
    if (nres == -1 && L->top() >= L->ci()->top()) \
        L->ci()->top() = L->top();
}


struct CallS
{ /* data to `f_call' */
    tvalue* func;
    int nresults;
};

using f_call_t = void(__cdecl*)(lua_state*, void*);


void __fastcall roblox::lua::api::execute(lua_state* ls, const std::string_view& code) {
	const auto& bytecode = Compiler::compile(code.data(), nullptr, { 0, 2 });

    luau_load(ls, bytecode, "");

    pcall(ls, 0, 1);
}


void __fastcall roblox::lua::api::luau_load(lua_state* state, const std::string& bytecode, const char* chunkname, int env) {
    /*volatile auto a5 = 0;
    volatile int result = spoof::fastcall<int>(util::find(xor("Compiler::deserialize")), state, chunkname, data, size, a5);

   __asm add esp, 12

   if (result) {
       smh::println("luau_deserialize failed with error code: ", result);
   }

   return;*/

    // deserialize got inlined, Luau::Load is all that's left.

    static const char kBytecodeMagic[] = "RSB1";
    static const unsigned int kBytecodeHashSeed = 42;
    static const unsigned int kBytecodeHashMultiplier = 41;

    int data_size = bytecode.size();
    const int max_size = ZSTD_compressBound(data_size);

    std::vector<char> compressed(max_size);
    const int compressed_size = ZSTD_compress(compressed.data(), max_size, bytecode.data(), bytecode.size(), 1);

    std::string result = kBytecodeMagic;
    result.append(reinterpret_cast<char*>(&data_size), sizeof(data_size));
    result.append(compressed.data(), compressed_size);

    const unsigned int hash = XXH32(result.data(), result.size(), kBytecodeHashSeed);

    unsigned char hb[4];
    memcpy(hb, &hash, sizeof(hash));

    for (size_t i = 0; i < result.size(); ++i)
        result[i] ^= hb[i % 4] + i * kBytecodeHashMultiplier;


    //int res = reinterpret_cast<int(__fastcall*)(lua_state*, const std::string&, const char*, int env)>(memory::rebase(0x7005F0))(state, result, chunkname, env);

    const int res = spoof::fastcall<int>(memory::rebase(0x7005F0), state, &result, chunkname, env);

    std::printf("luau_deserialize result: %i\n", res);
}


int mini_precall(lua_state* L, tvalue* func, int nresults) {
    Closure* ccl = clvalue(func);

    call_info* ci = L->ci()++;
    ci->func() = func;
    ci->base() = func + 1;
    ci->top() = L->top() + ccl->stacksize;
    ci->savedpc() = NULL;
    ci->flags() = 0;
    ci->nresults() = nresults;

    L->base() = ci->base();

    L->top() = ci->top();

    //L->ci()->savedpc() = ccl->l.p->code;

    return 0;
}

f_call_t f_call = nullptr;

__declspec(noinline) void __fastcall f_call_invoker(auto L, void* ud) {
    __asm { // fuck you cdecl
        push edi
        mov edi, finished


        push edx
        push ecx
        // fuck retaddr spoof
        jmp f_call


        finished :
        pop edi
    	add esp, 8
    }

    // this shit drives me crazier than mudock
}

int __fastcall roblox::lua::api::pcall(lua_state* state, int args, int results) {
    CallS c;
    int status;

    c.func = state->top() - (args + 1);
    c.nresults = results;

    f_call = reinterpret_cast<f_call_t>(memory::rebase(0x199F120));

    status = reinterpret_cast<int(__fastcall*)(lua_state*, f_call_t, void*)>(memory::rebase(0x1981DA0))(state, f_call, c.func);

    if (status) {
        volatile auto ts = &state->top()->value.gc->ts;
        volatile auto str = ts->data;
        smh::println("PCALL FAILED WITH REASON: ", str);
    }

	adjust_results(state, args);

    return status;
}


tvalue* __fastcall roblox::lua::api::index2adr(lua_state* state, int idx) {
    if (idx > 0)
    {
        tvalue* o = state->base() + (idx - 1);
        return o;
    }
    if (idx > -10000)
    {
	    return state->top() + idx;
    }
    switch (idx)
    { /* pseudo-indices */
    case -10000:
    /*return registry(L);*/
    case -10001:
    {
	    /*sethvalue(L, &L->env, getcurrenv(L));
            return &L->env;*/
    }
    case -10002:
	    return 0;
    default:
    {
	    Closure* func = curr_func(state);
	    idx = -10002 - idx;
	    return (idx <= func->nupvalues) ? &func->c.upvals[idx - 1] : nullptr;
    }
    }
}

unsigned __fastcall roblox::lua::api::luaS_hash(std::string_view string) {
    const char* str = string.data();
    volatile size_t len = string.size();

    volatile unsigned int a = 0, b = 0;
    volatile unsigned int h = static_cast<unsigned>(len);

    // hash prefix in 12b chunks (using aligned reads) with ARX based hash (LuaJIT v2.1, lookup3)
    // note that we stop at length<32 to maintain compatibility with Lua 5.1
    while (len >= 32)
    {
#define rol(x, s) ((x >> s) | (x << (32 - s)))
#define mix(u, v, w) a ^= h, a -= rol(h, u), b ^= a, b -= rol(a, v), h ^= b, h -= rol(b, w)

        // should compile into fast unaligned reads
        uint32_t block[3];
        memcpy(block, str, 12);

        a += block[0];
        b += block[1];
        h += block[2];
        mix(14, 11, 25);
        str += 12;
        len -= 12;

#undef mix
#undef rol
    }

    // original Lua 5.1 hash for compatibility (exact match when len<32)
    for (size_t i = len; i > 0; --i)
        h ^= (h << 5) + (h >> 2) + (uint8_t)str[i - 1];

    return h;
}

void __fastcall roblox::lua::api::pop(lua_state* ls) {
    ls->top()--;
}