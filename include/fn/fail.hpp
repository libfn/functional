// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_FAIL
#define INCLUDE_FN_FAIL

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
concept invocable_fail //
    = (some_expected_non_void<V> && requires(Fn &&fn, V &&v) {
        {
          ::fn::invoke(FWD(fn), FWD(v).value())
        } -> ::std::convertible_to<typename ::std::remove_cvref_t<V>::error_type>;
      }) || (some_expected_void<V> && requires(Fn &&fn) {
        { ::fn::invoke(FWD(fn)) } -> ::std::convertible_to<typename ::std::remove_cvref_t<V>::error_type>;
      }) || (some_optional<V> && requires(Fn &&fn, V &&v) {
        { ::fn::invoke(FWD(fn), FWD(v).value()) } -> ::std::same_as<void>;
      });

/**
 * @brief TODO
 */
constexpr inline struct fail_t final {
  [[nodiscard]] constexpr auto operator()(auto &&fn) const noexcept -> functor<fail_t, decltype(fn)> //
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
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const noexcept -> ::std::remove_cvref_t<V>
    requires invocable_fail<Fn &&, V &&>
  {
    using type = ::std::remove_cvref_t<V>;
    if (v.has_value()) {
      return type{::pfn::unexpect, ::fn::invoke(FWD(fn), FWD(v).value())};
    }
    return type{::pfn::unexpect, FWD(v).error()};
  }

  /**
   * @brief TODO
   *
   * @param v TODO
   * @param fn TODO
   * @return TODO
   */
  template <some_expected_void V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const noexcept -> ::std::remove_cvref_t<V>
    requires invocable_fail<Fn &&, V &&>
  {
    using type = ::std::remove_cvref_t<V>;
    if (v.has_value()) {
      return type{::pfn::unexpect, ::fn::invoke(FWD(fn))};
    }
    return type{::pfn::unexpect, FWD(v).error()};
  }

  /**
   * @brief TODO
   *
   * @param v TODO
   * @param fn TODO
   * @return TODO
   */
  template <some_optional V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const noexcept -> ::std::remove_cvref_t<V>
    requires invocable_fail<Fn &&, V &&>
  {
    using type = ::std::remove_cvref_t<V>;
    if (v.has_value()) {
      ::fn::invoke(FWD(fn), FWD(v).value());
    }
    return type{::std::nullopt};
  }

  // No support for choice since there's no error to operate on
  auto operator()(some_choice auto &&v, auto &&...args) const noexcept = delete;
};

} // namespace fn

#endif // INCLUDE_FN_FAIL
