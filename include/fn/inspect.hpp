// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_INSPECT
#define INCLUDE_FN_INSPECT

#include <fn/expected.hpp>
#include <fn/functional.hpp>
#include <fn/functor.hpp>

#include <concepts>
#include <utility>

namespace fn {
/**
 * @brief TODO
 *
 * @tparam Fn TODO
 * @tparam V TODO
 */
template <typename Fn, typename V>
concept invocable_inspect //
    = (some_expected_non_void<V> && requires(Fn &&fn, V &&v) {
        { ::fn::invoke(FWD(fn), std::as_const(v).value()) } -> std::same_as<void>;
      }) || (some_expected_void<V> && requires(Fn &&fn) {
        { ::fn::invoke(FWD(fn)) } -> std::same_as<void>;
      }) || (some_optional<V> && requires(Fn &&fn, V &&v) {
        { ::fn::invoke(FWD(fn), std::as_const(v).value()) } -> std::same_as<void>;
      }) || (some_choice<V> && requires(Fn &&fn, V &&v) {
        { ::fn::invoke(FWD(fn), std::as_const(v).value()) } -> std::same_as<void>;
      });

/**
 * @brief TODO
 */
constexpr inline struct inspect_t final {
  /**
   * @brief TODO
   *
   * @param fn TODO
   * @return TODO
   */
  [[nodiscard]] constexpr auto operator()(auto &&fn) const noexcept -> functor<inspect_t, decltype(fn)>
  {
    return {FWD(fn)};
  }

  struct apply;
} inspect = {};

struct inspect_t::apply final {
  /**
   * @brief TODO
   *
   * @param v TODO
   * @param fn TODO
   * @return TODO
   */
  [[nodiscard]] static constexpr auto operator()(some_expected_non_void auto &&v, auto &&fn) noexcept -> decltype(v)
    requires invocable_inspect<decltype(fn), decltype(v)>
  {
    if (v.has_value()) {
      ::fn::invoke(FWD(fn), std::as_const(v).value()); // side-effects only
    }
    return FWD(v);
  }

  /**
   * @brief TODO
   *
   * @param v TODO
   * @param fn TODO
   * @return TODO
   */
  [[nodiscard]] static constexpr auto operator()(some_expected_void auto &&v, auto &&fn) noexcept -> decltype(v)
    requires invocable_inspect<decltype(fn), decltype(v)>
  {
    if (v.has_value()) {
      ::fn::invoke(FWD(fn)); // side-effects only
    }
    return FWD(v);
  }

  /**
   * @brief TODO
   *
   * @param v TODO
   * @param fn TODO
   * @return TODO
   */
  [[nodiscard]] static constexpr auto operator()(some_optional auto &&v, auto &&fn) noexcept -> decltype(v)
    requires invocable_inspect<decltype(fn), decltype(v)>
  {
    if (v.has_value()) {
      ::fn::invoke(FWD(fn), std::as_const(v).value()); // side-effects only
    }
    return FWD(v);
  }

  /**
   * @brief TODO
   *
   * @param v TODO
   * @param fn TODO
   * @return TODO
   */
  [[nodiscard]] static constexpr auto operator()(some_choice auto &&v, auto &&fn) noexcept -> decltype(v)
    requires invocable_inspect<decltype(fn), decltype(v)>
  {
    ::fn::invoke(FWD(fn), std::as_const(v).value()); // side-effects only
    return FWD(v);
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_INSPECT
