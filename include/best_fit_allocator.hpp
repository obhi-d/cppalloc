#pragma once

#include <detail/cppalloc_common.hpp>

namespace cppalloc {

template <typename size_type = std::uint32_t, const bool k_growable = true>
class best_fit_allocator {
public:
	enum { kInvalidOffset = 0xffffffff };
	using address = size_type;

	best_fit_allocator() = default;

	//! Interface
	address allocate(size_type i_size);
	void deallocate(address i_offset, size_type i_size);

	//! Unittst
	bool validate();
	static void unit_test();

private:
	enum { k_null = 0xffffffff };
	struct list_type {
		std::uint32_t first = k_null;
		std::uint32_t last  = k_null;
	};

	struct ordering_type {
		std::uint32_t next = k_null;
		std::uint32_t prev = k_null;
	};

	enum ordering_by : std::uint32_t { e_size, e_offset, k_count };

	struct block_type {
		size_type offset = 0;
		size_type size   = 0;

		ordering_type orderings[k_count];

		inline size_type next_offset() const { return offset + size; }
	};

	using ordering_lists = std::array<list_type, k_count>;

	template <ordering_by i_order>
	inline ordering_type& order(std::uint32_t i_index);

	template <ordering_by i_order>
	inline const ordering_type& get(std::uint32_t i_index) const;

	template <ordering_by i_order>
	void insert(std::uint32_t i_where, std::uint32_t i_what);

	template <ordering_by i_order> void unlink_node(std::uint32_t i_where);

	template <ordering_by i_order> void erase(std::uint32_t i_where);

	static inline bool cmp_by_size(const block_type& i_first,
	                               const block_type& i_sec);

	static inline bool cmp_by_size(const block_type& i_first, size_type i_offset,
	                               size_type i_size);

	static inline bool cmp_by_offset(const block_type& i_first,
	                                 const block_type& i_sec);

	using block_list = std::vector<block_type>;

	// forward iterator for free list
	template <enum ordering_by i_order>
	struct iterator
	    : public std::iterator<std::bidirectional_iterator_tag, block_type> {
		iterator(const iterator& i_other);
		iterator(iterator&& i_other);

		iterator(best_fit_allocator<k_growable, size_type>& iFather);
		iterator(best_fit_allocator<k_growable, size_type>& iFather,
		         std::uint32_t                              iLoc);

		inline iterator& operator=(iterator&& i_other) {
			index        = i_other.index;
			i_other.index = k_null;
			return *this;
		}

		inline iterator& operator=(const iterator& i_other) {
			index = i_other.index;
			return *this;
		}

		inline bool operator==(const iterator& i_other) const {
			return (index == i_other.index) != 0;
		}

		inline bool operator!=(const iterator& i_other) const {
			return (index != i_other.index) != 0;
		}

		inline iterator& operator++() {
			index = owner.get_block(index).orderings[i_order].next;
			return *this;
		}

		inline iterator operator++(int) {
			iterator ret(*this);
			index = owner.get_block(index).orderings[i_order].next;
			return ret;
		}

		inline iterator& operator--() {
			index = owner.get_block(index).orderings[i_order].prev;
			return *this;
		}

		inline iterator operator--(int) {
			iterator ret(*this);
			index = owner.get_block(index).orderings[i_order].prev;
			return ret;
		}

		inline const block_type& operator*() const { return owner.get_block(index); }

		inline const block_type* operator->() const { return &owner.get_block(index); }

		inline block_type& operator*() { return owner.get_block(index); }

		inline block_type* operator->() { return &owner.get_block(index); }

		inline std::uint32_t prev() const {
			// assert(index < owner.size());
			return owner.get_block(index).orderings[i_order].prev;
		}

		inline std::uint32_t next() const {
			// assert(index < owner.size());
			return owner.get_block(*this).orderings[i_order].next;
		}

		inline std::uint32_t get_raw() const { return index; }
		best_fit_allocator<k_growable, size_type>& owner;
		std::uint32_t                                 index = k_null;
	};

	using free_iterator     = iterator<ordering_by::e_size>;
	using OccupiedIterator = iterator<ordering_by::e_offset>;

	inline block_type&       get_block(std::uint32_t iLoc);
	inline const block_type& get_block(std::uint32_t iLoc) const;

	inline free_iterator free_lookup(const free_iterator& i_begin,
	                                 const free_iterator& i_end,
	                                 size_type            i_size);

	inline free_iterator free_lookup(const free_iterator& i_begin,
	                               const free_iterator& i_end, size_type i_offset, size_type i_size);

	template <enum ordering_by i_order> iterator<i_order> begin();
	template <enum ordering_by i_order> iterator<i_order> end();
	template <enum ordering_by i_order>
	iterator<i_order> prev(const iterator<i_order>& i_other);

	size_type alloc_from(const free_iterator& i_it, size_type i_size);

	void reinsert_left(const free_iterator& i_it);

	void reinsert_right(const free_iterator& i_it);

	void erase_entry(std::uint32_t i_it);

	std::uint32_t get_slot() {
		if (firstFreeBlock != 0xffffffff) {
			std::uint32_t offset = firstFreeBlock;
			firstFreeBlock       = blocks[offset].offset;
			return offset;
		} else {
			std::uint32_t offset = static_cast<std::uint32_t>(blocks.size());
			blocks.resize(offset + 1);
			return offset;
		}
	}


private:
	block_list     blocks;
	ordering_lists  ordering_lists;
	std::uint32_t firstFreeBlock = 0xffffffff;
	size_type     totalSize      = 0;
};

template <typename size_type, bool k_growable>
template <ordering_by i_order>
inline ordering_type& best_fit_allocator<size_type, k_growable>::order(
    std::uint32_t i_index) {
	return blocks[i_index].orderings[i_order];
}

template <typename size_type, bool k_growable>
template <ordering_by i_order>
inline const ordering_type& best_fit_allocator<size_type, k_growable>::get(
    std::uint32_t i_index) const {
	return blocks[i_index].orderings[i_order];
}

template <typename size_type, bool k_growable>
template <ordering_by i_order>
inline void best_fit_allocator<size_type, k_growable>::insert(
    std::uint32_t i_where, std::uint32_t i_what) {
	unlink_node<i_order>(i_what);
	if (i_where < blocks.size()) {
		std::uint32_t prev = order<i_order>(i_where).prev;
		if (prev != k_null) {
			order<i_order>(prev).next   = i_what;
			order<i_order>(i_what).prev = prev;
		} else {
			ordering_lists[i_order].first = i_what;
			order<i_order>(i_what).prev   = k_null;
		}
		order<i_order>(i_where).prev = i_what;
		order<i_order>(i_what).next  = i_where;
	} else {
		order<i_order>(i_what).next = k_null;
		order<i_order>(i_what).prev = ordering_lists[i_order].last;
		if (ordering_lists[i_order].last < blocks.size())
			order<i_order>(ordering_lists[i_order].last).next = i_what;
		ordering_lists[i_order].last = i_what;
		if (ordering_lists[i_order].first == k_null)
			ordering_lists[i_order].first = i_what;
	}
}

template <typename size_type, bool k_growable>
template <ordering_by i_order>
inline void best_fit_allocator<size_type, k_growable>::unlink_node(
    std::uint32_t i_where) {
	std::uint32_t prev = order<i_order>(i_where).prev;
	std::uint32_t next = order<i_order>(i_where).next;

	if (prev != k_null)
		order<i_order>(prev).next = next;

	if (next != k_null)
		order<i_order>(next).prev = prev;

	if (ordering_lists[i_order].first == i_where)
		ordering_lists[i_order].first = next;
	if (ordering_lists[i_order].last == i_where)
		ordering_lists[i_order].last = prev;

	order<i_order>(i_where).prev = k_null;
	order<i_order>(i_where).next = k_null;
}

template <typename size_type, bool k_growable>
template <ordering_by i_order>
inline void best_fit_allocator<size_type, k_growable>::erase(
    std::uint32_t i_where) {
	std::uint32_t prev = order<i_order>(i_where).prev;
	std::uint32_t next = order<i_order>(i_where).next;

	if (prev != k_null)
		order<i_order>(prev).next = next;
	else
		ordering_lists[i_order].first = next;

	if (next != k_null)
		order<i_order>(next).prev = prev;
	else
		ordering_lists[i_order].last = prev;

	order<i_order>(i_where).prev = k_null;
	order<i_order>(i_where).next = k_null;
}

template <typename size_type, bool k_growable>
template <>
inline iterator<i_order> best_fit_allocator<size_type, k_growable>::begin() {
	return iterator<i_order>(*this, ordering_lists[i_order].first);
}

template <typename size_type, bool k_growable>
template <>
inline iterator<i_order> best_fit_allocator<size_type, k_growable>::end() {
	return iterator<i_order>(*this);
}

template <typename size_type, bool k_growable>
template <>
inline iterator<i_order> best_fit_allocator<size_type, k_growable>::prev(
    const iterator<i_order>& i_other) {
	return i_other.get_raw() == k_null
	           ? iterator<i_order>(*this, ordering_lists[i_order].last)
	           : iterator<i_order>(*this, i_other.prev());
}

template <typename size_type, bool k_growable>
inline bool best_fit_allocator<size_type, k_growable>::cmp_by_size(
    const block_type& i_first, const block_type& i_sec) {
	return (i_first.size < i_sec.size) ||
	       (i_first.size == i_sec.size && i_first.offset < i_sec.offset);
}

template <typename size_type, bool k_growable>
inline bool best_fit_allocator<size_type, k_growable>::cmp_by_size(
    const block_type& i_first, size_type i_offset, size_type i_size) {
	return (i_first.size < i_size) ||
	       (i_first.size == i_size && i_first.offset < i_offset);
}

template <typename size_type, bool k_growable>
inline bool best_fit_allocator<size_type, k_growable>::cmp_by_offset(
    const block_type& i_first, const block_type& i_sec) {
	return i_first.offset < i_sec.offset;
}

template <typename size_type, bool k_growable>
inline block_type& best_fit_allocator<size_type, k_growable>::get_block(
    std::uint32_t iLoc) {
	return blocks[iLoc];
}

template <typename size_type, bool k_growable>
inline const block_type& best_fit_allocator<size_type, k_growable>::get_block(
    std::uint32_t iLoc) const {
	return blocks[iLoc];
}

template <typename size_type, bool k_growable>
inline free_iterator best_fit_allocator<size_type, k_growable>::free_lookup(
    const free_iterator& i_begin, const free_iterator& i_end,
    size_type i_size) {
	return std::lower_bound(
	    i_begin, i_end, i_size,
	    [this](const block_type& i_it, size_type i_size) -> bool {
		    return i_it.size < i_size;
	    });
}

template <typename size_type, bool k_growable>
inline free_iterator best_fit_allocator<size_type, k_growable>::free_lookup(
    const free_iterator& i_begin, const free_iterator& i_end,
    size_type i_offset, size_type i_size) {
	return std::lower_bound(
	    i_begin, i_end, i_size,
	    [this, i_offset](const block_type& i_it, size_type i_size) -> bool {
		    return cmp_by_size(i_it, i_offset, i_size);
	    });
}

template <typename size_type, bool k_growable>
inline size_type best_fit_allocator<size_type, k_growable>::alloc_from(
    const free_iterator& i_it, size_type i_size) {
	std::uint32_t block  = i_it.get_raw();
	size_type     offset = blocks[block].offset;
	blocks[block].offset += i_size;
	blocks[block].size -= i_size;
	if (blocks[block].size > 0) {
		reinsert_left(i_it);
	} else {
		erase_entry(i_it.get_raw());
	}
	return offset;
}

template <typename size_type, bool k_growable>
inline void best_fit_allocator<size_type, k_growable>::reinsert_left(
    const free_iterator& i_it) {
	free_iterator begin = begin<ordering_by::e_size>();
	free_iterator end(i_it);
	auto          it = free_lookup(begin, end, i_it->size);
	if (it != i_it)
		insert<ordering_by::e_size>(it.get_raw(), i_it.get_raw());
}

template <typename size_type, bool k_growable>
inline void best_fit_allocator<size_type, k_growable>::reinsert_right(
    const free_iterator& i_it) {
	free_iterator begin = i_it;
	free_iterator end(*this);
	auto          it = free_lookup(begin, end, (*i_it).size);
	if (it != i_it)
		insert<ordering_by::e_size>(it.get_raw(), i_it.get_raw());
}

template <typename size_type, bool k_growable>
inline void best_fit_allocator<size_type, k_growable>::erase_entry(
    std::uint32_t i_it) {
	erase<ordering_by::e_size>(i_it);
	erase<ordering_by::e_offset>(i_it);
	blocks[i_it].offset = firstFreeBlock;
	firstFreeBlock      = i_it;
}

template <typename size_type, bool k_growable>
inline address best_fit_allocator<size_type, k_growable>::allocate(
    size_type i_size) {
	auto end   = end<ordering_by::e_size>();
	auto found = free_lookup(begin<ordering_by::e_size>(), end, i_size);
	if (found == end) {
		if (k_growable) {
			size_type offset = totalSize;
			totalSize += i_size;
			return offset;
		} else
			return kInvalidOffset;
	}
	return alloc_from(found, i_size);
}

template <typename size_type, bool k_growable>
inline void best_fit_allocator<size_type, k_growable>::deallocate(
    address i_offset, size_type i_size) {
	auto begin = begin<ordering_by::e_offset>();
	auto end   = end<ordering_by::e_offset>();
	auto node =
	    std::lower_bound(begin, end, i_offset,
	                     [](const block_type& block, size_type offset) -> bool {
		                     return block.offset < offset;
	                     });
	size_type next_offset = i_offset + i_size;
	if (next_offset > totalSize)
		totalSize = next_offset;
	decltype(node) prev_free(*this);
	if (node != begin && (prev_free = prev(node))->next_offset() == i_offset) {
		if (node != end && next_offset == node->offset) {
			// merge prev_free + this + node
			free_iterator prev_free_list_it(*this, prev_free.get_raw());
			prev_free->size += i_size;
			prev_free->size += node->size;
			erase_entry(node.get_raw());
			reinsert_right(prev_free_list_it);
		} else {
			// merge prev_free + this
			free_iterator prev_free_list_it(*this, prev_free.get_raw());
			prev_free->size += i_size;
			reinsert_right(prev_free_list_it);
		}
	} else if (node != end && next_offset == node->offset) {
		// merge this + node
		node->offset = i_offset;
		node->size += i_size;
		free_iterator nodeFLI(*this, node.get_raw());
		reinsert_right(nodeFLI);
	} else {
		// new node
		auto          begin      = begin<ordering_by::e_size>();
		auto          end        = end<ordering_by::e_size>();
		std::uint32_t slot       = get_slot();
		block_type&   next_block = blocks[slot];
		next_block.offset        = i_offset;
		next_block.size          = i_size;

		auto it = free_lookup(begin, end, i_offset, i_size);
		insert<ordering_by::e_offset>(node.get_raw(), slot);
		insert<ordering_by::e_size>(it.get_raw(), slot);
	}
}

template <typename size_type, bool k_growable>
inline bool best_fit_allocator<size_type, k_growable>::validate() {
	auto ordered = [](const std::uint32_t first, const block_type& second,
	                  ordering_by i_order) -> bool {
		return (second.orderings[i_order].prev == first);
	};
	auto sizeIt  = begin<ordering_by::e_size>();
	auto sizeEnd = prev(end<ordering_by::e_size>());
	while (sizeIt != sizeEnd) {
		std::uint32_t     raw = sizeIt.get_raw();
		const block_type& it  = *sizeIt;
		const block_type& nx  = *(++sizeIt);

		if (!ordered(raw, nx, ordering_by::e_size)) {
			assert(false);
			return false;
		}

		size_type size = it.size;
		size_type next = nx.size;

		if (size > next) {
			assert(false);
			return false;
		}
	}

	auto offsetIt  = begin<ordering_by::e_offset>();
	auto offsetEnd = prev(end<ordering_by::e_offset>());
	while (offsetIt != offsetEnd) {
		std::uint32_t     raw    = offsetIt.get_raw();
		const block_type& it     = *offsetIt;
		const block_type& nx     = *(++offsetIt);
		size_type         offset = it.offset;
		size_type         next   = nx.offset;

		if (!ordered(raw, nx, ordering_by::e_offset)) {
			assert(false);
			return false;
		}

		if (offset > next) {
			assert(false);
			return false;
		}
	}
	return true;
}

template <typename size_type, bool k_growable>
inline void best_fit_allocator<size_type, k_growable>::unit_test() {
	best_fit_allocator<k_growable, size_type>    allocator;
	std::minstd_rand                             gen;
	std::bernoulli_distribution                  allocOrFree(0.7);
	std::uniform_int_distribution<std::uint32_t> allocGen(1, 100000);

	struct Record {
		size_type offset, size;
	};
	std::vector<Record> allocated;
	for (std::uint32_t allocs = 0; allocs < 10000; ++allocs) {
		if (allocOrFree(gen)) {
			Record r;
			r.size   = allocGen(gen);
			r.offset = allocator.allocate(r.size);
			if (r.offset != k_null)
				allocated.push_back(r);
			assert(allocator.validate());
		} else {
			std::uniform_int_distribution<std::uint32_t> choose(0,
			                                                    allocated.size() - 1);
			std::uint32_t                                chosen = choose(gen);
			allocator.Free(allocated[chosen].offset, allocated[chosen].size);
			allocated.erase(allocated.begin() + chosen);
			assert(allocator.validate());
		}
	}
}

template <typename size_type, bool k_growable>
inline best_fit_allocator<size_type, k_growable>::iterator<>::iterator(
    const iterator& i_other)
    : owner(i_other.owner), index(i_other.index) {}

template <typename size_type, bool k_growable>
inline best_fit_allocator<size_type, k_growable>::iterator<>::iterator(
    iterator&& i_other)
    : owner(i_other.owner), index(i_other.index) {
	i_other.index = k_null;
}

template <typename size_type, bool k_growable>
inline best_fit_allocator<size_type, k_growable>::iterator<>::iterator(
    best_fit_allocator<k_growable, size_type>& iFather)
    : owner(iFather), index(k_null) {}

template <typename size_type, bool k_growable>
inline best_fit_allocator<size_type, k_growable>::iterator<>::iterator(
    best_fit_allocator<k_growable, size_type>& iFather, std::uint32_t iLoc)
    : owner(iFather), index(iLoc) {}

} // namespace cppalloc