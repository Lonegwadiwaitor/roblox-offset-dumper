#pragma once

namespace con {
	extern std::ostream& fg_magenta(std::ostream& os);
	extern std::ostream& fg_green(std::ostream& os);
	extern std::ostream& fg_none(std::ostream& os);
}

namespace smh {
	template<typename ...args_t>
	constexpr void print(args_t... args) {
		std::cout << "[" << con::fg_magenta << 'S' << con::fg_green << 'M' << con::fg_none << ']' << " : ";
		(std::cout << ... << args);
		std::cout << con::fg_none;
	}
	template<typename ...args_t>
	constexpr void println(args_t... args) {
		std::cout << "[" << con::fg_magenta << 'S' << con::fg_green << 'M' << con::fg_none << ']' << " : ";
		(std::cout << ... << args) << '\n';
		std::cout << con::fg_none;
	}
}