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
    using error_type = detail::as_value_t<decltype(fn(v.value()))>;
    using value_type = std::remove_cvref_t<decltype(v)>::value_type;
    static_assert(
        std::is_convertible_v<
            typename std::remove_cvref_t<decltype(v)>::error_type, error_type>);
    using type = std::expected<value_type, error_type>;
    return std::forward<decltype(v)>(v).and_then( //
        [&fn](auto &&...args) noexcept -> type {
          return std::unexpected<error_type>{std::forward<decltype(fn)>(fn)(
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
    return std::forward<decltype(v)>(v).and_then( //
        [&fn](auto &&arg) noexcept -> type {
          std::forward<decltype(fn)>(fn)(
              std::forward<decltype(arg)>(arg)); // side-effects only
          return {std::nullopt};
        });
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_FAIL
