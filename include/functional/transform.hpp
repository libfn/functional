// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_TRANSFORM
#define INCLUDE_FUNCTIONAL_TRANSFORM

#include "functional/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"

#include <type_traits>
#include <utility>

namespace fn {

struct transform_t final {
  auto operator()(auto &&fn) const noexcept
      -> functor<transform_t, std::decay_t<decltype(fn)>>
  {
    using functor_type = functor<transform_t, std::decay_t<decltype(fn)>>;
    return functor_type{{std::forward<decltype(fn)>(fn)}};
  }
};
constexpr static transform_t transform = {};

auto monadic_apply(some_monadic_type auto &&v, transform_t const &,
                   some_tuple auto &&fn) noexcept -> auto
  requires(std::tuple_size_v<std::decay_t<decltype(fn)>> == 1)
{
  return std::forward<decltype(v)>(v).transform(
      std::get<0>(std::forward<decltype(fn)>(fn)));
}

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_TRANSFORM
