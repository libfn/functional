// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_OR_ELSE
#define INCLUDE_FUNCTIONAL_OR_ELSE

#include <fn/concepts.hpp>
#include <fn/functor.hpp>

#include <type_traits>

namespace fn {
/**
 * @brief TODO
 *
 * @tparam V TODO
 * @tparam Args TODO
 */
template <typename V, typename... Args>
concept invocable_value_or //
    = (some_expected_non_void<V> && std::is_constructible_v<typename std::remove_cvref_t<V>::value_type, Args...>)
      || (some_optional<V> && std::is_constructible_v<typename std::remove_cvref_t<V>::value_type, Args...>);

/**
 * @brief TODO
 */
constexpr inline struct value_or_t final {
  /**
   * @brief TODO
   *
   * @param args TODO
   * @return TODO
   */
  [[nodiscard]] constexpr auto operator()(auto &&...args) const noexcept -> functor<value_or_t, decltype(args)...> //
  {
    return {FWD(args)...};
  }

  struct apply;
} value_or = {};

/**
 * @brief TODO
 */
struct value_or_t::apply final {
  /**
   * @brief TODO
   *
   * @param v TODO
   * @param args TODO
   * @return TODO
   */
  [[nodiscard]] static constexpr auto operator()(some_monadic_type auto &&v, auto &&...args) noexcept //
      -> same_value_kind<decltype(v)> auto
    requires invocable_value_or<decltype(v), decltype(args)...>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    return FWD(v).or_else([&](auto &&...) -> type { return type{std::in_place, args...}; });
  }

  // No support for choice since there's no error to recover from
  static auto operator()(some_choice auto &&v, auto &&...args) noexcept = delete;
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_OR_ELSE
