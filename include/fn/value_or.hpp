// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_VALUE_OR
#define INCLUDE_FN_VALUE_OR

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
    = (some_expected_non_void<V> && ::std::is_constructible_v<typename ::std::remove_cvref_t<V>::value_type, Args...>)
      || (some_optional<V> && ::std::is_constructible_v<typename ::std::remove_cvref_t<V>::value_type, Args...>);

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
  template <typename... Args>
  [[nodiscard]] constexpr auto operator()(Args &&...args) const
      noexcept(noexcept(functor<value_or_t, Args &&...>{FWD(args)...})) -> functor<value_or_t, Args &&...> //
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
  // The fallback is built inside, so its construction is weighed here rather than propagated: the
  // callable or_else receives is a lambda, which cannot be named in this specification.
  template <some_monadic_type V, typename... Args>
  [[nodiscard]] constexpr auto operator()(V &&v, Args &&...args) const //
      noexcept(::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, ::std::in_place_t, Args...>
               && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, V>) -> ::std::remove_cvref_t<V>
    requires invocable_value_or<V &&, Args...>
  {
    using type = ::std::remove_cvref_t<V>;
    return FWD(v).or_else([&args...](auto &&...) -> type { return type{::std::in_place, FWD(args)...}; });
  }

  // No support for choice since there's no error to recover from
  auto operator()(some_choice auto &&v, auto &&...args) const noexcept = delete;
};

} // namespace fn

#endif // INCLUDE_FN_VALUE_OR
