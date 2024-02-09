// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef TESTS_UTIL_STATIC_CHECK
#define TESTS_UTIL_STATIC_CHECK

#include "functional/concepts.hpp"
#include "functional/functor.hpp"

namespace util {

template <typename OperandType> struct lvalue {
  using type = std::add_lvalue_reference_t<OperandType>;
};

template <typename OperandType> struct rvalue {
  using type = std::add_rvalue_reference_t<OperandType>;
};

template <typename OperandType> struct clvalue {
  using type = std::add_lvalue_reference_t<std::add_const_t<OperandType>>;
};

template <typename OperandType> struct crvalue {
  using type = std::add_rvalue_reference_t<std::add_const_t<OperandType>>;
};

template <typename OperandType> struct cvalue {
  using type = std::add_const_t<OperandType>;
};

template <typename OperandType> struct prvalue {
  using type = OperandType;
};

template <typename OperationType, typename OperandType> struct static_check {
  template <typename... HandlerTypes> struct bind_right {
    template <template <typename> typename... Categories>
    [[nodiscard]] static constexpr auto invocable(auto &&...fns) noexcept -> bool
    {
      return (fn::monadic_invocable< //
                  OperationType, typename Categories<OperandType>::type, decltype(fns)..., HandlerTypes...>
              && ...);
    }

    template <template <typename> typename... Categories>
    [[nodiscard]] static constexpr auto not_invocable(auto &&...fns) noexcept -> bool
    {
      return (not fn::monadic_invocable< //
                  OperationType, typename Categories<OperandType>::type, decltype(fns)..., HandlerTypes...>
              && ...);
    }

    [[nodiscard]] static constexpr auto invocable_with_any(auto &&...fns) noexcept -> bool
    {
      return invocable<lvalue, cvalue, rvalue, clvalue, crvalue, prvalue>(FWD(fns)...);
    }

    [[nodiscard]] static constexpr auto not_invocable_with_any(auto &&...fns) noexcept -> bool
    {
      return not_invocable<lvalue, cvalue, rvalue, clvalue, crvalue, prvalue>(FWD(fns)...);
    }
  };

  template <typename... HandlerTypes> struct bind_left {
    template <template <typename> typename... Categories>
    [[nodiscard]] static constexpr auto invocable(auto &&...fns) noexcept -> bool
    {
      return (fn::monadic_invocable< //
                  OperationType, typename Categories<OperandType>::type, HandlerTypes..., decltype(fns)...>
              && ...);
    }

    template <template <typename> typename... Categories>
    [[nodiscard]] static constexpr auto not_invocable(auto &&...fns) noexcept -> bool
    {
      return (not fn::monadic_invocable< //
                  OperationType, typename Categories<OperandType>::type, HandlerTypes..., decltype(fns)...>
              && ...);
    }

    [[nodiscard]] static constexpr auto invocable_with_any(auto &&...fns) noexcept -> bool
    {
      return invocable<lvalue, cvalue, rvalue, clvalue, crvalue, prvalue>(FWD(fns)...);
    }

    [[nodiscard]] static constexpr auto not_invocable_with_any(auto &&...fns) noexcept -> bool
    {
      return not_invocable<lvalue, cvalue, rvalue, clvalue, crvalue, prvalue>(FWD(fns)...);
    }
  };

  using bind = bind_left<>;

  [[nodiscard]] static constexpr auto invocable_with_any(auto &&...fns) noexcept -> bool
  {
    return bind::invocable_with_any(FWD(fns)...);
  }

  [[nodiscard]] static constexpr auto not_invocable_with_any(auto &&...fns) noexcept -> bool
  {
    return bind::not_invocable_with_any(FWD(fns)...);
  }

  template <template <typename> typename... Categories>
  [[nodiscard]] static constexpr auto invocable(auto &&...fns) noexcept -> bool
  {
    return bind::template invocable<Categories...>(FWD(fns)...);
  }

  template <template <typename> typename... Categories>
  [[nodiscard]] static constexpr auto not_invocable(auto &&...fns) noexcept -> bool
  {
    return bind::template not_invocable<Categories...>(FWD(fns)...);
  }
};

} // namespace util

#endif // TESTS_UTIL_STATIC_CHECK
