// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_UTILITY
#define INCLUDE_FUNCTIONAL_UTILITY

#include "functional/detail/fwd_macro.hpp"
#include "functional/detail/meta.hpp"
#include "functional/detail/pack.hpp"
#include "functional/detail/sum_storage.hpp"
#include "functional/detail/traits.hpp"

#include <concepts>
#include <type_traits>
#include <utility>
#include <variant>

namespace fn {
template <typename T> using as_value_t = decltype(detail::_as_value<T>);

// NOTE: Unlike apply_const_lvalue_t above, this is not exact: prvalue parameters are
// returned as xvalue. This is meant to disable copying of the return value.
template <typename T> [[nodiscard]] constexpr auto apply_const_lvalue(auto &&v) noexcept -> decltype(auto)
{
  return static_cast<apply_const_lvalue_t<T, decltype(v)>>(v);
}

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

namespace detail {
template <typename... Ts> constexpr bool _is_some_pack = false;
template <typename... Ts> constexpr bool _is_some_pack<::fn::pack<Ts...> &> = true;
template <typename... Ts> constexpr bool _is_some_pack<::fn::pack<Ts...> const &> = true;
} // namespace detail

template <typename T>
concept some_pack = detail::_is_some_pack<T &>;

template <typename... Ts> struct sum;

namespace detail {
template <typename... Ts> constexpr bool _is_some_sum = false;
template <typename... Ts> constexpr bool _is_some_sum<::fn::sum<Ts...> &> = true;
template <typename... Ts> constexpr bool _is_some_sum<::fn::sum<Ts...> const &> = true;
} // namespace detail

template <typename T>
concept some_sum = detail::_is_some_sum<T &>;

// NOTE Identical concept is defined inside detail
template <typename T>
concept some_in_place_type = detail::_is_in_place_type<T &>;

template <> struct sum<>; // Intentionally incomplete

template <typename... Ts>
  requires(sizeof...(Ts) > 0)
struct sum<Ts...> {
  template <typename T>
  static constexpr bool _is_valid_subtype //
      = (not std::same_as<void, T>)&&(not std::is_reference_v<T>)&&(not some_in_place_type<T>);
  static_assert((... && _is_valid_subtype<Ts>));

  using type = typename detail::normalized<Ts...>::template apply<detail::sum_storage>;
  static constexpr bool is_normal = detail::is_normal_v<Ts...>;
  static constexpr std::size_t size = type::size;
  template <typename T> static constexpr bool has_type = is_normal && (... || std::same_as<Ts, T>);
  type value;
  std::size_t const index;

  template <typename Fn, typename Self>
  static constexpr bool invocable //
      = is_normal && (... && std::is_invocable_v<Fn, apply_const_lvalue_t<Self, Ts &&>>);
  template <typename Fn, typename Self>
  static constexpr bool invocable_typed
      = is_normal && (... && std::is_invocable_v<Fn, std::in_place_type_t<Ts>, apply_const_lvalue_t<Self, Ts &&>>);

  template <typename Fn, typename Self> struct invoke_result;
  template <typename Fn, typename Self>
    requires invocable<Fn, Self>
  struct invoke_result<Fn, Self> final {
    using _T0 = detail::select_nth_t<0, Ts...>;
    using _R0 = std::invoke_result_t<Fn, apply_const_lvalue_t<Self, _T0 &&>>;
    static_assert((... && std::same_as<_R0, std::invoke_result_t<Fn, apply_const_lvalue_t<Self, Ts &&>>>));
    using type = _R0;
  };
  template <typename Fn, typename Self>
    requires invocable_typed<Fn, Self>
  struct invoke_result<Fn, Self> final {
    using _T0 = detail::select_nth_t<0, Ts...>;
    using _R0 = std::invoke_result_t<Fn, std::in_place_type_t<_T0>, apply_const_lvalue_t<Self, _T0 &&>>;
    static_assert(
        (...
         && std::same_as<_R0, std::invoke_result_t<Fn, std::in_place_type_t<_T0>, apply_const_lvalue_t<Self, Ts &&>>>));
    using type = _R0;
  };

  template <typename Fn, typename Self> using invoke_result_t = typename invoke_result<Fn, Self>::type;

  [[nodiscard]] static constexpr auto make(some_in_place_type auto d, auto &&...args) noexcept
  {
    using type = detail::normalized<Ts...>::template apply<sum>;
    return type(d, FWD(args)...);
  }

  [[nodiscard]] static constexpr auto make_from(some_sum auto &&arg) noexcept
    requires(detail::is_superset_of<sum<Ts...>, std::remove_cvref_t<decltype(arg)>>)
  {
    using type = detail::normalized<Ts...>::template apply<sum>;
    return detail::invoke_sum_storage(
        arg.index,
        []<typename U>(std::in_place_type_t<U> d, auto &&v) noexcept -> type {
          return type{d, FWD(v)};
        },
        FWD(arg).value);
  }

  template <typename T>
  constexpr sum(T &&v)
    requires(size == 1) && (not std::same_as<std::remove_cvref_t<T>, sum<Ts...>>) && (not some_in_place_type<T>) //
                && (... || std::is_constructible_v<Ts, T>) && (... || std::is_convertible_v<T, Ts>)
      : value(std::in_place_type<T>, FWD(v)), index(detail::type_index<T, Ts...>)
  {
  }

  template <typename T>
  constexpr explicit sum(T &&v)
    requires(size == 1) && (not std::same_as<std::remove_cvref_t<T>, sum<Ts...>>) && (not some_in_place_type<T>) //
                && (... || std::is_constructible_v<Ts, T>) && (not(... || std::is_convertible_v<T, Ts>))
      : value(std::in_place_type<T>, FWD(v)), index(detail::type_index<T, Ts...>)
  {
  }

  template <typename T>
  constexpr sum(std::in_place_type_t<T> d, auto &&...args) noexcept
    requires is_normal && (... || std::is_same_v<T, Ts>)
      : value(d, FWD(args)...), index(detail::type_index<T, Ts...>)
  {
  }

  constexpr ~sum()
  {
    _apply_sum_storage_ptr(
        index, [](some_in_place_type auto, auto *ptr) noexcept { std::destroy_at(ptr); }, value);
  }

  template <typename T>
    requires has_type<T>
  [[nodiscard]] constexpr bool has_value() const noexcept
  {
    return invoke([]<typename U>(std::in_place_type_t<U>, auto &&) constexpr -> bool { return std::same_as<T, U>; });
  }

  template <typename T>
    requires has_type<T>
  [[nodiscard]] constexpr bool has_value(std::in_place_type_t<T>) const noexcept
  {
    return invoke([]<typename U>(std::in_place_type_t<U>, auto &&) constexpr -> bool { return std::same_as<T, U>; });
  }

  template <typename T>
  [[nodiscard]] constexpr bool operator==(T const &rh) const noexcept
    requires(has_type<T> || has_type<T const>) && std::equality_comparable<T>
  {
    return invoke([&rh]<typename U>(std::in_place_type_t<U>, auto const &lh) -> bool { //
      if constexpr (std::same_as<T, U> || std::same_as<T const, U>)
        return rh == lh;
      return false;
    });
  }

  template <typename T>
  [[nodiscard]] constexpr bool operator!=(T const &rh) const noexcept
    requires(has_type<T> || has_type<T const>) && std::equality_comparable<T>
  {
    return not(*this == rh);
  }

  [[nodiscard]] constexpr bool operator==(sum<Ts...> const &other) const noexcept
    requires(... && std::equality_comparable<Ts>)
  {
    return (this->index == other.index //
            && invoke([&other]<typename T>(std::in_place_type_t<T>, auto const &lh) -> bool {
                 return other.invoke([&lh]<typename U>(std::in_place_type_t<U>, auto const &rh) -> bool { //
                   if constexpr (std::same_as<T, U>)
                     return lh == rh;
                   std::unreachable();
                 });
               }));
  }

  [[nodiscard]] constexpr bool operator!=(sum<Ts...> const &other) const noexcept
    requires(... && std::equality_comparable<Ts>)
  {
    return not(*this == other);
  }

  template <typename Fn> [[nodiscard]] constexpr auto invoke(Fn &&fn) & noexcept -> invoke_result_t<Fn, sum &>
  {
    return detail::invoke_sum_storage(index, FWD(fn), this->value);
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) const & noexcept -> invoke_result_t<Fn, sum const &>
  {
    return detail::invoke_sum_storage(index, FWD(fn), this->value);
  }

  template <typename Fn> [[nodiscard]] constexpr auto invoke(Fn &&fn) && noexcept -> invoke_result_t<Fn, sum &&>
  {
    return detail::invoke_sum_storage(index, FWD(fn), std::move(this->value));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) const && noexcept -> invoke_result_t<Fn, sum const &&>
  {
    return detail::invoke_sum_storage(index, FWD(fn), std::move(this->value));
  }
};

// CTAD for single-element sum
template <typename T> sum(std::in_place_type_t<T>, auto &&...) -> sum<T>;
template <typename T> sum(T) -> sum<T>;

template <typename... Ts> struct overload final : Ts... {
  using Ts::operator()...;
};
template <typename... Ts> overload(Ts const &...) -> overload<Ts...>;
} // namespace fn

#endif // INCLUDE_FUNCTIONAL_UTILITY
