// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_OPTIONAL
#define INCLUDE_FUNCTIONAL_OPTIONAL

#include "functional/detail/functional.hpp"
#include "functional/fwd.hpp"
#include "functional/pack.hpp"
#include "functional/utility.hpp"

#include <concepts>
#include <functional>
#include <optional>
#include <type_traits>

namespace fn {

template <typename T>
concept some_optional = detail::_some_optional<T>;

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

  template <typename Fn> constexpr auto and_then(Fn &&fn) &
  {
    using type = ::fn::detail::_invoke_result_t<Fn, value_type &>;
    static_assert(some_optional<type>);
    if (this->has_value())
      return ::fn::detail::_invoke(FWD(fn), this->value());
    else
      return type(std::nullopt);
  }

  template <typename Fn> constexpr auto and_then(Fn &&fn) const &
  {
    using type = ::fn::detail::_invoke_result_t<Fn, value_type const &>;
    static_assert(some_optional<type>);
    if (this->has_value())
      return ::fn::detail::_invoke(FWD(fn), this->value());
    else
      return type(std::nullopt);
  }

  template <typename Fn> constexpr auto and_then(Fn &&fn) &&
  {
    using type = ::fn::detail::_invoke_result_t<Fn, value_type &&>;
    static_assert(some_optional<type>);
    if (this->has_value())
      return ::fn::detail::_invoke(FWD(fn), std::move(this->value()));
    else
      return type(std::nullopt);
  }

  template <typename Fn> constexpr auto and_then(Fn &&fn) const &&
  {
    using type = ::fn::detail::_invoke_result_t<Fn, value_type const &&>;
    static_assert(some_optional<type>);
    if (this->has_value())
      return ::fn::detail::_invoke(FWD(fn), std::move(this->value()));
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
      return ::fn::detail::_invoke(FWD(fn));
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
      return ::fn::detail::_invoke(FWD(fn));
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
      return ::fn::detail::_invoke(FWD(fn));
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
      return ::fn::detail::_invoke(FWD(fn));
  }

  template <typename Fn> constexpr auto transform(Fn &&fn) &
  {
    using value_type = ::fn::detail::_invoke_result_t<Fn, value_type &>;
    using type = optional<value_type>;
    if (this->has_value())
      return type(std::in_place, ::fn::detail::_invoke(FWD(fn), this->value()));
    else
      return type(std::nullopt);
  }

  template <typename Fn> constexpr auto transform(Fn &&fn) const &
  {
    using value_type = ::fn::detail::_invoke_result_t<Fn, value_type const &>;
    using type = optional<value_type>;
    if (this->has_value())
      return type(std::in_place, ::fn::detail::_invoke(FWD(fn), this->value()));
    else
      return type(std::nullopt);
  }

  template <typename Fn> constexpr auto transform(Fn &&fn) &&
  {
    using value_type = ::fn::detail::_invoke_result_t<Fn, value_type &&>;
    using type = optional<value_type>;
    if (this->has_value())
      return type(std::in_place, ::fn::detail::_invoke(FWD(fn), std::move(this->value())));
    else
      return type(std::nullopt);
  }

  template <typename Fn> constexpr auto transform(Fn &&fn) const &&
  {
    using value_type = ::fn::detail::_invoke_result_t<Fn, value_type const &&>;
    using type = optional<value_type>;
    if (this->has_value())
      return type(std::in_place, ::fn::detail::_invoke(FWD(fn), std::move(this->value())));
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
