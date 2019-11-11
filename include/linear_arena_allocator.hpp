#pragma once

#include <linear_allocator.hpp>

namespace cppalloc {
template <typename underlying_allocator = cppalloc::default_allocator<>,
          bool k_compute_stats          = false>
class linear_arena_allocator : underlying_allocator,
                               detail::statistics<k_compute_stats> {
public:
	using statistics = detail::statistics<k_compute_stats>;
	using size_type  = typename underlying_allocator::size_type;
	using address    = typename underlying_allocator::address;
	enum : size_type {
		k_minimum_size =
		    static_cast<size_type>(std::hardware_destructive_interference_size)
	};

	template <typename... Args>
	linear_arena_allocator(size_type i_arena_size, Args&&... i_args)
	    : k_arena_size(i_arena_size),
	      underlying_allocator(std::forward<Args>(i_args)...) {
		// Initializing the cursor is important for the
		// allocate loop to work.
		current_arena = allocate_new_arena(k_arena_size);
	}

	address allocate(size_type i_size) {

		auto measure = statistics::report_allocate(i_size);

		for (size_type id  = current_arena,
		               end = static_cast<size_type>(arenas.size());
		     id < end; ++id) {
			size_type index = id - 1;
			if (arenas[index].left_over > i_size)
				return allocate_from(index, i_size);
			else {
				if (arenas[index].left_over < k_minimum_size && index != current_arena)
					std::swap(arenas[index], arenas[current_arena++]);
			}
		}
		size_type max_arena_size = std::max<size_type>(i_size, k_arena_size);
		return allocate_from(allocate_new_arena(max_arena_size), i_size);
	}

	void deallocate(address i_data, size_type i_size) {

		auto measure = statistics::report_deallocate(i_size);

		for (size_type id = static_cast<size_type>(arenas.size()) - 1;
		     id >= current_arena; --id) {
			if (in_range(arenas[id], i_data)) {
				// merge back?
				size_type new_left_over = arenas[id].left_over + i_size;
				size_type offset        = (arenas[id].arena_size - new_left_over);
				if (reinterpret_cast<std::uint8_t*>(arenas[id].buffer) + offset ==
				    reinterpret_cast<std::uint8_t*>(i_data))
					arenas[id].left_over = new_left_over;
				return;
			}
		}
	}

  static void unit_test();
  
private:
	struct arena {
		address   buffer;
		size_type left_over;
		size_type arena_size;
		arena() = default;
		arena(address i_buffer, size_type i_size, size_type i_arena_size)
		    : buffer(i_buffer), left_over(i_size), arena_size(i_arena_size) {}
	};

	inline bool in_range(const arena& i_arena, address i_data) {
		return (i_arena.buffer <= i_data &&
		        i_data < (static_cast<std::uint8_t*>(i_arena.buffer) +
		                  i_arena.arena_size)) != 0;
	}

	inline size_type allocate_new_arena(size_type size) {
		size_type index = static_cast<size_type>(arenas.size());
		arenas.emplace_back(underlying_allocator::allocate(size), size, size);
		return index;
	}

	inline address allocate_from(size_type id, size_type size) {
		size_type offset = k_arena_size - arenas[id].left_over;
		arenas[id].left_over -= size;
		return static_cast<std::uint8_t*>(arenas[id].buffer) + offset;
	}

	std::vector<arena> arenas;
	size_type          current_arena;

	const size_type k_arena_size;
};

} // namespace cppalloc