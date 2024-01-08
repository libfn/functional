// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_FILTER
#define INCLUDE_FUNCTIONAL_FILTER

#include "functional/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"

#include <type_traits>
#include <utility>

namespace fn {

struct filter_t final {
  auto operator()(auto &&...fn) const noexcept
      -> functor<filter_t, std::decay_t<decltype(fn)>...>
    requires(sizeof...(fn) > 0)
  {
    using functor_type = functor<filter_t, std::decay_t<decltype(fn)>...>;
    return functor_type{{std::forward<decltype(fn)>(fn)...}};
  }
};
constexpr static filter_t filter = {};

auto monadic_apply(some_optional auto &&v, filter_t const &,
                   some_tuple auto &&fn) noexcept -> auto
  requires(std::tuple_size_v<std::decay_t<decltype(fn)>> == 1)
{
  using type = std::decay_t<decltype(v)>;
  return std::forward<decltype(v)>(v).and_then([&fn](auto &&args) -> type {
    if (std::get<0>(std::forward<decltype(fn)>(fn))(std::as_const(args)))
      return std::forward<decltype(args)>(args);
    return {std::nullopt};
  });
}

auto monadic_apply(some_expected auto &&v, filter_t const &,
                   some_tuple auto &&fn) noexcept -> auto
  requires(std::tuple_size_v<std::decay_t<decltype(fn)>> == 2) // std::expected
{
  using type = std::decay_t<decltype(v)>;
  return std::forward<decltype(v)>(v).and_then([&fn](auto &&args) -> type {
    if (std::get<0>(std::forward<decltype(fn)>(fn))(std::as_const(args)))
      return {std::forward<decltype(args)>(args)};
    return {std::unexpected<typename type::error_type>{std::get<1>(
        std::forward<decltype(fn)>(fn))(std::forward<decltype(args)>(args))}};
  });
}

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_FILTER
