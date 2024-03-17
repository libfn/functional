// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_AND_THEN
#define INCLUDE_FUNCTIONAL_AND_THEN

#include "functional/choice.hpp"
#include "functional/concepts.hpp"
#include "functional/expected.hpp"
#include "functional/functional.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"
#include "functional/optional.hpp"
#include "functional/utility.hpp"

#include <concepts>
#include <utility>

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
      }) || (some_expected_void<V> && requires(Fn &&fn) {
        {
          ::fn::invoke(FWD(fn))
        } -> same_kind<V>;
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
  [[nodiscard]] static constexpr auto operator()(some_monadic_type auto &&v, auto &&fn) noexcept
      -> same_kind<decltype(v)> auto
    requires invocable_and_then<decltype(fn), decltype(v)>
  {
    return FWD(v).and_then(FWD(fn));
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_AND_THEN
