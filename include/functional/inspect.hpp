// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_INSPECT
#define INCLUDE_FUNCTIONAL_INSPECT

#include "functional/detail/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"

#include <concepts>
#include <utility>

namespace fn {

constexpr inline struct inspect_t final {
  auto operator()(auto &&...fn) const noexcept
      -> functor<inspect_t, decltype(fn)...>
    requires(sizeof...(fn) > 0)
  {
    return {std::forward<decltype(fn)>(fn)...};
  }

  struct apply;
} inspect = {};

struct inspect_t::apply final {
  static auto operator()(some_expected auto &&v, auto &&fn1,
                         auto &&fn2) noexcept -> decltype(v)
    requires std::invocable<decltype(fn1), decltype(std::as_const(v.value()))>
             && std::invocable<decltype(fn2),
                               decltype(std::as_const(v.error()))>
             && (!std::is_void_v<decltype(v.value())>)
  {
    static_assert(
        std::is_void_v<std::invoke_result_t<
            decltype(fn1), decltype(std::forward<decltype(v)>(v).value())>>);
    static_assert(
        std::is_void_v<std::invoke_result_t<
            decltype(fn2), decltype(std::forward<decltype(v)>(v).error())>>);
    if (v.has_value()) {
      std::forward<decltype(fn1)>(fn1)(std::as_const(v.value()));
    } else {
      std::forward<decltype(fn2)>(fn2)(std::as_const(v.error()));
    }
    return std::forward<decltype(v)>(v);
  }

  static auto operator()(some_expected auto &&v, auto &&fn1,
                         auto &&fn2) noexcept -> decltype(v)
    requires std::invocable<decltype(fn1)>
             && std::invocable<decltype(fn2),
                               decltype(std::as_const(v.error()))>
             && (std::is_void_v<decltype(v.value())>)
  {
    static_assert(std::is_void_v<std::invoke_result_t<decltype(fn1)>>);
    static_assert(
        std::is_void_v<std::invoke_result_t<
            decltype(fn2), decltype(std::forward<decltype(v)>(v).error())>>);
    if (v.has_value()) {
      std::forward<decltype(fn1)>(fn1)();
    } else {
      std::forward<decltype(fn2)>(fn2)(std::as_const(v.error()));
    }
    return std::forward<decltype(v)>(v);
  }

  static auto operator()(some_expected auto &&v, auto &&fn) noexcept
      -> decltype(v)
    requires std::invocable<decltype(fn), decltype(std::as_const(v.value()))>
             && std::invocable<decltype(fn), decltype(std::as_const(v.error()))>
             && (!std::is_void_v<decltype(v.value())>)
  {
    static_assert(
        std::is_void_v<std::invoke_result_t<
            decltype(fn), decltype(std::forward<decltype(v)>(v).value())>>);
    static_assert(
        std::is_void_v<std::invoke_result_t<
            decltype(fn), decltype(std::forward<decltype(v)>(v).error())>>);
    if (v.has_value()) {
      std::forward<decltype(fn)>(fn)(std::as_const(v.value()));
    } else {
      std::forward<decltype(fn)>(fn)(std::as_const(v.error()));
    }
    return std::forward<decltype(v)>(v);
  }

  static auto operator()(some_expected auto &&v, auto &&fn) noexcept
      -> decltype(v)
    requires std::invocable<decltype(fn)>
             && std::invocable<decltype(fn), decltype(std::as_const(v.error()))>
             && (std::is_void_v<decltype(v.value())>)
  {
    static_assert(std::is_void_v<std::invoke_result_t<decltype(fn)>>);
    static_assert(
        std::is_void_v<std::invoke_result_t<
            decltype(fn), decltype(std::forward<decltype(v)>(v).error())>>);
    if (v.has_value()) {
      std::forward<decltype(fn)>(fn)();
    } else {
      std::forward<decltype(fn)>(fn)(std::as_const(v.error()));
    }
    return std::forward<decltype(v)>(v);
  }

  static auto operator()(some_optional auto &&v, auto &&fn1,
                         auto &&fn2) noexcept -> decltype(v)
    requires std::invocable<decltype(fn1), decltype(std::as_const(v.value()))>
             && std::invocable<decltype(fn2)>
  {
    static_assert(
        std::is_void_v<std::invoke_result_t<
            decltype(fn1), decltype(std::forward<decltype(v)>(v).value())>>);
    static_assert(std::is_void_v<std::invoke_result_t<decltype(fn2)>>);
    if (v.has_value()) {
      std::forward<decltype(fn1)>(fn1)(std::as_const(v.value()));
    } else {
      std::forward<decltype(fn2)>(fn2)();
    }
    return std::forward<decltype(v)>(v);
  }

  static auto operator()(some_optional auto &&v, auto &&fn) noexcept
      -> decltype(v)
    requires std::invocable<decltype(fn), decltype(std::as_const(v.value()))>
             && std::invocable<decltype(fn)>
  {
    static_assert(
        std::is_void_v<std::invoke_result_t<
            decltype(fn), decltype(std::forward<decltype(v)>(v).value())>>);
    static_assert(std::is_void_v<std::invoke_result_t<decltype(fn)>>);
    if (v.has_value()) {
      std::forward<decltype(fn)>(fn)(std::as_const(v.value()));
    } else {
      std::forward<decltype(fn)>(fn)();
    }
    return std::forward<decltype(v)>(v);
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_INSPECT
