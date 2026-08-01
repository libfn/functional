// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_FAIL
#define INCLUDE_FN_FAIL

#include <fn/concepts.hpp>
#include <fn/functor.hpp>
#include <fn/optional.hpp>
#include <libfn_version.hpp>

#include <concepts>
#include <type_traits>

#include <fn/detail/macro_begin.hpp>

namespace fn {
inline namespace LIBFN_VERSION {
/**
 * @brief Checks if the monadic type can be used with the `fail` operation
 *
 * @tparam Fn The function to map the value into the error, or to observe it on `optional`
 * @tparam V The monadic type
 */
template <typename Fn, typename V>
concept applicable_fail //
    = (some_expected_non_void<V> && requires(Fn &&fn, V &&v) {
        {
          ::fn::apply(FWD(fn), FWD(v).value())
        } -> ::std::convertible_to<typename ::std::remove_cvref_t<V>::error_type>;
        requires detail::_relocatable_error<V>; // the error branch carries the existing error over
      }) || (some_expected_void<V> && requires(Fn &&fn) {
        { ::fn::apply(FWD(fn)) } -> ::std::convertible_to<typename ::std::remove_cvref_t<V>::error_type>;
        requires detail::_relocatable_error<V>;
      }) || (some_optional<V> && requires(Fn &&fn, V &&v) {
        { ::fn::apply(FWD(fn), FWD(v).value()) } -> ::std::same_as<void>;
      });

/**
 * @brief Intercept the success value and force a transition to the failure state
 *
 * The dual of `recover`. On `expected` the callback maps the value into the operand's existing
 * error type - `fail` never widens a graded error set - and an operand already holding an error
 * carries it over. On `optional` the callback observes the value, and must return `void`; the
 * result is empty. Rejected on the identity carriers, which have no failure state to enter; over
 * an uninhabited value side the operand passes through and the callback is neither invoked nor
 * instantiated.
 *
 * Use through the `fn::fail` nielbloid.
 */
constexpr inline struct fail_t final {
  /**
   * @brief Intercept the success value and force a transition to the failure state
   * @param fn On `expected`, the function to map the value into the existing error type; on
   *        `optional`, the function to observe the value, returning `void`
   * @return A functor that will fail the monadic type
   */
  [[nodiscard]] constexpr auto operator()(auto &&fn) const noexcept(noexcept(functor<fail_t, decltype(fn)>{FWD(fn)}))
      -> functor<fail_t, decltype(fn)> //
  {
    return {FWD(fn)};
  }

  struct apply;
} fail = {}; ///< Fails a value the predicate selects: `x | fail(f)`

struct fail_t::apply final {
  /**
   * @brief Fails the operand: a value maps into an error, an existing error carries over
   *
   * @param v The monad
   * @param fn The function to map the value into the error
   * @return An `expected` of the same type, holding an error
   */
  template <some_expected_non_void V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const //
      noexcept(
          ::fn::is_nothrow_applicable_v<Fn, decltype(FWD(v).value())>
          && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, ::fn::unexpect_t,
                                               ::fn::apply_result_t<Fn, decltype(FWD(v).value())>>
          && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, ::fn::unexpect_t, decltype(FWD(v).error())>)
          -> ::std::remove_cvref_t<V>
      // An identity expected cannot fail - there is no error to fail into - so the refusal is
      // stated, not left to the impossible conversion
    requires(not some_identity<V>) && applicable_fail<Fn &&, V &&>
  {
    using type = ::std::remove_cvref_t<V>;
    if (v.has_value()) {
      return type{::fn::unexpect, ::fn::apply(FWD(fn), FWD(v).value())};
    }
    return type{::fn::unexpect, FWD(v).error()};
  }

  template <some_expected_void V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const //
      noexcept(
          ::fn::is_nothrow_applicable_v<Fn>
          && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, ::fn::unexpect_t, ::fn::apply_result_t<Fn>>
          && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, ::fn::unexpect_t, decltype(FWD(v).error())>)
          -> ::std::remove_cvref_t<V>
    requires(not some_identity<V>) && applicable_fail<Fn &&, V &&>
  {
    using type = ::std::remove_cvref_t<V>;
    if (v.has_value()) {
      return type{::fn::unexpect, ::fn::apply(FWD(fn))};
    }
    return type{::fn::unexpect, FWD(v).error()};
  }

  /**
   * @brief Fails the operand: the value is observed, the result is empty
   *
   * @param v The optional
   * @param fn The function to observe the value; must return `void`
   * @return An `optional` of the same type, empty
   */
  template <some_optional V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const //
      noexcept(::fn::is_nothrow_applicable_v<Fn, decltype(FWD(v).value())>
               && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, ::std::nullopt_t>)
          -> ::std::remove_cvref_t<V>
    requires applicable_fail<Fn &&, V &&>
  {
    using type = ::std::remove_cvref_t<V>;
    if (v.has_value()) {
      ::fn::apply(FWD(fn), FWD(v).value());
    }
    return type{::std::nullopt};
  }

  // An uninhabited value side never holds a value to fail on, so the failure can never fire: the
  // operand passes through, and the callback is neither invoked nor instantiated. Unlike an identity
  // carrier, which is refused above, such an operand does have somewhere to fail into - it is
  // already there.
  template <some_monadic_type V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&) const
      noexcept(::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, V>) -> ::std::remove_cvref_t<V>
    requires some_empty_value<V> && detail::_relocatable<V>
  {
    return FWD(v);
  }
};

} // namespace LIBFN_VERSION
} // namespace fn

#include <fn/detail/macro_end.hpp>

#endif // INCLUDE_FN_FAIL
