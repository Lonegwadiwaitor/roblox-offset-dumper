#include "../../pch/pch.h"

#include "scheduler.hpp"

#include <ranges>

#include "../../memory/rebase.hpp"

#include "../../util/offset_cache.hpp"

using task_caller_t = char(*)();

class state {
public:
	std::vector<std::function<void()>> call_stack;
	task_caller_t original_task_caller;
};

static char task_caller() {
	auto& state = singleton<::state>::get_singleton();
	auto& call_stack = state.call_stack;

	if (!call_stack.empty()) {
		for (const auto& function : call_stack) {
			function();
		}

		call_stack.clear();
	}

	return state.original_task_caller();
}

void roblox::scheduler::task_scheduler::job::hook() {
	if (auto& state = singleton<::state>::get_singleton(); !state.original_task_caller) {
		auto new_vtable = new std::uintptr_t[52];
		DWORD old_protection{};

		std::memcpy(new_vtable, this->vtable, 52);
		state.original_task_caller = reinterpret_cast<task_caller_t>(new_vtable[1]);

		new_vtable[1] = reinterpret_cast<std::uintptr_t>(task_caller);

		VirtualProtect(this, sizeof(std::uintptr_t), PAGE_READWRITE, &old_protection);
		this->vtable = new_vtable;
		VirtualProtect(this, sizeof(std::uintptr_t), old_protection, &old_protection);
	}
}

roblox::scheduler::task_scheduler::iterator roblox::scheduler::task_scheduler::begin() const {
	return _begin;
}

roblox::scheduler::task_scheduler::iterator roblox::scheduler::task_scheduler::end() const {
	return _end;
}

roblox::scheduler::task_scheduler::reverse_iterator roblox::scheduler::task_scheduler::rbegin() const {
	return std::reverse_iterator(_end);
}

roblox::scheduler::task_scheduler::reverse_iterator roblox::scheduler::task_scheduler::rend() const {
	return std::reverse_iterator(_begin);
}

void roblox::scheduler::task_scheduler::set_fps_cap(const double& fps) {
	std::array<std::uint8_t, 0x100> buffer{};
	const auto& search_offset = 0x100;
	std::uint16_t delay_offset{};

	memcpy(buffer.data(), reinterpret_cast<std::uint8_t*>(reinterpret_cast<uintptr_t>(this) + search_offset), buffer.size());

	for (std::size_t i = 0; i < buffer.size() - sizeof(double); i += 4) {
		constexpr double frame_delay = 1.0 / 60.0;
		auto difference = *reinterpret_cast<double*>(buffer.data() + i) - frame_delay;

		difference = difference < 0 ? -difference : difference;

		if (difference < std::numeric_limits<double>::epsilon()) {
			delay_offset = search_offset + i;
		}
	}

	*reinterpret_cast<double*>(reinterpret_cast<uintptr_t>(this) + delay_offset) = 1.0 / fps;
	smh::println((xor("Set FPS to "), std::to_string((int)(1.0 / *reinterpret_cast<double*>(reinterpret_cast<uintptr_t>(this) + delay_offset)))));
}

void roblox::scheduler::task_scheduler::hook_job() {
	if (const auto& [_, original_task_caller] = singleton<state>::get_singleton(); !original_task_caller) {
		smh::println(xor("hooking script job"));
		for (const auto& job : std::ranges::reverse_view(*this) /* last scripts job is the game job */) {
			if (job->name == xor("WaitingHybridScriptsJob")) {
				smh::println(xor("script job hooked"));
				return job->hook();
			}
		}
	}
}

uintptr_t roblox::scheduler::task_scheduler::get_script_context() {
	if (const auto& [_, original_task_caller] = singleton<state>::get_singleton(); original_task_caller) {
		for (const auto& job : std::ranges::reverse_view(*this)) {
			if (job->name == xor("WaitingHybridScriptsJob")) {
				return *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(job.get()) + 304);
			}
		}
	}
	return 0;
}

void roblox::scheduler::task_scheduler::push(const std::function<void()>& callback) {
	auto& state = singleton<::state>::get_singleton();
	state.call_stack.push_back(callback);
}
