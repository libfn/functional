// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_RECOVER
#define INCLUDE_FUNCTIONAL_RECOVER

#include <fn/functional.hpp>
#include <fn/functor.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

namespace fn {
/**
 * @brief TODO
 *
 * @tparam Fn TODO
 * @tparam V TODO
 */
template <typename Fn, typename V>
concept invocable_recover //
    = (some_expected_non_void<V> && requires(Fn &&fn, V &&v) {
        { ::fn::invoke(FWD(fn), FWD(v).error()) } -> std::convertible_to<typename std::remove_cvref_t<V>::value_type>;
      }) || (some_expected_void<V> && requires(Fn &&fn, V &&v) {
        { ::fn::invoke(FWD(fn), FWD(v).error()) } -> std::same_as<void>;
      }) || (some_optional<V> && requires(Fn &&fn, V &&v) {
        { ::fn::invoke(FWD(fn)) } -> std::convertible_to<typename std::remove_cvref_t<V>::value_type>;
      });

/**
 * @brief TODO
 */
constexpr inline struct recover_t final {
  /**
   * @brief TODO
   *
   * @param fn TODO
   * @return TODO
   */
  [[nodiscard]] constexpr auto operator()(auto &&fn) const noexcept -> functor<recover_t, decltype(fn)> //
  {
    return {FWD(fn)};
  }

  struct apply;
} recover = {};

/**
 * @brief TODO
 */
struct recover_t::apply final {
  /**
   * @brief TODO
   *
   * @param v TODO
   * @param fn TODO
   * @return TODO
   */
  [[nodiscard]] static constexpr auto operator()(some_expected_non_void auto &&v,
                                                 auto &&fn) noexcept -> same_monadic_type_as<decltype(v)> auto
    requires invocable_recover<decltype(fn), decltype(v)>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    if (v.has_value()) {
      return type{std::in_place, FWD(v).value()};
    }
    return type{std::in_place, ::fn::invoke(FWD(fn), FWD(v).error())};
  }

  /**
   * @brief TODO
   *
   * @param v TODO
   * @param fn TODO
   * @return TODO
   */
  [[nodiscard]] static constexpr auto operator()(some_expected_void auto &&v,
                                                 auto &&fn) noexcept -> same_monadic_type_as<decltype(v)> auto
    requires invocable_recover<decltype(fn), decltype(v)>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    if (v.has_value()) {
      return type{std::in_place};
    }
    ::fn::invoke(FWD(fn), FWD(v).error()); // side-effects only
    return type{std::in_place};
  }

  /**
   * @brief TODO
   *
   * @param v TODO
   * @param fn TODO
   * @return TODO
   */
  [[nodiscard]] static constexpr auto operator()(some_optional auto &&v,
                                                 auto &&fn) noexcept -> same_monadic_type_as<decltype(v)> auto
    requires invocable_recover<decltype(fn), decltype(v)>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    if (v.has_value()) {
      return type{std::in_place, FWD(v).value()};
    }
    return type{std::in_place, ::fn::invoke(FWD(fn))};
  }

  // No support for choice since there's no error to recover from
  static auto operator()(some_choice auto &&v, auto &&...args) noexcept = delete;
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_RECOVER
