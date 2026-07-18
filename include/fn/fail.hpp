// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_FAIL
#define INCLUDE_FN_FAIL

#include <fn/concepts.hpp>
#include <fn/functor.hpp>
#include <fn/optional.hpp>

#include <concepts>
#include <type_traits>

namespace fn {
/**
 * @brief TODO
 *
 * @tparam Fn TODO
 * @tparam V TODO
 * @param fn TODO
 * @param v TODO
 */
template <typename Fn, typename V>
concept applicable_fail //
    = (some_expected_non_void<V> && requires(Fn &&fn, V &&v) {
        {
          ::fn::apply(FWD(fn), FWD(v).value())
        } -> ::std::convertible_to<typename ::std::remove_cvref_t<V>::error_type>;
        requires detail::_relocatable_error<V>; // the error branch carries the existing error over
      }) || (some_expected_void<V> && requires(Fn &&fn) {
        { ::fn::apply(FWD(fn)) } -> ::std::convertible_to<typename ::std::remove_cvref_t<V>::error_type>;
        requires detail::_relocatable_error<V>;
      }) || (some_optional<V> && requires(Fn &&fn, V &&v) {
        { ::fn::apply(FWD(fn), FWD(v).value()) } -> ::std::same_as<void>;
      });

/**
 * @brief TODO
 */
constexpr inline struct fail_t final {
  [[nodiscard]] constexpr auto operator()(auto &&fn) const noexcept(noexcept(functor<fail_t, decltype(fn)>{FWD(fn)}))
      -> functor<fail_t, decltype(fn)> //
  {
    return {FWD(fn)};
  }

  struct apply;
} fail = {};

/**
 * @brief TODO
 */
struct fail_t::apply final {
  /**
   * @brief TODO
   *
   * @param v TODO
   * @param fn TODO
   * @return TODO
   */
  template <some_expected_non_void V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const //
      noexcept(
          ::fn::is_nothrow_applicable_v<Fn, decltype(FWD(v).value())>
          && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, ::fn::unexpect_t,
                                               ::fn::apply_result_t<Fn, decltype(FWD(v).value())>>
          && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, ::fn::unexpect_t, decltype(FWD(v).error())>)
          -> ::std::remove_cvref_t<V>
      // An identity expected cannot fail - there is no error to fail into - so the refusal is
      // stated, not left to the impossible conversion
    requires(not some_identity<V>) && applicable_fail<Fn &&, V &&>
  {
    using type = ::std::remove_cvref_t<V>;
    if (v.has_value()) {
      return type{::fn::unexpect, ::fn::apply(FWD(fn), FWD(v).value())};
    }
    return type{::fn::unexpect, FWD(v).error()};
  }

  /**
   * @brief TODO
   *
   * @param v TODO
   * @param fn TODO
   * @return TODO
   */
  template <some_expected_void V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const //
      noexcept(
          ::fn::is_nothrow_applicable_v<Fn>
          && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, ::fn::unexpect_t, ::fn::apply_result_t<Fn>>
          && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, ::fn::unexpect_t, decltype(FWD(v).error())>)
          -> ::std::remove_cvref_t<V>
    requires(not some_identity<V>) && applicable_fail<Fn &&, V &&>
  {
    using type = ::std::remove_cvref_t<V>;
    if (v.has_value()) {
      return type{::fn::unexpect, ::fn::apply(FWD(fn))};
    }
    return type{::fn::unexpect, FWD(v).error()};
  }

  /**
   * @brief TODO
   *
   * @param v TODO
   * @param fn TODO
   * @return TODO
   */
  template <some_optional V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const //
      noexcept(::fn::is_nothrow_applicable_v<Fn, decltype(FWD(v).value())>
               && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, ::std::nullopt_t>)
          -> ::std::remove_cvref_t<V>
    requires applicable_fail<Fn &&, V &&>
  {
    using type = ::std::remove_cvref_t<V>;
    if (v.has_value()) {
      ::fn::apply(FWD(fn), FWD(v).value());
    }
    return type{::std::nullopt};
  }
};

} // namespace fn

#endif // INCLUDE_FN_FAIL
