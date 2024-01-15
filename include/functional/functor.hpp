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

template <typename V, typename Functor, typename... Args>
concept some_monadic_apply
    = some_monadic_type<V>                                   //
      && std::same_as<std::remove_cvref_t<Functor>, Functor> //
      && requires {
           monadic_apply(
               std::declval<V>(),       //
               std::declval<Functor>(), //
               std::declval<
                   std::add_rvalue_reference_t<detail::as_value_t<Args>>>()...);
           // NOTE, the type transformation on Args does not transform an lvalue
           // into rvalue (because add_rvalue_reference_t respects reference
           // collapsing rules). It is meant to simulate the return type of one
           // of the many possible overloads of detail::get(detail::not_tuple).
         };

template <typename Functor, typename... Args> struct functor final {
  using functor_type = Functor;
  constexpr static unsigned size = sizeof...(Args);
  detail::not_tuple<detail::as_value_t<Args>...> data;

  static_assert(sizeof...(Args) > 0); // NOTE Consider relaxing
  static_assert(std::is_same_v<std::remove_cvref_t<Functor>, Functor>
                && std::is_empty_v<Functor>
                && std::is_default_constructible_v<Functor>);

  friend auto operator|(some_monadic_type auto &&v, auto &&self) noexcept
      -> decltype(auto)
    requires std::same_as<std::remove_cvref_t<decltype(self)>, functor>
             && some_monadic_apply<decltype(v), functor_type, Args...>
  {
    return
        [&v, &self]<unsigned... I>(
            std::integer_sequence<unsigned, I...>) noexcept -> decltype(auto) {
          return monadic_apply(             //
              std::forward<decltype(v)>(v), //
              functor_type{},               //
              detail::get<I>(std::forward<decltype(self)>(self).data)...);
        }(std::make_integer_sequence<unsigned, size>{});
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_FUNCTOR
