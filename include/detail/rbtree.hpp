#pragma once

#include <detail/cppalloc_common.hpp>

namespace cppalloc::detail
{
//
struct tree_node
{
  std::uint32_t parent = k_null_32;
  std::uint32_t left   = k_null_32;
  std::uint32_t right  = k_null_32;
};

struct accessor_adapter
{
  using value_type = int;
  struct node_type
  {
    value_type value;
    tree_node  node;
  };

  using container = std::vector<value_type>;

  node_type const& node(container const&, std::uint32_t);
  node_type&       node(container&, std::uint32_t);
  tree_node const& links(value_type const&);
  tree_node&       links(value_type&);
  int              value(value_type const&);
  bool             is_set(node_type const&);
  bool             set_flag(node_type&);
  bool             unset_flag(node_type&);
};

template <typename Accessor>
class rbtree
{

private:
  using value_type = typename Accessor::value_type;
  using node_type  = typename Accessor::node_type;
  using container  = typename Accessor::container;

  static inline bool is_set(node_type const& node)
  {
    return Accessor::is_set(node);
  }

  static inline std::uint32_t set_parent(container& cont, std::uint32_t node, std::uint32_t parent)
  {
    tree_node& lnk_ref = Accessor::links(Accessor::node(cont, node));
    lnk_ref.parent     = parent;
  }

  static inline std::uint32_t unset_flag(node_type& node_ref)
  {
    Accessor::unset_flag(node_ref);
  }

  static inline std::uint32_t set_flag(node_type& node_ref)
  {
    Accessor::set_flag(node_ref);
  }

  struct node_it
  {
    void set()
    {
      set_flag(*ref);
    }

    void unset()
    {
      unset_flag(*ref);
    }

    bool is_set() const
    {
      return is_set(*ref);
    }

    void set_parent(std::uint32_t par)
    {
      Accessor::link(*ref).parent = par;
    }

    void set_left(std::uint32_t left)
    {
      Accessor::link(*ref).left = left;
    }

    void set_right(std::uint32_t right)
    {
      Accessor::link(*ref).right = right;
    }

    node_it parent(container& icont) const
    {
      return node_it(icont, Accessor::link(*ref).parent);
    }

    node_it left(container& icont) const
    {
      return node_it(icont, Accessor::link(*ref).left);
    }

    node_it right(container& icont) const
    {
      return node_it(icont, Accessor::link(*ref).right);
    }

    std::uint32_t parent() const
    {
      return Accessor::link(*ref).parent;
    }

    std::uint32_t left() const
    {
      return Accessor::link(*ref).left;
    }

    std::uint32_t right() const
    {
      return Accessor::link(*ref).right;
    }

    std::uint32_t index() const
    {
      return node;
    }

    node_it(container& icont, std::uint32_t inode) : node(inode), ref(&Accessor::node(icont, inode)) {}
    node_it(std::uint32_t inode, node_type* iref) : node(inode), ref(iref) {}
    node_it()               = default;
    node_it(node_it const&) = default;
    node_it(node_it&&)      = default;
    node_it& operator=(node_it const&) = default;
    node_it& operator=(node_it&&) = default;

    bool operator==(node_it const& iother) const
    {
      return node == iother.index();
    }
    bool operator!=(node_it const& iother) const
    {
      return node != iother.index();
    }

    std::uint32_t node = detail::k_null_32;
    node_type*    ref  = nullptr;
  };

  void delete_fixup(container& cont, node_it node)
  {
    node_it s;
    while (node.index() != root && !node.is_set())
    {
    }
  }

  void insert_fixup(container& cont, node_it node)
  {
    for (auto parent = node.parent(cont); parent.is_set(); parent = node.parent(cont))
    {
      auto parent_of_parent = parent.parent(cont);
      if (node.parent() == parent_of_parent.right())
      {
        auto update = parent_of_parent.left(cont);
        if (update.is_set())
        {
          update.unset();
          parent.unset();
          parent_of_parent.set();
          node = parent_of_parent;
        }
        else
        {
          if (node.value() == parent.left())
          {
            node             = parent;
            parent           = node.parent(cont);
            parent_of_parent = parent.parent(cont);
            right_rotate(cont, node);
          }
          parent.unset();
          parent_of_parent.set();
          left_rotate(cont, parent_of_parent);
        }
      }
      else
      {
        auto update = parent_of_parent.right(cont);
        if (update.is_set())
        {
          update.unset();
          parent.unset();
          parent_of_parent.set();
          node = parent_of_parent;
        }
        else
        {
          if (node.value() == parent.right())
          {
            node             = parent;
            parent           = node.parent(cont);
            parent_of_parent = parent.parent(cont);
            left_rotate(cont, node);
          }
          parent.unset();
          parent_of_parent.set();
          right_rotate(cont, parent_of_parent);
        }
      }
      if (node == root)
        break;
    }
    unset_flag(*Accessor::node_ptr(cont, root));
  }

public:
  std::uint32_t find(container& icont, std::uint32_t iroot, value_type ivalue) const
  {
    std::uint32_t node = iroot;
    while (node != k_null_32)
    {
      auto node_ptr = Accessor::node_ptr(icont, node);
      if (node_ptr
    }
  }

  std::uint32_t find(container& icont, value_type ivalue) const
  {
    return find(icont, root, ivalue);
  }

  void insert(container& cont, std::uint32_t node)
  {
    std::uint32_t parent     = k_null_32;
    std::uint32_t it         = root;
    auto          node_value = Accessor::value(cont, node);

    while (it != k_null_32)
    {
      parent = it;
      if (node_value < Accessor::value(cont, it))
        it = Accessor::node_ptr(cont, it)->left;
      else
        it = Accessor::node_ptr(cont, it)->right;
    }

    tree_node& node_ref = *Accessor::node_ptr(cont, node);
    set_parent(node_ref, parent);
    if (parent == k_null_32)
    {
      root = node;
      unset_flag(node_ref);
      return;
    }
    else if (node_value < Accessor::value(cont, parent))
      Accessor::node_ptr(cont, parent)->left = node;
    else
      Accessor::node_ptr(cont, parent)->right = node;

    if (Accessor::node_ptr(cont, parent)->parent == k_null_32)
      return;

    insert_fixup(cont, node_it(node, &node_ref));
  }

private:
  std::uint32_t root = k_null_i32;
};

} // namespace cppalloc::detail
