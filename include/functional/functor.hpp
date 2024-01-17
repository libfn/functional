// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_FUNCTOR
#define INCLUDE_FUNCTIONAL_FUNCTOR

#include "functional/detail/concepts.hpp"
#include "functional/detail/not_tuple.hpp"
#include "functional/utility.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace fn {

template <typename Functor, typename V, typename... Args>
concept monadic_invocable
    = some_monadic_type<V> //
      && std::invocable<typename Functor::apply, V, Args...>;

template <typename Functor, typename... Args> struct functor final {
  using functor_type = Functor;
  constexpr static unsigned size = sizeof...(Args);
  using data_t = not_tuple<as_value_t<Args>...>;
  data_t data;

  static_assert(sizeof...(Args) > 0); // NOTE Consider relaxing
  static_assert(std::is_empty_v<Functor>
                && std::is_empty_v<typename Functor::apply>
                && std::is_default_constructible_v<Functor>
                && std::is_default_constructible_v<typename Functor::apply>);

  friend auto operator|(some_monadic_type auto &&v, auto &&self) noexcept
      -> decltype(auto)
    requires std::same_as<std::remove_cvref_t<decltype(self)>, functor>
             && monadic_invocable<functor_type, decltype(v), Args...>
  {
    constexpr static typename functor_type::apply fn{};
    return data_t::invoke(FWD(self).data, FWD(fn), FWD(v));
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_FUNCTOR
