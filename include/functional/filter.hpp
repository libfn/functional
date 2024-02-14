// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_FILTER
#define INCLUDE_FUNCTIONAL_FILTER

#include "functional/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"
#include "functional/utility.hpp"

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

namespace fn {
template <typename Pred, typename Err, typename V>
concept invocable_filter //
    = (some_expected_pack<V> && requires(Pred &&pred, Err &&on_err, V &&v) {
        {
          std::as_const(v).value().invoke(FWD(pred))
        } -> convertible_to_bool;
        {
          FWD(v).value().invoke(FWD(on_err))
        } -> std::convertible_to<typename std::remove_cvref_t<V>::error_type>;
      }) || (some_expected_non_pack<V> && requires(Pred &&pred, Err &&on_err, V &&v) {
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
      }) || (some_optional_pack<V> && std::same_as<Err, void> && requires(Pred &&pred, V &&v) {
        {
          std::as_const(v).value().invoke(FWD(pred))
        } -> convertible_to_bool;
      }) || (some_optional_non_pack<V> && std::same_as<Err, void> && requires(Pred &&pred, V &&v) {
        {
          std::invoke(FWD(pred), std::as_const(v).value())
        } -> convertible_to_bool;
      });

constexpr inline struct filter_t final {
  // NOTE Optional needs one arguments, expected needs two
  [[nodiscard]] constexpr auto operator()(auto &&...args) const noexcept -> functor<filter_t, decltype(args)...>
    requires(sizeof...(args) >= 1) && (sizeof...(args) < 3)
  {
    return {FWD(args)...};
  }

  struct apply;
} filter = {};

struct filter_t::apply final {
  [[nodiscard]] static constexpr auto operator()(some_expected_pack auto &&v, auto &&pred, auto &&on_err) noexcept
      -> same_monadic_type_as<decltype(v)> auto
    requires invocable_filter<decltype(pred), decltype(on_err), decltype(v)>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    if (std::as_const(v).has_value()) {
      bool const keep{std::as_const(v).value().invoke(FWD(pred))};
      return (keep ? type{std::in_place, FWD(v).value()} : type{std::unexpect, FWD(v).value().invoke(FWD(on_err))});
    }
    return type{std::unexpect, FWD(v).error()};
  }

  [[nodiscard]] static constexpr auto operator()(some_expected_non_pack auto &&v, auto &&pred, auto &&on_err) noexcept
      -> same_monadic_type_as<decltype(v)> auto
    requires invocable_filter<decltype(pred), decltype(on_err), decltype(v)>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    return FWD(v).and_then( //
        [&pred, &on_err](auto &&arg) noexcept -> type {
          bool const keep{std::invoke(FWD(pred), std::as_const(arg))};
          return (keep ? type{std::in_place, FWD(arg)}
                       : type{std::unexpect, std::invoke(FWD(on_err), FWD(arg))}); // GCOVR_EXCL_BR_LINE
        });
  }

  [[nodiscard]] static constexpr auto operator()(some_expected_void auto &&v, auto &&pred, auto &&on_err) noexcept
      -> same_monadic_type_as<decltype(v)> auto
    requires invocable_filter<decltype(pred), decltype(on_err), decltype(v)>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    return FWD(v).and_then( //
        [&pred, &on_err]() noexcept -> type {
          bool const keep{std::invoke(FWD(pred))};
          return (keep ? type{std::in_place}                             //
                       : type{std::unexpect, std::invoke(FWD(on_err))}); // GCOVR_EXCL_BR_LINE
        });
  }

  [[nodiscard]] static constexpr auto operator()(some_optional_pack auto &&v, auto &&pred) noexcept
      -> same_monadic_type_as<decltype(v)> auto
    requires invocable_filter<decltype(pred), void, decltype(v)>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    if (std::as_const(v).has_value()) {
      bool const keep{std::as_const(v).value().invoke(FWD(pred))};
      return (keep ? type{std::in_place, FWD(v).value()} : type{std::nullopt});
    }
    return type{std::nullopt};
  }

  [[nodiscard]] static constexpr auto operator()(some_optional_non_pack auto &&v, auto &&pred) noexcept
      -> same_monadic_type_as<decltype(v)> auto
    requires invocable_filter<decltype(pred), void, decltype(v)>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    return FWD(v).and_then( //
        [&pred](auto &&arg) noexcept -> type {
          bool const keep{std::invoke(FWD(pred), std::as_const(arg))};
          return (keep ? type{std::in_place, FWD(arg)} // GCOVR_EXCL_BR_LINE
                       : type{std::nullopt});
        });
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_FILTER
