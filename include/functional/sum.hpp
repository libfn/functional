// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_SUM
#define INCLUDE_FUNCTIONAL_SUM

#include "functional/detail/variadic_union.hpp"
#include "functional/utility.hpp"
#include <utility>

namespace fn {

template <typename... Ts> struct sum;
template <> struct sum<>; // Intentionally incomplete

namespace detail {
template <typename... Ts> constexpr bool _is_sum = false;
template <typename... Ts> constexpr bool _is_sum<sum<Ts...> &> = true;
template <typename... Ts> constexpr bool _is_sum<sum<Ts...> const &> = true;
} // namespace detail

template <typename T>
concept some_sum = detail::_is_sum<T &>;

namespace detail {
template <typename T> constexpr bool _is_in_place_type = false;
template <typename T> constexpr bool _is_in_place_type<::std::in_place_type_t<T> &> = true;
template <typename T> constexpr bool _is_in_place_type<::std::in_place_type_t<T> const &> = true;
} // namespace detail

template <typename T>
concept some_in_place_type = detail::_is_in_place_type<T &>;

template <typename... Ts>
  requires(sizeof...(Ts) > 0)
struct sum<Ts...> final {
  template <typename T>
  static constexpr bool _is_valid_subtype //
      = (not std::same_as<void, T>)&&(not std::is_reference_v<T>)&&(not some_sum<T>)&&(not some_in_place_type<T>);
  static_assert((... && _is_valid_subtype<Ts>));
  static_assert(std::is_same_v<typename detail::normalized<Ts...>::template apply<sum>, sum>);
  static_assert(detail::is_normal_v<Ts...>);

  using data_t = detail::variadic_union<Ts...>;
  data_t data;
  std::size_t index;

  static constexpr std::size_t size = data_t::size;
  template <std::size_t I> using type = detail::select_nth_t<I, Ts...>;
  template <typename T> static constexpr bool has_type = data_t::template has_type<T>;

  template <typename Fn, typename Self> static constexpr bool invocable = detail::typelist_invocable<Fn, Self>;
  template <typename Fn, typename Self>
  static constexpr bool type_invocable = detail::typelist_type_invocable<Fn, Self>;

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
    requires type_invocable<Fn, Self>
  struct invoke_result<Fn, Self> final {
    using _T0 = detail::select_nth_t<0, Ts...>;
    using _R0 = std::invoke_result_t<Fn, std::in_place_type_t<_T0>, apply_const_lvalue_t<Self, _T0 &&>>;
    static_assert(
        (...
         && std::same_as<_R0, std::invoke_result_t<Fn, std::in_place_type_t<_T0>, apply_const_lvalue_t<Self, Ts &&>>>));
    using type = _R0;
  };
  template <typename Fn, typename Self> using invoke_result_t = invoke_result<Fn, Self>::type;

  template <typename T>
  constexpr sum(T &&v)
    requires has_type<T> && (size == 1) && (not some_sum<T>) && (not some_in_place_type<T>)
                 && (... || std::is_constructible_v<Ts, T>) && (... || std::is_convertible_v<T, Ts>)
      : data(detail::make_variadic_union<T, data_t>(FWD(v))), index(detail::type_index<T, Ts...>)
  {
  }

  template <typename T>
  constexpr explicit sum(T &&v)
    requires has_type<T> && (size == 1) && (not some_sum<T>) && (not some_in_place_type<T>)
                 && (... || std::is_constructible_v<Ts, T>) && (not(... || std::is_convertible_v<T, Ts>))
      : data(detail::make_variadic_union<T, data_t>(FWD(v))), index(detail::type_index<T, Ts...>)
  {
  }

  template <typename T>
  constexpr sum(std::in_place_type_t<T>, auto &&...args)
    requires has_type<T>
      : data(detail::make_variadic_union<T, data_t>(FWD(args)...)), index(detail::type_index<T, Ts...>)
  {
  }

  template <typename... Tx>
  explicit constexpr sum(sum<Tx...> const &arg) noexcept
    requires detail::is_superset_of<sum<Ts...>, sum<Tx...>> && (not std::same_as<sum<Tx...>, sum<Ts...>>)
      : data(FWD(arg).invoke([]<typename T>(std::in_place_type_t<T>, auto &&v) {
          return detail::make_variadic_union<T, data_t>(FWD(v));
        })),
        index(FWD(arg).invoke([]<typename T>(std::in_place_type_t<T>, auto &&) { //
          return detail::type_index<T, Ts...>;
        }))
  {
  }

  template <typename... Tx>
  explicit constexpr sum(sum<Tx...> &&arg) noexcept
    requires detail::is_superset_of<sum<Ts...>, sum<Tx...>> && (not std::same_as<sum<Tx...>, sum<Ts...>>)
      : data(FWD(arg).invoke([]<typename T>(std::in_place_type_t<T>, auto &&v) {
          return detail::make_variadic_union<T, data_t>(FWD(v));
        })),
        index(FWD(arg).invoke([]<typename T>(std::in_place_type_t<T>, auto &&) { //
          return detail::type_index<T, Ts...>;
        }))
  {
  }

  template <typename... Tx>
  constexpr sum(std::in_place_type_t<sum<Tx...>>, some_sum auto &&arg) noexcept
    requires std::same_as<std::remove_cvref_t<decltype(arg)>, sum<Tx...>>
                 && detail::is_superset_of<sum<Ts...>, sum<Tx...>> && (not std::same_as<sum<Tx...>, sum<Ts...>>)
      : data(FWD(arg).invoke([]<typename T>(std::in_place_type_t<T>, auto &&v) {
          return detail::make_variadic_union<T, data_t>(FWD(v));
        })),
        index(FWD(arg).invoke([]<typename T>(std::in_place_type_t<T>, auto &&) { //
          return detail::type_index<T, Ts...>;
        }))
  {
  }

  constexpr sum(sum const &other) noexcept
    requires(... && std::is_copy_constructible_v<Ts>)
      : data(detail::invoke_variadic_union<data_t>(                          //
          other.data, other.index,                                           //
          []<typename T>(std::in_place_type_t<T>, auto const &v) -> data_t { //
            return detail::make_variadic_union<T, data_t>(v);
          })),
        index(other.index)
  {
  }

  constexpr sum(sum &&other) noexcept
    requires(... && std::is_move_constructible_v<Ts>)
      : data(detail::invoke_variadic_union<data_t>(                     //
          std::move(other).data, other.index,                           //
          []<typename T>(std::in_place_type_t<T>, auto &&v) -> data_t { //
            return detail::make_variadic_union<T, data_t>(std::move(v));
          })),
        index(other.index)
  {
  }

  constexpr ~sum() noexcept
  {
    detail::invoke_variadic_union<data_t>(this->data, index, [this]<typename T>(std::in_place_type_t<T>, auto &&) {
      std::destroy_at(detail::ptr_variadic_union<T, data_t>(this->data));
    });
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
    requires has_type<T>
  [[nodiscard]] constexpr T *get_ptr(std::in_place_type_t<T>) noexcept
  {
    return has_value(std::in_place_type<T>) ? detail::ptr_variadic_union<T, data_t>(data) : nullptr;
  }

  template <typename T>
    requires has_type<T>
  [[nodiscard]] constexpr T const *get_ptr(std::in_place_type_t<T>) const noexcept
  {
    return has_value(std::in_place_type<T>) ? detail::ptr_variadic_union<T, data_t>(data) : nullptr;
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
    return invoke([&other]<typename T>(std::in_place_type_t<T>, auto const &lh) -> bool {
      return other.invoke([&lh]<typename U>(std::in_place_type_t<U>, auto const &rh) -> bool { //
        if constexpr (std::same_as<T, U>)
          return lh == rh;
        return false;
      });
    });
  }

  [[nodiscard]] constexpr bool operator!=(sum<Ts...> const &other) const noexcept
    requires(... && std::equality_comparable<Ts>)
  {
    return not(*this == other);
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) & noexcept
    requires invocable<Fn, sum &> || type_invocable<Fn, sum &>
  {
    return detail::invoke_variadic_union<data_t>(this->data, index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) const & noexcept
    requires invocable<Fn, sum const &> || type_invocable<Fn, sum const &>
  {
    return detail::invoke_variadic_union<data_t>(this->data, index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) && noexcept
    requires invocable<Fn, sum &&> || type_invocable<Fn, sum &&>
  {
    return detail::invoke_variadic_union<data_t>(std::move(*this).data, index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) const && noexcept
    requires invocable<Fn, sum const &&> || type_invocable<Fn, sum const &&>
  {
    return detail::invoke_variadic_union<data_t>(std::move(*this).data, index, FWD(fn));
  }
};

// CTAD for single-element sum
template <typename T> sum(std::in_place_type_t<T>, auto &&...) -> sum<T>;
template <typename T> sum(T) -> sum<T>;

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_SUM
