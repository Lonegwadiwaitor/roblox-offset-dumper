#pragma once

namespace roblox::obfuscations {
	enum obf_obfuscation {
		ADD,
		SUB,
		XOR
	};

	enum obf_direction {
		LEFT,
		RIGHT
	};

	struct obfuscation_t {
		obf_obfuscation obf_op;
		obf_direction obf_dir;
		obf_obfuscation deobf_op;
		obf_direction deobf_dir;

		[[nodiscard]] std::string parse_expression() const {
			auto get_symbol = [](const obf_obfuscation obf) {
				std::string str;

				switch (obf) {
				case ADD:
					str = "+";
					break;
				case SUB:
					str = "-";
					break;
				case XOR:
					str = "^";
					break;
				}

				return str;
			};

			auto get_expr = [](const std::string& symbol, const obf_direction dir) {
				return std::format("({} {} {})", dir == LEFT ? "*reinterpret_cast<std::uintptr_t*>(address)" : "address", symbol, dir == RIGHT ? "*reinterpret_cast<std::uintptr_t*>(address)" : "address");
			};

			const auto deobf_symbol = get_symbol(deobf_op);
			const auto obf_symbol = get_symbol(obf_op);

			return std::format("deob: {}, obf: {}", get_expr(deobf_symbol, deobf_dir), get_expr(obf_symbol, obf_dir));
		}
	};
}