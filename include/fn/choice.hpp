// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_CHOICE
#define INCLUDE_FN_CHOICE

#include <fn/detail/meta.hpp>
#include <fn/sum.hpp>

#include <type_traits>
#include <utility>

namespace fn {

/**
 * @brief TODO
 *
 * @tparam T TODO
 */
template <typename T>
concept some_choice = detail::_some_choice<T>;

template <> struct choice<>; // Intentionally incomplete

namespace detail {
template <typename T>
static constexpr bool _is_valid_choice_subtype //
    = (not ::std::is_same_v<void, T>)          //
    &&(not ::std::is_reference_v<T>)           //
    &&(not some_sum<T>)                        //
    &&(not some_in_place_type<T>)              //
    &&::std::is_same_v<T, ::std::remove_cv_t<T>>;
}

/**
 * @brief TODO
 *
 * @tparam Ts TODO
 */
template <typename... Ts>
  requires(sizeof...(Ts) > 0)
struct choice<Ts...> : sum<Ts...> {
  static_assert((... && detail::_is_valid_choice_subtype<Ts>));
  static_assert(::std::same_as<typename detail::normalized<Ts...>::template apply<::fn::choice>, choice>);
  using _impl = sum<Ts...>;
  using value_type = _impl;

  static constexpr ::std::size_t size = sizeof...(Ts);
  template <::std::size_t I> using select_nth = detail::select_nth_t<I, Ts...>;
  template <typename T> static constexpr bool has_type = _impl::template has_type<T>;

  /**
   * @brief TODO
   *
   * @tparam Ret TODO
   * @param fn TODO
   */
  template <typename Ret>
  [[nodiscard]] constexpr auto
  _invoke(auto &&fn) const & noexcept(detail::_is_nothrow_rtst_invocable<Ret, decltype(fn), choice const &>)
  {
    return detail::invoke_type_variadic_union<Ret, typename _impl::data_t>(this->data, this->index, FWD(fn));
  }

  /**
   * @brief TODO
   *
   * @tparam Ret TODO
   * @param fn TODO
   */
  template <typename Ret>
  [[nodiscard]] constexpr auto
  _invoke(auto &&fn) && noexcept(detail::_is_nothrow_rtst_invocable<Ret, decltype(fn), choice &&>)
  {
    return detail::invoke_type_variadic_union<Ret, typename _impl::data_t>( //
        ::std::move(*this).data, ::std::move(*this).index, FWD(fn));
  }

  /**
   * @brief TODO
   *
   * @tparam T TODO
   * @param v TODO
   */
  template <typename T>
  constexpr choice(T &&v) // NOSONAR cpp:S1709,S6458 implicit arm of the explicit pair; has_type excludes self
      noexcept(::std::is_nothrow_constructible_v<_impl, ::std::in_place_type_t<::std::remove_cvref_t<T>>, decltype(v)>)
    requires has_type<::std::remove_cvref_t<T>>
             && (::std::is_constructible_v<_impl, ::std::in_place_type_t<::std::remove_cvref_t<T>>, decltype(v)>)
             && (::std::is_convertible_v<decltype(v), ::std::remove_cvref_t<T>>)
      : _impl(::std::in_place_type<::std::remove_cvref_t<T>>, FWD(v))
  {
  }

  /**
   * @brief TODO
   *
   * @tparam T TODO
   * @param v TODO
   */
  template <typename T>
  constexpr explicit choice(T &&v) // NOSONAR cpp:S6458 has_type excludes self
      noexcept(::std::is_nothrow_constructible_v<_impl, ::std::in_place_type_t<::std::remove_cvref_t<T>>, decltype(v)>)
    requires has_type<::std::remove_cvref_t<T>>
             && (::std::is_constructible_v<_impl, ::std::in_place_type_t<::std::remove_cvref_t<T>>, decltype(v)>)
             && (not ::std::is_convertible_v<decltype(v), ::std::remove_cvref_t<T>>)
      : _impl(::std::in_place_type<::std::remove_cvref_t<T>>, FWD(v))
  {
  }

  /**
   * @brief TODO
   *
   * @tparam T TODO
   * @param d TODO
   * @param v TODO
   */
  template <typename T>
  constexpr explicit choice(::std::in_place_type_t<T> d, auto &&...args) //
      noexcept(::std::is_nothrow_constructible_v<_impl, ::std::in_place_type_t<T>, decltype(args)...>)
    requires has_type<T> && ::std::is_constructible_v<_impl, ::std::in_place_type_t<T>, decltype(args)...>
      : _impl(d, FWD(args)...)
  {
  }

  /**
   * @brief TODO
   *
   * @tparam Tx TODO
   * @param v TODO
   */
  template <typename... Tx>
  constexpr choice(sum<Tx...> const &v) // NOSONAR cpp:S1709 implicit widening by design
      noexcept(::std::is_nothrow_constructible_v<_impl, ::std::in_place_type_t<sum<Tx...>>, sum<Tx...> const &>)
    requires detail::is_superset_of<choice, choice<Tx...>>
             && (::std::is_constructible_v<_impl, ::std::in_place_type_t<sum<Tx...>>, sum<Tx...> const &>)
      : _impl(::std::in_place_type<sum<Tx...>>, FWD(v))
  {
  }

  /**
   * @brief TODO
   *
   * @tparam Tx TODO
   * @param v TODO
   */
  template <typename... Tx>
  constexpr choice(sum<Tx...> &&v) // NOSONAR cpp:S1709 implicit widening by design
      noexcept(::std::is_nothrow_constructible_v<_impl, ::std::in_place_type_t<sum<Tx...>>, sum<Tx...>>)
    requires detail::is_superset_of<choice, choice<Tx...>>
             && (::std::is_constructible_v<_impl, ::std::in_place_type_t<sum<Tx...>>, sum<Tx...>>)
      : _impl(::std::in_place_type<sum<Tx...>>, FWD(v))
  {
  }

  /**
   * @brief TODO
   *
   * @tparam Tx TODO
   * @param v TODO
   */
  template <typename... Tx>
  constexpr choice(::std::in_place_type_t<sum<Tx...>>, some_sum auto &&v) //
      noexcept(::std::is_nothrow_constructible_v<_impl, ::std::in_place_type_t<sum<Tx...>>, decltype(v)>)
    requires ::std::is_same_v<::std::remove_cvref_t<decltype(v)>, sum<Tx...>>
             && detail::is_superset_of<choice, choice<Tx...>>
      : _impl(::std::in_place_type<sum<Tx...>>, FWD(v))
  {
  }

  constexpr choice(choice const &other) = default;
  constexpr choice(choice &&other) = default;
  constexpr ~choice() = default;

  // Declared because the move constructor above would otherwise delete the implicit copy assignment
  // and suppress the implicit move assignment. Defaulted, so both inherit the base sum's - its
  // constraints, its strong guarantee, and its computed noexcept (which an explicit one here would
  // contradict, and thereby delete).
  constexpr choice &operator=(choice const &other) = default;
  constexpr choice &operator=(choice &&other) = default;

  /**
   * @brief TODO
   *
   * @return TODO
   */
  [[nodiscard]] constexpr value_type &value() & noexcept { return *this; }

  /**
   * @brief TODO
   *
   * @return TODO
   */
  [[nodiscard]] constexpr value_type const &value() const & noexcept { return *this; }

  /**
   * @brief TODO
   *
   * @return TODO
   */
  [[nodiscard]] constexpr value_type &&value() && noexcept { return ::std::move(*this); }

  /**
   * @brief TODO
   *
   * @return TODO
   */
  [[nodiscard]] constexpr value_type const &&value() const && noexcept { return ::std::move(*this); }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @param fn TODO
   * @return TODO
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) & noexcept(
      detail::_is_nothrow_rts_invocable<
          typename detail::_sum_invoke_result<detail::_invoke_autodetect_tag, decltype(fn), choice &>::type, Fn &&,
          choice &>)
    requires typelist_invocable<Fn, choice &>
  {
    using type = detail::_sum_invoke_result<detail::_invoke_autodetect_tag, decltype(fn), choice &>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @param fn TODO
   * @return TODO
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) const & noexcept(
      detail::_is_nothrow_rts_invocable<
          typename detail::_sum_invoke_result<detail::_invoke_autodetect_tag, decltype(fn), choice const &>::type,
          Fn &&, choice const &>)
    requires typelist_invocable<Fn, choice const &>
  {
    using type = detail::_sum_invoke_result<detail::_invoke_autodetect_tag, decltype(fn), choice const &>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @param fn TODO
   * @return TODO
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) && noexcept(
      detail::_is_nothrow_rts_invocable<
          typename detail::_sum_invoke_result<detail::_invoke_autodetect_tag, decltype(fn), choice &&>::type, Fn &&,
          choice &&>)
    requires typelist_invocable<Fn, choice &&>
  {
    using type = detail::_sum_invoke_result<detail::_invoke_autodetect_tag, decltype(fn), choice &&>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(::std::move(_impl::data), _impl::index, FWD(fn));
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @param fn TODO
   * @return TODO
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) const && noexcept(
      detail::_is_nothrow_rts_invocable<
          typename detail::_sum_invoke_result<detail::_invoke_autodetect_tag, decltype(fn), choice const &&>::type,
          Fn &&, choice const &&>)
    requires typelist_invocable<Fn, choice const &&>
  {
    using type = detail::_sum_invoke_result<detail::_invoke_autodetect_tag, decltype(fn), choice const &&>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(::std::move(_impl::data), _impl::index, FWD(fn));
  }

  /**
   * @brief TODO
   *
   * @tparam T TODO
   * @tparam Fn TODO
   * @param fn TODO
   * @return TODO
   */
  template <typename T, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn) & noexcept(
      detail::_is_nothrow_rts_invocable<typename detail::_sum_invoke_result<T, decltype(fn), choice &>::type, Fn &&,
                                        choice &>)
    requires typelist_invocable<Fn, choice &>
  {
    using type = detail::_sum_invoke_result<T, decltype(fn), choice &>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
  }

  /**
   * @brief TODO
   *
   * @tparam T TODO
   * @tparam Fn TODO
   * @param fn TODO
   * @return TODO
   */
  template <typename T, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn) const & noexcept(
      detail::_is_nothrow_rts_invocable<typename detail::_sum_invoke_result<T, decltype(fn), choice const &>::type,
                                        Fn &&, choice const &>)
    requires typelist_invocable<Fn, choice const &>
  {
    using type = detail::_sum_invoke_result<T, decltype(fn), choice const &>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
  }

  /**
   * @brief TODO
   *
   * @tparam T TODO
   * @tparam Fn TODO
   * @param fn TODO
   * @return TODO
   */
  template <typename T, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn) && noexcept(
      detail::_is_nothrow_rts_invocable<typename detail::_sum_invoke_result<T, decltype(fn), choice &&>::type, Fn &&,
                                        choice &&>)
    requires typelist_invocable<Fn, choice &&>
  {
    using type = detail::_sum_invoke_result<T, decltype(fn), choice &&>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(::std::move(_impl::data), _impl::index, FWD(fn));
  }

  /**
   * @brief TODO
   *
   * @tparam T TODO
   * @tparam Fn TODO
   * @param fn TODO
   * @return TODO
   */
  template <typename T, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn) const && noexcept(
      detail::_is_nothrow_rts_invocable<typename detail::_sum_invoke_result<T, decltype(fn), choice const &&>::type,
                                        Fn &&, choice const &&>)
    requires typelist_invocable<Fn, choice const &&>
  {
    using type = detail::_sum_invoke_result<T, decltype(fn), choice const &&>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(::std::move(_impl::data), _impl::index, FWD(fn));
  }

  // NOTE Monadic operations, only `and_then` and `transform` are supported
  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @param fn TODO
   * @return TODO
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) & noexcept(
      detail::_is_nothrow_rts_invocable<
          typename detail::_sum_invoke_result<detail::_collapsing_sum_tag, decltype(fn), choice &>::type, Fn &&,
          choice &>)
    requires typelist_invocable<Fn, choice &>
  {
    using type = detail::_sum_invoke_result<detail::_collapsing_sum_tag, decltype(fn), choice &>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @param fn TODO
   * @return TODO
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) const & noexcept(
      detail::_is_nothrow_rts_invocable<
          typename detail::_sum_invoke_result<detail::_collapsing_sum_tag, decltype(fn), choice const &>::type, Fn &&,
          choice const &>)
    requires typelist_invocable<Fn, choice const &>
  {
    using type = detail::_sum_invoke_result<detail::_collapsing_sum_tag, decltype(fn), choice const &>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @param fn TODO
   * @return TODO
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) && noexcept(
      detail::_is_nothrow_rts_invocable<
          typename detail::_sum_invoke_result<detail::_collapsing_sum_tag, decltype(fn), choice &&>::type, Fn &&,
          choice &&>)
    requires typelist_invocable<Fn, choice &&>
  {
    using type = detail::_sum_invoke_result<detail::_collapsing_sum_tag, decltype(fn), choice &&>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(::std::move(_impl::data), _impl::index, FWD(fn));
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @param fn TODO
   * @return TODO
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) const && noexcept(
      detail::_is_nothrow_rts_invocable<
          typename detail::_sum_invoke_result<detail::_collapsing_sum_tag, decltype(fn), choice const &&>::type, Fn &&,
          choice const &&>)
    requires typelist_invocable<Fn, choice const &&>
  {
    using type = detail::_sum_invoke_result<detail::_collapsing_sum_tag, decltype(fn), choice const &&>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(::std::move(_impl::data), _impl::index, FWD(fn));
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @param fn TODO
   * @return TODO
   */
  template <typename Fn>
  constexpr auto and_then(Fn &&fn) & noexcept(noexcept(this->invoke(FWD(fn)))) -> decltype(this->invoke(FWD(fn)))
  {
    static_assert(some_choice<decltype(this->invoke(FWD(fn)))>);
    return this->invoke(FWD(fn));
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @param fn TODO
   * @return TODO
   */
  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const & noexcept(noexcept(this->invoke(FWD(fn)))) -> decltype(this->invoke(FWD(fn)))
  {
    static_assert(some_choice<decltype(this->invoke(FWD(fn)))>);
    return this->invoke(FWD(fn));
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @param fn TODO
   * @return TODO
   */
  template <typename Fn>
  constexpr auto and_then(Fn &&fn) && noexcept(noexcept(::std::move(*this).invoke(FWD(fn))))
      -> decltype(::std::move(*this).invoke(FWD(fn)))
  {
    static_assert(some_choice<decltype(::std::move(*this).invoke(FWD(fn)))>);
    return ::std::move(*this).invoke(FWD(fn));
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @param fn TODO
   * @return TODO
   */
  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const && noexcept(noexcept(::std::move(*this).invoke(FWD(fn))))
      -> decltype(::std::move(*this).invoke(FWD(fn)))
  {
    static_assert(some_choice<decltype(::std::move(*this).invoke(FWD(fn)))>);
    return ::std::move(*this).invoke(FWD(fn));
  }
};

// CTAD for single-element choice
template <typename T> explicit choice(::std::in_place_type_t<T>, auto &&...) -> choice<T>;
template <typename T> explicit choice(T) -> choice<T>;

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
[[nodiscard]] constexpr bool operator==(choice<Ts...> const &lh, choice<Tx...> const &rh) noexcept
  requires(... && (::std::equality_comparable<Ts> || not detail::type_one_of<Ts, Tx...>))
          and (not ::std::is_same_v<choice<Ts...>, choice<Tx...>>)
{
  return lh.template _invoke<bool>([&rh]<typename T>(::std::in_place_type_t<T> d, auto const &lh) noexcept {
    if constexpr (::std::remove_cvref_t<decltype(rh)>::template has_type<T>) {
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
[[nodiscard]] constexpr bool operator!=(choice<Ts...> const &lh, choice<Tx...> const &rh) noexcept
  requires(... && (::std::equality_comparable<Ts> || not detail::type_one_of<Ts, Tx...>))
{
  return not(lh == rh);
}

/**
 * @brief TODO
 *
 * @tparam Ts TODO
 */
template <typename... Ts>
using choice_for = detail::_collapsing_sum::normalized<::fn::choice, detail::_collapsing_sum::flattened<Ts...>>::type;

} // namespace fn

#endif // INCLUDE_FN_CHOICE
