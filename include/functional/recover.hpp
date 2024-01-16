// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_RECOVER
#define INCLUDE_FUNCTIONAL_RECOVER

#include "functional/detail/concepts.hpp"
#include "functional/detail/traits.hpp"
#include "functional/functor.hpp"
#include "functional/fwd.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace fn {

constexpr inline struct recover_t final {
  auto operator()(auto &&fn) const noexcept -> functor<recover_t, decltype(fn)>
  {
    return {std::forward<decltype(fn)>(fn)};
  }

  struct apply;
} recover = {};

struct recover_t::apply final {
  static auto operator()(some_expected auto &&v, auto &&fn) noexcept
      -> decltype(auto)
    requires std::invocable<decltype(fn),
                            decltype(std::forward<decltype(v)>(v).error())>
             && (!std::is_void_v<decltype(v.value())>)
  {
    using type = std::remove_cvref_t<decltype(v)>;
    static_assert(
        std::is_convertible_v<
            std::invoke_result_t<
                decltype(fn), decltype(std::forward<decltype(v)>(v).error())>,
            typename type::value_type>);
    return std::forward<decltype(v)>(v).or_else([&fn](auto &&arg) noexcept {
      return type{std::in_place, std::forward<decltype(fn)>(fn)(
                                     std::forward<decltype(arg)>(arg))};
    });
  }

  static auto operator()(some_expected auto &&v, auto &&fn) noexcept
      -> decltype(auto)
    requires std::invocable<decltype(fn),
                            decltype(std::forward<decltype(v)>(v).error())>
             && (std::is_void_v<decltype(v.value())>)
  {
    using type = std::remove_cvref_t<decltype(v)>;
    static_assert(
        std::is_void_v<std::invoke_result_t<
            decltype(fn), decltype(std::forward<decltype(v)>(v).error())>>);
    return std::forward<decltype(v)>(v).or_else([&fn](auto &&arg) noexcept {
      std::forward<decltype(fn)>(fn)(
          std::forward<decltype(arg)>(arg)); // side-effects only
      return type{std::in_place};
    });
  }

  static auto operator()(some_optional auto &&v, auto &&fn) noexcept
      -> decltype(auto)
    requires std::invocable<decltype(fn)>
  {
    using type = std::remove_cvref_t<decltype(v)>;
    static_assert(std::is_convertible_v<std::invoke_result_t<decltype(fn)>,
                                        typename type::value_type>);

    return std::forward<decltype(v)>(v).or_else([&fn]() noexcept {
      return type{std::in_place, std::forward<decltype(fn)>(fn)()};
    });
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_RECOVER
