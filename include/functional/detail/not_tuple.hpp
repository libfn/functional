#ifndef INCLUDE_FUNCTIONAL_DETAIL_NOT_TUPLE
#define INCLUDE_FUNCTIONAL_DETAIL_NOT_TUPLE

#include "traits.hpp"

#include <type_traits>
#include <utility>

namespace fn::detail {

template <unsigned Size, typename... Args> struct not_tuple;

template <typename Arg0> struct not_tuple<1, Arg0> final {
  constexpr static unsigned size = 1;
  Arg0 v0;

  static_assert(!std::is_rvalue_reference_v<Arg0>);
};

template <typename Arg0, typename Arg1> struct not_tuple<2, Arg0, Arg1> final {
  constexpr static unsigned size = 2;
  Arg0 v0;
  Arg1 v1;

  static_assert(!std::is_rvalue_reference_v<Arg0>
                && !std::is_rvalue_reference_v<Arg1>);
};

template <typename Arg0, typename Arg1, typename Arg2>
struct not_tuple<3, Arg0, Arg1, Arg2> final {
  constexpr static unsigned size = 3;
  Arg0 v0;
  Arg1 v1;
  Arg2 v2;

  static_assert(!std::is_rvalue_reference_v<Arg0>
                && !std::is_rvalue_reference_v<Arg1>
                && !std::is_rvalue_reference_v<Arg2>);
};

template <typename Arg0, typename Arg1, typename Arg2, typename Arg3>
struct not_tuple<4, Arg0, Arg1, Arg2, Arg3> final {
  constexpr static unsigned size = 4;
  Arg0 v0;
  Arg1 v1;
  Arg2 v2;
  Arg3 v3;

  static_assert(!std::is_rvalue_reference_v<Arg0>
                && !std::is_rvalue_reference_v<Arg1>
                && !std::is_rvalue_reference_v<Arg2>
                && !std::is_rvalue_reference_v<Arg3>);
};

template <typename T> constexpr bool is_not_tuple = false;
template <unsigned Size, typename... Args>
constexpr bool is_not_tuple<not_tuple<Size, Args...>> = true;
template <unsigned Size, typename... Args>
constexpr bool is_not_tuple<not_tuple<Size, Args...> const> = true;
template <unsigned Size, typename... Args>
constexpr bool is_not_tuple<not_tuple<Size, Args...> &> = true;
template <unsigned Size, typename... Args>
constexpr bool is_not_tuple<not_tuple<Size, Args...> const &> = true;
template <unsigned Size, typename... Args>
constexpr bool is_not_tuple<not_tuple<Size, Args...> &&> = true;
template <unsigned Size, typename... Args>
constexpr bool is_not_tuple<not_tuple<Size, Args...> const &&> = true;

template <typename T>
concept some_not_tuple = is_not_tuple<T>;

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

template <unsigned I>
auto get(some_not_tuple auto &&v) noexcept -> decltype(auto) //
  requires(I == 2) && (std::decay_t<decltype(v)>::size >= 3)
{
  return apply_const<decltype(v)>(std::forward<decltype(v)>(v).v2);
}

template <unsigned I>
auto get(some_not_tuple auto &&v) noexcept -> decltype(auto) //
  requires(I == 3) && (std::decay_t<decltype(v)>::size >= 4)
{
  return apply_const<decltype(v)>(std::forward<decltype(v)>(v).v3);
}

} // namespace fn::detail

#endif // INCLUDE_FUNCTIONAL_DETAIL_NOT_TUPLE
