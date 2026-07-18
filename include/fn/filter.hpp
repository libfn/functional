// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_FILTER
#define INCLUDE_FN_FILTER

#include <fn/concepts.hpp>
#include <fn/functor.hpp>

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
// A rejected operand is returned whole (`return FWD(v)`), and a kept one is rebuilt around its
// existing value - so both sides must survive the trip.
template <typename Pred, typename Err, typename V>
concept applicable_filter //
    = (some_expected_non_void<V> && requires(Pred &&pred, Err &&on_err, V &&v) {
        { ::fn::apply(FWD(pred), ::std::as_const(v).value()) } -> convertible_to_bool;
        {
          ::fn::apply(FWD(on_err), FWD(v).value())
        } -> ::std::convertible_to<typename ::std::remove_cvref_t<V>::error_type>;
        requires detail::_relocatable<V> && detail::_relocatable_value<V>;
      }) || (some_expected_void<V> && requires(Pred &&pred, Err &&on_err, V &&v) {
        { ::fn::apply(FWD(pred)) } -> convertible_to_bool;
        { ::fn::apply(FWD(on_err)) } -> ::std::convertible_to<typename ::std::remove_cvref_t<V>::error_type>;
        requires detail::_relocatable<V>;
      }) || (some_optional<V> && ::std::same_as<Err, void> && requires(Pred &&pred, V &&v) {
        { ::fn::apply(FWD(pred), ::std::as_const(v).value()) } -> convertible_to_bool;
        requires detail::_relocatable<V> && detail::_relocatable_value<V>;
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
  [[nodiscard]] constexpr auto operator()(auto &&pred, auto &&on_err) const
      noexcept(noexcept(functor<filter_t, decltype(pred), decltype(on_err)>{FWD(pred), FWD(on_err)}))
          -> functor<filter_t, decltype(pred), decltype(on_err)>
  {
    return {FWD(pred), FWD(on_err)};
  }

  /**
   * @brief Filter the value of the `fn::optional` using a predicate and an error handler
   * @param pred The predicate to filter the value, takes the value by const reference and returns bool
   * @return A functor that will filter the value of the monadic type
   */
  [[nodiscard]] constexpr auto operator()(auto &&pred) const
      noexcept(noexcept(functor<filter_t, decltype(pred)>{FWD(pred)})) -> functor<filter_t, decltype(pred)>
  {
    return {FWD(pred)};
  }

  struct apply;
} filter = {};

/**
 * @brief TODO
 */
struct filter_t::apply final {
  /**
   * @brief TODO
   *
   * @param v TODO
   * @param pred TODO
   * @param on_err TODO
   * @return TODO
   */
  template <some_expected_non_void V, typename Pred, typename OnErr>
  [[nodiscard]] constexpr auto operator()(V &&v, Pred &&pred, OnErr &&on_err) const //
      noexcept(
          ::fn::is_nothrow_applicable_v<Pred, decltype(::std::as_const(v).value())>
          && ::fn::is_nothrow_applicable_v<OnErr, decltype(FWD(v).value())>
          && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, ::std::in_place_t, decltype(FWD(v).value())>
          && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, ::fn::unexpect_t,
                                               ::fn::apply_result_t<OnErr, decltype(FWD(v).value())>>
          && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, V>) -> ::std::remove_cvref_t<V>
      // An identity expected cannot reject a value - there is no error to fail into - so the refusal
      // is stated, not left to the impossible conversion
    requires(not some_identity<V>) && applicable_filter<Pred &&, OnErr &&, V &&>
  {
    using type = ::std::remove_cvref_t<V>;
    if (::std::as_const(v).has_value()) {
      bool const keep = ::fn::apply(FWD(pred), ::std::as_const(v).value());
      return (keep ? type{::std::in_place, FWD(v).value()}
                   : type{::fn::unexpect, ::fn::apply(FWD(on_err), FWD(v).value())});
    }
    return FWD(v);
  }

  /**
   * @brief TODO
   *
   * @param v TODO
   * @param pred TODO
   * @param on_err TODO
   * @return TODO
   */
  template <some_expected_void V, typename Pred, typename OnErr>
  [[nodiscard]] constexpr auto operator()(V &&v, Pred &&pred, OnErr &&on_err) const //
      noexcept(
          ::fn::is_nothrow_applicable_v<Pred> && ::fn::is_nothrow_applicable_v<OnErr>
          && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, ::std::in_place_t>
          && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, ::fn::unexpect_t, ::fn::apply_result_t<OnErr>>
          && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, V>) -> ::std::remove_cvref_t<V>
    requires(not some_identity<V>) && applicable_filter<Pred &&, OnErr &&, V &&>
  {
    using type = ::std::remove_cvref_t<V>;
    if (::std::as_const(v).has_value()) {
      bool const keep = ::fn::apply(FWD(pred));
      return (keep ? type{::std::in_place} //
                   : type{::fn::unexpect, ::fn::apply(FWD(on_err))});
    }
    return FWD(v);
  }

  /**
   * @brief TODO
   *
   * @param v TODO
   * @param pred TODO
   * @return TODO
   */
  template <some_optional V, typename Pred>
  [[nodiscard]] constexpr auto operator()(V &&v, Pred &&pred) const //
      noexcept(
          ::fn::is_nothrow_applicable_v<Pred, decltype(::std::as_const(v).value())>
          && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, ::std::in_place_t, decltype(FWD(v).value())>
          && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, ::std::nullopt_t>
          && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, V>) -> ::std::remove_cvref_t<V>
    requires applicable_filter<Pred &&, void, V &&>
  {
    using type = ::std::remove_cvref_t<V>;
    if (::std::as_const(v).has_value()) {
      bool const keep = ::fn::apply(FWD(pred), ::std::as_const(v).value());
      return (keep ? type{::std::in_place, FWD(v).value()} //
                   : type{::std::nullopt});
    }
    return FWD(v);
  }
};

} // namespace fn

#endif // INCLUDE_FN_FILTER
