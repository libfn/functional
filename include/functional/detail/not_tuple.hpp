#ifndef INCLUDE_FUNCTIONAL_DETAIL_NOT_TUPLE
#define INCLUDE_FUNCTIONAL_DETAIL_NOT_TUPLE

#include "traits.hpp"

#include <type_traits>
#include <utility>

namespace fn::detail {

template <std::size_t I, typename T> struct _element {
  T v;

  static_assert(not std::is_rvalue_reference_v<T>);
};

template <typename, typename...> struct not_tuple_base;

template <std::size_t... Is, typename... Ts>
struct not_tuple_base<std::index_sequence<Is...>, Ts...> : _element<Is, Ts>... {
  template <typename Self, typename Fn, typename... Args>
  static auto invoke(Self &&self, Fn &&fn, Args &&...args) noexcept
      -> decltype(auto)
    requires std::invocable<
        decltype(fn), decltype(args)...,
        decltype(apply_const_t<Self, Ts>(self._element<Is, Ts>::v))...>
  {
    return std::forward<decltype(fn)>(fn)(
        std::forward<decltype(args)>(args)...,
        apply_const_t<Self, Ts>(self._element<Is, Ts>::v)...);
  }
};

} // namespace fn::detail

#endif // INCLUDE_FUNCTIONAL_DETAIL_NOT_TUPLE
