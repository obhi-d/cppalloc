#pragma once
#include <detail/cppalloc_common.hpp>
#include <detail/memory_tracker.hpp>

namespace cppalloc {

struct default_allocator_tag {};
struct aligned_allocator_tag {};

namespace traits {
template <> constexpr bool is_static_v<default_allocator_tag> = true;
template <> constexpr bool is_static_v<aligned_allocator_tag> = true;
} // namespace traits

namespace detail {
template <bool k_compute = false> struct default_alloc_statistics {
	static int report_allocate(std::size_t) { return 0; }
	static int report_deallocate(std::size_t) { return 0; }
};

template <bool k_compute = false> struct aligned_alloc_statistics {
	static int report_allocate(std::size_t) { return 0; }
	static int report_deallocate(std::size_t) { return 0; }
};

#ifndef CPPALLOC_NO_STATS

CPPALLOC_EXTERN CPPALLOC_API detail::statistics<default_allocator_tag, true>
             default_allocator_statistics_instance;
CPPALLOC_EXTERN CPPALLOC_API  detail::statistics<aligned_allocator_tag, true>
             aligned_alloc_statistics_instance;

template <> struct default_alloc_statistics<true> {
	inline static timer_t::scoped report_allocate(std::size_t i_sz) {
		return get_instance().report_allocate(i_sz);
	}
	inline static timer_t::scoped report_deallocate(std::size_t i_sz) {
		return get_instance().report_deallocate(i_sz);
	}
	inline static detail::statistics<default_allocator_tag, true>&
	get_instance() {
		return default_allocator_statistics_instance;
	}
};

template <> struct aligned_alloc_statistics<true> {
	inline static timer_t::scoped report_allocate(std::size_t i_sz) {
		return get_instance().report_allocate(i_sz);
	}
	inline static timer_t::scoped report_deallocate(std::size_t i_sz) {
		return get_instance().report_deallocate(i_sz);
	}
	inline static detail::statistics<aligned_allocator_tag, true>&
	get_instance() {
		return aligned_alloc_statistics_instance;
	}
};

#endif

} // namespace detail

template <typename size_ty = std::uint32_t, bool k_compute_stats = false,
          bool k_track_memory = false, typename stack_tracer = std::monostate,
          typename output_stream = std::monostate>
struct default_allocator
    : detail::default_alloc_statistics<k_compute_stats>,
      detail::memory_tracker<size_ty, stack_tracer, output_stream,
                             k_track_memory> {
	using tag        = default_allocator_tag;
	using address    = void*;
	using size_type  = size_ty;
	using statistics = detail::default_alloc_statistics<k_compute_stats>;
	using tracker = detail::memory_tracker<size_ty, stack_tracer, output_stream,
	                                       k_track_memory>;

	default_allocator() { }
	static address allocate(size_type i_sz) {
		auto measure = statistics::report_allocate(i_sz);
		return tracker::when_allocate(std::malloc(i_sz), i_sz);
	}
	static void deallocate(address i_addr, size_type i_sz) {
		auto measure = statistics::report_deallocate(i_sz);
		std::free(tracker::when_deallocate(i_addr, i_sz));
	}
};


template <std::uint32_t k_alignment, typename size_ty = std::uint32_t,
          bool k_compute_stats = false, bool k_track_memory = false,
          typename stack_tracer  = std::monostate,
          typename output_stream = std::monostate>
struct aligned_allocator
    : detail::aligned_alloc_statistics<k_compute_stats>,
      detail::memory_tracker<size_ty, stack_tracer, output_stream,
                             k_track_memory> {

	using tag        = aligned_allocator_tag;
	using address    = void*;
	using size_type  = size_ty;
	using statistics = detail::aligned_alloc_statistics<k_compute_stats>;
	using tracker = detail::memory_tracker<size_ty, stack_tracer, output_stream,
	                                       k_track_memory>;

	static address allocate(size_type i_sz) {
		auto measure = statistics::report_allocate(i_sz);
#ifdef _MSC_VER
		return tracker::when_allocate(_aligned_malloc(i_sz, k_alignment), i_sz);
#else
		return tracker::when_allocate(aligned_alloc(k_alignment, i_sz), i_sz);
#endif
	}
	static void deallocate(address i_addr, size_type i_sz) {
		auto measure = statistics::report_deallocate(i_sz);
#ifdef _MSC_VER
		_aligned_free(tracker::when_deallocate(i_addr, i_sz));
#else
		free(tracker::when_deallocate(i_addr, i_sz));
#endif
	}
};

} // namespace cppalloc