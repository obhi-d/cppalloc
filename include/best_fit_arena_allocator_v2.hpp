
#include <detail/cppalloc_common.hpp>
#include <detail/table.hpp>
#include <detail/vlist.hpp>
#include <type_traits>
#include <utility>

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
    size_type         offset = detail::k_null_sz<size_type>;
    size_type         size   = detail::k_null_sz<size_type>;
    detail::uhandle   data   = detail::k_null_sz<detail::uhandle>;
    std::uint32_t     arena  = detail::k_null_32;
    detail::list_node arena_order;

    block() {}
    block(size_type i_offset, std::uint32_t i_arena) : offset(i_offset), arena(i_arena) {}
    block(size_type i_offset, std::uint32_t i_arena, detail::uhandle i_data) : block(i_offset, i_arena), data(i_data) {}
  };

  using block_bank = detail::table<block>;
  using arena_bank = detail::table<arena>;

  struct block_accessor
  {
    using value_type = block;
    static list_node& node(block_bank& bank, std::uint32_t node)
    {
      return bank[node].arena_order;
    }

    static list_node const& node(block_bank const& bank, std::uint32_t node)
    {
      return bank[node].arena_order;
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

  using block_list = detail::vlist<block_accessor, block_bank>;
  using free_list  = std::vector<std::uint32_t>;

  struct arena
  {
    block_list    blocks;
    list_node     order;
    size_type     size;
    size_type     free;
    std::uint64_t data = detail::k_null_64;
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

  static constexpr address null()
  {
    return detail::k_null_32;
  }

private:
  inline size_type                        block_size(std::uint32_t block_id);
  inline size_type                        block_size(block const& block_ref);
  std::pair<std::uint32_t, std::uint32_t> add_arena(std::uint64_t i_handle, size_type i_arena_size);
  void                                    defragment();
  size_type                               get_max_free_block_size();
  std::uint32_t                           find_free(size_type i_size);

  block_bank     blocks;
  arena_bank     arenas;
  arena_list     arena_ordering;
  free_list      free_ordering;
  arena_manager& manager;
  size_type      arena_size;
  size_type      free_size = 0;
};

template <typename arena_manager, typename size_type, bool k_compute_stats>
template <typename... Args>
inline best_fit_arena_allocator_v2<arena_manager, size_type, k_compute_stats>::best_fit_arena_allocator_v2(
    size_type i_arena_size, arena_manager& i_manager, Args&&... i_args)
    : statistics(std::forward<Args>(i_args)...), manager(i_manager), arena_size(i_arena_size)
{
  // False node
  blocks.emplace(arena_size, detail::k_null_32);
}

template <typename arena_manager, typename size_type, bool k_compute_stats>
inline std::pair<std::uint32_t, std::uint32_t> best_fit_arena_allocator_v2<
    arena_manager, size_type, k_compute_stats>::add_arena(std::uint64_t i_handle, size_type i_arena_size)
{
  statistics::report_new_arena();

  std::uint32_t arena_id  = arenas.emplace();
  arena&        arena_ref = arenas[arena_id];
  arena_ref.size          = i_arena_size;
  std::uint32_t block_id  = blocks.emplace();
  block&        block_ref = blocks[block_id];
  block_ref.offset        = 0;
  block_ref.arena         = arena_id;
  block_ref.data          = i_handle;

  if (i_handle == detail::k_null_64)
  {
    arena_ref.free = i_arena_size;
    free_ordering.push_back(block_id);
    free_size += i_arena_size;
  }
  else
  {
  }
  arena_ref.blocks.push_back(blocks, block_id);
  if (i_arena_size = = arena_size)
    arena_ref.blocks.push_back(blocks, 0);
  else
  {
    // create a false node
    arena_ref.blocks.push_back(blocks, blocks.emplace(i_arena_size, arena_id));
  }
  arena_ordering.push_back(arena_id);
  return std::make_pair(arena_id, block_id);
}

template <typename arena_manager, typename size_type, bool k_compute_stats>
inline size_type best_fit_arena_allocator_v2<arena_manager, size_type, k_compute_stats>::block_size(
    block const& block_ref)
{
  std::uint32_t next = block_ref.order[detail::ordering_by::e_offset].next;
  if (next != detail::k_null_32)
    return blocks[next].offset - block_ref.offset;
  return 0;
}

template <typename arena_manager, typename size_type, bool k_compute_stats>
inline size_type best_fit_arena_allocator_v2<arena_manager, size_type, k_compute_stats>::block_size(
    std::uint32_t block_id)
{
  return block_size(blocks[block_id]);
}

template <typename arena_manager, typename size_type, bool k_compute_stats>
std::uint32_t best_fit_arena_allocator_v2<arena_manager, size_type, k_compute_stats>::find_free(size_type i_size)
{
  auto it = std::lower_bound(free_ordering.begin(), free_ordering.end(), i_size,
                             [this](std::uint32_t block, size_type i_size) -> bool {
                               return blocks[block].size < i_size;
                             });
  return it != free_ordering.end() ? static_cast<std::uint32_t>(std::distance(free_ordering.begin(), it)) : k_null_32;
}

template <typename arena_manager, typename size_type, bool k_compute_stats>
inline best_fit_arena_allocator_v2<arena_manager, size_type, k_compute_stats>::address best_fit_arena_allocator_v2<
    arena_manager, size_type, k_compute_stats>::allocate(size_type i_size, size_type i_alignment,
                                                         std::uint64_t i_user_handle, option_flags i_options)
{
  auto measure = statistics::report_allocate(i_size);

  assert(i_user_handle != detail::k_null_64);
  if (i_options & f_dedicated_arena || i_size >= arena_size)
  {
    return add_arena(i_user_handle, i_size).second;
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
        add_arena(detail::k_null_64, arena_size);
    }
    else
      add_arena(detail::k_null_64, arena_size);
  }

  std::uint32_t found = find_free(i_size);
  if (found == detail::k_null_32)
  {
    return null();
  }
  std::uint32_t free_node = free_ordering[found];
  // Marker
  size_type     offset    = blocks[free_node].offset;
  std::uint32_t arena_num = blocks[free_node].arena;

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

  return {((offset + align_mask) & ~align_mask), arena_num};
}

} // namespace cppalloc