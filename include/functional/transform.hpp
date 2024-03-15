// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_TRANSFORM
#define INCLUDE_FUNCTIONAL_TRANSFORM

#include "functional/choice.hpp"
#include "functional/concepts.hpp"
#include "functional/expected.hpp"
#include "functional/functional.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"
#include "functional/optional.hpp"
#include "functional/utility.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace fn {
template <typename Fn, typename V>
concept invocable_transform //
    = (some_expected_non_void<V> && requires(Fn &&fn, V &&v) {
        {
          ::fn::invoke(FWD(fn), FWD(v).value())
        } -> convertible_to_expected<typename std::remove_cvref_t<decltype(v)>::error_type>;
      }) || (some_expected_void<V> && requires(Fn &&fn, V &&v) {
        {
          ::fn::invoke(FWD(fn))
        } -> convertible_to_expected<typename std::remove_cvref_t<decltype(v)>::error_type>;
      }) || (some_optional<V> && requires(Fn &&fn, V &&v) {
        {
          ::fn::invoke(FWD(fn), FWD(v).value())
        } -> convertible_to_optional;
      }) || (some_choice<V> && requires(Fn &&fn, V &&v) {
        {
          FWD(v).transform(FWD(fn))
        } -> convertible_to_choice;
      });

static constexpr struct transform_t final {
  [[nodiscard]] constexpr auto operator()(auto &&fn) const noexcept -> functor<transform_t, decltype(fn)> //
  {
    return {FWD(fn)};
  }

  struct apply;
} transform = {};

struct transform_t::apply final {
  [[nodiscard]] static constexpr auto operator()(some_monadic_type auto &&v, auto &&fn) noexcept
      -> same_kind<decltype(v)> auto
    requires invocable_transform<decltype(fn), decltype(v)>
  {
    return FWD(v).transform(FWD(fn));
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_TRANSFORM
