// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_OR_ELSE
#define INCLUDE_FN_OR_ELSE

#include <fn/concepts.hpp>
#include <fn/expected.hpp>
#include <fn/functional.hpp>
#include <fn/functor.hpp>
#include <fn/optional.hpp>
#include <libfn_version.hpp>

#include <fn/detail/macro_begin.hpp>

namespace fn {
inline namespace LIBFN_VERSION {

namespace detail {
// optional -> expected: the empty state's nullary callback names the target carrier, and self's
// value joins the target's value side under the grade-sticky rules
template <typename T, typename Fn> struct _or_else_to_expected {};
template <typename T, typename Fn>
  requires(_is_applicable<Fn>::value)
          && _joining_superset::is_kind<::fn::expected, ::std::remove_cvref_t<typename _apply_result<Fn>::type>>
          && requires {
               typename _joining_expected::graded_join<T, typename _joining_expected::parts<::std::remove_cvref_t<
                                                              typename _apply_result<Fn>::type>>::value_type>::type;
             }
struct _or_else_to_expected<T, Fn> {
  using _result = ::std::remove_cvref_t<typename _apply_result<Fn>::type>;
  using type = ::fn::expected<
      typename _joining_expected::graded_join<T, typename _joining_expected::parts<_result>::value_type>::type,
      typename _joining_expected::parts<_result>::error_type>;
};

// expected -> optional: the error's callback (per alternative when graded) names the target;
// the pass-through value joins every branch's value
template <typename T, typename Fn, typename ErrArg> struct _or_else_to_optional {};
template <typename T, typename Fn, typename ErrArg>
  requires(not _some_copack<::std::remove_cvref_t<ErrArg>>) && (_is_applicable<Fn, ErrArg>::value)
          && _joining_superset::is_kind<::fn::optional, ::std::remove_cvref_t<typename _apply_result<Fn, ErrArg>::type>>
struct _or_else_to_optional<T, Fn, ErrArg>
    : _joined_optional_recovery<::fn::optional, T, ::std::remove_cvref_t<typename _apply_result<Fn, ErrArg>::type>> {};
template <typename T, typename Fn, typename ErrArg>
  requires _some_copack<::std::remove_cvref_t<ErrArg>>
struct _or_else_to_optional<T, Fn, ErrArg>
    : _copack_apply_result<_joining_optional_recovery_tag<::fn::optional, T>, Fn, ErrArg> {};

// the promises, computed per arm; a dead arm never weighs
template <typename T, typename Fn, typename ValArg> struct _nothrow_or_else_to_expected {
  using _result = ::std::remove_cvref_t<typename _apply_result<Fn>::type>;
  using _type = typename _or_else_to_expected<T, Fn>::type;
  static constexpr bool _convert
      = ::std::is_same_v<_result, _type>
        || ((empty_copack<typename _result::value_type>
             || ::std::is_nothrow_constructible_v<_type, ::std::in_place_t, typename _result::value_type &&>)
            && ::std::is_nothrow_constructible_v<_type, ::fn::unexpect_t, typename _result::error_type &&>);
  static constexpr bool value
      = _is_nothrow_applicable<Fn>::value && _convert
        && (empty_copack<T> || ::std::is_nothrow_constructible_v<_type, ::std::in_place_t, ValArg>);
};

template <typename T, typename Fn, typename ErrArg, typename ValArg> struct _nothrow_or_else_to_optional;
template <typename T, typename Fn, typename ErrArg, typename ValArg>
  requires _some_copack<::std::remove_cvref_t<ErrArg>>
struct _nothrow_or_else_to_optional<T, Fn, ErrArg, ValArg> {
  using _type = typename _or_else_to_optional<T, Fn, ErrArg>::type;
  static constexpr bool value
      = _is_nothrow_rts_applicable<_type, Fn, ErrArg>
        && (empty_copack<T> || ::std::is_nothrow_constructible_v<_type, ::std::in_place_t, ValArg>);
};
template <typename T, typename Fn, typename ErrArg, typename ValArg>
  requires(not _some_copack<::std::remove_cvref_t<ErrArg>>)
struct _nothrow_or_else_to_optional<T, Fn, ErrArg, ValArg> {
  using _result = ::std::remove_cvref_t<typename _apply_result<Fn, ErrArg>::type>;
  using _type = typename _or_else_to_optional<T, Fn, ErrArg>::type;
  static constexpr bool _convert
      = ::std::is_same_v<_result, _type>
        || (empty_copack<typename _result::value_type>
            || ::std::is_nothrow_constructible_v<_type, ::std::in_place_t, typename _result::value_type &&>);
  static constexpr bool value
      = _is_nothrow_applicable<Fn, ErrArg>::value && _convert
        && (empty_copack<T> || ::std::is_nothrow_constructible_v<_type, ::std::in_place_t, ValArg>);
};
} // namespace detail
/**
 * @brief Checks if the monadic type can be used with the `or_else` operation
 *
 * @tparam Fn The function to execute on the dead state
 * @tparam V The monadic type
 */
template <typename Fn, typename V>
concept applicable_or_else //
    = (some_expected<V> && requires(V &&v) {
        typename detail::_or_else_dispatch<typename ::std::remove_cvref_t<V>::value_type, Fn,
                                           decltype(FWD(v).error())>::type;
        requires same_value_kind<V, typename detail::_or_else_dispatch<typename ::std::remove_cvref_t<V>::value_type,
                                                                       Fn, decltype(FWD(v).error())>::type>;
      }) || (some_expected<V> //
         && some_copack<typename ::std::remove_cvref_t<V>::value_type> && requires(V &&v) {
        typename detail::_or_else_dispatch<typename ::std::remove_cvref_t<V>::value_type, Fn,
                                           decltype(FWD(v).error())>::type;
        requires some_expected<typename detail::_or_else_dispatch<typename ::std::remove_cvref_t<V>::value_type, Fn,
                                                                  decltype(FWD(v).error())>::type>;
      }) || (some_optional<V> && requires(Fn &&fn) {
        {
          ::fn::apply(FWD(fn))
        } -> same_value_kind<V>;
      }) || (some_optional<V>  //
         && some_copack<typename ::std::remove_cvref_t<V>::value_type> && requires(Fn &&fn, V &&v) {
        {
          ::fn::apply(FWD(fn))
        } -> some_optional;
      });

/**
 * @brief Checks if the callback can be used with `or_else` recovering across the expected/optional
 *        pair
 *
 * The callback witnesses the inhabited dead state - optional's empty state (nullary) or expected's
 * error (per alternative) - and its declared carrier names the result kind; self's value joins the
 * target's value side under the grade-sticky rules. An identity expected is excluded: the fold of
 * the callback over zero error alternatives has no inputs, so those keep the vacuous identity.
 *
 * @tparam Fn The function to execute on the dead state
 * @tparam V The monadic type
 */
template <typename Fn, typename V>
concept applicable_or_else_across //
    = (some_optional<V> && (not applicable_or_else<Fn, V>) && requires {
        typename detail::_or_else_to_expected<typename ::std::remove_cvref_t<V>::value_type, Fn>::type;
      } && (empty_copack<typename ::std::remove_cvref_t<V>::value_type> || requires(V &&v) {
         requires ::std::is_constructible_v<
             typename detail::_or_else_to_expected<typename ::std::remove_cvref_t<V>::value_type, Fn>::type,
             ::std::in_place_t, decltype(*FWD(v))>;
       })) || (some_expected<V> && (not some_identity<V>) && (not applicable_or_else<Fn, V>) && requires(V &&v) {
        typename detail::_or_else_to_optional<typename ::std::remove_cvref_t<V>::value_type, Fn,
                                              decltype(FWD(v).error())>::type;
      } && (empty_copack<typename ::std::remove_cvref_t<V>::value_type> || requires(V &&v) {
                 requires ::std::is_constructible_v<
                     typename detail::_or_else_to_optional<typename ::std::remove_cvref_t<V>::value_type, Fn,
                                                           decltype(FWD(v).error())>::type,
                     ::std::in_place_t, decltype(FWD(v).value())>;
               }));

/**
 * @brief Sequence a recovery computation on the failure path, joining its values
 *
 * The callback witnesses the dead state - `expected`'s error, per alternative when graded, or
 * `optional`'s empty state, with no arguments - and returns a carrier; a successful operand passes
 * through, bypassing it. Recovery branches may return heterogeneous carriers of the same value
 * kind: their values join, and an error alternative handled by a branch leaves the grade unless
 * re-returned. In the pipeline form the callback's carrier may also bridge across the
 * expected/optional pair; on the identity `expected` the operation is vacuous - nothing is asked
 * of the handler, not even that it be callable.
 *
 * Use through the `fn::or_else` nielbloid.
 */
constexpr inline struct or_else_t final {
  /**
   * @brief Sequence a recovery computation on the failure path, joining its values
   * @param fn The function to execute on the dead state - the error on `expected`, no arguments
   *        on `optional` - returning a carrier
   * @return A functor that will recover the monadic type
   */
  [[nodiscard]] constexpr auto operator()(auto &&fn) const noexcept(noexcept(functor<or_else_t, decltype(fn)>{FWD(fn)}))
      -> functor<or_else_t, decltype(fn)> //
  {
    return {FWD(fn)};
  }

  struct apply;
} or_else = {};

struct or_else_t::apply final {
  /**
   * @brief Recovers through the carrier's own `or_else` member
   *
   * @param v The monad
   * @param fn The function to execute on the dead state
   * @return A carrier of the same value kind
   */
  template <some_monadic_type V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const //
      noexcept(noexcept(FWD(v).or_else(FWD(fn))))               //
      -> same_value_kind<V &&> auto
    requires(not some_identity<V>) && applicable_or_else<Fn &&, V &&>
  {
    return FWD(v).or_else(FWD(fn));
  }

  // An identity expected's error side is uninhabited - delegate to the vacuous member, which
  // accepts any callback and never instantiates it
  template <some_expected V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const //
      noexcept(noexcept(FWD(v).or_else(FWD(fn))))               //
      -> same_value_kind<V &&> auto
    requires some_identity<V>
  {
    return FWD(v).or_else(FWD(fn));
  }

  // The cross arms: the callback witnesses the inhabited dead state and names the target carrier -
  // or_else invokes on 1-states and is silent on 0-states, which is why an identity expected keeps
  // the vacuous arm above. Engine-direct: the members are each carrier's own recovery, and the
  // verb layer is the licensed cross-carrier place.
  template <some_optional V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const //
      noexcept(detail::_nothrow_or_else_to_expected<typename ::std::remove_cvref_t<V>::value_type, Fn &&,
                                                    decltype(*::std::declval<V>())>::value) //
      -> some_expected auto
    requires applicable_or_else_across<Fn &&, V &&>
  {
    using T = typename ::std::remove_cvref_t<V>::value_type;
    using type = typename detail::_or_else_to_expected<T, Fn &&>::type;
    if constexpr (not empty_copack<T>)
      if (v.has_value())
        return type{::std::in_place, *FWD(v)};
    using result = ::std::remove_cvref_t<typename detail::_apply_result<Fn &&>::type>;
    if constexpr (::std::is_same_v<result, type>)
      return ::fn::detail::_apply(FWD(fn));
    else {
      auto t = ::fn::detail::_apply(FWD(fn));
      if (t.has_value()) {
        if constexpr (not empty_copack<typename result::value_type>)
          return type{::std::in_place, ::std::move(t).value()};
        else
          ::pfn::unreachable(); // LCOV_EXCL_LINE
      } else
        return type{::fn::unexpect, ::std::move(t).error()};
    }
  }

  template <some_expected V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const //
      noexcept(detail::_nothrow_or_else_to_optional<typename ::std::remove_cvref_t<V>::value_type, Fn &&,
                                                    decltype(::std::declval<V>().error()),
                                                    decltype(::std::declval<V>().value())>::value) //
      -> some_optional auto
    requires applicable_or_else_across<Fn &&, V &&>
  {
    using T = typename ::std::remove_cvref_t<V>::value_type;
    using type = typename detail::_or_else_to_optional<T, Fn &&, decltype(FWD(v).error())>::type;
    if constexpr (not empty_copack<T>)
      if (v.has_value())
        return type{::std::in_place, FWD(v).value()};
    if constexpr (detail::_some_copack<::std::remove_cvref_t<decltype(v.error())>>)
      return ::fn::detail::_tagged_join_apply<detail::_joining_optional_recovery_tag<::fn::optional, T>>(FWD(v).error(),
                                                                                                         FWD(fn));
    else {
      using result = ::std::remove_cvref_t<typename detail::_apply_result<Fn &&, decltype(FWD(v).error())>::type>;
      if constexpr (::std::is_same_v<result, type>)
        return ::fn::detail::_apply(FWD(fn), FWD(v).error());
      else {
        auto t = ::fn::detail::_apply(FWD(fn), FWD(v).error());
        if (t.has_value()) {
          if constexpr (not empty_copack<typename result::value_type>)
            return type{::std::in_place, ::std::move(t).value()};
          else
            ::pfn::unreachable(); // LCOV_EXCL_LINE
        } else
          return type{::std::nullopt};
      }
    }
  }
};

} // namespace LIBFN_VERSION
} // namespace fn

#include <fn/detail/macro_end.hpp>

#endif // INCLUDE_FN_OR_ELSE
