// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_FAIL
#define INCLUDE_FUNCTIONAL_FAIL

#include "functional/detail/concepts.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"
#include "functional/utility.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace fn {

constexpr inline struct fail_t final {
  auto operator()(auto &&fn) const noexcept -> functor<fail_t, decltype(fn)>
  {
    return {FWD(fn)};
  }

  struct apply;
} fail = {};

struct fail_t::apply final {
  static auto operator()(some_expected auto &&v, auto &&fn) noexcept
      -> decltype(auto)
    requires std::invocable<decltype(fn), decltype(FWD(v).value())>
             && (!std::is_void_v<decltype(v.value())>)
  {
    using type = std::remove_cvref_t<decltype(v)>;
    static_assert(std::is_convertible_v<
                  std::invoke_result_t<decltype(fn), decltype(FWD(v).value())>,
                  typename type::error_type>);
    return FWD(v).and_then( //
        [&fn](auto &&arg) noexcept -> type {
          return std::unexpected<typename type::error_type>{FWD(fn)(FWD(arg))};
        });
  }

  static auto operator()(some_expected auto &&v, auto &&fn) noexcept
      -> decltype(auto)
    requires std::invocable<decltype(fn)>
             && (std::is_void_v<decltype(v.value())>)
  {
    using type = std::remove_cvref_t<decltype(v)>;
    static_assert(std::is_convertible_v<std::invoke_result_t<decltype(fn)>,
                                        typename type::error_type>);
    return FWD(v).and_then( //
        [&fn]() noexcept -> type {
          return std::unexpected<typename type::error_type>{FWD(fn)()};
        });
  }

  static auto operator()(some_optional auto &&v, auto &&fn) noexcept
      -> decltype(auto)
    requires std::invocable<decltype(fn), decltype(FWD(v).value())>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    static_assert(
        std::is_void_v<
            std::invoke_result_t<decltype(fn), decltype(FWD(v).value())>>);
    if (v.has_value()) {
      FWD(fn)(FWD(v).value()); // side-effects only
    }
    return type{std::nullopt};
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_FAIL
