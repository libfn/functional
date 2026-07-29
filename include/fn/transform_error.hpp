// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_TRANSFORM_ERROR
#define INCLUDE_FN_TRANSFORM_ERROR

#include <fn/concepts.hpp>
#include <fn/functional.hpp>
#include <fn/functor.hpp>
#include <libfn_version.hpp>

#include <fn/detail/macro_begin.hpp>

namespace fn {
inline namespace LIBFN_VERSION {
/**
 * @brief Checks if the monadic type can be used with the `transform_error` operation
 *
 * @tparam Fn The function to execute on the error
 * @tparam V The monadic type
 */
template <typename Fn, typename V>
concept applicable_transform_error //
    = (some_expected<V> && (not some_copack<typename ::std::remove_cvref_t<V>::error_type>) && requires(Fn &&fn, V &&v) {
        { ::fn::apply(FWD(fn), FWD(v).error()) } -> convertible_to_unexpected;
      }) || (some_expected<V> && some_copack<typename ::std::remove_cvref_t<V>::error_type> && requires(Fn &&fn, V &&v) {
        {
          FWD(v).error().transform(FWD(fn))
        } -> convertible_to_expected<typename ::std::remove_cvref_t<decltype(v)>::error_type>;
      });

/**
 * @brief Map the error of the `expected`, keeping the carrier's shape
 *
 * The operation that narrows a graded error set: over a graded (copack) error side the matching is
 * exhaustive, so the branches may map diverse alternatives into one common type - the grade
 * collapses to its singular copack - or into a narrower copack. Rejected on `optional`, which has
 * no error value to map; on the identity `expected` it is vacuous - well-formed, with the callback
 * neither invoked nor instantiated.
 *
 * Use through the `fn::transform_error` nielbloid.
 */
constexpr inline struct transform_error_t final {
  /**
   * @brief Map the error of the `expected`, keeping the carrier's shape
   * @param fn The function to execute on the error
   * @return A functor that will execute the function on the error
   */
  [[nodiscard]] constexpr auto operator()(auto &&fn) const
      noexcept(noexcept(functor<transform_error_t, decltype(fn)>{FWD(fn)}))
          -> functor<transform_error_t, decltype(fn)> //
  {
    return {FWD(fn)};
  }

  struct apply;
} transform_error = {};

struct transform_error_t::apply final {
  /**
   * @brief Maps the error through the carrier's own `transform_error` member
   *
   * @param v The monad
   * @param fn The function to apply
   * @return An `expected` with the same value side and the mapped error side
   */
  template <some_expected V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const noexcept(noexcept(FWD(v).transform_error(FWD(fn))))
      -> same_value_kind<V &&> auto
    requires(not some_identity<V>) && applicable_transform_error<Fn &&, V &&>
  {
    return FWD(v).transform_error(FWD(fn));
  }

  // An identity expected's error side is uninhabited - delegate to the vacuous member, which
  // accepts any callback and never instantiates it
  template <some_expected V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const noexcept(noexcept(FWD(v).transform_error(FWD(fn))))
      -> same_value_kind<V &&> auto
    requires some_identity<V>
  {
    return FWD(v).transform_error(FWD(fn));
  }
};

} // namespace LIBFN_VERSION
} // namespace fn

#include <fn/detail/macro_end.hpp>

#endif // INCLUDE_FN_TRANSFORM_ERROR
