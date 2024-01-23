// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_TRANSFORM_ERROR
#define INCLUDE_FUNCTIONAL_TRANSFORM_ERROR

#include "functional/detail/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"
#include "functional/utility.hpp"

#include <concepts>
#include <expected>
#include <utility>

namespace fn {
template <typename Fn, typename V>
concept invocable_transform_error
    = (some_expected<V> && requires(Fn &&fn, V &&v) {
        {
          std::invoke(FWD(fn), FWD(v).error())
        } -> convertible_to_unexpected;
      });

constexpr inline struct transform_error_t final {
  auto operator()(auto &&fn) const noexcept
      -> functor<transform_error_t, decltype(fn)>
  {
    return {FWD(fn)};
  }

  struct apply;
} transform_error = {};

struct transform_error_t::apply final {
  static auto operator()(some_expected auto &&v, auto &&fn) noexcept
      -> same_value_kind<decltype(v)> auto
    requires invocable_transform_error<decltype(fn) &&, decltype(v) &&>
  {
    return FWD(v).transform_error(FWD(fn));
  }

  // No support for optional since there's no error state to operate on
  static auto operator()(some_optional auto &&v, auto &&...args) noexcept
      = delete;
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_TRANSFORM_ERROR
