#if defined(__clang__)
#define CPPALLOC_FUNC_NAME  __PRETTY_FUNCTION__
#define CPPALLOC_FUNC_START sizeof("detail::const_string_typeh cppalloc::detail::type_name() [T = ") - 1
#define CPPALLOC_FUNC_END   sizeof("]") - 1
#elif defined(__GNUC__) || defined(__GNUG__)
#define CPPALLOC_FUNC_NAME __PRETTY_FUNCTION__
#define CPPALLOC_FUNC_START                                                                                            \
  sizeof("constexpr cppalloc::detail::const_string_typeh "                                                             \
         "cppalloc::detail::type_name() [with T = ") -                                                                 \
      1
#define CPPALLOC_FUNC_END sizeof("]") - 1
#elif defined(_MSC_VER)
#define CPPALLOC_FUNC_NAME __FUNCSIG__
#define CPPALLOC_FUNC_START                                                                                            \
  sizeof("struct cppalloc::detail::const_string_typeh __cdecl "                                                        \
         "cppalloc::detail::type_name<") -                                                                             \
      1
#define CPPALLOC_FUNC_END sizeof(">(void)") - 1
#endif

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace cppalloc
{

namespace detail
{
struct const_string_typeh
{
  template <std::size_t N>
  constexpr explicit const_string_typeh(const char (&a)[N]) : p(a), sz(N - 1)
  {
  }
  constexpr const_string_typeh(char const* a, std::size_t const N) : p(a), sz(N) {}

  constexpr char operator[](std::size_t n) const
  {
    return n < sz ? p[n] : (char)0;
  }

  [[nodiscard]] constexpr std::size_t size() const
  {
    return sz;
  }
  [[nodiscard]] constexpr char const* name() const
  {
    return p;
  }
  [[nodiscard]] constexpr const_string_typeh substring(const std::size_t start, const std::size_t end) const
  {
    return const_string_typeh(p + start, size() - (end + start));
  }
  [[nodiscard]] constexpr std::uint32_t hash() const
  {
    return compute(p, sz - 1);
  }

  static constexpr std::uint32_t compute(char const* const s, std::size_t count)
  {
    return ((count ? compute(s, count - 1) : 2166136261u) ^ static_cast<std::uint8_t>(s[count])) * 16777619u;
  }

  inline constexpr explicit operator std::string_view() const
  {
    return std::string_view(p, sz);
  }

  char const*       p;
  std::size_t const sz;
};

template <typename T>
constexpr detail::const_string_typeh type_name()
{
  return detail::const_string_typeh{CPPALLOC_FUNC_NAME}.substring(CPPALLOC_FUNC_START, CPPALLOC_FUNC_END);
}

template <typename T>
constexpr std::uint32_t type_hash()
{
  return detail::const_string_typeh{CPPALLOC_FUNC_NAME}.substring(CPPALLOC_FUNC_START, CPPALLOC_FUNC_END).hash();
}
} // namespace detail

template <typename T>
constexpr std::string_view type_name()
{
  return std::string_view(detail::type_name<std::remove_cv_t<std::remove_reference_t<T>>>().name(),
                          detail::type_name<std::remove_cv_t<std::remove_reference_t<T>>>().size());
}

template <typename T>
constexpr std::uint32_t type_hash()
{
  return detail::type_hash<std::remove_cv_t<std::remove_reference_t<T>>>();
}
} // namespace cppalloc
