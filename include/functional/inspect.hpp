// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_INSPECT
#define INCLUDE_FUNCTIONAL_INSPECT

#include "functional/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"

#include <type_traits>
#include <utility>

namespace fn {

struct inspect_t final {
  auto operator()(auto &&...fn) const noexcept
      -> functor<inspect_t, std::decay_t<decltype(fn)>...>
    requires(sizeof...(fn) > 0)
  {
    using functor_type = functor<inspect_t, std::decay_t<decltype(fn)>...>;
    return functor_type{{std::forward<decltype(fn)>(fn)...}};
  }
};
constexpr static inspect_t inspect = {};

auto monadic_apply(some_optional auto &&v, inspect_t const &,
                   some_tuple auto &&fn) noexcept -> auto
  requires(std::tuple_size_v<std::decay_t<decltype(fn)>> == 2)
{
  if (v.has_value()) {
    std::get<0>(std::forward<decltype(fn)>(fn))(std::as_const(*v));
  } else {
    std::get<1>(std::forward<decltype(fn)>(fn))();
  }
  return std::forward<decltype(v)>(v);
}

auto monadic_apply(some_expected auto &&v, inspect_t const &,
                   some_tuple auto &&fn) noexcept -> auto
  requires(std::tuple_size_v<std::decay_t<decltype(fn)>> == 2)
{
  if (v.has_value()) {
    std::get<0>(std::forward<decltype(fn)>(fn))(std::as_const(v.value()));
  } else {
    std::get<1>(std::forward<decltype(fn)>(fn))(std::as_const(v.error()));
  }
  return std::forward<decltype(v)>(v);
}

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_INSPECT
