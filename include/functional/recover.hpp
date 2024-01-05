// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_RECOVER
#define INCLUDE_FUNCTIONAL_RECOVER

#include "functional/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"

#include <type_traits>
#include <utility>

namespace fn {

struct recover_t final {
  auto operator()(auto &&fn) const noexcept
      -> functor<recover_t, std::decay_t<decltype(fn)>>
  {
    using functor_type = functor<recover_t, std::decay_t<decltype(fn)>>;
    return functor_type{{std::forward<decltype(fn)>(fn)}};
  }
};
constexpr static recover_t recover = {};

auto monadic_apply(some_monadic_type auto &&v, recover_t const &,
                   some_tuple auto &&fn) noexcept -> auto
  requires(std::tuple_size_v<std::decay_t<decltype(fn)>> == 1)
{
  using type = std::decay_t<decltype(v)>;
  return std::forward<decltype(v)>(v).or_else([&fn](auto &&...args) -> type {
    return {std::get<0>(std::forward<decltype(fn)>(fn))(
        std::forward<decltype(args)>(args)...)};
  });
}

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_RECOVER
