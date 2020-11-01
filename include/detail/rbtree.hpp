#include <detail/cppalloc_common.hpp>

namespace cppalloc::detail
{
//
struct tree_node
{
  std::uint32_t parent = k_null_32; // sign indicates color
  std::uint32_t left   = k_null_32;
  std::uint32_t right  = k_null_32;
};

constexpr std::uint32_t k_flag_mask   = 1u << 31u;
constexpr std::uint32_t k_parent_mask = ~k_flag_mask;
template <typename Accessor, typename Container>
class rbtree
{

private:
  static inline bool is_set(tree_node const& node)
  {
    return (node.parent & k_flag_mask) != 0;
  }

  static inline std::uint32_t set_parent(Container& cont, std::uint32_t node, std::uint32_t parent)
  {
    tree_node& node_ref = Accessor::node_ptr(cont, node);
    node_ref.parent     = (node_ref.parent & k_flag_mask) | parent;
  }

  static inline std::uint32_t set_parent(tree_node& node_ref, std::uint32_t parent)
  {
    node_ref.parent = (node_ref.parent & k_flag_mask) | parent;
  }

  static inline std::uint32_t unset_flag(tree_node& node_ref)
  {
    node_ref.parent = (node_ref.parent & k_parent_mask);
  }

  static inline std::uint32_t set_flag(tree_node& node_ref)
  {
    node_ref.parent = (node_ref.parent & k_parent_mask) | k_flag_mask;
  }

  struct node_it
  {
    void set()
    {
      set_flag(*node);
    }

    void unset()
    {
      unset_flag(*node);
    }

    bool is_set() const
    {
      return is_set(*node);
    }

    void set_parent(std::uint32_t par)
    {
      set_parent(*ref, par);
    }

    void set_left(std::uint32_t left)
    {
      ref->left = left;
    }

    void set_right(std::uint32_t right)
    {
      ref->right = right;
    }

    node_it parent(Container& icont) const
    {
      return node_it(icont, ref->parent & k_parent_mask);
    }

    node_it left(Container& icont) const
    {
      return node_it(icont, ref->left);
    }

    node_it right(Container& icont) const
    {
      return node_it(icont, ref->right);
    }

    std::uint32_t parent() const
    {
      return ref->parent & k_parent_mask;
    }

    std::uint32_t left() const
    {
      return ref->left;
    }

    std::uint32_t right() const
    {
      return ref->right;
    }

    std::uint32_t value() const
    {
      return node;
    }

    node_it(Container& icont, std::uint32_t inode) : node(inode), ref(Accessor::node_ptr(icont, inode)) {}
    node_it(std::uint32_t inode, tree_node* iref) : node(inode), ref(iref) {}
    node_it()               = default;
    node_it(node_it const&) = default;
    node_it(node_it&&)      = default;
    node_it& operator=(node_it const&) = default;
    node_it& operator=(node_it&&) = default;

    bool operator==(node_it const& iother) const
    {
      return node == iother.value();
    }
    bool operator!=(node_it const& iother) const
    {
      return node != iother.value();
    }

    std::uint32_t node = detail::k_null_32;
    tree_node*    ref  = nullptr;
  };

  void delete_fixup(Container& cont, node_it node)
  {
    node_it s;
    while (node.value() != root && !node.is_set())
    {
    }
  }

  void insert_fixup(Container& cont, node_it node)
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
    unset_flag(Accessor::node_ptr(cont, root));
  }

public:
  void insert(Container& cont, std::uint32_t node)
  {
    std::uint32_t parent     = k_null_32;
    std::uint32_t it         = root;
    auto          node_value = Accessor::value(cont, node);

    while (it != k_null_32)
    {
      parent = it;
      if (node_value < Accessor::value(cont, it))
        it = Accessor::node_ptr(cont, it).left;
      else
        it = Accessor::node_ptr(cont, it).right;
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
      Accessor::node_ptr(cont, parent).left = node;
    else
      Accessor::node_ptr(cont, parent).right = node;

    if (Accessor::node_ptr(cont, parent).parent == k_null_32)
      return;

    insert_fixup(cont, node_it(node, &node_ref));
  }

private:
  std::uint32_t root = k_null_i32;
};

} // namespace cppalloc::detail