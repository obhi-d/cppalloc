
#include <bit>
#include <detail/cppalloc_common.hpp>
#include <detail/table.hpp>
#include <detail/vlist.hpp>
#include <type_traits>
#include <utility>

namespace cppalloc
{

enum class alloc_strategy
{
  best_fit,
  best_fit_tree,
  first_fit
};

enum alloc_option_bits : std::uint32_t
{
  f_defrag          = 1u << 0u,
  f_dedicated_arena = 1u << 1u,
};

using alloc_options = std::uint32_t;

template <typename size_type>
struct alloc_desc
{
  size_type       size;
  size_type       alignment = 0;
  alloc_options   flags     = 0;
  detail::uhandle huser     = 0;
};

//! Inteface
namespace detail
{

struct arena_allocator_tag
{
  template <typename manager_, typename size_type_, typename extension_, alloc_strategy strat_, bool k_compute_stats_>
  struct traits
  {
    static inline constexpr bool           k_compute_stats = k_compute_stats_;
    static inline constexpr alloc_strategy strategy        = strat_;
    using manager                                          = manager_;
    using size_type                                        = size_type_;
    using extension                                        = extension_;
  };
};

//  ██████╗-██╗------██████╗--██████╗██╗--██╗
//  ██╔══██╗██║-----██╔═══██╗██╔════╝██║-██╔╝
//  ██████╔╝██║-----██║---██║██║-----█████╔╝-
//  ██╔══██╗██║-----██║---██║██║-----██╔═██╗-
//  ██████╔╝███████╗╚██████╔╝╚██████╗██║--██╗
//  ╚═════╝-╚══════╝-╚═════╝--╚═════╝╚═╝--╚═╝
//  -----------------------------------------
template <typename traits>
struct block
{
  enum block_flags : std::uint16_t
  {
    f_free_node = 1 << 0,
  };

  using size_type = typename traits::size_type;
  using extension = typename traits::extension;

  size_type                       offset      = detail::k_null_sz<size_type>;
  size_type                       size        = detail::k_null_sz<size_type>;
  detail::uhandle                 data        = detail::k_null_sz<detail::uhandle>;
  std::uint32_t                   arena       = detail::k_null_32;
  detail::list_node               arena_order = detail::list_node();
  std::uint16_t                   flags       = 0;
  std::uint8_t                    alignment   = 0;
  [[no_unique_address]] extension ext;

  block() {}
  block(size_type i_offset, size_type i_size, std::uint32_t i_arena) : offset(i_offset), size(i_size), arena(i_arena) {}
  block(size_type i_offset, size_type i_size, std::uint32_t i_arena, detail::uhandle i_data)
      : block(i_offset, i_size, i_arena), data(i_data)
  {
  }
  block(size_type ioffset, size_type isize, std::uint32_t iarena, detail::uhandle idata, std::uint16_t iflags)
      : block(ioffset, isize, iarena), data(idata), flags(iflags)
  {
  }
};

template <typename traits>
using block_bank = detail::table<block<traits>>;

template <typename traits>
struct block_accessor
{
  using value_type = block<traits>;
  using bank_type  = block_bank<traits>;
  using size_type  = typename traits::size_type;

  inline static detail::list_node& node(bank_type& bank, std::uint32_t node)
  {
    return bank[node].arena_order;
  }

  inline static detail::list_node const& node(bank_type const& bank, std::uint32_t node)
  {
    return bank[node].arena_order;
  }

  inline static value_type const& get(bank_type const& bank, std::uint32_t node)
  {
    return bank[node];
  }

  inline static value_type& get(bank_type& bank, std::uint32_t node)
  {
    return bank[node];
  }
};

template <typename traits>
using block_list = detail::vlist<block_accessor<traits>, block_bank<traits>>;

//  -█████╗-██████╗-███████╗███╗---██╗-█████╗-
//  ██╔══██╗██╔══██╗██╔════╝████╗--██║██╔══██╗
//  ███████║██████╔╝█████╗--██╔██╗-██║███████║
//  ██╔══██║██╔══██╗██╔══╝--██║╚██╗██║██╔══██║
//  ██║--██║██║--██║███████╗██║-╚████║██║--██║
//  ╚═╝--╚═╝╚═╝--╚═╝╚══════╝╚═╝--╚═══╝╚═╝--╚═╝
//  ------------------------------------------
template <typename traits>
struct arena
{
  using size_type = typename traits::size_type;

  block_list<traits> blocks;
  detail::list_node  order;
  size_type          size;
  size_type          free;
  detail::uhandle    data = detail::k_null_sz<detail::uhandle>;
};

template <typename traits>
using arena_bank = detail::table<detail::arena<traits>>;

template <typename traits>
struct arena_accessor
{
  using value_type = block<traits>;
  using bank_type  = arena_bank<traits>;
  using size_type  = typename traits::size_type;

  inline static detail::list_node& node(bank_type& bank, std::uint32_t node)
  {
    return bank[node].order;
  }

  inline static detail::list_node const& node(bank_type const& bank, std::uint32_t node)
  {
    return bank[node].order;
  }

  inline static value_type const& get(bank_type const& bank, std::uint32_t node)
  {
    return bank[node];
  }

  inline static value_type& get(bank_type& bank, std::uint32_t node)
  {
    return bank[node];
  }
};

template <typename traits>
using arena_list = detail::vlist<arena_accessor<traits>, arena_bank<traits>>;

using free_list = std::vector<std::uint32_t>;

template <alloc_strategy strategy, typename traits>
class alloc_strategy_impl;

template <alloc_strategy strategy>
struct block_extension
{
};

//  ██████╗-███████╗███████╗████████╗-----███████╗██╗████████╗
//  ██╔══██╗██╔════╝██╔════╝╚══██╔══╝-----██╔════╝██║╚══██╔══╝
//  ██████╔╝█████╗--███████╗---██║--------█████╗--██║---██║---
//  ██╔══██╗██╔══╝--╚════██║---██║--------██╔══╝--██║---██║---
//  ██████╔╝███████╗███████║---██║███████╗██║-----██║---██║---
//  ╚═════╝-╚══════╝╚══════╝---╚═╝╚══════╝╚═╝-----╚═╝---╚═╝---
//  ----------------------------------------------------------
template <typename traits>
class alloc_strategy_impl<alloc_strategy::best_fit, traits>
{
public:
  using size_type  = typename traits::size_type;
  using arena_bank = detail::arena_bank<traits>;
  using block_bank = detail::block_bank<traits>;
  using block      = detail::block<traits>;
  using alloc_desc = cppalloc::alloc_desc<size_type>;

  inline free_list::iterator try_allocate(arena_bank& arenas, block_bank& blocks, alloc_desc const& desc);
  inline free_list::iterator try_allocate(arena_bank& arenas, block_bank& blocks, alloc_desc const& desc,
                                          free_list::iterator prev);
  inline std::uint32_t commit(arena_bank& arenas, block_bank& blocks, alloc_desc const& desc, free_list::iterator);

  inline void add_free_arena(std::uint32_t block);
  inline void add_free(block_bank& blocks, std::uint32_t block);
  inline void add_free_after(block_bank& blocks, free_list::iterator loc, std::uint32_t block);

  inline void replace(block_bank& blocks, std::uint32_t block, std::uint32_t new_block, size_type new_size);

  inline std::uint32_t node(free_list::iterator it);
  inline bool          is_valid(free_list::iterator it);

  inline void erase(block_bank& blocks, std::uint32_t node);

private:
  // Private

  inline free_list::iterator find_free(block_bank& blocks, free_list::iterator b, free_list::iterator e,
                                       size_type i_size);
  inline free_list::iterator reinsert_left(block_bank& blocks, free_list::iterator of, std::uint32_t node);
  inline free_list::iterator reinsert_right(block_bank& blocks, free_list::iterator of, std::uint32_t node);

  inline static constexpr std::uint32_t null()
  {
    return detail::k_null_32;
  }

  free_list free_ordering;
};

/// alloc_strategy::best_fit Impl

template <typename traits>
inline free_list::iterator alloc_strategy_impl<alloc_strategy::best_fit, traits>::try_allocate(arena_bank&       arenas,
                                                                                               block_bank&       blocks,
                                                                                               alloc_desc const& desc)
{
  auto size = desc.alignment ? desc.size + desc.alignment - 1 : desc.size;
  if (free_ordering.size() == 0 || blocks[free_ordering.back()] < size)
    return free_ordering.end();
  return find_free(blocks, free_ordering.begin(), free_ordering.end(), size);
}

template <typename traits>
inline free_list::iterator alloc_strategy_impl<alloc_strategy::best_fit, traits>::try_allocate(arena_bank&       arenas,
                                                                                               block_bank&       blocks,
                                                                                               alloc_desc const& desc,
                                                                                               free_list::iterator prev)
{
  auto size = desc.alignment ? desc.size + desc.alignment - 1 : desc.size;
  if (prev != free_ordering.end())
    return find_free(blocks, std::next(prev), free_ordering.end(), size);
  return free_ordering.end();
}

template <typename traits>
inline std::uint32_t alloc_strategy_impl<alloc_strategy::best_fit, traits>::commit(arena_bank&         arenas,
                                                                                   block_bank&         blocks,
                                                                                   alloc_desc const&   desc,
                                                                                   free_list::iterator found)
{
  auto size = desc.alignment ? desc.size + desc.alignment - 1 : desc.size;
  if (found == free_ordering.end())
  {
    return null();
  }
  std::uint32_t free_node = *found;
  auto&         blk       = blocks[free_node];
  // Marker
  size_type     offset    = blk.offset;
  std::uint32_t arena_num = blk.arena;

  blk.flags &= ~block::f_free_node;

  auto remaining = blk.size - size;

  if (remaining > 0)
  {
    auto& list   = arenas[blk.arena].blocks;
    auto  newblk = blocks.emplace(blk.offset + i_size, remaining, blk.arena, detail::k_null_sz<detail::uhandle>,
                                 block::f_free_node);
    list.insert(free_node + 1, newblk);
    // reinsert the left-over size in free list
    reinsert_left(blocks, found, newblk);
  }
  else
  {
    // delete the existing found index from free list
    free_ordering.erase(found);
  }

  return free_node;
}

template <typename traits>
inline void alloc_strategy_impl<alloc_strategy::best_fit, traits>::add_free_arena(std::uint32_t block)
{
  free_ordering.push_back(block);
}

template <typename traits>
inline void alloc_strategy_impl<alloc_strategy::best_fit, traits>::replace(block_bank& blocks, std::uint32_t block,
                                                                           std::uint32_t new_block, size_type new_size)
{
  size_type size = blocks[block].size;
  if (size == new_size && block == new_block)
    return;

  auto it = find_free(blocks, free_ordering.begin(), free_ordering.end(), size);
  while (*it != block && it != free_ordering.end())
    it++;
  assert(it != free_ordering.end());

  blocks[new_block].size = new_size;
  if (size < new_size)
    reinsert_right(blocks, it, new_block);
  else if (size > new_size)
    reinsert_left(blocks, it, new_block);
  else
    *it = new_block;
}

template <typename traits>
inline void alloc_strategy_impl<alloc_strategy::best_fit, traits>::add_free(block_bank& blocks, std::uint32_t block)
{
  add_free_after(blocks, free_ordering.begin(), block);
}

template <typename traits>
inline void alloc_strategy_impl<alloc_strategy::best_fit, traits>::add_free_after(block_bank&         blocks,
                                                                                  free_list::iterator loc,
                                                                                  std::uint32_t       block)
{
  blocks[block].flags |= block::f_free_node;
  auto it = find_free(blocks, loc, free_ordering.end(), blocks[block].size);
  free_ordering.emplace(it, block);
}

template <typename traits>
inline std::uint32_t alloc_strategy_impl<alloc_strategy::best_fit, traits>::node(free_list::iterator it)
{
  return *it;
}

template <typename traits>
inline bool alloc_strategy_impl<alloc_strategy::best_fit, traits>::is_valid(free_list::iterator it)
{
  return it != free_ordering.end();
}

template <typename traits>
inline void alloc_strategy_impl<alloc_strategy::best_fit, traits>::erase(block_bank& blocks, std::uint32_t node)
{
  auto it = find_free(blocks, loc, free_ordering.end(), blocks[node].size);
  while (*it != node && it != free_ordering.end())
    it++;
  assert(it != free_ordering.end());
  free_ordering.erase(it);
}

template <typename traits>
inline free_list::iterator alloc_strategy_impl<alloc_strategy::best_fit, traits>::find_free(block_bank&         blocks,
                                                                                            free_list::iterator b,
                                                                                            free_list::iterator e,
                                                                                            size_type           i_size)
{
  return std::lower_bound(b, e, i_size, [&blocks](std::uint32_t block, size_type i_size) -> bool {
    return blocks[block].size < i_size;
  });
}

template <typename traits>
inline free_list::iterator alloc_strategy_impl<alloc_strategy::best_fit, traits>::reinsert_left(block_bank& blocks,
                                                                                                free_list::iterator of,
                                                                                                std::uint32_t node)
{
  auto begin = free_list.begin();
  auto it    = find_free(blocks, begin, of, node);
  if (it != of)
  {
    std::uint32_t* src   = &*it;
    std::uint32_t* dest  = src + 1;
    size_t         count = std::distance(it, of);
    std::memmove(dest, src, count * sizeof(std::uint32_t));
    *it = node;
    return it;
  }
  else
  {
    *it = node;
    return it;
  }
}

template <typename traits>
inline free_list::iterator alloc_strategy_impl<alloc_strategy::best_fit, traits>::reinsert_right(block_bank& blocks,
                                                                                                 free_list::iterator of,
                                                                                                 std::uint32_t node)
{

  auto end  = free_ordering.end();
  auto next = std::next(of);
  auto it   = find_free(blocks, next, end, node);
  if (it != next)
  {
    std::uint32_t* dest  = &*node;
    std::uint32_t* src   = dest + 1;
    size_t         count = std::distance(next, it);
    std::memmove(dest, src, count * sizeof(std::uint32_t));
    auto ptr = (dest + count);
    *ptr     = node;
    return free_list.begin() + std::distance(free_list.data(), ptr);
  }
  else
  {
    *it = node;
    return it;
  }
}

template <typename traits>
using alloc_strategy = alloc_strategy_impl<traits::strategy, traits>;

//  -█████╗-██████╗-███████╗███╗---██╗-█████╗----------█████╗-██╗-----██╗------██████╗--██████╗-█████╗-████████╗-██████╗-██████╗-
//  ██╔══██╗██╔══██╗██╔════╝████╗--██║██╔══██╗--------██╔══██╗██║-----██║-----██╔═══██╗██╔════╝██╔══██╗╚══██╔══╝██╔═══██╗██╔══██╗
//  ███████║██████╔╝█████╗--██╔██╗-██║███████║--------███████║██║-----██║-----██║---██║██║-----███████║---██║---██║---██║██████╔╝
//  ██╔══██║██╔══██╗██╔══╝--██║╚██╗██║██╔══██║--------██╔══██║██║-----██║-----██║---██║██║-----██╔══██║---██║---██║---██║██╔══██╗
//  ██║--██║██║--██║███████╗██║-╚████║██║--██║███████╗██║--██║███████╗███████╗╚██████╔╝╚██████╗██║--██║---██║---╚██████╔╝██║--██║
//  ╚═╝--╚═╝╚═╝--╚═╝╚══════╝╚═╝--╚═══╝╚═╝--╚═╝╚══════╝╚═╝--╚═╝╚══════╝╚══════╝-╚═════╝--╚═════╝╚═╝--╚═╝---╚═╝----╚═════╝-╚═╝--╚═╝
//  -----------------------------------------------------------------------------------------------------------------------------
template <typename traits>
class arena_allocator : public detail::statistics<detail::arena_allocator_tag, traits::k_compute_stats>
{
  using size_type      = typename traits::size_type;
  using block          = detail::block<traits>;
  using block_bank     = detail::block_bank<traits>;
  using block_accessor = detail::block_accessor<traits>;
  using block_list     = detail::block_list<traits>;
  using arena_bank     = detail::arena_bank<traits>;
  using arena_list     = detail::arena_list<traits>;
  using statistics     = detail::statistics<detail::arena_allocator_tag, traits::k_compute_stats>;
  using strategy       = detail::alloc_strategy<traits>;
  using alloc_desc     = cppalloc::alloc_desc<size_type>;
  using arena_manager  = typename traits::manager;

public:
  using option_flags = std::uint32_t;
  using address      = std::uint32_t;

  struct alloc_info
  {
    detail::uhandle harena = detail::k_null_sz<detail::uhandle>;
    size_type       offset = detail::k_null_sz<size_type>;
    address         halloc = detail::k_null_32;
  };

  template <typename... Args>
  arena_allocator(size_type i_arena_size, arena_manager& i_manager, Args&&...);

  //! Allocate
  alloc_info allocate(alloc_desc const& desc);
  //! Deallocate, size is optional
  void deallocate(address i_address, size_type size);

  inline static constexpr address null()
  {
    return detail::k_null_32;
  }

private:
  std::pair<std::uint32_t, std::uint32_t> add_arena(detail::uhandle i_handle, size_type i_arena_size);
  void                                    defragment();

  block_bank                     blocks;
  arena_bank                     arenas;
  arena_list                     arena_ordering;
  arena_manager&                 manager;
  size_type                      arena_size;
  size_type                      free_size = 0;
  [[no_unique_address]] strategy strat;
};

template <typename traits>
template <typename... Args>
inline arena_allocator<traits>::arena_allocator(size_type i_arena_size, arena_manager& i_manager, Args&&... i_args)
    : statistics(std::forward<Args>(i_args)...), manager(i_manager), arena_size(i_arena_size)
{
  // False node
  blocks.emplace(arena_size, detail::k_null_32);
}

template <typename traits>
inline std::pair<std::uint32_t, std::uint32_t> arena_allocator<traits>::add_arena(detail::uhandle i_handle,
                                                                                  size_type       i_arena_size)
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

  if (i_handle == detail::k_null_sz<detail::uhandle>)
  {
    block_ref.flags |= block::f_free_node;
    arena_ref.free = i_arena_size;
    strat.add_free_arena(block_id);
    free_size += i_arena_size;
  }
  else
  {
    arena_ref.free = 0;
  }
  arena_ref.blocks.push_back(blocks, block_id);
  if (i_arena_size == arena_size)
    arena_ref.blocks.push_back(blocks, 0);
  else
  {
    // create a false node
    arena_ref.blocks.push_back(blocks, blocks.emplace(i_arena_size, arena_id));
  }
  return std::make_pair(arena_id, block_id);
}

template <typename traits>
inline void arena_allocator<traits>::defragment()
{
  std::uint32_t arena_id = arena_ordering.first;
  strategy      new_strat;

  struct memory_move
  {
    size_type     from;
    size_type     to;
    size_type     size;
    std::uint32_t arena_src;
    std::uint32_t arena_dst;
  };

  std::vector<std::pair<std::uint32_t, std::uint32_t>> rebinds;
  std::vector<memory_move>                             moves;

  while (arena_id != k_null_32)
  {
    auto&         arena      = arenas[arena_id];
    std::uint32_t curr_blk   = arena.blocks.first;
    std::uint32_t free_start = detail::k_null_32;
    std::uint32_t num_free   = 0;

    while (crr_blk != k_null_32)
    {
      auto& blk = blocks[curr_blk];
      if (blk.flags & block::f_free_node)
      {
        if (free_start != k_null_32)
          num_free++;
        else
          free_start = curr_blk;
      }
      else
      {
        auto id = new_strat.try_allocate(arenas, blocks, blk.size);
        if (new_strat.is_valid(id))
        {
          auto new_blk = new_start.commit(arenas, blocks, id);
          rebinds.emplace_back(curr_blk, new_blk);
        }
      }
    }

    arena_id = Accessor::node(cont, arena_id).next;
  }
}

template <typename traits>
inline typename arena_allocator<traits>::alloc_info arena_allocator<traits>::allocate(alloc_desc const& desc)
{

  auto measure = statistics::report_allocate(desc.size);

  assert(desc.huser != detail::k_null_64);
  if (desc.flags & f_dedicated_arena || desc.size >= arena_size)
  {
    return add_arena(desc.huser, desc.size).second;
  }

  std::uint32_t id = strat.commit(arenas, blocks, desc, strat.try_allocate(arenas, blocks, desc));
  if (id == null())
  {
    if (i_options & f_defrag)
    {
      defragment();
      id = strat.commit(arenas, blocks, desc, strat.try_allocate(arenas, blocks, desc));
    }

    if (id == null())
    {
      add_arena(detail::k_null_sz<detail::uhandle>, arena_size);
      id = strat.commit(arenas, blocks, desc, strat.try_allocate(arenas, blocks, desc));
    }
  }

  if (id == null())
    return alloc_info();

  auto& blk        = blocks[id];
  blk.data         = desc.huser;
  blk.alignment    = 0;
  size_type offset = blk.offset;
  if (desc.alignment)
  {
    auto alignment = desc.alignment - 1;
    blk.alignment  = static_cast<std::uint8_t>(std::popcount(alignment));
    offset         = ((blk.offset + alignment) & ~alignment);
  }

  arenas[blk.arena].free -= size;
  free_size -= size;

  return alloc_info{.harena = arenas[blk.arena].data, .offset = offset, .halloc = id};
}

template <typename traits>
inline void arena_allocator<traits>::deallocate(address node, size_type size)
{
  auto measure = statistics::report_deallocate(size);

  enum
  {
    f_left  = 1 << 0,
    f_right = 1 << 1,
  };

  enum merge_type
  {
    e_none,
    e_left,
    e_right,
    e_left_and_right
  };

  auto& blk       = blocks[node];
  auto& arena     = arenas[blk.arena];
  auto& node_list = arena.blocks;

  // last index is not used
  free_size += blk.size;
  arena.free += blk.size;
  size = blk.size;

  std::uint32_t left = detail::k_null_32, right = detail::k_null_32;
  std::uint32_t merges = 0;

  if (node != node_list.begin() && strategy::is_free(blocks[blk.arena_order.prev]))
  {
    left = blk.arena_order.prev;
    merges |= f_left;
  }

  if (node != node_list.end() && strategy::is_free(blocks[blk.arena_order.next]))
  {
    right = blk.arena_order.next;
    merges |= f_right;
  }

  if (arena.free == arena.size && manager.drop_arena(arena.data))
  {
    // drop arena?
    if (left != k_null_32)
      strat.erase_free_rec(left);
    if (right != k_null_32)
      strat.erase_free_rec(right);

    std::uint32_t arena_id = blk.arena;
    free_size -= arena.size;
    arena.size = 0;
    arena.blocks.clear(blocks);
    arena_ordering.erase(arenas, arena_id);
    return;
  }

  switch (merges)
  {
  case merge_type::e_none:
    strat.add_free(blocks, node);
    break;
  case merge_type::e_left:
    strat.replace(blocks, left, left, blocks[left].size + size);
    node_list.earse(blocks, node);
    break;
  case merge_type::e_right:
    strat.replace(blocks, right, node, blocks[right].size + size);
    node_list.earse(blocks, right);
    break;
  case merge_type::e_left_and_right:
    strat.erase(blocks, right);
    strat.replace(blocks, left, left, blocks[left].size + blocks[right].size + size);
    node_list.earse2(blocks, node);
  }
}

} // namespace detail
} // namespace cppalloc