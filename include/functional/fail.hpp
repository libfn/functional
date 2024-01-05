// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_FAIL
#define INCLUDE_FUNCTIONAL_FAIL

#include "functional/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"

#include <type_traits>
#include <utility>

namespace fn {

struct fail_t final {
  auto operator()(auto &&fn) const noexcept
      -> functor<fail_t, std::decay_t<decltype(fn)>>
  {
    using functor_type = functor<fail_t, std::decay_t<decltype(fn)>>;
    return functor_type{{std::forward<decltype(fn)>(fn)}};
  }
};
constexpr static fail_t fail = {};

auto monadic_apply(some_optional auto &&v, fail_t const &,
                   some_tuple auto &&fn) noexcept -> auto
  requires(std::tuple_size_v<std::decay_t<decltype(fn)>> == 1)
{
  using type = std::decay_t<decltype(v)>;
  return std::forward<decltype(v)>(v).and_then([&fn](auto &&...args) -> type {
    std::get<0>(std::forward<decltype(fn)>(fn))(
        std::forward<decltype(args)>(args)...); // side-effects only
    return {std::nullopt};
  });
}

auto monadic_apply(some_expected auto &&v, fail_t const &,
                   some_tuple auto &&fn) noexcept -> auto
  requires(std::tuple_size_v<std::decay_t<decltype(fn)>> == 1)
{
  using type = std::decay_t<decltype(v)>;
  return std::forward<decltype(v)>(v).and_then([&fn](auto &&...args) -> type {
    return {std::unexpected<typename type::error_type>{
        std::get<0>(std::forward<decltype(fn)>(fn))(
            std::forward<decltype(args)>(args)...)}};
  });
}

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_FAIL
