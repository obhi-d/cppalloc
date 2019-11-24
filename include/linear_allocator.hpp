#pragma once

#include <default_allocator.hpp>
#include <detail/cppalloc_common.hpp>

namespace cppalloc {

struct linear_allocator_tag {};

template <typename underlying_allocator = cppalloc::default_allocator<>,
          bool k_compute_stats          = false>
class linear_allocator
    : detail::statistics<linear_allocator_tag, k_compute_stats,
                         underlying_allocator> {
public:
	using tag = linear_allocator_tag;
	using statistics = detail::statistics<linear_allocator_tag, k_compute_stats,
	                                      underlying_allocator>;
	using size_type  = typename underlying_allocator::size_type;
	using address    = typename underlying_allocator::address;
	template <typename... Args>
	linear_allocator(size_type i_arena_size, Args&&... i_args)
	    : k_arena_size(i_arena_size), left_over(i_arena_size),
	      statistics(std::forward<Args>(i_args)...) {
		statistics::report_new_arena();
		buffer = underlying_allocator::allocate(k_arena_size);
	}

	~linear_allocator() {
		underlying_allocator::deallocate(buffer, k_arena_size);
	}

	address allocate(size_type i_size) {
		auto measure = statistics::report_allocate(i_size);
		// assert
		assert(left_over >= i_size);
		size_type offset = k_arena_size - left_over;
		left_over -= i_size;
		return reinterpret_cast<std::uint8_t*>(buffer) + offset;
	}

	void deallocate(address i_data, size_type i_size) {
		auto measure = statistics::report_deallocate(i_size);
		// merge back?
		size_type new_left_over = left_over + i_size;
		size_type offset        = (k_arena_size - new_left_over);
		if (reinterpret_cast<std::uint8_t*>(buffer) + offset ==
		    reinterpret_cast<std::uint8_t*>(i_data))
			left_over = new_left_over;
	}

	size_type get_free_size() const { return left_over; }

	static void unit_test() {
		using allocator_t =
		    linear_allocator<aligned_allocator<16, std::uint32_t, true>, true>;
		struct record {
			void*         data;
			std::uint32_t size;
		};
		constexpr std::uint32_t k_arena_size = 1000;
		allocator_t             allocator(k_arena_size);
		auto start  = cppalloc::allocate<std::uint8_t*>(allocator, 40);
		auto off100 = cppalloc::allocate<std::uint8_t*>(allocator, 100);
		assert(start + 40 == off100);
		allocator.deallocate(off100, 100);
		off100 = cppalloc::allocate<std::uint8_t*>(allocator, 100);
		assert(start + 40 == off100);
	}

private:
	address         buffer;
	size_type       left_over;
	const size_type k_arena_size;
};

} // namespace cppalloc