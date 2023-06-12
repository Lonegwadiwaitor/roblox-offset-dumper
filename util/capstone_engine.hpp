#pragma once

#include <capstone/capstone.h>
#include <capstone/x86.h>
#include <memory>

class cs_instruction: public cs_insn {
	// this isn't a constructed class, rather casted in the below struct.
	// ** DO NOT ** STORE ANYTHING BUT VIRTUAL METHODS IN HERE
	// DOING SO WILL BREAK THINGS
public:
	[[nodiscard]] cs_x86 get_detail() const { /* these instructions are only ever x86 */
		return this->detail->x86;
	}

	[[nodiscard]] bool is(const x86_insn& x86_insn) const {
		return this->id == x86_insn;
	}

	template<int Imm>
	[[nodiscard]] int64_t get_op_value() const {
		auto&& op = this->get_detail().operands[Imm];

		return op.type == X86_OP_IMM ? op.imm : op.mem.disp;
	}

	template<int Imm>
	[[nodiscard]] x86_op_type get_op_type() const {
		return this->get_detail().operands[Imm].type;
	}

	template<int Imm>
	[[nodiscard]] x86_reg get_op_reg() const {
		return this->get_detail().operands[Imm].reg;
	}

	template<int Op>
	[[nodiscard]] int64_t get_imm() const {
		return this->detail->x86.operands[Op].imm;
	}

	template<int Op>
	[[nodiscard]] cs_x86_op get_op() const {
		return this->detail->x86.operands[Op];
	}

	// very shit approach but it works
	template<x86_insn Insn>
	constexpr cs_instruction& get_next_mnemonic() {
		for (int i = 0; i < 1000000; i++) {
			cs_instruction inst;

			if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(reinterpret_cast<uintptr_t>(this) + (sizeof(cs_insn) * i)), &inst, sizeof cs_insn, nullptr))
				return inst;

			if (inst.id == Insn)
				return this[i];
		}
	}

	template<x86_insn Insn>
	constexpr cs_instruction& get_previous_mnemonic() {
		for (int i = 0; i < 1000000; i++) {
			cs_instruction inst;

			if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(reinterpret_cast<uintptr_t>(this) - (sizeof(cs_insn) * i)), &inst, sizeof cs_insn, nullptr))
				return inst;

			if (inst.id == Insn)
				return this[-i];
		}

		return this[0];
	}

	template<uint8_t Op, x86_reg Reg>
	constexpr cs_instruction& get_next_reg() {
		for (int i = 0; i < 1000000; i++) {
			cs_instruction inst;

			if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(reinterpret_cast<uintptr_t>(this) + (sizeof(cs_insn) * i)), &inst, sizeof cs_insn, nullptr))
				return inst;

			if (inst.detail->x86.operands[Op].reg == Reg)
				return this[i];
		}
	}

	template<uint8_t Op, x86_reg Reg>
	constexpr cs_instruction& get_previous_reg() {
		for (int i = 0; i < 1000000; i++) {
			cs_instruction inst;

			if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(reinterpret_cast<uintptr_t>(this) - (sizeof(cs_insn) * i)), &inst, sizeof cs_insn, nullptr))
				return inst;

			if (inst.detail->x86.operands[Op].reg == Reg)
				return this[i];
		}
	}
};

struct cs_instructions {
	cs_instruction* instructions;
	std::size_t count;
};

enum direction {
	next,
	previous
};

class capstone_engine {
	std::unique_ptr<csh> cs_handle;
public:
	static bool is_function(const std::uintptr_t& address);
	static std::vector<std::uint8_t> get_func_bytes(const std::uintptr_t& address);

	template<direction Dir>
	static std::uintptr_t get_prologue(const std::uintptr_t& address) {
		auto prologue = address;

		if constexpr (Dir == next) {
			prologue++;
			while (!is_function(prologue)) {
				prologue++;
			}
		}
		else if constexpr (Dir == previous) {
			prologue--;
			while (!is_function(prologue)) {
				prologue--;
			}
		}

		return prologue;
	}
public:
	capstone_engine(): cs_handle(std::make_unique<csh>()) {
		cs_open(CS_ARCH_X86, CS_MODE_32, cs_handle.get());
		cs_option(*cs_handle, CS_OPT_DETAIL, 1);
	}

	[[nodiscard]] cs_instructions disassemble(std::uintptr_t start = 0, std::uintptr_t end = 0) const {
		if (!start && !end)
			return {};

		if (start && end) {
			cs_insn* inst;

			const auto func_sz = end - start;

			char* buffer = new char[func_sz];

			ReadProcessMemory(GetCurrentProcess(), (LPCVOID)start, buffer, func_sz, nullptr);

			const size_t count = cs_disasm(*cs_handle, (std::uint8_t*)buffer, func_sz - 1, start, 0, &inst);

			return { (cs_instruction*)inst, count };


			// disassemble til provided range is met

		}
		// use is_function to determine size
		cs_insn* inst;

		if (!is_function(start))
			start = get_prologue<previous>(start);

		const auto& function_bytes = get_func_bytes(start);

		const size_t count = cs_disasm(*cs_handle, function_bytes.data(), function_bytes.size() - 1, start, 0, &inst);

		return { (cs_instruction*)inst, count };
	}
};