// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef TESTS_UTIL_STATIC_CHECK
#define TESTS_UTIL_STATIC_CHECK

#include "functional/detail/concepts.hpp"
#include "functional/functor.hpp"

namespace util {

struct static_check {
  template <typename OperationType, typename... HandlerTypes> struct bind_left {
    template <typename OperandType> static constexpr bool invocable(auto &&fn)
    {
      return fn::monadic_invocable<OperationType, OperandType, HandlerTypes..., decltype(fn)>;
    }
  };

  template <typename OperationType, typename... HandlerTypes> struct bind_right {
    template <typename OperandType> static constexpr bool invocable(auto &&fn)
    {
      return fn::monadic_invocable<OperationType, OperandType, decltype(fn), HandlerTypes...>;
    }
  };
};

} // namespace util

#endif // TESTS_UTIL_STATIC_CHECK
