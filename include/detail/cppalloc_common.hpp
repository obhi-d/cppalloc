#pragma once

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <limits>
#include <cassert>
#include <algorithm>
#include <array>
#include <random>
#include <vector>
#include <chrono>
#include <new>

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
    scoped(scoped && other) : timer(other.timer), start(other.start) { other.timer = nullptr; }
    scoped& operator = (scoped const&) = delete;
    scoped& operator = (scoped && other) { timer = other.timer; start = other.start; other.timer = nullptr; return *this; }

    ~scoped() {
      if (timer) {
        auto end = std::chrono::high_resolution_clock::now();
        timer->elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
      }
    }

    timer_t* timer = nullptr;
    std::chrono::high_resolution_clock::time_point start;
  };

  std::chrono::microseconds elapsed_time = 0;
};

template <bool k_compute_stats = true>
struct statistics {
  std::uint32_t arenas_allocated = 0;
  std::size_t peak_allocation = 0;
  std::size_t allocation = 0;
  std::size_t deallocation_count = 0;
  std::size_t allocation_count = 0;
  timer_t allocation_timing;
  timer_t deallocation_timing;

  void report_new_arena(std::uint32_t count = 1) {
    arenas_allocated += count;
  }
  [[nodiscard]] timer_t::scoped report_allocate(std::size_t size) {
    allocation_count++;
    allocation += size;
    peak_allocation = std::max<std::size_t>(allocation, peak_allocation);
    return timer_t::scoped(allocation_timing);
  }
  [[nodiscard]] timer_t::scoped report_deallocate(std::size_t size) {
    deallocation_count++;
    allocation -= size;
    return timer_t::scoped(deallocation_timing);
  }
};

template <>
struct statistics<false> {
  std::false_type report_new_arena(std::uint32_t count) {
    return std::false_type{};
  }
  std::false_type report_allocate(std::size_t size) {
    return std::false_type{};
  }
  std::false_type report_deallocate(std::size_t size) {
    return std::false_type{};
  }
};

}
}