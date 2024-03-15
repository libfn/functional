// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef TESTS_UTIL_STATIC_CHECK
#define TESTS_UTIL_STATIC_CHECK

#include "functional/concepts.hpp"
#include "functional/functor.hpp"

#include <type_traits>

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

struct static_check {
  template <typename CheckType> struct bind {
    [[nodiscard]] static constexpr auto invocable(auto &&...fns) noexcept -> bool
      requires(fn::is_invocable_r<bool, CheckType, decltype(fns)...>::value)
    {
      return CheckType()(FWD(fns)...);
    }

    [[nodiscard]] static constexpr auto not_invocable(auto &&...fns) noexcept -> bool
      requires(fn::is_invocable_r<bool, CheckType, decltype(fns)...>::value)
    {
      return not invocable(FWD(fns)...);
    }
  };
};

template <typename OperandType, template <typename> typename CommandType> struct static_check_with_value_categories {
  template <template <typename> typename... Categories>
  [[nodiscard]] static constexpr auto invocable(auto &&...fns) noexcept -> bool
  {
    return (static_check::bind<CommandType<typename Categories<OperandType>::type>>::invocable(FWD(fns)...) && ...);
  }

  template <template <typename> typename... Categories>
  [[nodiscard]] static constexpr auto not_invocable(auto &&...fns) noexcept -> bool
  {
    return (static_check::bind<CommandType<typename Categories<OperandType>::type>>::not_invocable(FWD(fns)...) && ...);
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

template <typename OperationType, typename OperandType> class monadic_static_check {
  template <typename... HandlerTypes> struct binder {
    template <typename T> struct right {
      [[nodiscard]] static constexpr auto operator()(auto &&...fns) noexcept -> bool
      {
        return fn::monadic_invocable<OperationType, T, decltype(fns)..., HandlerTypes...>;
      }
    };

    template <typename T> struct left {
      [[nodiscard]] static constexpr auto operator()(auto &&...fns) noexcept -> bool
      {
        return fn::monadic_invocable<OperationType, T, HandlerTypes..., decltype(fns)...>;
      }
    };
  };

public:
  template <typename... HandlerTypes>
  using bind_right = static_check_with_value_categories<OperandType, binder<HandlerTypes...>::template right>;

  template <typename... HandlerTypes>
  using bind_left = static_check_with_value_categories<OperandType, binder<HandlerTypes...>::template left>;

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
