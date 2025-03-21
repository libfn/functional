// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_OR_ELSE
#define INCLUDE_FUNCTIONAL_OR_ELSE

#include <fn/concepts.hpp>
#include <fn/functional.hpp>
#include <fn/functor.hpp>

namespace fn {
/**
 * @brief TODO
 *
 * @tparam Fn TODO
 * @tparam V TODO
 */
template <typename Fn, typename V>
concept invocable_or_else //
    = (some_expected<V> && requires(Fn &&fn, V &&v) {
        {
          ::fn::invoke(FWD(fn), FWD(v).error())
        } -> same_value_kind<V>;
      }) || (some_expected<V> //
         && some_sum<typename std::remove_cvref_t<V>::value_type> && requires(Fn &&fn, V &&v) {
        {
          ::fn::invoke(FWD(fn), FWD(v).error())
        } -> some_expected;
      }) || (some_optional<V> && requires(Fn &&fn) {
        {
          ::fn::invoke(FWD(fn))
        } -> same_value_kind<V>;
      }) || (some_optional<V>  //
         && some_sum<typename std::remove_cvref_t<V>::value_type> && requires(Fn &&fn, V &&v) {
        {
          ::fn::invoke(FWD(fn))
        } -> some_optional;
      });

/**
 * @brief TODO
 */
constexpr inline struct or_else_t final {
  /**
   * @brief TODO
   *
   * @param fn TODO
   * @return TODO
   */
  [[nodiscard]] constexpr auto operator()(auto &&fn) const noexcept -> functor<or_else_t, decltype(fn)> //
  {
    return {FWD(fn)};
  }

  struct apply;
} or_else = {};

/**
 * @brief TODO
 */
struct or_else_t::apply final {
  /**
   * @brief TODO
   *
   * @param v TODO
   * @param fn TODO
   * @return TODO
   */
  [[nodiscard]] static constexpr auto operator()(some_monadic_type auto &&v, auto &&fn) noexcept //
      -> same_value_kind<decltype(v)> auto
    requires invocable_or_else<decltype(fn), decltype(v)>
  {
    return FWD(v).or_else(FWD(fn));
  }

  // No support for choice since there's no error to recover from
  static auto operator()(some_choice auto &&v, auto &&...args) noexcept = delete;
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_OR_ELSE
