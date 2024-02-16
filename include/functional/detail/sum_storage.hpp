// Copyright (c) 2024 Gašper Ažman, Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_DETAIL_SUM_STORAGE
#define INCLUDE_FUNCTIONAL_DETAIL_SUM_STORAGE

#include "functional/detail/fwd_macro.hpp"
#include "functional/detail/meta.hpp"
#include "functional/detail/traits.hpp"

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

namespace fn::detail {

template <typename... Ts> union sum_storage;

template <> union sum_storage<>; // Intentionally incomplete

template <typename... Ts> constexpr bool _is_sum_storage = false;
template <typename... Ts> constexpr bool _is_sum_storage<sum_storage<Ts...> &> = true;
template <typename... Ts> constexpr bool _is_sum_storage<sum_storage<Ts...> const &> = true;

template <typename T>
concept some_sum_storage = _is_sum_storage<T &>;

template <typename T> constexpr bool _is_in_place_type = false;
template <typename T> constexpr bool _is_in_place_type<::std::in_place_type_t<T> &> = true;
template <typename T> constexpr bool _is_in_place_type<::std::in_place_type_t<T> const &> = true;

template <typename T>
concept some_in_place_type = _is_in_place_type<T &>;

// NOTE Define several specializations to keep compilation times low, then recursive variadic one
template <typename T0> union sum_storage<T0> final {
  static_assert(not std::is_same_v<void, T0>);
  static_assert(not std::is_reference_v<T0>);
  static_assert(not some_in_place_type<T0>);
  static_assert(not some_sum_storage<T0>);

  T0 v0;

  static constexpr std::size_t size = 1;
  template <std::size_t I> using type = T0;
  template <typename Fn, typename Self>
  static constexpr bool invocable //
      = std::is_invocable_v<Fn, apply_const_lvalue_t<Self, T0 &&>>;
  template <typename Fn, typename Self>
  static constexpr bool invocable_typed
      = std::is_invocable_v<Fn, std::in_place_type_t<T0>, apply_const_lvalue_t<Self, T0 &&>>;

  constexpr sum_storage(std::in_place_type_t<T0>, auto &&...args) noexcept : v0{FWD(args)...} {}
  constexpr ~sum_storage() noexcept {}
  constexpr sum_storage(sum_storage const &) = delete;
  constexpr sum_storage &operator=(sum_storage const &) = delete;

  constexpr T0 *get_ptr(std::in_place_type_t<T0>) noexcept { return &v0; }
  constexpr T0 const *get_ptr(std::in_place_type_t<T0>) const noexcept { return &v0; }
};

template <typename T0, typename T1> union sum_storage<T0, T1> final {
  static_assert(not(std::is_same_v<void, T0> || std::is_same_v<void, T1>));
  static_assert(not(std::is_reference_v<T0> || std::is_reference_v<T1>));
  static_assert(not(some_in_place_type<T0> || some_in_place_type<T1>));
  static_assert(not(some_sum_storage<T0> || some_sum_storage<T1>));

  T0 v0;
  T1 v1;

  static constexpr std::size_t size = 2;
  template <std::size_t I>
    requires(I < size)
  using type = select_nth_t<I, T0, T1>;
  template <typename Fn, typename Self>
  static constexpr bool invocable //
      = std::is_invocable_v<Fn, apply_const_lvalue_t<Self, T0 &&>>
        && std::is_invocable_v<Fn, apply_const_lvalue_t<Self, T1 &&>>;
  template <typename Fn, typename Self>
  static constexpr bool invocable_typed
      = std::is_invocable_v<Fn, std::in_place_type_t<T0>, apply_const_lvalue_t<Self, T0 &&>>
        && std::is_invocable_v<Fn, std::in_place_type_t<T1>, apply_const_lvalue_t<Self, T1 &&>>;

  constexpr sum_storage(std::in_place_type_t<T0>, auto &&...args) noexcept : v0{FWD(args)...} {}
  constexpr sum_storage(std::in_place_type_t<T1>, auto &&...args) noexcept : v1{FWD(args)...} {}
  constexpr ~sum_storage() noexcept {}
  constexpr sum_storage(sum_storage const &) = delete;
  constexpr sum_storage &operator=(sum_storage const &) = delete;

  constexpr T0 *get_ptr(std::in_place_type_t<T0>) noexcept { return &v0; }
  constexpr T0 const *get_ptr(std::in_place_type_t<T0>) const noexcept { return &v0; }
  constexpr T1 *get_ptr(std::in_place_type_t<T1>) noexcept { return &v1; }
  constexpr T1 const *get_ptr(std::in_place_type_t<T1>) const noexcept { return &v1; }
};

template <typename T0, typename T1, typename T2> union sum_storage<T0, T1, T2> final {
  static_assert(not(std::is_same_v<void, T0> || std::is_same_v<void, T1> || std::is_same_v<void, T2>));
  static_assert(not(std::is_reference_v<T0> || std::is_reference_v<T1> || std::is_reference_v<T2>));
  static_assert(not(some_in_place_type<T0> || some_in_place_type<T1> || some_in_place_type<T2>));
  static_assert(not(some_sum_storage<T0> || some_sum_storage<T1> || some_sum_storage<T2>));

  T0 v0;
  T1 v1;
  T2 v2;

  static constexpr std::size_t size = 3;
  template <std::size_t I>
    requires(I < size)
  using type = select_nth_t<I, T0, T1, T2>;
  template <typename Fn, typename Self>
  static constexpr bool invocable //
      = std::is_invocable_v<Fn, apply_const_lvalue_t<Self, T0 &&>>
        && std::is_invocable_v<Fn, apply_const_lvalue_t<Self, T1 &&>>
        && std::is_invocable_v<Fn, apply_const_lvalue_t<Self, T2 &&>>;
  template <typename Fn, typename Self>
  static constexpr bool invocable_typed
      = std::is_invocable_v<Fn, std::in_place_type_t<T0>, apply_const_lvalue_t<Self, T0 &&>>
        && std::is_invocable_v<Fn, std::in_place_type_t<T1>, apply_const_lvalue_t<Self, T1 &&>>
        && std::is_invocable_v<Fn, std::in_place_type_t<T2>, apply_const_lvalue_t<Self, T2 &&>>;

  constexpr sum_storage(std::in_place_type_t<T0>, auto &&...args) noexcept : v0{FWD(args)...} {}
  constexpr sum_storage(std::in_place_type_t<T1>, auto &&...args) noexcept : v1{FWD(args)...} {}
  constexpr sum_storage(std::in_place_type_t<T2>, auto &&...args) noexcept : v2{FWD(args)...} {}
  constexpr ~sum_storage() noexcept {}
  constexpr sum_storage(sum_storage const &) = delete;
  constexpr sum_storage &operator=(sum_storage const &) = delete;

  constexpr T0 *get_ptr(std::in_place_type_t<T0>) noexcept { return &v0; }
  constexpr T0 const *get_ptr(std::in_place_type_t<T0>) const noexcept { return &v0; }
  constexpr T1 *get_ptr(std::in_place_type_t<T1>) noexcept { return &v1; }
  constexpr T1 const *get_ptr(std::in_place_type_t<T1>) const noexcept { return &v1; }
  constexpr T2 *get_ptr(std::in_place_type_t<T2>) noexcept { return &v2; }
  constexpr T2 const *get_ptr(std::in_place_type_t<T2>) const noexcept { return &v2; }
};

template <typename T0, typename T1, typename T2, typename T3> union sum_storage<T0, T1, T2, T3> final {
  static_assert(not(std::is_same_v<void, T0> || std::is_same_v<void, T1> || std::is_same_v<void, T2>
                    || std::is_same_v<void, T3>));
  static_assert(not(std::is_reference_v<T0> || std::is_reference_v<T1> || std::is_reference_v<T2>
                    || std::is_reference_v<T3>));
  static_assert(not(some_in_place_type<T0> || some_in_place_type<T1> || some_in_place_type<T2>
                    || some_in_place_type<T3>));
  static_assert(not(some_sum_storage<T0> || some_sum_storage<T1> || some_sum_storage<T2> || some_sum_storage<T3>));

  T0 v0;
  T1 v1;
  T2 v2;
  T3 v3;

  static constexpr std::size_t size = 4;
  template <std::size_t I>
    requires(I < size)
  using type = select_nth_t<I, T0, T1, T2, T3>;
  template <typename Fn, typename Self>
  static constexpr bool invocable //
      = std::is_invocable_v<Fn, apply_const_lvalue_t<Self, T0 &&>>
        && std::is_invocable_v<Fn, apply_const_lvalue_t<Self, T1 &&>>
        && std::is_invocable_v<Fn, apply_const_lvalue_t<Self, T2 &&>>
        && std::is_invocable_v<Fn, apply_const_lvalue_t<Self, T3 &&>>;
  template <typename Fn, typename Self>
  static constexpr bool invocable_typed
      = std::is_invocable_v<Fn, std::in_place_type_t<T0>, apply_const_lvalue_t<Self, T0 &&>>
        && std::is_invocable_v<Fn, std::in_place_type_t<T1>, apply_const_lvalue_t<Self, T1 &&>>
        && std::is_invocable_v<Fn, std::in_place_type_t<T2>, apply_const_lvalue_t<Self, T2 &&>>
        && std::is_invocable_v<Fn, std::in_place_type_t<T3>, apply_const_lvalue_t<Self, T3 &&>>;

  constexpr sum_storage(std::in_place_type_t<T0>, auto &&...args) noexcept : v0{FWD(args)...} {}
  constexpr sum_storage(std::in_place_type_t<T1>, auto &&...args) noexcept : v1{FWD(args)...} {}
  constexpr sum_storage(std::in_place_type_t<T2>, auto &&...args) noexcept : v2{FWD(args)...} {}
  constexpr sum_storage(std::in_place_type_t<T3>, auto &&...args) noexcept : v3{FWD(args)...} {}
  constexpr ~sum_storage() noexcept {}
  constexpr sum_storage(sum_storage const &) = delete;
  constexpr sum_storage &operator=(sum_storage const &) = delete;

  constexpr T0 *get_ptr(std::in_place_type_t<T0>) noexcept { return &v0; }
  constexpr T0 const *get_ptr(std::in_place_type_t<T0>) const noexcept { return &v0; }
  constexpr T1 *get_ptr(std::in_place_type_t<T1>) noexcept { return &v1; }
  constexpr T1 const *get_ptr(std::in_place_type_t<T1>) const noexcept { return &v1; }
  constexpr T2 *get_ptr(std::in_place_type_t<T2>) noexcept { return &v2; }
  constexpr T2 const *get_ptr(std::in_place_type_t<T2>) const noexcept { return &v2; }
  constexpr T3 *get_ptr(std::in_place_type_t<T3>) noexcept { return &v3; }
  constexpr T3 const *get_ptr(std::in_place_type_t<T3>) const noexcept { return &v3; }
};

template <typename T0, typename T1, typename T2, typename T3, typename... Ts>
  requires(sizeof...(Ts) > 0)
union sum_storage<T0, T1, T2, T3, Ts...> final {
  static_assert(not(std::is_same_v<void, T0> || std::is_same_v<void, T1> || std::is_same_v<void, T2>
                    || std::is_same_v<void, T3> || (... || std::is_same_v<void, Ts>)));
  static_assert(not(std::is_reference_v<T0> || std::is_reference_v<T1> || std::is_reference_v<T2>
                    || std::is_reference_v<T3> || (... || std::is_reference_v<Ts>)));
  static_assert(not(some_in_place_type<T0> || some_in_place_type<T1> || some_in_place_type<T2> || some_in_place_type<T3>
                    || (... || some_in_place_type<Ts>)));
  static_assert(not(some_sum_storage<T0> || some_sum_storage<T1> || some_sum_storage<T2> || some_sum_storage<T3>
                    || (... || some_sum_storage<Ts>)));

  T0 v0;
  T1 v1;
  T2 v2;
  T3 v3;
  sum_storage<Ts...> more;

  static constexpr std::size_t size = 4 + sum_storage<Ts...>::size;
  template <std::size_t I>
    requires(I < size)
  using type = select_nth_t<I, T0, T1, T2, T3, Ts...>;
  template <typename Fn, typename Self>
  static constexpr bool invocable //
      = std::is_invocable_v<Fn, apply_const_lvalue_t<Self, T0 &&>>
        && std::is_invocable_v<Fn, apply_const_lvalue_t<Self, T1 &&>>
        && std::is_invocable_v<Fn, apply_const_lvalue_t<Self, T2 &&>>
        && std::is_invocable_v<Fn, apply_const_lvalue_t<Self, T3 &&>>
        && sum_storage<Ts...>::template invocable<Fn, Self>;
  template <typename Fn, typename Self>
  static constexpr bool invocable_typed
      = std::is_invocable_v<Fn, std::in_place_type_t<T0>, apply_const_lvalue_t<Self, T0 &&>>
        && std::is_invocable_v<Fn, std::in_place_type_t<T1>, apply_const_lvalue_t<Self, T1 &&>>
        && std::is_invocable_v<Fn, std::in_place_type_t<T2>, apply_const_lvalue_t<Self, T2 &&>>
        && std::is_invocable_v<Fn, std::in_place_type_t<T3>, apply_const_lvalue_t<Self, T3 &&>>
        && sum_storage<Ts...>::template invocable_typed<Fn, Self>;

  constexpr sum_storage(std::in_place_type_t<T0>, auto &&...args) noexcept : v0{FWD(args)...} {}
  constexpr sum_storage(std::in_place_type_t<T1>, auto &&...args) noexcept : v1{FWD(args)...} {}
  constexpr sum_storage(std::in_place_type_t<T2>, auto &&...args) noexcept : v2{FWD(args)...} {}
  constexpr sum_storage(std::in_place_type_t<T3>, auto &&...args) noexcept : v3{FWD(args)...} {}
  template <typename T>
    requires(... || std::same_as<T, Ts>)
  constexpr sum_storage(std::in_place_type_t<T>, auto &&...args) noexcept : more(std::in_place_type<T>, FWD(args)...)
  {
  }
  constexpr ~sum_storage() noexcept {}
  constexpr sum_storage(sum_storage const &) = delete;
  constexpr sum_storage &operator=(sum_storage const &) = delete;

  constexpr T0 *get_ptr(std::in_place_type_t<T0>) noexcept { return &v0; }
  constexpr T0 const *get_ptr(std::in_place_type_t<T0>) const noexcept { return &v0; }
  constexpr T1 *get_ptr(std::in_place_type_t<T1>) noexcept { return &v1; }
  constexpr T1 const *get_ptr(std::in_place_type_t<T1>) const noexcept { return &v1; }
  constexpr T2 *get_ptr(std::in_place_type_t<T2>) noexcept { return &v2; }
  constexpr T2 const *get_ptr(std::in_place_type_t<T2>) const noexcept { return &v2; }
  constexpr T3 *get_ptr(std::in_place_type_t<T3>) noexcept { return &v3; }
  constexpr T3 const *get_ptr(std::in_place_type_t<T3>) const noexcept { return &v3; }

  template <typename T>
    requires(... || std::same_as<Ts, T>)
  constexpr auto get_ptr(std::in_place_type_t<T>) noexcept
  {
    return more.get_ptr(std::in_place_type<T>);
  }

  template <typename T>
    requires(... || std::same_as<Ts, T>)
  constexpr auto get_ptr(std::in_place_type_t<T>) const noexcept
  {
    return more.get_ptr(std::in_place_type<T>);
  }
};

template <typename Fn>
constexpr auto _apply_sum_storage_ptr(std::size_t index, Fn &&fn, some_sum_storage auto &&s) noexcept
{
  if constexpr (std::remove_cvref_t<decltype(s)>::size == 1) {
    using T0 = std::remove_cvref_t<decltype(s)>::template type<0>;
    switch (index) { // GCOVR_EXCL_BR_LINE
    case 0:
      return fn(std::in_place_type<T0>, s.get_ptr(std::in_place_type<T0>));
    default:; // GCOVR_EXCL_LINE
    }
  } else if constexpr (std::remove_cvref_t<decltype(s)>::size == 2) {
    using T0 = std::remove_cvref_t<decltype(s)>::template type<0>;
    using T1 = std::remove_cvref_t<decltype(s)>::template type<1>;
    switch (index) { // GCOVR_EXCL_BR_LINE
    case 0:
      return fn(std::in_place_type<T0>, s.get_ptr(std::in_place_type<T0>));
    case 1:
      return fn(std::in_place_type<T1>, s.get_ptr(std::in_place_type<T1>));
    default:; // GCOVR_EXCL_LINE
    }
  } else if constexpr (std::remove_cvref_t<decltype(s)>::size == 3) {
    using T0 = std::remove_cvref_t<decltype(s)>::template type<0>;
    using T1 = std::remove_cvref_t<decltype(s)>::template type<1>;
    using T2 = std::remove_cvref_t<decltype(s)>::template type<2>;
    switch (index) { // GCOVR_EXCL_BR_LINE
    case 0:
      return fn(std::in_place_type<T0>, s.get_ptr(std::in_place_type<T0>));
    case 1:
      return fn(std::in_place_type<T1>, s.get_ptr(std::in_place_type<T1>));
    case 2:
      return fn(std::in_place_type<T2>, s.get_ptr(std::in_place_type<T2>));
    default:; // GCOVR_EXCL_LINE
    }
  } else if constexpr (std::remove_cvref_t<decltype(s)>::size == 4) {
    using T0 = std::remove_cvref_t<decltype(s)>::template type<0>;
    using T1 = std::remove_cvref_t<decltype(s)>::template type<1>;
    using T2 = std::remove_cvref_t<decltype(s)>::template type<2>;
    using T3 = std::remove_cvref_t<decltype(s)>::template type<3>;
    switch (index) { // GCOVR_EXCL_BR_LINE
    case 0:
      return fn(std::in_place_type<T0>, s.get_ptr(std::in_place_type<T0>));
    case 1:
      return fn(std::in_place_type<T1>, s.get_ptr(std::in_place_type<T1>));
    case 2:
      return fn(std::in_place_type<T2>, s.get_ptr(std::in_place_type<T2>));
    case 3:
      return fn(std::in_place_type<T3>, s.get_ptr(std::in_place_type<T3>));
    default:; // GCOVR_EXCL_LINE
    }
  } else if constexpr (std::remove_cvref_t<decltype(s)>::size > 4) {
    using T0 = std::remove_cvref_t<decltype(s)>::template type<0>;
    using T1 = std::remove_cvref_t<decltype(s)>::template type<1>;
    using T2 = std::remove_cvref_t<decltype(s)>::template type<2>;
    using T3 = std::remove_cvref_t<decltype(s)>::template type<3>;
    switch (index) {
    case 0:
      return fn(std::in_place_type<T0>, s.get_ptr(std::in_place_type<T0>));
    case 1:
      return fn(std::in_place_type<T1>, s.get_ptr(std::in_place_type<T1>));
    case 2:
      return fn(std::in_place_type<T2>, s.get_ptr(std::in_place_type<T2>));
    case 3:
      return fn(std::in_place_type<T3>, s.get_ptr(std::in_place_type<T3>));
    default:
      return _apply_sum_storage_ptr(index - 4, FWD(fn), FWD(s).more);
    }
  }
  std::unreachable(); // GCOVR_EXCL_LINE
}

template <typename Fn>
constexpr auto invoke_sum_storage(std::size_t index, Fn &&fn, some_sum_storage auto &&s) noexcept
  requires(std::remove_cvref_t<decltype(s)>::template invocable<Fn, decltype(s)>)
{
  if constexpr (std::remove_cvref_t<decltype(s)>::size == 1) {
    using T0 = std::remove_cvref_t<decltype(s)>::template type<0>;
    switch (index) { // GCOVR_EXCL_BR_LINE
    case 0:
      return std::invoke(fn, static_cast<apply_const_lvalue_t<decltype(s), T0 &&>>(*s.get_ptr(std::in_place_type<T0>)));
    default:; // GCOVR_EXCL_LINE
    }
  } else if constexpr (std::remove_cvref_t<decltype(s)>::size == 2) {
    using T0 = std::remove_cvref_t<decltype(s)>::template type<0>;
    using T1 = std::remove_cvref_t<decltype(s)>::template type<1>;
    switch (index) { // GCOVR_EXCL_BR_LINE
    case 0:
      return std::invoke(fn, static_cast<apply_const_lvalue_t<decltype(s), T0 &&>>(*s.get_ptr(std::in_place_type<T0>)));
    case 1:
      return std::invoke(fn, static_cast<apply_const_lvalue_t<decltype(s), T1 &&>>(*s.get_ptr(std::in_place_type<T1>)));
    default:; // GCOVR_EXCL_LINE
    }
  } else if constexpr (std::remove_cvref_t<decltype(s)>::size == 3) {
    using T0 = std::remove_cvref_t<decltype(s)>::template type<0>;
    using T1 = std::remove_cvref_t<decltype(s)>::template type<1>;
    using T2 = std::remove_cvref_t<decltype(s)>::template type<2>;
    switch (index) { // GCOVR_EXCL_BR_LINE
    case 0:
      return std::invoke(fn, static_cast<apply_const_lvalue_t<decltype(s), T0 &&>>(*s.get_ptr(std::in_place_type<T0>)));
    case 1:
      return std::invoke(fn, static_cast<apply_const_lvalue_t<decltype(s), T1 &&>>(*s.get_ptr(std::in_place_type<T1>)));
    case 2:
      return std::invoke(fn, static_cast<apply_const_lvalue_t<decltype(s), T2 &&>>(*s.get_ptr(std::in_place_type<T2>)));
    default:; // GCOVR_EXCL_LINE
    }
  } else if constexpr (std::remove_cvref_t<decltype(s)>::size == 4) {
    using T0 = std::remove_cvref_t<decltype(s)>::template type<0>;
    using T1 = std::remove_cvref_t<decltype(s)>::template type<1>;
    using T2 = std::remove_cvref_t<decltype(s)>::template type<2>;
    using T3 = std::remove_cvref_t<decltype(s)>::template type<3>;
    switch (index) { // GCOVR_EXCL_BR_LINE
    case 0:
      return std::invoke(fn, static_cast<apply_const_lvalue_t<decltype(s), T0 &&>>(*s.get_ptr(std::in_place_type<T0>)));
    case 1:
      return std::invoke(fn, static_cast<apply_const_lvalue_t<decltype(s), T1 &&>>(*s.get_ptr(std::in_place_type<T1>)));
    case 2:
      return std::invoke(fn, static_cast<apply_const_lvalue_t<decltype(s), T2 &&>>(*s.get_ptr(std::in_place_type<T2>)));
    case 3:
      return std::invoke(fn, static_cast<apply_const_lvalue_t<decltype(s), T3 &&>>(*s.get_ptr(std::in_place_type<T3>)));
    default:; // GCOVR_EXCL_LINE
    }
  } else if constexpr (std::remove_cvref_t<decltype(s)>::size > 4) {
    using T0 = std::remove_cvref_t<decltype(s)>::template type<0>;
    using T1 = std::remove_cvref_t<decltype(s)>::template type<1>;
    using T2 = std::remove_cvref_t<decltype(s)>::template type<2>;
    using T3 = std::remove_cvref_t<decltype(s)>::template type<3>;
    switch (index) {
    case 0:
      return std::invoke(fn, static_cast<apply_const_lvalue_t<decltype(s), T0 &&>>(*s.get_ptr(std::in_place_type<T0>)));
    case 1:
      return std::invoke(fn, static_cast<apply_const_lvalue_t<decltype(s), T1 &&>>(*s.get_ptr(std::in_place_type<T1>)));
    case 2:
      return std::invoke(fn, static_cast<apply_const_lvalue_t<decltype(s), T2 &&>>(*s.get_ptr(std::in_place_type<T2>)));
    case 3:
      return std::invoke(fn, static_cast<apply_const_lvalue_t<decltype(s), T3 &&>>(*s.get_ptr(std::in_place_type<T3>)));
    default:
      return invoke_sum_storage(index - 4, FWD(fn), FWD(s).more);
    }
  }
  std::unreachable(); // GCOVR_EXCL_LINE
}

template <typename Fn>
constexpr auto invoke_sum_storage(std::size_t index, Fn &&fn, some_sum_storage auto &&s) noexcept
  requires(std::remove_cvref_t<decltype(s)>::template invocable_typed<Fn, decltype(s)>)
{
  if constexpr (std::remove_cvref_t<decltype(s)>::size == 1) {
    using T0 = std::remove_cvref_t<decltype(s)>::template type<0>;
    switch (index) { // GCOVR_EXCL_BR_LINE
    case 0:
      return std::invoke(fn, std::in_place_type<T0>,
                         static_cast<apply_const_lvalue_t<decltype(s), T0 &&>>(*s.get_ptr(std::in_place_type<T0>)));
    default:; // GCOVR_EXCL_LINE
    }
  } else if constexpr (std::remove_cvref_t<decltype(s)>::size == 2) {
    using T0 = std::remove_cvref_t<decltype(s)>::template type<0>;
    using T1 = std::remove_cvref_t<decltype(s)>::template type<1>;
    switch (index) { // GCOVR_EXCL_BR_LINE
    case 0:
      return std::invoke(fn, std::in_place_type<T0>,
                         static_cast<apply_const_lvalue_t<decltype(s), T0 &&>>(*s.get_ptr(std::in_place_type<T0>)));
    case 1:
      return std::invoke(fn, std::in_place_type<T1>,
                         static_cast<apply_const_lvalue_t<decltype(s), T1 &&>>(*s.get_ptr(std::in_place_type<T1>)));
    default:; // GCOVR_EXCL_LINE
    }
  } else if constexpr (std::remove_cvref_t<decltype(s)>::size == 3) {
    using T0 = std::remove_cvref_t<decltype(s)>::template type<0>;
    using T1 = std::remove_cvref_t<decltype(s)>::template type<1>;
    using T2 = std::remove_cvref_t<decltype(s)>::template type<2>;
    switch (index) { // GCOVR_EXCL_BR_LINE
    case 0:
      return std::invoke(fn, std::in_place_type<T0>,
                         static_cast<apply_const_lvalue_t<decltype(s), T0 &&>>(*s.get_ptr(std::in_place_type<T0>)));
    case 1:
      return std::invoke(fn, std::in_place_type<T1>,
                         static_cast<apply_const_lvalue_t<decltype(s), T1 &&>>(*s.get_ptr(std::in_place_type<T1>)));
    case 2:
      return std::invoke(fn, std::in_place_type<T2>,
                         static_cast<apply_const_lvalue_t<decltype(s), T2 &&>>(*s.get_ptr(std::in_place_type<T2>)));
    default:; // GCOVR_EXCL_LINE
    }
  } else if constexpr (std::remove_cvref_t<decltype(s)>::size == 4) {
    using T0 = std::remove_cvref_t<decltype(s)>::template type<0>;
    using T1 = std::remove_cvref_t<decltype(s)>::template type<1>;
    using T2 = std::remove_cvref_t<decltype(s)>::template type<2>;
    using T3 = std::remove_cvref_t<decltype(s)>::template type<3>;
    switch (index) { // GCOVR_EXCL_BR_LINE
    case 0:
      return std::invoke(fn, std::in_place_type<T0>,
                         static_cast<apply_const_lvalue_t<decltype(s), T0 &&>>(*s.get_ptr(std::in_place_type<T0>)));
    case 1:
      return std::invoke(fn, std::in_place_type<T1>,
                         static_cast<apply_const_lvalue_t<decltype(s), T1 &&>>(*s.get_ptr(std::in_place_type<T1>)));
    case 2:
      return std::invoke(fn, std::in_place_type<T2>,
                         static_cast<apply_const_lvalue_t<decltype(s), T2 &&>>(*s.get_ptr(std::in_place_type<T2>)));
    case 3:
      return std::invoke(fn, std::in_place_type<T3>,
                         static_cast<apply_const_lvalue_t<decltype(s), T3 &&>>(*s.get_ptr(std::in_place_type<T3>)));
    default:; // GCOVR_EXCL_LINE
    }
  } else if constexpr (std::remove_cvref_t<decltype(s)>::size > 4) {
    using T0 = std::remove_cvref_t<decltype(s)>::template type<0>;
    using T1 = std::remove_cvref_t<decltype(s)>::template type<1>;
    using T2 = std::remove_cvref_t<decltype(s)>::template type<2>;
    using T3 = std::remove_cvref_t<decltype(s)>::template type<3>;
    switch (index) {
    case 0:
      return std::invoke(fn, std::in_place_type<T0>,
                         static_cast<apply_const_lvalue_t<decltype(s), T0 &&>>(*s.get_ptr(std::in_place_type<T0>)));
    case 1:
      return std::invoke(fn, std::in_place_type<T1>,
                         static_cast<apply_const_lvalue_t<decltype(s), T1 &&>>(*s.get_ptr(std::in_place_type<T1>)));
    case 2:
      return std::invoke(fn, std::in_place_type<T2>,
                         static_cast<apply_const_lvalue_t<decltype(s), T2 &&>>(*s.get_ptr(std::in_place_type<T2>)));
    case 3:
      return std::invoke(fn, std::in_place_type<T3>,
                         static_cast<apply_const_lvalue_t<decltype(s), T3 &&>>(*s.get_ptr(std::in_place_type<T3>)));
    default:
      return invoke_sum_storage(index - 4, FWD(fn), FWD(s).more);
    }
  }
  std::unreachable(); // GCOVR_EXCL_LINE
}

} // namespace fn::detail

#endif // INCLUDE_FUNCTIONAL_DETAIL_SUM_STORAGE
