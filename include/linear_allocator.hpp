#pragma once

#include <default_allocator.hpp>
#include <detail/cppalloc_common.hpp>

namespace cppalloc
{

struct linear_allocator_tag
{
};

template <typename underlying_allocator = cppalloc::default_allocator<>, bool k_compute_stats = false>
class linear_allocator : detail::statistics<linear_allocator_tag, k_compute_stats, underlying_allocator>
{
public:
  using tag        = linear_allocator_tag;
  using statistics = detail::statistics<linear_allocator_tag, k_compute_stats, underlying_allocator>;
  using size_type  = typename underlying_allocator::size_type;
  using address    = typename underlying_allocator::address;
  template <typename... Args>
  linear_allocator(size_type i_arena_size, Args&&... i_args)
      : k_arena_size(i_arena_size), left_over(i_arena_size), statistics(std::forward<Args>(i_args)...)
  {
    statistics::report_new_arena();
    buffer = underlying_allocator::allocate(k_arena_size, 0);
  }

  ~linear_allocator()
  {
    underlying_allocator::deallocate(buffer, k_arena_size);
  }

  address allocate(size_type i_size, size_type i_alignment = 0)
  {
    auto measure = statistics::report_allocate(i_size);
    // assert
    auto fixup = i_alignment - 1;
    if (i_alignment)
      i_size += fixup + 4;

    assert(left_over >= i_size);
    size_type offset = k_arena_size - left_over;
    left_over -= i_size;
    if (i_alignment)
    {
      auto pointer = reinterpret_cast<std::uintptr_t>(buffer) + static_cast<std::uintptr_t>(offset);
      auto ret     = ((pointer + 4 + static_cast<std::uintptr_t>(fixup)) + ~static_cast<std::uintptr_t>(fixup));
      *(reinterpret_cast<std::uint32_t*>(ret) - 1) = static_cast<std::uint32_t>(ret - pointer);
      return reinterpret_cast<address>(ret);
    }
    else
      return reinterpret_cast<std::uint8_t*>(buffer) + offset;
  }

  void deallocate(address i_data, size_type i_size, size_type i_alignment = 0)
  {
    auto measure = statistics::report_deallocate(i_size);

    if (i_alignment)
    {
      i_size += i_alignment + 3;
      std::uint32_t off_by = *(reinterpret_cast<std::uint32_t*>(i_data) - 1);
      i_data               = reinterpret_cast<address>(reinterpret_cast<std::uint8_t*>(i_data) - off_by);
    }
    // merge back?
    size_type new_left_over = left_over + i_size;
    size_type offset        = (k_arena_size - new_left_over);
    if (reinterpret_cast<std::uint8_t*>(buffer) + offset == reinterpret_cast<std::uint8_t*>(i_data))
      left_over = new_left_over;
  }

  size_type get_free_size() const
  {
    return left_over;
  }

private:
  address         buffer;
  size_type       left_over;
  const size_type k_arena_size;
};

} // namespace cppalloc