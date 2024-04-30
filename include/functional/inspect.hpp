// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_INSPECT
#define INCLUDE_FUNCTIONAL_INSPECT

#include "functional/expected.hpp"
#include "functional/functional.hpp"
#include "functional/functor.hpp"

#include <concepts>
#include <utility>

namespace fn {
template <typename Fn, typename V>
concept invocable_inspect //
    = (some_expected_non_void<V> && requires(Fn &&fn, V &&v) {
        {
          ::fn::invoke(FWD(fn), std::as_const(v).value())
        } -> std::same_as<void>;
      }) || (some_expected_void<V> && requires(Fn &&fn) {
        {
          ::fn::invoke(FWD(fn))
        } -> std::same_as<void>;
      }) || (some_optional<V> && requires(Fn &&fn, V &&v) {
        {
          ::fn::invoke(FWD(fn), std::as_const(v).value())
        } -> std::same_as<void>;
      }) || (some_choice<V> && requires(Fn &&fn, V &&v) {
        {
          ::fn::invoke(FWD(fn), std::as_const(v).value())
        } -> std::same_as<void>;
      });

constexpr inline struct inspect_t final {
  [[nodiscard]] constexpr auto operator()(auto &&fn) const noexcept -> functor<inspect_t, decltype(fn)>
  {
    return {FWD(fn)};
  }

  struct apply;
} inspect = {};

struct inspect_t::apply final {
  [[nodiscard]] static constexpr auto operator()(auto &&fn, some_expected_non_void auto &&v) noexcept -> decltype(v)
    requires invocable_inspect<decltype(fn), decltype(v)>
  {
    if (v.has_value()) {
      ::fn::invoke(FWD(fn), std::as_const(v).value()); // side-effects only
    }
    return FWD(v);
  }

  [[nodiscard]] static constexpr auto operator()(auto &&fn, some_expected_void auto &&v) noexcept -> decltype(v)
    requires invocable_inspect<decltype(fn), decltype(v)>
  {
    if (v.has_value()) {
      ::fn::invoke(FWD(fn)); // side-effects only
    }
    return FWD(v);
  }

  [[nodiscard]] static constexpr auto operator()(auto &&fn, some_optional auto &&v) noexcept -> decltype(v)
    requires invocable_inspect<decltype(fn), decltype(v)>
  {
    if (v.has_value()) {
      ::fn::invoke(FWD(fn), std::as_const(v).value()); // side-effects only
    }
    return FWD(v);
  }

  [[nodiscard]] static constexpr auto operator()(auto &&fn, some_choice auto &&v) noexcept -> decltype(v)
    requires invocable_inspect<decltype(fn), decltype(v)>
  {
    ::fn::invoke(FWD(fn), std::as_const(v).value()); // side-effects only
    return FWD(v);
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_INSPECT
