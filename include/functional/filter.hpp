// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_FILTER
#define INCLUDE_FUNCTIONAL_FILTER

#include "functional/concepts.hpp"
#include "functional/functor.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace fn {
/**
 * @brief Checks if the monadic type can be used with the `filter` operation
 *
 * @tparam Pred The predicate to filter the value
 * @tparam Err The error handler
 * @tparam V The monadic type
 */
template <typename Pred, typename Err, typename V>
concept invocable_filter //
    = (some_expected_non_void<V> && requires(Pred &&pred, Err &&on_err, V &&v) {
        {
          ::fn::invoke(FWD(pred), std::as_const(v).value())
        } -> convertible_to_bool;
        {
          ::fn::invoke(FWD(on_err), FWD(v).value())
        } -> std::convertible_to<typename std::remove_cvref_t<V>::error_type>;
      }) || (some_expected_void<V> && requires(Pred &&pred, Err &&on_err, V &&v) {
        {
          ::fn::invoke(FWD(pred))
        } -> convertible_to_bool;
        {
          ::fn::invoke(FWD(on_err))
        } -> std::convertible_to<typename std::remove_cvref_t<V>::error_type>;
      }) || (some_optional<V> && std::same_as<Err, void> && requires(Pred &&pred, V &&v) {
        {
          ::fn::invoke(FWD(pred), std::as_const(v).value())
        } -> convertible_to_bool;
      });

/**
 * @brief Filter the value of the monadic type using a predicate and an error handler
 *
 * When used on `fn::expected`, this operation takes both a predicate and an error handler.
 * However, when used on `fn::optional`, this operation only takes a predicate.
 *
 * Use through the `fn::filter` nielbloid.
 */
constexpr inline struct filter_t final {
  /**
   * @brief Filter the value of the monadic type using a predicate and an error handler
   * @param pred The predicate to filter the value, takes the value by const reference and returns bool
   * @param on_err The error handler, takes the value by const reference and returns the error type
   * @return A functor that will filter the value of the monadic type
   */
  [[nodiscard]] constexpr auto operator()(auto &&pred, auto &&on_err) const noexcept
      -> functor<filter_t, decltype(pred), decltype(on_err)>
  {
    return {FWD(pred), FWD(on_err)};
  }

  /**
   * @brief Filter the value of the `fn::optional` using a predicate and an error handler
   * @param pred The predicate to filter the value, takes the value by const reference and returns bool
   * @return A functor that will filter the value of the monadic type
   */
  [[nodiscard]] constexpr auto operator()(auto &&pred) const noexcept -> functor<filter_t, decltype(pred)>
  {
    return {FWD(pred)};
  }

  struct apply;
} filter = {};

struct filter_t::apply final {
  [[nodiscard]] static constexpr auto operator()(some_expected_non_void auto &&v, auto &&pred, auto &&on_err) noexcept
      -> same_monadic_type_as<decltype(v)> auto
    requires invocable_filter<decltype(pred), decltype(on_err), decltype(v)>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    if (std::as_const(v).has_value()) {
      bool const keep = ::fn::invoke(FWD(pred), std::as_const(v).value());
      return (keep ? type{std::in_place, FWD(v).value()}
                   : type{std::unexpect, ::fn::invoke(FWD(on_err), FWD(v).value())});
    }
    return FWD(v);
  }

  [[nodiscard]] static constexpr auto operator()(some_expected_void auto &&v, auto &&pred, auto &&on_err) noexcept
      -> same_monadic_type_as<decltype(v)> auto
    requires invocable_filter<decltype(pred), decltype(on_err), decltype(v)>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    if (std::as_const(v).has_value()) {
      bool const keep = ::fn::invoke(FWD(pred));
      return (keep ? type{std::in_place} //
                   : type{std::unexpect, ::fn::invoke(FWD(on_err))});
    }
    return FWD(v);
  }

  [[nodiscard]] static constexpr auto operator()(some_optional auto &&v, auto &&pred) noexcept
      -> same_monadic_type_as<decltype(v)> auto
    requires invocable_filter<decltype(pred), void, decltype(v)>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    if (std::as_const(v).has_value()) {
      bool const keep = ::fn::invoke(FWD(pred), std::as_const(v).value());
      return (keep ? type{std::in_place, FWD(v).value()} //
                   : type{std::nullopt});
    }
    return FWD(v);
  }

  // No support for choice since there's no error to operate on
  static auto operator()(some_choice auto &&v, auto &&...args) noexcept = delete;
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_FILTER
