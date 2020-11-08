#pragma once
#include <bit>
#include <detail/cppalloc_common.hpp>
#include <detail/rbtree.hpp>
#include <detail/table.hpp>
#include <detail/vlist.hpp>
#include <type_traits>
#include <utility>

namespace cppalloc
{

enum class alloc_strategy
{
  best_fit,
  best_fit_tree
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
  bool drop_arena([[maybe_unused]] uhandle id)
  {
    return true;
  }

  uhandle add_arena([[maybe_unused]] ihandle id, [[maybe_unused]] size_type size)
  {
    return id;
  }

  void begin_defragment() {}
  void end_defragment() {}

  void move_memory([[maybe_unused]] uhandle src_arena, [[maybe_unused]] uhandle dst_arena,
                   [[maybe_unused]] size_type from, [[maybe_unused]] size_type to, size_type size)
  {
  }
  template <typename alloc_info>
  void rebind_alloc([[maybe_unused]] uhandle halloc, alloc_info info)
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
  alloc_info(uhandle iharena, size_type ioffset, ihandle ihalloc) : harena(iharena), offset(ioffset), ihalloc(halloc) {}
};

} // namespace cppalloc