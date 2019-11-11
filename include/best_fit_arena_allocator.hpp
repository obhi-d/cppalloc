#pragma once

#include <detail/cppalloc_common.hpp>

namespace cppalloc {

//! @remarks Class represents an allocator
template <typename arena_manager, typename size_type = std::uint32_t,
          bool k_compute_stats = false>
class best_fit_arena_allocator : detail::statistics<k_compute_stats>,
                                 public arena_manager {
	using statistics = detail::statistics<k_compute_stats>;

public:
	enum : size_type {
		k_invalid_offset = std::numeric_limits<size_type>::max(),
		k_invalid_size   = std::numeric_limits<size_type>::max(),
		k_invalid_handle = std::numeric_limits<size_type>::max(),
		k_invalid_page   = std::numeric_limits<size_type>::max()
	};

	enum option : std::uint32_t {
		f_defrag          = 1 << 0,
		f_alloc_new_arena = 1 << 1,
	};

	using option_flags = std::uint32_t;

	//! Address
	struct address {
		size_type offset;
		size_type arena;
	};

private:
	struct block_type {
		size_type offset;
		size_type id;
	};

	using block_list    = std::vector<block_type>;
	using block_list_it = typename block_list::iterator;
	bool      is_valid() const;
	size_type add_arena(size_type i_user_handle, size_type i_arena_size);

public:
	//! Inteface
	template <typename... Args>
	best_fit_arena_allocator(size_type i_arena_size, Args&&...);

	//! Allocate, second paramter is the user_handle to be associated with the
	//! allocation
	address allocate(size_type i_size, size_type i_user_handle = k_invalid_handle,
	                 option_flags i_flags = 0);
	//! Deallocate, size is optional
	void deallocate(address i_address, size_type i_size);

	//! Helpers
	size_type get_free_size() const;
	size_type get_max_free_block_size() const const;
	void      defragment();
	float     get_fragmentation();

	static void unit_test();

	class move_iterator {
		using iterator = block_list_it;

	public:
		move_iterator(iterator i_it, iterator i_end, size_type i_delta);

		inline bool has_next(size_type& o_handle);
		inline void modify_offset(size_type& io_offset);

	private:
		iterator  it;
		iterator  end;
		size_type delta_offset;
	};

private:
	struct arena {
		size_type  total_size;
		size_type  free_size;
		block_list blocks;
	};

	using arena_list = std::vector<arena>;

	struct locator_type {
		size_type offset;
		size_type size;
	};

	struct free_block_type {
		size_type arena;
		size_type offset;
		size_type size;

		friend inline bool operator==(const free_block_type& i_first,
		                              const free_block_type& i_sec) {
			return i_first.size == i_sec.size && i_first.arena == i_sec.arena &&
			       i_first.offset == i_sec.offset;
		}

		friend inline bool operator!=(const free_block_type& i_first,
		                              const free_block_type& i_sec) {
			return !(i_first == i_sec);
		}

		friend inline bool operator<(const free_block_type& i_first,
		                             const free_block_type& i_sec) {
			return (i_first.size < i_sec.size) ||
			       ((i_first.size == i_sec.size) && (i_first.arena < i_sec.arena)) ||
			       ((i_first.size == i_sec.size) && (i_first.arena == i_sec.arena) &&
			        (i_first.offset < i_sec.offset));
		}
	};

	using free_block_list    = std::vector<free_block_type>;
	using free_block_list_it = typename free_block_list::iterator;

	inline free_block_list_it free_lookup(free_block_list_it begin,
	                                      free_block_list_it end,
	                                      free_block_type    lookup);
	inline free_block_list_it merge_free(free_block_type i_rem,
	                                     size_type       i_merge_size,
	                                     size_type       i_new_offset);

	inline free_block_list_it reinsert_left(free_block_list_it node);
	inline free_block_list_it reinsert_right(free_block_list_it node);
	inline void               erase_and_reinsert(free_block_list_it erase,
	                                             free_block_list_it reinsert);

	arena_list             arenas;
	free_block_list        free_list;
	std::vector<size_type> free_pages;
	size_type              free_size = 0;
	size_type              arena_size;
};

template <typename arena_manager, typename size_type, bool k_compute_stats>
template <typename... Args>
inline best_fit_arena_allocator<arena_manager, size_type, k_compute_stats>::
    best_fit_arena_allocator(size_type i_arena_size, Args&&... i_args)
    : arena_manager(std::forward<Args>(i_args)...), arena_size(i_arena_size) {}

template <typename arena_manager, typename size_type, bool k_compute_stats>
inline typename best_fit_arena_allocator<arena_manager, size_type,
                                         k_compute_stats>::address
best_fit_arena_allocator<arena_manager, size_type, k_compute_stats>::allocate(
    size_type i_size, size_type i_user_handle, option_flags i_options) {

	auto measure = statistics::report_allocate(i_size);

	assert(i_user_handle != k_invalid_handle);
	if (i_options & f_alloc_new_arena) {
		return {0, add_arena(i_user_handle, i_size)};
	}

	if (i_size > get_max_free_block_size()) {
		if (i_options & f_defrag) {
			defragment();
			if (i_size > get_max_free_block_size())
				add_arena(k_invalid_handle, arena_size);
		} else
			add_arena(k_invalid_handle, arena_size);
	}

	auto end = free_list.end();
	auto found =
	    std::lower_bound(free_list.begin(), end, i_size,
	                     [](free_block_type block, size_type size) -> bool {
		                     return block.size < size;
	                     });

	if (found == end) {
		return {k_invalid_offset, k_invalid_page};
	}

	size_type offset = (*found).offset;
	found->size -= i_size;
	size_type arena_num = found->arena;
	auto&     arena     = arenas[arena_num];
	auto&     node_list = arena.blocks;
	auto      alEnd     = node_list.end();
	auto      it = std::lower_bound(node_list.begin(), alEnd, (*found).offset,
                             [](block_type block, size_type offset) -> bool {
                               return block.offset < offset;
                             });

	// set allocated to right size, increment it
	it->id = i_user_handle;
	if (found->size > 0) {
		size_type numAllocCount = static_cast<size_type>(node_list.size());
		// reinsert the left-over size in free list
		found->offset += i_size;
		size_type offset = found->offset;
		reinsert_left(found);
		node_list.insert(it + 1, block_type{offset, k_invalid_handle});
	} else {
		// delete the existing found index from free list
		free_list.erase(found);
	}
	arena.free_size -= i_size;
	free_size -= i_size;
	return {offset, arena_num};
}

//! Helpers
template <typename arena_manager, typename size_type, bool k_compute_stats>
inline size_type best_fit_arena_allocator<
    arena_manager, size_type, k_compute_stats>::get_free_size() const {
	return free_size;
}

template <typename arena_manager, typename size_type, bool k_compute_stats>
inline void best_fit_arena_allocator<arena_manager, size_type,
                                     k_compute_stats>::
    deallocate(best_fit_arena_allocator<arena_manager, size_type,
                                        k_compute_stats>::address i_address,
               size_type                                          i_size) {

	auto measure = statistics::report_deallocate(i_size);

	auto& arena     = arenas[i_address.arena];
	auto& node_list = arena.blocks;
	auto  end       = node_list.end();
	auto  node      = std::lower_bound(node_list.begin(), end, i_address.offset,
                               [](block_type block, size_type offset) -> bool {
                                 return block.offset < offset;
                               });

	assert(node->offset == i_address.offset);
	// last index is not used
	block_list_it next = std::next(node);
	size_type     size = next->offset - node->offset;
	free_size += size;
	arena.free_size += size;
	block_list_it it = node;

	size_type     merges = 0;
	block_list_it prev;
	locator_type  left_removal = {
      k_invalid_offset,
      best_fit_arena_allocator<arena_manager, size_type,
                               k_compute_stats>::k_invalid_size};
	locator_type right_removal = {
	    k_invalid_offset,
	    best_fit_arena_allocator<arena_manager, size_type,
	                             k_compute_stats>::k_invalid_size};

	if (node != node_list.begin() &&
	    (prev = std::prev(node))->id == k_invalid_handle) {
		it                = prev;
		size_type prev_sz = node->offset - prev->offset;
		left_removal      = locator_type{prev->offset, prev_sz};
		merges++;
	}
	if (std::next(next) != node_list.end() && next->id == k_invalid_handle) {
		size_type next_sz = std::next(next)->offset - next->offset;
		right_removal     = locator_type{next->offset, next_sz};
		merges++;
	}
	if (arena.free_size == arena.total_size &&
	    arena_manager::drop_arena(i_address.arena)) {
		// drop arena?
		if (left_removal.offset != k_invalid_offset) {
			auto it = free_lookup(
			    free_list.begin(), free_list.end(),
			    {i_address.arena, left_removal.offset, left_removal.size});
			free_list.erase(it);
		}
		if (right_removal.offset != k_invalid_offset) {
			auto it = free_lookup(
			    free_list.begin(), free_list.end(),
			    {i_address.arena, right_removal.offset, right_removal.size});
			free_list.erase(it);
		}
		free_size -= arena.total_size;
		arena.total_size = 0;
		arena.free_size  = 0;
		arena.blocks.clear();
		free_pages.push_back(i_address.arena);
		return;
	}
	if (merges > 0) {
		if (merges == 2) {
			// erase left, reinsert right
			auto left = free_lookup(
			    free_list.begin(), free_list.end(),
			    {i_address.arena, left_removal.offset, left_removal.size});
			decltype(left) right;
			if (left_removal.size <= right_removal.size) {
				right = free_lookup(
				    left, free_list.end(),
				    {i_address.arena, right_removal.offset, right_removal.size});
				right->offset = left_removal.offset;
				right->size += (size + left->size);
				erase_and_reinsert(left, right);
			} else {
				right = free_lookup(
				    free_list.begin(), left,
				    {i_address.arena, right_removal.offset, right_removal.size});
				left->size += (size + right->size);
				erase_and_reinsert(right, left);
			}
		} else if (left_removal.offset != k_invalid_offset)
			merge_free({i_address.arena, left_removal.offset, left_removal.size},
			           size, left_removal.offset);
		else
			merge_free({i_address.arena, right_removal.offset, right_removal.size},
			           size, node->offset);

		it->id          = k_invalid_handle;
		block_type* dst = &*it + 1;
		block_type* src = dst + merges;
		size_t      count =
		    reinterpret_cast<size_t>(node_list.data() + node_list.size()) -
		    reinterpret_cast<size_t>(src);
		std::memmove(dst, src, count);
		node_list.resize(node_list.size() - merges);
	} else {
		free_block_type newBlock{i_address.arena, i_address.offset, size};
		auto found = free_lookup(free_list.begin(), free_list.end(), newBlock);
		free_list.emplace(found, newBlock);
		node->id = k_invalid_handle;
	}
}

template <typename arena_manager, typename size_type, bool k_compute_stats>
inline void best_fit_arena_allocator<arena_manager, size_type,
                                     k_compute_stats>::defragment() {
	// move all memory to one end
	free_block_list new_free_list;
	size_type       arena_count = static_cast<size_type>(arenas.size());
	new_free_list.reserve(arena_count);
	size_type total_remove_size = 0;
	for (size_type p = 0; p < arena_count; ++p) {
		auto& arena = arenas[p];
		if (arena.blocks.size() <= 0)
			continue;
		auto& node_list = arena.blocks;

		size_type end = (size_type)node_list.size() - 1;
		size_type i   = 0;

		size_type last = k_invalid_offset;
		size_type occ  = k_invalid_offset;

		while (i < end) {
			if (node_list[i].id == k_invalid_handle) {
				last = i++;
				break;
			}
			i++;
		}
		while (i < end) {
			if (node_list[i].id == k_invalid_handle) {
				if (occ == k_invalid_offset) {
					i++;
					continue;
				}
				// dispatch
				size_type     move_offset = node_list[occ].offset;
				size_type     cur_offset  = node_list[i].offset;
				size_type     last_offset = node_list[last].offset;
				size_type     size        = cur_offset - move_offset;
				auto          it          = node_list.begin() + occ;
				auto          itEnd       = node_list.begin() + i;
				move_iterator mv(it, itEnd, move_offset - last_offset);
				arena_manager::move_memory({it->offset, p}, {last_offset, p}, size, mv);

				for (size_type cpy = occ, l = last; cpy < i; ++cpy) {
					size_type size = node_list[cpy + 1].offset - node_list[cpy].offset;
					size_type id   = node_list[cpy].id;
					node_list[l++] = block_type{last_offset, id};
					last_offset += size;
				}

				last                   = i - (occ - last);
				node_list[last].offset = last_offset;
				occ                    = k_invalid_offset;
			} else if (occ == k_invalid_offset)
				occ = i;
			i++;
		}
		if (last != k_invalid_offset) {
			// merge all nodes from last to occ
			free_block_type new_free;
			new_free.arena  = p;
			new_free.offset = node_list[last].offset;
			size_type removed_size;
			if (occ == k_invalid_offset) {
				removed_size  = arena_size - node_list[last].offset;
				new_free.size = removed_size;
				node_list.resize(last + 1);
				node_list[last].id = k_invalid_handle;
				node_list.push_back({arena_size, k_invalid_handle});
			} else {
				removed_size        = node_list[occ].offset - node_list[last].offset;
				new_free.size       = removed_size;
				node_list[last].id  = k_invalid_handle;
				auto      data      = node_list.data();
				size_type numToMove = static_cast<size_type>(node_list.size() - occ);
				std::memmove(data + last + 1, data + occ,
				             (numToMove) * sizeof(block_type));
				node_list.resize(numToMove + last +
				                 1); // because size = node_list.size - (occ - last)
			}
			new_free_list.push_back(new_free);
			total_remove_size += removed_size;
		}
	}

	std::sort(new_free_list.begin(), new_free_list.end());
	free_list = std::move(new_free_list);
	assert(total_remove_size == free_size);
}

template <typename arena_manager, typename size_type, bool k_compute_stats>
inline size_type best_fit_arena_allocator<
    arena_manager, size_type,
    k_compute_stats>::add_arena(size_type i_handle, size_type i_arena_size) {

	statistics::report_new_arena();

	size_type arena_num = static_cast<size_type>(arenas.size());
	if (free_pages.size() > 0) {
		arena_num = free_pages.back();
		free_pages.pop_back();
	} else
		arenas.push_back(arena());

	auto& arena      = arenas[arena_num];
	auto& node_list  = arena.blocks;
	arena.total_size = i_arena_size;
	if (i_handle == k_invalid_handle) {
		arena.free_size = i_arena_size;
		free_list.push_back({arena_num, 0, i_arena_size});
		node_list.push_back({0, k_invalid_handle});
		free_size += i_arena_size;
	} else {
		node_list.push_back({0, i_handle});
		arena.free_size = 0;
	}
	node_list.push_back({i_arena_size, k_invalid_handle});
	arena_manager::add_arena(arena_num, i_arena_size);
	return arena_num;
}

template <typename arena_manager, typename size_type, bool k_compute_stats>
inline size_type best_fit_arena_allocator<
    arena_manager, size_type, k_compute_stats>::get_max_free_block_size()
    const {
	return free_list.size() > 0 ? free_list.back().size : 0;
}

template <typename arena_manager, typename size_type, bool k_compute_stats>
inline float best_fit_arena_allocator<arena_manager, size_type,
                                      k_compute_stats>::get_fragmentation() {
	return free_list.size() > 0
	           ? (float)(free_size - free_list.back().size) / (float)free_size
	           : 0.0f;
}

template <typename arena_manager, typename size_type, bool k_compute_stats>
inline bool best_fit_arena_allocator<arena_manager, size_type,
                                     k_compute_stats>::is_valid() const {
	size_type computed_size = 0;

	for (size_type i = 0; i < free_list.size(); ++i) {
		auto f = free_list[i];
		if (i > 0 && free_list[i - 1].size > free_list[i].size)
			return false;
		computed_size += f.size;
		auto& node_list = arenas[f.arena].blocks;
		auto  it = std::lower_bound(node_list.begin(), node_list.end(), f.offset,
                               [](block_type block, size_type offset) -> bool {
                                 return block.offset < offset;
                               });
		if (it == node_list.end() || (*it).id != k_invalid_handle ||
		    (*it).offset != f.offset)
			return false;
	}

	if (computed_size != free_size)
		return false;

	for (auto& p : arenas) {
		auto& node_list = p.blocks;
		for (size_type i = 0; i < node_list.size(); ++i) {
			if (i > 0 && (node_list[i - 1].offset > node_list[i].offset ||
			              node_list[i - 1].offset == node_list[i].offset))
				return false;
		}
		if (node_list.size() > 0 && (node_list.back().id != k_invalid_handle ||
		                             node_list.back().offset != arena_size))
			return false;
	}
	return true;
}

template <typename arena_manager, typename size_type, bool k_compute_stats>
inline void best_fit_arena_allocator<
    arena_manager, size_type,
    k_compute_stats>::erase_and_reinsert(free_block_list_it erase,
                                         free_block_list_it reinsert) {
	// erase is left to reinsert, so we can reinsert to right and still have a
	// valid iterator.
	reinsert_right(reinsert);
	free_list.erase(erase);
}

template <typename arena_manager, typename size_type, bool k_compute_stats>
inline typename best_fit_arena_allocator<arena_manager, size_type,
                                         k_compute_stats>::free_block_list_it
best_fit_arena_allocator<arena_manager, size_type, k_compute_stats>::
    reinsert_right(free_block_list_it node) {
	free_block_list_it end  = free_list.end();
	free_block_type    copy = *node;
	auto               next = std::next(node);
	auto               it   = free_lookup(next, end, *node);
	if (it != next) {
		free_block_type* dest  = &*node;
		free_block_type* src   = dest + 1;
		size_t           count = std::distance(next, it);
		std::memmove(dest, src, count * sizeof(free_block_type));
		auto ptr = (dest + count);
		*ptr     = copy;
		return free_list.begin() + std::distance(free_list.data(), ptr);
	}
	return node;
}

template <typename arena_manager, typename size_type, bool k_compute_stats>
inline typename best_fit_arena_allocator<arena_manager, size_type,
                                         k_compute_stats>::free_block_list_it
best_fit_arena_allocator<arena_manager, size_type, k_compute_stats>::
    reinsert_left(free_block_list_it node) {
	free_block_list_it begin = free_list.begin();
	free_block_type    copy  = *node;
	auto               it    = free_lookup(begin, node, *node);
	if (it != node) {
		free_block_type* src   = &*it;
		free_block_type* dest  = src + 1;
		size_t           count = std::distance(it, node);
		std::memmove(dest, src, count * sizeof(free_block_type));
		*it = copy;
		return it;
	}
	return node;
}

template <typename arena_manager, typename size_type, bool k_compute_stats>
inline typename best_fit_arena_allocator<arena_manager, size_type,
                                         k_compute_stats>::free_block_list_it
best_fit_arena_allocator<arena_manager, size_type, k_compute_stats>::merge_free(
    free_block_type i_rem, size_type i_merge_size, size_type i_new_offset) {
	auto found = free_lookup(free_list.begin(), free_list.end(), i_rem);
	assert(found != free_list.end() && (*found) == i_rem);
	found->size += i_merge_size;
	found->offset = i_new_offset;
	return reinsert_right(found);
}

template <typename arena_manager, typename size_type, bool k_compute_stats>
inline typename best_fit_arena_allocator<arena_manager, size_type,
                                         k_compute_stats>::free_block_list_it
best_fit_arena_allocator<arena_manager, size_type,
                         k_compute_stats>::free_lookup(free_block_list_it begin,
                                                       free_block_list_it end,
                                                       free_block_type lookup) {
	return std::lower_bound(begin, end, lookup);
}

template <typename arena_manager, typename size_type, bool k_compute_stats>
inline best_fit_arena_allocator<arena_manager, size_type, k_compute_stats>::
    move_iterator::move_iterator(block_list_it i_it, block_list_it i_end,
                                 size_type i_delta)
    : it(i_it), end(i_end), delta_offset(i_delta) {}

template <typename arena_manager, typename size_type, bool k_compute_stats>
inline bool best_fit_arena_allocator<
    arena_manager, size_type,
    k_compute_stats>::move_iterator::has_next(size_type& o_handle) {
	if (it != end) {
		o_handle = (*it++).id;
		return true;
	}
	return false;
}

template <typename arena_manager, typename size_type, bool k_compute_stats>
inline void best_fit_arena_allocator<
    arena_manager, size_type,
    k_compute_stats>::move_iterator::modify_offset(size_type& io_offset) {
	io_offset -= delta_offset;
}

} // namespace cppalloc
