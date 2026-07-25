// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_AND_THEN
#define INCLUDE_FN_AND_THEN

#include <fn/choice.hpp>
#include <fn/concepts.hpp>
#include <fn/expected.hpp>
#include <fn/functional.hpp>
#include <fn/functor.hpp>
#include <fn/optional.hpp>
#include <libfn_version.hpp>

#include <type_traits>

#include <fn/detail/macro_begin.hpp>

namespace fn {
inline namespace LIBFN_VERSION {

namespace detail {
// the first join that announces a `type` wins; neither is a hard error to ask
template <typename A, typename B> struct _either_join : B {};
template <typename A, typename B>
  requires requires { typename A::type; }
struct _either_join<A, B> : A {};

template <typename Fn, typename Cp>
constexpr inline bool _cluster_join_forms
    = requires { typename _copack_apply_result<_joining_cluster_tag<::fn::choice>, Fn, Cp>::type; };

// The engine's result over a bound payload, computed assert-free: a copack payload goes through
// the cluster trait - all-choice branch results answer the superset choice, convergent ones the
// select trait's answer - and a branch set the cluster join does not own may still join as
// optionals (the #376 bridge); every other set answers none at all, so asking about any divergent
// callback answers instead of tripping select's convergence assert; a single value goes through
// _apply_result as always.
template <typename Fn, typename... V> struct _and_then_result : _apply_result<Fn, V...> {};
template <typename Fn, typename V>
  requires _some_copack<::std::remove_cvref_t<V>>
struct _and_then_result<Fn, V>
    : _either_join<_typelist_joining_cluster<::fn::choice, Fn, V, ::std::remove_cvref_t<V>>,
                   _typelist_joining_optional<::fn::optional, Fn, V, ::std::remove_cvref_t<V>>> {};

// the copack arm's promise, split on the same choice of tag its body makes
template <typename Fn, typename Cp> struct _nothrow_and_then_join {
  static constexpr bool value = [] {
    if constexpr (_cluster_join_forms<Fn, Cp>)
      return _is_nothrow_rts_applicable<typename _copack_apply_result<_joining_cluster_tag<::fn::choice>, Fn, Cp>::type,
                                        Fn, Cp>;
    else
      return _is_nothrow_rts_applicable<
          typename _copack_apply_result<_joining_optional_tag<::fn::optional>, Fn, Cp>::type, Fn, Cp>;
  }();
};
} // namespace detail

/**
 * @brief Checks if the monadic type can be used with the `and_then` operation
 *
 * @tparam Fn The function to execute on the value
 * @tparam V The monadic type
 */
template <typename Fn, typename V>
concept applicable_and_then //
    = (some_expected_non_void<V> && requires(V &&v) {
        typename detail::_and_then_dispatch<typename ::std::remove_cvref_t<V>::error_type, Fn,
                                            decltype(FWD(v).value())>::type;
        requires same_kind<V, typename detail::_and_then_dispatch<typename ::std::remove_cvref_t<V>::error_type, Fn,
                                                                  decltype(FWD(v).value())>::type>;
      }) || (some_expected_non_void<V> //
         && some_copack<typename ::std::remove_cvref_t<V>::error_type> && requires(V &&v) {
        typename detail::_and_then_dispatch<typename ::std::remove_cvref_t<V>::error_type, Fn,
                                            decltype(FWD(v).value())>::type;
        requires some_expected<typename detail::_and_then_dispatch<typename ::std::remove_cvref_t<V>::error_type, Fn,
                                                                   decltype(FWD(v).value())>::type>;
      }) || (some_expected_void<V> && requires(Fn &&fn) {
        {
          ::fn::apply(FWD(fn))
        } -> same_kind<V>;
      }) || (some_expected_void<V> //
         && some_copack<typename ::std::remove_cvref_t<V>::error_type> && requires(Fn &&fn) {
        {
          ::fn::apply(FWD(fn))
        } -> some_expected;
      }) || (some_optional<V> && requires(V &&v) {
        typename detail::_optional_and_then_dispatch<Fn, decltype(FWD(v).value())>::type;
        requires same_kind<V, typename detail::_optional_and_then_dispatch<Fn, decltype(FWD(v).value())>::type>;
      }) || (some_choice<V> && requires(V &&v) {
        // asked of the assert-free trait, never the member: a divergent branch set must answer
        // false here, where naming the member would trip select's convergence assert
        typename detail::_and_then_result<Fn, decltype(FWD(v).value())>::type;
        requires same_kind<V, typename detail::_and_then_result<Fn, decltype(FWD(v).value())>::type>;
      }) || (some_just<V> && requires(Fn &&fn, V &&v) {
        {
          FWD(v).and_then(FWD(fn))
        } -> same_kind<V>;
      });

/**
 * @brief Checks if the identity carrier can be used with `and_then` binding across the cluster's
 *        carrier kinds
 *
 * The callback may return any identity carrier and the bind follows the function; results of the
 * input's own kind are left to `applicable_and_then` and the carrier's member. Over a copack
 * payload the branches either all return choices - joined into the superset - or converge on one
 * carrier type.
 *
 * @tparam Fn The function to execute on the value
 * @tparam V The identity carrier
 */
template <typename Fn, typename V>
concept applicable_and_then_across //
    = some_identity<V>
      && ((some_choice<V> && (not applicable_and_then<Fn, V>) && requires(V &&v) {
            typename detail::_and_then_result<Fn, decltype(FWD(v).value())>::type;
            requires some_identity<typename detail::_and_then_result<Fn, decltype(FWD(v).value())>::type>
                         || some_optional<typename detail::_and_then_result<Fn, decltype(FWD(v).value())>::type>
                         || some_expected<typename detail::_and_then_result<Fn, decltype(FWD(v).value())>::type>;
          }) || (some_just<V> && (not applicable_and_then<Fn, V>) && (not ::std::is_void_v<typename ::std::remove_cvref_t<V>::value_type>) && requires(V &&v) {
            typename detail::_and_then_result<Fn, decltype(FWD(v).value())>::type;
            requires some_identity<typename detail::_and_then_result<Fn, decltype(FWD(v).value())>::type>
                         || some_optional<typename detail::_and_then_result<Fn, decltype(FWD(v).value())>::type>
                         || some_expected<typename detail::_and_then_result<Fn, decltype(FWD(v).value())>::type>;
          }) || (some_expected_non_void<V> && requires(V &&v) {
            typename detail::_and_then_result<Fn, decltype(FWD(v).value())>::type;
            requires some_identity<typename detail::_and_then_result<Fn, decltype(FWD(v).value())>::type>
                         || some_optional<typename detail::_and_then_result<Fn, decltype(FWD(v).value())>::type>;
            requires not some_expected<typename detail::_and_then_result<Fn, decltype(FWD(v).value())>::type>;
          }) || (some_expected_void<V> && requires {
            typename detail::_and_then_result<Fn>::type;
            requires some_identity<typename detail::_and_then_result<Fn>::type>
                         || some_optional<typename detail::_and_then_result<Fn>::type>;
            // expected results stay on the member: the void identity expected keeps its graded
            // world (a copack<> grade acquiring the callback's error), never the bare carrier
            requires not some_expected<typename detail::_and_then_result<Fn>::type>;
          }) || (some_just<V> && ::std::is_void_v<typename ::std::remove_cvref_t<V>::value_type> && (not applicable_and_then<Fn, V>) && requires {
            typename detail::_and_then_result<Fn>::type;
            requires some_identity<typename detail::_and_then_result<Fn>::type>
                         || some_optional<typename detail::_and_then_result<Fn>::type>
                         || some_expected<typename detail::_and_then_result<Fn>::type>;
          }));

/**
 * @brief Execute a function on the value of the monadic type if the value is present
 *
 * Use through the `fn::and_then` nielbloid.
 */
constexpr inline struct and_then_t final {
  /**
   * @brief Execute a function on the value of the monadic type if the value is present
   * @param fn The function to execute on the value
   * @return A functor that will execute the function on the value
   */
  [[nodiscard]] constexpr auto operator()(auto &&fn) const
      noexcept(noexcept(functor<and_then_t, decltype(fn)>{FWD(fn)})) -> functor<and_then_t, decltype(fn)> //
  {
    return {FWD(fn)};
  }

  struct apply;
} and_then = {};

struct and_then_t::apply final {
  /**
   * @brief TODO
   *
   * @param v The monad
   * @param fn The function to apply
   */
  template <some_monadic_type V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const //
      noexcept(noexcept(FWD(v).and_then(FWD(fn))))              //
      -> same_kind<V &&> auto
    requires(not(some_expected<V> && some_identity<V>)) && applicable_and_then<Fn &&, V &&>
  {
    return FWD(v).and_then(FWD(fn));
  }

  // An identity expected keeps its member semantics for expected results - the same-kind and
  // error-widening paths above - but must not evaluate applicable_and_then's optional/choice
  // disjuncts through the exclusion in the arm above, hence its own arm.
  template <some_monadic_type V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const //
      noexcept(noexcept(FWD(v).and_then(FWD(fn))))              //
      -> same_kind<V &&> auto
    requires some_expected<V> && some_identity<V> && applicable_and_then<Fn &&, V &&>
  {
    return FWD(v).and_then(FWD(fn));
  }

  // An uninhabited value side leaves no value to bind - delegate to the member, which is the
  // identity there and neither invokes nor instantiates the callback, as in transform.
  template <some_monadic_type V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const //
      noexcept(noexcept(FWD(v).and_then(FWD(fn))))              //
      -> same_kind<V &&> auto
    requires some_empty_value<V>
  {
    return FWD(v).and_then(FWD(fn));
  }

  // The cluster arms: an identity input binds across the carrier kinds, and the bind follows the
  // function. Engine-direct - the members are each carrier's own endo-bind, so the cluster cannot
  // ride delegation, and the verb layer is the licensed cross-carrier place. Dropping the input's
  // channels forgets nothing: they are uninhabited, which is what admits the input here at all.
  template <some_monadic_type V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const                                     //
      noexcept(detail::_nothrow_and_then_join<Fn &&, decltype(::std::declval<V>().value())>::value) //
      -> some_monadic_type auto
    requires applicable_and_then_across<Fn &&, V &&>
             && (not ::std::is_void_v<typename ::std::remove_cvref_t<V>::value_type>)
             && some_copack<::std::remove_cvref_t<decltype(::std::declval<V>().value())>>
  {
    if constexpr (detail::_cluster_join_forms<Fn &&, decltype(FWD(v).value())>)
      return detail::_tagged_join_apply<detail::_joining_cluster_tag<choice>>(FWD(v).value(), FWD(fn));
    else
      return detail::_tagged_join_apply<detail::_joining_optional_tag<::fn::optional>>(FWD(v).value(), FWD(fn));
  }

  template <some_monadic_type V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const //
      noexcept(noexcept(::fn::apply(FWD(fn), FWD(v).value())))  //
      -> some_monadic_type auto
    requires applicable_and_then_across<Fn &&, V &&>
             && (not ::std::is_void_v<typename ::std::remove_cvref_t<V>::value_type>)
             && (not some_copack<::std::remove_cvref_t<decltype(::std::declval<V>().value())>>)
  {
    return ::fn::apply(FWD(fn), FWD(v).value());
  }

  template <some_monadic_type V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&, Fn &&fn) const //
      noexcept(noexcept(::fn::apply(FWD(fn))))                 //
      -> some_monadic_type auto
    requires applicable_and_then_across<Fn &&, V &&> && ::std::is_void_v<typename ::std::remove_cvref_t<V>::value_type>
  {
    return ::fn::apply(FWD(fn));
  }
};

} // namespace LIBFN_VERSION
} // namespace fn

#include <fn/detail/macro_end.hpp>

#endif // INCLUDE_FN_AND_THEN
