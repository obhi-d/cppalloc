#pragma once
#include <detail/arena_allocator_impl.hpp>

namespace cppalloc
{

//  -█████╗-██████╗-███████╗███╗---██╗-█████╗----------█████╗-██╗-----██╗------██████╗--██████╗-█████╗-████████╗-██████╗-██████╗-
//  ██╔══██╗██╔══██╗██╔════╝████╗--██║██╔══██╗--------██╔══██╗██║-----██║-----██╔═══██╗██╔════╝██╔══██╗╚══██╔══╝██╔═══██╗██╔══██╗
//  ███████║██████╔╝█████╗--██╔██╗-██║███████║--------███████║██║-----██║-----██║---██║██║-----███████║---██║---██║---██║██████╔╝
//  ██╔══██║██╔══██╗██╔══╝--██║╚██╗██║██╔══██║--------██╔══██║██║-----██║-----██║---██║██║-----██╔══██║---██║---██║---██║██╔══██╗
//  ██║--██║██║--██║███████╗██║-╚████║██║--██║███████╗██║--██║███████╗███████╗╚██████╔╝╚██████╗██║--██║---██║---╚██████╔╝██║--██║
//  ╚═╝--╚═╝╚═╝--╚═╝╚══════╝╚═╝--╚═══╝╚═╝--╚═╝╚══════╝╚═╝--╚═╝╚══════╝╚══════╝-╚═════╝--╚═════╝╚═╝--╚═╝---╚═╝----╚═════╝-╚═╝--╚═╝
//  -----------------------------------------------------------------------------------------------------------------------------
template <typename manager_t, typename usize_t = std::size_t, alloc_strategy strategy_v = alloc_strategy::best_fit_tree,
          bool k_compute_stats_v = false>
class arena_allocator : public detail::arena_allocator_impl<
                            detail::arena_allocator_traits<manager_t, usize_t, strategy_v, k_compute_stats_v>>
{
  using traits         = detail::arena_allocator_traits<manager_t, usize_t, strategy_v, k_compute_stats_v>;
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
  using arena_manager  = typename traits::manager;
  using bank_data      = detail::bank_data<traits>;
  using base           = public detail::arena_allocator_impl<traits>;

public:
  using alloc_info = cppalloc::alloc_info<size_type>;
  using alloc_desc = cppalloc::alloc_desc<size_type>;

  template <typename... Args>
  arena_allocator(size_type i_arena_size, arena_manager& i_manager, Args&&... args)
      : base(i_arena_size, i_manager, std::forward<Args>(args)...)
  {
  }
};

} // namespace cppalloc
