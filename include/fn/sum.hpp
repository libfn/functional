// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_SUM
#define INCLUDE_FUNCTIONAL_SUM

#include <fn/detail/functional.hpp>
#include <fn/detail/meta.hpp>
#include <fn/detail/traits.hpp>
#include <fn/detail/variadic_union.hpp>
#include <fn/functional.hpp>

#include <type_traits>
#include <utility>

namespace fn {

/**
 * @brief TODO
 *
 * @tparam T TODO
 */
template <typename T>
concept some_sum = detail::_some_sum<T>;

/**
 * @brief TODO
 *
 * @tparam T TODO
 */
template <typename T>
concept some_in_place_type = detail::_some_in_place_type<T>;

namespace detail {
template <typename T>
static constexpr bool _is_valid_sum_subtype //
    = (not std::is_same_v<void, T>)         //
    &&(not std::is_reference_v<T>)          //
    &&(not some_sum<T>)                     //
    &&(not some_in_place_type<T>)           //
    &&(std::is_same_v<T, std::remove_cv_t<T>>);

struct _invoke_autodetect_tag final {};

template <typename Fn, typename Self, typename T, typename... Args> struct _typelist_select_invoke_result;
template <typename Fn, typename Self, template <typename...> typename Tpl, typename... Ts, typename... Args>
struct _typelist_select_invoke_result<Fn, Self, Tpl<Ts...>, Args...> {
  using T0 = select_nth_t<0, Ts...>;
  using R0 = ::fn::detail::_invoke_result<Fn, apply_const_lvalue_t<Self, T0>, Args...>::type;
  static_assert((
      ...
      && std::is_same_v<R0, typename ::fn::detail::_invoke_result<Fn, apply_const_lvalue_t<Self, Ts>, Args...>::type>));
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

template <typename Fn, typename Self, typename T, typename... Args> struct _typelist_collapsing_sum;
template <typename Fn, typename Self, template <typename...> typename Tpl, typename... Ts, typename... Args>
struct _typelist_collapsing_sum<Fn, Self, Tpl<Ts...>, Args...> {
  using type = _collapsing_sum::normalized<
      Tpl, _collapsing_sum::flattened<std::remove_cvref_t<
               typename ::fn::detail::_invoke_result<Fn, apply_const_lvalue_t<Self, Ts>, Args...>::type>...>>::type;
};

template <typename T, typename Fn, typename Self, typename... Args> struct _sum_invoke_result final {
  using type = T;
};
template <typename Fn, typename Self, typename... Args>
struct _sum_invoke_result<_invoke_autodetect_tag, Fn, Self, Args...> final {
  using type = _typelist_select_invoke_result<Fn, Self, std::remove_cvref_t<Self>, Args...>::type;
};
template <typename Fn, typename Self, typename... Args>
struct _sum_invoke_result<_collapsing_sum_tag, Fn, Self, Args...> final {
  using type = _typelist_collapsing_sum<Fn, Self, std::remove_cvref_t<Self>, Args...>::type;
};

template <typename Fn, typename Self, typename T> struct _typelist_type_select_invoke_result;
template <typename Fn, typename Self, template <typename...> typename Tpl, typename... Ts>
struct _typelist_type_select_invoke_result<Fn, Self, Tpl<Ts...>> {
  using T0 = select_nth_t<0, Ts...>;
  using R0 = ::fn::detail::_invoke_type_result<T0, Fn, apply_const_lvalue_t<Self, T0>>::type;
  static_assert((
      ...
      && std::is_same_v<R0, typename ::fn::detail::_invoke_type_result<Ts, Fn, apply_const_lvalue_t<Self, Ts>>::type>));
  using type = R0;
};

template <typename Fn, typename Self, typename T> struct _typelist_type_collapsing_sum;
template <typename Fn, typename Self, template <typename...> typename Tpl, typename... Ts>
struct _typelist_type_collapsing_sum<Fn, Self, Tpl<Ts...>> {
  using type = _collapsing_sum::normalized<
      Tpl, _collapsing_sum::flattened<std::remove_cvref_t<
               typename ::fn::detail::_invoke_type_result<Ts, Fn, apply_const_lvalue_t<Self, Ts>>::type>...>>::type;
};

template <typename T, typename Fn, typename Self> struct _sum_invoke_type_result final {
  using type = T;
};
template <typename Fn, typename Self> struct _sum_invoke_type_result<_invoke_autodetect_tag, Fn, Self> final {
  using type = _typelist_type_select_invoke_result<Fn, Self, std::remove_cvref_t<Self>>::type;
};
template <typename Fn, typename Self> struct _sum_invoke_type_result<_collapsing_sum_tag, Fn, Self> final {
  using type = _typelist_type_collapsing_sum<Fn, Self, std::remove_cvref_t<Self>>::type;
};

} // namespace detail

/**
 * @brief TODO
 *
 * @tparam Ts TODO
 */
template <typename... Ts> struct sum;

/**
 * @brief A unit of sum<> monoid, cannot be created but offers useful static functions and can be put in an union
 */
template <> struct sum<> final {
  constexpr sum() noexcept = delete; // NOTE, `= delete` here is the whole point
  constexpr ~sum() noexcept = default;
  constexpr sum(sum const &) noexcept = default;
  constexpr sum(sum &&) noexcept = default;

  static constexpr std::size_t size = 0;
  template <typename T> static constexpr bool has_type = false;
};

/**
 * @brief TODO
 *
 * @tparam Ts TODO
 */
template <typename... Ts>
  requires(sizeof...(Ts) > 0)
struct sum<Ts...> {
  static_assert((... && detail::_is_valid_sum_subtype<Ts>));
  static_assert(std::same_as<typename detail::normalized<Ts...>::template apply<::fn::sum>, sum>);

  using data_t = detail::variadic_union<Ts...>;
  data_t data;
  std::size_t index;

  static constexpr std::size_t size = sizeof...(Ts);

  /**
   * @brief TODO
   *
   * @tparam Ts I
   */
  template <std::size_t I> using select_nth = detail::select_nth_t<I, Ts...>;

  /**
   * @brief TODO
   *
   * @tparam T TODO
   */
  template <typename T> static constexpr bool has_type = data_t::template has_type<T>;

  template <typename Ret> [[nodiscard]] constexpr auto _invoke(auto &&fn) const & noexcept
  {
    return detail::invoke_type_variadic_union<Ret, data_t>(this->data, this->index, FWD(fn));
  }

  template <typename Ret> [[nodiscard]] constexpr auto _invoke(auto &&fn) && noexcept
  {
    return detail::invoke_type_variadic_union<Ret, data_t>(std::move(*this).data, std::move(*this).index, FWD(fn));
  }

  template <typename Fn> [[nodiscard]] constexpr auto _transform(Fn &&fn) const & noexcept
  {
    using type = detail::_sum_invoke_type_result<detail::_collapsing_sum_tag, decltype(fn), sum const &>::type;
    return detail::invoke_type_variadic_union<type, data_t>(this->data, index, FWD(fn));
  }

  template <typename Fn> [[nodiscard]] constexpr auto _transform(Fn &&fn) && noexcept
  {
    using type = detail::_sum_invoke_type_result<detail::_collapsing_sum_tag, decltype(fn), sum &&>::type;
    return detail::invoke_type_variadic_union<type, data_t>(std::move(*this).data, index, FWD(fn));
  }

  /**
   * @brief TODO
   *
   * @tparam T TODO
   * @param v TODO
   */
  template <typename T>
  constexpr sum(T &&v)
    requires has_type<std::remove_cvref_t<T>> && (std::is_constructible_v<std::remove_cvref_t<T>, decltype(v)>)
                 && (std::is_convertible_v<decltype(v), std::remove_cvref_t<T>>)
      : data(detail::make_variadic_union<std::remove_cvref_t<T>, data_t>(FWD(v))),
        index(detail::type_index<std::remove_cvref_t<T>, Ts...>)
  {
  }

  /**
   * @brief TODO
   *
   * @tparam T TODO
   * @param v TODO
   */
  template <typename T>
  constexpr explicit sum(T &&v)
    requires has_type<std::remove_cvref_t<T>> && (std::is_constructible_v<std::remove_cvref_t<T>, decltype(v)>)
                 && (not std::is_convertible_v<decltype(v), std::remove_cvref_t<T>>)
      : data(detail::make_variadic_union<std::remove_cvref_t<T>, data_t>(FWD(v))),
        index(detail::type_index<std::remove_cvref_t<T>, Ts...>)
  {
  }

  /**
   * @brief TODO
   *
   * @tparam T TODO
   * @param args TODO
   */
  template <typename T>
  constexpr sum(std::in_place_type_t<T>, auto &&...args)
    requires has_type<T>
      : data(detail::make_variadic_union<T, data_t>(FWD(args)...)), index(detail::type_index<T, Ts...>)
  {
  }

  /**
   * @brief TODO
   *
   * @tparam Tx TODO
   * @param arg TODO
   */
  template <typename... Tx>
  constexpr sum(sum<Tx...> const &arg) noexcept
    requires detail::is_superset_of<sum, sum<Tx...>> && (not std::is_same_v<sum, sum<Tx...>>)
                 && (... && std::is_copy_constructible_v<Tx>) && (sizeof...(Tx) > 0)
      : data(FWD(arg).template _invoke<data_t>([]<typename T>(std::in_place_type_t<T>, auto &&v) {
          return detail::make_variadic_union<T, data_t>(FWD(v));
        })),
        index(FWD(arg).template _invoke<std::size_t>([]<typename T>(std::in_place_type_t<T>, auto &&) { //
          return detail::type_index<T, Ts...>;
        }))
  {
  }

  /**
   * @brief TODO
   *
   * @tparam Tx TODO
   * @param arg TODO
   */
  template <typename... Tx>
  constexpr sum(sum<Tx...> &&arg) noexcept
    requires detail::is_superset_of<sum, sum<Tx...>> && (not std::is_same_v<sum, sum<Tx...>>)
                 && (... && std::is_move_constructible_v<Tx>) && (sizeof...(Tx) > 0)
      : data(FWD(arg).template _invoke<data_t>([]<typename T>(std::in_place_type_t<T>, auto &&v) {
          return detail::make_variadic_union<T, data_t>(FWD(v));
        })),
        index(FWD(arg).template _invoke<std::size_t>([]<typename T>(std::in_place_type_t<T>, auto &&) { //
          return detail::type_index<T, Ts...>;
        }))
  {
  }

  /**
   * @brief TODO
   *
   * @tparam Tx TODO
   * @param arg TODO
   */
  template <typename... Tx>
  constexpr sum(std::in_place_type_t<sum<Tx...>>, some_sum auto &&arg) noexcept
    requires std::is_same_v<std::remove_cvref_t<decltype(arg)>, sum<Tx...>> && detail::is_superset_of<sum, sum<Tx...>>
                 && (sizeof...(Tx) > 0)
      : data(FWD(arg).template _invoke<data_t>([]<typename T>(std::in_place_type_t<T>, auto &&v) {
          return detail::make_variadic_union<T, data_t>(FWD(v));
        })),
        index(FWD(arg).template _invoke<std::size_t>([]<typename T>(std::in_place_type_t<T>, auto &&) { //
          return detail::type_index<T, Ts...>;
        }))
  {
  }

  /**
   * @brief TODO
   *
   * @param other TODO
   */
  constexpr sum(sum const &other) noexcept
    requires(... && std::is_copy_constructible_v<Ts>)
      : data(detail::invoke_type_variadic_union<data_t, data_t>(     //
            other.data, other.index,                                 //
            []<typename T>(std::in_place_type_t<T>, auto const &v) { //
              return detail::make_variadic_union<T, data_t>(v);
            })),
        index(other.index)
  {
  }

  /**
   * @brief TODO
   *
   * @param other TODO
   */
  constexpr sum(sum &&other) noexcept
    requires(... && std::is_move_constructible_v<Ts>)
      : data(detail::invoke_type_variadic_union<data_t, data_t>( //
            std::move(other).data, other.index,                  //
            []<typename T>(std::in_place_type_t<T>, auto &&v) {  //
              return detail::make_variadic_union<T, data_t>(std::move(v));
            })),
        index(other.index)
  {
  }

  constexpr ~sum() noexcept
  {
    detail::invoke_type_variadic_union<void, data_t>( //
        this->data, index, [this]<typename T>(std::in_place_type_t<T>, auto &&) {
          std::destroy_at(detail::ptr_variadic_union<T, data_t>(this->data));
        });
  }

  /**
   * @brief TODO
   *
   * @tparam T TODO
   * @return TODO
   */
  template <typename T>
    requires has_type<T>
  [[nodiscard]] constexpr bool has_value(std::in_place_type_t<T> = std::in_place_type<T>) const noexcept
  {
    return detail::invoke_type_variadic_union<bool, data_t>( //
        this->data, index,
        []<typename U>(std::in_place_type_t<U>, auto &&) constexpr -> bool { return std::is_same_v<T, U>; });
  }

  /**
   * @brief TODO
   *
   * @tparam T TODO
   * @return TODO
   */
  template <typename T>
    requires has_type<T>
  [[nodiscard]] constexpr T *get_ptr(std::in_place_type_t<T> = std::in_place_type<T>) noexcept
  {
    return has_value(std::in_place_type<T>) ? detail::ptr_variadic_union<T, data_t>(data) : nullptr;
  }

  /**
   * @brief TODO
   *
   * @tparam T TODO
   * @return TODO
   */
  template <typename T>
    requires has_type<T>
  [[nodiscard]] constexpr T const *get_ptr(std::in_place_type_t<T> = std::in_place_type<T>) const noexcept
  {
    return has_value(std::in_place_type<T>) ? detail::ptr_variadic_union<T, data_t>(data) : nullptr;
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto invoke(Fn &&fn, Args &&...args) & noexcept
    requires typelist_invocable<Fn, sum &, Args &&...>
  {
    using type = detail::_sum_invoke_result<detail::_invoke_autodetect_tag, decltype(fn), sum &, Args &&...>::type;
    return detail::invoke_variadic_union<type, data_t>(this->data, index, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto invoke(Fn &&fn, Args &&...args) const & noexcept
    requires typelist_invocable<Fn, sum const &, Args &&...>
  {
    using type
        = detail::_sum_invoke_result<detail::_invoke_autodetect_tag, decltype(fn), sum const &, Args &&...>::type;
    return detail::invoke_variadic_union<type, data_t>(this->data, index, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto invoke(Fn &&fn, Args &&...args) && noexcept
    requires typelist_invocable<Fn, sum &&, Args &&...>
  {
    using type = detail::_sum_invoke_result<detail::_invoke_autodetect_tag, decltype(fn), sum &&, Args &&...>::type;
    return detail::invoke_variadic_union<type, data_t>(std::move(*this).data, index, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto invoke(Fn &&fn, Args &&...args) const && noexcept
    requires typelist_invocable<Fn, sum const &&, Args &&...>
  {
    using type
        = detail::_sum_invoke_result<detail::_invoke_autodetect_tag, decltype(fn), sum const &&, Args &&...>::type;
    return detail::invoke_variadic_union<type, data_t>(std::move(*this).data, index, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Ret TODO
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn, Args &&...args) & noexcept
    requires typelist_invocable_r<Ret, Fn, sum &, Args &&...>
  {
    using type = detail::_sum_invoke_result<Ret, decltype(fn), sum &, Args &&...>::type;
    return detail::invoke_variadic_union<type, data_t>(this->data, index, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Ret TODO
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn, Args &&...args) const & noexcept -> Ret
    requires typelist_invocable_r<Ret, Fn, sum const &, Args &&...>
  {
    using type = detail::_sum_invoke_result<Ret, decltype(fn), sum const &, Args &&...>::type;
    return detail::invoke_variadic_union<type, data_t>(this->data, index, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Ret TODO
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn, Args &&...args) && noexcept -> Ret
    requires typelist_invocable_r<Ret, Fn, sum &&, Args &&...>
  {
    using type = detail::_sum_invoke_result<Ret, decltype(fn), sum &&, Args &&...>::type;
    return detail::invoke_variadic_union<type, data_t>(std::move(*this).data, index, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Ret TODO
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn, Args &&...args) const && noexcept -> Ret
    requires typelist_invocable_r<Ret, Fn, sum const &&, Args &&...>
  {
    using type = detail::_sum_invoke_result<Ret, decltype(fn), sum const &&, Args &&...>::type;
    return detail::invoke_variadic_union<type, data_t>(std::move(*this).data, index, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto transform(Fn &&fn, Args &&...args) & noexcept
    requires typelist_invocable<Fn, sum &, Args &&...>
  {
    using type = detail::_sum_invoke_result<detail::_collapsing_sum_tag, decltype(fn), sum &, Args &&...>::type;
    return detail::invoke_variadic_union<type, data_t>(this->data, index, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto transform(Fn &&fn, Args &&...args) const & noexcept
    requires typelist_invocable<Fn, sum const &, Args &&...>
  {
    using type = detail::_sum_invoke_result<detail::_collapsing_sum_tag, decltype(fn), sum const &, Args &&...>::type;
    return detail::invoke_variadic_union<type, data_t>(this->data, index, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto transform(Fn &&fn, Args &&...args) && noexcept
    requires typelist_invocable<Fn, sum &&, Args &&...>
  {
    using type = detail::_sum_invoke_result<detail::_collapsing_sum_tag, decltype(fn), sum &&, Args &&...>::type;
    return detail::invoke_variadic_union<type, data_t>(std::move(*this).data, index, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto transform(Fn &&fn, Args &&...args) const && noexcept
    requires typelist_invocable<Fn, sum const &&, Args &&...>
  {
    using type = detail::_sum_invoke_result<detail::_collapsing_sum_tag, decltype(fn), sum const &&, Args &&...>::type;
    return detail::invoke_variadic_union<type, data_t>(std::move(*this).data, index, FWD(fn), FWD(args)...);
  }
};

// CTAD for single-element sum
template <typename T> explicit sum(std::in_place_type_t<T>, auto &&...) -> sum<T>;
template <typename T> explicit sum(T) -> sum<std::remove_cvref_t<T>>;

// Lifts
/**
 * @brief TODO
 *
 * @param src TODO
 * @return TODO
 */
[[nodiscard]] constexpr auto as_sum(auto &&src) -> decltype(auto)
{
  return sum<std::remove_cvref_t<decltype(src)>>(FWD(src));
}

/**
 * @brief TODO
 *
 * @param src TODO
 * @return TODO
 */
template <typename T> [[nodiscard]] constexpr auto as_sum(std::in_place_type_t<T>, auto &&...args) -> decltype(auto)
{
  return sum<T>(std::in_place_type<T>, FWD(args)...);
}

/**
 * @brief TODO
 *
 * @tparam Ts TODO
 * @tparam Tx TODO
 * @param lh TODO
 * @param rh TODO
 * @return TODO
 */
template <typename... Ts, typename... Tx>
[[nodiscard]] constexpr bool operator==(sum<Ts...> const &lh, sum<Tx...> const &rh) noexcept
  requires(... && (std::equality_comparable<Ts> || not detail::type_one_of<Ts, Tx...>)) //
          && ((sizeof...(Ts)) > 0) && ((sizeof...(Tx)) > 0)
{
  return lh.template _invoke<bool>([&rh]<typename T>(std::in_place_type_t<T> d, auto const &lh) noexcept {
    if constexpr (std::remove_cvref_t<decltype(rh)>::template has_type<T>) {
      return rh.has_value(d) && lh == *rh.get_ptr(d);
    } else {
      return false;
    }
  });
}

/**
 * @brief TODO
 *
 * @tparam Ts TODO
 * @tparam Tx TODO
 * @param lh TODO
 * @param rh TODO
 * @return TODO
 */
template <typename... Ts, typename... Tx>
[[nodiscard]] constexpr bool operator!=(sum<Ts...> const &lh, sum<Tx...> const &rh) noexcept
  requires(... && (std::equality_comparable<Ts> || not detail::type_one_of<Ts, Tx...>))
{
  return not(lh == rh);
}

/**
 * @brief TODO
 *
 * @tparam Ts TODO
 */
template <typename... Ts>
using sum_for = detail::_collapsing_sum::normalized<::fn::sum, detail::_collapsing_sum::flattened<Ts...>>::type;

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_SUM
