
#pragma once

#include <memory>

namespace cppalloc
{
namespace detail
{

template <typename T, typename underlying_allocator, bool static_ua>
struct std_allocator_wrapper;

template <typename T, typename underlying_allocator>
class std_allocator_wrapper<T, underlying_allocator, false> : public std::allocator<T>
{
public:
  //    typedefs
  using pointer       = typename std::allocator_traits<std::allocator<T>>::pointer;
  using const_pointer = typename std::allocator_traits<std::allocator<T>>::const_pointer;
  using size_type     = typename std::allocator_traits<std::allocator<T>>::size_type;
  using value_type    = typename std::allocator_traits<std::allocator<T>>::value_type;

  using underlying_size_t = typename underlying_allocator::size_type;

public:
  //    convert an allocator<T> to allocator<U>

  template <typename U>
  struct rebind
  {
    typedef std_allocator_wrapper<U, underlying_allocator, false> other;
  };

public:
  inline explicit std_allocator_wrapper(underlying_allocator& impl) : impl_(&impl) {}

  inline ~std_allocator_wrapper() {}

  /*	inline explicit std_allocator_wrapper(std_allocator_wrapper const&) {}*/
  template <typename U>
  inline std_allocator_wrapper(std_allocator_wrapper<U, underlying_allocator, false> const& other)
      : impl_(other.get_impl())
  {
  }

  //    memory allocation

  inline pointer allocate(size_type cnt, const_pointer = 0) const
  {
    pointer ret = reinterpret_cast<pointer>(impl_->allocate(static_cast<underlying_size_t>(sizeof(T) * cnt)));
    return ret;
  }

  inline void deallocate(pointer p, size_type cnt) const
  {
    impl_->deallocate(p, static_cast<underlying_size_t>(sizeof(T) * cnt));
  }
  //    construction/destruction

  template <typename... Args>
  inline void construct(pointer p, Args&&... args) const
  {
    new (p) T(std::forward<Args>(args)...);
  }

  inline void destroy(pointer p) const
  {
    reinterpret_cast<T*>(p)->~T();
  }
  inline bool operator==(const std_allocator_wrapper& x)
  {
    return impl_ == x.impl_;
  }
  inline bool operator!=(const std_allocator_wrapper& x)
  {
    return impl_ != x.impl_;
  }

  underlying_allocator* get_impl() const
  {
    return impl_;
  }

protected:
  underlying_allocator* impl_;
};

template <typename T, typename underlying_allocator>
class std_allocator_wrapper<T, underlying_allocator, true> : public std::allocator<T>, public underlying_allocator
{
public:
  //    typedefs
  using pointer       = typename std::allocator_traits<std::allocator<T>>::pointer;
  using const_pointer = typename std::allocator_traits<std::allocator<T>>::const_pointer;
  using size_type     = typename std::allocator_traits<std::allocator<T>>::size_type;
  using value_type    = typename std::allocator_traits<std::allocator<T>>::value_type;

  using underlying_size_t = typename underlying_allocator::size_type;

public:
  //    convert an allocator<T> to allocator<U>

  template <typename U>
  struct rebind
  {
    typedef std_allocator_wrapper<U, underlying_allocator, true> other;
  };

public:
  inline std_allocator_wrapper() {}

  inline explicit std_allocator_wrapper(underlying_allocator const& impl) : underlying_allocator(impl) {}
  inline std_allocator_wrapper(std_allocator_wrapper<T, underlying_allocator, true> const& impl)
      : underlying_allocator((underlying_allocator const&)impl)
  {
  }

  inline ~std_allocator_wrapper() {}

  /*	inline explicit std_allocator_wrapper(std_allocator_wrapper const&) {}*/
  template <typename U>
  inline std_allocator_wrapper(std_allocator_wrapper<U, underlying_allocator, true> const& other)
      : underlying_allocator((underlying_allocator&)other)
  {
  }

  //    memory allocation

  inline pointer allocate(size_type cnt, const_pointer = 0) const
  {
    pointer ret =
        reinterpret_cast<pointer>(underlying_allocator::allocate(static_cast<underlying_size_t>(sizeof(T) * cnt)));
    return ret;
  }

  inline void deallocate(pointer p, size_type cnt) const
  {
    underlying_allocator::deallocate(p, static_cast<underlying_size_t>(sizeof(T) * cnt));
  }
  //    construction/destruction

  template <typename... Args>
  inline void construct(pointer p, Args&&... args) const
  {
    new (p) T(std::forward<Args>(args)...);
  }

  inline void destroy(pointer p) const
  {
    reinterpret_cast<T*>(p)->~T();
  }
  inline bool operator==(const std_allocator_wrapper& x)
  {
    return true;
  }
  inline bool operator!=(const std_allocator_wrapper& x)
  {
    return false;
  }
};

} // namespace detail

template <typename T, typename underlying_allocator>
class std_allocator_wrapper
    : public detail::std_allocator_wrapper<T, underlying_allocator,
                                           traits::is_static_v<traits::tag_t<underlying_allocator>>>
{
public:
  //    typedefs
  using base_type =
      detail::std_allocator_wrapper<T, underlying_allocator, traits::is_static_v<traits::tag_t<underlying_allocator>>>;
  using pointer       = typename std::allocator_traits<std::allocator<T>>::pointer;
  using const_pointer = typename std::allocator_traits<std::allocator<T>>::const_pointer;
  using size_type     = typename std::allocator_traits<std::allocator<T>>::size_type;
  using value_type    = typename std::allocator_traits<std::allocator<T>>::value_type;

  using underlying_size_t = typename underlying_allocator::size_type;

  std_allocator_wrapper() = default;

  template <typename... Args>
  inline std_allocator_wrapper(Args&&... args) : base_type(std::forward<Args>(args)...)
  {
  }
};

} // namespace cppalloc