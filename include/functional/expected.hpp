// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_EXPECTED
#define INCLUDE_FUNCTIONAL_EXPECTED

#include "functional/functional.hpp"
#include "functional/fwd.hpp"
#include "functional/pack.hpp"
#include "functional/sum.hpp"
#include "functional/utility.hpp"

#include <concepts>
#include <expected>
#include <functional>
#include <type_traits>

namespace fn {

template <typename T>
concept some_expected = detail::_some_expected<T>;

template <typename T>
concept some_expected_non_void = //
    some_expected<T>             //
    && !std::is_same_v<void, typename std::remove_cvref_t<T>::value_type>;

template <typename T>
concept some_expected_void = //
    some_expected<T>         //
    && std::is_same_v<void, typename std::remove_cvref_t<T>::value_type>;

template <typename T, typename Err> struct expected final : std::expected<T, Err> {
  using value_type = std::expected<T, Err>::value_type;
  using error_type = std::expected<T, Err>::error_type;
  using unexpected_type = std::unexpected<Err>;

  using std::expected<T, Err>::expected;

  // and_then non void
  template <typename Fn>
  constexpr auto and_then(Fn &&fn) &
    requires std::is_constructible_v<Err, Err &> && (not std::is_same_v<value_type, void>)
  {
    using type = ::fn::detail::_invoke_result<Fn, value_type &>::type;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type>);
    if (this->has_value())
      return ::fn::detail::_invoke(FWD(fn), this->value());
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const &
    requires std::is_constructible_v<Err, Err const &> && (not std::is_same_v<value_type, void>)
  {
    using type = ::fn::detail::_invoke_result<Fn, value_type const &>::type;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type>);
    if (this->has_value())
      return ::fn::detail::_invoke(FWD(fn), this->value());
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) &&
    requires std::is_constructible_v<Err, Err> && (not std::is_same_v<value_type, void>)
  {
    using type = ::fn::detail::_invoke_result<Fn, value_type &&>::type;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type>);
    if (this->has_value())
      return ::fn::detail::_invoke(FWD(fn), std::move(this->value()));
    else
      return type(std::unexpect, std::move(this->error()));
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const &&
    requires std::is_constructible_v<Err, Err const> && (not std::is_same_v<value_type, void>)
  {
    using type = ::fn::detail::_invoke_result<Fn, value_type const &&>::type;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type>);
    if (this->has_value())
      return ::fn::detail::_invoke(FWD(fn), std::move(this->value()));
    else
      return type(std::unexpect, std::move(this->error()));
  }

  // and_then void
  template <typename Fn>
  constexpr auto and_then(Fn &&fn) &
    requires std::is_constructible_v<Err, Err &> && std::is_same_v<value_type, void>
  {
    using type = ::fn::detail::_invoke_result<Fn>::type;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type>);
    if (this->has_value())
      return ::fn::detail::_invoke(FWD(fn));
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const &
    requires std::is_constructible_v<Err, Err const &> && std::is_same_v<value_type, void>
  {
    using type = ::fn::detail::_invoke_result<Fn>::type;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type>);
    if (this->has_value())
      return ::fn::detail::_invoke(FWD(fn));
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) &&
    requires std::is_constructible_v<Err, Err> && std::is_same_v<value_type, void>
  {
    using type = ::fn::detail::_invoke_result<Fn>::type;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type>);
    if (this->has_value())
      return ::fn::detail::_invoke(FWD(fn));
    else
      return type(std::unexpect, std::move(this->error()));
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const &&
    requires std::is_constructible_v<Err, Err const> && std::is_same_v<value_type, void>
  {
    using type = ::fn::detail::_invoke_result<Fn>::type;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type>);
    if (this->has_value())
      return ::fn::detail::_invoke(FWD(fn));
    else
      return type(std::unexpect, std::move(this->error()));
  }

  // or_else
  template <typename Fn>
  constexpr auto or_else(Fn &&fn) &
    requires (std::is_constructible_v<T, T &> || std::same_as<T, void>)
  {
    using type = ::fn::detail::_invoke_result<Fn, error_type &>::type;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::value_type, value_type>);
    if (this->has_value())
      if constexpr (not std::is_same_v<value_type, void>)
        return type(std::in_place, this->value());
      else
        return type();
    else
      return ::fn::detail::_invoke(FWD(fn), this->error());
  }

  template <typename Fn>
  constexpr auto or_else(Fn &&fn) const &
    requires(std::is_constructible_v<T, T const &> || std::same_as<T, void>)
  {
    using type = ::fn::detail::_invoke_result<Fn, error_type const &>::type;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::value_type, value_type>);
    if (this->has_value())
      if constexpr (not std::is_same_v<value_type, void>)
        return type(std::in_place, this->value());
      else
        return type();
    else
      return ::fn::detail::_invoke(FWD(fn), this->error());
  }

  template <typename Fn>
  constexpr auto or_else(Fn &&fn) &&
    requires(std::is_constructible_v<T, T> || std::same_as<T, void>)
  {
    using type = ::fn::detail::_invoke_result<Fn, error_type &&>::type;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::value_type, value_type>);
    if (this->has_value())
      if constexpr (not std::is_same_v<value_type, void>)
        return type(std::in_place, std::move(this->value()));
      else
        return type();
    else
      return ::fn::detail::_invoke(FWD(fn), std::move(this->error()));
  }

  template <typename Fn>
  constexpr auto or_else(Fn &&fn) const &&
    requires(std::is_constructible_v<T, T const> || std::same_as<T, void>)
  {
    using type = ::fn::detail::_invoke_result<Fn, error_type const &&>::type;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::value_type, value_type>);
    if (this->has_value())
      if constexpr (not std::is_same_v<value_type, void>)
        return type(std::in_place, std::move(this->value()));
      else
        return type();
    else
      return ::fn::detail::_invoke(FWD(fn), std::move(this->error()));
  }

  // transform not void
  template <typename Fn> constexpr auto transform(Fn &&fn) & requires (not std::is_same_v<value_type, void>)
  {
    using value_type = detail::_transform_result<Fn, T &>::type;
    using type = expected<value_type, Err>;
    if (this->has_value())
      if constexpr (std::is_same_v<value_type, void>) {
        ::fn::detail::_invoke(FWD(fn), this->value());
        return type();
      } else
        return type(std::in_place, ::fn::detail::_invoke_r<value_type>(FWD(fn), this->value()));
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn> constexpr auto transform(Fn &&fn) const & requires (not std::is_same_v<value_type, void>)
  {
    using value_type = detail::_transform_result<Fn, T const &>::type;
    using type = expected<value_type, Err>;
    if (this->has_value())
      if constexpr (std::is_same_v<value_type, void>) {
        ::fn::detail::_invoke(FWD(fn), this->value());
        return type();
      } else
        return type(std::in_place, ::fn::detail::_invoke_r<value_type>(FWD(fn), this->value()));
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn> constexpr auto transform(Fn &&fn) && requires (not std::is_same_v<value_type, void>)
  {
    using value_type = detail::_transform_result<Fn, T &&>::type;
    using type = expected<value_type, Err>;
    if (this->has_value())
        if constexpr (std::is_same_v<value_type, void>) {
          ::fn::detail::_invoke(FWD(fn), std::move(this->value()));
          return type();
        } else
          return type(std::in_place, ::fn::detail::_invoke_r<value_type>(FWD(fn), std::move(this->value())));
    else
      return type(std::unexpect, std::move(this->error()));
  }

  template <typename Fn> constexpr auto transform(Fn &&fn) const && requires (not std::is_same_v<value_type, void>)
  {
    using value_type = detail::_transform_result<Fn, T const &&>::type;
    using type = expected<value_type, Err>;
    if (this->has_value())
      if constexpr (std::is_same_v<value_type, void>) {
        ::fn::detail::_invoke(FWD(fn), std::move(this->value()));
        return type();
      } else
        return type(std::in_place, ::fn::detail::_invoke_r<value_type>(FWD(fn), std::move(this->value())));
    else
      return type(std::unexpect, std::move(this->error()));
  }

  // transform void
  template <typename Fn>
  constexpr auto transform(Fn &&fn) &
    requires std::is_same_v<value_type, void>
  {
    using value_type = detail::_invoke_result<Fn>::type;
    using type = expected<value_type, Err>;
    if (this->has_value())
      if constexpr (std::is_same_v<value_type, void>) {
        ::fn::detail::_invoke(FWD(fn));
        return type();
      } else
        return type(std::in_place, ::fn::detail::_invoke(FWD(fn)));
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const &
    requires std::is_same_v<value_type, void>
  {
    using value_type = detail::_invoke_result<Fn>::type;
    using type = expected<value_type, Err>;
    if (this->has_value())
      if constexpr (std::is_same_v<value_type, void>) {
        ::fn::detail::_invoke(FWD(fn));
        return type();
      } else
        return type(std::in_place, ::fn::detail::_invoke(FWD(fn)));
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) &&
    requires std::is_same_v<value_type, void>
  {
    using value_type = detail::_invoke_result<Fn>::type;
    using type = expected<value_type, Err>;
    if (this->has_value())
      if constexpr (std::is_same_v<value_type, void>) {
        ::fn::detail::_invoke(FWD(fn));
        return type();
      } else
        return type(std::in_place, ::fn::detail::_invoke(FWD(fn)));
    else
      return type(std::unexpect, std::move(this->error()));
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const &&
    requires std::is_same_v<value_type, void>
  {
    using value_type = detail::_invoke_result<Fn>::type;
    using type = expected<value_type, Err>;
    if (this->has_value())
      if constexpr (std::is_same_v<value_type, void>) {
        ::fn::detail::_invoke(FWD(fn));
        return type();
      } else
        return type(std::in_place, ::fn::detail::_invoke(FWD(fn)));
    else
      return type(std::unexpect, std::move(this->error()));
  }

  // transform_error
  template <typename Fn> constexpr auto transform_error(Fn &&fn) &
  {
    using error_type = detail::_transform_result<Fn, error_type &>::type;
    using type = expected<T, error_type>;
    if (this->has_value())
      if constexpr (not std::is_same_v<value_type, void>)
        return type(std::in_place, this->value());
      else
        return type();
    else
      return type(std::unexpect, ::fn::detail::_invoke_r<error_type>(FWD(fn), this->error()));
  }

  template <typename Fn> constexpr auto transform_error(Fn &&fn) const &
  {
    using error_type = detail::_transform_result<Fn, error_type const &>::type;
    using type = expected<T, error_type>;
    if (this->has_value())
      if constexpr (not std::is_same_v<value_type, void>)
        return type(std::in_place, this->value());
      else
        return type();
    else
      return type(std::unexpect, ::fn::detail::_invoke_r<error_type>(FWD(fn), this->error()));
  }

  template <typename Fn> constexpr auto transform_error(Fn &&fn) &&
  {
    using error_type = detail::_transform_result<Fn, error_type &&>::type;
    using type = expected<T, error_type>;
    if (this->has_value())
      if constexpr (not std::is_same_v<value_type, void>)
        return type(std::in_place, std::move(this->value()));
      else
        return type();
    else
      return type(std::unexpect, ::fn::detail::_invoke_r<error_type>(FWD(fn), std::move(this->error())));
  }

  template <typename Fn> constexpr auto transform_error(Fn &&fn) const &&
  {
    using error_type = detail::_transform_result<Fn, error_type const &&>::type;
    using type = expected<T, error_type>;
    if (this->has_value())
      if constexpr (not std::is_same_v<value_type, void>)
        return type(std::in_place, std::move(this->value()));
      else
        return type();
    else
      return type(std::unexpect, ::fn::detail::_invoke_r<error_type>(FWD(fn), std::move(this->error())));
  }
};

// When any of the sides is expected<void, ...>, we do not produce expected<pack<...>, ...>
// Instead just elide void and carry non-void (or elide both voids if that's what we get)
template <some_expected_non_void Lh, some_expected_void Rh>
  requires std::is_same_v<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>
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
  requires std::is_same_v<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>
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
  requires std::is_same_v<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using error_type = std::remove_cvref_t<Rh>::error_type;
  using type = expected<void, error_type>;
  if (lh.has_value() && rh.has_value())
    return type();
  else if (not lh.has_value())
    return type{std::unexpect, FWD(lh).error()};
  else
    return type{std::unexpect, FWD(rh).error()};
}

// Overloads when both sides are non-void, producing expected<pack<...>, ...>
template <some_expected_non_void Lh, some_expected_non_void Rh>
  requires std::is_same_v<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>
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
  requires std::is_same_v<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>
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
  requires std::is_same_v<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>
               && (not some_pack<typename std::remove_cvref_t<Lh>::value_type>)
               && (some_pack<typename std::remove_cvref_t<Rh>::value_type>)
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept = delete;

template <some_expected_non_void Lh, some_expected_non_void Rh>
  requires std::is_same_v<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>
               && (some_pack<typename std::remove_cvref_t<Lh>::value_type>)
               && (some_pack<typename std::remove_cvref_t<Rh>::value_type>)
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept = delete;

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_EXPECTED
