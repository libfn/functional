// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_PACK
#define INCLUDE_FN_PACK

#include <fn/detail/macro_deduced_return.hpp>
#include <fn/detail/macro_fwd.hpp>
#include <fn/detail/meta.hpp>
#include <fn/detail/pack_impl.hpp>
#include <fn/sum.hpp>

#include <type_traits>

namespace fn {

/**
 * @brief TODO
 *
 * @tparam T TODO
 */
template <typename T>
concept some_pack = detail::_some_pack<T>;

/**
 * @brief TODO
 *
 * Some more text here.
 *
 * @tparam Ts TODO
 */
template <typename... Ts> struct pack : detail::pack_impl<::std::index_sequence_for<Ts...>, Ts...> {
  using _impl = detail::pack_impl<::std::index_sequence_for<Ts...>, Ts...>;
  static_assert((... && detail::_is_valid_pack_element<Ts>));

  template <typename T> using append_type = _impl::template append_type<T>;

  /**
   * @brief appends a thing
   *
   * @tparam T TODO
   * @param args TODO
   * @return TODO
   */
  template <typename T>
  [[nodiscard]] constexpr auto append(::std::in_place_type_t<T>, auto &&...args) & //
      noexcept(noexcept(_impl::template _append<T>(::std::declval<pack &>(), FWD(args)...))) -> append_type<T>
    requires requires { append_type<T>{_impl::template _append<T>(*this, FWD(args)...)}; }
  {
    return {_impl::template _append<T>(*this, FWD(args)...)};
  }

  /**
   * @brief appends a thing
   *
   * @tparam T TODO
   * @param args TODO
   * @return TODO
   */
  template <typename T>
  [[nodiscard]] constexpr auto append(::std::in_place_type_t<T>, auto &&...args) const & //
      noexcept(noexcept(_impl::template _append<T>(::std::declval<pack const &>(), FWD(args)...))) -> append_type<T>
    requires requires { append_type<T>{_impl::template _append<T>(*this, FWD(args)...)}; }
  {
    return {_impl::template _append<T>(*this, FWD(args)...)};
  }

  /**
   * @brief appends a thing
   *
   * @tparam T TODO
   * @param args TODO
   * @return TODO
   */
  template <typename T>
  [[nodiscard]] constexpr auto append(::std::in_place_type_t<T>, auto &&...args) && //
      noexcept(noexcept(_impl::template _append<T>(::std::declval<pack &&>(), FWD(args)...))) -> append_type<T>
    requires requires { append_type<T>{_impl::template _append<T>(::std::move(*this), FWD(args)...)}; }
  {
    return {_impl::template _append<T>(::std::move(*this), FWD(args)...)};
  }

  /**
   * @brief appends a thing
   *
   * @tparam T TODO
   * @param args TODO
   * @return TODO
   */
  template <typename T>
  [[nodiscard]] constexpr auto append(::std::in_place_type_t<T>, auto &&...args) const && //
      noexcept(noexcept(_impl::template _append<T>(::std::declval<pack const &&>(), FWD(args)...))) -> append_type<T>
    requires requires { append_type<T>{_impl::template _append<T>(::std::move(*this), FWD(args)...)}; }
  {
    return {_impl::template _append<T>(::std::move(*this), FWD(args)...)};
  }

  /**
   * @brief appends a thing
   *
   * @tparam Arg TODO
   * @param arg TODO
   * @return TODO
   */
  template <typename Arg>
  [[nodiscard]] constexpr auto append(Arg &&arg) & //
      noexcept(noexcept(_impl::template _append<Arg>(::std::declval<pack &>(), FWD(arg)))) -> append_type<Arg>
    requires(not some_in_place_type<Arg>)
            && requires { append_type<Arg>{_impl::template _append<Arg>(*this, FWD(arg))}; }
  {
    return {_impl::template _append<Arg>(*this, FWD(arg))};
  }

  /**
   * @brief appends a thing
   *
   * @tparam Arg TODO
   * @param arg TODO
   * @return TODO
   */
  template <typename Arg>
  [[nodiscard]] constexpr auto append(Arg &&arg) const & //
      noexcept(noexcept(_impl::template _append<Arg>(::std::declval<pack const &>(), FWD(arg)))) -> append_type<Arg>
    requires(not some_in_place_type<Arg>)
            && requires { append_type<Arg>{_impl::template _append<Arg>(*this, FWD(arg))}; }
  {
    return {_impl::template _append<Arg>(*this, FWD(arg))};
  }

  /**
   * @brief appends a thing
   *
   * @tparam Arg TODO
   * @param arg TODO
   * @return TODO
   */
  template <typename Arg>
  [[nodiscard]] constexpr auto append(Arg &&arg) && //
      noexcept(noexcept(_impl::template _append<Arg>(::std::declval<pack &&>(), FWD(arg)))) -> append_type<Arg>
    requires(not some_in_place_type<Arg>)
            && requires { append_type<Arg>{_impl::template _append<Arg>(::std::move(*this), FWD(arg))}; }
  {
    return {_impl::template _append<Arg>(::std::move(*this), FWD(arg))};
  }

  /**
   * @brief appends a thing
   *
   * @tparam Arg TODO
   * @param arg TODO
   * @return TODO
   */
  template <typename Arg>
  [[nodiscard]] constexpr auto append(Arg &&arg) const && //
      noexcept(noexcept(_impl::template _append<Arg>(::std::declval<pack const &&>(), FWD(arg)))) -> append_type<Arg>
    requires(not some_in_place_type<Arg>)
            && requires { append_type<Arg>{_impl::template _append<Arg>(::std::move(*this), FWD(arg))}; }
  {
    return {_impl::template _append<Arg>(::std::move(*this), FWD(arg))};
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto apply(Fn &&fn, auto &&...args) & //
      noexcept(noexcept(_impl::_apply(::std::declval<pack &>(), FWD(fn), FWD(args)...)))
          -> DEDUCED_RETURN(_impl::_apply(*this, FWD(fn), FWD(args)...))
    requires requires { _impl::_apply(*this, FWD(fn), FWD(args)...); }
  {
    return _impl::_apply(*this, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto apply(Fn &&fn, auto &&...args) const & //
      noexcept(noexcept(_impl::_apply(::std::declval<pack const &>(), FWD(fn), FWD(args)...)))
          -> DEDUCED_RETURN(_impl::_apply(*this, FWD(fn), FWD(args)...))
    requires requires { _impl::_apply(*this, FWD(fn), FWD(args)...); }
  {
    return _impl::_apply(*this, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto apply(Fn &&fn, auto &&...args) && //
      noexcept(noexcept(_impl::_apply(::std::declval<pack &&>(), FWD(fn), FWD(args)...)))
          -> DEDUCED_RETURN(_impl::_apply(::std::move(*this), FWD(fn), FWD(args)...))
    requires requires { _impl::_apply(::std::move(*this), FWD(fn), FWD(args)...); }
  {
    return _impl::_apply(::std::move(*this), FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto apply(Fn &&fn, auto &&...args) const && //
      noexcept(noexcept(_impl::_apply(::std::declval<pack const &&>(), FWD(fn), FWD(args)...)))
          -> DEDUCED_RETURN(_impl::_apply(::std::move(*this), FWD(fn), FWD(args)...))
    requires requires { _impl::_apply(::std::move(*this), FWD(fn), FWD(args)...); }
  {
    return _impl::_apply(::std::move(*this), FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Ret TODO
   * @tparam Fn TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Ret, typename Fn>
  [[nodiscard]] constexpr auto apply_r(Fn &&fn, auto &&...args) & //
      noexcept(noexcept(_impl::template _apply_r<Ret>(::std::declval<pack &>(), FWD(fn), FWD(args)...))) -> Ret
    requires requires { _impl::template _apply_r<Ret>(*this, FWD(fn), FWD(args)...); }
  {
    return _impl::template _apply_r<Ret>(*this, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Ret TODO
   * @tparam Fn TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Ret, typename Fn>
  [[nodiscard]] constexpr auto apply_r(Fn &&fn, auto &&...args) const & //
      noexcept(noexcept(_impl::template _apply_r<Ret>(::std::declval<pack const &>(), FWD(fn), FWD(args)...))) -> Ret
    requires requires { _impl::template _apply_r<Ret>(*this, FWD(fn), FWD(args)...); }
  {
    return _impl::template _apply_r<Ret>(*this, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Ret TODO
   * @tparam Fn TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Ret, typename Fn>
  [[nodiscard]] constexpr auto apply_r(Fn &&fn, auto &&...args) && //
      noexcept(noexcept(_impl::template _apply_r<Ret>(::std::declval<pack &&>(), FWD(fn), FWD(args)...))) -> Ret
    requires requires { _impl::template _apply_r<Ret>(::std::move(*this), FWD(fn), FWD(args)...); }
  {
    return _impl::template _apply_r<Ret>(::std::move(*this), FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Ret TODO
   * @tparam Fn TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Ret, typename Fn>
  [[nodiscard]] constexpr auto apply_r(Fn &&fn, auto &&...args) const && //
      noexcept(noexcept(_impl::template _apply_r<Ret>(::std::declval<pack const &&>(), FWD(fn), FWD(args)...))) -> Ret
    requires requires { _impl::template _apply_r<Ret>(::std::move(*this), FWD(fn), FWD(args)...); }
  {
    return _impl::template _apply_r<Ret>(::std::move(*this), FWD(fn), FWD(args)...);
  }
};

template <typename... Args> pack(Args &&...args) -> pack<Args...>;

/**
 * @brief Tuple-protocol element access
 *
 * Returns the `I`-th element carrying the pack's cv-qualification and value
 * category, exactly as `apply` would pass it. Found by ADL, so it also serves
 * structured bindings and the generic `using ::std::get; get<I>(p)` idiom.
 *
 * @tparam I element index
 * @param p the pack
 * @return reference to the `I`-th element
 */
template <::std::size_t I, some_pack P>
[[nodiscard]] constexpr decltype(auto) get(P &&p) noexcept
  requires(I < ::std::remove_cvref_t<P>::size)
{
  return ::std::remove_cvref_t<P>::template _get<I>(FWD(p));
}

// Lifts
/**
 * @brief TODO
 *
 * @tparam T TODO
 * @tparam Args TODO
 * @param src TODO
 * @param args TODO
 * @return TODO
 */
[[nodiscard]] constexpr auto as_pack() noexcept -> pack<> { return {}; }
template <typename T, typename... Args>
  requires(not some_in_place_type<T>) && detail::_initializable<pack<T, Args...>, T, Args...>
[[nodiscard]] constexpr auto as_pack(T &&src, Args &&...args) //
    noexcept(detail::_nothrow_initializable<pack<T, Args...>, T, Args...>) -> pack<T, Args...>
{
  return pack<T, Args...>{FWD(src), FWD(args)...};
}

namespace detail {

// `value()` throws when the monad holds no value, but the join only reaches it once `has_value()`
// has answered - so the specification below asks what folding and lifting the value promise, with
// the accessor spelled as a type rather than as a call which would drag its own throw in.
template <typename Monad> using _value_of_t = decltype(::std::declval<Monad>().value());

template <typename Lh, typename Rh>
using _joined_t = decltype(::fn::detail::_fold_detail::fold<typename ::std::remove_cvref_t<Lh>::value_type,
                                                            typename ::std::remove_cvref_t<Rh>::value_type>(
    ::std::declval<_value_of_t<Lh>>(), ::std::declval<_value_of_t<Rh>>()));

// `_join` invokes `efn` as an lvalue - a named parameter - so the `Efn &` questions ask about the
// call the body performs; the reference collapses to it whatever category the callable arrived in.
template <template <typename> typename Tpl, typename Lh, typename Rh, typename Efn>
constexpr inline bool _nothrow_join
    = noexcept(::fn::detail::_fold_detail::fold<typename ::std::remove_cvref_t<Lh>::value_type,
                                                typename ::std::remove_cvref_t<Rh>::value_type>(
          ::std::declval<_value_of_t<Lh>>(), ::std::declval<_value_of_t<Rh>>()))
      && _nothrow_initializable<Tpl<_joined_t<Lh, Rh>>, ::std::in_place_t, _joined_t<Lh, Rh>>
      && ::std::is_nothrow_invocable_v<Efn &, Lh> && ::std::is_nothrow_invocable_v<Efn &, Rh>
      && _nothrow_initializable<Tpl<_joined_t<Lh, Rh>>, ::std::invoke_result_t<Efn &, Lh>>
      && _nothrow_initializable<Tpl<_joined_t<Lh, Rh>>, ::std::invoke_result_t<Efn &, Rh>>;

template <template <typename> typename Tpl>
[[nodiscard]] constexpr auto _join(auto &&lh, auto &&rh, auto &&efn) //
    noexcept(_nothrow_join<Tpl, decltype(lh), decltype(rh), decltype(efn)>)
        -> Tpl<_joined_t<decltype(lh), decltype(rh)>>
{
  using Lh = ::std::remove_cvref_t<decltype(lh)>::value_type;
  using Rh = ::std::remove_cvref_t<decltype(rh)>::value_type;
  using type = Tpl<_joined_t<decltype(lh), decltype(rh)>>;
  if (lh.has_value() && rh.has_value())
    return type{::std::in_place, ::fn::detail::_fold_detail::fold<Lh, Rh>(FWD(lh).value(), FWD(rh).value())};
  else if (not lh.has_value())
    return type{efn(FWD(lh))};
  else
    return type{efn(FWD(rh))};
}

} // namespace detail

/**
 * @brief Data concantenation operator - creates a pack or a sum of packs
 *
 * @param lh TODO
 * @param rh TODO
 * @return TODO
 */
[[nodiscard]] constexpr auto operator&(auto &&lh, auto &&rh) //
    noexcept(noexcept(::fn::detail::_fold_detail::fold<::std::remove_cvref_t<decltype(lh)>,
                                                       ::std::remove_cvref_t<decltype(rh)>>(FWD(lh), FWD(rh))))
  requires(some_sum<decltype(lh)> || some_pack<decltype(lh)>)
{
  using Lh = ::std::remove_cvref_t<decltype(lh)>;
  using Rh = ::std::remove_cvref_t<decltype(rh)>;
  return ::fn::detail::_fold_detail::fold<Lh, Rh>(FWD(lh), FWD(rh));
}

/**
 * @brief Identity, which is basically lift for operator & above
 */
constexpr inline struct identity_t {
  /**
   * @brief TODO
   *
   * @tparam Arg TODO
   * @param arg TODO
   * @return TODO
   */
  template <typename Arg> [[nodiscard]] constexpr auto operator()(Arg &&arg) const -> decltype(arg) { return FWD(arg); }

  /**
   * @brief TODO
   *
   * @tparam Arg TODO
   * @tparam Args TODO
   * @param arg TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Arg, typename... Args>
    requires(not some_sum<Arg>) && (not some_pack<Arg>)
  [[nodiscard]] constexpr auto operator()(Arg &&arg, Args &&...args) const
  {
    return (::fn::pack{FWD(arg)} & ... & FWD(args));
  }

  /**
   * @brief TODO
   *
   * @tparam Arg TODO
   * @tparam Args TODO
   * @param arg TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Arg, typename... Args>
    requires some_sum<Arg> || some_pack<Arg>
  [[nodiscard]] constexpr auto operator()(Arg &&arg, Args &&...args) const
  {
    return (FWD(arg) & ... & FWD(args));
  }
} identity;

} // namespace fn

namespace std {
template <typename... Ts>
struct tuple_size<::fn::pack<Ts...>> : ::std::integral_constant<::std::size_t, sizeof...(Ts)> {};

template <::std::size_t I, typename... Ts> struct tuple_element<I, ::fn::pack<Ts...>> {
  using type = ::fn::detail::select_nth_t<I, Ts...>;
};

// A const pack propagates const onto reference elements, so `tuple_element<I, pack const>`
// must match `get` and cannot defer to the generic `tuple_element<I, const T>`.
template <::std::size_t I, typename... Ts> struct tuple_element<I, ::fn::pack<Ts...> const> {
  using type = decltype(::fn::detail::_apply_const<::fn::pack<Ts...> const &, ::fn::detail::select_nth_t<I, Ts...>>);
};
} // namespace std

#endif // INCLUDE_FN_PACK
