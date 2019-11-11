#pragma once
#include <detail/cppalloc_common.hpp>

namespace cppalloc {

struct default_allocator {
	using address   = void*;
	using size_type = std::size_t;

	address allocate(size_type i_sz) { return std::malloc(sz); }
	void    deallocate(address i_addr, size_type) { return std::free(i_addr); }
};

template <std::size_t k_alignment>
struct aligned_allocator {
	using address   = void*;
	using size_type = std::size_t;

	address allocate(size_type i_sz) { 
#ifdef _MSC_VER 
		_aligned_alloc(i_sz, k_alignment);
#else
		return std::aligned_alloc(k_alignment, i_sz);
#endif
}
	void    deallocate(address i_addr, size_type) { 
#ifdef _MSC_VER
		_aligned_free(i_addr);
#else
		return std::free(i_addr); 
#endif
	}
};


} // namespace cppalloc