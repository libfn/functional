// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_PACK
#define INCLUDE_FUNCTIONAL_PACK

#include "functional/detail/pack.hpp"
#include "functional/utility.hpp"

namespace fn {

template <typename... Ts> struct pack : detail::pack_base<std::index_sequence_for<Ts...>, Ts...> {
  using _base = detail::pack_base<std::index_sequence_for<Ts...>, Ts...>;

  template <typename Arg> using append_type = pack<Ts..., Arg>;

  template <typename T>
  [[nodiscard]] constexpr auto append(std::in_place_type_t<T>, auto &&...args) & noexcept -> append_type<T>
    requires std::is_constructible_v<T, decltype(args)...>
             && requires { static_cast<append_type<T>>(_base::template _append<T>(*this, FWD(args)...)); }
  {
    return {_base::template _append<T>(*this, FWD(args)...)};
  }

  template <typename T>
  [[nodiscard]] constexpr auto append(std::in_place_type_t<T>, auto &&...args) const & noexcept -> append_type<T>
    requires std::is_constructible_v<T, decltype(args)...>
             && requires { static_cast<append_type<T>>(_base::template _append<T>(*this, FWD(args)...)); }
  {
    return {_base::template _append<T>(*this, FWD(args)...)};
  }

  template <typename T>
  [[nodiscard]] constexpr auto append(std::in_place_type_t<T>, auto &&...args) && noexcept -> append_type<T>
    requires std::is_constructible_v<T, decltype(args)...>
             && requires { static_cast<append_type<T>>(_base::template _append<T>(std::move(*this), FWD(args)...)); }
  {
    return {_base::template _append<T>(std::move(*this), FWD(args)...)};
  }

  template <typename T>
  [[nodiscard]] constexpr auto append(std::in_place_type_t<T>, auto &&...args) const && noexcept -> append_type<T>
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
} // namespace fn

#endif // INCLUDE_FUNCTIONAL_PACK
