﻿#pragma once
#include <alloc_desc.hpp>

namespace cppalloc::detail
{
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

} // namespace cppalloc::detail
