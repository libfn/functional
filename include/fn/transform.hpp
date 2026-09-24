// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_TRANSFORM
#define INCLUDE_FN_TRANSFORM

#include <fn/concepts.hpp>
#include <fn/copack.hpp>
#include <fn/expected.hpp>
#include <fn/functional.hpp>
#include <fn/functor.hpp>
#include <fn/just.hpp>
#include <fn/optional.hpp>
#include <libfn_version.hpp>

#include <type_traits>

#include <fn/detail/macro_begin.hpp>

namespace fn {
inline namespace LIBFN_VERSION {
/**
 * @brief Checks if the monadic type can be used with the `transform` operation
 *
 * @tparam Fn The function to execute on the value
 * @tparam V The monadic type
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
      }) || (some_just<V> && requires(Fn &&fn, V &&v) {
        {
          FWD(v).transform(FWD(fn))
        } -> same_kind<V>;
      });

/**
 * @brief Map the value of the monadic type, keeping the carrier's shape
 *
 * The callable's result becomes the new value, in the same carrier family; a bare (non-monadic)
 * callback result belongs here rather than to `and_then`. Over a copack-valued carrier the
 * callable is dispatched per alternative, heterogeneous branch results joining into a normalized
 * copack. Over a `just`, a copack result yields the `choice` over its alternatives.
 *
 * Use through the `fn::transform` nielbloid.
 */
constexpr inline struct transform_t final {
  /**
   * @brief Map the value of the monadic type, keeping the carrier's shape
   * @param fn The function to execute on the value
   * @return A functor that will execute the function on the value
   */
  [[nodiscard]] constexpr auto operator()(auto &&fn) const
      noexcept(noexcept(functor<transform_t, decltype(fn)>{FWD(fn)})) -> functor<transform_t, decltype(fn)> //
  {
    return {FWD(fn)};
  }

  struct apply;
} transform = {}; ///< Maps the value, staying in the carrier: `x | transform(f)`

struct transform_t::apply final {
  /**
   * @brief Maps the value through the carrier's own `transform` member
   *
   * @param v The monad
   * @param fn The function to apply
   * @return A carrier of the same kind, holding the mapped value
   */
  template <some_monadic_type V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const noexcept(noexcept(FWD(v).transform(FWD(fn))))
      -> same_kind<V &&> auto
    requires applicable_transform<Fn &&, V &&>
  {
    return FWD(v).transform(FWD(fn));
  }

  // An uninhabited value side leaves no value to map - delegate to the member, which is the
  // identity there and neither invokes nor instantiates the callback. The verb must reach wherever
  // the member does, so this arm mirrors transform_error's over an uninhabited error side.
  template <some_monadic_type V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const noexcept(noexcept(FWD(v).transform(FWD(fn))))
      -> same_kind<V &&> auto
    requires some_empty_value<V>
  {
    return FWD(v).transform(FWD(fn));
  }
};

} // namespace LIBFN_VERSION
} // namespace fn

#include <fn/detail/macro_end.hpp>

#endif // INCLUDE_FN_TRANSFORM
