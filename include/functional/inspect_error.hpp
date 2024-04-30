// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_INSPECT_ERROR
#define INCLUDE_FUNCTIONAL_INSPECT_ERROR

#include "functional/functional.hpp"
#include "functional/functor.hpp"

#include <concepts>
#include <utility>

namespace fn {
template <typename Fn, typename V>
concept invocable_inspect_error //
    = (some_expected<V> && requires(Fn &&fn, V &&v) {
        {
          ::fn::invoke(FWD(fn), std::as_const(v).error())
        } -> std::same_as<void>;
      }) || (some_optional<V> && requires(Fn &&fn) {
        {
          ::fn::invoke(FWD(fn))
        } -> std::same_as<void>;
      });

constexpr inline struct inspect_error_t final {
  [[nodiscard]] constexpr auto operator()(auto &&fn) const noexcept -> functor<inspect_error_t, decltype(fn)> //
  {
    return {FWD(fn)};
  }

  struct apply;
} inspect_error = {};

struct inspect_error_t::apply final {
  [[nodiscard]] static constexpr auto operator()(auto &&fn, some_expected auto &&v) noexcept -> decltype(v)
    requires invocable_inspect_error<decltype(fn), decltype(v)>
  {
    if (not v.has_value()) {
      ::fn::invoke(FWD(fn), std::as_const(v).error()); // side-effects only
    }
    return FWD(v);
  }

  [[nodiscard]] static constexpr auto operator()(auto &&fn, some_optional auto &&v) noexcept -> decltype(v)
    requires invocable_inspect_error<decltype(fn), decltype(v)>
  {
    if (not v.has_value()) {
      ::fn::invoke(FWD(fn)); // side-effects only
    }
    return FWD(v);
  }

  // No support for choice since there's no error to operate on
  static auto operator()(auto &&, some_choice auto &&) noexcept = delete;
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_INSPECT_ERROR
