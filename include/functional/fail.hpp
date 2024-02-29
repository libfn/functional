// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_FAIL
#define INCLUDE_FUNCTIONAL_FAIL

#include "functional/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"
#include "functional/optional.hpp"
#include "functional/utility.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace fn {
template <typename Fn, typename V>
concept invocable_fail //
    = (some_expected_pack<V> && requires(Fn &&fn, V &&v) {
        {
          FWD(v).value().invoke(FWD(fn))
        } -> std::convertible_to<typename std::remove_cvref_t<V>::error_type>;
      }) || (some_expected_non_pack<V> && requires(Fn &&fn, V &&v) {
        {
          std::invoke(FWD(fn), FWD(v).value())
        } -> std::convertible_to<typename std::remove_cvref_t<V>::error_type>;
      }) || (some_expected_void<V> && requires(Fn &&fn) {
        {
          std::invoke(FWD(fn))
        } -> std::convertible_to<typename std::remove_cvref_t<V>::error_type>;
      }) || (some_optional_pack<V> && requires(Fn &&fn, V &&v) {
        {
          FWD(v).value().invoke(FWD(fn))
        } -> std::same_as<void>;
      }) || (some_optional_non_pack<V> && requires(Fn &&fn, V &&v) {
        {
          std::invoke(FWD(fn), FWD(v).value())
        } -> std::same_as<void>;
      });

constexpr inline struct fail_t final {
  [[nodiscard]] constexpr auto operator()(auto &&fn) const noexcept -> functor<fail_t, decltype(fn)> //
  {
    return {FWD(fn)};
  }

  struct apply;
} fail = {};

struct fail_t::apply final {
  [[nodiscard]] static constexpr auto operator()(some_expected auto &&v, auto &&fn) noexcept //
      -> same_monadic_type_as<decltype(v)> auto
    requires invocable_fail<decltype(fn), decltype(v)>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    return FWD(v).and_then( //
        [&fn](auto &&...arg) noexcept -> type {
          return type{std::unexpect, std::invoke(FWD(fn), FWD(arg)...)};
        });
  }

  [[nodiscard]] static constexpr auto operator()(some_optional_pack auto &&v, auto &&fn) noexcept
      -> same_monadic_type_as<decltype(v)> auto
    requires invocable_fail<decltype(fn), decltype(v)>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    if (v.has_value()) {
      FWD(v).value().invoke(FWD(fn));
    }
    return type{std::nullopt};
  }

  [[nodiscard]] static constexpr auto operator()(some_optional_non_pack auto &&v, auto &&fn) noexcept
      -> same_monadic_type_as<decltype(v)> auto
    requires invocable_fail<decltype(fn), decltype(v)>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    if (v.has_value()) {
      std::invoke(FWD(fn), FWD(v).value());
    }
    return type{std::nullopt};
  }

  // No support for choice since there's no error to operate on
  static auto operator()(some_choice auto &&v, auto &&...args) noexcept = delete;
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_FAIL
