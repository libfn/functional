// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_OPTIONAL
#define INCLUDE_FUNCTIONAL_OPTIONAL

#include "functional/detail/fwd_macro.hpp"

#include <concepts>
#include <functional>
#include <optional>
#include <type_traits>

namespace fn {

template <typename T> struct optional;

namespace detail {
template <typename T> constexpr bool _is_some_optional = false;
template <typename T> constexpr bool _is_some_optional<::fn::optional<T> &> = true;
template <typename T> constexpr bool _is_some_optional<::fn::optional<T> const &> = true;
} // namespace detail

template <typename T>
concept some_optional = detail::_is_some_optional<T &>;

template <typename T> struct optional final : std::optional<T> {
  using value_type = std::optional<T>::value_type;
  using std::optional<T>::optional;

  // NOTE All the functions below are polyfills. They are needed because monadic
  // operations must return fn::optional rather than std::optional

  template <typename Fn> constexpr auto and_then(Fn &&fn) &
  {
    using type = std::invoke_result_t<Fn, value_type &>;
    static_assert(some_optional<type>);
    if (this->has_value())
      return std::invoke(FWD(fn), this->value());
    else
      return type(std::nullopt);
  }

  template <typename Fn> constexpr auto and_then(Fn &&fn) const &
  {
    using type = std::invoke_result_t<Fn, value_type const &>;
    static_assert(some_optional<type>);
    if (this->has_value())
      return std::invoke(FWD(fn), this->value());
    else
      return type(std::nullopt);
  }

  template <typename Fn> constexpr auto and_then(Fn &&fn) &&
  {
    using type = std::invoke_result_t<Fn, value_type &&>;
    static_assert(some_optional<type>);
    if (this->has_value())
      return std::invoke(FWD(fn), std::move(this->value()));
    else
      return type(std::nullopt);
  }

  template <typename Fn> constexpr auto and_then(Fn &&fn) const &&
  {
    using type = std::invoke_result_t<Fn, value_type const &&>;
    static_assert(some_optional<type>);
    if (this->has_value())
      return std::invoke(FWD(fn), std::move(this->value()));
    else
      return type(std::nullopt);
  }

  template <typename Fn>
    requires std::is_constructible_v<T, T &>
  constexpr auto or_else(Fn &&fn) &
  {
    using type = optional<T>;
    static_assert(some_optional<type>);
    if (this->has_value())
      return type(std::in_place, this->value());
    else
      return std::invoke(FWD(fn));
  }

  template <typename Fn>
    requires std::is_constructible_v<T, T const &>
  constexpr auto or_else(Fn &&fn) const &
  {
    using type = optional<T>;
    static_assert(some_optional<type>);
    if (this->has_value())
      return type(std::in_place, this->value());
    else
      return std::invoke(FWD(fn));
  }

  template <typename Fn>
    requires std::is_constructible_v<T, T>
  constexpr auto or_else(Fn &&fn) &&
  {
    using type = optional<T>;
    static_assert(some_optional<type>);
    if (this->has_value())
      return type(std::in_place, std::move(this->value()));
    else
      return std::invoke(FWD(fn));
  }

  template <typename Fn>
    requires std::is_constructible_v<T, T const>
  constexpr auto or_else(Fn &&fn) const &&
  {
    using type = optional<T>;
    static_assert(some_optional<type>);
    if (this->has_value())
      return type(std::in_place, std::move(this->value()));
    else
      return std::invoke(FWD(fn));
  }

  template <typename Fn> constexpr auto transform(Fn &&fn) &
  {
    using value_type = std::invoke_result_t<Fn, value_type &>;
    using type = optional<value_type>;
    if (this->has_value())
      return type(std::in_place, std::invoke(FWD(fn), this->value()));
    else
      return type(std::nullopt);
  }

  template <typename Fn> constexpr auto transform(Fn &&fn) const &
  {
    using value_type = std::invoke_result_t<Fn, value_type const &>;
    using type = optional<value_type>;
    if (this->has_value())
      return type(std::in_place, std::invoke(FWD(fn), this->value()));
    else
      return type(std::nullopt);
  }

  template <typename Fn> constexpr auto transform(Fn &&fn) &&
  {
    using value_type = std::invoke_result_t<Fn, value_type &&>;
    using type = optional<value_type>;
    if (this->has_value())
      return type(std::in_place, std::invoke(FWD(fn), std::move(this->value())));
    else
      return type(std::nullopt);
  }

  template <typename Fn> constexpr auto transform(Fn &&fn) const &&
  {
    using value_type = std::invoke_result_t<Fn, value_type const &&>;
    using type = optional<value_type>;
    if (this->has_value())
      return type(std::in_place, std::invoke(FWD(fn), std::move(this->value())));
    else
      return type(std::nullopt);
  }
};

template <class T> optional(T) -> optional<T>;

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_OPTIONAL
