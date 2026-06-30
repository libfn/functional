// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_TRANSFORM_ERROR
#define INCLUDE_FN_TRANSFORM_ERROR

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
concept invocable_transform_error //
    = (some_expected<V> && (not some_sum<typename ::std::remove_cvref_t<V>::error_type>) && requires(Fn &&fn, V &&v) {
        { ::fn::invoke(FWD(fn), FWD(v).error()) } -> convertible_to_unexpected;
      }) || (some_expected<V> && some_sum<typename ::std::remove_cvref_t<V>::error_type> && requires(Fn &&fn, V &&v) {
        {
          FWD(v).error().transform(FWD(fn))
        } -> convertible_to_expected<typename ::std::remove_cvref_t<decltype(v)>::error_type>;
      });

/**
 * @brief TODO
 */
constexpr inline struct transform_error_t final {
  /**
   * @brief TODO
   *
   * @param fn TODO
   * @return TODO
   */
  [[nodiscard]] constexpr auto operator()(auto &&fn) const noexcept -> functor<transform_error_t, decltype(fn)> //
  {
    return {FWD(fn)};
  }

  struct apply;
} transform_error = {};

/**
 * @brief TODO
 */
struct transform_error_t::apply final {
  /**
   * @brief TODO
   *
   * @param v TODO
   * @param fn TODO
   * @return TODO
   */
  template <some_expected V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const noexcept -> same_value_kind<V &&> auto
    requires invocable_transform_error<Fn &&, V &&>
  {
    return FWD(v).transform_error(FWD(fn));
  }

  // No support for optional since there's no error state to operate on
  auto operator()(some_optional auto &&v, auto &&...args) const noexcept = delete;

  // No support for choice since there's no error to operate on
  auto operator()(some_choice auto &&v, auto &&...args) const noexcept = delete;
};

} // namespace fn

#endif // INCLUDE_FN_TRANSFORM_ERROR
