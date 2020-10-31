#include <detail/cppalloc_common.hpp>

namespace cppalloc::detail
{
template <class T>
concept is_not_const = !std::is_const_v<T>;

struct list_node
{
  std::uint32_t next = k_null_32;
  std::uint32_t prev = k_null_32;
};

template <typename Accessor, typename Container>
struct vlist
{
  std::uint32_t first = k_null_32;
  std::uint32_t last  = k_null_32;

  template <typename ContainerTy>
  struct iterator_t
  {
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type        = typename Accessor::value_type;
    using difference_type   = std::ptrdiff_t;
    using pointer           = value_type*;
    using reference         = value_type&;

    iterator(const iterator& i_other) : owner(i_other.owner), index(i_other.index) {}
    iterator(iterator&& i_other) noexcept : owner(i_other.owner), index(i_other.index)
    {
      i_other.index = k_null_32;
    }

    explicit iterator(ContainerTy& i_owner) : owner(i_owner), index(k_null_32) {}
    iterator(ContainerTy& i_owner, std::uint32_t start) : owner(i_owner), index(start) {}

    inline iterator& operator=(iterator&& i_other) noexcept
    {
      index         = i_other.index;
      i_other.index = k_null_32;
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
      index = Accessor::node(owner, index).next;
      return *this;
    }

    inline iterator operator++(int)
    {
      iterator ret(*this);
      index = Accessor::node(owner, index).next;
      return ret;
    }

    inline iterator& operator--()
    {
      index = Accessor::node(owner, index).prev;
      return *this;
    }

    inline iterator operator--(int)
    {
      iterator ret(*this);
      index = Accessor::node(owner, index).prev;
      return ret;
    }

    inline const block_type& operator*() const
    {
      return Accessor::get(owner, index);
    }

    inline const block_type* operator->() const
    {
      return &Accessor::get(owner, index);
    }

    inline block_type& operator*() requires is_not_const<ContainerTy>
    {
      return Accessor::get(owner, index);
    }

    inline block_type* operator->() requires is_not_const<ContainerTy>
    {
      return &Accessor::get(owner, index);
    }

    [[nodiscard]] inline std::uint32_t prev() const
    {
      // assert(index < owner.size());
      return Accessor::node(owner, index).prev;
    }

    [[nodiscard]] inline std::uint32_t next() const
    {
      // assert(index < owner.size());
      return Accessor::node(owner, index).next;
    }

    [[nodiscard]] inline std::uint32_t value() const
    {
      return index;
    }

    ContainerTy&  owner;
    std::uint32_t index = k_null_32;
  };

  using iterator       = iterator_t<Container>;
  using const_iterator = iterator_t<Container const>;

  inline std::uint32_t begin() const
  {
    return first;
  }

  inline constexpr std::uint32_t end() const
  {
    return k_null_32;
  }

  inline const_iterator begin(Container const& cont) const
  {
    return const_iterator(*this, first);
  }

  inline const_iterator end(Container const& cont) const
  {
    return const_iterator(*this);
  }

  inline iterator begin(Container& cont)
  {
    return iterator(*this, first);
  }

  inline iterator end(Container& cont)
  {
    return iterator(*this);
  }

  inline std::uint32_t front() const
  {
    return first;
  }

  inline std::uint32_t back() const
  {
    return last;
  }

  inline void insert(Container& cont, std::uint32_t loc, std::uint32_t node)
  {
    // end?
    if (loc == k_null_32)
    {
      if (last != k_null_32)
        Accessor::node(cont, last).next = node;
      if (first == k_null_32)
        first = node;
      Accessor::node(cont, node).prev = last;
      last                            = node;
    }
    else
    {
      auto& l_node = Accessor::node(cont, node);
      auto& l_loc  = Accessor::node(cont, loc);

      if (l_loc.prev != k_null_32)
      {
        Accessor::node(cont, l_loc.prev).next = node;
        l_node.prev                           = l_loc.prev;
      }
      else
      {
        first       = node;
        l_node.prev = k_null_32;
      }

      l_loc.prev  = node;
      l_node.next = loc;
    }
  }

  inline void erase(Container& cont, std::uint32_t node)
  {
    auto& l_node = Accessor::node(cont, node);

    std::uint32_t prev = order<i_order>(i_where).prev;
    std::uint32_t next = order<i_order>(i_where).next;

    if (l_node.prev != k_null)
      Accessor::node(cont, l_node.prev).next = l_node.next;
    else
      first = l_node.next;

    if (l_node.next != k_null)
      Accessor::node(cont, l_node.next).prev = l_node.prev;
    else
      last = l_node.prev;

    l_node.prev = k_null_32;
    l_node.next = k_null_32;
  }
};

} // namespace cppalloc::detail