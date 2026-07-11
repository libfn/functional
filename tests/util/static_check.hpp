// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef TESTS_UTIL_STATIC_CHECK
#define TESTS_UTIL_STATIC_CHECK

#include <type_traits>
#include <utility>

// Sweeps a verb's invocability across every value category of its operand - the instrument that pins
// down libfn's ref-qualified overload resolution. Deliberately built on `std::is_invocable` alone,
// never on fn's own concepts: a harness that judged fn with fn would share any bug with the code it
// judges, and so be blind to it. Nothing here may include a libfn header.
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
      requires(std::is_invocable_r_v<bool, CheckType, decltype(fns)...>)
    {
      return CheckType()(std::forward<decltype(fns)>(fns)...);
    }

    [[nodiscard]] static constexpr auto not_invocable(auto &&...fns) noexcept -> bool
      requires(std::is_invocable_r_v<bool, CheckType, decltype(fns)...>)
    {
      return not invocable(std::forward<decltype(fns)>(fns)...);
    }
  };
};

template <typename OperandType, template <typename> typename CommandType> struct static_check_with_value_categories {
  template <template <typename> typename... Categories>
  [[nodiscard]] static constexpr auto invocable(auto &&...fns) noexcept -> bool
  {
    return (static_check::bind<CommandType<typename Categories<OperandType>::type>>::invocable(
                std::forward<decltype(fns)>(fns)...)
            && ...);
  }

  template <template <typename> typename... Categories>
  [[nodiscard]] static constexpr auto not_invocable(auto &&...fns) noexcept -> bool
  {
    return (static_check::bind<CommandType<typename Categories<OperandType>::type>>::not_invocable(
                std::forward<decltype(fns)>(fns)...)
            && ...);
  }

  [[nodiscard]] static constexpr auto invocable_with_any(auto &&...fns) noexcept -> bool
  {
    return invocable<lvalue, cvalue, rvalue, clvalue, crvalue, prvalue>(std::forward<decltype(fns)>(fns)...);
  }

  [[nodiscard]] static constexpr auto not_invocable_with_any(auto &&...fns) noexcept -> bool
  {
    return not_invocable<lvalue, cvalue, rvalue, clvalue, crvalue, prvalue>(std::forward<decltype(fns)>(fns)...);
  }
};

template <typename OperationType, typename OperandType> class monadic_static_check {
  // The verb is reached through its `apply`, exactly as `operator|` reaches it, so this asks the same
  // question the pipeline does - but asks it of std::is_invocable rather than fn::monadic_invocable.
  // Each verb's `apply` constrains its own operand to a monadic type, so no separate gate is needed.
  template <typename... HandlerTypes> struct binder {
    template <typename T> struct right {
      [[nodiscard]] constexpr auto operator()(auto &&...fns) const noexcept -> bool
      {
        return std::is_invocable_v<typename OperationType::apply, T, decltype(fns)..., HandlerTypes...>;
      }
    };

    template <typename T> struct left {
      [[nodiscard]] constexpr auto operator()(auto &&...fns) const noexcept -> bool
      {
        return std::is_invocable_v<typename OperationType::apply, T, HandlerTypes..., decltype(fns)...>;
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
    return bind::invocable_with_any(std::forward<decltype(fns)>(fns)...);
  }

  [[nodiscard]] static constexpr auto not_invocable_with_any(auto &&...fns) noexcept -> bool
  {
    return bind::not_invocable_with_any(std::forward<decltype(fns)>(fns)...);
  }

  template <template <typename> typename... Categories>
  [[nodiscard]] static constexpr auto invocable(auto &&...fns) noexcept -> bool
  {
    return bind::template invocable<Categories...>(std::forward<decltype(fns)>(fns)...);
  }

  template <template <typename> typename... Categories>
  [[nodiscard]] static constexpr auto not_invocable(auto &&...fns) noexcept -> bool
  {
    return bind::template not_invocable<Categories...>(std::forward<decltype(fns)>(fns)...);
  }
};

} // namespace util

#endif // TESTS_UTIL_STATIC_CHECK
