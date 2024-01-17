#ifndef INCLUDE_FUNCTIONAL_DETAIL_NOT_TUPLE
#define INCLUDE_FUNCTIONAL_DETAIL_NOT_TUPLE

#include "traits.hpp"

#include <type_traits>
#include <utility>

namespace fn::detail {

template <typename... Args> struct not_tuple;

template <typename Arg0> struct not_tuple<Arg0> final {
  constexpr static unsigned size = 1;
  Arg0 v0;

  static_assert(!std::is_rvalue_reference_v<Arg0>);
};

template <typename Arg0, typename Arg1> struct not_tuple<Arg0, Arg1> final {
  constexpr static unsigned size = 2;
  Arg0 v0;
  Arg1 v1;

  static_assert(!std::is_rvalue_reference_v<Arg0>
                && !std::is_rvalue_reference_v<Arg1>);
};

template <typename T> constexpr bool _is_not_tuple = false;
template <typename... Args>
constexpr bool _is_not_tuple<not_tuple<Args...> &> = true;
template <typename... Args>
constexpr bool _is_not_tuple<not_tuple<Args...> const &> = true;

template <typename T>
concept some_not_tuple = _is_not_tuple<T &>;

template <unsigned I>
auto get(some_not_tuple auto &&v) noexcept -> decltype(auto) //
  requires(I == 0) && (std::decay_t<decltype(v)>::size >= 1)
{
  return apply_const<decltype(v)>(std::forward<decltype(v)>(v).v0);
}

template <unsigned I>
auto get(some_not_tuple auto &&v) noexcept -> decltype(auto) //
  requires(I == 1) && (std::decay_t<decltype(v)>::size >= 2)
{
  return apply_const<decltype(v)>(std::forward<decltype(v)>(v).v1);
}

} // namespace fn::detail

#endif // INCLUDE_FUNCTIONAL_DETAIL_NOT_TUPLE
