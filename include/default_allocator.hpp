#pragma once
#include <detail/cppalloc_common.hpp>

namespace cppalloc {

struct default_allocator_tag {};

template <typename size_ty = std::uint32_t, bool k_compute_stats = false>
struct default_allocator
    : detail::statistics<default_allocator_tag, k_compute_stats> {
	using address    = void*;
	using size_type  = size_ty;
	using statistics = detail::statistics<default_allocator_tag, k_compute_stats>;

	default_allocator() { statistics::report_new_arena(); }
	address allocate(size_type i_sz) {
		auto measure = statistics::report_allocate(i_sz);
		return std::malloc(i_sz);
	}
	void deallocate(address i_addr, size_type i_sz) {
		auto measure = statistics::report_deallocate(i_sz);
		return std::free(i_addr);
	}
};

struct aligned_allocator_tag {};

template <std::uint32_t k_alignment, typename size_ty = std::uint32_t,
          bool          k_compute_stats = false>
struct aligned_allocator
    : detail::statistics<aligned_allocator_tag, k_compute_stats> {
	using address    = void*;
	using size_type  = size_ty;
	using statistics = detail::statistics<aligned_allocator_tag, k_compute_stats>;

	address allocate(size_type i_sz) {
		auto measure = statistics::report_allocate(i_sz);
#ifdef _MSC_VER
		return _aligned_malloc(i_sz, k_alignment);
#else
		return aligned_alloc(k_alignment, i_sz);
#endif
	}
	void deallocate(address i_addr, size_type i_sz) {
		auto measure = statistics::report_allocate(i_sz);
#ifdef _MSC_VER
		_aligned_free(i_addr);
#else
		free(i_addr);
#endif
	}
};

} // namespace cppalloc