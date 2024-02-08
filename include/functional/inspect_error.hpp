// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_INSPECT_ERROR
#define INCLUDE_FUNCTIONAL_INSPECT_ERROR

#include "functional/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"
#include "functional/utility.hpp"

#include <concepts>
#include <utility>

namespace fn {
template <typename Fn, typename V>
concept invocable_inspect_error //
    = (some_expected<V> && requires(Fn &&fn, V &&v) {
        {
          std::invoke(FWD(fn), std::as_const(v).error())
        } -> std::same_as<void>;
      }) || (some_optional<V> && requires(Fn &&fn) {
        {
          std::invoke(FWD(fn))
        } -> std::same_as<void>;
      });

constexpr inline struct inspect_error_t final {
  constexpr auto operator()(auto &&fn) const noexcept -> functor<inspect_error_t, decltype(fn)> //
  {
    return {FWD(fn)};
  }

  struct apply;
} inspect_error = {};

struct inspect_error_t::apply final {
  static constexpr auto operator()(some_expected auto &&v, auto &&fn) noexcept -> decltype(v)
    requires invocable_inspect_error<decltype(fn), decltype(v)>
  {
    if (not v.has_value()) {
      std::invoke(FWD(fn), std::as_const(v).error()); // side-effects only
    }
    return FWD(v);
  }

  static constexpr auto operator()(some_optional auto &&v, auto &&fn) noexcept -> decltype(v)
    requires invocable_inspect_error<decltype(fn), decltype(v)>
  {
    if (not v.has_value()) {
      std::invoke(FWD(fn)); // side-effects only
    }
    return FWD(v);
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_INSPECT_ERROR
