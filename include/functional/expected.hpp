// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_EXPECTED
#define INCLUDE_FUNCTIONAL_EXPECTED

#include "functional/detail/fwd_macro.hpp"
#include "functional/pack.hpp"
#include "functional/utility.hpp"

#include <concepts>
#include <expected>
#include <functional>
#include <type_traits>

namespace fn {

template <typename T, typename Err> struct expected;

namespace detail {
template <typename T> constexpr bool _is_some_expected = false;
template <typename T, typename Err> constexpr bool _is_some_expected<::fn::expected<T, Err> &> = true;
template <typename T, typename Err> constexpr bool _is_some_expected<::fn::expected<T, Err> const &> = true;
} // namespace detail

template <typename T>
concept some_expected = detail::_is_some_expected<T &>;

template <typename T>
concept some_expected_non_void = //
    some_expected<T>             //
    && !std::same_as<void, typename std::remove_cvref_t<T>::value_type>;

template <typename T>
concept some_expected_non_pack = //
    some_expected<T>             //
    && !std::same_as<void, typename std::remove_cvref_t<T>::value_type>
    && !some_pack<typename std::remove_cvref_t<T>::value_type>;

template <typename T>
concept some_expected_pack = //
    some_expected<T>         //
    && some_pack<typename std::remove_cvref_t<T>::value_type>;

template <typename T>
concept some_expected_void = //
    some_expected<T>         //
    && std::same_as<void, typename std::remove_cvref_t<T>::value_type>;

template <typename T, typename Err> struct expected final : std::expected<T, Err> {
  using value_type = std::expected<T, Err>::value_type;
  using error_type = std::expected<T, Err>::error_type;
  using unexpected_type = std::unexpected<Err>;

  using std::expected<T, Err>::expected;

  // and_then pack
  template <typename Fn>
  constexpr auto and_then(Fn &&fn) &
    requires std::is_constructible_v<Err, Err &> && some_pack<value_type>
  {
    using type = decltype(this->value().invoke(FWD(fn)));
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type>);
    if (this->has_value())
      return this->value().invoke(FWD(fn));
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const &
    requires std::is_constructible_v<Err, Err const &> && some_pack<value_type>
  {
    using type = decltype(this->value().invoke(FWD(fn)));
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type>);
    if (this->has_value())
      return this->value().invoke(FWD(fn));
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) &&
    requires std::is_constructible_v<Err, Err &&> && some_pack<value_type>
  {
    using type = decltype(std::move(*this).value().invoke(FWD(fn)));
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type>);
    if (this->has_value())
      return std::move(*this).value().invoke(FWD(fn));
    else
      return type(std::unexpect, std::move(this->error()));
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const &&
    requires std::is_constructible_v<Err, Err const &&> && some_pack<value_type>
  {
    using type = decltype(std::move(*this).value().invoke(FWD(fn)));
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type>);
    if (this->has_value())
      return std::move(*this).value().invoke(FWD(fn));
    else
      return type(std::unexpect, std::move(this->error()));
  }

  // transform pack
  template <typename Fn>
  constexpr auto transform(Fn &&fn) &
    requires some_pack<value_type>
  {
    using value_type = decltype(this->value().invoke(FWD(fn)));
    using type = expected<value_type, Err>;
    if (this->has_value())
      if constexpr (std::same_as<value_type, void>) {
        this->value().invoke(FWD(fn));
        return type();
      } else
        return type(std::in_place, this->value().invoke(FWD(fn)));
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const &
    requires some_pack<value_type>
  {
    using value_type = decltype(this->value().invoke(FWD(fn)));
    using type = expected<value_type, Err>;
    if (this->has_value())
      if constexpr (std::same_as<value_type, void>) {
        this->value().invoke(FWD(fn));
        return type();
      } else
        return type(std::in_place, this->value().invoke(FWD(fn)));
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) &&
    requires some_pack<value_type>
  {
    using value_type = decltype(std::move(*this).value().invoke(FWD(fn)));
    using type = expected<value_type, Err>;
    if (this->has_value())
      if constexpr (std::same_as<value_type, void>) {
        std::move(*this).value().invoke(FWD(fn));
        return type();
      } else
        return type(std::in_place, std::move(*this).value().invoke(FWD(fn)));
    else
      return type(std::unexpect, std::move(this->error()));
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const &&
    requires some_pack<value_type>
  {
    using value_type = decltype(std::move(*this).value().invoke(FWD(fn)));
    using type = expected<value_type, Err>;
    if (this->has_value())
      if constexpr (std::same_as<value_type, void>) {
        std::move(*this).value().invoke(FWD(fn));
        return type();
      } else
        return type(std::in_place, std::move(*this).value().invoke(FWD(fn)));
    else
      return type(std::unexpect, std::move(this->error()));
  }

  // NOTE All the functions below are polyfills. They are needed because monadic
  // operations must return fn::expected rather than std::expected

  // and_then not void
  template <typename Fn>
  constexpr auto and_then(Fn &&fn) &
    requires std::is_constructible_v<Err, Err &> && (not std::same_as<T, void>) && (not some_pack<value_type>)
  {
    using type = std::invoke_result_t<Fn, value_type &>;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type>);
    if (this->has_value())
      return std::invoke(FWD(fn), this->value());
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const &
    requires std::is_constructible_v<Err, Err const &> && (not std::same_as<T, void>) && (not some_pack<value_type>)
  {
    using type = std::invoke_result_t<Fn, value_type const &>;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type>);
    if (this->has_value())
      return std::invoke(FWD(fn), this->value());
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) &&
    requires std::is_constructible_v<Err, Err> && (not std::same_as<T, void>) && (not some_pack<value_type>)
  {
    using type = std::invoke_result_t<Fn, value_type &&>;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type>);
    if (this->has_value())
      return std::invoke(FWD(fn), std::move(this->value()));
    else
      return type(std::unexpect, std::move(this->error()));
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const &&
    requires std::is_constructible_v<Err, Err const> && (not std::same_as<T, void>) && (not some_pack<value_type>)
  {
    using type = std::invoke_result_t<Fn, value_type const &&>;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type>);
    if (this->has_value())
      return std::invoke(FWD(fn), std::move(this->value()));
    else
      return type(std::unexpect, std::move(this->error()));
  }

  // and_then void
  template <typename Fn>
  constexpr auto and_then(Fn &&fn) &
    requires std::same_as<T, void>
  {
    using type = std::invoke_result_t<Fn>;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type>);
    if (this->has_value())
      return std::invoke(FWD(fn));
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const &
    requires std::same_as<T, void>
  {
    using type = std::invoke_result_t<Fn>;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type>);
    if (this->has_value())
      return std::invoke(FWD(fn));
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) &&
    requires std::same_as<T, void>
  {
    using type = std::invoke_result_t<Fn>;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type>);
    if (this->has_value())
      return std::invoke(FWD(fn));
    else
      return type(std::unexpect, std::move(this->error()));
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const &&
    requires std::same_as<T, void>
  {
    using type = std::invoke_result_t<Fn>;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type>);
    if (this->has_value())
      return std::invoke(FWD(fn));
    else
      return type(std::unexpect, std::move(this->error()));
  }

  // or_else not void
  template <typename Fn>
  constexpr auto or_else(Fn &&fn) &
    requires std::is_constructible_v<T, T &> && (not std::same_as<T, void>)
  {
    using type = std::invoke_result_t<Fn, error_type &>;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::value_type, value_type>);
    if (this->has_value())
      return type(std::in_place, this->value());
    else
      return std::invoke(FWD(fn), this->error());
  }

  template <typename Fn>
  constexpr auto or_else(Fn &&fn) const &
    requires std::is_constructible_v<T, T const &> && (not std::same_as<T, void>)
  {
    using type = std::invoke_result_t<Fn, error_type const &>;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::value_type, value_type>);
    if (this->has_value())
      return type(std::in_place, this->value());
    else
      return std::invoke(FWD(fn), this->error());
  }

  template <typename Fn>
  constexpr auto or_else(Fn &&fn) &&
    requires std::is_constructible_v<T, T> && (not std::same_as<T, void>)
  {
    using type = std::invoke_result_t<Fn, error_type &&>;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::value_type, value_type>);
    if (this->has_value())
      return type(std::in_place, std::move(this->value()));
    else
      return std::invoke(FWD(fn), std::move(this->error()));
  }

  template <typename Fn>
  constexpr auto or_else(Fn &&fn) const &&
    requires std::is_constructible_v<T, T const> && (not std::same_as<T, void>)
  {
    using type = std::invoke_result_t<Fn, error_type const &&>;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::value_type, value_type>);
    if (this->has_value())
      return type(std::in_place, std::move(this->value()));
    else
      return std::invoke(FWD(fn), std::move(this->error()));
  }

  // or_else void
  template <typename Fn>
  constexpr auto or_else(Fn &&fn) &
    requires std::same_as<T, void>
  {
    using type = std::invoke_result_t<Fn, error_type &>;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::value_type, void>);
    if (this->has_value())
      return type();
    else
      return std::invoke(FWD(fn), this->error());
  }

  template <typename Fn>
  constexpr auto or_else(Fn &&fn) const &
    requires std::same_as<T, void>
  {
    using type = std::invoke_result_t<Fn, error_type const &>;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::value_type, void>);
    if (this->has_value())
      return type();
    else
      return std::invoke(FWD(fn), this->error());
  }

  template <typename Fn>
  constexpr auto or_else(Fn &&fn) &&
    requires std::same_as<T, void>
  {
    using type = std::invoke_result_t<Fn, error_type &&>;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::value_type, void>);
    if (this->has_value())
      return type();
    else
      return std::invoke(FWD(fn), std::move(this->error()));
  }

  template <typename Fn>
  constexpr auto or_else(Fn &&fn) const &&
    requires std::same_as<T, void>
  {
    using type = std::invoke_result_t<Fn, error_type const &&>;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::value_type, void>);
    if (this->has_value())
      return type();
    else
      return std::invoke(FWD(fn), std::move(this->error()));
  }

  // transform not void
  template <typename Fn>
          constexpr auto transform(Fn &&fn) & requires(not std::same_as<T, void>)
      && (not some_pack<value_type>)
  {
    using value_type = std::invoke_result_t<Fn, value_type &>;
    using type = expected<value_type, Err>;
    if (this->has_value())
      if constexpr (std::same_as<value_type, void>) {
        std::invoke(FWD(fn), this->value());
        return type();
      } else
        return type(std::in_place, std::invoke(FWD(fn), this->value()));
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const &
    requires(not std::same_as<T, void>) && (not some_pack<value_type>)
  {
    using value_type = std::invoke_result_t<Fn, value_type const &>;
    using type = expected<value_type, Err>;
    if (this->has_value())
      if constexpr (std::same_as<value_type, void>) {
        std::invoke(FWD(fn), this->value());
        return type();
      } else
        return type(std::in_place, std::invoke(FWD(fn), this->value()));
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn>
      constexpr auto transform(Fn &&fn) && requires(not std::same_as<T, void>) && (not some_pack<value_type>)
  {
    using value_type = std::invoke_result_t<Fn, value_type &&>;
    using type = expected<value_type, Err>;
    if (this->has_value())
      if constexpr (std::same_as<value_type, void>) {
        std::invoke(FWD(fn), std::move(this->value()));
        return type();
      } else
        return type(std::in_place, std::invoke(FWD(fn), std::move(this->value())));
    else
      return type(std::unexpect, std::move(this->error()));
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const &&
    requires(not std::same_as<T, void>) && (not some_pack<value_type>)
  {
    using value_type = std::invoke_result_t<Fn, value_type const &&>;
    using type = expected<value_type, Err>;
    if (this->has_value())
      if constexpr (std::same_as<value_type, void>) {
        std::invoke(FWD(fn), std::move(this->value()));
        return type();
      } else
        return type(std::in_place, std::invoke(FWD(fn), std::move(this->value())));
    else
      return type(std::unexpect, std::move(this->error()));
  }

  // transform void
  template <typename Fn>
  constexpr auto transform(Fn &&fn) &
    requires std::same_as<T, void>
  {
    using value_type = std::invoke_result_t<Fn>;
    using type = expected<value_type, Err>;
    if (this->has_value())
      if constexpr (std::same_as<value_type, void>) {
        std::invoke(FWD(fn));
        return type();
      } else
        return type(std::in_place, std::invoke(FWD(fn)));
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const &
    requires std::same_as<T, void>
  {
    using value_type = std::invoke_result_t<Fn>;
    using type = expected<value_type, Err>;
    if (this->has_value())
      if constexpr (std::same_as<value_type, void>) {
        std::invoke(FWD(fn));
        return type();
      } else
        return type(std::in_place, std::invoke(FWD(fn)));
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) &&
    requires std::same_as<T, void>
  {
    using value_type = std::invoke_result_t<Fn>;
    using type = expected<value_type, Err>;
    if (this->has_value())
      if constexpr (std::same_as<value_type, void>) {
        std::invoke(FWD(fn));
        return type();
      } else
        return type(std::in_place, std::invoke(FWD(fn)));
    else
      return type(std::unexpect, std::move(this->error()));
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const &&
    requires std::same_as<T, void>
  {
    using value_type = std::invoke_result_t<Fn &>;
    using type = expected<value_type, Err>;
    if (this->has_value())
      if constexpr (std::same_as<value_type, void>) {
        std::invoke(FWD(fn));
        return type();
      } else
        return type(std::in_place, std::invoke(FWD(fn)));
    else
      return type(std::unexpect, std::move(this->error()));
  }

  // transform_error not void
  template <typename Fn>
    requires(not std::same_as<T, void>)
  constexpr auto transform_error(Fn &&fn) &
  {
    using error_type = std::invoke_result_t<Fn, error_type &>;
    using type = expected<T, error_type>;
    if (this->has_value())
      return type(std::in_place, this->value());
    else
      return type(std::unexpect, std::invoke(FWD(fn), this->error()));
  }

  template <typename Fn>
    requires(not std::same_as<T, void>)
  constexpr auto transform_error(Fn &&fn) const &
  {
    using error_type = std::invoke_result_t<Fn, error_type const &>;
    using type = expected<T, error_type>;
    if (this->has_value())
      return type(std::in_place, this->value());
    else
      return type(std::unexpect, std::invoke(FWD(fn), this->error()));
  }

  template <typename Fn> constexpr auto transform_error(Fn &&fn) && requires (not std::same_as<T, void>)
  {
    using error_type = std::invoke_result_t<Fn, error_type &&>;
    using type = expected<T, error_type>;
    if (this->has_value())
      return type(std::in_place, std::move(this->value()));
    else
      return type(std::unexpect, std::invoke(FWD(fn), std::move(this->error())));
  }

  template <typename Fn> constexpr auto transform_error(Fn &&fn) const && requires (not std::same_as<T, void>)
  {
    using error_type = std::invoke_result_t<Fn, error_type const &&>;
    using type = expected<T, error_type>;
    if (this->has_value())
      return type(std::in_place, std::move(this->value()));
    else
      return type(std::unexpect, std::invoke(FWD(fn), std::move(this->error())));
  }

  // transform_error void
  template <typename Fn>
  constexpr auto transform_error(Fn &&fn) &
    requires std::same_as<T, void>
  {
    using error_type = std::invoke_result_t<Fn, error_type &>;
    using type = expected<T, error_type>;
    if (this->has_value())
      return type();
    else
      return type(std::unexpect, std::invoke(FWD(fn), this->error()));
  }

  template <typename Fn>
  constexpr auto transform_error(Fn &&fn) const &
    requires std::same_as<T, void>
  {
    using error_type = std::invoke_result_t<Fn, error_type const &>;
    using type = expected<T, error_type>;
    if (this->has_value())
      return type();
    else
      return type(std::unexpect, std::invoke(FWD(fn), this->error()));
  }

  template <typename Fn>
  constexpr auto transform_error(Fn &&fn) &&
    requires std::same_as<T, void>
  {
    using error_type = std::invoke_result_t<Fn, error_type &&>;
    using type = expected<T, error_type>;
    if (this->has_value())
      return type();
    else
      return type(std::unexpect, std::invoke(FWD(fn), std::move(this->error())));
  }

  template <typename Fn>
  constexpr auto transform_error(Fn &&fn) const &&
    requires std::same_as<T, void>
  {
    using error_type = std::invoke_result_t<Fn, error_type const &&>;
    using type = expected<T, error_type>;
    if (this->has_value())
      return type();
    else
      return type(std::unexpect, std::invoke(FWD(fn), std::move(this->error())));
  }
};

// When any of the sides is expected<void, ...>, we do not produce expected<pack<...>, ...>
// Instead just elide void and carry non-void (or elide both voids if that's what we get)
template <some_expected_non_void Lh, some_expected_void Rh>
  requires std::same_as<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using value_type = std::remove_cvref_t<Lh>::value_type;
  using error_type = std::remove_cvref_t<Lh>::error_type;
  using type = expected<value_type, error_type>;
  if (lh.has_value() && rh.has_value())
    return type{std::in_place, FWD(lh).value()};
  else if (not lh.has_value())
    return type{std::unexpect, FWD(lh).error()};
  else
    return type{std::unexpect, FWD(rh).error()};
}

template <some_expected_void Lh, some_expected_non_void Rh>
  requires std::same_as<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using value_type = std::remove_cvref_t<Rh>::value_type;
  using error_type = std::remove_cvref_t<Rh>::error_type;
  using type = expected<value_type, error_type>;
  if (lh.has_value() && rh.has_value())
    return type{std::in_place, FWD(rh).value()};
  else if (not lh.has_value())
    return type{std::unexpect, FWD(lh).error()};
  else
    return type{std::unexpect, FWD(rh).error()};
}

template <some_expected_void Lh, some_expected_void Rh>
  requires std::same_as<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using error_type = std::remove_cvref_t<Rh>::error_type;
  using type = expected<void, error_type>;
  if (lh.has_value() && rh.has_value())
    return type{};
  else if (not lh.has_value())
    return type{std::unexpect, FWD(lh).error()};
  else
    return type{std::unexpect, FWD(rh).error()};
}

// Overloads when both sides are non-void, producing expected<pack<...>, ...>
template <some_expected_non_void Lh, some_expected_non_void Rh>
  requires std::same_as<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>
           && (not some_pack<typename std::remove_cvref_t<Lh>::value_type>)
           && (not some_pack<typename std::remove_cvref_t<Rh>::value_type>)
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using lh_type = std::remove_cvref_t<Lh>::value_type;
  using rh_type = std::remove_cvref_t<Rh>::value_type;
  using value_type = pack<lh_type, rh_type>;
  using error_type = std::remove_cvref_t<Rh>::error_type;
  using type = expected<value_type, error_type>;
  if (lh.has_value() && rh.has_value())
    return type{std::in_place, pack<lh_type>{FWD(lh).value()}.append(std::in_place_type_t<rh_type>{}, FWD(rh).value())};
  else if (not lh.has_value())
    return type{std::unexpect, FWD(lh).error()};
  else
    return type{std::unexpect, FWD(rh).error()};
}

template <some_expected_non_void Lh, some_expected_non_void Rh>
  requires std::same_as<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>
           && (some_pack<typename std::remove_cvref_t<Lh>::value_type>)
           && (not some_pack<typename std::remove_cvref_t<Rh>::value_type>)
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using lh_type = std::remove_cvref_t<Lh>::value_type;
  using rh_type = std::remove_cvref_t<Rh>::value_type;
  using value_type = typename lh_type::template append_type<rh_type>;
  using error_type = std::remove_cvref_t<Rh>::error_type;
  using type = expected<value_type, error_type>;
  if (lh.has_value() && rh.has_value())
    return type{std::in_place, FWD(lh).value().append(std::in_place_type_t<rh_type>{}, FWD(rh).value())};
  else if (not lh.has_value())
    return type{std::unexpect, FWD(lh).error()};
  else
    return type{std::unexpect, FWD(rh).error()};
}

template <some_expected_non_void Lh, some_expected_non_void Rh>
  requires std::same_as<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>
               && (not some_pack<typename std::remove_cvref_t<Lh>::value_type>)
               && (some_pack<typename std::remove_cvref_t<Rh>::value_type>)
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept = delete;

template <some_expected_non_void Lh, some_expected_non_void Rh>
  requires std::same_as<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>
               && (some_pack<typename std::remove_cvref_t<Lh>::value_type>)
               && (some_pack<typename std::remove_cvref_t<Rh>::value_type>)
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept = delete;

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_EXPECTED
