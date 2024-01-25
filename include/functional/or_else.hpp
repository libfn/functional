// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_OR_ELSE
#define INCLUDE_FUNCTIONAL_OR_ELSE

#include "functional/concepts.hpp"
#include "functional/detail/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"
#include "functional/utility.hpp"

#include <concepts>
#include <utility>

namespace fn {
template <typename Fn, typename V>
concept invocable_or_else //
    = (some_expected<V> && requires(Fn &&fn, V &&v) {
        {
          std::invoke(FWD(fn), FWD(v).error())
        } -> same_value_kind<V>;
      }) || (some_optional<V> && requires(Fn &&fn) {
        {
          std::invoke(FWD(fn))
        } -> same_value_kind<V>;
      });

constexpr inline struct or_else_t final {
  constexpr auto operator()(auto &&fn) const noexcept
      -> functor<or_else_t, decltype(fn)>
  {
    return {FWD(fn)};
  }

  struct apply;
} or_else = {};

struct or_else_t::apply final {
  static constexpr auto operator()(some_monadic_type auto &&v,
                                   auto &&fn) noexcept
      -> same_value_kind<decltype(v)> auto
    requires invocable_or_else<decltype(fn) &&, decltype(v) &&>
  {
    return FWD(v).or_else(FWD(fn));
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_OR_ELSE
