// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_INSPECT
#define INCLUDE_FUNCTIONAL_INSPECT

#include "functional/detail/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"
#include "functional/utility.hpp"

#include <concepts>
#include <utility>

namespace fn {

constexpr inline struct inspect_t final {
  auto operator()(auto &&...fn) const noexcept
      -> functor<inspect_t, decltype(fn)...>
    requires(sizeof...(fn) > 0) && (sizeof...(fn) <= 2)
  {
    return {FWD(fn)...};
  }

  struct apply;
} inspect = {};

struct inspect_t::apply final {
  static auto operator()(some_expected auto &&v, auto &&fn) noexcept
      -> decltype(v)
    requires std::invocable<decltype(fn), decltype(std::as_const(v.value()))>
             && (!std::is_void_v<decltype(v.value())>)
  {
    static_assert(std::is_void_v<std::invoke_result_t<
                      decltype(fn), decltype(std::as_const(v.value()))>>);
    if (v.has_value()) {
      FWD(fn)(std::as_const(v.value()));
    }
    return FWD(v);
  }

  static auto operator()(some_expected auto &&v, auto &&fn) noexcept
      -> decltype(v)
    requires std::invocable<decltype(fn)>
             && (std::is_void_v<decltype(v.value())>)
  {
    static_assert(std::is_void_v<std::invoke_result_t<decltype(fn)>>);
    if (v.has_value()) {
      FWD(fn)();
    }
    return FWD(v);
  }

  static auto operator()(some_optional auto &&v, auto &&fn) noexcept
      -> decltype(v)
    requires std::invocable<decltype(fn), decltype(std::as_const(v.value()))>
  {
    static_assert(std::is_void_v<std::invoke_result_t<
                      decltype(fn), decltype(std::as_const(v.value()))>>);
    if (v.has_value()) {
      FWD(fn)(std::as_const(v.value()));
    }
    return FWD(v);
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_INSPECT
