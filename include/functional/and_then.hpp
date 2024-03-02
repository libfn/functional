// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_AND_THEN
#define INCLUDE_FUNCTIONAL_AND_THEN

#include "functional/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"
#include "functional/utility.hpp"

#include <concepts>
#include <utility>

namespace fn {
template <typename Fn, typename V>
concept invocable_and_then //
    = (some_expected_pack<V> && requires(Fn &&fn, V &&v) {
        {
          FWD(v).value().invoke(FWD(fn))
        } -> same_kind<V>;
      }) || (some_expected_non_pack<V> && requires(Fn &&fn, V &&v) {
        {
          std::invoke(FWD(fn), FWD(v).value())
        } -> same_kind<V>;
      }) || (some_expected_void<V> && requires(Fn &&fn) {
        {
          std::invoke(FWD(fn))
        } -> same_kind<V>;
      }) || (some_optional_pack<V> && requires(Fn &&fn, V &&v) {
        {
          FWD(v).value().invoke(FWD(fn))
        } -> same_kind<V>;
      }) || (some_optional_non_pack<V> && requires(Fn &&fn, V &&v) {
        {
          std::invoke(FWD(fn), FWD(v).value())
        } -> same_kind<V>;
      }) || (some_choice<V> && requires(Fn &&fn, V &&v) {
        {
          FWD(v).transform_to(FWD(fn))
        } -> same_kind<V>;
      });

constexpr inline struct and_then_t final {
  [[nodiscard]] constexpr auto operator()(auto &&fn) const noexcept -> functor<and_then_t, decltype(fn)> //
  {
    return {FWD(fn)};
  }

  struct apply;
} and_then = {};

struct and_then_t::apply final {
  [[nodiscard]] static constexpr auto operator()(some_monadic_type auto &&v, auto &&fn) noexcept
      -> same_kind<decltype(v)> auto
    requires invocable_and_then<decltype(fn), decltype(v)>
  {
    return FWD(v).and_then(FWD(fn));
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_AND_THEN
