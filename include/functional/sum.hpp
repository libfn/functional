// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_SUM
#define INCLUDE_FUNCTIONAL_SUM

#include "functional/detail/functional.hpp"
#include "functional/detail/meta.hpp"
#include "functional/detail/traits.hpp"
#include "functional/detail/variadic_union.hpp"
#include "functional/functional.hpp"
#include "functional/fwd.hpp"
#include "functional/utility.hpp"

#include <type_traits>
#include <utility>

namespace fn {

template <typename T>
concept some_sum = detail::_some_sum<T>;

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

struct _invoke_autodetect_tag final {};

template <typename Fn, typename Self, typename T> struct _typelist_select_invoke_result;
template <typename Fn, typename Self, template <typename...> typename Tpl, typename... Ts>
struct _typelist_select_invoke_result<Fn, Self, Tpl<Ts...>> {
  using T0 = select_nth_t<0, Ts...>;
  using R0 = ::fn::detail::_invoke_result_t<Fn, apply_const_lvalue_t<Self, T0>>;
  static_assert((...
                 && std::is_same_v<R0, typename ::fn::detail::_invoke_result_t<Fn, apply_const_lvalue_t<Self, Ts>>>));
  using type = R0;
};

template <typename Fn, typename Self, typename T> struct _typelist_type_select_invoke_result;
template <typename Fn, typename Self, template <typename...> typename Tpl, typename... Ts>
struct _typelist_type_select_invoke_result<Fn, Self, Tpl<Ts...>> {
  using T0 = select_nth_t<0, Ts...>;
  using R0 = ::fn::detail::_invoke_result_t<Fn, std::in_place_type_t<T0>, apply_const_lvalue_t<Self, T0>>;
  static_assert((...
                 && std::is_same_v<R0, typename ::fn::detail::_invoke_result_t<Fn, std::in_place_type_t<Ts>,
                                                                               apply_const_lvalue_t<Self, Ts>>>));
  using type = R0;
};

struct _collapsing_sum_tag final {};

namespace _collapsing_sum {
template <typename... Ts> struct typelist;
template <typename... Ts> extern typelist<Ts...> const &typelist_v;

template <typename... Ts, typename T>
auto operator^(typelist<Ts...> const &, typelist<T> const &) -> typelist<Ts..., T> const &;
template <typename... Ts, typename... Us>
auto operator^(typelist<Ts...> const &, typelist<::fn::sum<Us...>> const &) -> typelist<Ts..., Us...> const &;

template <typename... Ts> using flattened = std::remove_cvref_t<decltype((typelist_v<> ^ ... ^ typelist_v<Ts>))>;

template <template <typename...> typename Tpl, typename T> struct normalized;
template <template <typename...> typename Tpl, typename... Ts> struct normalized<Tpl, typelist<Ts...>> {
  using type = typename ::fn::detail::normalized<Ts...>::template apply<Tpl>;
};
} // namespace _collapsing_sum

template <typename Fn, typename Self, typename T> struct _typelist_collapsing_sum;
template <typename Fn, typename Self, template <typename...> typename Tpl, typename... Ts>
struct _typelist_collapsing_sum<Fn, Self, Tpl<Ts...>> {
  using type = _collapsing_sum::normalized<
      Tpl, _collapsing_sum::flattened<
               std::remove_cvref_t<::fn::detail::_invoke_result_t<Fn, apply_const_lvalue_t<Self, Ts>>>...>>::type;
};

template <typename Fn, typename Self, typename T> struct _typelist_type_collapsing_sum;
template <typename Fn, typename Self, template <typename...> typename Tpl, typename... Ts>
struct _typelist_type_collapsing_sum<Fn, Self, Tpl<Ts...>> {
  using type
      = _collapsing_sum::normalized<Tpl, _collapsing_sum::flattened<std::remove_cvref_t<::fn::detail::_invoke_result_t<
                                             Fn, std::in_place_type_t<Ts>, apply_const_lvalue_t<Self, Ts>>>...>>::type;
};

template <typename T, typename Fn, typename Self> struct _select_invoke_result final {
  using type = T;
};
template <typename Fn, typename Self> struct _select_invoke_result<_invoke_autodetect_tag, Fn, Self> final {
  using type = _typelist_select_invoke_result<Fn, Self, std::remove_cvref_t<Self>>::type;
};
template <typename Fn, typename Self> struct _select_invoke_result<_collapsing_sum_tag, Fn, Self> final {
  using type = _typelist_collapsing_sum<Fn, Self, std::remove_cvref_t<Self>>::type;
};

template <typename T, typename Fn, typename Self> struct _invoke_type_result final {
  using type = T;
};
template <typename Fn, typename Self> struct _invoke_type_result<_invoke_autodetect_tag, Fn, Self> final {
  using type = _typelist_type_select_invoke_result<Fn, Self, std::remove_cvref_t<Self>>::type;
};
template <typename Fn, typename Self> struct _invoke_type_result<_collapsing_sum_tag, Fn, Self> final {
  using type = _typelist_type_collapsing_sum<Fn, Self, std::remove_cvref_t<Self>>::type;
};

} // namespace detail

template <typename... Ts> struct sum;
template <> struct sum<>; // Intentionally incomplete

template <typename... Ts>
  requires(sizeof...(Ts) > 0)
struct sum<Ts...> {
  static_assert((... && detail::_is_valid_sum_subtype<Ts>));
  static_assert(std::same_as<typename detail::normalized<Ts...>::template apply<::fn::sum>, sum>);

  using data_t = detail::variadic_union<Ts...>;
  data_t data;
  std::size_t index;

  static constexpr std::size_t size = sizeof...(Ts);
  template <std::size_t I> using select_nth = detail::select_nth_t<I, Ts...>;
  template <typename T> static constexpr bool has_type = data_t::template has_type<T>;

  template <typename T>
  constexpr sum(T &&v)
    requires has_type<std::remove_cvref_t<T>> && (std::is_constructible_v<std::remove_cvref_t<T>, decltype(v)>)
                 && (std::is_convertible_v<decltype(v), std::remove_cvref_t<T>>)
      : data(detail::make_variadic_union<std::remove_cvref_t<T>, data_t>(FWD(v))),
        index(detail::type_index<std::remove_cvref_t<T>, Ts...>)
  {
  }

  template <typename T>
  constexpr explicit sum(T &&v)
    requires has_type<std::remove_cvref_t<T>> && (std::is_constructible_v<std::remove_cvref_t<T>, decltype(v)>)
                 && (not std::is_convertible_v<decltype(v), std::remove_cvref_t<T>>)
      : data(detail::make_variadic_union<std::remove_cvref_t<T>, data_t>(FWD(v))),
        index(detail::type_index<std::remove_cvref_t<T>, Ts...>)
  {
  }

  template <typename T>
  constexpr sum(std::in_place_type_t<T>, auto &&...args)
    requires has_type<T>
      : data(detail::make_variadic_union<T, data_t>(FWD(args)...)), index(detail::type_index<T, Ts...>)
  {
  }

  template <typename... Tx>
  constexpr sum(sum<Tx...> const &arg) noexcept
    requires detail::is_superset_of<sum, sum<Tx...>>
                 && (not std::is_same_v<sum, sum<Tx...>>) && (... && std::is_copy_constructible_v<Tx>)
      : data(FWD(arg).template invoke_r<data_t>([]<typename T>(std::in_place_type_t<T>, auto &&v) {
          return detail::make_variadic_union<T, data_t>(FWD(v));
        })),
        index(FWD(arg).template invoke_r<std::size_t>([]<typename T>(std::in_place_type_t<T>, auto &&) { //
          return detail::type_index<T, Ts...>;
        }))
  {
  }

  template <typename... Tx>
  constexpr sum(sum<Tx...> &&arg) noexcept
    requires detail::is_superset_of<sum, sum<Tx...>>
                 && (not std::is_same_v<sum, sum<Tx...>>) && (... && std::is_move_constructible_v<Tx>)
      : data(FWD(arg).template invoke_r<data_t>([]<typename T>(std::in_place_type_t<T>, auto &&v) {
          return detail::make_variadic_union<T, data_t>(FWD(v));
        })),
        index(FWD(arg).template invoke_r<std::size_t>([]<typename T>(std::in_place_type_t<T>, auto &&) { //
          return detail::type_index<T, Ts...>;
        }))
  {
  }

  template <typename... Tx>
  constexpr sum(std::in_place_type_t<sum<Tx...>>, some_sum auto &&arg) noexcept
    requires std::is_same_v<std::remove_cvref_t<decltype(arg)>, sum<Tx...>> && detail::is_superset_of<sum, sum<Tx...>>
      : data(FWD(arg).template invoke_r<data_t>([]<typename T>(std::in_place_type_t<T>, auto &&v) {
          return detail::make_variadic_union<T, data_t>(FWD(v));
        })),
        index(FWD(arg).template invoke_r<std::size_t>([]<typename T>(std::in_place_type_t<T>, auto &&) { //
          return detail::type_index<T, Ts...>;
        }))
  {
  }

  constexpr sum(sum const &other) noexcept
    requires(... && std::is_copy_constructible_v<Ts>)
      : data(detail::invoke_variadic_union<data_t, data_t>(        //
          other.data, other.index,                                 //
          []<typename T>(std::in_place_type_t<T>, auto const &v) { //
            return detail::make_variadic_union<T, data_t>(v);
          })),
        index(other.index)
  {
  }

  constexpr sum(sum &&other) noexcept
    requires(... && std::is_move_constructible_v<Ts>)
      : data(detail::invoke_variadic_union<data_t, data_t>(   //
          std::move(other).data, other.index,                 //
          []<typename T>(std::in_place_type_t<T>, auto &&v) { //
            return detail::make_variadic_union<T, data_t>(std::move(v));
          })),
        index(other.index)
  {
  }

  constexpr ~sum() noexcept
  {
    detail::invoke_variadic_union<void, data_t>( //
        this->data, index, [this]<typename T>(std::in_place_type_t<T>, auto &&) {
          std::destroy_at(detail::ptr_variadic_union<T, data_t>(this->data));
        });
  }

  template <typename T>
    requires has_type<T>
  [[nodiscard]] constexpr bool has_value(std::in_place_type_t<T> = std::in_place_type<T>) const noexcept
  {
    return invoke_r<bool>(
        []<typename U>(std::in_place_type_t<U>, auto &&) constexpr -> bool { return std::is_same_v<T, U>; });
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

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) & noexcept
    requires typelist_invocable<Fn, sum &> && (not typelist_type_invocable<Fn, sum &>)
  {
    using type = detail::_select_invoke_result<detail::_collapsing_sum_tag, decltype(fn), sum &>::type;
    return detail::invoke_variadic_union<type, data_t>(this->data, index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) & noexcept
    requires typelist_type_invocable<Fn, sum &>
  {
    using type = detail::_invoke_type_result<detail::_collapsing_sum_tag, decltype(fn), sum &>::type;
    return detail::invoke_variadic_union<type, data_t>(this->data, index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) const & noexcept
    requires typelist_invocable<Fn, sum const &> && (not typelist_type_invocable<Fn, sum const &>)
  {
    using type = detail::_select_invoke_result<detail::_collapsing_sum_tag, decltype(fn), sum const &>::type;
    return detail::invoke_variadic_union<type, data_t>(this->data, index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) const & noexcept
    requires typelist_type_invocable<Fn, sum const &>
  {
    using type = detail::_invoke_type_result<detail::_collapsing_sum_tag, decltype(fn), sum const &>::type;
    return detail::invoke_variadic_union<type, data_t>(this->data, index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) && noexcept
    requires typelist_invocable<Fn, sum &&> && (not typelist_type_invocable<Fn, sum &&>)
  {
    using type = detail::_select_invoke_result<detail::_collapsing_sum_tag, decltype(fn), sum &&>::type;
    return detail::invoke_variadic_union<type, data_t>(std::move(*this).data, index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) && noexcept
    requires typelist_type_invocable<Fn, sum &&>
  {
    using type = detail::_invoke_type_result<detail::_collapsing_sum_tag, decltype(fn), sum &&>::type;
    return detail::invoke_variadic_union<type, data_t>(std::move(*this).data, index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) const && noexcept
    requires typelist_invocable<Fn, sum const &&> && (not typelist_type_invocable<Fn, sum const &&>)
  {
    using type = detail::_select_invoke_result<detail::_collapsing_sum_tag, decltype(fn), sum const &&>::type;
    return detail::invoke_variadic_union<type, data_t>(std::move(*this).data, index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) const && noexcept
    requires typelist_type_invocable<Fn, sum const &&>
  {
    using type = detail::_invoke_type_result<detail::_collapsing_sum_tag, decltype(fn), sum const &&>::type;
    return detail::invoke_variadic_union<type, data_t>(std::move(*this).data, index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) & noexcept
    requires typelist_invocable<Fn, sum &> && (not typelist_type_invocable<Fn, sum &>)
  {
    using type = detail::_select_invoke_result<detail::_invoke_autodetect_tag, decltype(fn), sum &>::type;
    return detail::invoke_variadic_union<type, data_t>(this->data, index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) & noexcept
    requires typelist_type_invocable<Fn, sum &>
  {
    using type = detail::_invoke_type_result<detail::_invoke_autodetect_tag, decltype(fn), sum &>::type;
    return detail::invoke_variadic_union<type, data_t>(this->data, index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) const & noexcept
    requires typelist_invocable<Fn, sum const &> && (not typelist_type_invocable<Fn, sum const &>)
  {
    using type = detail::_select_invoke_result<detail::_invoke_autodetect_tag, decltype(fn), sum const &>::type;
    return detail::invoke_variadic_union<type, data_t>(this->data, index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) const & noexcept
    requires typelist_type_invocable<Fn, sum const &>
  {
    using type = detail::_invoke_type_result<detail::_invoke_autodetect_tag, decltype(fn), sum const &>::type;
    return detail::invoke_variadic_union<type, data_t>(this->data, index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) && noexcept
    requires typelist_invocable<Fn, sum &&> && (not typelist_type_invocable<Fn, sum &&>)
  {
    using type = detail::_select_invoke_result<detail::_invoke_autodetect_tag, decltype(fn), sum &&>::type;
    return detail::invoke_variadic_union<type, data_t>(std::move(*this).data, index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) && noexcept
    requires typelist_type_invocable<Fn, sum &&>
  {
    using type = detail::_invoke_type_result<detail::_invoke_autodetect_tag, decltype(fn), sum &&>::type;
    return detail::invoke_variadic_union<type, data_t>(std::move(*this).data, index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) const && noexcept
    requires typelist_invocable<Fn, sum const &&> && (not typelist_type_invocable<Fn, sum const &&>)
  {
    using type = detail::_select_invoke_result<detail::_invoke_autodetect_tag, decltype(fn), sum const &&>::type;
    return detail::invoke_variadic_union<type, data_t>(std::move(*this).data, index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) const && noexcept
    requires typelist_type_invocable<Fn, sum const &&>
  {
    using type = detail::_invoke_type_result<detail::_invoke_autodetect_tag, decltype(fn), sum const &&>::type;
    return detail::invoke_variadic_union<type, data_t>(std::move(*this).data, index, FWD(fn));
  }

  template <typename T, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn) & noexcept
    requires typelist_invocable<Fn, sum &> && (not typelist_type_invocable<Fn, sum &>)
  {
    using type = detail::_select_invoke_result<T, decltype(fn), sum &>::type;
    return detail::invoke_variadic_union<type, data_t>(this->data, index, FWD(fn));
  }

  template <typename T, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn) & noexcept
    requires typelist_type_invocable<Fn, sum &>
  {
    using type = detail::_invoke_type_result<T, decltype(fn), sum &>::type;
    return detail::invoke_variadic_union<type, data_t>(this->data, index, FWD(fn));
  }

  template <typename T, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn) const & noexcept
    requires typelist_invocable<Fn, sum const &> && (not typelist_type_invocable<Fn, sum const &>)
  {
    using type = detail::_select_invoke_result<T, decltype(fn), sum const &>::type;
    return detail::invoke_variadic_union<type, data_t>(this->data, index, FWD(fn));
  }

  template <typename T, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn) const & noexcept
    requires typelist_type_invocable<Fn, sum const &>
  {
    using type = detail::_invoke_type_result<T, decltype(fn), sum const &>::type;
    return detail::invoke_variadic_union<type, data_t>(this->data, index, FWD(fn));
  }

  template <typename T, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn) && noexcept
    requires typelist_invocable<Fn, sum &&> && (not typelist_type_invocable<Fn, sum &&>)
  {
    using type = detail::_select_invoke_result<T, decltype(fn), sum &&>::type;
    return detail::invoke_variadic_union<type, data_t>(std::move(*this).data, index, FWD(fn));
  }

  template <typename T, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn) && noexcept
    requires typelist_type_invocable<Fn, sum &&>
  {
    using type = detail::_invoke_type_result<T, decltype(fn), sum &&>::type;
    return detail::invoke_variadic_union<type, data_t>(std::move(*this).data, index, FWD(fn));
  }

  template <typename T, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn) const && noexcept
    requires typelist_invocable<Fn, sum const &&> && (not typelist_type_invocable<Fn, sum const &&>)
  {
    using type = detail::_select_invoke_result<T, decltype(fn), sum const &&>::type;
    return detail::invoke_variadic_union<type, data_t>(std::move(*this).data, index, FWD(fn));
  }

  template <typename T, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn) const && noexcept
    requires typelist_type_invocable<Fn, sum const &&>
  {
    using type = detail::_invoke_type_result<T, decltype(fn), sum const &&>::type;
    return detail::invoke_variadic_union<type, data_t>(std::move(*this).data, index, FWD(fn));
  }
};

// CTAD for single-element sum
template <typename T> explicit sum(std::in_place_type_t<T>, auto &&...) -> sum<T>;
template <typename T> explicit sum(T) -> sum<T>;

template <typename... Ts, typename... Tx>
[[nodiscard]] constexpr bool operator==(sum<Ts...> const &lh, sum<Tx...> const &rh) noexcept
  requires(... && (std::equality_comparable<Ts> || not detail::type_one_of<Ts, Tx...>))
{
  return lh.template invoke_r<bool>([&rh]<typename T>(std::in_place_type_t<T> d, auto const &lh) noexcept {
    if constexpr (std::remove_cvref_t<decltype(rh)>::template has_type<T>) {
      return rh.has_value(d) && lh == *rh.get_ptr(d); // GCOVR_EXCL_BR_LINE
    } else {
      return false;
    }
  });
}

template <typename... Ts, typename... Tx>
[[nodiscard]] constexpr bool operator!=(sum<Ts...> const &lh, sum<Tx...> const &rh) noexcept
  requires(... && (std::equality_comparable<Ts> || not detail::type_one_of<Ts, Tx...>))
{
  return not(lh == rh);
}

template <typename... Ts> using sum_for = detail::normalized<Ts...>::template apply<sum>;

namespace detail {
template <typename Fn, typename T, typename... Ts> struct _transform_result;

template <typename Fn, typename T, typename... Ts>
  requires(not some_sum<T>)
struct _transform_result<Fn, T, Ts...> {
  using type = _invoke_result<Fn, T, Ts...>::type;
};

template <typename Fn, typename T>
  requires some_sum<T>
struct _transform_result<Fn, T> {
  using type = decltype(std::declval<T>().transform(std::declval<Fn>()));
};
} // namespace detail

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_SUM
