// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_RECOVER
#define INCLUDE_FUNCTIONAL_RECOVER

#include "functional/detail/concepts.hpp"
#include "functional/detail/traits.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace fn {

constexpr static struct recover_t final {
  auto operator()(auto &&fn) const noexcept -> functor<recover_t, decltype(fn)>
  {
    return {std::forward<decltype(fn)>(fn)};
  }
} recover = {};

auto monadic_apply(some_expected auto &&v, recover_t, auto &&fn) noexcept
    -> decltype(auto)
  requires requires { fn(v.error()); } && (!std::is_void_v<decltype(v.value())>)
{
  using error_type = std::remove_cvref_t<decltype(v)>::error_type;
  using value_type = detail::as_value_t<decltype(fn(v.error()))>;
  static_assert(
      std::is_convertible_v<
          typename std::remove_cvref_t<decltype(v)>::value_type, value_type>);
  using type = std::expected<value_type, error_type>;
  return std::forward<decltype(v)>(v).or_else([&fn](
                                                  auto &&arg) noexcept -> type {
    return {std::forward<decltype(fn)>(fn)(std::forward<decltype(arg)>(arg))};
  });
}

auto monadic_apply(some_expected auto &&v, recover_t, auto &&fn) noexcept
    -> decltype(auto)
  requires requires { fn(v.error()); } && (std::is_void_v<decltype(v.value())>)
{
  using error_type = std::remove_cvref_t<decltype(v)>::error_type;
  using value_type = detail::as_value_t<decltype(fn(v.error()))>;
  static_assert(std::is_default_constructible_v<value_type>);
  using type = std::expected<value_type, error_type>;
  return std::forward<decltype(v)>(v).or_else([&fn](
                                                  auto &&arg) noexcept -> type {
    return {std::forward<decltype(fn)>(fn)(std::forward<decltype(arg)>(arg))};
  });
}

auto monadic_apply(some_optional auto &&v, recover_t, auto &&fn) noexcept
    -> decltype(auto)
  requires requires { fn(); }
{
  using value_type = detail::as_value_t<decltype(fn())>;
  // NOTE optional::or_else(...) does not do type conversions
  static_assert(
      std::is_same_v<value_type,
                     typename std::remove_cvref_t<decltype(v)>::value_type>);
  using type = std::optional<value_type>;
  return std::forward<decltype(v)>(v).or_else(
      [&fn]() noexcept -> type { return {std::forward<decltype(fn)>(fn)()}; });
}

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_RECOVER
