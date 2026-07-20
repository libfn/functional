// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_TRANSFORM
#define INCLUDE_FN_TRANSFORM

#include <fn/choice.hpp>
#include <fn/concepts.hpp>
#include <fn/copack.hpp>
#include <fn/expected.hpp>
#include <fn/functional.hpp>
#include <fn/functor.hpp>
#include <fn/optional.hpp>
#include <libfn_version.hpp>

#include <type_traits>

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
concept applicable_transform //
    = (some_expected_non_void<V>//
           && (not some_copack<typename ::std::remove_cvref_t<V>::value_type>) && requires(Fn &&fn, V &&v) {
        {
          ::fn::apply(FWD(fn), FWD(v).value())
        } -> convertible_to_expected<typename ::std::remove_cvref_t<decltype(v)>::error_type>;
      }) || (some_expected<V> && some_copack<typename ::std::remove_cvref_t<V>::value_type> && requires(Fn &&fn, V &&v) {
        {
          FWD(v).value().transform(FWD(fn))
        } -> convertible_to_expected<typename ::std::remove_cvref_t<decltype(v)>::error_type>;
      }) || (some_expected_void<V> && requires(Fn &&fn, V &&v) {
        {
          ::fn::apply(FWD(fn))
        } -> convertible_to_expected<typename ::std::remove_cvref_t<decltype(v)>::error_type>;
      }) || (some_optional<V> //
            && (not some_copack<typename ::std::remove_cvref_t<V>::value_type>) && requires(Fn &&fn, V &&v) {
        {
          ::fn::apply(FWD(fn), FWD(v).value())
        } -> convertible_to_optional;
      }) || (some_optional<V> && some_copack<typename ::std::remove_cvref_t<V>::value_type> && requires(Fn &&fn, V &&v) {
        {
          FWD(v).value().transform(FWD(fn))
        } -> convertible_to_optional;
      }) || (some_choice<V> && requires(Fn &&fn, V &&v) {
        {
          FWD(v).transform(FWD(fn))
        } -> convertible_to_choice;
      }) || (some_just<V> && requires(Fn &&fn, V &&v) {
        {
          FWD(v).transform(FWD(fn))
        } -> same_kind<V>;
      });

namespace detail {
// A copack result of a just's transform is guaranteed a mandate error as just<copack<...>> - the
// verb instead promotes it to the carrier which means exactly that: the choice over the same
// alternatives. Verb-level only, along the canonical isomorphism; the member stays uncoupled.
template <typename T> struct _promoted_choice;
template <typename... Ts> struct _promoted_choice<::fn::copack<Ts...>> {
  using type = ::fn::choice<Ts...>;
};
template <typename Fn, typename... V>
using _promote_t = typename _promoted_choice<::std::remove_cvref_t<typename _apply_result<Fn, V...>::type>>::type;
} // namespace detail

/**
 * @brief Checks if `transform` of a `just` operand promotes the callable's copack result to the
 *        choice over the same alternatives
 *
 * @tparam Fn The function to execute on the value
 * @tparam V The just operand
 */
template <typename Fn, typename V>
concept applicable_transform_promote //
    = (some_just<V> && (not ::std::is_void_v<typename ::std::remove_cvref_t<V>::value_type>) && requires(V &&v) {
        typename detail::_apply_result<Fn, decltype(FWD(v).value())>::type;
        requires some_copack<::std::remove_cvref_t<typename detail::_apply_result<Fn, decltype(FWD(v).value())>::type>>;
        requires not empty_copack<
            ::std::remove_cvref_t<typename detail::_apply_result<Fn, decltype(FWD(v).value())>::type>>;
      }) || (some_just<V> && ::std::is_void_v<typename ::std::remove_cvref_t<V>::value_type> && requires {
        typename detail::_apply_result<Fn>::type;
        requires some_copack<::std::remove_cvref_t<typename detail::_apply_result<Fn>::type>>;
        requires not empty_copack<::std::remove_cvref_t<typename detail::_apply_result<Fn>::type>>;
      });

/**
 * @brief TODO
 */
constexpr inline struct transform_t final {
  /**
   * @brief TODO
   *
   * @param fn TODO
   * @return TODO
   */
  [[nodiscard]] constexpr auto operator()(auto &&fn) const
      noexcept(noexcept(functor<transform_t, decltype(fn)>{FWD(fn)})) -> functor<transform_t, decltype(fn)> //
  {
    return {FWD(fn)};
  }

  struct apply;
} transform = {};

/**
 * @brief TODO
 */
struct transform_t::apply final {
  /**
   * @brief TODO
   *
   * @param v TODO
   * @param fn TODO
   * @return TODO
   */
  template <some_monadic_type V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const noexcept(noexcept(FWD(v).transform(FWD(fn))))
      -> same_kind<V &&> auto
    requires applicable_transform<Fn &&, V &&>
  {
    return FWD(v).transform(FWD(fn));
  }

  // The promotion arms: a copack result over a just operand becomes the choice over the same
  // alternatives - verb-level only, along the canonical isomorphism the member's mandate names
  template <some_monadic_type V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const
      noexcept(noexcept(detail::_promote_t<Fn &&, decltype(FWD(v).value())>{::fn::apply(FWD(fn), FWD(v).value())})) //
      -> some_choice auto
    requires applicable_transform_promote<Fn &&, V &&>
             && (not ::std::is_void_v<typename ::std::remove_cvref_t<V>::value_type>)
  {
    return detail::_promote_t<Fn &&, decltype(FWD(v).value())>{::fn::apply(FWD(fn), FWD(v).value())};
  }

  template <some_monadic_type V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&, Fn &&fn) const            //
      noexcept(noexcept(detail::_promote_t<Fn &&>{::fn::apply(FWD(fn))})) //
      -> some_choice auto
    requires applicable_transform_promote<Fn &&, V &&>
             && ::std::is_void_v<typename ::std::remove_cvref_t<V>::value_type>
  {
    return detail::_promote_t<Fn &&>{::fn::apply(FWD(fn))};
  }
};

} // namespace LIBFN_VERSION
} // namespace fn

#include <fn/detail/macro_end.hpp>

#endif // INCLUDE_FN_TRANSFORM
