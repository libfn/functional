// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_RECOVER
#define INCLUDE_FN_RECOVER

#include <fn/concepts.hpp>
#include <fn/functional.hpp>
#include <fn/functor.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

namespace fn {
/**
 * @brief TODO
 *
 * @tparam Fn TODO
 * @tparam V TODO
 */
// The recovered value builds the RESULT, not merely its value type: for an `optional<T&>` those
// differ - the value type is the referent, which a prvalue can construct, but the result binds a
// reference to it, which a prvalue cannot. The success branch carries the existing value over, so
// that must survive the trip too.
template <typename Fn, typename V>
concept applicable_recover //
    = (some_expected_non_void<V> && requires(Fn &&fn, V &&v) {
        {
          ::fn::apply(FWD(fn), FWD(v).error())
        } -> ::std::convertible_to<typename ::std::remove_cvref_t<V>::value_type>;
        requires ::std::is_constructible_v<::std::remove_cvref_t<V>, ::std::in_place_t,
                                           decltype(::fn::apply(FWD(fn), FWD(v).error()))>;
        requires detail::_relocatable_value<V>;
      }) || (some_expected_void<V> && requires(Fn &&fn, V &&v) {
        { ::fn::apply(FWD(fn), FWD(v).error()) } -> ::std::same_as<void>;
      }) || (some_optional<V> && requires(Fn &&fn, V &&v) {
        { ::fn::apply(FWD(fn)) } -> ::std::convertible_to<typename ::std::remove_cvref_t<V>::value_type>;
        requires ::std::is_constructible_v<::std::remove_cvref_t<V>, ::std::in_place_t, decltype(::fn::apply(FWD(fn)))>;
        requires detail::_relocatable_value<V>;
      });

/**
 * @brief TODO
 */
constexpr inline struct recover_t final {
  /**
   * @brief TODO
   *
   * @param fn TODO
   * @return TODO
   */
  [[nodiscard]] constexpr auto operator()(auto &&fn) const noexcept(noexcept(functor<recover_t, decltype(fn)>{FWD(fn)}))
      -> functor<recover_t, decltype(fn)> //
  {
    return {FWD(fn)};
  }

  struct apply;
} recover = {};

/**
 * @brief TODO
 */
struct recover_t::apply final {
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
          ::fn::is_nothrow_applicable_v<Fn, decltype(FWD(v).error())>
          && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, ::std::in_place_t, decltype(FWD(v).value())>
          && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, ::std::in_place_t,
                                               ::fn::apply_result_t<Fn, decltype(FWD(v).error())>>)
          -> ::std::remove_cvref_t<V>
    requires(not some_identity<V>) && applicable_recover<Fn &&, V &&>
  {
    using type = ::std::remove_cvref_t<V>;
    if (v.has_value()) {
      return type{::std::in_place, FWD(v).value()};
    }
    return type{::std::in_place, ::fn::apply(FWD(fn), FWD(v).error())};
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
      noexcept(::fn::is_nothrow_applicable_v<Fn, decltype(FWD(v).error())>
               && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, ::std::in_place_t>)
          -> ::std::remove_cvref_t<V>
    requires(not some_identity<V>) && applicable_recover<Fn &&, V &&>
  {
    using type = ::std::remove_cvref_t<V>;
    if (v.has_value()) {
      return type{::std::in_place};
    }
    ::fn::apply(FWD(fn), FWD(v).error()); // side-effects only
    return type{::std::in_place};
  }

  /**
   * @brief TODO
   *
   * @param v TODO
   * @return TODO
   */
  // An identity expected has nothing to recover from - the input passes through and the callback
  // is never instantiated
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
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const //
      noexcept(
          ::fn::is_nothrow_applicable_v<Fn>
          && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, ::std::in_place_t, decltype(FWD(v).value())>
          && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, ::std::in_place_t, ::fn::apply_result_t<Fn>>)
          -> ::std::remove_cvref_t<V>
    requires applicable_recover<Fn &&, V &&>
  {
    using type = ::std::remove_cvref_t<V>;
    if (v.has_value()) {
      return type{::std::in_place, FWD(v).value()};
    }
    return type{::std::in_place, ::fn::apply(FWD(fn))};
  }
};

} // namespace fn

#endif // INCLUDE_FN_RECOVER
