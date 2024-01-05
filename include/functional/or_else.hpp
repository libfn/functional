// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_OR_ELSE
#define INCLUDE_FUNCTIONAL_OR_ELSE

#include "functional/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"

#include <type_traits>
#include <utility>

namespace fn {

struct or_else_t final {
  auto operator()(auto &&fn) const noexcept
      -> functor<or_else_t, std::decay_t<decltype(fn)>>
  {
    using functor_type = functor<or_else_t, std::decay_t<decltype(fn)>>;
    return functor_type{{std::forward<decltype(fn)>(fn)}};
  }
};
constexpr static or_else_t or_else = {};

auto monadic_apply(some_monadic_type auto &&v, or_else_t const &,
                   some_tuple auto &&fn) noexcept -> auto
  requires(std::tuple_size_v<std::decay_t<decltype(fn)>> == 1)
{
  return std::forward<decltype(v)>(v).or_else(
      std::get<0>(std::forward<decltype(fn)>(fn)));
}

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_OR_ELSE
