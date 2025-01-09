// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_INSPECT_ERROR
#define INCLUDE_FUNCTIONAL_INSPECT_ERROR

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
concept invocable_inspect_error //
    = (some_expected<V> && requires(Fn &&fn, V &&v) {
        { ::fn::invoke(FWD(fn), std::as_const(v).error()) } -> std::same_as<void>;
      }) || (some_optional<V> && requires(Fn &&fn) {
        { ::fn::invoke(FWD(fn)) } -> std::same_as<void>;
      });

/**
 * @brief TODO
 */
constexpr inline struct inspect_error_t final {
  /**
   * @brief TODO
   *
   * @param fn TODO
   * @return TODO
   */
  [[nodiscard]] constexpr auto operator()(auto &&fn) const noexcept -> functor<inspect_error_t, decltype(fn)> //
  {
    return {FWD(fn)};
  }

  struct apply;
} inspect_error = {};

/**
 * @brief TODO
 */
struct inspect_error_t::apply final {
  /**
   * @brief TODO
   *
   * @param v TODO
   * @param fn TODO
   * @return TODO
   */
  [[nodiscard]] static constexpr auto operator()(some_expected auto &&v, auto &&fn) noexcept -> decltype(v)
    requires invocable_inspect_error<decltype(fn), decltype(v)>
  {
    if (not v.has_value()) {
      ::fn::invoke(FWD(fn), std::as_const(v).error()); // side-effects only
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
    requires invocable_inspect_error<decltype(fn), decltype(v)>
  {
    if (not v.has_value()) {
      ::fn::invoke(FWD(fn)); // side-effects only
    }
    return FWD(v);
  }

  // No support for choice since there's no error to operate on
  static auto operator()(some_choice auto &&v, auto &&...args) noexcept = delete;
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_INSPECT_ERROR
