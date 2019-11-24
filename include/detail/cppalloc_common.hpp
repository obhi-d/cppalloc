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
#include <atomic>

#include "type_name.hpp"

#ifdef _MSC_VER
#include <malloc.h>
#endif

#if defined(_MSC_VER)
#define CPPALLOC_EXPORT __declspec(dllexport)
#define CPPALLOC_IMPORT __declspec(dllimport)
#else
#define CPPALLOC_EXPORT __attribute__((visibility("default")))
#define CPPALLOC_IMPORT __attribute__((visibility("default")))
#endif

#ifdef CPPALLOC_DLL_IMPL
#ifdef CPPALLOC_EXPORT_SYMBOLS
#define CPPALLOC_API CPPALLOC_EXPORT
#else
#define CPPALLOC_API CPPALLOC_IMPORT
#endif
#else
#define CPPALLOC_API
#endif

#define CPPALLOC_EXTERN extern

namespace cppalloc {
namespace traits {

template <typename underlying_allocator_tag> constexpr bool is_static_v = false;
template <typename underlying_allocator_tag>
constexpr bool is_memory_allocator_v = true;

template <typename ua_t, class = std::void_t<>> struct tag {
	using type = void;
};
template <typename ua_t> struct tag<ua_t, std::void_t<typename ua_t::tag>> {
	using type = typename ua_t::tag;
};

template <typename ua_t> using tag_t = typename tag<ua_t>::type;

} // namespace traits
namespace detail {

template <typename tag, bool                     k_compute_stats = false,
          typename base_t = std::monostate, bool k_print = true>
struct statistics : public base_t {

	template <typename... Args>
	statistics(Args&&... i_args) : base_t(std::forward<Args>(i_args)...) {}

	static std::false_type report_new_arena(std::uint32_t count = 1) {
		return std::false_type{};
	}
	static std::false_type report_allocate(std::size_t size) {
		return std::false_type{};
	}
	static std::false_type report_deallocate(std::size_t size) {
		return std::false_type{};
	}

	std::uint32_t get_arenas_allocated() const { return 0; }
};

#ifndef CPPALLOC_NO_STATS

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
				timer->elapsed_time +=
				    std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
			}
		}

		timer_t*                                       timer = nullptr;
		std::chrono::high_resolution_clock::time_point start;
	};

	std::uint64_t elapsed_time_count() { return elapsed_time.load(); }

	std::atomic_uint64_t elapsed_time = 0;
};

template <typename tag, typename base_t, bool k_print>
struct statistics<tag, true, base_t, k_print> : public base_t {
	
	std::atomic_uint32_t arenas_allocated   = 0;
	std::atomic_uint64_t peak_allocation    = 0;
	std::atomic_uint64_t allocation         = 0;
	std::atomic_uint64_t deallocation_count = 0;
	std::atomic_uint64_t allocation_count   = 0;
	timer_t       allocation_timing;
	timer_t       deallocation_timing;

	template <typename... Args>
	statistics(Args&&... i_args) : base_t(std::forward<Args>(i_args)...) {}
	~statistics() {

		if (k_print) {
			std::cout << "\n[INFO] Stats for: " << cppalloc::type_name<tag>()
			          << std::endl;
			std::cout << "[INFO] Arenas allocated: " << arenas_allocated.load() << std::endl;
			std::cout << "[INFO] Peak allocation: " << peak_allocation.load()
			          << std::endl;
			std::cout << "[INFO] Final allocation: " << allocation.load()
			          << std::endl;
			std::cout << "[INFO] Total allocation call: " << allocation_count.load()
			          << std::endl;
			std::cout << "[INFO] Total deallocation call: "
			          << deallocation_count.load()
			          << std::endl;
			std::cout << "[INFO] Total allocation time: "
			          << allocation_timing.elapsed_time_count() << " us" << std::endl;
			std::cout << "[INFO] Total deallocation time: "
			          << deallocation_timing.elapsed_time_count() << " us"
			          << std::endl;
			if (allocation_count > 0)
				std::cout << "[INFO] Avg allocation time: "
				          << allocation_timing.elapsed_time_count() /
				                 allocation_count.load()
				          << " us" << std::endl;
			if (deallocation_count > 0)
				std::cout << "[INFO] Avg deallocation time: "
				          << deallocation_timing.elapsed_time_count() /
				                 deallocation_count.load()
				          << " us" << std::endl;
		}
	}

	void report_new_arena(std::uint32_t count = 1) { arenas_allocated += count; }

	[[nodiscard]] timer_t::scoped report_allocate(std::size_t size) {
		allocation_count++;
		allocation += size;
		peak_allocation = std::max<std::size_t>(allocation.load(), peak_allocation.load());
		return timer_t::scoped(allocation_timing);
	}
	[[nodiscard]] timer_t::scoped report_deallocate(std::size_t size) {
		deallocation_count++;
		allocation -= size;
		return timer_t::scoped(deallocation_timing);
	}

	std::uint32_t get_arenas_allocated() const { return arenas_allocated.load(); }
};

#endif

} // namespace detail

template <typename address_type, typename allocator, typename size_type,
          typename... Args>
address_type allocate(allocator& i_alloc, size_type i_size, Args&&... i_args) {
	return reinterpret_cast<address_type>(
	    i_alloc.allocate(i_size, std::forward<Args>(i_args)...));
}

} // namespace cppalloc