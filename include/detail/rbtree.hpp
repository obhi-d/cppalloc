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

/**
 * @remarks
 * Accessor adapter
 *
 * struct accessor_adapter
 * {
 *   using value_type = int;
 *   struct node_type
 *   {
 *     value_type value;
 *     tree_node  node;
 *   };
 *
 *   using container = std::vector<value_type>;
 *
 *   node_type const& node(container const&, std::uint32_t);
 *   node_type&       node(container&, std::uint32_t);
 *   tree_node const& links(node_type const&);
 *   tree_node&       links(node_type&);
 *   auto             value(node_type const&);
 *   bool             is_set(node_type const&);
 *   void             set_flag(node_type&);
 *   void             set_flag(node_type&, bool);
 *   bool             unset_flag(node_type&);
 * };
 *
 **/

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

  static inline void set_parent(container& cont, std::uint32_t node, std::uint32_t parent)
  {
    tree_node& lnk_ref = Accessor::links(Accessor::node(cont, node));
    lnk_ref.parent     = parent;
  }

  static inline void unset_flag(node_type& node_ref)
  {
    Accessor::unset_flag(node_ref);
  }

  static inline void set_flag(node_type& node_ref)
  {
    Accessor::set_flag(node_ref);
  }

  template <typename Container>
  struct tnode_it
  {
    void set() requires(!std::is_const_v<Container>)
    {
      Accessor::set_flag(*ref);
    }

    void set(bool b) requires(!std::is_const_v<Container>)
    {
      Accessor::set_flag(*ref, b);
    }

    void unset() requires(!std::is_const_v<Container>)
    {
      Accessor::unset_flag(*ref);
    }

    bool is_set() const
    {
      return Accessor::is_set(*ref);
    }

    void set_parent(std::uint32_t par) requires(!std::is_const_v<Container>)
    {
      Accessor::links(*ref).parent = par;
    }

    void set_left(std::uint32_t left) requires(!std::is_const_v<Container>)
    {
      Accessor::links(*ref).left = left;
    }

    void set_right(std::uint32_t right) requires(!std::is_const_v<Container>)
    {
      Accessor::links(*ref).right = right;
    }

    tnode_it parent(Container& icont) const
    {
      return tnode_it(icont, Accessor::links(*ref).parent);
    }

    tnode_it left(Container& icont) const
    {
      return tnode_it(icont, Accessor::links(*ref).left);
    }

    tnode_it right(Container& icont) const
    {
      return tnode_it(icont, Accessor::links(*ref).right);
    }

    std::uint32_t parent() const
    {
      return Accessor::links(*ref).parent;
    }

    std::uint32_t left() const
    {
      return Accessor::links(*ref).left;
    }

    std::uint32_t right() const
    {
      return Accessor::links(*ref).right;
    }

    std::uint32_t index() const
    {
      return node;
    }

    value_type const& value() const
    {
      return Accessor::value(*ref);
    }

    using node_t = std::conditional_t<std::is_const_v<Container>, node_type const, node_type>;

    tnode_it(Container& icont, std::uint32_t inode) : node(inode), ref(nullptr)
    {
      if (inode != k_null_32)
        ref = &Accessor::node(icont, inode);
    }

    tnode_it(std::uint32_t inode, node_t* iref) : node(inode), ref(iref) {}
    tnode_it()                = default;
    tnode_it(tnode_it const&) = default;
    tnode_it(tnode_it&&)      = default;
    tnode_it& operator=(tnode_it const&) = default;
    tnode_it& operator=(tnode_it&&) = default;

    bool operator==(tnode_it const& iother) const
    {
      return node == iother.index();
    }
    bool operator!=(tnode_it const& iother) const
    {
      return node != iother.index();
    }

    std::uint32_t node = detail::k_null_32;
    node_t*       ref  = nullptr;
  };

  using node_it  = tnode_it<container>;
  using cnode_it = tnode_it<container const>;

  inline cnode_it minimum(container const& icont, cnode_it u) const
  {
    while (u.left() != k_null_32)
      u = u.left(icont);
    return u;
  }

  inline cnode_it maximum(container const& icont, cnode_it u) const
  {
    while (u.right() != k_null_32)
      u = u.right(icont);
    return u;
  }

  inline node_it minimum(container& icont, node_it u) const
  {
    while (u.left() != k_null_32)
      u = u.left(icont);
    return u;
  }

  inline node_it maximum(container& icont, node_it u) const
  {
    while (u.right() != k_null_32)
      u = u.right(icont);
    return u;
  }

  inline void left_rotate(container& icont, node_it x)
  {
    node_it y = x.right(icont);
    x.set_right(y.left());
    if (y.left() == k_null_32)
      y.left(icont).set_parent(x.index());
    y.set_parent(x.parent());
    if (x.parent() == k_null_32)
      root = y.index();
    else if (x.index() == x.parent(icont).left())
      x.parent(icont).set_left(y.index());
    else
      x.parent(icont).set_right(y.index());
    y.set_left(x.index());
    x.set_parent(y.index());
  }

  inline void right_rotate(container& icont, node_it x)
  {
    node_it y = x.left(icont);
    x.set_left(y.right());
    if (y.right() == k_null_32)
      y.right(icont).set_parent(x.index());
    y.set_parent(x.parent());
    if (x.parent() == k_null_32)
      root = y.index();
    else if (x.index() == x.parent(icont).right())
      x.parent(icont).set_right(y.index());
    else
      x.parent(icont).set_left(y.index());
    y.set_right(x.index());
    x.set_parent(y.index());
  }

  inline void transplant(container& icont, node_it u, node_it v)
  {
    auto parent = u.parent(icont);
    if (parent.index() == k_null_32)
      root = v.index();
    else if (parent.left() == u.index())
      parent.set_left(v.index());
    else
      parent.set_right(v.index());
    v.set_parent(parent.index());
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
          if (node.index() == parent.left())
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
          if (node.index() == parent.right())
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
      if (node.index() == root)
        break;
    }
    unset_flag(Accessor::node(cont, root));
  }

  void erase_fix(container& icont, node_it x)
  {
    node_it s;
    while (x.index() != root && !x.is_set())
    {
      auto parent = x.parent(icont);
      if (x.index() == parent.left())
      {
        s = parent.right(icont);
        if (s.is_set())
        {
          s.unset();
          parent.set();
          left_rotate(icont, parent);
          s = x.parent(icont).right(icont);
        }

        auto s_left  = s.left(icont);
        auto s_right = s.right(icont);
        if (!s_left.is_set() && !s_right.is_set())
        {
          s.set();
          x = x.parent(icont);
        }
        else
        {
          if (!s_right.is_set())
          {
            s_left.unset();
            s.set();
            right_rotate(icont, s);
            s = x.parent(icont).right(icont);
          }

          parent = x.parent(icont);
          s.set(parent.is_set());
          parent.unset();
          s_right.unset();
          left_rotate(icont, parent);
          x = node_it(icont, root);
        }
      }
      else
      {
        s = parent.left(icont);
        if (s.is_set())
        {
          s.unset();
          parent.set();
          right_rotate(icont, parent);
          s = x.parent(icont).left(icont);
        }

        auto s_left  = s.left(icont);
        auto s_right = s.right(icont);
        if (!s_left.is_set() && !s_right.is_set())
        {
          s.set();
          x = x.parent(icont);
        }
        else
        {
          if (!s_left.is_set())
          {
            s_right.unset();
            s.set();
            left_rotate(icont, s);
            s = x.parent(icont).left(icont);
          }

          parent = x.parent(icont);
          s.set(parent.is_set());
          parent.unset();
          s_left.unset();
          right_rotate(icont, parent);
          x = node_it(icont, root);
        }
      }
    }
    x.unset();
  }

public:
  value_type minimum(container const& icont) const
  {
    return root == k_null_32 ? value_type() : minimum(icont, cnode_it(icont, root)).value();
  }

  value_type maximum(container const& icont) const
  {
    return root == k_null_32 ? value_type() : maximum(icont, cnode_it(icont, root)).value();
  }

  std::uint32_t find(container const& icont, std::uint32_t iroot, value_type ivalue) const
  {
    std::uint32_t node = iroot;
    while (node != k_null_32)
    {
      auto const& node_ref = Accessor::node(icont, node);
      if (Accessor::value(node_ref) == ivalue)
        break;
      else if (Accessor::value(node_ref) <= ivalue)
        node = Accessor::links(node_ref).right;
      else
        node = Accessor::links(node_ref).left;
    }
    return node;
  }

  std::uint32_t next_less(container& icont, std::uint32_t node)
  {
    if (node != k_null_32)
      return Accessor::links(Accessor::node(icont, node)).left;
    return k_null_32;
  }

  std::uint32_t next_more(container& icont, std::uint32_t node)
  {
    if (node != k_null_32)
      return Accessor::links(Accessor::node(icont, node)).right;
    return k_null_32;
  }

  std::uint32_t lower_bound(container& icont, value_type ivalue) const
  {
    return lower_bound(icont, root, ivalue);
  }

  std::uint32_t lower_bound(container& icont, std::uint32_t iroot, value_type ivalue) const
  {
    std::uint32_t node = iroot;
    std::uint32_t last = iroot;
    while (node != k_null_32)
    {
      last                 = node;
      auto const& node_ref = Accessor::node(icont, node);
      if (Accessor::value(node_ref) == ivalue)
        break;
      else if (Accessor::value(node_ref) <= ivalue)
        node = Accessor::links(node_ref).right;
      else
        node = Accessor::links(node_ref).left;
    }
    return last;
  }

  std::uint32_t find(container& icont, value_type ivalue) const
  {
    return find(icont, root, ivalue);
  }

  void insert(container& cont, std::uint32_t inode)
  {
    std::uint32_t parent = k_null_32;
    std::uint32_t it     = root;
    node_it       node(cont, inode);
    while (it != k_null_32)
    {
      parent   = it;
      auto& nr = Accessor::node(cont, it);
      if (node.value() < Accessor::value(nr))
        it = Accessor::links(nr).left;
      else
        it = Accessor::links(nr).right;
    }

    node.set_parent(parent);
    if (parent == k_null_32)
    {
      root = inode;
      node.unset();
      return;
    }
    else if (node.value() < Accessor::value(Accessor::node(cont, parent)))
      Accessor::links(Accessor::node(cont, parent)).left = inode;
    else
      Accessor::links(Accessor::node(cont, parent)).right = inode;

    if (Accessor::links(Accessor::node(cont, parent)).parent == k_null_32)
      return;

    insert_fixup(cont, node);
  }

  void erase(container& icont, std::uint32_t inode)
  {
    assert(inode != k_null_32);

    node_it node(icont, inode);
    bool    flag  = node.is_set();
    node_it ynode = node;
    node_it xnode;
    if (node.left() == k_null_32)
    {
      xnode = node.right(icont);
      transplant(icont, node, node.right(icont));
    }
    else if (node.right() == k_null_32)
    {
      xnode = node.left(icont);
      transplant(icont, node, node.left(icont));
    }
    else
    {
      ynode = minimum(icont, node.right(icont));
      flag  = ynode.is_set();
      xnode = ynode.right(icont);
      if (ynode.parent() == inode)
        xnode.set_parent(ynode.index());
      else
      {
        transplant(icont, ynode, xnode);
        ynode.set_right(node.right());
        node.right(icont).set_parent(ynode.index());
      }

      transplant(icont, node, ynode);
      ynode.set_left(node.left());
      ynode.left(icont).set_parent(ynode.index());
      ynode.set(node.is_set());
    }
    node.set_left(k_null_32);
    node.set_right(k_null_32);
    node.set_parent(k_null_32);
    if (!flag)
      erase_fix(icont, xnode);
  }

  template <typename L>
  void in_order_traversal(container const& blocks, std::uint32_t node, L&& visitor) const
  {
    if (node != k_null_32)
    {
      auto& n = Accessor::node(blocks, node);
      in_order_traversal(blocks, Accessor::links(n).left, std::forward<L>(visitor));
      visitor(Accessor::node(blocks, node));
      in_order_traversal(blocks, Accessor::links(n).right, std::forward<L>(visitor));
    }
  }
  template <typename L>
  void in_order_traversal(container& blocks, L&& visitor)
  {
    in_order_traversal(blocks, root, std::forward<L>(visitor));
  }

  template <typename L>
  void in_order_traversal(container& blocks, std::uint32_t node, L&& visitor)
  {
    if (node != k_null_32)
    {
      auto& n = Accessor::node(blocks, node);
      in_order_traversal(blocks, Accessor::links(n).left, std::forward<L>(visitor));
      visitor(Accessor::node(blocks, node));
      in_order_traversal(blocks, Accessor::links(n).right, std::forward<L>(visitor));
    }
  }

  template <typename L>
  void in_order_traversal(container const& blocks, L&& visitor) const
  {
    in_order_traversal(blocks, root, std::forward<L>(visitor));
  }

  std::uint32_t node_count(container const& blocks) const
  {
    std::uint32_t cnt = 0;
    in_order_traversal(blocks, [&cnt](node_type const&) {
      cnt++;
    });
    return cnt;
  }

  void validate_integrity(container const& blocks) const
  {
    if (root == k_null_32)
      return;
    value_type last = minimum(blocks);
    in_order_traversal(blocks, [&last](node_type const& n) {
      assert(last <= Accessor::value(n));
      last = Accessor::value(n);
    });
  }

private:
  std::uint32_t root = k_null_32;
};

} // namespace cppalloc::detail
