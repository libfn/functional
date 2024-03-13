// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_OPTIONAL
#define INCLUDE_FUNCTIONAL_OPTIONAL

#include "functional/detail/fwd_macro.hpp"
#include "functional/pack.hpp"
#include "functional/utility.hpp"

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

template <typename T>
concept some_optional_non_pack = //
    some_optional<T>             //
    && !some_pack<typename std::remove_cvref_t<T>::value_type>;

template <typename T>
concept some_optional_pack = //
    some_optional<T>         //
    && some_pack<typename std::remove_cvref_t<T>::value_type>;

template <typename T> struct optional final : std::optional<T> {
  using value_type = std::optional<T>::value_type;

  using std::optional<T>::optional;

  // and_then pack
  template <typename Fn>
  constexpr auto and_then(Fn &&fn) &
    requires some_pack<value_type>
  {
    using type = decltype(this->value().invoke(FWD(fn)));
    static_assert(some_optional<type>);
    if (this->has_value())
      return this->value().invoke(FWD(fn));
    else
      return type(std::nullopt);
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const &
    requires some_pack<value_type>
  {
    using type = decltype(this->value().invoke(FWD(fn)));
    static_assert(some_optional<type>);
    if (this->has_value())
      return this->value().invoke(FWD(fn));
    else
      return type(std::nullopt);
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) &&
    requires some_pack<value_type>
  {
    using type = decltype(std::move(*this).value().invoke(FWD(fn)));
    static_assert(some_optional<type>);
    if (this->has_value())
      return std::move(*this).value().invoke(FWD(fn));
    else
      return type(std::nullopt);
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const &&
    requires some_pack<value_type>
  {
    using type = decltype(std::move(*this).value().invoke(FWD(fn)));
    static_assert(some_optional<type>);
    if (this->has_value())
      return std::move(*this).value().invoke(FWD(fn));
    else
      return type(std::nullopt);
  }

  // transform pack
  template <typename Fn>
  constexpr auto transform(Fn &&fn) &
    requires some_pack<value_type>
  {
    using value_type = decltype(this->value().invoke(FWD(fn)));
    using type = optional<value_type>;
    if (this->has_value())
      return type(std::in_place, this->value().invoke(FWD(fn)));
    else
      return type(std::nullopt);
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const &
    requires some_pack<value_type>
  {
    using value_type = decltype(this->value().invoke(FWD(fn)));
    using type = optional<value_type>;
    if (this->has_value())
      return type(std::in_place, this->value().invoke(FWD(fn)));
    else
      return type(std::nullopt);
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) &&
    requires some_pack<value_type>
  {
    using value_type = decltype(std::move(*this).value().invoke(FWD(fn)));
    using type = optional<value_type>;
    if (this->has_value())
      return type(std::in_place, std::move(*this).value().invoke(FWD(fn)));
    else
      return type(std::nullopt);
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const &&
    requires some_pack<value_type>
  {
    using value_type = decltype(std::move(*this).value().invoke(FWD(fn)));
    using type = optional<value_type>;
    if (this->has_value())
      return type(std::in_place, std::move(*this).value().invoke(FWD(fn)));
    else
      return type(std::nullopt);
  }

  // NOTE All the functions below are polyfills. They are needed because monadic
  // operations must return fn::optional rather than std::optional

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) &
    requires(not some_pack<value_type>)
  {
    using type = std::invoke_result_t<Fn, value_type &>;
    static_assert(some_optional<type>);
    if (this->has_value())
      return std::invoke(FWD(fn), this->value());
    else
      return type(std::nullopt);
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const &
    requires(not some_pack<value_type>)
  {
    using type = std::invoke_result_t<Fn, value_type const &>;
    static_assert(some_optional<type>);
    if (this->has_value())
      return std::invoke(FWD(fn), this->value());
    else
      return type(std::nullopt);
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) &&
    requires(not some_pack<value_type>)
  {
    using type = std::invoke_result_t<Fn, value_type &&>;
    static_assert(some_optional<type>);
    if (this->has_value())
      return std::invoke(FWD(fn), std::move(this->value()));
    else
      return type(std::nullopt);
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const &&
    requires(not some_pack<value_type>)
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

  template <typename Fn>
  constexpr auto transform(Fn &&fn) &
    requires(not some_pack<value_type>)
  {
    using value_type = std::invoke_result_t<Fn, value_type &>;
    using type = optional<value_type>;
    if (this->has_value())
      return type(std::in_place, std::invoke(FWD(fn), this->value()));
    else
      return type(std::nullopt);
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const &
    requires(not some_pack<value_type>)
  {
    using value_type = std::invoke_result_t<Fn, value_type const &>;
    using type = optional<value_type>;
    if (this->has_value())
      return type(std::in_place, std::invoke(FWD(fn), this->value()));
    else
      return type(std::nullopt);
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) &&
    requires(not some_pack<value_type>)
  {
    using value_type = std::invoke_result_t<Fn, value_type &&>;
    using type = optional<value_type>;
    if (this->has_value())
      return type(std::in_place, std::invoke(FWD(fn), std::move(this->value())));
    else
      return type(std::nullopt);
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const &&
    requires(not some_pack<value_type>)
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

template <some_optional Lh, some_optional Rh>
  requires(not some_pack<typename std::remove_cvref_t<Lh>::value_type>)
          && (not some_pack<typename std::remove_cvref_t<Rh>::value_type>)
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using lh_type = std::remove_cvref_t<Lh>::value_type;
  using rh_type = std::remove_cvref_t<Rh>::value_type;
  using value_type = pack<lh_type, rh_type>;
  using type = optional<value_type>;
  if (lh.has_value() && rh.has_value())
    return type{std::in_place, pack<lh_type>{FWD(lh).value()}.append(std::in_place_type_t<rh_type>{}, FWD(rh).value())};
  else if (not lh.has_value())
    return type{std::nullopt};
  else
    return type{std::nullopt};
}

template <some_optional Lh, some_optional Rh>
  requires some_pack<typename std::remove_cvref_t<Lh>::value_type>
           && (not some_pack<typename std::remove_cvref_t<Rh>::value_type>)
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using lh_type = std::remove_cvref_t<Lh>::value_type;
  using rh_type = std::remove_cvref_t<Rh>::value_type;
  using value_type = typename lh_type::template append_type<rh_type>;
  using type = optional<value_type>;
  if (lh.has_value() && rh.has_value())
    return type{std::in_place, FWD(lh).value().append(std::in_place_type_t<rh_type>{}, FWD(rh).value())};
  else if (not lh.has_value())
    return type{std::nullopt};
  else
    return type{std::nullopt};
}

template <some_optional Lh, some_optional Rh>
  requires(not some_pack<typename std::remove_cvref_t<Lh>::value_type>)
              && some_pack<typename std::remove_cvref_t<Rh>::value_type>
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept = delete;

template <some_optional Lh, some_optional Rh>
  requires some_pack<typename std::remove_cvref_t<Lh>::value_type>
               && some_pack<typename std::remove_cvref_t<Rh>::value_type>
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept = delete;

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_OPTIONAL
