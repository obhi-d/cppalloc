#pragma once

#include <detail/cppalloc_common.hpp>

namespace cppalloc
{

namespace detail
{
enum : std::uint32_t
{
  k_null_32 = std::numeric_limits<std::uint32_t>::max()
};
enum ordering_by : std::uint32_t
{
  e_size,
  e_offset,
  k_count
};
struct ordering_type
{
  std::uint32_t next = k_null_32;
  std::uint32_t prev = k_null_32;
};

} // namespace detail
struct best_fit_allocator_tag
{
};

template <typename size_type = std::uint32_t, const bool k_growable = true, bool k_compute_stats = false>
class best_fit_allocator : public detail::statistics<best_fit_allocator_tag, k_compute_stats>
{
  using statistics = detail::statistics<best_fit_allocator_tag, k_compute_stats>;

public:
  enum
  {
    k_invalid_offset = std::numeric_limits<size_type>::max(),
  };
  using tag     = best_fit_allocator_tag;
  using address = size_type;

  best_fit_allocator()
  {
    statistics::report_new_arena();
  };

  explicit best_fit_allocator(size_type i_fixed_sz)
  {
    statistics::report_new_arena();

    std::uint32_t slot       = get_slot();
    block_type&   next_block = blocks[slot];
    next_block.offset        = 0;
    next_block.size          = i_fixed_sz;

    insert<ordering_by::e_offset>(end<ordering_by::e_offset>().get_raw(), slot);
    insert<ordering_by::e_size>(end<ordering_by::e_offset>().get_raw(), slot);
  }

  //! Interface
  address allocate(size_type i_size, size_type i_alignment = 0);
  void    deallocate(address i_offset, size_type i_size, size_type i_alignment = 0);

  //! Unittst
  bool validate();

private:
  using ordering_by   = detail::ordering_by;
  using ordering_type = detail::ordering_type;

  enum : std::uint32_t
  {
    k_null = std::numeric_limits<std::uint32_t>::max()
  };
  struct list_type
  {
    std::uint32_t first = k_null;
    std::uint32_t last  = k_null;
  };

  struct block_type
  {
    size_type offset = 0;
    size_type size   = 0;

    ordering_type orderings[ordering_by::k_count];

    inline size_type next_offset() const
    {
      return offset + size;
    }
  };

  using ordering_lists_array = std::array<list_type, ordering_by::k_count>;

  template <ordering_by i_order>
  inline ordering_type& order(std::uint32_t i_index);

  template <ordering_by i_order>
  [[nodiscard]] inline const ordering_type& get(std::uint32_t i_index) const;

  template <ordering_by i_order>
  void insert(std::uint32_t i_where, std::uint32_t i_what);

  template <ordering_by i_order>
  void unlink_node(std::uint32_t i_where);

  template <ordering_by i_order>
  void erase(std::uint32_t i_where);

  static inline bool cmp_by_size(const block_type& i_first, const block_type& i_sec);

  static inline bool cmp_by_size(const block_type& i_first, size_type i_offset, size_type i_size);

  static inline bool cmp_by_offset(const block_type& i_first, const block_type& i_sec);

  using block_list = std::vector<block_type>;

  // forward iterator for free list
  template <ordering_by i_order>
  struct iterator
  {

    using iterator_category = std::bidirectional_iterator_tag;
    using value_type        = block_type;
    using difference_type   = std::ptrdiff_t;
    using pointer           = block_type*;
    using reference         = block_type&;

    iterator(const iterator& i_other);
    iterator(iterator&& i_other) noexcept;

    explicit iterator(best_fit_allocator<size_type, k_growable, k_compute_stats>& i_other);
    iterator(best_fit_allocator<size_type, k_growable, k_compute_stats>& i_other, std::uint32_t i_loc);

    inline iterator& operator=(iterator&& i_other) noexcept
    {
      index         = i_other.index;
      i_other.index = k_null;
      return *this;
    }

    inline iterator& operator=(const iterator& i_other)
    {
      index = i_other.index;
      return *this;
    }

    inline bool operator==(const iterator& i_other) const
    {
      return (index == i_other.index) != 0;
    }

    inline bool operator!=(const iterator& i_other) const
    {
      return (index != i_other.index) != 0;
    }

    inline iterator& operator++()
    {
      index = owner.get_block(index).orderings[i_order].next;
      return *this;
    }

    inline iterator operator++(int)
    {
      iterator ret(*this);
      index = owner.get_block(index).orderings[i_order].next;
      return ret;
    }

    inline iterator& operator--()
    {
      index = owner.get_block(index).orderings[i_order].prev;
      return *this;
    }

    inline iterator operator--(int)
    {
      iterator ret(*this);
      index = owner.get_block(index).orderings[i_order].prev;
      return ret;
    }

    inline const block_type& operator*() const
    {
      return owner.get_block(index);
    }

    inline const block_type* operator->() const
    {
      return &owner.get_block(index);
    }

    inline block_type& operator*()
    {
      return owner.get_block(index);
    }

    inline block_type* operator->()
    {
      return &owner.get_block(index);
    }

    [[nodiscard]] inline std::uint32_t prev() const
    {
      // assert(index < owner.size());
      return owner.get_block(index).orderings[i_order].prev;
    }

    [[nodiscard]] inline std::uint32_t next() const
    {
      // assert(index < owner.size());
      return owner.get_block(index).orderings[i_order].next;
    }

    [[nodiscard]] inline std::uint32_t get_raw() const
    {
      return index;
    }
    best_fit_allocator<size_type, k_growable, k_compute_stats>& owner;
    std::uint32_t                                               index = k_null;
  };

  using free_iterator     = iterator<ordering_by::e_size>;
  using occupied_iterator = iterator<ordering_by::e_offset>;

  inline block_type&                        get_block(std::uint32_t i_loc);
  [[maybe_unused]] inline const block_type& get_block(std::uint32_t i_loc) const;

  inline free_iterator free_lookup(const free_iterator& i_begin, const free_iterator& i_end, size_type i_size);

  inline free_iterator free_lookup(const free_iterator& i_begin, const free_iterator& i_end, size_type i_offset,
                                   size_type i_size);

  template <ordering_by i_order>
  iterator<i_order> begin();
  template <ordering_by i_order>
  iterator<i_order> end();
  template <ordering_by i_order>
  iterator<i_order> back();
  template <ordering_by i_order>
  iterator<i_order> prev(const iterator<i_order>& i_other);
  template <ordering_by i_order>
  iterator<i_order> next(const iterator<i_order>& i_other);

  size_type alloc_from(const free_iterator& i_it, size_type i_size);

  void reinsert_left(const free_iterator& i_it);

  void reinsert_right(const free_iterator& i_it);

  void erase_entry(std::uint32_t i_it);

  bool is_free(std::uint32_t i_it) const;

  std::uint32_t get_slot()
  {
    if (first_free_block != 0xffffffff)
    {
      std::uint32_t offset = first_free_block;
      first_free_block     = blocks[offset].offset;
      return offset;
    }
    else
    {
      auto offset = static_cast<std::uint32_t>(blocks.size());
      blocks.resize(offset + 1);
      return offset;
    }
  }

private:
  block_list           blocks;
  ordering_lists_array ordering_lists;
  std::uint32_t        first_free_block = 0xffffffff;
  size_type            total_size       = 0;
};

template <typename size_type, bool k_growable, bool k_compute_stats>
template <detail::ordering_by i_order>
inline detail::ordering_type& best_fit_allocator<size_type, k_growable, k_compute_stats>::order(std::uint32_t i_index)
{
  return blocks[i_index].orderings[i_order];
}

template <typename size_type, bool k_growable, bool k_compute_stats>
template <detail::ordering_by i_order>
inline const detail::ordering_type& best_fit_allocator<size_type, k_growable, k_compute_stats>::get(
    std::uint32_t i_index) const
{
  return blocks[i_index].orderings[i_order];
}

template <typename size_type, bool k_growable, bool k_compute_stats>
template <detail::ordering_by i_order>
inline void best_fit_allocator<size_type, k_growable, k_compute_stats>::insert(std::uint32_t i_where,
                                                                               std::uint32_t i_what)
{
  unlink_node<i_order>(i_what);
  if (i_where < blocks.size())
  {
    std::uint32_t prev = order<i_order>(i_where).prev;
    if (prev != k_null)
    {
      order<i_order>(prev).next   = i_what;
      order<i_order>(i_what).prev = prev;
    }
    else
    {
      ordering_lists[i_order].first = i_what;
      order<i_order>(i_what).prev   = k_null;
    }
    order<i_order>(i_where).prev = i_what;
    order<i_order>(i_what).next  = i_where;
  }
  else
  {
    order<i_order>(i_what).next = k_null;
    order<i_order>(i_what).prev = ordering_lists[i_order].last;
    if (ordering_lists[i_order].last < blocks.size())
      order<i_order>(ordering_lists[i_order].last).next = i_what;
    ordering_lists[i_order].last = i_what;
    if (ordering_lists[i_order].first == k_null)
      ordering_lists[i_order].first = i_what;
  }
}

template <typename size_type, bool k_growable, bool k_compute_stats>
template <detail::ordering_by i_order>
inline void best_fit_allocator<size_type, k_growable, k_compute_stats>::unlink_node(std::uint32_t i_where)
{
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

template <typename size_type, bool k_growable, bool k_compute_stats>
template <detail::ordering_by i_order>
inline void best_fit_allocator<size_type, k_growable, k_compute_stats>::erase(std::uint32_t i_where)
{
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

template <typename size_type, bool k_growable, bool k_compute_stats>
template <detail::ordering_by i_order>
inline typename best_fit_allocator<size_type, k_growable, k_compute_stats>::template iterator<i_order>
best_fit_allocator<size_type, k_growable, k_compute_stats>::begin()
{
  return iterator<i_order>(*this, ordering_lists[i_order].first);
}

template <typename size_type, bool k_growable, bool k_compute_stats>
template <detail::ordering_by i_order>
inline typename best_fit_allocator<size_type, k_growable, k_compute_stats>::template iterator<i_order>
best_fit_allocator<size_type, k_growable, k_compute_stats>::end()
{
  return iterator<i_order>(*this);
}

template <typename size_type, bool k_growable, bool k_compute_stats>
template <detail::ordering_by i_order>
inline typename best_fit_allocator<size_type, k_growable, k_compute_stats>::template iterator<i_order>
best_fit_allocator<size_type, k_growable, k_compute_stats>::back()
{
  return iterator<i_order>(*this, ordering_lists[i_order].last);
}

template <typename size_type, bool k_growable, bool k_compute_stats>
template <detail::ordering_by i_order>
inline typename best_fit_allocator<size_type, k_growable, k_compute_stats>::template iterator<i_order>
best_fit_allocator<size_type, k_growable, k_compute_stats>::prev(const iterator<i_order>& i_other)
{
  return i_other.get_raw() == k_null ? iterator<i_order>(*this, ordering_lists[i_order].last)
                                     : iterator<i_order>(*this, i_other.prev());
}

template <typename size_type, bool k_growable, bool k_compute_stats>
template <detail::ordering_by i_order>
inline typename best_fit_allocator<size_type, k_growable, k_compute_stats>::template iterator<i_order>
best_fit_allocator<size_type, k_growable, k_compute_stats>::next(const iterator<i_order>& i_other)
{
  return i_other.get_raw() == k_null ? iterator<i_order>(*this, ordering_lists[i_order].first)
                                     : iterator<i_order>(*this, i_other.next());
}

template <typename size_type, bool k_growable, bool k_compute_stats>
inline bool best_fit_allocator<size_type, k_growable, k_compute_stats>::cmp_by_size(const block_type& i_first,
                                                                                    const block_type& i_sec)
{
  return (i_first.size < i_sec.size) || (i_first.size == i_sec.size && i_first.offset < i_sec.offset);
}

template <typename size_type, bool k_growable, bool k_compute_stats>
inline bool best_fit_allocator<size_type, k_growable, k_compute_stats>::cmp_by_size(const block_type& i_first,
                                                                                    size_type         i_offset,
                                                                                    size_type         i_size)
{
  return (i_first.size < i_size) || (i_first.size == i_size && i_first.offset < i_offset);
}

template <typename size_type, bool k_growable, bool k_compute_stats>
inline bool best_fit_allocator<size_type, k_growable, k_compute_stats>::cmp_by_offset(const block_type& i_first,
                                                                                      const block_type& i_sec)
{
  return i_first.offset < i_sec.offset;
}

template <typename size_type, bool k_growable, bool k_compute_stats>
inline typename best_fit_allocator<size_type, k_growable, k_compute_stats>::block_type& best_fit_allocator<
    size_type, k_growable, k_compute_stats>::get_block(std::uint32_t i_loc)
{
  return blocks[i_loc];
}

template <typename size_type, bool k_growable, bool k_compute_stats>
[[maybe_unused]] inline typename best_fit_allocator<size_type, k_growable, k_compute_stats>::block_type const&
best_fit_allocator<size_type, k_growable, k_compute_stats>::get_block(std::uint32_t i_loc) const
{
  return blocks[i_loc];
}

template <typename size_type, bool k_growable, bool k_compute_stats>
inline typename best_fit_allocator<size_type, k_growable, k_compute_stats>::free_iterator best_fit_allocator<
    size_type, k_growable, k_compute_stats>::free_lookup(const free_iterator& i_begin, const free_iterator& i_end,
                                                         size_type i_size)
{
  return std::lower_bound(i_begin, i_end, i_size, [this](const block_type& i_it, size_type i_size) -> bool {
    return i_it.size < i_size;
  });
}

template <typename size_type, bool k_growable, bool k_compute_stats>
inline typename best_fit_allocator<size_type, k_growable, k_compute_stats>::free_iterator best_fit_allocator<
    size_type, k_growable, k_compute_stats>::free_lookup(const free_iterator& i_begin, const free_iterator& i_end,
                                                         size_type i_offset, size_type i_size)
{
  return std::lower_bound(i_begin, i_end, i_size, [this, i_offset](const block_type& i_it, size_type i_size) -> bool {
    return cmp_by_size(i_it, i_offset, i_size);
  });
}

template <typename size_type, bool k_growable, bool k_compute_stats>
inline size_type best_fit_allocator<size_type, k_growable, k_compute_stats>::alloc_from(const free_iterator& i_it,
                                                                                        size_type            i_size)
{
  std::uint32_t block  = i_it.get_raw();
  size_type     offset = blocks[block].offset;
  blocks[block].offset += i_size;
  blocks[block].size -= i_size;

  auto slot           = get_slot();
  blocks[slot].offset = offset;
  blocks[slot].size   = i_size;
  insert<ordering_by::e_offset>(block, slot);

  if (blocks[block].size > 0)
  {
    reinsert_left(i_it);
  }
  else
  {
    erase_entry(block);
  }

  return offset;
}

template <typename size_type, bool k_growable, bool k_compute_stats>
inline void best_fit_allocator<size_type, k_growable, k_compute_stats>::reinsert_left(const free_iterator& i_it)
{
  free_iterator begin_it = begin<ordering_by::e_size>();
  free_iterator end(i_it);
  auto          it = free_lookup(begin_it, end, i_it->size);
  if (it != i_it)
    insert<ordering_by::e_size>(it.get_raw(), i_it.get_raw());
}

template <typename size_type, bool k_growable, bool k_compute_stats>
inline void best_fit_allocator<size_type, k_growable, k_compute_stats>::reinsert_right(const free_iterator& i_it)
{
  free_iterator begin = i_it;
  free_iterator end(*this);
  auto          it = free_lookup(begin, end, (*i_it).size);
  if (it != i_it)
    insert<ordering_by::e_size>(it.get_raw(), i_it.get_raw());
}

template <typename size_type, bool k_growable, bool k_compute_stats>
inline bool best_fit_allocator<size_type, k_growable, k_compute_stats>::is_free(std::uint32_t i_it) const
{
  if (blocks[i_it].orderings[ordering_by::e_size].next != k_null ||
      blocks[i_it].orderings[ordering_by::e_size].prev != k_null || i_it == ordering_lists[ordering_by::e_size].first)
    return true;
  return false;
}

template <typename size_type, bool k_growable, bool k_compute_stats>
inline void best_fit_allocator<size_type, k_growable, k_compute_stats>::erase_entry(std::uint32_t i_it)
{
  erase<ordering_by::e_size>(i_it);
  erase<ordering_by::e_offset>(i_it);
  blocks[i_it].offset = first_free_block;
  first_free_block    = i_it;
}

template <typename size_type, bool k_growable, bool k_compute_stats>
inline typename best_fit_allocator<size_type, k_growable, k_compute_stats>::address best_fit_allocator<
    size_type, k_growable, k_compute_stats>::allocate(size_type i_size, size_type i_alignment)
{

  assert(i_size > 0);
  auto fixup = i_alignment - 1;
  if (i_alignment)
    i_size += fixup;
  size_type offset;
  auto      measure = statistics::report_allocate(i_size);

  auto end_it = end<ordering_by::e_size>();
  auto found  = free_lookup(begin<ordering_by::e_size>(), end_it, i_size);
  if (found == end_it)
  {
    if (k_growable)
    {
      offset = total_size;
      total_size += i_size;

      std::uint32_t slot       = get_slot();
      block_type&   next_block = blocks[slot];
      next_block.offset        = offset;
      next_block.size          = i_size;

      insert<ordering_by::e_offset>(end<ordering_by::e_offset>().get_raw(), slot);
    }
    else
      return k_invalid_offset;
  }
  else
    offset = alloc_from(found, i_size);
  return i_alignment ? ((offset + fixup) & ~fixup) : offset;
}

template <typename size_type, bool k_growable, bool k_compute_stats>
inline void best_fit_allocator<size_type, k_growable, k_compute_stats>::deallocate(address i_offset, size_type i_size,
                                                                                   size_type i_alignment)
{

  auto measure = statistics::report_deallocate(i_size);

  auto begin_it = begin<ordering_by::e_offset>();
  auto back_it  = back<ordering_by::e_offset>();
  auto node     = std::lower_bound(begin_it, end<ordering_by::e_offset>(), i_offset,
                               [](const block_type& block, size_type offset) -> bool {
                                 return block.offset < offset;
                               });
  assert(node != end<ordering_by::e_offset>());
  // fixup offset based on alignment
  if (i_alignment)
  {
    i_offset = (*node).offset;
    i_size += i_alignment - 1;
  }

  size_type next_offset = i_offset + i_size;
  if (next_offset > total_size)
    total_size = next_offset;
  decltype(node) prev_free(*this);
  decltype(node) next_free(*this);
  if (node != begin_it && is_free((prev_free = prev(node)).get_raw()))
  {
    if (node != back_it && is_free((next_free = next(node)).get_raw()))
    {
      // merge prev_free + this + node
      free_iterator prev_free_list_it(*this, prev_free.get_raw());
      prev_free->size += i_size;
      prev_free->size += next_free->size;
      erase<ordering_by::e_offset>(node.get_raw());
      erase_entry(next_free.get_raw());
      reinsert_right(prev_free_list_it);
    }
    else
    {
      // merge prev_free + this
      free_iterator prev_free_list_it(*this, prev_free.get_raw());
      prev_free->size += i_size;
      erase<ordering_by::e_offset>(node.get_raw());
      reinsert_right(prev_free_list_it);
    }
  }
  else if (node != back_it && is_free((next_free = next(node)).get_raw()))
  {
    // merge this + node
    next_free->offset = i_offset;
    next_free->size += i_size;
    erase<ordering_by::e_offset>(node.get_raw());
    free_iterator node_fli(*this, next_free.get_raw());
    reinsert_right(node_fli);
  }
  else
  {
    // new node
    auto begin_it = begin<ordering_by::e_size>();
    auto end_it   = end<ordering_by::e_size>();

    auto it = free_lookup(begin_it, end_it, i_offset, i_size);
    insert<ordering_by::e_size>(it.get_raw(), node.get_raw());
  }
}

template <typename size_type, bool k_growable, bool k_compute_stats>
inline bool best_fit_allocator<size_type, k_growable, k_compute_stats>::validate()
{
  auto ordered = [](const std::uint32_t first, const block_type& second, ordering_by i_order) -> bool {
    return (second.orderings[i_order].prev == first);
  };
  auto size_iter = begin<ordering_by::e_size>();
  auto size_end  = prev(end<ordering_by::e_size>());
  while (size_iter != size_end)
  {
    std::uint32_t     raw = size_iter.get_raw();
    const block_type& it  = *size_iter;
    const block_type& nx  = *(++size_iter);

    if (!ordered(raw, nx, ordering_by::e_size))
    {
      assert(false);
      return false;
    }

    size_type size = it.size;
    size_type next = nx.size;

    if (size > next)
    {
      assert(false);
      return false;
    }
  }

  auto offset_it  = begin<ordering_by::e_offset>();
  auto offset_end = prev(end<ordering_by::e_offset>());
  while (offset_it != offset_end)
  {
    std::uint32_t     raw    = offset_it.get_raw();
    const block_type& it     = *offset_it;
    const block_type& nx     = *(++offset_it);
    size_type         offset = it.offset;
    size_type         next   = nx.offset;

    if (!ordered(raw, nx, ordering_by::e_offset))
    {
      assert(false);
      return false;
    }

    if (offset > next)
    {
      assert(false);
      return false;
    }
  }
  return true;
}
template <typename size_type, bool k_growable, bool k_compute_stats>
template <detail::ordering_by i_order>
inline best_fit_allocator<size_type, k_growable, k_compute_stats>::iterator<i_order>::iterator(
    const iterator<i_order>& i_other)
    : owner(i_other.owner), index(i_other.index)
{
}

template <typename size_type, bool k_growable, bool k_compute_stats>
template <detail::ordering_by i_order>
inline best_fit_allocator<size_type, k_growable, k_compute_stats>::iterator<i_order>::iterator(
    iterator&& i_other) noexcept
    : owner(i_other.owner), index(i_other.index)
{
  i_other.index = k_null;
}

template <typename size_type, bool k_growable, bool k_compute_stats>
template <detail::ordering_by i_order>
inline best_fit_allocator<size_type, k_growable, k_compute_stats>::iterator<i_order>::iterator(
    best_fit_allocator<size_type, k_growable, k_compute_stats>& i_other)
    : owner(i_other), index(k_null)
{
}

template <typename size_type, bool k_growable, bool k_compute_stats>
template <detail::ordering_by i_order>
inline best_fit_allocator<size_type, k_growable, k_compute_stats>::iterator<i_order>::iterator(
    best_fit_allocator<size_type, k_growable, k_compute_stats>& i_other, std::uint32_t i_loc)
    : owner(i_other), index(i_loc)
{
}

} // namespace cppalloc