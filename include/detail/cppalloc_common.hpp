#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <new>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <variant>
#include <vector>

#include "cppalloc_traits.hpp"
#include "type_name.hpp"

#ifdef _MSC_VER
#include <malloc.h>
#endif

#if defined(_MSC_VER)
#define CPPALLOC_EXPORT      __declspec(dllexport)
#define CPPALLOC_IMPORT      __declspec(dllimport)
#define CPPALLOC_EMPTY_BASES __declspec(empty_bases)
#else
#define CPPALLOC_EXPORT __attribute__((visibility("default")))
#define CPPALLOC_IMPORT __attribute__((visibility("default")))
#define CPPALLOC_EMPTY_BASES
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

#ifndef CPPALLOC_PRINT_DEBUG
#define CPPALLOC_PRINT_DEBUG(info) cppalloc::detail::print_debug_info(info)
#endif

#define CPPALLOC_EXTERN extern "C"

namespace cppalloc
{
constexpr std::uint32_t safety_offset = alignof(void*);

using uhandle = std::uint32_t;
using ihandle = std::uint32_t;

namespace detail
{

template <typename size_type>
constexpr size_type     k_null_sz  = std::numeric_limits<size_type>::max();
constexpr std::uint32_t k_null_32  = std::numeric_limits<std::uint32_t>::max();
constexpr std::int32_t  k_null_i32 = std::numeric_limits<std::int32_t>::min();
constexpr std::uint64_t k_null_64  = std::numeric_limits<std::uint64_t>::max();
constexpr uhandle       k_null_uh  = std::numeric_limits<uhandle>::max();

enum ordering_by : std::uint32_t
{
  e_size,
  e_offset,
  k_count
};

inline void print_debug_info(std::string const& s)
{
  std::cout << s;
}

template <typename tag, bool k_compute_stats = false, typename base_t = std::monostate, bool k_print = true>
struct statistics : public base_t
{
  template <typename... Args>
  statistics(Args&&... i_args) : base_t(std::forward<Args>(i_args)...)
  {
  }

  void                   print() {}
  static std::false_type report_new_arena(std::uint32_t count = 1)
  {
    return std::false_type{};
  }
  static std::false_type report_allocate(std::size_t size)
  {
    return std::false_type{};
  }
  static std::false_type report_deallocate(std::size_t size)
  {
    return std::false_type{};
  }

  std::uint32_t get_arenas_allocated() const
  {
    return 0;
  }
};

#ifndef CPPALLOC_NO_STATS

struct timer_t
{
  struct scoped
  {
    scoped(timer_t& t) : timer(&t)
    {
      start = std::chrono::high_resolution_clock::now();
    }
    scoped(scoped const&) = delete;
    scoped(scoped&& other) : timer(other.timer), start(other.start)
    {
      other.timer = nullptr;
    }
    scoped& operator=(scoped const&) = delete;
    scoped& operator                 =(scoped&& other)
    {
      timer       = other.timer;
      start       = other.start;
      other.timer = nullptr;
      return *this;
    }

    ~scoped()
    {
      if (timer)
      {
        auto end = std::chrono::high_resolution_clock::now();
        timer->elapsed_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
      }
    }

    timer_t*                                       timer = nullptr;
    std::chrono::high_resolution_clock::time_point start;
  };

  std::uint64_t elapsed_time_count()
  {
    return elapsed_time.load();
  }

  std::atomic_uint64_t elapsed_time = 0;
};

template <typename tag_arg, typename base_arg, bool k_print>
struct statistics<tag_arg, true, base_arg, k_print> : public base_arg
{
  std::atomic_uint32_t arenas_allocated   = 0;
  std::atomic_uint64_t peak_allocation    = 0;
  std::atomic_uint64_t allocation         = 0;
  std::atomic_uint64_t deallocation_count = 0;
  std::atomic_uint64_t allocation_count   = 0;
  std::atomic_uint64_t allocator_data     = 0;
  timer_t              allocation_timing;
  timer_t              deallocation_timing;
  bool                 stats_printed = false;

  template <typename... Args>
  statistics(Args&&... i_args) : base_arg(std::forward<Args>(i_args)...)
  {
  }
  ~statistics()
  {
    print();
  }

  void print()
  {
    if (k_print && !stats_printed)
    {
      std::stringstream ss;
      ss << "Stats for: " << cppalloc::type_name<tag_arg>() << "\n"
         << "Arenas allocated: " << arenas_allocated.load() << "\n"
         << "Peak allocation: " << peak_allocation.load() << "\n"
         << "Final allocation: " << allocation.load() << "\n"
         << "Total allocation call: " << allocation_count.load() << "\n"
         << "Total deallocation call: " << deallocation_count.load() << "\n"
         << "Total allocation time: " << allocation_timing.elapsed_time_count() << " us \n"
         << "Total deallocation time: " << deallocation_timing.elapsed_time_count() << " us\n";
      if (allocation_count > 0)
        ss << "Avg allocation time: " << allocation_timing.elapsed_time_count() / allocation_count.load() << " us\n";
      if (deallocation_count > 0)
        ss << "Avg deallocation time: " << deallocation_timing.elapsed_time_count() / deallocation_count.load()
           << " us\n";

      CPPALLOC_PRINT_DEBUG(ss.str());
      stats_printed = true;
    }
  }

  void report_new_arena(std::uint32_t count = 1)
  {
    arenas_allocated += count;
  }

  [[nodiscard]] timer_t::scoped report_allocate(std::size_t size)
  {
    allocation_count++;
    allocation += size;
    peak_allocation = std::max<std::size_t>(allocation.load(), peak_allocation.load());
    return timer_t::scoped(allocation_timing);
  }
  [[nodiscard]] timer_t::scoped report_deallocate(std::size_t size)
  {
    deallocation_count++;
    allocation -= size;
    return timer_t::scoped(deallocation_timing);
  }

  std::uint32_t get_arenas_allocated() const
  {
    return arenas_allocated.load();
  }
};

#endif
} // namespace detail

template <typename address_type, typename allocator, typename size_type, typename... Args>
address_type allocate(allocator& i_alloc, size_type i_size, Args&&... i_args)
{
  return reinterpret_cast<address_type>(i_alloc.allocate(i_size, std::forward<Args>(i_args)...));
}
} // namespace cppalloc