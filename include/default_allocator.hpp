#pragma once
#include <detail/cppalloc_common.hpp>
#include <detail/memory_tracker.hpp>

namespace cppalloc {
struct default_allocator_tag {};
struct aligned_allocator_tag {};
struct general_allocator_tag {};

namespace traits {
template <> struct is_static<default_allocator_tag> {
	constexpr inline static bool value = true;
};
template <> struct is_static<aligned_allocator_tag> {
	constexpr inline static bool value = true;
};
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
CPPALLOC_EXTERN CPPALLOC_API detail::statistics<aligned_allocator_tag, true>
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

inline void print_final_stats() {
	detail::aligned_alloc_statistics_instance.print();
	detail::default_allocator_statistics_instance.print();
}

template <typename size_arg = std::uint32_t, bool k_compute_stats = false,
          bool k_track_memory = false, typename debug_tracer = std::monostate>
struct CPPALLOC_EMPTY_BASES default_allocator
    : detail::default_alloc_statistics<k_compute_stats>,
      detail::memory_tracker<default_allocator_tag, debug_tracer,
                             k_track_memory> {
	using tag        = default_allocator_tag;
	using address    = void*;
	using size_type  = size_arg;
	using statistics = detail::default_alloc_statistics<k_compute_stats>;
	using tracker    = detail::memory_tracker<default_allocator_tag, debug_tracer,
                                         k_track_memory>;

	default_allocator() {}
	default_allocator(default_allocator<size_arg, k_compute_stats, k_track_memory,
	                                    debug_tracer> const&) {}
	default_allocator& operator=(
	    default_allocator<size_arg, k_compute_stats, k_track_memory,
	                      debug_tracer> const&) {
		return *this;
	}
	static address allocate(size_type i_sz) {
		auto measure = statistics::report_allocate(i_sz);
		return tracker::when_allocate(std::malloc(i_sz), i_sz);
	}
	static void deallocate(address i_addr, size_type i_sz) {
		auto measure = statistics::report_deallocate(i_sz);
		std::free(tracker::when_deallocate(i_addr, i_sz));
	}
};

template <std::uint32_t k_alignment, typename size_arg = std::uint32_t,
          bool k_compute_stats = false, bool k_track_memory = false,
          typename debug_tracer = std::monostate>
struct CPPALLOC_EMPTY_BASES aligned_allocator
    : detail::aligned_alloc_statistics<k_compute_stats>,
      detail::memory_tracker<aligned_allocator_tag, debug_tracer,
                             k_track_memory> {
	using tag        = aligned_allocator_tag;
	using address    = void*;
	using size_type  = size_arg;
	using statistics = detail::aligned_alloc_statistics<k_compute_stats>;
	using tracker    = detail::memory_tracker<aligned_allocator_tag, debug_tracer,
                                         k_track_memory>;

	aligned_allocator() {}
	aligned_allocator(aligned_allocator<k_alignment, size_arg, k_compute_stats,
	                                    k_track_memory, debug_tracer> const&) {}

	aligned_allocator& operator=(
	    aligned_allocator<k_alignment, size_arg, k_compute_stats, k_track_memory,
	                      debug_tracer> const&) {
		return *this;
	}

	static address allocate(size_type i_sz) {
		auto measure = statistics::report_allocate(i_sz);
#ifdef _MSC_VER
		return tracker::when_allocate(_aligned_malloc(i_sz, k_alignment), i_sz);
#else
		return tracker::when_allocate(
		    aligned_alloc(k_alignment, i_sz + (i_sz & (k_alignment - 1))), i_sz);
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

template <typename size_arg = std::uint32_t, bool k_compute_stats = false,
          bool k_track_memory = false, typename debug_tracer = std::monostate>
struct CPPALLOC_EMPTY_BASES general_allocator
    : detail::aligned_alloc_statistics<k_compute_stats>,
      detail::memory_tracker<general_allocator_tag, debug_tracer,
                             k_track_memory> {
	using tag        = aligned_allocator_tag;
	using address    = void*;
	using size_type  = size_arg;
	using statistics = detail::aligned_alloc_statistics<k_compute_stats>;
	using tracker    = detail::memory_tracker<general_allocator_tag, debug_tracer,
                                         k_track_memory>;

	general_allocator() {}
	general_allocator(general_allocator<size_arg, k_compute_stats, k_track_memory,
	                                    debug_tracer> const&) {}

	general_allocator& operator=(
	    general_allocator<size_arg, k_compute_stats, k_track_memory,
	                      debug_tracer> const&) {
		return *this;
	}

	static address allocate(size_type i_alignment, size_type i_sz) {
		auto measure = statistics::report_allocate(i_sz);
#ifdef _MSC_VER
		return tracker::when_allocate(_aligned_malloc(i_sz, i_alignment), i_sz);
#else
		return tracker::when_allocate(aligned_alloc(i_alignment, i_sz), i_sz);
#endif
	}
	static void deallocate(address i_addr, [[maybe_unused]] size_type i_alignment,
	                       size_type i_sz) {
		auto measure = statistics::report_deallocate(i_sz);
#ifdef _MSC_VER
		_aligned_free(tracker::when_deallocate(i_addr, i_sz));
#else
		free(tracker::when_deallocate(i_addr, i_sz));
#endif
	}
};
} // namespace cppalloc