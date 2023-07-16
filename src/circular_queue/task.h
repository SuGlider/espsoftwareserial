#pragma once
/*
task.h - Implementation of a C++20 async coroutines task.
Copyright (c) 2023 Dirk O. Kaar. All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <atomic>
#include <memory>

namespace ghostl
{
	template<class T = void>
	struct task
	{
		struct promise_type
		{
			auto get_return_object()
			{
				return task(std::coroutine_handle<promise_type>::from_promise(*this));
			}
			std::suspend_always initial_suspend() { return {}; }
			struct final_awaiter
			{
				bool await_ready() noexcept { return false; }
				void await_resume() noexcept {}
				std::coroutine_handle<>
					await_suspend(std::coroutine_handle<promise_type> h) noexcept
				{
					// final_awaiter::await_suspend is called when the execution of the
					// current coroutine (referred to by 'h') is about to finish.
					// If the current coroutine was resumed by another coroutine via
					// co_await get_task(), a handle to that coroutine has been stored
					// as h.promise().previous. In that case, return the handle to resume
					// the previous coroutine.
					// Otherwise, return noop_coroutine(), whose resumption does nothing.

					if (auto previous = h.promise().previous; previous)
						return previous;
					else
						return std::noop_coroutine();
				}
			};
			final_awaiter final_suspend() noexcept { return {}; }
			void unhandled_exception() { throw; }
			void return_value(T value) { result = std::move(value); }

			T result;
			std::coroutine_handle<> previous;
		};

		task() noexcept : coroutine(nullptr) {}
		explicit task(std::coroutine_handle<promise_type> h) : coroutine(h) {}
		task(const task&) = delete;
		task(task&& other) noexcept : coroutine(std::exchange(other.coroutine, nullptr)) {};
		~task() { if (coroutine) coroutine.destroy(); }
		task& operator=(const task&) = delete;
		task& operator=(task&& other) noexcept
		{
			if (std::addressof(other) != this)
			{
				if (coroutine) coroutine.destroy();
				coroutine = std::exchange(other.coroutine, nullptr);
			}
			return *this;
		}

		struct awaiter
		{
			bool await_ready() { return false; }
			T await_resume() { return std::move(coroutine.promise().result); }
			auto await_suspend(std::coroutine_handle<> h)
			{
				coroutine.promise().previous = h;
				return coroutine;
			}
			std::coroutine_handle<promise_type> coroutine;
		};
		awaiter operator co_await() { return awaiter{ coroutine }; }
		T resume()
		{
			coroutine.resume();
			return std::move(coroutine.promise().result);
		}
		T operator()()
		{
			return resume();
		}

	private:
		std::coroutine_handle<promise_type> coroutine;
	};

	template<>
	struct task<void>
	{
		struct promise_type
		{
			auto get_return_object()
			{
				return task(std::coroutine_handle<promise_type>::from_promise(*this));
			}
			std::suspend_always initial_suspend() { return {}; }
			struct final_awaiter
			{
				bool await_ready() noexcept { return false; }
				void await_resume() noexcept {}
				std::coroutine_handle<>
					await_suspend(std::coroutine_handle<promise_type> h) noexcept
				{
					// final_awaiter::await_suspend is called when the execution of the
					// current coroutine (referred to by 'h') is about to finish.
					// If the current coroutine was resumed by another coroutine via
					// co_await get_task(), a handle to that coroutine has been stored
					// as h.promise().previous. In that case, return the handle to resume
					// the previous coroutine.
					// Otherwise, return noop_coroutine(), whose resumption does nothing.

					if (auto previous = h.promise().previous; previous)
						return previous;
					else
						return std::noop_coroutine();
				}
			};
			final_awaiter final_suspend() noexcept { return {}; }
			void unhandled_exception() { throw; }
			void return_void() { }

			std::coroutine_handle<> previous;
		};

		task() noexcept : coroutine(nullptr) {}
		explicit task(std::coroutine_handle<promise_type> h) : coroutine(h) {}
		task(const task&) = delete;
		task(task&& other) noexcept : coroutine(std::exchange(other.coroutine, nullptr)) {};
		~task() { if (coroutine) coroutine.destroy(); }
		task& operator=(const task&) = delete;
		task& operator=(task&& other) noexcept
		{
			if (std::addressof(other) != this)
			{
				if (coroutine) coroutine.destroy();
				coroutine = std::exchange(other.coroutine, nullptr);
			}
			return *this;
		}

		struct awaiter
		{
			bool await_ready() { return false; }
			constexpr void await_resume() const noexcept {}
			auto await_suspend(std::coroutine_handle<> h)
			{
				coroutine.promise().previous = h;
				return coroutine;
			}
			std::coroutine_handle<promise_type> coroutine;
		};
		awaiter operator co_await() { return awaiter{ coroutine }; }
		void resume()
		{
			coroutine.resume();
		}
		void operator()()
		{
			resume();
		}

	private:
		std::coroutine_handle<promise_type> coroutine;
	};
} // namespace ghostl
