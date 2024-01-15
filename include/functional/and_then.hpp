// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_AND_THEN
#define INCLUDE_FUNCTIONAL_AND_THEN

#include "functional/detail/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"

#include <concepts>
#include <utility>

namespace fn {

constexpr inline struct and_then_t final {
  auto operator()(auto &&fn) const noexcept -> functor<and_then_t, decltype(fn)>
  {
    return {std::forward<decltype(fn)>(fn)};
  }
} and_then = {};

auto monadic_apply(some_monadic_type auto &&v, and_then_t, auto &&fn) noexcept
    -> decltype(auto)
  requires std::invocable<decltype(fn),
                          decltype(std::forward<decltype(v)>(v).value())>
{
  return std::forward<decltype(v)>(v).and_then(std::forward<decltype(fn)>(fn));
}

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_AND_THEN
