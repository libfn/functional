// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_TRANSFORM
#define INCLUDE_FUNCTIONAL_TRANSFORM

#include "functional/detail/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"

#include <type_traits>
#include <utility>

namespace fn {

constexpr static struct transform_t final {
  auto operator()(auto &&fn) const noexcept
      -> functor<transform_t, decltype(fn)>
  {
    return {std::forward<decltype(fn)>(fn)};
  }
} transform = {};

auto monadic_apply(some_monadic_type auto &&v, transform_t, auto &&fn) noexcept
    -> decltype(auto)
  requires requires { fn(v.value()); }
{
  return std::forward<decltype(v)>(v).transform(std::forward<decltype(fn)>(fn));
}

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_TRANSFORM
