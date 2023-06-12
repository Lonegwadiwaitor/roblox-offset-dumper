#pragma once
#include <cstdint>
#include <random>

#include "../memory/rebase.hpp"

// MSVC sucks

	extern std::uintptr_t spoof_address;

	class spoof {
	public:
		enum register_type {
			eax,
			ebx,
			ecx,
			edx,
			esi,
			edi,

			ebp,
			esp,


			stack
		};
	private:
		template <register_type type>
		static constexpr __forceinline auto push_arg_1(auto arg) {
			if constexpr (type == eax) {
				__asm mov al, arg
			}
			if constexpr (type == ebx) {
				__asm mov bl, arg
			}
			if constexpr (type == ecx) {
				__asm mov cl, arg
			}
			if constexpr (type == edx) {
				__asm mov dl, arg
			}
			if constexpr (type == esi) {
				__asm mov sil, arg
			}
			if constexpr (type == edi) {
				__asm mov dil, arg
			}

			if constexpr (type == ebp) {
				__asm mov bpl, arg
			}

			if constexpr (type == esp) {
				__asm mov spl, arg
			}


			if constexpr (type == stack) {
				__asm push arg
			}
		}

		template <register_type type>
		static constexpr __forceinline auto push_arg_2(auto arg) {
			if constexpr (type == eax) {
				__asm mov ax, arg
			}
			if constexpr (type == ebx) {
				__asm mov bx, arg
			}
			if constexpr (type == ecx) {
				__asm mov cx, arg
			}
			if constexpr (type == edx) {
				__asm mov dx, arg
			}
			if constexpr (type == esi) {
				__asm mov si, arg
			}
			if constexpr (type == edi) {
				__asm mov di, arg
			}

			if constexpr (type == ebp) {
				__asm mov bp, arg
			}

			if constexpr (type == esp) {
				__asm mov sp, arg
			}

			if constexpr (type == stack) {
				__asm push arg
			}
		}

		template <register_type type>
		static constexpr __forceinline auto push_arg_4(auto arg) {
			if constexpr (type == eax) {
				__asm mov eax, arg
			}
			if constexpr (type == ebx) {
				__asm mov ebx, arg
			}
			if constexpr (type == ecx) {
				__asm mov ecx, arg
			}
			if constexpr (type == edx) {
				__asm mov edx, arg
			}
			if constexpr (type == esi) {
				__asm mov esi, arg
			}
			if constexpr (type == edi) {
				__asm mov edi, arg
			}

			if constexpr (type == ebp) {
				__asm mov ebp, arg
			}

			if constexpr (type == esp) {
				__asm mov esp, arg
			}


			if constexpr (type == stack) {
				__asm push arg
			}
		}


		template <register_type type>
		static constexpr auto __forceinline push_arg(auto arg) {
			// edge cases first
			// classes/structs are passed as pointers and sizeof will reflect class size not ptr size, so lets fix that!

			if constexpr(std::is_class_v<decltype(arg)> && !std::is_pointer_v<decltype(arg)>) {
				push_arg_4<type>(&arg);
				return;
			}

			if constexpr(sizeof arg == 4) {
				push_arg_4<type>(arg);
				return;
			}

			if constexpr (sizeof arg == 2) {
				push_arg_2<type>(arg);
				return;
			}

			if constexpr (sizeof arg == 1) {
				push_arg_2<type>(arg);
				return;
			}
		}

		static constexpr __forceinline int get_size_of_arg(auto a1) {
			if constexpr (std::is_class_v<decltype(a1)> && !std::is_pointer_v<decltype(a1)>) {
				return 4;
			}

			return sizeof a1;
		}
	public:
		template<typename ret>
		static constexpr FORCEINLINE ret call(uintptr_t func) {
			__asm {
				push edi

				mov edi, finished
				push spoof_address
				jmp func

				finished :
				pop edi
			}
		}

		template<typename ret>
		static constexpr FORCEINLINE ret fastcall(uintptr_t func, auto a1) {
			__asm {
				push edi
				push ecx

				mov edi, finished
			}

			push_arg<ecx>(a1);

			__asm {
				push spoof_address
				jmp func

				finished :
				//pop ecx
				add esp, 4
				pop edi
			}
		}
		template<typename ret>
		static constexpr FORCEINLINE ret fastcall(uintptr_t func, auto a1, auto a2) {
			__asm {
				push edi // alloc stack space
				push ecx
				push edx

				mov edi, finished
			}

			push_arg<ecx>(a1);
			push_arg<edx>(a2);

			/* mov ecx, a1
			   mov edx, a2 */

			__asm {
				
				push spoof_address
				jmp func

				finished :
				pop edx // clear stack space
				pop ecx
				pop edi
			}
		}
		template<typename ret>
		static constexpr FORCEINLINE ret fastcall(uintptr_t func, auto a1, auto a2, auto a3) {
			__asm {
				push edi
				push ecx
				push edx

				mov edi, finished
			}

			/* mov ecx, a1
			   mov edx, a2
			   push a3*/

			push_arg<ecx>(a1);
			push_arg<edx>(a2);
			push_arg<stack>(a3);

			__asm {
				push spoof_address
				jmp func

				finished :
				pop edx
				pop ecx
				pop edi

				add esp, 4
			}
		}

		template<typename ret>
		static constexpr FORCEINLINE ret fastcall(uintptr_t func, auto a1, auto a2, auto a3, auto a4) {
			__asm {
				push edi
				push ecx
				push edx

				mov edi, finished
			}

			/* mov ecx, a1
			   mov edx, a2
			   push a4
			   push a3*/

			push_arg<ecx>(a1);
			push_arg<edx>(a2);
			push_arg<stack>(a4);
			push_arg<stack>(a3);

			__asm {
				
				push spoof_address
				jmp func

				finished :
				pop edx
				pop ecx
				pop edi

				add esp, 8
			}
		}

		template<typename ret>
		static constexpr FORCEINLINE ret fastcall(uintptr_t func, auto a1, auto a2, auto a3, auto a4, auto a5) {
			__asm {
				push edi // save all used registers
				push ecx
				push edx

				mov edi, finished // push args

			}
				/*mov ecx, a1
				mov edx, a2
				push a5
				push a4
				push a3*/

			push_arg<ecx>(a1);
			push_arg<edx>(a2);
			push_arg<stack>(a5);
			push_arg<stack>(a4);
			push_arg<stack>(a3);

			__asm {
				push spoof_address // push return
				jmp func // based

				finished :
				pop edx // restore previous register values from stack
				pop ecx
				pop edi

				add esp, 12

				// might need to pop more after this, *cough* deserializer
			}
		}


		template<typename ret>
		static constexpr FORCEINLINE ret fastcall(uintptr_t func, auto a1, auto a2, auto a3, auto a4, auto a5, auto a6) {
			__asm {
				push edi
				push ecx
				push edx

				mov edi, finished
			}

				/*mov ecx, a1
				mov edx, a2
				push a6
				push a5
				push a4
				push a3*/

			push_arg<ecx>(a1);
			push_arg<edx>(a2);
			push_arg<stack>(a6);
			push_arg<stack>(a5);
			push_arg<stack>(a4);
			push_arg<stack>(a3);

			__asm {
				push spoof_address
				jmp func

				finished :
				pop edx
				pop ecx
				pop edi

				add esp, 16
			}
		}
	};


/*
namespace spoof {
	extern std::uintptr_t address;
}
*/
