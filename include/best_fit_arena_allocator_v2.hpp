
#include <detail/cppalloc_common.hpp>
#include <detail/table.hpp>
#include <type_traits>

namespace cppalloc
{
namespace detail
{

struct best_fit_arena_allocator_v2_tag
{
};

} // namespace detail

template <typename arena_manager, typename size_type = std::uint32_t, bool k_compute_stats = false>
class best_fit_arena_allocator_v2 : public detail::statistics<detail::best_fit_arena_allocator_v2_tag, k_compute_stats>
{
  struct block
  {
    std::uint64_t data;
    list_node     order[ordering_by::k_count];
    size_type     offset;
    std::uint32_t arena;
  };

  using block_bank = detail::table<block>;
  using arena_bank = detail::table<arena>;

  template <detail::ordering_by k_ordering>
  struct block_accessor
  {
    using value_type = block;
    static list_node& node(block_bank& bank, std::uint32_t node)
    {
      return bank[node].order[k_ordering];
    }

    static list_node const& node(block_bank const& bank, std::uint32_t node)
    {
      return bank[node].order[k_ordering];
    }

    static value_type const& get(block_bank const& bank, std::uint32_t node)
    {
      return bank[node];
    }

    static value_type& get(block_bank& bank, std::uint32_t node)
    {
      return bank[node];
    }
  };

  using block_list = detail::vlist<block_accessor<detail::ordering_by::e_offset>, block_bank>;
  using free_list  = detail::vlist<block_accessor<detail::ordering_by::e_size>, block_bank>;

  struct arena
  {
    block_list    blocks;
    list_node     order;
    size_type     size;
    std::uint64_t data = k_null_64;
  };

  struct arena_accessor
  {
    using value_type = arena;
    static list_node& node(arena_bank& bank, std::uint32_t node)
    {
      return bank[node].order;
    }

    static list_node const& node(arena_bank const& bank, std::uint32_t node)
    {
      return bank[node].order;
    }

    static value_type const& get(arena_bank const& bank, std::uint32_t node)
    {
      return bank[node];
    }

    static value_type& get(arena_bank& bank, std::uint32_t node)
    {
      return bank[node];
    }
  };

  using arena_list = detail::vlist<arena_accessor, arena_bank>;
  using statistics = detail::statistics<best_fit_arena_allocator_v2_tag, k_compute_stats>;

public:
  enum option : std::uint32_t
  {
    f_defrag          = 1u << 0u,
    f_dedicated_arena = 1u << 1u,
  };

  using std::uint32_t = option_flags;

  using std::uint32_t = address;

  //! Inteface
  template <typename... Args>
  best_fit_arena_allocator_v2(size_type i_arena_size, arena_manager& i_manager, Args&&...);

  //! Allocate, second paramter is the user_handle to be associated with the
  //! allocation
  address allocate(size_type i_size, size_type i_alignment = 0, std::uint64_t i_user_handle = 0,
                   option_flags i_options = 0);
  //! Deallocate, size is optional
  void deallocate(address i_address, size_type i_size, size_type i_alignment = 0);

private:
  std::uint32_t add_arena(size_type i_handle, size_type i_arena_size);

  block_bank     blocks;
  arena_bank     arenas;
  arena_list     arena_ordering;
  free_list      free_ordering;
  arena_manager& manager;
  size_type      arena_size;
};

template <typename arena_manager, typename size_type, bool k_compute_stats>
template <typename... Args>
inline best_fit_arena_allocator_v2<arena_manager, size_type, k_compute_stats>::best_fit_arena_allocator_v2(
    size_type i_arena_size, arena_manager& i_manager, Args&&... i_args)
    : statistics(std::forward<Args>(i_args)...), manager(i_manager), arena_size(i_arena_size)
{
}

template <typename arena_manager, typename size_type, bool k_compute_stats>
inline std::uint32_t best_fit_arena_allocator_v2<arena_manager, size_type, k_compute_stats>::add_arena(
    size_type i_handle, size_type i_arena_size)
{
  std::uint32_t arena_id = arenas.emplace({});
  arena_ordering.push_back(arena_id);
  return arena_id;
}

template <typename arena_manager, typename size_type, bool k_compute_stats>
inline best_fit_arena_allocator_v2<arena_manager, size_type, k_compute_stats>::address best_fit_arena_allocator_v2<
    arena_manager, size_type, k_compute_stats>::allocate(size_type i_size, size_type i_alignment,
                                                         std::uint64_t i_user_handle, option_flags i_options)
{
  auto measure = statistics::report_allocate(i_size);

  assert(i_user_handle != k_null_64);
  if (i_options & f_dedicated_arena || i_size >= arena_size)
  {
    return {0, add_arena(i_user_handle, i_size)};
  }

  size_type align_mask = 0;
  if (i_alignment)
  {
    align_mask = i_alignment - 1;
    i_size += i_alignment;
  }

  if (i_size > get_max_free_block_size())
  {
    if (i_options & f_defrag)
    {
      defragment();
      if (i_size > get_max_free_block_size())
        add_arena(k_invalid_handle, arena_size);
    }
    else
      add_arena(k_invalid_handle, arena_size);
  }

  auto end   = free_list.end();
  auto found = std::lower_bound(free_list.begin(), end, i_size, [](free_block_type block, size_type size) -> bool {
    return block.size < size;
  });

  if (found == end)
  {
    return null();
  }

  size_type offset       = (*found).offset;
  size_type fixed_offset = ((offset + align_mask) & ~align_mask);
  size_type arena_num    = found->arena;

  // We dont need a align_mask if offset and fixed_offset is same
  if (fixed_offset == offset)
    i_size -= i_alignment;

  found->size -= i_size;
  auto& arena     = arenas[arena_num];
  auto& node_list = arena.blocks;
  auto  al_end    = node_list.end();
  auto  it =
      std::lower_bound(node_list.begin(), al_end, (*found).offset, [](block_type block, size_type offset) -> bool {
        return block.offset < offset;
      });

  // set allocated to right size, increment it
  it->id         = i_user_handle;
  it->align_mask = align_mask;
  if (found->size > 0)
  {
    size_type num_allocated = static_cast<size_type>(node_list.size());
    // reinsert the left-over size in free list
    found->offset += i_size;
    size_type offset = found->offset;
    reinsert_left(found);
    node_list.insert(it + 1, block_type{offset, k_invalid_handle});
  }
  else
  {
    // delete the existing found index from free list
    free_list.erase(found);
  }
  arena.free_size -= i_size;
  free_size -= i_size;
  return {fixed_offset, arena_num};
}

} // namespace cppalloc