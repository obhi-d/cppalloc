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

  static inline std::uint32_t parent_idx(std::int32_t parent)
  {
    return static_cast<std::uint32_t>(parent) & k_parent_mask;
  }

  static inline std::uint32_t set_parent(Container& cont, std::uint32_t node, std::uint32_t parent)
  {
    tree_node& node_ref = Accessor::node(cont, node);
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

  void insert_fixup(Container& cont, std::uint32_t node, tree_node* node_ref)
  {
    std::uint32_t update;
    tree_node*    parent = &Accessor::node(cont, node_ref->parent);
    while (is_set(*parent))
    {
      auto& parent_of_parent = Accecssor::node(parent->parent);
      if (node_ref->parent == parent_of_parent.right)
      {
        update = parent_of_parent.left;
        if (is_set(Accessor::node(cont, update)))
        {
        }
      }
    }
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
        it = Accessor::node(cont, it).left;
      else
        it = Accessor::node(cont, it).right;
    }

    tree_node& node_ref = Accessor::node(cont, node);
    set_parent(node_ref, parent);
    if (parent == k_null_32)
    {
      root = node;
      unset_flag(node_ref);
      return;
    }
    else if (node_value < Accessor::value(cont, parent))
      Accessor::node(cont, parent).left = node;
    else
      Accessor::node(cont, parent).right = node;

    if (Accessor::node(cont, parent).parent == k_null_32)
      return;

    insert_fixup(cont, node, node_ref);
  }

private:
  std::uint32_t root = k_null_i32;
};

} // namespace cppalloc::detail