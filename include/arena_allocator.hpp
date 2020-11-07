
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
struct memory_manager_adapter
{
  static bool drop_arena([[maybe_unused]] uhandle id)
  {
    return true;
  }

  static uhandle add_arena([[maybe_unused]] ihandle id, [[maybe_unused]] size_type size)
  {
    return id;
  }

  static void begin_defragment() {}
  static void end_defragment() {}

  template <typename iter_type>
  static void move_memory([[maybe_unused]] uhandle src_arena, [[maybe_unused]] uhandle dst_arena,
                          [[maybe_unused]] size_type from, [[maybe_unused]] size_type to, size_type size)
  {
  }
};

//  -█████╗-██╗-----██╗------██████╗--██████╗--------██████╗-███████╗███████╗-██████╗
//  ██╔══██╗██║-----██║-----██╔═══██╗██╔════╝--------██╔══██╗██╔════╝██╔════╝██╔════╝
//  ███████║██║-----██║-----██║---██║██║-------------██║--██║█████╗--███████╗██║-----
//  ██╔══██║██║-----██║-----██║---██║██║-------------██║--██║██╔══╝--╚════██║██║-----
//  ██║--██║███████╗███████╗╚██████╔╝╚██████╗███████╗██████╔╝███████╗███████║╚██████╗
//  ╚═╝--╚═╝╚══════╝╚══════╝-╚═════╝--╚═════╝╚══════╝╚═════╝-╚══════╝╚══════╝-╚═════╝
//  ---------------------------------------------------------------------------------
template <typename size_type>
class alloc_desc
{
public:
  alloc_desc(size_type isize) : size_(isize) {}
  alloc_desc(size_type isize, size_type ialignment) : alloc_desc(isize), alignment_(ialignment - 1) {}
  alloc_desc(size_type isize, size_type ialignment, uhandle ihuser) : alloc_desc(isize, ialignment), huser_(ihuser) {}
  alloc_desc(size_type isize, size_type ialignment, uhandle ihuser, alloc_options iflags)
      : alloc_desc(isize, ialignment, ihuser), flags_(iflags)
  {
  }

  size_type size() const
  {
    return size;
  }

  size_type alignment_mask() const
  {
    return alignment_mask_;
  }

  uhandle huser() const
  {
    return huser_;
  }

  alloc_options flags() const
  {
    return flags_;
  }

  size_type adjusted_size() const
  {
    return size() + alignment_mask();
  }

private:
  size_type     size_           = 0;
  size_type     alignment_mask_ = 0;
  uhandle       huser_          = 0;
  alloc_options flags_          = 0;
};

// -█████╗-██╗-----██╗------██████╗--██████╗--------██╗███╗---██╗███████╗-██████╗-
// ██╔══██╗██║-----██║-----██╔═══██╗██╔════╝--------██║████╗--██║██╔════╝██╔═══██╗
// ███████║██║-----██║-----██║---██║██║-------------██║██╔██╗-██║█████╗--██║---██║
// ██╔══██║██║-----██║-----██║---██║██║-------------██║██║╚██╗██║██╔══╝--██║---██║
// ██║--██║███████╗███████╗╚██████╔╝╚██████╗███████╗██║██║-╚████║██║-----╚██████╔╝
// ╚═╝--╚═╝╚══════╝╚══════╝-╚═════╝--╚═════╝╚══════╝╚═╝╚═╝--╚═══╝╚═╝------╚═════╝-
// -------------------------------------------------------------------------------
template <typename size_type>
struct alloc_info
{
  uhandle   harena = detail::k_null_sz<uhandle>;
  size_type offset = detail::k_null_sz<size_type>;
  ihandle   halloc = detail::k_null_32;
  alloc_info()     = default;
  alloc_info(uhandle ihandle, size_type ioffset, address ihalloc) : harena(ihandle), offset(ioffset), ihalloc(halloc) {}
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
// ███╗---███╗███████╗███╗---███╗-██████╗-██████╗-██╗---██╗-----███╗---███╗-██████╗-██╗---██╗███████╗
// ████╗-████║██╔════╝████╗-████║██╔═══██╗██╔══██╗╚██╗-██╔╝-----████╗-████║██╔═══██╗██║---██║██╔════╝
// ██╔████╔██║█████╗--██╔████╔██║██║---██║██████╔╝-╚████╔╝------██╔████╔██║██║---██║██║---██║█████╗--
// ██║╚██╔╝██║██╔══╝--██║╚██╔╝██║██║---██║██╔══██╗--╚██╔╝-------██║╚██╔╝██║██║---██║╚██╗-██╔╝██╔══╝--
// ██║-╚═╝-██║███████╗██║-╚═╝-██║╚██████╔╝██║--██║---██║███████╗██║-╚═╝-██║╚██████╔╝-╚████╔╝-███████╗
// ╚═╝-----╚═╝╚══════╝╚═╝-----╚═╝-╚═════╝-╚═╝--╚═╝---╚═╝╚══════╝╚═╝-----╚═╝-╚═════╝---╚═══╝--╚══════╝
// --------------------------------------------------------------------------------------------------
template <typename traits>
struct memory_move
{
  using size_type = typename traits::size_type;

  size_type     from;
  size_type     to;
  size_type     size;
  std::uint32_t arena_src;
  std::uint32_t arena_dst;

  inline bool is_moved() const;

  memory_move() = default;
  memory_move(size_type ifrom, size_type ito, size_type isize, std::uint32_t iarena_src, std::uint32_t iarena_dst)
      : from(ifrom), to(ito), size(isize), arena_src(iarena_src), arena_dst(iarena_dst)
  {
  }
};

template <typename traits>
inline bool memory_move<traits>::is_moved() const
{
  return (from != to || arena_src != arena_dst);
}

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
  using size_type = typename traits::size_type;
  using extension = typename traits::extension;

  size_type                       offset      = detail::k_null_sz<size_type>;
  size_type                       size        = detail::k_null_sz<size_type>;
  uhandle                         data        = detail::k_null_sz<uhandle>;
  std::uint32_t                   arena       = detail::k_null_32;
  detail::list_node               arena_order = detail::list_node();
  bool                            is_free     = false;
  std::uint8_t                    alignment   = 0;
  std::uint8_t                    reserved    = 0;
  bool                            is_flagged  = false;
  [[no_unique_address]] extension ext;

  block() {}
  block(size_type i_offset, size_type i_size, std::uint32_t i_arena) : offset(i_offset), size(i_size), arena(i_arena) {}
  block(size_type i_offset, size_type i_size, std::uint32_t i_arena, uhandle i_data)
      : block(i_offset, i_size, i_arena), data(i_data)
  {
  }
  block(size_type ioffset, size_type isize, std::uint32_t iarena, uhandle idata, bool ifree)
      : block(ioffset, isize, iarena), data(idata), is_free(ifree)
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
  using container  = bank_type;

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
using block_list = detail::vlist<block_accessor<traits>>;

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

  block_list<traits> block_order;
  detail::list_node  order;
  size_type          size;
  size_type          free;
  uhandle            data = detail::k_null_sz<uhandle>;
};

template <typename traits>
using arena_bank = detail::table<detail::arena<traits>>;

template <typename traits>
struct arena_accessor
{
  using value_type = block<traits>;
  using bank_type  = arena_bank<traits>;
  using size_type  = typename traits::size_type;
  using container  = bank_type;

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
using arena_list = detail::vlist<arena_accessor<traits>>;

using free_list = std::vector<std::uint32_t>;

template <alloc_strategy strategy, typename traits>
class alloc_strategy_impl;

template <alloc_strategy strategy>
struct block_extension
{
};

template <typename traits>
using alloc_strategy_type = alloc_strategy_impl<traits::strategy, traits>;

// ██████╗--█████╗-███╗---██╗██╗--██╗--------██████╗--█████╗-████████╗-█████╗-
// ██╔══██╗██╔══██╗████╗--██║██║-██╔╝--------██╔══██╗██╔══██╗╚══██╔══╝██╔══██╗
// ██████╔╝███████║██╔██╗-██║█████╔╝---------██║--██║███████║---██║---███████║
// ██╔══██╗██╔══██║██║╚██╗██║██╔═██╗---------██║--██║██╔══██║---██║---██╔══██║
// ██████╔╝██║--██║██║-╚████║██║--██╗███████╗██████╔╝██║--██║---██║---██║--██║
// ╚═════╝-╚═╝--╚═╝╚═╝--╚═══╝╚═╝--╚═╝╚══════╝╚═════╝-╚═╝--╚═╝---╚═╝---╚═╝--╚═╝
// ---------------------------------------------------------------------------
template <typename traits>
struct bank_data
{
  block_bank<traits>                                        blocks;
  arena_bank<traits>                                        arenas;
  arena_list<traits>                                        arena_order;
  [[no_unique_address]] detail::alloc_strategy_type<traits> strat;
  typename traits::size_type                                free_size = 0;
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
  using bank_data  = detail::bank_data<traits>;

  inline free_list::iterator try_allocate(bank_data& bank, size_type size);
  inline free_list::iterator try_allocate(bank_data& bank, size_type size, free_list::iterator from);
  inline std::uint32_t       commit(bank_data& bank, size_type size, free_list::iterator);

  inline void add_free_arena(std::uint32_t block);
  inline void add_free(block_bank& blocks, std::uint32_t block);
  inline void add_free_after(block_bank& blocks, free_list::iterator loc, std::uint32_t block);

  inline void replace(block_bank& blocks, std::uint32_t block, std::uint32_t new_block, size_type new_size);

  inline std::uint32_t node(free_list::iterator it);
  inline bool          is_valid(free_list::iterator it);

  inline void erase(block_bank& blocks, std::uint32_t node);

  inline std::uint32_t total_free_nodes() const;
  inline size_type     total_free_size(block_bank& blocks) const;

  void validate_integrity(block_bank& blocks);

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
inline free_list::iterator alloc_strategy_impl<alloc_strategy::best_fit, traits>::try_allocate(bank_data& bank,
                                                                                               size_type  size)
{
  if (free_ordering.size() == 0 || bank.blocks[free_ordering.back()] < size)
    return free_ordering.end();
  return find_free(bank.blocks, free_ordering.begin(), free_ordering.end(), size);
}

template <typename traits>
inline free_list::iterator alloc_strategy_impl<alloc_strategy::best_fit, traits>::try_allocate(bank_data&          bank,
                                                                                               size_type           size,
                                                                                               free_list::iterator prev)
{
  return find_free(bank.blocks, prev, free_ordering.end(), size);
}

template <typename traits>
inline std::uint32_t alloc_strategy_impl<alloc_strategy::best_fit, traits>::commit(bank_data& bank, size_type size,
                                                                                   free_list::iterator found)
{
  if (found == free_ordering.end())
  {
    return null();
  }
  std::uint32_t free_node = *found;
  auto&         blk       = bank.blocks[free_node];
  // Marker
  size_type     offset    = blk.offset;
  std::uint32_t arena_num = blk.arena;

  blk.is_free = false;

  auto remaining = blk.size - size;

  if (remaining > 0)
  {
    auto& list   = bank.arenas[blk.arena].blocks;
    auto  newblk = bank.blocks.emplace(blk.offset + i_size, remaining, blk.arena, detail::k_null_sz<uhandle>, true);
    list.insert(free_node + 1, newblk);
    // reinsert the left-over size in free list
    reinsert_left(bank.blocks, found, newblk);
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
  while (it != free_ordering.end() && *it != block)
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
  blocks[block].is_free = true;
  auto it               = find_free(blocks, loc, free_ordering.end(), blocks[block].size);
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
  while (it != free_ordering.end() && *it != node)
    it++;
  assert(it != free_ordering.end());
  free_ordering.erase(it);
}

template <typename traits>
inline std::uint32_t alloc_strategy_impl<alloc_strategy::best_fit, traits>::total_free_nodes() const
{
  return static_cast<std::uint32_t>(free_ordering.size());
}

template <typename traits>
inline typename alloc_strategy_impl<alloc_strategy::best_fit, traits>::size_type alloc_strategy_impl<
    alloc_strategy::best_fit, traits>::total_free_size(block_bank& blocks) const
{
  size_type sz = 0;
  for (auto fn : free_ordering)
  {
    assert(blocks[fn].is_free);
    sz += blocks[fn].size;
  }

  return sz;
}

template <typename traits>
inline void alloc_strategy_impl<alloc_strategy::best_fit, traits>::validate_integrity(block_bank& blocks)
{
  size_type sz = 0;
  for (auto fn : free_ordering)
  {
    assert(sz <= blocks[fn].size);
    sz = blocks[fn].size;
  }
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
  using strategy       = detail::alloc_strategy_type<traits>;
  using memory_move    = detail::memory_move<triats>;
  using alloc_desc     = cppalloc::alloc_desc<size_type>;
  using arena_manager  = typename traits::manager;
  using bank_data      = detail::bank_data<traits>;

public:
  using alloc_info   = cppalloc::alloc_info<size_type>;
  using option_flags = std::uint32_t;
  using address      = std::uint32_t;

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

  // validate
  void validate_integrity();

private:
  std::pair<std::uint32_t, std::uint32_t>        add_arena(uhandle ihandle, size_type iarena_size, bool empty);
  static std::pair<std::uint32_t, std::uint32_t> add_arena(bank_data& ibank, uhandle ihandle, size_type iarena_size,
                                                           bool empty);

  void        defragment();
  size_type   finalize_commit(block& blk, uhandle huser, size_type alignment);
  static void copy(block const& src, block& dst);
  static void push_memmove(std::vector<memory_move>& dst, memory_move value);

  bank_data      bank;
  arena_manager& manager;
  size_type      arena_size;
};

template <typename traits>
template <typename... Args>
inline arena_allocator<traits>::arena_allocator(size_type i_arena_size, arena_manager& i_manager, Args&&... i_args)
    : statistics(std::forward<Args>(i_args)...), manager(i_manager), arena_size(i_arena_size)
{
}

template <typename traits>
inline typename arena_allocator<traits>::alloc_info arena_allocator<traits>::allocate(alloc_desc const& desc)
{
  auto measure = statistics::report_allocate(desc.size());
  auto size    = desc.adjusted_size();

  assert(desc.huser != detail::k_null_64);
  if (desc.flags & f_dedicated_arena || desc.size() >= arena_size)
  {
    return add_arena(desc.huser(), desc.size()).second;
  }

  std::uint32_t id = strat.commit(bank, size, strat.try_allocate(bank, size));
  if (id == null())
  {
    if (i_options & f_defrag)
    {
      defragment();
      id = strat.commit(bank, size, strat.try_allocate(bank, size));
    }

    if (id == null())
    {
      add_arena(detail::k_null_sz<uhandle>, arena_size);
      id = strat.commit(bank, size, strat.try_allocate(bank, size));
    }
  }

  if (id == null())
    return alloc_info();
  auto& blk = bank.blocks[id];
  return alloc_info(bank.arenas[blk.arena].data, finalize_commit(blk, desc.huser(), desc.alignment_mask()), id);
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

  auto& blk       = bank.blocks[node];
  auto& arena     = bank.arenas[blk.arena];
  auto& node_list = arena.blocks;

  // last index is not used
  free_size += blk.size;
  arena.free += blk.size;
  size = blk.size;

  std::uint32_t left = detail::k_null_32, right = detail::k_null_32;
  std::uint32_t merges = 0;

  if (node != node_list.begin() && strategy::is_free(bank.blocks[blk.arena_order.prev]))
  {
    left = blk.arena_order.prev;
    merges |= f_left;
  }

  if (node != node_list.end() && strategy::is_free(bank.blocks[blk.arena_order.next]))
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
    arena.blocks.clear(bank.blocks);
    arena_ordering.erase(bank.arenas, arena_id);
    return;
  }

  switch (merges)
  {
  case merge_type::e_none:
    strat.add_free(bank.blocks, node);
    break;
  case merge_type::e_left:
    strat.replace(bank.blocks, left, left, bank.blocks[left].size + size);
    node_list.earse(blocks, node);
    break;
  case merge_type::e_right:
    strat.replace(bank.blocks, right, node, bank.blocks[right].size + size);
    node_list.earse(blocks, right);
    break;
  case merge_type::e_left_and_right:
    strat.erase(bank.blocks, right);
    strat.replace(bank.blocks, left, left, bank.blocks[left].size + bank.blocks[right].size + size);
    node_list.earse2(bank.blocks, node);
  }
}

template <typename traits>
inline void arena_allocator<traits>::validate_integrity()
{
  std::uint32_t total_free_nodes = 0;
  for (auto arena_it = bank.arena_order.begin(bank.arenas), arena_end_it = bank.arena_order.end(bank.arenas);
       arena_it != arena_end_it; ++arena_it)
  {
    auto& arena           = *arena_it;
    bool  arena_allocated = false;

    for (auto blk_it = arena.block_order.begin(bank.blocks), blk_end_it = arena.block_order.end(bank.blocks);
         blk_it != blk_end_it; ++blk_it)
    {
      auto& blk = *blk_it;
      if ((blk.is_free))
        total_free_nodes++;
    }
  }

  assert(total_free_nodes == bank.strat.total_free_nodes());
  assert(bank.strat.total_free_size() == bank.strat.free_size);

  for (auto arena_it = bank.arena_order.begin(bank.arenas), arena_end_it = bank.arena_order.end(bank.arenas);
       arena_it != arena_end_it; ++arena_it)
  {
    auto&     arena           = *arena_it;
    bool      arena_allocated = false;
    size_type expected_offset = 0;

    for (auto blk_it = arena.block_order.begin(bank.blocks), blk_end_it = arena.block_order.end(bank.blocks);
         blk_it != blk_end_it; ++blk_it)
    {
      auto& blk = *blk_it;
      assert(blk.offset == expected_offset);
      expected_offset += blk.size;
    }
  }

  bank.start.validate_integrity(bank.blocks);
}

template <typename traits>
inline std::pair<std::uint32_t, std::uint32_t> arena_allocator<traits>::add_arena(uhandle   ihandle,
                                                                                  size_type iarena_size, bool empty)
{
  statistics::report_new_arena();
  auto ret                    = add_arena(bank, ihandle, iarena_size, empty);
  bank.arenas[ret.first].data = manager.add_arena(ret.first, iarena_size);
}

template <typename traits>
inline std::pair<std::uint32_t, std::uint32_t> arena_allocator<traits>::add_arena(bank_data& ibank, uhandle ihandle,
                                                                                  size_type iarena_size, bool iempty)
{

  std::uint32_t arena_id  = ibank.arenas.emplace();
  arena&        arena_ref = ibank.arenas[arena_id];
  arena_ref.size          = iarena_size;
  std::uint32_t block_id  = ibank.blocks.emplace();
  block&        block_ref = ibank.blocks[block_id];
  block_ref.offset        = 0;
  block_ref.arena         = arena_id;
  block_ref.data          = ihandle;

  if (iempty)
  {
    block_ref.is_free = true;
    arena_ref.free    = iarena_size;
    ibank.strat.add_free_arena(block_id);
    ibank.free_size += i_arena_size;
  }
  else
  {
    arena_ref.free = 0;
  }
  arena_ref.block_order.push_back(blocks, block_id);
  return std::make_pair(arena_id, block_id);
}

template <typename traits>
inline void arena_allocator<traits>::defragment()
{
  manager.begin_defragment();
  std::uint32_t arena_id = bank.arena_order.first;
  // refresh all banks
  bank_data refresh;

  std::vector<std::uint32_t> rebinds;
  std::vector<memory_move>   moves;

  for (auto arena_it = bank.arena_order.begin(bank.arenas), arena_end_it = bank.arena_order.end(bank.arenas);
       arena_it != arena_end_it; ++arena_it)
  {
    auto& arena           = *arena_it;
    bool  arena_allocated = false;

    for (auto blk_it = arena.block_order.begin(bank.blocks), blk_end_it = arena.block_order.end(bank.blocks);
         blk_it != blk_end_it; ++blk_it)
    {
      auto& blk = *blk_it;
      if (!blk.is_free)
      {
        auto id = refresh.strat.try_allocate(refresh.arenas, refresh.blocks, blk.size);
        if (!refresh.strat.is_valid(id) && !arena_allocated)
        {
          auto p                       = add_arena(refresh, detail::k_null_uh, arena.size, true);
          refresh.arenas[p.first].data = arena.data;
          id                           = refresh.strat.try_allocate(refresh.arenas, refresh.blocks, blk.size);
        }
        assert(refresh.strat.is_valid(id));

        auto  new_blk_id = refresh.strat.commit(refresh.arenas, refresh.blocks, blk.size, id);
        auto& new_blk    = refresh.blocks[new_blk_id];

        copy(blk, new_blk);
        rebinds.emplace_back(new_blk_id);
        push_memmove(moves, memory_move(blk.offset, new_blk.offset, blk.size, curr_blk.arena, new_blk.arena));
      }
    }
  }

  for (auto& m : moves)
    // follow the copy sequence to ensure there is no overwrite
    manager.move_memory(bank.arenas[m.arena_src].data, refresh.arenas[m.arena_dst].data, m.from, m.to, m.size);

  for (auto rb : rebinds)
  {
    auto&     dst_blk        = refresh.blocks[rb];
    size_type alignment_mask = (1 << dst_blk.alignment) - 1;
    size_type offset         = (dst_blk.offset + alignment_mask) & ~alignment_mask;
    manager.rebind_alloc(dst_blk.data, alloc_info(refresh.arenas[dst_blk.arena].data, offset, rb));
  }

  bank = std::move(refresh);

  manager.end_defragment();
}

template <typename traits>
inline typename arena_allocator<traits>::size_type arena_allocator<traits>::finalize_commit(block& blk, uhandle huser,
                                                                                            size_type alignment)
{
  blk.data      = huser;
  blk.alignment = static_cast<std::uint8_t>(std::popcount(alignment));
  bank.arenas[blk.arena].free -= blk.size;
  bank.free_size -= blk.size;
  return ((blk.offset + alignment) & ~alignment);
}

template <typename traits>
inline void arena_allocator<traits>::copy(block& dst, block const& src)
{
  dst.data      = src.data;
  dst.alignment = src.alignment;
}

template <typename traits>
inline void arena_allocator<traits>::push_memmove(std::vector<memory_move>& dst, memory_move value)
{
  if (!value.is_moved())
    return;
  auto can_merge = [](memory_move const& m1, memory_move const& m2) -> bool {
    return ((m1.arena_dst == m2.arena_dst && m1.arena_src == m2.arena_src) &&
            (m1.from + m1.size == m2.from && m1.to + m1.size == m2.to));
  };
  if (dst.empty() || !can_merge(dst.back(), value))
    dst.emplace_back(value);
  else
    dst.back().size += value.size;
}

} // namespace detail
} // namespace cppalloc