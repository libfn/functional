// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_OR_ELSE
#define INCLUDE_FN_OR_ELSE

#include <fn/concepts.hpp>
#include <fn/functional.hpp>
#include <fn/functor.hpp>
#include <libfn_version.hpp>

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
concept applicable_or_else //
    = (some_expected<V> && requires(V &&v) {
        typename detail::_or_else_dispatch<typename ::std::remove_cvref_t<V>::value_type, Fn,
                                           decltype(FWD(v).error())>::type;
        requires same_value_kind<V, typename detail::_or_else_dispatch<typename ::std::remove_cvref_t<V>::value_type,
                                                                       Fn, decltype(FWD(v).error())>::type>;
      }) || (some_expected<V> //
         && some_copack<typename ::std::remove_cvref_t<V>::value_type> && requires(V &&v) {
        typename detail::_or_else_dispatch<typename ::std::remove_cvref_t<V>::value_type, Fn,
                                           decltype(FWD(v).error())>::type;
        requires some_expected<typename detail::_or_else_dispatch<typename ::std::remove_cvref_t<V>::value_type, Fn,
                                                                  decltype(FWD(v).error())>::type>;
      }) || (some_optional<V> && requires(Fn &&fn) {
        {
          ::fn::apply(FWD(fn))
        } -> same_value_kind<V>;
      }) || (some_optional<V>  //
         && some_copack<typename ::std::remove_cvref_t<V>::value_type> && requires(Fn &&fn, V &&v) {
        {
          ::fn::apply(FWD(fn))
        } -> some_optional;
      });

/**
 * @brief TODO
 */
constexpr inline struct or_else_t final {
  /**
   * @brief TODO
   *
   * @param fn TODO
   * @return TODO
   */
  [[nodiscard]] constexpr auto operator()(auto &&fn) const noexcept(noexcept(functor<or_else_t, decltype(fn)>{FWD(fn)}))
      -> functor<or_else_t, decltype(fn)> //
  {
    return {FWD(fn)};
  }

  struct apply;
} or_else = {};

/**
 * @brief TODO
 */
struct or_else_t::apply final {
  /**
   * @brief TODO
   *
   * @param v TODO
   * @param fn TODO
   * @return TODO
   */
  template <some_monadic_type V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const //
      noexcept(noexcept(FWD(v).or_else(FWD(fn))))               //
      -> same_value_kind<V &&> auto
    requires(not some_identity<V>) && applicable_or_else<Fn &&, V &&>
  {
    return FWD(v).or_else(FWD(fn));
  }

  /**
   * @brief TODO
   *
   * @param v TODO
   * @param fn TODO
   * @return TODO
   */
  // An identity expected's error side is uninhabited - delegate to the vacuous member, which
  // accepts any callback and never instantiates it
  template <some_expected V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const //
      noexcept(noexcept(FWD(v).or_else(FWD(fn))))               //
      -> same_value_kind<V &&> auto
    requires some_identity<V>
  {
    return FWD(v).or_else(FWD(fn));
  }
};

} // namespace LIBFN_VERSION
} // namespace fn

#include <fn/detail/macro_end.hpp>

#endif // INCLUDE_FN_OR_ELSE
