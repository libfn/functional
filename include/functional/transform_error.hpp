// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_TRANSFORM_ERROR
#define INCLUDE_FUNCTIONAL_TRANSFORM_ERROR

#include "functional/detail/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"

#include <type_traits>
#include <utility>

namespace fn {

constexpr static struct transform_error_t final {
  auto operator()(auto &&fn) const noexcept
      -> functor<transform_error_t, decltype(fn)>
  {
    return {std::forward<decltype(fn)>(fn)};
  }
} transform_error = {};

// Not supported by optional since there's no error type
auto monadic_apply(some_expected auto &&v, transform_error_t,
                   auto &&fn) noexcept -> decltype(auto)
  requires requires { fn(v.error()); }
{
  return std::forward<decltype(v)>(v).transform_error(
      std::forward<decltype(fn)>(fn));
}

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_TRANSFORM_ERROR
