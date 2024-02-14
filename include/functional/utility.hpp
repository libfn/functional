// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_UTILITY
#define INCLUDE_FUNCTIONAL_UTILITY

#include "functional/detail/fwd_macro.hpp"
#include "functional/detail/pack.hpp"
#include "functional/detail/traits.hpp"

#include <type_traits>
#include <utility>

namespace fn {
template <typename T> using as_value_t = decltype(detail::_as_value<T>);

// NOTE: Unlike apply_const_lvalue_t above, this is not exact: prvalue parameters are
// returned as xvalue. This is meant to disable copying of the return value.
template <typename T> [[nodiscard]] constexpr auto apply_const_lvalue(auto &&v) noexcept -> decltype(auto)
{
  return static_cast<apply_const_lvalue_t<T, decltype(v)>>(v);
}

template <typename T> constexpr auto type() noexcept -> std::type_identity<T> { return {}; }

template <typename... Ts> struct pack : detail::pack_base<std::index_sequence_for<Ts...>, Ts...> {
  using _base = detail::pack_base<std::index_sequence_for<Ts...>, Ts...>;

  template <typename Arg> using append_type = pack<Ts..., Arg>;

  template <typename T>
  [[nodiscard]] constexpr auto append(std::type_identity<T>, auto &&...args) & noexcept -> append_type<T>
    requires std::is_constructible_v<T, decltype(args)...>
             && requires { static_cast<append_type<T>>(_base::template _append<T>(*this, FWD(args)...)); }
  {
    return {_base::template _append<T>(*this, FWD(args)...)};
  }

  template <typename T>
  [[nodiscard]] constexpr auto append(std::type_identity<T>, auto &&...args) const & noexcept -> append_type<T>
    requires std::is_constructible_v<T, decltype(args)...>
             && requires { static_cast<append_type<T>>(_base::template _append<T>(*this, FWD(args)...)); }
  {
    return {_base::template _append<T>(*this, FWD(args)...)};
  }

  template <typename T>
  [[nodiscard]] constexpr auto append(std::type_identity<T>, auto &&...args) && noexcept -> append_type<T>
    requires std::is_constructible_v<T, decltype(args)...>
             && requires { static_cast<append_type<T>>(_base::template _append<T>(std::move(*this), FWD(args)...)); }
  {
    return {_base::template _append<T>(std::move(*this), FWD(args)...)};
  }

  template <typename T>
  [[nodiscard]] constexpr auto append(std::type_identity<T>, auto &&...args) const && noexcept -> append_type<T>
    requires std::is_constructible_v<T, decltype(args)...>
             && requires { static_cast<append_type<T>>(_base::template _append<T>(std::move(*this), FWD(args)...)); }
  {
    return {_base::template _append<T>(std::move(*this), FWD(args)...)};
  }

  template <typename Arg>
  [[nodiscard]] constexpr auto append(Arg &&arg) & noexcept -> append_type<Arg>
    requires requires { static_cast<append_type<Arg>>(_base::template _append<Arg>(*this, FWD(arg))); }
  {
    return {_base::template _append<Arg>(*this, FWD(arg))};
  }

  template <typename Arg>
  [[nodiscard]] constexpr auto append(Arg &&arg) const & noexcept -> append_type<Arg>
    requires requires { static_cast<append_type<Arg>>(_base::template _append<Arg>(*this, FWD(arg))); }
  {
    return {_base::template _append<Arg>(*this, FWD(arg))};
  }

  template <typename Arg>
  [[nodiscard]] constexpr auto append(Arg &&arg) && noexcept -> append_type<Arg>
    requires requires { static_cast<append_type<Arg>>(_base::template _append<Arg>(std::move(*this), FWD(arg))); }
  {
    return {_base::template _append<Arg>(std::move(*this), FWD(arg))};
  }

  template <typename Arg>
  [[nodiscard]] constexpr auto append(Arg &&arg) const && noexcept -> append_type<Arg>
    requires requires { static_cast<append_type<Arg>>(_base::template _append<Arg>(std::move(*this), FWD(arg))); }
  {
    return {_base::template _append<Arg>(std::move(*this), FWD(arg))};
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn, auto &&...args) & noexcept -> decltype(auto)
    requires requires { _base::_invoke(*this, FWD(fn), FWD(args)...); }
  {
    return _base::_invoke(*this, FWD(fn), FWD(args)...);
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn, auto &&...args) const & noexcept -> decltype(auto)
    requires requires { _base::_invoke(*this, FWD(fn), FWD(args)...); }
  {
    return _base::_invoke(*this, FWD(fn), FWD(args)...);
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn, auto &&...args) && noexcept -> decltype(auto)
    requires requires { _base::_invoke(std::move(*this), FWD(fn), FWD(args)...); }
  {
    return _base::_invoke(std::move(*this), FWD(fn), FWD(args)...);
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn, auto &&...args) const && noexcept -> decltype(auto)
    requires requires { _base::_invoke(std::move(*this), FWD(fn), FWD(args)...); }
  {
    return _base::_invoke(std::move(*this), FWD(fn), FWD(args)...);
  }
};

template <typename... Args> pack(Args &&...args) -> pack<Args...>;

namespace detail {
template <typename... Ts> constexpr bool _is_some_pack = false;
template <typename... Ts> constexpr bool _is_some_pack<::fn::pack<Ts...> &> = true;
template <typename... Ts> constexpr bool _is_some_pack<::fn::pack<Ts...> const &> = true;
} // namespace detail

template <typename T>
concept some_pack = detail::_is_some_pack<T &>;

template <typename... Ts> struct overload final : Ts... {
  using Ts::operator()...;
};
template <typename... Ts> overload(Ts const &...) -> overload<Ts...>;
} // namespace fn

#endif // INCLUDE_FUNCTIONAL_UTILITY
