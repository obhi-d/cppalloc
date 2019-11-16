#pragma once
#include <detail/cppalloc_common.hpp>
#include <detail/memory_tracker.hpp>

namespace cppalloc {

struct default_allocator_tag {};

template <typename size_ty = std::uint32_t, bool k_compute_stats = false,
          bool k_track_memory = false, typename stack_tracer = std::monostate, typename output_stream = std::monostate>
struct default_allocator
    : detail::statistics<default_allocator_tag, k_compute_stats>,
      detail::memory_tracker<size_ty, stack_tracer,
                             output_stream, k_track_memory> {
	using address    = void*;
	using size_type  = size_ty;
	using statistics = detail::statistics<default_allocator_tag, k_compute_stats>;
	using tracker = detail::memory_tracker<size_ty, stack_tracer, output_stream, k_track_memory>;

	default_allocator() { statistics::report_new_arena(); }
	address allocate(size_type i_sz) {
		auto measure = statistics::report_allocate(i_sz);
		return tracker::when_allocate(std::malloc(i_sz), i_sz);
	}
	void deallocate(address i_addr, size_type i_sz) {
		auto measure = statistics::report_deallocate(i_sz);
		std::free(tracker::when_deallocate(i_addr, i_sz));
	}
};

template <std::uint32_t k_alignment> struct aligned_allocator_tag {};

template <std::uint32_t k_alignment, typename size_ty = std::uint32_t,
          bool k_compute_stats = false, bool k_track_memory = false,
          typename stack_tracer  = std::monostate,
          typename output_stream = std::monostate>
struct aligned_allocator
    : detail::statistics<aligned_allocator_tag<k_alignment>, k_compute_stats>,
      detail::memory_tracker<size_ty, stack_tracer,
                             output_stream, k_track_memory> {
	using address   = void*;
	using size_type = size_ty;
	using statistics =
	    detail::statistics<aligned_allocator_tag<k_alignment>, k_compute_stats>;
	using tracker = detail::memory_tracker<size_ty, stack_tracer, output_stream, k_track_memory>;

	address allocate(size_type i_sz) {
		auto measure = statistics::report_allocate(i_sz);
#ifdef _MSC_VER
		return tracker::when_allocate(_aligned_malloc(i_sz, k_alignment), i_sz);
#else
		return tracker::when_allocate(aligned_alloc(k_alignment, i_sz), i_sz);
#endif
	}
	void deallocate(address i_addr, size_type i_sz) {
		auto measure = statistics::report_deallocate(i_sz);
#ifdef _MSC_VER
		_aligned_free(tracker::when_deallocate(i_addr, i_sz));
#else
		free(tracker::when_deallocate(i_addr, i_sz));
#endif
	}
};

} // namespace cppalloc