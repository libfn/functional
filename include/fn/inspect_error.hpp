// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_INSPECT_ERROR
#define INCLUDE_FN_INSPECT_ERROR

#include <fn/functional.hpp>
#include <fn/functor.hpp>
#include <libfn_version.hpp>

#include <concepts>
#include <utility>

#include <fn/detail/macro_begin.hpp>

namespace fn {
inline namespace LIBFN_VERSION {
/**
 * @brief TODO
 *
 * @tparam Fn TODO
 * @tparam V TODO
 */
template <typename Fn, typename V>
concept applicable_inspect_error //
    = (some_expected<V> && requires(Fn &&fn, V &&v) {
        { ::fn::apply(FWD(fn), ::std::as_const(v).error()) } -> ::std::same_as<void>;
      }) || (some_optional<V> && requires(Fn &&fn) {
        { ::fn::apply(FWD(fn)) } -> ::std::same_as<void>;
      });

/**
 * @brief TODO
 */
constexpr inline struct inspect_error_t final {
  /**
   * @brief TODO
   *
   * @param fn TODO
   * @return TODO
   */
  [[nodiscard]] constexpr auto operator()(auto &&fn) const
      noexcept(noexcept(functor<inspect_error_t, decltype(fn)>{FWD(fn)})) -> functor<inspect_error_t, decltype(fn)> //
  {
    return {FWD(fn)};
  }

  struct apply;
} inspect_error = {};

/**
 * @brief TODO
 */
struct inspect_error_t::apply final {
  /**
   * @brief TODO
   *
   * @param v TODO
   * @param fn TODO
   * @return TODO
   */
  template <some_expected V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const
      noexcept(::fn::is_nothrow_applicable_v<Fn, decltype(::std::as_const(v).error())>) -> V &&
    requires(not some_identity<V>) && applicable_inspect_error<Fn &&, V &&>
  {
    if (not v.has_value()) {
      ::fn::apply(FWD(fn), ::std::as_const(v).error()); // side-effects only
    }
    return FWD(v);
  }

  /**
   * @brief TODO
   *
   * @param v TODO
   * @return TODO
   */
  // An identity expected's error side is uninhabited - there is nothing to observe, the operand
  // passes through and the callback is never instantiated
  template <some_expected V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&) const noexcept -> V &&
    requires some_identity<V>
  {
    return FWD(v);
  }

  /**
   * @brief TODO
   *
   * @param v TODO
   * @param fn TODO
   * @return TODO
   */
  template <some_optional V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const noexcept(::fn::is_nothrow_applicable_v<Fn>) -> V &&
    requires applicable_inspect_error<Fn &&, V &&>
  {
    if (not v.has_value()) {
      ::fn::apply(FWD(fn)); // side-effects only
    }
    return FWD(v);
  }
};

} // namespace LIBFN_VERSION
} // namespace fn

#include <fn/detail/macro_end.hpp>

#endif // INCLUDE_FN_INSPECT_ERROR
