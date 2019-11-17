
#pragma once

#include <memory>

namespace cppalloc {

template <typename T, typename underlying_allocator>
class std_allocator_wrapper : public std::allocator<T> {
public:
	//    typedefs
	using pointer = typename std::allocator_traits<std::allocator<T>>::pointer;
	using const_pointer =
	    typename std::allocator_traits<std::allocator<T>>::const_pointer;
	using size_type =
	    typename std::allocator_traits<std::allocator<T>>::size_type;
	using value_type =
	    typename std::allocator_traits<std::allocator<T>>::value_type;

	using underlying_size_t = typename underlying_allocator::size_type;
public:
	//    convert an allocator<T> to allocator<U>

	template <typename U> struct rebind {
		typedef std_allocator_wrapper<U, underlying_allocator> other;
	};

public:
	inline explicit std_allocator_wrapper(underlying_allocator& impl)
	    : impl_(&impl) {}

	inline ~std_allocator_wrapper() {}

	/*	inline explicit std_allocator_wrapper(std_allocator_wrapper const&) {}*/
	template <typename U>
	inline std_allocator_wrapper(
	    std_allocator_wrapper<U, underlying_allocator> const& other)
	    : impl_(other.get_impl()) {}

	//    memory allocation

	inline pointer allocate(size_type cnt, const_pointer = 0) const {
		pointer ret = reinterpret_cast<pointer>(
		    impl_->allocate(static_cast<underlying_size_t>(sizeof(T) * cnt)));
		return ret;
	}

	inline void deallocate(pointer p, size_type cnt) const {
		impl_->deallocate(p, static_cast<underlying_size_t>(sizeof(T) * cnt));
	}
	//    construction/destruction

	template <typename... Args>
	inline void construct(pointer p, Args&&... args) const {
		new (p) T(std::forward<Args>(args)...);
	}

	inline void destroy(pointer p) const { reinterpret_cast<T*>(p)->~T(); }
	inline bool operator==(const std_allocator_wrapper& x) {
		return impl_ == x.impl_;
	}
	inline bool operator!=(const std_allocator_wrapper& x) {
		return impl_ != x.impl_;
	}

	underlying_allocator* get_impl() const { return impl_; }
protected:
	underlying_allocator* impl_;
};

} // namespace cppalloc