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

namespace detail {
template <typename T>
static constexpr bool _is_valid_sum_subtype //
    = (not std::is_same_v<void, T>)&&(not std::is_reference_v<T>)&&(not some_sum<T>)&&(not some_in_place_type<T>);
}

template <typename... Ts>
  requires(sizeof...(Ts) > 0)
struct sum<Ts...> {
  static_assert((... && detail::_is_valid_sum_subtype<Ts>));
  static_assert(detail::is_normal_v<Ts...>);

  using data_t = detail::variadic_union<Ts...>;
  data_t data;
  std::size_t index;

  static constexpr std::size_t size = sizeof...(Ts);
  template <std::size_t I> using select_nth = detail::select_nth_t<I, Ts...>;
  template <typename T> static constexpr bool has_type = data_t::template has_type<T>;

  template <typename T>
  constexpr sum(T &&v)
    requires has_type<T> && (std::is_constructible_v<T, decltype(v)>) && (std::is_convertible_v<decltype(v), T>)
      : data(detail::make_variadic_union<T, data_t>(FWD(v))), index(detail::type_index<T, Ts...>)
  {
  }

  template <typename T>
  constexpr explicit sum(T &&v)
    requires has_type<T> && (std::is_constructible_v<T, decltype(v)>) && (not std::is_convertible_v<decltype(v), T>)
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
    requires detail::is_superset_of<sum, sum<Tx...>>
                 && (not std::is_same_v<sum, sum<Tx...>>) && (... && std::is_copy_constructible_v<Tx>)
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
    requires detail::is_superset_of<sum, sum<Tx...>>
                 && (not std::is_same_v<sum, sum<Tx...>>) && (... && std::is_move_constructible_v<Tx>)
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
    requires std::is_same_v<std::remove_cvref_t<decltype(arg)>, sum<Tx...>> && detail::is_superset_of<sum, sum<Tx...>>
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
  [[nodiscard]] constexpr bool has_value(std::in_place_type_t<T> = std::in_place_type<T>) const noexcept
  {
    return invoke([]<typename U>(std::in_place_type_t<U>, auto &&) constexpr -> bool { return std::is_same_v<T, U>; });
  }

  template <typename T>
    requires has_type<T>
  [[nodiscard]] constexpr T *get_ptr(std::in_place_type_t<T> = std::in_place_type<T>) noexcept
  {
    return has_value(std::in_place_type<T>) ? detail::ptr_variadic_union<T, data_t>(data) : nullptr;
  }

  template <typename T>
    requires has_type<T>
  [[nodiscard]] constexpr T const *get_ptr(std::in_place_type_t<T> = std::in_place_type<T>) const noexcept
  {
    return has_value(std::in_place_type<T>) ? detail::ptr_variadic_union<T, data_t>(data) : nullptr;
  }

  [[nodiscard]] constexpr bool operator==(sum const &other) const noexcept
    requires(... && std::equality_comparable<Ts>)
  {
    return invoke([&other]<typename T>(std::in_place_type_t<T>, auto const &lh) -> bool {
      return other.invoke([&lh]<typename U>(std::in_place_type_t<U>, auto const &rh) -> bool { //
        if constexpr (std::is_same_v<T, U>)
          return lh == rh;
        return false;
      });
    });
  }

  [[nodiscard]] constexpr bool operator!=(sum const &other) const noexcept
    requires(... && std::equality_comparable<Ts>)
  {
    return not(*this == other);
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) & noexcept
    requires detail::typelist_invocable<Fn, sum &> || detail::typelist_type_invocable<Fn, sum &>
  {
    return detail::invoke_variadic_union<data_t>(this->data, index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) const & noexcept
    requires detail::typelist_invocable<Fn, sum const &> || detail::typelist_type_invocable<Fn, sum const &>
  {
    return detail::invoke_variadic_union<data_t>(this->data, index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) && noexcept
    requires detail::typelist_invocable<Fn, sum &&> || detail::typelist_type_invocable<Fn, sum &&>
  {
    return detail::invoke_variadic_union<data_t>(std::move(*this).data, index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) const && noexcept
    requires detail::typelist_invocable<Fn, sum const &&> || detail::typelist_type_invocable<Fn, sum const &&>
  {
    return detail::invoke_variadic_union<data_t>(std::move(*this).data, index, FWD(fn));
  }
};

// CTAD for single-element sum
template <typename T> sum(std::in_place_type_t<T>, auto &&...) -> sum<T>;
template <typename T> sum(T) -> sum<T>;

template <typename... Ts> using sum_for = detail::normalized<Ts...>::template apply<sum>;

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_SUM
