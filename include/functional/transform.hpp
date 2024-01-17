// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_TRANSFORM
#define INCLUDE_FUNCTIONAL_TRANSFORM

#include "functional/detail/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"
#include "functional/utility.hpp"

#include <concepts>
#include <utility>

namespace fn {

constexpr static struct transform_t final {
  auto operator()(auto &&fn) const noexcept
      -> functor<transform_t, decltype(fn)>
  {
    return {FWD(fn)};
  }

  struct apply;
} transform = {};

struct transform_t::apply final {
  static auto operator()(some_expected auto &&v, auto &&fn) noexcept
      -> decltype(auto)
    requires std::invocable<decltype(fn), decltype(FWD(v).value())>
             && (!std::is_void_v<decltype(v.value())>)
  {
    return FWD(v).transform(FWD(fn));
  }

  static auto operator()(some_expected auto &&v, auto &&fn) noexcept
      -> decltype(auto)
    requires std::invocable<decltype(fn)>
             && (std::is_void_v<decltype(v.value())>)
  {
    return FWD(v).transform(FWD(fn));
  }

  static auto operator()(some_optional auto &&v, auto &&fn) noexcept
      -> decltype(auto)
    requires std::invocable<decltype(fn), decltype(FWD(v).value())>
  {
    return FWD(v).transform(FWD(fn));
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_TRANSFORM
