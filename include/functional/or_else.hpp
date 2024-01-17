// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_OR_ELSE
#define INCLUDE_FUNCTIONAL_OR_ELSE

#include "functional/detail/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"

#include <concepts>
#include <utility>

namespace fn {

constexpr inline struct or_else_t final {
  auto operator()(auto &&fn) const noexcept -> functor<or_else_t, decltype(fn)>
  {
    return {std::forward<decltype(fn)>(fn)};
  }

  struct apply;
} or_else = {};

struct or_else_t::apply final {
  static auto operator()(some_expected auto &&v, auto &&fn) noexcept
      -> decltype(auto)
    requires std::invocable<decltype(fn),
                            decltype(std::forward<decltype(v)>(v).error())>
  {
    return std::forward<decltype(v)>(v).or_else(std::forward<decltype(fn)>(fn));
  }

  static auto operator()(some_optional auto &&v, auto &&fn) noexcept
      -> decltype(auto)
    requires std::invocable<decltype(fn)>
  {
    return std::forward<decltype(v)>(v).or_else(std::forward<decltype(fn)>(fn));
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_OR_ELSE
