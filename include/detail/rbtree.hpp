#pragma once

#include <detail/cppalloc_common.hpp>

namespace cppalloc::detail
{
//
struct tree_node
{
  std::uint32_t parent = 0;
  std::uint32_t left   = 0;
  std::uint32_t right  = 0;
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

    tnode_it(Container& icont, std::uint32_t inode = 0) : node(inode), ref(&Accessor::node(icont, inode)) {}
    tnode_it(std::uint32_t inode, node_t* iref) : node(inode), ref(iref) {}
    tnode_it()                = delete;
    tnode_it(tnode_it const&) = default;
    tnode_it(tnode_it&&)      = default;
    tnode_it& operator=(tnode_it const&) = default;
    tnode_it& operator=(tnode_it&&) = default;

    bool operator==(tnode_it const& iother) const
    {
      return node == iother.index();
    }

    bool operator==(std::uint32_t iother) const
    {
      return node == iother;
    }
    bool operator!=(tnode_it const& iother) const
    {
      return node != iother.index();
    }
    bool operator!=(std::uint32_t iother) const
    {
      return node != iother;
    }

    inline operator bool() const
    {
      return node != 0;
    }

    std::uint32_t node = 0;
    node_t*       ref  = nullptr;
  };

  using node_it  = tnode_it<container>;
  using cnode_it = tnode_it<container const>;

  inline cnode_it minimum(container const& icont, cnode_it u) const
  {
    while (u.left() != 0)
      u = u.left(icont);
    return u;
  }

  inline cnode_it maximum(container const& icont, cnode_it u) const
  {
    while (u.right() != 0)
      u = u.right(icont);
    return u;
  }

  inline node_it minimum(container& icont, node_it u) const
  {
    while (u.left() != 0)
      u = u.left(icont);
    return u;
  }

  inline node_it maximum(container& icont, node_it u) const
  {
    while (u.right() != 0)
      u = u.right(icont);
    return u;
  }

  inline void left_rotate(container& icont, node_it x)
  {
    node_it y = x.right(icont);
    x.set_right(y.left());
    if (y.left() == 0)
      y.left(icont).set_parent(x.index());
    y.set_parent(x.parent());
    if (x.parent() == 0)
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
    if (y.right() == 0)
      y.right(icont).set_parent(x.index());
    y.set_parent(x.parent());
    if (x.parent() == 0)
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
    if (!parent)
      root = v.index();
    else if (parent.left() == u.index())
      parent.set_left(v.index());
    else
      parent.set_right(v.index());
    v.set_parent(parent.index());
  }

  void insert_fixup(container& cont, node_it z)
  {
    for (node_it z_p = z.parent(cont); z_p.is_set(); z_p = z.parent(cont))
    {
      node_it z_p_p = z_p.parent(cont);
      if (z_p == z_p_p.left())
      {
        node_it y = z_p_p.right(cont);
        if (!y.is_set())
        {
          z_p.unset();
          y.unset();
          z_p_p.set();
          z = z_p_p;
        }
        else
        {
          if (z == z_p.right())
          {
            z = z_p;
            left_rotate(cont, z);
            z_p   = z.parent(cont);
            z_p_p = z_p.parent(cont);
          }
          z_p.unset();
          z_p_p.set();
          right_rotate(cont, z_p_p);
        }
      }
      else
      {
        node_it y = z_p_p.left(cont);
        if (!y.is_set())
        {
          z_p.unset();
          y.unset();
          z_p_p.set();
          z = z_p_p;
        }
        else
        {
          if (z == z_p.left())
          {
            z = z_p;
            right_rotate(cont, z);
            z_p   = z.parent(cont);
            z_p_p = z_p.parent(cont);
          }
          z_p.unset();
          z_p_p.set();
          left_rotate(cont, z_p_p);
        }
      }
    }

    unset_flag(Accessor::node(cont, root));
  }

  void erase_fix(container& icont, node_it x)
  {
    node_it w(icont);
    while (x.index() != root && !x.is_set())
    {
      auto x_p = x.parent(icont);
      if (x.index() == x_p.left())
      {
        w = x_p.right(icont);
        if (w.is_set())
        {
          w.unset();
          x_p.set();
          left_rotate(icont, x_p);
          w = x.parent(icont).right(icont);
        }

        if (!w.left(icont).is_set() && !w.right(icont).is_set())
        {
          w.set();
          x = x.parent(icont);
        }
        else
        {
          if (!w.right(icont).is_set())
          {
            w.left(icont).unset();
            w.set();
            right_rotate(icont, w);
            w = x.parent(icont).right(icont);
          }

          x_p = x.parent(icont);
          w.set(x_p.is_set());
          x_p.unset();
          w.right(icont).unset();
          left_rotate(icont, x_p);
          x = node_it(icont, root);
        }
      }
      else
      {
        w = x_p.left(icont);
        if (w.is_set())
        {
          w.unset();
          x_p.set();
          right_rotate(icont, x_p);
          w = x.parent(icont).left(icont);
        }

        if (!w.right(icont).is_set() && !w.left(icont).is_set())
        {
          w.set();
          x = x.parent(icont);
        }
        else
        {
          if (!w.left(icont).is_set())
          {
            w.right(icont).unset();
            w.set();
            left_rotate(icont, w);
            w = x.parent(icont).left(icont);
          }

          x_p = x.parent(icont);
          w.set(x_p.is_set());
          x_p.unset();
          w.left(icont).unset();
          right_rotate(icont, x_p);
          x = node_it(icont, root);
        }
      }
    }
    x.unset();
  }

public:
  value_type minimum(container const& icont) const
  {
    return root == 0 ? value_type() : minimum(icont, cnode_it(icont, root)).value();
  }

  value_type maximum(container const& icont) const
  {
    return root == 0 ? value_type() : maximum(icont, cnode_it(icont, root)).value();
  }

  std::uint32_t find(container const& icont, std::uint32_t iroot, value_type ivalue) const
  {
    std::uint32_t node = iroot;
    while (node != 0)
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
    if (node != 0)
      return Accessor::links(Accessor::node(icont, node)).left;
    return 0;
  }

  std::uint32_t next_more(container& icont, std::uint32_t node)
  {
    if (node != 0)
      return Accessor::links(Accessor::node(icont, node)).right;
    return 0;
  }

  std::uint32_t lower_bound(container& icont, value_type ivalue) const
  {
    return lower_bound(icont, root, ivalue);
  }

  std::uint32_t lower_bound(container& icont, std::uint32_t iroot, value_type ivalue) const
  {
    std::uint32_t node = iroot;
    std::uint32_t last = iroot;
    while (node != 0)
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

  void insert(container& cont, std::uint32_t iz)
  {
    node_it y(cont);
    node_it x(cont, root);
    node_it z(cont, iz);
    while (x)
    {
      y = x;
      if (z.value() < x.value())
        x = x.left(cont);
      else
        x = x.right(cont);
    }
    z.set_parent(y.index());
    if (!y)
    {
      root = z.index();
      return;
    }
    else if (z.value() < y.value())
      y.set_left(z.index());
    else
      y.set_right(z.index());
    z.set();
    insert_fixup(cont, z);
  }

  void erase(container& icont, std::uint32_t iz)
  {
    assert(iz != 0);

    node_it z(icont, iz);
    bool    flag = z.is_set();
    node_it y    = z;
    node_it x(icont);
    if (z.left() == 0)
    {
      x = z.right(icont);
      transplant(icont, z, x);
    }
    else if (z.right() == 0)
    {
      x = z.left(icont);
      transplant(icont, z, x);
    }
    else
    {
      y    = minimum(icont, z.right(icont));
      flag = y.is_set();
      x    = y.right(icont);
      if (y.parent() == iz)
        x.set_parent(y.index());
      else
      {
        transplant(icont, y, x);
        y.set_right(z.right());
        y.right(icont).set_parent(y.index());
      }

      transplant(icont, z, y);
      y.set_left(z.left());
      y.left(icont).set_parent(y.index());
      y.set(z.is_set());
    }
    z.unset();
    z.set_left(0);
    z.set_right(0);
    z.set_parent(0);
    if (!flag)
      erase_fix(icont, x);
  }

  template <typename L>
  void in_order_traversal(container const& blocks, std::uint32_t node, L&& visitor) const
  {
    if (node != 0)
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
    if (node != 0)
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
    if (root == 0)
      return;
    value_type last = minimum(blocks);
    in_order_traversal(blocks, [&last](node_type const& n) {
      assert(last <= Accessor::value(n));
      last = Accessor::value(n);
    });
  }

private:
  std::uint32_t root = 0;
};

} // namespace cppalloc::detail
