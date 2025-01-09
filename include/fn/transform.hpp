// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_TRANSFORM
#define INCLUDE_FUNCTIONAL_TRANSFORM

#include <fn/choice.hpp>
#include <fn/concepts.hpp>
#include <fn/expected.hpp>
#include <fn/functional.hpp>
#include <fn/functor.hpp>
#include <fn/optional.hpp>
#include <fn/sum.hpp>

#include <type_traits>

namespace fn {
/**
 * @brief TODO
 *
 * @tparam Fn TODO
 * @tparam V TODO
 */
template <typename Fn, typename V>
concept invocable_transform //
    = (some_expected_non_void<V>//
           && (not some_sum<typename std::remove_cvref_t<V>::value_type>) && requires(Fn &&fn, V &&v) {
        {
          ::fn::invoke(FWD(fn), FWD(v).value())
        } -> convertible_to_expected<typename std::remove_cvref_t<decltype(v)>::error_type>;
      }) || (some_expected<V> && some_sum<typename std::remove_cvref_t<V>::value_type> && requires(Fn &&fn, V &&v) {
        {
          FWD(v).value().transform(FWD(fn))
        } -> convertible_to_expected<typename std::remove_cvref_t<decltype(v)>::error_type>;
      }) || (some_expected_void<V> && requires(Fn &&fn, V &&v) {
        {
          ::fn::invoke(FWD(fn))
        } -> convertible_to_expected<typename std::remove_cvref_t<decltype(v)>::error_type>;
      }) || (some_optional<V> //
            && (not some_sum<typename std::remove_cvref_t<V>::value_type>) && requires(Fn &&fn, V &&v) {
        {
          ::fn::invoke(FWD(fn), FWD(v).value())
        } -> convertible_to_optional;
      }) || (some_optional<V> && some_sum<typename std::remove_cvref_t<V>::value_type> && requires(Fn &&fn, V &&v) {
        {
          FWD(v).value().transform(FWD(fn))
        } -> convertible_to_optional;
      }) || (some_choice<V> && requires(Fn &&fn, V &&v) {
        {
          FWD(v).transform(FWD(fn))
        } -> convertible_to_choice;
      });

/**
 * @brief TODO
 */
constexpr inline struct transform_t final {
  /**
   * @brief TODO
   *
   * @param fn TODO
   * @return TODO
   */
  [[nodiscard]] constexpr auto operator()(auto &&fn) const noexcept -> functor<transform_t, decltype(fn)> //
  {
    return {FWD(fn)};
  }

  struct apply;
} transform = {};

/**
 * @brief TODO
 */
struct transform_t::apply final {
  /**
   * @brief TODO
   *
   * @param v TODO
   * @param fn TODO
   * @return TODO
   */
  [[nodiscard]] static constexpr auto operator()(some_monadic_type auto &&v,
                                                 auto &&fn) noexcept -> same_kind<decltype(v)> auto
    requires invocable_transform<decltype(fn), decltype(v)>
  {
    return FWD(v).transform(FWD(fn));
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_TRANSFORM
