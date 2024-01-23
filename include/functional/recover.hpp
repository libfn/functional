// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_RECOVER
#define INCLUDE_FUNCTIONAL_RECOVER

#include "functional/detail/concepts.hpp"
#include "functional/detail/traits.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"
#include "functional/utility.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace fn {
template <typename Fn, typename V>
concept invocable_recover
    = (some_expected_non_void<V> && requires(Fn &&fn, V &&v) {
        {
          std::invoke(FWD(fn), FWD(v).error())
        } -> std::convertible_to<typename std::remove_cvref_t<V>::value_type>;
      }) || (some_expected_void<V> && requires(Fn &&fn, V &&v) {
        {
          std::invoke(FWD(fn), FWD(v).error())
        } -> std::same_as<void>;
      }) || (some_optional<V> && requires(Fn &&fn, V &&v) {
        {
          std::invoke(FWD(fn))
        } -> std::convertible_to<typename std::remove_cvref_t<V>::value_type>;
      });

constexpr inline struct recover_t final {
  auto operator()(auto &&fn) const noexcept -> functor<recover_t, decltype(fn)>
  {
    return {FWD(fn)};
  }

  struct apply;
} recover = {};

struct recover_t::apply final {
  static auto operator()(some_expected_non_void auto &&v, auto &&fn) noexcept
      -> same_kind<decltype(v)> auto
    requires invocable_recover<decltype(fn) &&, decltype(v) &&>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    return FWD(v).or_else([&fn](auto &&arg) noexcept -> type {
      return type{std::in_place, FWD(fn)(FWD(arg))};
    });
  }

  static auto operator()(some_expected_void auto &&v, auto &&fn) noexcept
      -> same_kind<decltype(v)> auto
    requires invocable_recover<decltype(fn) &&, decltype(v) &&>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    return FWD(v).or_else([&fn](auto &&arg) noexcept -> type {
      FWD(fn)(FWD(arg)); // side-effects only
      return type{std::in_place};
    });
  }

  static auto operator()(some_optional auto &&v, auto &&fn) noexcept
      -> same_kind<decltype(v)> auto
    requires invocable_recover<decltype(fn) &&, decltype(v) &&>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    return FWD(v).or_else([&fn]() noexcept -> type {
      return type{std::in_place, FWD(fn)()};
    });
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_RECOVER
