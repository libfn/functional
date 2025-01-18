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

#include <type_traits>

namespace fn {
/**
 * @brief Checks if the monadic type can be used with the `and_then` operation
 *
 * @tparam Fn The function to execute on the value
 * @tparam V The monadic type
 */
template <typename Fn, typename V>
concept invocable_and_then //
    = (some_expected_non_void<V> && requires(Fn &&fn, V &&v) {
        {
          ::fn::invoke(FWD(fn), FWD(v).value())
        } -> same_kind<V>;
      }) || (some_expected_non_void<V> //
         && some_sum<typename std::remove_cvref_t<V>::error_type> && requires(Fn &&fn, V &&v) {
        {
          ::fn::invoke(FWD(fn), FWD(v).value())
        } -> some_expected;
      }) || (some_expected_void<V> && requires(Fn &&fn) {
        {
          ::fn::invoke(FWD(fn))
        } -> same_kind<V>;
      }) || (some_expected_void<V> //
         && some_sum<typename std::remove_cvref_t<V>::error_type> && requires(Fn &&fn, V &&v) {
        {
          ::fn::invoke(FWD(fn))
        } -> some_expected;
      }) || (some_optional<V> && requires(Fn &&fn, V &&v) {
        {
          ::fn::invoke(FWD(fn), FWD(v).value())
        } -> same_kind<V>;
      }) || (some_choice<V> && requires(Fn &&fn, V &&v) {
        {
          ::fn::invoke(FWD(fn), FWD(v).value())
        } -> same_kind<V>;
      });

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
  [[nodiscard]] constexpr auto operator()(auto &&fn) const noexcept -> functor<and_then_t, decltype(fn)> //
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
  [[nodiscard]] static constexpr auto operator()(some_monadic_type auto &&v, auto &&fn) noexcept //
      -> same_kind<decltype(v)> auto
    requires invocable_and_then<decltype(fn), decltype(v)>
  {
    return FWD(v).and_then(FWD(fn));
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_AND_THEN
