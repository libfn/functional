// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_FUNCTOR
#define INCLUDE_FUNCTIONAL_FUNCTOR

#include "functional/detail/concepts.hpp"
#include "functional/detail/not_tuple.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace fn {

template <typename T>
concept some_expected = detail::_is_some_expected<T &>;

template <typename T>
concept some_optional = detail::_is_some_optional<T &>;

template <typename T>
concept some_monadic_type = some_expected<T> || some_optional<T>;

template <typename Functor, typename V, typename... Args>
concept monadic_invocable
    = some_monadic_type<V> //
      && std::invocable<typename Functor::apply, V, Args...>;

template <typename Functor, typename... Args> struct functor final {
  using functor_type = Functor;
  constexpr static unsigned size = sizeof...(Args);
  detail::not_tuple<detail::as_value_t<Args>...> data;

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
    return
        [&v, &self]<unsigned... I>(
            std::integer_sequence<unsigned, I...>) noexcept -> decltype(auto) {
          return Functor::apply::template operator()( //
              std::forward<decltype(v)>(v),           //
              detail::get<I>(std::forward<decltype(self)>(self).data)...);
        }(std::make_integer_sequence<unsigned, size>{});
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_FUNCTOR
