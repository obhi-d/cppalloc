#pragma once
#include <detail/cppalloc_common.hpp>
#include <detail/memory_tracker.hpp>

namespace cppalloc {

struct default_allocator_tag {};
struct aligned_allocator_tag {};

namespace detail {
template <bool k_compute = false> struct default_alloc_statistics {
	static int report_allocate(std::size_t) { return 0; }
	static int report_deallocate(std::size_t) { return 0; }
};

template <bool k_compute = false> struct aligned_alloc_statistics {
	static int report_allocate(std::size_t) { return 0; }
	static int report_deallocate(std::size_t) { return 0; }
};

template <> struct default_alloc_statistics<true> {
	static timer_t::scoped report_allocate(std::size_t i_sz) {
		return instance.report_allocate(i_sz);
	}
	static timer_t::scoped report_deallocate(std::size_t i_sz) {
		return instance.report_deallocate(i_sz);
	}
	CPPALLOC_API static detail::statistics<default_allocator_tag, true> instance;
};

template <> struct aligned_alloc_statistics<true> {
	static timer_t::scoped report_allocate(std::size_t i_sz) {
		return instance.report_allocate(i_sz);
	}
	static timer_t::scoped report_deallocate(std::size_t i_sz) {
		return instance.report_deallocate(i_sz);
	}
	CPPALLOC_API static detail::statistics<aligned_allocator_tag, true> instance;
};

} // namespace detail

template <typename size_ty = std::uint32_t, bool k_compute_stats = false,
          bool k_track_memory = false, typename stack_tracer = std::monostate,
          typename output_stream = std::monostate>
struct default_allocator
    : detail::default_alloc_statistics<k_compute_stats>,
      detail::memory_tracker<size_ty, stack_tracer, output_stream,
                             k_track_memory> {
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
	using address   = void*;
	using size_type = size_ty;
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