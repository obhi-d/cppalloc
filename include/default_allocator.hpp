#pragma once
#include <detail/cppalloc_common.hpp>
#include <detail/memory_tracker.hpp>

namespace cppalloc
{
struct default_allocator_tag
{
};

namespace traits
{
template <>
struct is_static<default_allocator_tag>
{
  constexpr inline static bool value = true;
};
} // namespace traits

namespace detail
{
template <bool k_compute = false>
struct default_alloc_statistics
{
  static int report_allocate(std::size_t)
  {
    return 0;
  }
  static int report_deallocate(std::size_t)
  {
    return 0;
  }
};

#ifndef CPPALLOC_NO_STATS

CPPALLOC_EXTERN CPPALLOC_API detail::statistics<default_allocator_tag, true> default_allocator_statistics_instance;

template <>
struct default_alloc_statistics<true>
{
  inline static timer_t::scoped report_allocate(std::size_t i_sz)
  {
    return get_instance().report_allocate(i_sz);
  }
  inline static timer_t::scoped report_deallocate(std::size_t i_sz)
  {
    return get_instance().report_deallocate(i_sz);
  }
  inline static detail::statistics<default_allocator_tag, true>& get_instance()
  {
    return default_allocator_statistics_instance;
  }
};

#endif
} // namespace detail

inline void print_final_stats()
{
  detail::default_allocator_statistics_instance.print();
}

template <typename size_arg = std::uint32_t, std::size_t k_default_alignment = 0, bool k_compute_stats = false,
          bool k_track_memory = false, typename debug_tracer = std::monostate>
struct CPPALLOC_EMPTY_BASES default_allocator
    : detail::default_alloc_statistics<k_compute_stats>,
      detail::memory_tracker<default_allocator_tag, debug_tracer, k_track_memory>
{
  using tag        = default_allocator_tag;
  using address    = void*;
  using size_type  = size_arg;
  using statistics = detail::default_alloc_statistics<k_compute_stats>;
  using tracker    = detail::memory_tracker<default_allocator_tag, debug_tracer, k_track_memory>;

  default_allocator() {}
  default_allocator(
      default_allocator<size_arg, k_default_alignment, k_compute_stats, k_track_memory, debug_tracer> const&)
  {
  }
  default_allocator& operator=(
      default_allocator<size_arg, k_default_alignment, k_compute_stats, k_track_memory, debug_tracer> const&)
  {
    return *this;
  }

  inline static address allocate(size_type i_sz, size_type i_alignment = 0)
  {
    auto measure = statistics::report_allocate(i_sz);
    i_alignment  = std::max<size_type>(i_alignment, static_cast<size_type>(k_default_alignment));
    return tracker::when_allocate(i_alignment ?
#ifdef _MSC_VER
                                              _aligned_malloc(i_sz, i_alignment)
#else
                                              aligned_alloc(i_alignment, i_sz)
#endif
                                              : std::malloc(i_sz),
                                  i_sz);
  }

  static void deallocate(address i_addr, size_type i_sz, size_type i_alignment = 0)
  {
    auto  measure = statistics::report_deallocate(i_sz);
    void* fixup   = tracker::when_deallocate(i_addr, i_sz);
    if (i_alignment || k_default_alignment)
#ifdef _MSC_VER
      _aligned_free(fixup);
#else
      free(fixup);
#endif
    else
      std::free(fixup);
  }

  static constexpr void* null()
  {
    return nullptr;
  }
};

} // namespace cppalloc