// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_ALGEBRA
#define INCLUDE_FN_ALGEBRA

#include <fn/copack.hpp>
#include <fn/detail/functional.hpp>
#include <fn/detail/traits.hpp>
#include <fn/monadic.hpp>
#include <fn/pack.hpp>
#include <libfn_version.hpp>

#include <type_traits>
#include <utility>

#include <fn/detail/macro_begin.hpp>

namespace fn {
inline namespace LIBFN_VERSION {

namespace detail {

// The join checks has_value() before value(); exclude the accessor
// from noexcept calculations for folding and result construction.
template <typename Monad> using _value_of_t = decltype(::std::declval<Monad>().value());

template <typename Monad>
using _factor_t = ::std::conditional_t<::std::is_void_v<typename ::std::remove_cvref_t<Monad>::value_type>,
                                       ::fn::pack<>, typename ::std::remove_cvref_t<Monad>::value_type>;
template <typename Monad>
using _factor_of_t = ::std::conditional_t<::std::is_void_v<typename ::std::remove_cvref_t<Monad>::value_type>,
                                          ::fn::pack<>, _value_of_t<Monad>>;

template <typename Monad> [[nodiscard]] constexpr auto _factor(Monad &&m) -> _factor_of_t<Monad>
{
  if constexpr (::std::is_void_v<typename ::std::remove_cvref_t<Monad>::value_type>)
    return {};
  else
    return FWD(m).value();
}

// An uninhabited factor has no value fold. Avoid instantiating it in types,
// noexcept specifications and bodies.
template <typename Lh, typename Rh>
constexpr inline bool _uninhabited_join = empty_copack<typename ::std::remove_cvref_t<Lh>::value_type>
                                          || empty_copack<typename ::std::remove_cvref_t<Rh>::value_type>;

template <bool Uninhabited, typename Lh, typename Rh> struct _joined {
  using type = copack<>;
};
template <typename Lh, typename Rh> struct _joined<false, Lh, Rh> {
  using type = decltype(::fn::detail::_fold_detail::fold<_factor_t<Lh>, _factor_t<Rh>>(
      ::std::declval<_factor_of_t<Lh>>(), ::std::declval<_factor_of_t<Rh>>()));
};
template <typename Lh, typename Rh> using _joined_t = typename _joined<_uninhabited_join<Lh, Rh>, Lh, Rh>::type;

// Match the named efn parameter, which _join invokes as an lvalue.
template <bool Uninhabited, template <typename> typename Tpl, typename Lh, typename Rh, typename Efn>
struct _nothrow_join_arm {
  static constexpr bool value = ::std::is_nothrow_invocable_v<Efn &, Lh> && ::std::is_nothrow_invocable_v<Efn &, Rh>
                                && _nothrow_initializable<Tpl<copack<>>, ::std::invoke_result_t<Efn &, Lh>>
                                && _nothrow_initializable<Tpl<copack<>>, ::std::invoke_result_t<Efn &, Rh>>;
};
template <template <typename> typename Tpl, typename Lh, typename Rh, typename Efn>
struct _nothrow_join_arm<false, Tpl, Lh, Rh, Efn> {
  static constexpr bool value = noexcept(::fn::detail::_fold_detail::fold<_factor_t<Lh>, _factor_t<Rh>>(
                                    ::std::declval<_factor_of_t<Lh>>(), ::std::declval<_factor_of_t<Rh>>()))
                                && _nothrow_initializable<Tpl<_joined_t<Lh, Rh>>, ::std::in_place_t, _joined_t<Lh, Rh>>
                                && ::std::is_nothrow_invocable_v<Efn &, Lh> && ::std::is_nothrow_invocable_v<Efn &, Rh>
                                && _nothrow_initializable<Tpl<_joined_t<Lh, Rh>>, ::std::invoke_result_t<Efn &, Lh>>
                                && _nothrow_initializable<Tpl<_joined_t<Lh, Rh>>, ::std::invoke_result_t<Efn &, Rh>>;
};
template <template <typename> typename Tpl, typename Lh, typename Rh, typename Efn>
constexpr inline bool _nothrow_join = _nothrow_join_arm<_uninhabited_join<Lh, Rh>, Tpl, Lh, Rh, Efn>::value;

template <typename Lh, typename Rh>
using _disjoined_t = copack_for<_sum_element_t<typename ::std::remove_cvref_t<Lh>::value_type>,
                                _sum_element_t<typename ::std::remove_cvref_t<Rh>::value_type>>;

template <typename T> constexpr inline bool _dead_value = empty_copack<typename ::std::remove_cvref_t<T>::value_type>;

// Uninhabited values cannot reach the injection arm.
template <bool Dead, typename Type, typename Side> struct _nothrow_disj_inject {
  static constexpr bool value = true;
};
template <typename Type, typename Side> struct _nothrow_disj_inject<false, Type, Side> {
  static constexpr bool value
      = _nothrow_initializable<Type, ::std::in_place_t, decltype(::std::declval<Side>().value())>;
};

template <template <typename> typename Tpl>
[[nodiscard]] constexpr auto _join(auto &&lh, auto &&rh, auto &&efn) //
    noexcept(_nothrow_join<Tpl, decltype(lh), decltype(rh), decltype(efn)>)
        -> Tpl<_joined_t<decltype(lh), decltype(rh)>>
{
  using type = Tpl<_joined_t<decltype(lh), decltype(rh)>>;
  if constexpr (_uninhabited_join<decltype(lh), decltype(rh)>) {
    if (not lh.has_value())
      return type{efn(FWD(lh))};
    else
      return type{efn(FWD(rh))};
  } else {
    using Lh = _factor_t<decltype(lh)>;
    using Rh = _factor_t<decltype(rh)>;
    if (lh.has_value() && rh.has_value())
      return type{::std::in_place, ::fn::detail::_fold_detail::fold<Lh, Rh>(_factor(FWD(lh)), _factor(FWD(rh)))};
    else if (not lh.has_value())
      return type{efn(FWD(lh))};
    else
      return type{efn(FWD(rh))};
  }
}

} // namespace detail

/**
 * @brief The data conjunction: concatenates into a `pack`, distributing over `copack` alternatives
 *
 * With plain data on both sides the fields concatenate into one flat `pack`. When either operand
 * is a `copack`, the product distributes over its alternatives - two copacks yield the full
 * cartesian product - producing a normalized `copack` of `pack`s. Dispatches on its left operand:
 * a bare `scalar & scalar` is not part of the algebra, so lift one side first, as in
 * `fn::as_pack(a) & b`.
 *
 * @param lh A `pack` or a `copack`
 * @param rh The data to conjoin: a scalar, a `pack` or a `copack`
 * @return A `pack`, or a `copack` of `pack`s where alternatives distribute
 */
[[nodiscard]] constexpr auto operator&(auto &&lh, auto &&rh) //
    noexcept(noexcept(::fn::detail::_fold_detail::fold<::std::remove_cvref_t<decltype(lh)>,
                                                       ::std::remove_cvref_t<decltype(rh)>>(FWD(lh), FWD(rh))))
  requires(some_copack<decltype(lh)> || some_pack<decltype(lh)>)
{
  using Lh = ::std::remove_cvref_t<decltype(lh)>;
  using Rh = ::std::remove_cvref_t<decltype(rh)>;
  return ::fn::detail::_fold_detail::fold<Lh, Rh>(FWD(lh), FWD(rh));
}

namespace detail {
// Reject carriers here to avoid silently storing them as pack elements.
template <typename... Ts>
concept _no_carrier = (... && (not some_monadic_type<Ts>));

template <typename... Ts>
concept _all_carriers = (... && some_monadic_type<Ts>);
} // namespace detail

/**
 * @brief The n-ary fold of `operator &` above; a single argument is forwarded unchanged
 *
 * Arguments must be all carriers or all data. Data forms a product, with a leading scalar
 * lifted into a `pack`; carriers compose through their `operator &`.
 */
constexpr inline struct conjoin_t {
  /**
   * @brief Forwards a single argument unchanged
   * @param arg The argument
   * @return The argument, forwarded
   */
  template <typename Arg> [[nodiscard]] constexpr auto operator()(Arg &&arg) const -> decltype(arg) { return FWD(arg); }

  /**
   * @brief Folds data into a product, or carriers into their conjunction
   *
   * @param arg The leading argument
   * @param args Further arguments - all data, or all carriers, never the two mixed
   * @return The folded product, or the folded conjunction
   */
  template <typename Arg, typename... Args>
    requires(not some_copack<Arg>) && (not some_pack<Arg>) && detail::_no_carrier<Arg, Args...>
  [[nodiscard]] constexpr auto operator()(Arg &&arg, Args &&...args) const
  {
    return (::fn::pack{FWD(arg)} & ... & FWD(args));
  }

  template <typename Arg, typename... Args>
    requires(some_copack<Arg> || some_pack<Arg>) && detail::_no_carrier<Args...>
  [[nodiscard]] constexpr auto operator()(Arg &&arg, Args &&...args) const
  {
    return (FWD(arg) & ... & FWD(args));
  }

  template <typename Arg, typename... Args>
    requires(sizeof...(Args) > 0)
            && detail::_all_carriers<Arg, Args...> && requires(Arg &&a, Args &&...as) { (FWD(a) & ... & FWD(as)); }
  [[nodiscard]] constexpr auto operator()(Arg &&arg, Args &&...args) const //
      noexcept(noexcept((FWD(arg) & ... & FWD(args))))
  {
    return (FWD(arg) & ... & FWD(args));
  }
} conjoin; ///< The n-ary conjunction: `conjoin(a, b, c)`

/**
 * @brief The n-ary fold of the disjunction `operator |` over the monadic carriers; a single
 *        argument is forwarded unchanged
 *
 * Carriers only, in every arity - disjunction has no data-level form. An identity-cluster operand
 * makes the whole disjunction total, folding the result into `just` or `choice`.
 */
constexpr inline struct disjoin_t {
  /**
   * @brief Forwards a single carrier unchanged
   * @param arg The carrier
   * @return The carrier, forwarded
   */
  template <some_monadic_type Arg> [[nodiscard]] constexpr auto operator()(Arg &&arg) const -> decltype(arg)
  {
    return FWD(arg);
  }

  /**
   * @brief Folds the carriers into their disjunction
   *
   * The n-ary form of `operator |`: the result holds the first operand that worked, its values
   * summing into a `copack`, and the errors multiply into a `pack` reached only where every
   * operand failed. An identity-cluster operand makes the whole disjunction total.
   *
   * @param arg The leading carrier
   * @param args Further carriers to disjoin
   * @return The folded disjunction
   */
  template <typename Arg, typename... Args>
    requires(sizeof...(Args) > 0)
            && detail::_all_carriers<Arg, Args...> && requires(Arg &&a, Args &&...as) { (FWD(a) | ... | FWD(as)); }
  [[nodiscard]] constexpr auto operator()(Arg &&arg, Args &&...args) const //
      noexcept(noexcept((FWD(arg) | ... | FWD(args))))
  {
    return (FWD(arg) | ... | FWD(args));
  }
} disjoin; ///< The n-ary disjunction: `disjoin(a, b, c)`

} // namespace LIBFN_VERSION
} // namespace fn

#include <fn/detail/macro_end.hpp>

#endif // INCLUDE_FN_ALGEBRA
