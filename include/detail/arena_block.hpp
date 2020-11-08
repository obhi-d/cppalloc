#pragma once
#include <detail/memory_move.hpp>

namespace cppalloc::detail
{

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

} // namespace cppalloc::detail