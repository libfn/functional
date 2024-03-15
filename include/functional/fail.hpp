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
    = (some_expected_non_void<V> && requires(Fn &&fn, V &&v) {
        {
          ::fn::invoke(FWD(fn), FWD(v).value())
        } -> std::convertible_to<typename std::remove_cvref_t<V>::error_type>;
      }) || (some_expected_void<V> && requires(Fn &&fn) {
        {
          ::fn::invoke(FWD(fn))
        } -> std::convertible_to<typename std::remove_cvref_t<V>::error_type>;
      }) || (some_optional<V> && requires(Fn &&fn, V &&v) {
        {
          ::fn::invoke(FWD(fn), FWD(v).value())
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
  [[nodiscard]] static constexpr auto operator()(some_expected_non_void auto &&v, auto &&fn) noexcept //
      -> same_monadic_type_as<decltype(v)> auto
    requires invocable_fail<decltype(fn), decltype(v)>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    if (v.has_value()) {
      return type{std::unexpect, ::fn::invoke(FWD(fn), FWD(v).value())};
    }
    return type{std::unexpect, FWD(v).error()};
  }

  [[nodiscard]] static constexpr auto operator()(some_expected_void auto &&v, auto &&fn) noexcept //
      -> same_monadic_type_as<decltype(v)> auto
    requires invocable_fail<decltype(fn), decltype(v)>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    if (v.has_value()) {
      return type{std::unexpect, ::fn::invoke(FWD(fn))};
    }
    return type{std::unexpect, FWD(v).error()};
  }

  [[nodiscard]] static constexpr auto operator()(some_optional auto &&v, auto &&fn) noexcept
      -> same_monadic_type_as<decltype(v)> auto
    requires invocable_fail<decltype(fn), decltype(v)>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    if (v.has_value()) {
      ::fn::invoke(FWD(fn), FWD(v).value());
    }
    return type{std::nullopt};
  }

  // No support for choice since there's no error to operate on
  static auto operator()(some_choice auto &&v, auto &&...args) noexcept = delete;
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_FAIL
