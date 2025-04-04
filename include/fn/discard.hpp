// Copyright (c) 2025 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_DISCARD
#define INCLUDE_FUNCTIONAL_DISCARD

#include <fn/concepts.hpp>
#include <fn/functor.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

namespace fn {
/**
 * @brief Discard the value explicitly
 *
 * This is useful when only side-effects are desired and the final value is not needed.
 *
 * Use through the `fn::discard` nielbloid.
 */
constexpr inline struct discard_t final {
  /**
   * @brief Unconditionally discards the value
   * @return A functor that explicitly discards the value
   */
  [[nodiscard]] constexpr auto operator()() const noexcept -> functor<discard_t> { return {}; }

  struct apply final {
    static constexpr auto operator()(some_monadic_type auto &&) noexcept -> void {}
  };
} discard = {};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_DISCARD
