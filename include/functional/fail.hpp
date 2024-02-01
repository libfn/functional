// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_FAIL
#define INCLUDE_FUNCTIONAL_FAIL

#include "functional/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"
#include "functional/utility.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace fn {
template <typename Fn, typename V>
concept invocable_fail //
    = (some_expected_non_void<V> && requires(Fn &&fn, V &&v) {
        {
          std::invoke(FWD(fn), FWD(v).value())
        } -> std::convertible_to<typename std::remove_cvref_t<V>::error_type>;
      }) || (some_expected_void<V> && requires(Fn &&fn) {
        {
          std::invoke(FWD(fn))
        } -> std::convertible_to<typename std::remove_cvref_t<V>::error_type>;
      }) || (some_optional<V> && requires(Fn &&fn, V &&v) {
        {
          std::invoke(FWD(fn), FWD(v).value())
        } -> std::same_as<void>;
      });

constexpr inline struct fail_t final {
  constexpr auto operator()(auto &&fn) const noexcept -> functor<fail_t, decltype(fn)> //
  {
    return {FWD(fn)};
  }

  struct apply;
} fail = {};

struct fail_t::apply final {
  static constexpr auto operator()(some_expected auto &&v, auto &&fn) noexcept //
      -> same_monadic_type_as<decltype(v)> auto
    requires invocable_fail<decltype(fn), decltype(v)>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    return FWD(v).and_then( //
        [&fn](auto &&...arg) noexcept -> type {
          return std::unexpected<typename type::error_type>{std::invoke(FWD(fn), FWD(arg)...)};
        });
  }

  static constexpr auto operator()(some_optional auto &&v, auto &&fn) noexcept -> same_monadic_type_as<decltype(v)> auto
    requires invocable_fail<decltype(fn), decltype(v)>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    if (v.has_value()) {
      std::invoke(FWD(fn), FWD(v).value());
    }
    return type{std::nullopt};
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_FAIL
