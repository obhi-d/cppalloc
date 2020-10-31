#include <detail/cppalloc_common.hpp>
#include <type_traits>

namespace cppalloc::detail
{

template <typename T>
struct table
{
  template <typename... Args>
  std::uint32_t emplace(Args&&... args)
  {
    std::uint32_t index;
    if (unused != k_null_32)
    {
      index  = unused;
      unused = reinterpret_cast<std::uint32_t&>(pool[unused]);
    }
    else
    {
      index = static_cast<std::uint32_t>(pool.size());
      pool.resize(index + 1);
    }
    new (&pool[index]) T(std::forward<Args>(args)...);
  }

  void erase(std::uint32_t index)
  {
    auto& t = reinterpret_cast<T&>(pool[index]);
    t.~T();
    reinterpret_cast<std::uint32_t&>(pool[index]) = unused;
    unused                                        = index;
  }

  T& operator[](std::uint32_t i)
  {
    return reinterpret_cast<T&>(pool[i]);
  }

  T const& operator[](std::uint32_t i) const
  {
    return reinterpret_cast<T&>(pool[i]);
  }

  using storage = std::aligned_storage_t<sizeof(T), alignof(T)>;
  std::vector<storage> pool;
  std::uint32_t        unused = k_null_32;
};

} // namespace cppalloc::detail
