// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_FAIL
#define INCLUDE_FUNCTIONAL_FAIL

#include "functional/detail/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"

#include <concepts>
#include <utility>

namespace fn {

constexpr inline struct fail_t final {
  auto operator()(auto &&fn) const noexcept -> functor<fail_t, decltype(fn)>
  {
    return {std::forward<decltype(fn)>(fn)};
  }

  struct apply;
} fail = {};

struct fail_t::apply final {
  static auto operator()(some_expected auto &&v, auto &&fn) noexcept
      -> decltype(auto)
    requires std::invocable<decltype(fn),
                            decltype(std::forward<decltype(v)>(v).value())>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    return std::forward<decltype(v)>(v).and_then( //
        [&fn](auto &&...args) noexcept -> type {
          return std::unexpected<typename type::error_type>{
              std::forward<decltype(fn)>(fn)(
                  std::forward<decltype(args)>(args)...)};
        });
  }

  static auto operator()(some_optional auto &&v, auto &&fn) noexcept
      -> decltype(auto)
    requires std::invocable<decltype(fn),
                            decltype(std::forward<decltype(v)>(v).value())>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    static_assert(std::is_void_v<decltype(fn(v.value()))>);
    if (v.has_value()) {
      std::forward<decltype(fn)>(fn)(
          std::forward<decltype(v)>(v).value()); // side-effects only
    }
    return type{std::nullopt};
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_FAIL
