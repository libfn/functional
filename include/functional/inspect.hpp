// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_INSPECT
#define INCLUDE_FUNCTIONAL_INSPECT

#include "functional/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"
#include "functional/utility.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace fn {
template <typename Fn, typename V>
concept invocable_inspect //
    = (some_expected_pack<V> && requires(Fn &&fn, V &&v) {
        {
          std::as_const(v).value().invoke(FWD(fn))
        } -> std::same_as<void>;
      }) || (some_expected_non_pack<V> && requires(Fn &&fn, V &&v) {
        {
          std::invoke(FWD(fn), std::as_const(v).value())
        } -> std::same_as<void>;
      }) || (some_expected_void<V> && requires(Fn &&fn) {
        {
          std::invoke(FWD(fn))
        } -> std::same_as<void>;
      }) || (some_optional_pack<V> && requires(Fn &&fn, V &&v) {
        {
          std::as_const(v).value().invoke(FWD(fn))
        } -> std::same_as<void>;
      }) || (some_optional_non_pack<V> && requires(Fn &&fn, V &&v) {
        {
          std::invoke(FWD(fn), std::as_const(v).value())
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
  [[nodiscard]] static constexpr auto operator()(some_expected auto &&v, auto &&fn) noexcept -> decltype(v)
    requires invocable_inspect<decltype(fn), decltype(v)>
  {
    std::as_const(v).transform([&fn](auto const &...args) -> void {
      std::invoke(FWD(fn), FWD(args)...); // side-effects only
    });
    return FWD(v);
  }

  [[nodiscard]] static constexpr auto operator()(some_optional_pack auto &&v, auto &&fn) noexcept -> decltype(v)
    requires invocable_inspect<decltype(fn), decltype(v)>
  {
    if (v.has_value()) {
      std::as_const(v).value().invoke(FWD(fn)); // side-effects only
    }
    return FWD(v);
  }

  [[nodiscard]] static constexpr auto operator()(some_optional_non_pack auto &&v, auto &&fn) noexcept -> decltype(v)
    requires invocable_inspect<decltype(fn), decltype(v)>
  {
    if (v.has_value()) {
      std::invoke(FWD(fn), std::as_const(v).value()); // side-effects only
    }
    return FWD(v);
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_INSPECT
