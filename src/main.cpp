#include "../pch/pch.h"

#include "../memory/scanner.hpp"

#include "../util/offset_cache.hpp"

#include "../roblox/shuffles/structs.hpp"

#include "../roblox/shuffles/shuffles.hpp"

#include "../roblox/scheduler/scheduler.hpp"

#include "../roblox/obfuscations/bruteforce.hpp"

#include "../roblox/obfuscations/deobfuscate.hpp"

#include "../roblox/obfuscations/obfuscate.hpp"

#include "../roblox/lua/mini_api/objects.hpp"

#include "../roblox/lua/compiler/Compilerator.h"

#include "../roblox/lua/mini_api/api.hpp"

#include "../memory/rebase.hpp"
#include "../memory/pattern.hpp"

#include "../security/spoof.hpp"

#include "../util/capstone_engine.hpp"

// as with all exploiting-related sources: things are typically not written the best, things are simply written to work.
// the reasoning for this is simple: roblox will simply break it 1 week after you wrote it.
// enjoy the suffering

using namespace std::string_view_literals;
using namespace std::string_literals;

using scheduler_sig = std::uintptr_t(__cdecl*)();

static capstone_engine engine{};

constexpr std::array<scheduler_sig, 2> scheduler_sigs{
	[] {
		if (const auto& scheduler_xref = memory::scan_string(std::string(xor("WatcherThread Detected hang.")).c_str()); scheduler_xref.size() == 1) {
			std::uintptr_t result = 0;
			auto&& [insn, count] = engine.disassemble(capstone_engine::get_prologue<previous>(scheduler_xref.front()));

			for (int i = 0; i < count; i++) {
				if (auto&& detail = insn[i].detail->x86; insn[i].id == X86_INS_MOV && detail.operands[0].type == X86_OP_REG && detail.operands[1].type == X86_OP_REG && detail.operands[0].reg == X86_REG_ECX && detail.operands[1].reg == X86_REG_EAX) {
					result = insn[i].get_previous_mnemonic<X86_INS_CALL>().get_imm<0>();
						// get_imm<0>(get_previous_mnemonic<X86_INS_CALL>(&insn[i]));
					break;
				}
			}
			cs_free(insn, count);
			return result;
		}
	},
	[] {
		if (const auto& scheduler_xref = memory::scan_string(std::string(xor("stepContactsAsyncPrepare")).c_str()); scheduler_xref.size() == 1) {
			std::uintptr_t result = 0;
			auto&& [insn, count] = engine.disassemble(capstone_engine::get_prologue<previous>(scheduler_xref.front()));

			for (int i = 0; i < count; i++) {
				if (auto&& detail = insn[i].detail->x86; insn[i].id == X86_INS_LEA && detail.operands[0].type == X86_OP_REG && detail.operands[1].mem.base == X86_REG_ESI && detail.operands[1].mem.disp > 100) {
					result = insn[i].get_previous_mnemonic<X86_INS_CALL>().get_imm<0>();
				}
			}

			cs_free(insn, count);
			return result;
		}

		return std::uintptr_t();
	}
};

int epic() {
	clock_t start_clock = clock();

	//capstone_engine::init();

	util::insert(std::string(xor("gettaskscheduler")).c_str(), scheduler_sigs.back()());

	roblox::shuffles::init();

	[] {
		//using namespace capstone_engine;

		if (const auto& callcheck_xref = memory::scan_string(xor("ROBLOX_singletonEvent"), true); callcheck_xref.size() == 1) {
			auto&& [raw_instructions, count] = engine.disassemble(capstone_engine::get_prologue<previous>(callcheck_xref.front()));
			for (unsigned i = 0; i < count; i++) {
				if (raw_instructions[i].is(X86_INS_CALL)) {
					util::insert(xor("callcheck"), raw_instructions[i].address);
					return cs_free(raw_instructions, count);
				}
			}
			cs_free(raw_instructions, count);
		}
	}();

	[] {
		auto pat = memory::pattern_scan("C1 E0 04 03 C6 5E C3 B8 ? ? ? ? 5E C3");

		if (pat) {
			util::insert(xor ("index2adr"), capstone_engine::get_prologue<previous>(pat));
		}
	}();

	[] {
		const auto& pcall_xref = memory::scan_string(xor("pcall"), true);

		auto&& [pcall_instructions, count] = engine.disassemble(capstone_engine::get_prologue<previous>(pcall_xref.front()));

		for (int i = 0; i < count; i++) {
			auto&& insn = pcall_instructions[i];

			if (insn.id == X86_INS_XORPS && insn.detail->x86.operands[1].mem.disp) {
				util::insert(xor("n_xor"), insn.detail->x86.operands[1].mem.disp);
				break;
			}
		}

		cs_free(pcall_instructions, count);
	}();


	// finds an address for a retaddr spoof/retcheck bp, we use a random (typically VMP-generated) "jmp edi" for this
	if constexpr (true) [] {
		auto addr = memory::rebase(0x80000);

		bool break_while = false;

		

		while (!spoof_address) {
			auto [cs_insn, count] = engine.disassemble(addr, addr + 10000);

			for (int i = 0; i < count; i++) {
				auto&& insn = cs_insn[i];

				if (insn.id == X86_INS_JMP && insn.detail->x86.operands[0].type == X86_OP_REG && insn.detail->x86.operands[0].reg == X86_REG_EDI) {
					smh::println(xor ("Spoof entry-point (ty kind spoof ^^): 0x"), std::hex, memory::unbase(static_cast<int>(insn.address)), std::dec);
					spoof_address = (static_cast<int>(insn.address));
					break_while = true;
					break;
				}
			}

			addr += 10000;

			cs_free(cs_insn, count);

			if (break_while)
				break;
		}
	}();

	roblox::shuffles::new_structs::ls deob_ls{};
	roblox::shuffles::new_structs::gch deob_gch{};
	roblox::shuffles::new_structs::ci deob_ci{};
	roblox::shuffles::new_structs::strt deob_strt{};
	roblox::shuffles::new_structs::global deob_global{};
	roblox::shuffles::new_structs::proto deob_proto{};

	auto& scheduler = *spoof::call<roblox::scheduler::task_scheduler*>(util::find(xor("gettaskscheduler")));

	scheduler.hook_job();

	auto script_context = scheduler.get_script_context();

	auto get_tt = [](std::string_view name) {
		for (int i = 0; i < types.size(); i++) {
			if (types[i] == name)
				return i;
		}

		return 0;
	};

	roblox::shuffles::deobfuscate({ &deob_gch.type, &deob_gch.marked, &deob_gch.modkey });

	auto ls_obf = roblox::obfuscations::get_ptr_obfuscation(script_context + lua_state::script_context_offset, deob_gch.type, 1, { get_tt(xor("thread")) });

	smh::println(xor("lua State obf: "), ls_obf.deobf_op, " ", ls_obf.deobf_dir, " ", ls_obf.obf_op, " ", ls_obf.obf_dir, " | ", ls_obf.parse_expression());

	auto ls = roblox::obfuscations::deobfuscate<lua_state*>(script_context, lua_state::script_context_offset, ls_obf.deobf_op, ls_obf.deobf_dir);

	roblox::shuffles::deobfuscate({ &deob_strt.hash, &deob_strt.size, &deob_strt.capacity });

	roblox::shuffles::init_vm_shuffle6((uintptr_t)ls, deob_strt.capacity);

	// NOTE: DO NOT use ls here, use it AFTER shuffle deobfuscation and ls init.
	// Things will obviously go wrong otherwise...
	// why did i do this

	roblox::shuffles::deobfuscate({ &deob_ls.gt, &deob_ls.openupval, &deob_ls.gclist });
	roblox::shuffles::deobfuscate({ &deob_ci.base, &deob_ci.func, &deob_ci.top, &deob_ci.savedpc });
	roblox::shuffles::deobfuscate({ &deob_ls.top, &deob_ls.base, &deob_ls.global, &deob_ls.ci, &deob_ls.stack_last, &deob_ls.stack });

	gc_header::init(&deob_gch);
	lua_state::init(&deob_ls);
	call_info::init(&deob_ci);

	auto stacksize_obf = roblox::obfuscations::get_obfuscation<int>(reinterpret_cast<int>(ls) + deob_ls.stacksize, 0, { 45 });

	smh::println(xor("stacksize obf: "), stacksize_obf.deobf_op, " ", stacksize_obf.deobf_dir, " ", stacksize_obf.obf_op, " ", stacksize_obf.obf_dir, " | ", stacksize_obf.parse_expression());

	auto global_obf = roblox::obfuscations::get_ptr_obfuscation(reinterpret_cast<uintptr_t>(ls) + deob_ls.global, deob_strt.capacity, 4, { 256, 512, 1024, 2048, 4096, 8192, 16384, 32768 });

	smh::println(xor("global obf: "), global_obf.deobf_op, " ", global_obf.deobf_dir, " ", global_obf.obf_op, " ", global_obf.obf_dir, " | ", global_obf.parse_expression());

	auto global = roblox::obfuscations::deobfuscate<uintptr_t>(reinterpret_cast<uintptr_t>(ls), deob_ls.global, global_obf.deobf_op, global_obf.deobf_dir);

	roblox::shuffles::init_vm_shuffle5(global);

	roblox::shuffles::new_structs::tbl deob_tbl{};

	roblox::shuffles::deobfuscate({ &deob_global.GCthreshold, &deob_global.totalbytes, &deob_global.gcgoal, &deob_global.gcstepmul, &deob_global.gcstepsize });
	roblox::shuffles::deobfuscate({ &deob_tbl.flags, &deob_tbl.readonly, &deob_tbl.safeenv, &deob_tbl.lsizenode, &deob_tbl.nodemask8 });
	roblox::shuffles::deobfuscate({ &deob_tbl.metatable, &deob_tbl.array, &deob_tbl.node, &deob_tbl.gclist });
	roblox::shuffles::deobfuscate({ &deob_global.mainthread, &deob_global.uvhead, &deob_global.mt, &deob_global.ttname, &deob_global.tmname });

	table::init(&deob_tbl);
	//table::dump_function(xor("Table.txt"), &deob_tbl);

	roblox::obfuscations::obfuscation_t string_len_obf{};
	roblox::obfuscations::obfuscation_t string_hash_obf{};

	[ls, &string_len_obf, &string_hash_obf] { /* TString */
		using namespace roblox;
		using namespace obfuscations;
		using namespace lua::api;


		execute(ls, xor("return 'asdf123'"));

		volatile auto val = &ls->top()->value.gc->ts;

		volatile auto smh_hash = luaS_hash(xor("asdf123"));

		string_len_obf = get_obfuscation<unsigned>(reinterpret_cast<uintptr_t>(val), offsetof(roblox::shuffles::original_structs::TString, len), {7});
		string_hash_obf = get_obfuscation<unsigned>(reinterpret_cast<uintptr_t>(val), offsetof(roblox::shuffles::original_structs::TString, hash), { smh_hash });

		smh::println(xor("TString len obf: "), string_len_obf.deobf_op, " ", string_len_obf.deobf_dir, " ", string_len_obf.obf_op, " ", string_len_obf.obf_dir, " | ", string_len_obf.parse_expression());
		smh::println(xor("TString hash obf: "), string_hash_obf.deobf_op, " ", string_hash_obf.deobf_dir, " ", string_hash_obf.obf_op, " ", string_hash_obf.obf_dir, " | ", string_hash_obf.parse_expression());

		ls->top()--;
	}();

	roblox::obfuscations::obfuscation_t table_array_obf{};
	roblox::obfuscations::obfuscation_t table_node_obf{};
	roblox::obfuscations::obfuscation_t table_mt_obf{};

	[ls, deob_tbl, deob_gch, get_tt, &table_array_obf, &table_node_obf, &table_mt_obf] { /* Table */
		using namespace roblox;
		using namespace obfuscations;
		using namespace lua::api;

		execute(ls, xor("local tbl = { 'hello!', 'it', 'is', 'gamer', 'time.' }; table.freeze(tbl); return tbl"));
		volatile auto val = &ls->top()->value.gc->t;
		table_array_obf = get_ptr_obfuscation(reinterpret_cast<uintptr_t>(val) + deob_tbl.array, offsetof(tvalue, tt), 4, { 5 });
		smh::println(xor("Table array obf: "), table_array_obf.deobf_op, " ", table_array_obf.deobf_dir, " ", table_array_obf.obf_op, " ", table_array_obf.obf_dir, " | ", table_array_obf.parse_expression());

		ls->top()--;

		execute(ls, xor("return { weight = 50, wally = 'fat' }"));
		val = &ls->top()->value.gc->t;
		table_node_obf = get_ptr_obfuscation(reinterpret_cast<uintptr_t>(val) + deob_tbl.node, offsetof(node, val.tt), 4, { 5 });
		smh::println(xor("Table node obf: "), table_node_obf.deobf_op, " ", table_node_obf.deobf_dir, " ", table_node_obf.obf_op, " ", table_node_obf.obf_dir, " | ", table_mt_obf.parse_expression());
		ls->top()--;

		execute(ls, xor(R"""( local a={1,2,3,gamer="vision",epically="epic"}setmetatable(a,{__tostring=function(b,c)return'epic object'end,__index=function(b,c) return getbitches() end})return a )"""));
		val = &ls->top()->value.gc->t;
		table_mt_obf = get_ptr_obfuscation(reinterpret_cast<uintptr_t>(val) + deob_tbl.metatable, deob_gch.type, 1, { get_tt("table") });
		smh::println(xor("Table metatable obf: "), table_mt_obf.deobf_op, " ", table_mt_obf.deobf_dir, " ", table_mt_obf.obf_op, " ", table_mt_obf.obf_dir, " | ", table_mt_obf.parse_expression());
		ls->top()--;
	}();

	[ls, deob_tbl, &table_node_obf] {
		using namespace roblox;
		using namespace obfuscations;
		using namespace lua::api;

		execute(ls, xor("return {}"));
		auto val = &ls->top()->value.gc->t;

		const auto dummynode = deobfuscate<uintptr_t>(reinterpret_cast<uintptr_t>(val), deob_tbl.node, table_node_obf.deobf_op, table_node_obf.deobf_dir);

		smh::println(xor("dummynode: 0x"), std::hex, memory::unbase(dummynode), std::dec);

		ls->top()--;

		//execute(ls, xor("return nil"));

		

		//ls->top()--;
	}();

	roblox::obfuscations::obfuscation_t udata_mt_obf{};

	[ls, deob_gch, get_tt, &udata_mt_obf] { /* Udata */

		using namespace roblox;
		using namespace obfuscations;
		using namespace lua::api;

		execute(ls, xor("return game.Players.LocalPlayer"));
		auto top = ls->top();
		auto val = &top->value.gc->u;

		udata_mt_obf = get_ptr_obfuscation(reinterpret_cast<uintptr_t>(val) + offsetof(Udata, metatable), deob_gch.type, 1, {get_tt("table")});
		smh::println(xor("Udata mt obf: "), udata_mt_obf.deobf_op, " ", udata_mt_obf.deobf_dir, " ", udata_mt_obf.obf_op, " ", udata_mt_obf.obf_dir, " | ", udata_mt_obf.parse_expression());
		ls->top()--;
	}();

	roblox::obfuscations::obfuscation_t cl_l_proto_obf{};
	roblox::obfuscations::obfuscation_t proto_k_obf{};
	roblox::obfuscations::obfuscation_t proto_code_obf{};
	roblox::obfuscations::obfuscation_t proto_abslineinfo_obf{};
	roblox::obfuscations::obfuscation_t proto_source_obf{};
	roblox::obfuscations::obfuscation_t proto_debugname_obf{};

	[ls, deob_gch, &cl_l_proto_obf, &deob_proto, &proto_k_obf, &proto_code_obf, &proto_abslineinfo_obf, &proto_source_obf, &proto_debugname_obf] { /* Proto */
		using namespace roblox;
		using namespace obfuscations;
		using namespace lua::api;

		execute(ls, xor("return function() print'gaming' end"));
		auto top = ls->top();
		auto val = &top->value.gc->p;

		cl_l_proto_obf = iterate_obfuscations([val, deob_gch](obf_obfuscation obf, obf_direction dir) {
			const auto deobfuscated_ptr = deobfuscate<std::uintptr_t>(reinterpret_cast<uintptr_t>(val), offsetof(Closure, l.p), obf, dir);

			std::uint8_t type;
			std::uint8_t marked;

			ReadProcessMemory(GetCurrentProcess(), (LPCVOID)(deobfuscated_ptr + deob_gch.type), &type, 1, nullptr);
			ReadProcessMemory(GetCurrentProcess(), (LPCVOID)(deobfuscated_ptr + deob_gch.marked), &marked, 1, nullptr);

			return type == 10 && marked <= 10;
			});

		cl_l_proto_obf = get_ptr_obfuscation(reinterpret_cast<uintptr_t>(val) + offsetof(Closure, l.p), deob_gch.type, 1, { 10 });
		smh::println(xor("Closure proto obf: "), cl_l_proto_obf.deobf_op, " ", cl_l_proto_obf.deobf_dir, " ", cl_l_proto_obf.obf_op, " ", cl_l_proto_obf.obf_dir, " | ", cl_l_proto_obf.parse_expression());
		ls->top()--;


		// p->sizecode: 24
		// p->sizep: 4
		// p->sizelocvars: 3
		// p->sizeupvalues: 0
		// p->sizek: 5
		// p->sizelineinfo: 32
		// p->linegaplog2: 24
		// p->linedefined: 2

		// this set offsets in a proto to values that we know, and therefore we can bruteforce the shuffle.
		execute(ls, xor(R"(
return function(epic, gaming, time)
    print('hello world', 'gamer vision', 'asdf');
    print(epic, gaming, time);
    (function() end)(); (function() end)(); (function() end)(); (function() end)();
    print("bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla")
end
)"));
		top = ls->top();
		val = &top->value.gc->p;

		auto deobfuscated_p = deobfuscate<std::uintptr_t>(reinterpret_cast<uintptr_t>(val), offsetof(Closure, l.p), cl_l_proto_obf.deobf_op, cl_l_proto_obf.deobf_dir);

		shuffles::init_vm_shuffle8(deobfuscated_p);


		shuffles::deobfuscate({ &deob_proto.k, &deob_proto.code, &deob_proto.p, &deob_proto.lineinfo, &deob_proto.abslineinfo, &deob_proto.locvars, &deob_proto.upvalues, &deob_proto.source });
		shuffles::deobfuscate({ &deob_proto.sizecode, &deob_proto.sizep, &deob_proto.sizelocvars, &deob_proto.sizeupvalues, &deob_proto.sizek, &deob_proto.sizelineinfo, &deob_proto.linegaplog2, &deob_proto.linedefined });

		ls->top()--;

		std::cout << xor("p->k: ") << deob_proto.k << '\n';
		std::cout << xor("p->code: ") << deob_proto.code << '\n';
		std::cout << xor("p->protos: ") << deob_proto.p << '\n';
		std::cout << xor("p->lineinfo: ") << deob_proto.lineinfo << '\n';
		std::cout << xor("p->abslineinfo: ") << deob_proto.abslineinfo << '\n';
		std::cout << xor("p->locvars: ") << deob_proto.locvars << '\n';
		std::cout << xor("p->upvalues: ") << deob_proto.upvalues << '\n';
		std::cout << xor("p->source: ") << deob_proto.source << '\n';

		std::cout << xor("p->sizecode: ") << deob_proto.sizecode << '\n';
		std::cout << xor("p->sizep: ") << deob_proto.sizep << '\n';
		std::cout << xor("p->sizelocvars: ") << deob_proto.sizelocvars << '\n';
		std::cout << xor("p->sizeupvalues: ") << deob_proto.sizeupvalues << '\n';
		std::cout << xor("p->sizek: ") << deob_proto.sizek << '\n';
		std::cout << xor("p->sizelineinfo: ") << deob_proto.sizelineinfo << '\n';
		std::cout << xor("p->linegaplog2: ") << deob_proto.linegaplog2 << '\n';
		std::cout << xor("p->linedefined: ") << deob_proto.linedefined << '\n';

		proto_k_obf = iterate_obfuscations([deobfuscated_p, deob_gch, deob_proto](obf_obfuscation obf, obf_direction dir) {
			auto deobfuscated_pointer = deobfuscate<uintptr_t>(deobfuscated_p, deob_proto.k, obf, dir);

			uint8_t buf;

			ReadProcessMemory(GetCurrentProcess(), (LPCVOID)(deobfuscated_pointer + deob_gch.type), &buf, 1, nullptr);

			return buf == 5;
			});

		proto_code_obf = iterate_obfuscations([deobfuscated_p, deob_proto](obf_obfuscation obf, obf_direction dir) {
			const auto deobfuscated_pointer = deobfuscate<uintptr_t>(deobfuscated_p, deob_proto.code, obf, dir);

			int buf;

			ReadProcessMemory(GetCurrentProcess(), (LPCVOID)(deobfuscated_pointer), &buf, 4, nullptr);

			return buf == 0x46000335;
			});

		proto_abslineinfo_obf = iterate_obfuscations([deobfuscated_p, deob_proto](obf_obfuscation obf, obf_direction dir) {
			auto deobfuscated_pointer = deobfuscate<uintptr_t>(deobfuscated_p, deob_proto.abslineinfo, obf, dir);

			int buf;

			ReadProcessMemory(GetCurrentProcess(), (LPCVOID)(deobfuscated_pointer), &buf, 4, nullptr);

			return buf == 3;
			});

		proto_source_obf = iterate_obfuscations([deobfuscated_p, deob_gch, deob_proto](obf_obfuscation obf, obf_direction dir) {
			auto deobfuscated_pointer = deobfuscate<uintptr_t>(deobfuscated_p, deob_proto.source, obf, dir);

			uint8_t buf;

			ReadProcessMemory(GetCurrentProcess(), (LPCVOID)(deobfuscated_pointer + deob_gch.type), &buf, 1, nullptr);

			return buf == 5;
			});

		smh::println(xor("p->k obf: "), proto_k_obf.deobf_op, " ", proto_k_obf.deobf_dir, " ", proto_k_obf.obf_op, " ", proto_k_obf.obf_dir, " | ", proto_k_obf.parse_expression());
		smh::println(xor("p->code obf: "), proto_code_obf.deobf_op, " ", proto_code_obf.deobf_dir, " ", proto_code_obf.obf_op, " ", proto_code_obf.obf_dir, " | ", proto_code_obf.parse_expression());
		smh::println(xor("p->abslineinfo obf: "), proto_abslineinfo_obf.deobf_op, " ", proto_abslineinfo_obf.deobf_dir, " ", proto_abslineinfo_obf.obf_op, " ", proto_abslineinfo_obf.obf_dir, " | ", proto_abslineinfo_obf.parse_expression());
		smh::println(xor("p->source obf: "), proto_source_obf.deobf_op, " ", proto_source_obf.deobf_dir, " ", proto_source_obf.obf_op, " ", proto_source_obf.obf_dir, " | ", proto_source_obf.parse_expression());

		execute(ls, xor(R"(
function epic()

end 
return epic)"));
		top = ls->top();
		val = &top->value.gc->p;

		deobfuscated_p = deobfuscate<std::uintptr_t>(reinterpret_cast<uintptr_t>(val), offsetof(Closure, l.p), cl_l_proto_obf.deobf_op, cl_l_proto_obf.deobf_dir);

		proto_debugname_obf = iterate_obfuscations([deobfuscated_p, deob_gch, deob_proto](obf_obfuscation obf, obf_direction dir) {
			auto deobfuscated_pointer = deobfuscate<uintptr_t>(deobfuscated_p + deob_proto.debugname, 0, obf, dir);

			if (!deobfuscated_pointer)
				return false;

			uint8_t buf = 0;

			SIZE_T bytesRead = 0;

			ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>((uintptr_t)(deobfuscated_pointer)), &buf, 1, &bytesRead);

			if (!bytesRead)
				return false;

			return strcmp(reinterpret_cast<const char*>(deobfuscated_pointer), "") == 0;
			});

		ls->top()--;

		smh::println(xor("p->debugname obf: "), proto_debugname_obf.deobf_op, " ", proto_debugname_obf.deobf_dir, " ", proto_debugname_obf.obf_op, " ", proto_debugname_obf.obf_dir, " | ", proto_debugname_obf.parse_expression());
	}();

	roblox::obfuscations::obfuscation_t cl_c_func_obf{};
	roblox::obfuscations::obfuscation_t cl_c_cont_obf{};
	roblox::obfuscations::obfuscation_t cl_c_debugname_obf{};

	[ls, &cl_c_func_obf, &cl_c_cont_obf, &cl_c_debugname_obf] { /* Closure */
		using namespace roblox;
		using namespace obfuscations;
		using namespace lua::api;

		execute(ls, xor("return pcall"));
		auto top = ls->top();
		auto val = &top->value.gc->cl;

		cl_c_func_obf = deobfuscate_ptr(reinterpret_cast<uintptr_t>(val) + offsetof(Closure, c.f), 0);
		cl_c_cont_obf = deobfuscate_ptr(reinterpret_cast<uintptr_t>(val) + offsetof(Closure, c.cont), 0);
		cl_c_debugname_obf = roblox::obfuscations::get_obfuscation<unsigned>(reinterpret_cast<uintptr_t>(val), offsetof(Closure, c.debugname), {memory::get_str_xref(xor("pcall"))});

		smh::println(xor("Closure c-func obf: "), cl_c_func_obf.deobf_op, " ", cl_c_func_obf.deobf_dir, " ", cl_c_func_obf.obf_op, " ", cl_c_func_obf.obf_dir, " | ", cl_c_func_obf.parse_expression());
		smh::println(xor("Closure c-cont obf: "), cl_c_cont_obf.deobf_op, " ", cl_c_cont_obf.deobf_dir, " ", cl_c_cont_obf.obf_op, " ", cl_c_cont_obf.obf_dir, " | ", cl_c_cont_obf.parse_expression());
		smh::println(xor("Closure c-dbgname obf: "), cl_c_debugname_obf.deobf_op, " ", cl_c_debugname_obf.deobf_dir, " ", cl_c_debugname_obf.obf_op, " ", cl_c_debugname_obf.obf_dir, " | ", cl_c_debugname_obf.parse_expression());
		ls->top()--;
	}();

	auto end_clock = clock();

	int64_t TYPE_KEY = 0;

	union conversion {
		int64_t int64;
		uint8_t bytes[8];
	};

	conversion conv(TYPE_KEY);

	conv.bytes[0] = roblox::shuffles::get_tt(xor("userdata")).front();
	conv.bytes[1] = roblox::shuffles::get_tt(xor("number")).front();
	conv.bytes[2] = roblox::shuffles::get_tt(xor("vector")).front();
	conv.bytes[3] = roblox::shuffles::get_tt(xor("table")).front();
	conv.bytes[4] = roblox::shuffles::get_tt(xor("function")).front();
	conv.bytes[5] = roblox::shuffles::get_tt(xor("userdata")).back();
	conv.bytes[6] = roblox::shuffles::get_tt(xor("thread")).front();

	std::cout << xor("constexpr int64_t TYPE_KEY = ") << std::to_string(conv.int64) << xor(";") << '\n';

	std::cout << "Done! (took " << end_clock - start_clock << "ms in total)\n";

	enum calling_convention {
		___fastcall,
		___cdecl,
		___thiscall,
		___stdcall
	};

	nlohmann::json functions{
		{ "rawrunprotected", {
			{ "address", memory::unbase(util::find(xor ("rawrunprotected"))) },
			{ "cc", ___fastcall }
		}},

		{ "step", {
			{ "address", memory::unbase(util::find(xor ("luaC_step"))) },
			{ "cc", ___fastcall }
		}},

		{ "f_call", {
			{ "address", memory::unbase(util::find(xor ("f_call"))) },
			{ "cc", ___fastcall }
		}},

		{ "rawrunprotected", {
			{ "address", memory::unbase(util::find(xor ("rawrunprotected"))) },
			{ "cc", ___fastcall }
		}},

		{ "deserialize", {
			{ "address", memory::unbase(util::find(xor ("Compiler::deserialize"))) },
			{ "cc", ___fastcall }
		}},

		{ "luau_execute", {
			{ "address", memory::unbase(util::find(xor ("luau_execute"))) },
			{ "cc", ___fastcall }
		}},

		{ "calltmres", {
			{ "address", memory::unbase(util::find(xor ("callTMres"))) },
			{ "cc", ___fastcall }
		}},

		{ "calltm", {
			{ "address", memory::unbase(util::find(xor ("callTM"))) },
			{ "cc", ___fastcall }
		}},

		{ "auxgetinfo", {
			{ "address", memory::unbase(util::find(xor ("auxgetinfo"))) },
			{ "cc", ___fastcall }
		}},

		{ "pushinstance", {
			{ "address", memory::unbase(util::find(xor ("pushinstance"))) },
			{ "cc", ___cdecl }
		}},

		{ "fireclickdetector", {
			{ "address", memory::unbase(util::find(xor ("fireclickdetector"))) },
			{ "cc", ___thiscall }
		}},

		{ "firetouchinterest", {
			{ "address", memory::unbase(util::find(xor ("firetouchinterest"))) },
			{ "cc", ___stdcall }
		}},

		{ "fireproximityprompt", {
			{ "address", memory::unbase(util::find(xor ("fireproximityprompt"))) },
			{ "cc", ___thiscall }
		}},
	};

	nlohmann::json constants{
		//{ xor ("spoof"), memory::unbase(spoof::address) },
		{ xor ("nilobject"), 0 },
		{ xor ("dummynode"), 0 },
		{ xor ("sizeclass"), 0 },
		{ xor ("number_xor"), 0 },
		{ xor ("scheduler"), 0 },
		{ xor ("callcheck"), 0 },
	};

	nlohmann::json offsets{
		{ xor ("task_scheduler::job"), {
			{ xor ("script_context"), 308 }
		}},
		{ xor ("script_context"), {
			{ xor ("state"), lua_state::script_context_offset }
		}},
		{ xor ("string"), {
			{ xor ("atom"), offsetof(roblox::shuffles::original_structs::TString, atom) },
			{ xor ("next"), offsetof(roblox::shuffles::original_structs::TString, next) },
			{ xor ("hash"), offsetof(roblox::shuffles::original_structs::TString, hash) },
			{ xor ("len"), offsetof(roblox::shuffles::original_structs::TString, atom) },
			{ xor ("data"), offsetof(roblox::shuffles::original_structs::TString, atom) },
		}},
		{ xor ("table"), {
			{ xor ("tmcache"), deob_tbl.flags },
			{ xor ("readonly"), deob_tbl.readonly },
			{ xor ("safeenv"), deob_tbl.safeenv },
			{ xor ("lsizenode"), deob_tbl.lsizenode },
			{ xor ("nodemask8"), deob_tbl.nodemask8 },
			{ xor ("sizearray"), deob_tbl.sizearray },
			{ xor ("lastfree"), deob_tbl.lastfree },
			{ xor ("metatable"), deob_tbl.metatable },
			{ xor ("array"), deob_tbl.array },
			{ xor ("node"), deob_tbl.node },
			{ xor ("gclist"), deob_tbl.gclist },
		}},
		{ xor ("global"), {
			{ xor ("strt"), offsetof(roblox::shuffles::original_structs::global_State, strt) },

			{ xor ("frealloc"), offsetof(roblox::shuffles::original_structs::global_State, frealloc) },
			{ xor ("ud"), offsetof(roblox::shuffles::original_structs::global_State, ud) },

			{ xor ("currentwhite"), offsetof(roblox::shuffles::original_structs::global_State, currentwhite) },
			{ xor ("gcstate"), offsetof(roblox::shuffles::original_structs::global_State, gcstate) },

			{ xor ("gray"), deob_global.gray },
			{ xor ("grayagain"), deob_global.grayagain },
			{ xor ("weak"), deob_global.weak },

			{ xor ("GCthreshold"), deob_global.GCthreshold },
			{ xor ("totalbytes"), deob_global.totalbytes },
			{ xor ("gcgoal"), deob_global.gcgoal },
			{ xor ("gcstepmul"), deob_global.gcstepmul },
			{ xor ("gcstepsize"), deob_global.gcstepsize },

			{ xor ("freepages"), offsetof(roblox::shuffles::original_structs::global_State, freepages) },
			{ xor ("freegcopages"), offsetof(roblox::shuffles::original_structs::global_State, freegcopages) },
			{ xor ("allgcopages"), offsetof(roblox::shuffles::original_structs::global_State, allgcopages) },
			{ xor ("sweepgcopage"), offsetof(roblox::shuffles::original_structs::global_State, sweepgcopage) },
			{ xor ("memcatbytes"), offsetof(roblox::shuffles::original_structs::global_State, memcatbytes) },

			{ xor ("mainthread"), deob_global.mainthread },
			{ xor ("uvhead"), deob_global.uvhead },
			{ xor ("mt"), deob_global.mt },
			{ xor ("ttname"), deob_global.ttname },
			{ xor ("tmname"), deob_global.tmname },

			{ xor ("pseudotemp"), offsetof(roblox::shuffles::original_structs::global_State, pseudotemp) },

			{ xor ("registry"), offsetof(roblox::shuffles::original_structs::global_State, registry) },
			{ xor ("registryfree"), offsetof(roblox::shuffles::original_structs::global_State, registryfree) },
		}},
		{ xor ("userdata"), {
			{ xor ("tag"), offsetof(roblox::shuffles::original_structs::Udata, tag) },
			{ xor ("len"), offsetof(roblox::shuffles::original_structs::Udata, len) },
			{ xor ("metatable"), offsetof(roblox::shuffles::original_structs::Udata, metatable) },
			{ xor ("data"), offsetof(roblox::shuffles::original_structs::Udata, data) },
		}},
		{ xor ("thread"), {
			{ xor ("status"), offsetof(roblox::shuffles::original_structs::lua_State, status) },
			{ xor ("activememcat"), offsetof(roblox::shuffles::original_structs::lua_State, activememcat) },
			{ xor ("stackstate"), offsetof(roblox::shuffles::original_structs::lua_State, stackstate) },
			{ xor ("singlestep"), offsetof(roblox::shuffles::original_structs::lua_State, singlestep) },

			{ xor ("top"), deob_ls.top },
			{ xor ("base"), deob_ls.base },
			{ xor ("global"), deob_ls.global },
			{ xor ("ci"), deob_ls.ci },
			{ xor ("stack_last"), deob_ls.stack_last },
			{ xor ("stack"), deob_ls.stack },

			{ xor ("end_ci"), offsetof(roblox::shuffles::original_structs::lua_State, end_ci) },
			{ xor ("base_ci"), offsetof(roblox::shuffles::original_structs::lua_State, base_ci) },

			{ xor ("stacksize"), offsetof(roblox::shuffles::original_structs::lua_State, stacksize) },
			{ xor ("size_ci"), offsetof(roblox::shuffles::original_structs::lua_State, size_ci) },

			{ xor ("nCcalls"), offsetof(roblox::shuffles::original_structs::lua_State, nCcalls) },
			{ xor ("baseCcalls"), offsetof(roblox::shuffles::original_structs::lua_State, baseCcalls) },

			{ xor ("cachedslot"), offsetof(roblox::shuffles::original_structs::lua_State, cachedslot) },

			{ xor ("gt"), deob_ls.gt },
			{ xor ("openupval"), deob_ls.openupval },
			{ xor ("gclist"), deob_ls.gclist },

			{ xor ("namecall"), offsetof(roblox::shuffles::original_structs::lua_State, namecall) },
		}},
		{ xor ("closure"), {
			{ xor ("isC"), offsetof(roblox::shuffles::original_structs::Closure, isC) },
			{ xor ("nupvalues"), offsetof(roblox::shuffles::original_structs::Closure, nupvalues) },
			{ xor ("stacksize"), offsetof(roblox::shuffles::original_structs::Closure, stacksize) },
			{ xor ("preload"), offsetof(roblox::shuffles::original_structs::Closure, preload) },
			{ xor ("gclist"), offsetof(roblox::shuffles::original_structs::Closure, gclist) },
			{ xor ("env"), offsetof(roblox::shuffles::original_structs::Closure, env) },

			{ xor ("f"), offsetof(roblox::shuffles::original_structs::Closure, c.f) },
			{ xor ("cont"), offsetof(roblox::shuffles::original_structs::Closure, c.cont) },
			{ xor ("debugname"), offsetof(roblox::shuffles::original_structs::Closure, c.debugname) },
			{ xor ("upvals"), offsetof(roblox::shuffles::original_structs::Closure, c.upvals) },

			{ xor ("p"), offsetof(roblox::shuffles::original_structs::Closure, l.p) },
			{ xor ("uprefs"), offsetof(roblox::shuffles::original_structs::Closure, l.uprefs) },
		}},
		{ xor ("gc::header"), {
			{ xor ("type"), deob_gch.type },
			{ xor ("marked"), deob_gch.marked },
			{ xor ("memory_category"), deob_gch.modkey }
		}},
		{ xor ("call_info"), {
			{ xor ("base"), deob_ci.base },
			{ xor ("func"), deob_ci.func },
			{ xor ("top"), deob_ci.top },
			{ xor ("savedpc"), deob_ci.savedpc },

			{ xor ("nresults"), deob_ci.nresults },
			{ xor ("flags"), deob_ci.flags },
		}},
		{ xor ("proto"), {
			{ xor ("k"), deob_proto.k },
			{ xor ("code"), deob_proto.code },
			{ xor ("p"), deob_proto.p },
			{ xor ("lineinfo"), deob_proto.lineinfo },
			{ xor ("abslineinfo"), deob_proto.abslineinfo },
			{ xor ("locvars"), deob_proto.locvars },
			{ xor ("upvalues"), deob_proto.upvalues },
			{ xor ("source"), deob_proto.source },

			{ xor ("debugname"), deob_proto.debugname },
			{ xor ("debuginsn"), offsetof(roblox::shuffles::original_structs::Proto, debuginsn) },

			{ xor ("gclist"), offsetof(roblox::shuffles::original_structs::Proto, gclist) },

			{ xor ("sizecode"), deob_proto.sizecode },
			{ xor ("sizep"), deob_proto.sizep },
			{ xor ("sizelocvars"), deob_proto.sizelocvars },
			{ xor ("sizeupvalues"), deob_proto.sizeupvalues },
			{ xor ("sizek"), deob_proto.sizek },
			{ xor ("sizelineinfo"), deob_proto.sizelineinfo },
			{ xor ("linegaplog2"), deob_proto.linegaplog2 },
			{ xor ("linedefined"), deob_proto.linedefined },
		}},
		{ xor ("string_table"), {
			{ xor ("hash"), deob_strt.hash },
			{ xor ("size"), deob_strt.size },
			{ xor ("capacity"), deob_strt.capacity }
		}}
	};

	nlohmann::json dump{
		{ xor ("addresses"), functions },
		{ xor ("constants"), constants },
		{ xor ("offsets"), offsets },
		//{ xor ("obfuscations"), obfuscations },
	};

	std::cout << dump.dump(4) << '\n';

	return 1;
}

int main() {
	return epic();
}