// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_CHOICE
#define INCLUDE_FN_CHOICE

#include <fn/copack.hpp>
#include <fn/detail/meta.hpp>
#include <fn/fwd.hpp>
#include <fn/just.hpp>
#include <fn/pack.hpp>
#include <libfn_version.hpp>

#include <type_traits>
#include <utility>

#include <fn/detail/macro_begin.hpp>

namespace fn {
inline namespace LIBFN_VERSION {

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
    &&(not some_copack<T>)                     //
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
struct choice<Ts...> : copack<Ts...> {
  static_assert((... && detail::_is_valid_choice_subtype<Ts>));
  static_assert(::std::same_as<typename detail::normalized<Ts...>::template apply<::fn::choice>, choice>);
  using _impl = copack<Ts...>;
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
  constexpr choice(copack<Tx...> const &v) // NOSONAR cpp:S1709 implicit widening by design
      noexcept(::std::is_nothrow_constructible_v<_impl, ::std::in_place_type_t<copack<Tx...>>, copack<Tx...> const &>)
    requires detail::is_superset_of<choice, choice<Tx...>>
             && (::std::is_constructible_v<_impl, ::std::in_place_type_t<copack<Tx...>>, copack<Tx...> const &>)
      : _impl(::std::in_place_type<copack<Tx...>>, FWD(v))
  {
  }

  /**
   * @brief TODO
   *
   * @tparam Tx TODO
   * @param v TODO
   */
  template <typename... Tx>
  constexpr choice(copack<Tx...> &&v) // NOSONAR cpp:S1709 implicit widening by design
      noexcept(::std::is_nothrow_constructible_v<_impl, ::std::in_place_type_t<copack<Tx...>>, copack<Tx...>>)
    requires detail::is_superset_of<choice, choice<Tx...>>
             && (::std::is_constructible_v<_impl, ::std::in_place_type_t<copack<Tx...>>, copack<Tx...>>)
      : _impl(::std::in_place_type<copack<Tx...>>, FWD(v))
  {
  }

  /**
   * @brief TODO
   *
   * @tparam Tx TODO
   * @param v TODO
   */
  template <typename... Tx>
  constexpr choice(::std::in_place_type_t<copack<Tx...>>, some_copack auto &&v) //
      noexcept(::std::is_nothrow_constructible_v<_impl, ::std::in_place_type_t<copack<Tx...>>, decltype(v)>)
    requires ::std::is_same_v<::std::remove_cvref_t<decltype(v)>, copack<Tx...>>
             && detail::is_superset_of<choice, choice<Tx...>>
      : _impl(::std::in_place_type<copack<Tx...>>, FWD(v))
  {
  }

  constexpr choice(choice const &other) = default;
  constexpr choice(choice &&other) = default;
  constexpr ~choice() = default;

  // Declared because the move constructor above would otherwise delete the implicit copy assignment
  // and suppress the implicit move assignment. Defaulted, so both inherit the base copack's - its
  // constraints, its strong guarantee, and its computed noexcept (which an explicit one here would
  // contradict, and thereby delete).
  constexpr choice &operator=(choice const &other) = default;
  constexpr choice &operator=(choice &&other) = default;

  // choice declares its copy and move assignment, and a declared operator= hides every base
  // overload - copack's widening assignment must be restated here to exist at all. Delegating keeps
  // the answer copack's, and admits a copack over the same alternatives, which would otherwise pay for
  // the temporary the widening constructor builds.
  template <typename... Tx>
  constexpr choice &operator=(copack<Tx...> const &arg) //
      noexcept(::std::is_nothrow_assignable_v<copack<Ts...> &, copack<Tx...> const &>)
    requires ::std::is_assignable_v<copack<Ts...> &, copack<Tx...> const &>
  {
    static_cast<copack<Ts...> &>(*this) = arg;
    return *this;
  }
  template <typename... Tx>
  constexpr choice &operator=(copack<Tx...> &&arg) //
      noexcept(::std::is_nothrow_assignable_v<copack<Ts...> &, copack<Tx...>>)
    requires ::std::is_assignable_v<copack<Ts...> &, copack<Tx...>>
  {
    static_cast<copack<Ts...> &>(*this) = ::std::move(arg);
    return *this;
  }

  // The delegating value assignment restates copack's for the same name-hiding reason. Copack- and
  // choice-typed sources are excluded to leave them to the assignments above: for a non-const
  // lvalue source a forwarding reference would otherwise outrank their `const &` bindings.
  template <typename U>
  constexpr choice &operator=(U &&v) //
      noexcept(::std::is_nothrow_assignable_v<copack<Ts...> &, decltype(v)>)
    requires(not some_copack<U>) && (not some_choice<U>) && ::std::is_assignable_v<copack<Ts...> &, decltype(v)>
  {
    static_cast<copack<Ts...> &>(*this) = FWD(v);
    return *this;
  }

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
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply(Fn &&fn, Args &&...args) & noexcept(
      detail::_is_nothrow_rts_applicable<typename detail::_copack_apply_result<
                                             detail::_apply_autodetect_tag, decltype(fn), choice &, Args &&...>::type,
                                         Fn &&, choice &, Args &&...>)
    requires typelist_applicable<Fn, choice &, Args &&...>
  {
    using type = detail::_copack_apply_result<detail::_apply_autodetect_tag, decltype(fn), choice &, Args &&...>::type;
    return detail::apply_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn), FWD(args)...);
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
  [[nodiscard]] constexpr auto apply(Fn &&fn, Args &&...args) const & noexcept(
      detail::_is_nothrow_rts_applicable<
          typename detail::_copack_apply_result<detail::_apply_autodetect_tag, decltype(fn), choice const &,
                                                Args &&...>::type,
          Fn &&, choice const &, Args &&...>)
    requires typelist_applicable<Fn, choice const &, Args &&...>
  {
    using type
        = detail::_copack_apply_result<detail::_apply_autodetect_tag, decltype(fn), choice const &, Args &&...>::type;
    return detail::apply_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn), FWD(args)...);
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
  [[nodiscard]] constexpr auto apply(Fn &&fn, Args &&...args) && noexcept(
      detail::_is_nothrow_rts_applicable<typename detail::_copack_apply_result<
                                             detail::_apply_autodetect_tag, decltype(fn), choice &&, Args &&...>::type,
                                         Fn &&, choice &&, Args &&...>)
    requires typelist_applicable<Fn, choice &&, Args &&...>
  {
    using type = detail::_copack_apply_result<detail::_apply_autodetect_tag, decltype(fn), choice &&, Args &&...>::type;
    return detail::apply_variadic_union<type, typename _impl::data_t>(::std::move(_impl::data), _impl::index, FWD(fn),
                                                                      FWD(args)...);
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
  [[nodiscard]] constexpr auto apply(Fn &&fn, Args &&...args) const && noexcept(
      detail::_is_nothrow_rts_applicable<
          typename detail::_copack_apply_result<detail::_apply_autodetect_tag, decltype(fn), choice const &&,
                                                Args &&...>::type,
          Fn &&, choice const &&, Args &&...>)
    requires typelist_applicable<Fn, choice const &&, Args &&...>
  {
    using type
        = detail::_copack_apply_result<detail::_apply_autodetect_tag, decltype(fn), choice const &&, Args &&...>::type;
    return detail::apply_variadic_union<type, typename _impl::data_t>(::std::move(_impl::data), _impl::index, FWD(fn),
                                                                      FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam T TODO
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename T, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_r(Fn &&fn, Args &&...args) & noexcept(
      detail::_is_nothrow_rts_applicable<
          typename detail::_copack_apply_result<T, decltype(fn), choice &, Args &&...>::type, Fn &&, choice &,
          Args &&...>)
    requires typelist_applicable<Fn, choice &, Args &&...>
  {
    using type = detail::_copack_apply_result<T, decltype(fn), choice &, Args &&...>::type;
    return detail::apply_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam T TODO
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename T, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_r(Fn &&fn, Args &&...args) const & noexcept(
      detail::_is_nothrow_rts_applicable<
          typename detail::_copack_apply_result<T, decltype(fn), choice const &, Args &&...>::type, Fn &&,
          choice const &, Args &&...>)
    requires typelist_applicable<Fn, choice const &, Args &&...>
  {
    using type = detail::_copack_apply_result<T, decltype(fn), choice const &, Args &&...>::type;
    return detail::apply_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam T TODO
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename T, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_r(Fn &&fn, Args &&...args) && noexcept(
      detail::_is_nothrow_rts_applicable<
          typename detail::_copack_apply_result<T, decltype(fn), choice &&, Args &&...>::type, Fn &&, choice &&,
          Args &&...>)
    requires typelist_applicable<Fn, choice &&, Args &&...>
  {
    using type = detail::_copack_apply_result<T, decltype(fn), choice &&, Args &&...>::type;
    return detail::apply_variadic_union<type, typename _impl::data_t>(::std::move(_impl::data), _impl::index, FWD(fn),
                                                                      FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam T TODO
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename T, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_r(Fn &&fn, Args &&...args) const && noexcept(
      detail::_is_nothrow_rts_applicable<
          typename detail::_copack_apply_result<T, decltype(fn), choice const &&, Args &&...>::type, Fn &&,
          choice const &&, Args &&...>)
    requires typelist_applicable<Fn, choice const &&, Args &&...>
  {
    using type = detail::_copack_apply_result<T, decltype(fn), choice const &&, Args &&...>::type;
    return detail::apply_variadic_union<type, typename _impl::data_t>(::std::move(_impl::data), _impl::index, FWD(fn),
                                                                      FWD(args)...);
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
  [[nodiscard]] constexpr auto apply_type(Fn &&fn, Args &&...args) & noexcept(
      detail::_is_nothrow_rtst_invocable<
          typename detail::_copack_invoke_type_result<detail::_apply_autodetect_tag, detail::_apply_type_fn<Fn>,
                                                      choice &, Args &&...>::type,
          detail::_apply_type_fn<Fn>, choice &, Args &&...>) ->
      typename detail::_copack_invoke_type_result<detail::_apply_autodetect_tag, detail::_apply_type_fn<Fn>, choice &,
                                                  Args &&...>::type
    requires detail::_typelist_type_invocable<detail::_apply_type_fn<Fn>, choice &, Args &&...>
  {
    using type = detail::_copack_invoke_type_result<detail::_apply_autodetect_tag, detail::_apply_type_fn<Fn>, choice &,
                                                    Args &&...>::type;
    return detail::invoke_type_variadic_union<type, typename _impl::data_t>(
        _impl::data, _impl::index, detail::_apply_type_fn<Fn>{FWD(fn)}, FWD(args)...);
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
  [[nodiscard]] constexpr auto apply_type(Fn &&fn, Args &&...args) const & noexcept(
      detail::_is_nothrow_rtst_invocable<
          typename detail::_copack_invoke_type_result<detail::_apply_autodetect_tag, detail::_apply_type_fn<Fn>,
                                                      choice const &, Args &&...>::type,
          detail::_apply_type_fn<Fn>, choice const &, Args &&...>) ->
      typename detail::_copack_invoke_type_result<detail::_apply_autodetect_tag, detail::_apply_type_fn<Fn>,
                                                  choice const &, Args &&...>::type
    requires detail::_typelist_type_invocable<detail::_apply_type_fn<Fn>, choice const &, Args &&...>
  {
    using type = detail::_copack_invoke_type_result<detail::_apply_autodetect_tag, detail::_apply_type_fn<Fn>,
                                                    choice const &, Args &&...>::type;
    return detail::invoke_type_variadic_union<type, typename _impl::data_t>(
        _impl::data, _impl::index, detail::_apply_type_fn<Fn>{FWD(fn)}, FWD(args)...);
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
  [[nodiscard]] constexpr auto apply_type(Fn &&fn, Args &&...args) && noexcept(
      detail::_is_nothrow_rtst_invocable<
          typename detail::_copack_invoke_type_result<detail::_apply_autodetect_tag, detail::_apply_type_fn<Fn>,
                                                      choice &&, Args &&...>::type,
          detail::_apply_type_fn<Fn>, choice &&, Args &&...>) ->
      typename detail::_copack_invoke_type_result<detail::_apply_autodetect_tag, detail::_apply_type_fn<Fn>, choice &&,
                                                  Args &&...>::type
    requires detail::_typelist_type_invocable<detail::_apply_type_fn<Fn>, choice &&, Args &&...>
  {
    using type = detail::_copack_invoke_type_result<detail::_apply_autodetect_tag, detail::_apply_type_fn<Fn>,
                                                    choice &&, Args &&...>::type;
    return detail::invoke_type_variadic_union<type, typename _impl::data_t>(
        ::std::move(_impl::data), _impl::index, detail::_apply_type_fn<Fn>{FWD(fn)}, FWD(args)...);
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
  [[nodiscard]] constexpr auto apply_type(Fn &&fn, Args &&...args) const && noexcept(
      detail::_is_nothrow_rtst_invocable<
          typename detail::_copack_invoke_type_result<detail::_apply_autodetect_tag, detail::_apply_type_fn<Fn>,
                                                      choice const &&, Args &&...>::type,
          detail::_apply_type_fn<Fn>, choice const &&, Args &&...>) ->
      typename detail::_copack_invoke_type_result<detail::_apply_autodetect_tag, detail::_apply_type_fn<Fn>,
                                                  choice const &&, Args &&...>::type
    requires detail::_typelist_type_invocable<detail::_apply_type_fn<Fn>, choice const &&, Args &&...>
  {
    using type = detail::_copack_invoke_type_result<detail::_apply_autodetect_tag, detail::_apply_type_fn<Fn>,
                                                    choice const &&, Args &&...>::type;
    return detail::invoke_type_variadic_union<type, typename _impl::data_t>(
        ::std::move(_impl::data), _impl::index, detail::_apply_type_fn<Fn>{FWD(fn)}, FWD(args)...);
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
  [[nodiscard]] constexpr auto apply_type_r(Fn &&fn, Args &&...args) & noexcept(
      detail::_is_nothrow_rtst_invocable<Ret, detail::_apply_type_fn<Fn>, choice &, Args &&...>) -> Ret
    requires detail::_typelist_type_invocable_r<Ret, detail::_apply_type_fn<Fn>, choice &, Args &&...>
  {
    using type = detail::_copack_invoke_type_result<Ret, detail::_apply_type_fn<Fn>, choice &, Args &&...>::type;
    return detail::invoke_type_variadic_union<type, typename _impl::data_t>(
        _impl::data, _impl::index, detail::_apply_type_fn<Fn>{FWD(fn)}, FWD(args)...);
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
  [[nodiscard]] constexpr auto apply_type_r(Fn &&fn, Args &&...args) const & noexcept(
      detail::_is_nothrow_rtst_invocable<Ret, detail::_apply_type_fn<Fn>, choice const &, Args &&...>) -> Ret
    requires detail::_typelist_type_invocable_r<Ret, detail::_apply_type_fn<Fn>, choice const &, Args &&...>
  {
    using type = detail::_copack_invoke_type_result<Ret, detail::_apply_type_fn<Fn>, choice const &, Args &&...>::type;
    return detail::invoke_type_variadic_union<type, typename _impl::data_t>(
        _impl::data, _impl::index, detail::_apply_type_fn<Fn>{FWD(fn)}, FWD(args)...);
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
  [[nodiscard]] constexpr auto apply_type_r(Fn &&fn, Args &&...args) && noexcept(
      detail::_is_nothrow_rtst_invocable<Ret, detail::_apply_type_fn<Fn>, choice &&, Args &&...>) -> Ret
    requires detail::_typelist_type_invocable_r<Ret, detail::_apply_type_fn<Fn>, choice &&, Args &&...>
  {
    using type = detail::_copack_invoke_type_result<Ret, detail::_apply_type_fn<Fn>, choice &&, Args &&...>::type;
    return detail::invoke_type_variadic_union<type, typename _impl::data_t>(
        ::std::move(_impl::data), _impl::index, detail::_apply_type_fn<Fn>{FWD(fn)}, FWD(args)...);
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
  [[nodiscard]] constexpr auto apply_type_r(Fn &&fn, Args &&...args) const && noexcept(
      detail::_is_nothrow_rtst_invocable<Ret, detail::_apply_type_fn<Fn>, choice const &&, Args &&...>) -> Ret
    requires detail::_typelist_type_invocable_r<Ret, detail::_apply_type_fn<Fn>, choice const &&, Args &&...>
  {
    using type = detail::_copack_invoke_type_result<Ret, detail::_apply_type_fn<Fn>, choice const &&, Args &&...>::type;
    return detail::invoke_type_variadic_union<type, typename _impl::data_t>(
        ::std::move(_impl::data), _impl::index, detail::_apply_type_fn<Fn>{FWD(fn)}, FWD(args)...);
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
      detail::_is_nothrow_rts_applicable<
          typename detail::_copack_apply_result<detail::_collapsing_copack_tag, decltype(fn), choice &>::type, Fn &&,
          choice &>) -> typename detail::_copack_apply_result<detail::_collapsing_copack_tag, Fn &&, choice &>::type
    requires typelist_applicable<Fn, choice &>
  {
    using type = detail::_copack_apply_result<detail::_collapsing_copack_tag, decltype(fn), choice &>::type;
    return detail::apply_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
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
      detail::_is_nothrow_rts_applicable<
          typename detail::_copack_apply_result<detail::_collapsing_copack_tag, decltype(fn), choice const &>::type,
          Fn &&, choice const &>) ->
      typename detail::_copack_apply_result<detail::_collapsing_copack_tag, Fn &&, choice const &>::type
    requires typelist_applicable<Fn, choice const &>
  {
    using type = detail::_copack_apply_result<detail::_collapsing_copack_tag, decltype(fn), choice const &>::type;
    return detail::apply_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
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
      detail::_is_nothrow_rts_applicable<
          typename detail::_copack_apply_result<detail::_collapsing_copack_tag, decltype(fn), choice &&>::type, Fn &&,
          choice &&>) -> typename detail::_copack_apply_result<detail::_collapsing_copack_tag, Fn &&, choice &&>::type
    requires typelist_applicable<Fn, choice &&>
  {
    using type = detail::_copack_apply_result<detail::_collapsing_copack_tag, decltype(fn), choice &&>::type;
    return detail::apply_variadic_union<type, typename _impl::data_t>(::std::move(_impl::data), _impl::index, FWD(fn));
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
      detail::_is_nothrow_rts_applicable<
          typename detail::_copack_apply_result<detail::_collapsing_copack_tag, decltype(fn), choice const &&>::type,
          Fn &&, choice const &&>) ->
      typename detail::_copack_apply_result<detail::_collapsing_copack_tag, Fn &&, choice const &&>::type
    requires typelist_applicable<Fn, choice const &&>
  {
    using type = detail::_copack_apply_result<detail::_collapsing_copack_tag, decltype(fn), choice const &&>::type;
    return detail::apply_variadic_union<type, typename _impl::data_t>(::std::move(_impl::data), _impl::index, FWD(fn));
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @param fn TODO
   * @return TODO
   */
  template <typename Fn>
  constexpr auto and_then(Fn &&fn) & noexcept(
      detail::_is_nothrow_rts_applicable<
          typename detail::_copack_apply_result<detail::_joining_superset_tag<choice>, decltype(fn), choice &>::type,
          Fn &&, choice &>) ->
      typename detail::_copack_apply_result<detail::_joining_superset_tag<choice>, Fn &&, choice &>::type
    requires typelist_applicable<Fn, choice &>
  {
    using type = detail::_copack_apply_result<detail::_joining_superset_tag<choice>, decltype(fn), choice &>::type;
    static_assert(some_choice<type>);
    return detail::apply_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @param fn TODO
   * @return TODO
   */
  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const & noexcept(
      detail::_is_nothrow_rts_applicable<typename detail::_copack_apply_result<detail::_joining_superset_tag<choice>,
                                                                               decltype(fn), choice const &>::type,
                                         Fn &&, choice const &>) ->
      typename detail::_copack_apply_result<detail::_joining_superset_tag<choice>, Fn &&, choice const &>::type
    requires typelist_applicable<Fn, choice const &>
  {
    using type
        = detail::_copack_apply_result<detail::_joining_superset_tag<choice>, decltype(fn), choice const &>::type;
    static_assert(some_choice<type>);
    return detail::apply_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @param fn TODO
   * @return TODO
   */
  template <typename Fn>
  constexpr auto and_then(Fn &&fn) && noexcept(
      detail::_is_nothrow_rts_applicable<
          typename detail::_copack_apply_result<detail::_joining_superset_tag<choice>, decltype(fn), choice &&>::type,
          Fn &&, choice &&>) ->
      typename detail::_copack_apply_result<detail::_joining_superset_tag<choice>, Fn &&, choice &&>::type
    requires typelist_applicable<Fn, choice &&>
  {
    using type = detail::_copack_apply_result<detail::_joining_superset_tag<choice>, decltype(fn), choice &&>::type;
    static_assert(some_choice<type>);
    return detail::apply_variadic_union<type, typename _impl::data_t>(::std::move(_impl::data), _impl::index, FWD(fn));
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @param fn TODO
   * @return TODO
   */
  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const && noexcept(
      detail::_is_nothrow_rts_applicable<typename detail::_copack_apply_result<detail::_joining_superset_tag<choice>,
                                                                               decltype(fn), choice const &&>::type,
                                         Fn &&, choice const &&>) ->
      typename detail::_copack_apply_result<detail::_joining_superset_tag<choice>, Fn &&, choice const &&>::type
    requires typelist_applicable<Fn, choice const &&>
  {
    using type
        = detail::_copack_apply_result<detail::_joining_superset_tag<choice>, decltype(fn), choice const &&>::type;
    static_assert(some_choice<type>);
    return detail::apply_variadic_union<type, typename _impl::data_t>(::std::move(_impl::data), _impl::index, FWD(fn));
  }
};

// CTAD for single-element choice
namespace detail {
template <typename T> struct _rechoice;
template <typename... Ts> struct _rechoice<copack<Ts...>> {
  using type = choice<Ts...>;
};
template <typename Lh, typename Rh>
using _choice_fold_t = decltype(_fold_detail::fold<typename ::std::remove_cvref_t<Lh>::value_type,
                                                   typename ::std::remove_cvref_t<Rh>::value_type>(
    ::std::declval<Lh>().value(), ::std::declval<Rh>().value()));
template <typename Lh, typename Rh>
constexpr inline bool _nothrow_choice_fold
    = noexcept(_fold_detail::fold<typename ::std::remove_cvref_t<Lh>::value_type,
                                  typename ::std::remove_cvref_t<Rh>::value_type>(::std::declval<Lh>().value(),
                                                                                  ::std::declval<Rh>().value()))
      && ::std::is_nothrow_constructible_v<typename _rechoice<_choice_fold_t<Lh, Rh>>::type, _choice_fold_t<Lh, Rh>>;
} // namespace detail

// The conjunction inside the cluster, choice on either side: the copack distributes through the
// value product, so the fold answers a copack of packs and the result stays a choice. just<void>
// is the product's unit and elides.
template <typename Lh, typename Rh>
  requires(detail::_some_choice<Lh> || detail::_some_choice<Rh>) && (detail::_some_just<Lh> || detail::_some_choice<Lh>)
          && (detail::_some_just<Rh> || detail::_some_choice<Rh>)
          && (not ::std::is_void_v<typename ::std::remove_cvref_t<Lh>::value_type>)
          && (not ::std::is_void_v<typename ::std::remove_cvref_t<Rh>::value_type>)
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) //
    noexcept(detail::_nothrow_choice_fold<Lh, Rh>)
{
  using VL = ::std::remove_cvref_t<Lh>::value_type;
  using VR = ::std::remove_cvref_t<Rh>::value_type;
  using type = typename detail::_rechoice<detail::_choice_fold_t<Lh, Rh>>::type;
  return type{::fn::detail::_fold_detail::fold<VL, VR>(FWD(lh).value(), FWD(rh).value())};
}

template <typename Lh, typename Rh>
  requires detail::_some_just<Lh> && ::std::is_void_v<typename ::std::remove_cvref_t<Lh>::value_type>
           && detail::_some_choice<Rh>
[[nodiscard]] constexpr auto operator&(Lh &&, Rh &&rh) //
    noexcept(::std::is_nothrow_constructible_v<::std::remove_cvref_t<Rh>, Rh>) -> ::std::remove_cvref_t<Rh>
{
  return ::std::remove_cvref_t<Rh>{FWD(rh)};
}

template <typename Lh, typename Rh>
  requires detail::_some_choice<Lh> && detail::_some_just<Rh>
           && ::std::is_void_v<typename ::std::remove_cvref_t<Rh>::value_type>
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&) //
    noexcept(::std::is_nothrow_constructible_v<::std::remove_cvref_t<Lh>, Lh>) -> ::std::remove_cvref_t<Lh>
{
  return ::std::remove_cvref_t<Lh>{FWD(lh)};
}

namespace detail {
template <typename T>
concept _identity_expected = _some_expected<T> && empty_copack<typename ::std::remove_cvref_t<T>::error_type>;
template <typename T>
concept _cluster_operand = _some_just<T> || _some_choice<T> || _identity_expected<T>;
template <typename T>
concept _some_carrier = _some_expected<T> || _some_optional<T> || _some_just<T> || _some_choice<T>;

// How a side enters the total disjunction's result: not at all (dead), as the unit pack<>, or as
// its value - the three-way split keeps the dead and void conjuncts from naming an accessor.
template <typename T>
constexpr inline int _inject_kind
    = ::std::is_void_v<typename ::std::remove_cvref_t<T>::value_type> ? 1 : (::fn::detail::_dead_value<T> ? 0 : 2);
template <int Kind, typename Type, typename Side> struct _nothrow_total_inject {
  static constexpr bool value = true;
};
template <typename Type, typename Side> struct _nothrow_total_inject<1, Type, Side> {
  static constexpr bool value = ::std::is_nothrow_constructible_v<Type, pack<>>;
};
template <typename Type, typename Side> struct _nothrow_total_inject<2, Type, Side> {
  static constexpr bool value = ::std::is_nothrow_constructible_v<Type, decltype(::std::declval<Side>().value())>;
};

// Type stays a template parameter so both branches are dependent: a non-dependent discarded
// statement would still be checked against choices without a pack<> alternative.
template <typename Type, typename Side>
[[nodiscard]] constexpr auto _total_inject(Side &&side) //
    noexcept(_nothrow_total_inject<_inject_kind<Side>, Type, Side>::value) -> Type
{
  if constexpr (::std::is_void_v<typename ::std::remove_cvref_t<Side>::value_type>)
    return Type{pack<>{}};
  else
    return Type{FWD(side).value()};
}
} // namespace detail

// The total disjunction: a cluster operand - just, choice, or the identity expected - puts an
// uninhabited factor into the error product, so the result never fails and collapses into the
// cluster: just when the value sum stays one bare type, choice when the union is genuine. The
// leftmost engaged operand wins; a cluster operand is always engaged.
template <typename Lh, typename Rh>
  requires(detail::_cluster_operand<Lh> || detail::_cluster_operand<Rh>) //
          && detail::_some_carrier<Lh> && detail::_some_carrier<Rh>
          && (not ::std::is_void_v<typename ::std::remove_cvref_t<Lh>::value_type>)
          && ::std::is_same_v<typename ::std::remove_cvref_t<Lh>::value_type,
                              typename ::std::remove_cvref_t<Rh>::value_type>
          && (not some_copack<typename ::std::remove_cvref_t<Lh>::value_type>)
[[nodiscard]] constexpr auto operator|(Lh &&lh, Rh &&rh) //
    noexcept(detail::_nothrow_total_inject<detail::_inject_kind<Lh>,
                                           ::fn::just<typename ::std::remove_cvref_t<Lh>::value_type>, Lh>::value
             && detail::_nothrow_total_inject<detail::_inject_kind<Rh>,
                                              ::fn::just<typename ::std::remove_cvref_t<Lh>::value_type>, Rh>::value)
{
  using type = ::fn::just<typename ::std::remove_cvref_t<Lh>::value_type>;
  if constexpr (detail::_cluster_operand<Lh>) {
    return type{FWD(lh).value()};
  } else {
    if (lh.has_value())
      return type{FWD(lh).value()};
    return type{FWD(rh).value()};
  }
}

template <typename Lh, typename Rh>
  requires(detail::_cluster_operand<Lh> || detail::_cluster_operand<Rh>) //
          && detail::_some_carrier<Lh> && detail::_some_carrier<Rh>
          && ::std::is_same_v<typename ::std::remove_cvref_t<Lh>::value_type,
                              typename ::std::remove_cvref_t<Rh>::value_type>
          && some_copack<typename ::std::remove_cvref_t<Lh>::value_type>
[[nodiscard]] constexpr auto operator|(Lh &&lh, Rh &&rh) //
    noexcept(detail::_nothrow_total_inject<
                 detail::_inject_kind<Lh>,
                 typename detail::_rechoice<typename ::std::remove_cvref_t<Lh>::value_type>::type, Lh>::value
             && detail::_nothrow_total_inject<
                 detail::_inject_kind<Rh>,
                 typename detail::_rechoice<typename ::std::remove_cvref_t<Lh>::value_type>::type, Rh>::value)
{
  using type = typename detail::_rechoice<typename ::std::remove_cvref_t<Lh>::value_type>::type;
  if constexpr (detail::_cluster_operand<Lh>) {
    return type{FWD(lh).value()};
  } else {
    if (lh.has_value())
      return type{FWD(lh).value()};
    return type{FWD(rh).value()};
  }
}

template <typename Lh, typename Rh>
  requires(detail::_cluster_operand<Lh> || detail::_cluster_operand<Rh>) //
          && detail::_some_carrier<Lh> && detail::_some_carrier<Rh>
          && ::std::is_void_v<typename ::std::remove_cvref_t<Lh>::value_type>
          && ::std::is_void_v<typename ::std::remove_cvref_t<Rh>::value_type>
[[nodiscard]] constexpr auto operator|(Lh &&, Rh &&) noexcept -> ::fn::just<void>
{
  return ::fn::just<void>{};
}

template <typename Lh, typename Rh>
  requires(detail::_cluster_operand<Lh> || detail::_cluster_operand<Rh>) //
          && detail::_some_carrier<Lh> && detail::_some_carrier<Rh>
          && (not ::std::is_same_v<typename ::std::remove_cvref_t<Lh>::value_type,
                                   typename ::std::remove_cvref_t<Rh>::value_type>)
[[nodiscard]] constexpr auto operator|(Lh &&lh, Rh &&rh) //
    noexcept(
        detail::_nothrow_total_inject<detail::_inject_kind<Lh>,
                                      typename detail::_rechoice<::fn::detail::_disjoined_t<Lh, Rh>>::type, Lh>::value
        && detail::_nothrow_total_inject<
            detail::_inject_kind<Rh>, typename detail::_rechoice<::fn::detail::_disjoined_t<Lh, Rh>>::type, Rh>::value)
{
  using type = typename detail::_rechoice<::fn::detail::_disjoined_t<Lh, Rh>>::type;
  if constexpr (detail::_cluster_operand<Lh>) {
    return detail::_total_inject<type>(FWD(lh));
  } else {
    if constexpr (not ::fn::detail::_dead_value<Lh>) {
      if (lh.has_value())
        return detail::_total_inject<type>(FWD(lh));
    }
    return detail::_total_inject<type>(FWD(rh));
  }
}

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
using choice_for
    = detail::_collapsing_copack::normalized<::fn::choice, detail::_collapsing_copack::flattened<Ts...>>::type;

} // namespace LIBFN_VERSION
} // namespace fn

#include <fn/detail/macro_end.hpp>

#endif // INCLUDE_FN_CHOICE
