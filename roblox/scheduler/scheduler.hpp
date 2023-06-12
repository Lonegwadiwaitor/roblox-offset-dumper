#pragma once
#include "singleton.hpp"

namespace roblox::scheduler {
	struct script_context {
		std::uint8_t _[308];
		std::uintptr_t lua_state;
	};

	class task_scheduler {
	private:
		std::uint8_t _[308];
	public:
		class job {
		private:
			void* vtable;
			std::uint8_t _[12];
		public:
			std::string name;
			void hook();
		};

		class waiting_script_job : public job {
		private:
			std::uint8_t __[264];
		public:
			script_context* script_context;
		};

		using iterator = std::vector<std::shared_ptr<job>>::iterator;
		using reverse_iterator = std::reverse_iterator<iterator>;
	private:
		iterator _begin, _end;
	public:
		task_scheduler() = delete;
		task_scheduler(const task_scheduler&) = delete;
		task_scheduler(const task_scheduler&&) = delete;

		iterator begin() const;
		iterator end() const;
		reverse_iterator rbegin() const;
		reverse_iterator rend() const;

		void set_fps_cap(const double& fps);
		void hook_job();
		uintptr_t get_script_context();
		static void push(const std::function<void()>&);
	};
}
