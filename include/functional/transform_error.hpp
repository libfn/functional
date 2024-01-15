// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_TRANSFORM_ERROR
#define INCLUDE_FUNCTIONAL_TRANSFORM_ERROR

#include "functional/detail/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"

#include <concepts>
#include <utility>

namespace fn {

constexpr inline struct transform_error_t final {
  auto operator()(auto &&fn) const noexcept
      -> functor<transform_error_t, decltype(fn)>
  {
    return {std::forward<decltype(fn)>(fn)};
  }

  struct apply;
} transform_error = {};

struct transform_error_t::apply final {
  // Not supported by optional since there's no error type
  static auto operator()(some_expected auto &&v, auto &&fn) noexcept
      -> decltype(auto)
    requires std::invocable<decltype(fn),
                            decltype(std::forward<decltype(v)>(v).error())>
  {
    return std::forward<decltype(v)>(v).transform_error(
        std::forward<decltype(fn)>(fn));
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_TRANSFORM_ERROR
