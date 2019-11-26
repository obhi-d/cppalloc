#include <type_traits>

#pragma once

namespace cppalloc {
namespace traits {

template <typename underlying_allocator_tag> struct is_static {
	constexpr inline static bool value = false;
};

template <typename underlying_allocator_tag> constexpr static bool is_static_v = is_static< underlying_allocator_tag>::value;

template <typename ua_t, class = std::void_t<>> struct tag {
	using type = void;
};
template <typename ua_t> struct tag<ua_t, std::void_t<typename ua_t::tag>> {
	using type = typename ua_t::tag;
};

template <typename ua_t> using tag_t = typename tag<ua_t>::type;

template <typename ua_t, class = std::void_t<>> struct size_type {
	using type = std::size_t;
};
template <typename ua_t>
struct size_type<ua_t, std::void_t<typename ua_t::size_type>> {
	using type = typename ua_t::size_type;
};

template <typename ua_t> using size_t = typename size_type<ua_t>::type;

} // namespace traits
} // namespace cppalloc
