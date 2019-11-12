#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <new>
#include <random>
#include <type_traits>
#include <typeinfo>
#include <variant>
#include <vector>

#include "type_name.hpp"

#ifdef _MSC_VER
#include <malloc.h>
#endif
namespace cppalloc {
namespace detail {

struct timer_t {

	struct scoped {
		scoped(timer_t& t) : timer(&t) {
			start = std::chrono::high_resolution_clock::now();
		}
		scoped(scoped const&) = delete;
		scoped(scoped&& other) : timer(other.timer), start(other.start) {
			other.timer = nullptr;
		}
		scoped& operator=(scoped const&) = delete;
		scoped& operator                 =(scoped&& other) {
      timer       = other.timer;
      start       = other.start;
      other.timer = nullptr;
      return *this;
		}

		~scoped() {
			if (timer) {
				auto end = std::chrono::high_resolution_clock::now();
				timer->elapsed_time =
				    std::chrono::duration_cast<std::chrono::microseconds>(end - start);
			}
		}

		timer_t*                                       timer = nullptr;
		std::chrono::high_resolution_clock::time_point start;
	};

	std::chrono::microseconds elapsed_time = std::chrono::seconds(0);
};

template <typename tag, bool                     k_compute_stats = true,
          typename base_t = std::monostate, bool k_print = true>
struct statistics : public base_t {
	std::uint32_t arenas_allocated   = 0;
	std::size_t   peak_allocation    = 0;
	std::size_t   allocation         = 0;
	std::size_t   deallocation_count = 0;
	std::size_t   allocation_count   = 0;
	timer_t       allocation_timing;
	timer_t       deallocation_timing;

	template <typename... Args>
	statistics(Args&&... i_args) : base_t(std::forward<Args>(i_args)...) {}
	~statistics() {

		if (k_print) {
			std::cout << "\n[INFO] Stats for: " << cppalloc::type_name<tag>() << std::endl;
			std::cout << "[INFO] Arenas allocated: " << arenas_allocated << std::endl;
			std::cout << "[INFO] Peak allocation: " << peak_allocation << std::endl;
			std::cout << "[INFO] Final allocation: " << allocation << std::endl;
			std::cout << "[INFO] Total allocation call: " << allocation_count
			          << std::endl;
			std::cout << "[INFO] Total deallocation call: " << deallocation_count
			          << std::endl;
			std::cout << "[INFO] Total allocation time: "
			          << allocation_timing.elapsed_time.count() << " us" << std::endl;
			std::cout << "[INFO] Total deallocation time: "
			          << deallocation_timing.elapsed_time.count() << " us"
			          << std::endl;
			if (allocation_count > 0)
				std::cout << "[INFO] Avg allocation time: "
				          << allocation_timing.elapsed_time.count() / allocation_count
				          << " us" << std::endl;
			if (deallocation_count > 0)
				std::cout << "[INFO] Avg deallocation time: "
				          << deallocation_timing.elapsed_time.count() /
				                 deallocation_count
				          << " us" << std::endl;
		}
	}

	void report_new_arena(std::uint32_t count = 1) { arenas_allocated += count; }

	[[nodiscard]] timer_t::scoped report_allocate(std::size_t size) {
		allocation_count++;
		allocation += size;
		peak_allocation = std::max<std::size_t>(allocation, peak_allocation);
		return timer_t::scoped(allocation_timing);
	}[[nodiscard]] timer_t::scoped report_deallocate(std::size_t size) {
		deallocation_count++;
		allocation -= size;
		return timer_t::scoped(deallocation_timing);
	}

	std::uint32_t get_arenas_allocated() const { return arenas_allocated; }
};

template <typename tag, typename base_t, bool k_print>
struct statistics<tag, false, base_t, k_print> : public base_t {

	template <typename... Args>
	statistics(Args&&... i_args) : base_t(std::forward<Args>(i_args)...) {}

	std::false_type report_new_arena(std::uint32_t count = 1) {
		return std::false_type{};
	}
	std::false_type report_allocate(std::size_t size) {
		return std::false_type{};
	}
	std::false_type report_deallocate(std::size_t size) {
		return std::false_type{};
	}

	std::uint32_t get_arenas_allocated() const { return 0; }
};

} // namespace detail

template <typename address_type, typename allocator, typename size_type,
          typename... Args>
address_type allocate(allocator& i_alloc, size_type i_size, Args&&... i_args) {
	return reinterpret_cast<address_type>(
	    i_alloc.allocate(i_size, std::forward<Args>(i_args)...));
}

} // namespace cppalloc