// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_FILTER
#define INCLUDE_FUNCTIONAL_FILTER

#include "functional/detail/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"
#include "functional/utility.hpp"

#include <concepts>
#include <expected>
#include <functional>
#include <type_traits>
#include <utility>

namespace fn {
template <typename Pred, typename Err, typename V>
concept invocable_filter
    = (some_expected_non_void<V> && requires(Pred &&pred, Err &&on_err, V &&v) {
        {
          std::invoke(FWD(pred), std::as_const(v).value())
        } -> convertible_to_bool;
        {
          std::invoke(FWD(on_err), FWD(v).value())
        } -> std::convertible_to<typename std::remove_cvref_t<V>::error_type>;
      }) || (some_expected_void<V> && requires(Pred &&pred, Err &&on_err, V &&v) {
        {
          std::invoke(FWD(pred))
        } -> convertible_to_bool;
        {
          std::invoke(FWD(on_err))
        } -> std::convertible_to<typename std::remove_cvref_t<V>::error_type>;
      }) || (some_optional<V> && std::same_as<Err, void> && requires(Pred &&pred, V &&v) {
        {
          std::invoke(FWD(pred), std::as_const(v).value())
        } -> convertible_to_bool;
      });

constexpr inline struct filter_t final {
  // NOTE Optional needs one arguments, expected needs two
  constexpr auto operator()(auto &&...args) const noexcept
      -> functor<filter_t, decltype(args)...>
    requires(sizeof...(args) >= 1) && (sizeof...(args) < 3)
  {
    return {FWD(args)...};
  }

  struct apply;
} filter = {};

struct filter_t::apply final {
  static constexpr auto operator()(some_expected_non_void auto &&v, auto &&pred,
                                   auto &&on_err) noexcept
      -> same_monadic_type_as<decltype(v)> auto
    requires invocable_filter<decltype(pred), decltype(on_err), decltype(v)>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    return FWD(v).and_then( //
        [&pred, &on_err](auto &&arg) noexcept -> type {
          const bool keep{std::invoke(FWD(pred), std::as_const(arg))};
          return (keep ? type{std::in_place, FWD(arg)}
                       : type{std::unexpect,
                              std::invoke(FWD(on_err),
                                          FWD(arg))}); // GCOVR_EXCL_BR_LINE
        });
  }

  static constexpr auto operator()(some_expected_void auto &&v, auto &&pred,
                                   auto &&on_err) noexcept
      -> same_monadic_type_as<decltype(v)> auto
    requires invocable_filter<decltype(pred), decltype(on_err), decltype(v)>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    return FWD(v).and_then( //
        [&pred, &on_err]() noexcept -> type {
          const bool keep{std::invoke(FWD(pred))};
          return (keep ? type{std::in_place}
                       : type{std::unexpect,
                              std::invoke(FWD(on_err))}); // GCOVR_EXCL_BR_LINE
        });
  }

  static constexpr auto operator()(some_optional auto &&v, auto &&pred) noexcept
      -> same_monadic_type_as<decltype(v)> auto
    requires invocable_filter<decltype(pred), void, decltype(v)>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    return FWD(v).and_then( //
        [&pred](auto &&arg) noexcept -> type {
          const bool keep{std::invoke(FWD(pred), std::as_const(arg))};
          return (keep ? type{std::in_place, FWD(arg)} // GCOVR_EXCL_BR_LINE
                       : type{std::nullopt});
        });
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_FILTER
