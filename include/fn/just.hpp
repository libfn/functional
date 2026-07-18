// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_JUST
#define INCLUDE_FN_JUST

#include <fn/detail/functional.hpp>
#include <fn/detail/fwd.hpp>
#include <fn/detail/variadic_union.hpp>

#include <memory>
#include <type_traits>
#include <utility>

namespace fn {

/**
 * @brief Checks if a type is a `just` (with any payload)
 *
 * @tparam T Type to check, possibly cv-ref qualified
 */
template <typename T>
concept some_just = detail::_some_just<T>;

namespace detail {
// transform's result is direct-non-list-initialized from the thunk's invocation: no extra move,
// and immovable result types work (the pattern of pfn's _expected_from_invoke)
constexpr inline struct _just_from_invoke_t {
  explicit _just_from_invoke_t() = default;
} _just_from_invoke{};
} // namespace detail

// Declared ahead of the primary, whose transform names it - without this the primary would be
// instantiated for void first, and the specialization below would come too late.
template <> struct just<void>;

/**
 * @brief The identity carrier: holds exactly one value of `T`, always present
 *
 * The unit of the monadic family as a value: no error channel, no empty state, one payload. As
 * trivial as `T` permits in copying, moving, assignment and destruction; a structural type when
 * `T` is one. A copack payload is deliberately rejected - dispatch granularity belongs to the
 * engine, uniform per payload, so identity over a copack is spelled `choice`; a `choice` payload
 * stays legal, an atom here as everywhere.
 *
 * @tparam T Payload type
 */
template <typename T> struct just {
  static_assert(not detail::_some_copack<T>, "a just over a copack is spelled choice");
  static_assert(not ::std::is_reference_v<T>);
  static_assert(not ::std::is_array_v<T>);
  static_assert(not detail::_some_in_place_type<T>);
  static_assert(::std::is_same_v<T, ::std::remove_cv_t<T>>);

  using value_type = T;

  T v_{};

  constexpr just() = default;
  constexpr just(just const &) = default;
  constexpr just(just &&) = default;
  constexpr just &operator=(just const &) = default;
  constexpr just &operator=(just &&) = default;
  constexpr ~just() = default;

  /**
   * @brief Constructs the payload from a value
   *
   * @param v Value to initialize the payload from
   */
  template <typename U>
  constexpr just(U &&v) // NOSONAR cpp:S1709 implicit arm of the explicit pair
      noexcept(::std::is_nothrow_constructible_v<T, decltype(v)>)
    requires(not some_just<::std::remove_cvref_t<U>>) && (not detail::_some_in_place_type<::std::remove_cvref_t<U>>)
            && ::std::is_constructible_v<T, decltype(v)> && ::std::is_convertible_v<decltype(v), T>
      : v_(FWD(v))
  {
  }

  /**
   * @brief Constructs the payload from a value which does not implicitly convert to `T`
   *
   * @param v Value to initialize the payload from
   */
  template <typename U>
  constexpr explicit just(U &&v) //
      noexcept(::std::is_nothrow_constructible_v<T, decltype(v)>)
    requires(not some_just<::std::remove_cvref_t<U>>) && (not detail::_some_in_place_type<::std::remove_cvref_t<U>>)
            && ::std::is_constructible_v<T, decltype(v)> && (not ::std::is_convertible_v<decltype(v), T>)
      : v_(FWD(v))
  {
  }

  /**
   * @brief Constructs the payload in place from the arguments
   *
   * @param args Arguments to construct the payload from
   */
  constexpr explicit just(::std::in_place_type_t<T>, auto &&...args) //
      noexcept(::std::is_nothrow_constructible_v<T, decltype(args)...>)
    requires ::std::is_constructible_v<T, decltype(args)...>
      : v_(FWD(args)...)
  {
  }

  /**
   * @brief Assignment from a value, through the payload's own assignment
   *
   * Assign-in-place is the whole meaning here - the payload is always a `T` - so this consults
   * `T`'s `operator=` alone, as copack's same-alternative arm does. Assigning the payload to
   * itself (`a = a.value()`) is a no-op by address identity.
   *
   * @param v Value to assign from
   * @return Reference to `*this`
   */
  template <typename U>
  constexpr just &operator=(U &&v) //
      noexcept(::std::is_nothrow_assignable_v<T &, decltype(v)>)
    requires(not some_just<::std::remove_cvref_t<U>>) && ::std::is_assignable_v<T &, decltype(v)>
  {
    if constexpr (::std::is_same_v<::std::remove_cvref_t<U>, T>) {
      if (::std::addressof(v) == ::std::addressof(v_))
        return *this;
    }
    v_ = FWD(v);
    return *this;
  }

  /**
   * @brief Destroys the payload and constructs a new one from the arguments, with the strong
   *        exception guarantee
   *
   * The mutation path for a payload that does not support assignment. Construction that can throw
   * goes through a temporary, relocated by nothrow move. Arguments that refer into the payload
   * held will dangle, as with std::optional's and std::variant's emplace.
   *
   * @param args Arguments to construct the new payload from
   * @return Reference to the new payload
   */
  constexpr T &emplace(auto &&...args) //
      noexcept(::std::is_nothrow_constructible_v<T, decltype(args)...>)
    requires ::std::is_constructible_v<T, decltype(args)...>
             && (::std::is_nothrow_constructible_v<T, decltype(args)...> || ::std::is_nothrow_move_constructible_v<T>)
  {
    if constexpr (::std::is_nothrow_constructible_v<T, decltype(args)...>) {
      ::std::destroy_at(::std::addressof(v_));
      ::std::construct_at(::std::addressof(v_), FWD(args)...);
    } else {
      T tmp(FWD(args)...);
      ::std::destroy_at(::std::addressof(v_));
      ::std::construct_at(::std::addressof(v_), ::std::move(tmp));
    }
    return v_;
  }

  /**
   * @brief Accesses the payload
   *
   * @return Reference to the payload, in `*this`'s value category
   */
  [[nodiscard]] constexpr T &value() & noexcept { return v_; }
  [[nodiscard]] constexpr T const &value() const & noexcept { return v_; }
  [[nodiscard]] constexpr T &&value() && noexcept { return ::std::move(v_); }
  [[nodiscard]] constexpr T const &&value() const && noexcept { return ::std::move(v_); }

  /**
   * @brief Maps the payload through the callable, wrapping the result
   *
   * @param fn Callable applied on the payload
   * @return `just` of the callable's result; `just<void>` for a void result
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) & //
      noexcept(detail::_is_nothrow_applicable<Fn, T &>::value) -> just<typename detail::_apply_result<Fn, T &>::type>
    requires detail::_is_applicable<Fn, T &>::value
  {
    using type = detail::_apply_result<Fn, T &>::type;
    if constexpr (::std::is_void_v<type>) {
      detail::_apply(FWD(fn), v_);
      return just<type>{};
    } else
      return just<type>{detail::_just_from_invoke, [&]() -> decltype(auto) { return detail::_apply(FWD(fn), v_); }};
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) const & //
      noexcept(detail::_is_nothrow_applicable<Fn, T const &>::value)
          -> just<typename detail::_apply_result<Fn, T const &>::type>
    requires detail::_is_applicable<Fn, T const &>::value
  {
    using type = detail::_apply_result<Fn, T const &>::type;
    if constexpr (::std::is_void_v<type>) {
      detail::_apply(FWD(fn), v_);
      return just<type>{};
    } else
      return just<type>{detail::_just_from_invoke, [&]() -> decltype(auto) { return detail::_apply(FWD(fn), v_); }};
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) && //
      noexcept(detail::_is_nothrow_applicable<Fn, T &&>::value) -> just<typename detail::_apply_result<Fn, T &&>::type>
    requires detail::_is_applicable<Fn, T &&>::value
  {
    using type = detail::_apply_result<Fn, T &&>::type;
    if constexpr (::std::is_void_v<type>) {
      detail::_apply(FWD(fn), ::std::move(v_));
      return just<type>{};
    } else
      return just<type>{detail::_just_from_invoke,
                        [&]() -> decltype(auto) { return detail::_apply(FWD(fn), ::std::move(v_)); }};
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) const && //
      noexcept(detail::_is_nothrow_applicable<Fn, T const &&>::value)
          -> just<typename detail::_apply_result<Fn, T const &&>::type>
    requires detail::_is_applicable<Fn, T const &&>::value
  {
    using type = detail::_apply_result<Fn, T const &&>::type;
    if constexpr (::std::is_void_v<type>) {
      detail::_apply(FWD(fn), ::std::move(v_));
      return just<type>{};
    } else
      return just<type>{detail::_just_from_invoke,
                        [&]() -> decltype(auto) { return detail::_apply(FWD(fn), ::std::move(v_)); }};
  }

  /**
   * @brief Binds the payload through the callable, which must return a `just`
   *
   * @param fn Callable applied on the payload
   * @return The callable's own `just`
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto and_then(Fn &&fn) & //
      noexcept(detail::_is_nothrow_applicable<Fn, T &>::value)
          -> ::std::remove_cvref_t<typename detail::_apply_result<Fn, T &>::type>
    requires detail::_is_applicable<Fn, T &>::value
  {
    // the member is the carrier's own bind; the cross-carrier bind lives in the and_then functor
    static_assert(some_just<::std::remove_cvref_t<typename detail::_apply_result<Fn, T &>::type>>);
    return detail::_apply(FWD(fn), v_);
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto and_then(Fn &&fn) const & //
      noexcept(detail::_is_nothrow_applicable<Fn, T const &>::value)
          -> ::std::remove_cvref_t<typename detail::_apply_result<Fn, T const &>::type>
    requires detail::_is_applicable<Fn, T const &>::value
  {
    static_assert(some_just<::std::remove_cvref_t<typename detail::_apply_result<Fn, T const &>::type>>);
    return detail::_apply(FWD(fn), v_);
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto and_then(Fn &&fn) && //
      noexcept(detail::_is_nothrow_applicable<Fn, T &&>::value)
          -> ::std::remove_cvref_t<typename detail::_apply_result<Fn, T &&>::type>
    requires detail::_is_applicable<Fn, T &&>::value
  {
    static_assert(some_just<::std::remove_cvref_t<typename detail::_apply_result<Fn, T &&>::type>>);
    return detail::_apply(FWD(fn), ::std::move(v_));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto and_then(Fn &&fn) const && //
      noexcept(detail::_is_nothrow_applicable<Fn, T const &&>::value)
          -> ::std::remove_cvref_t<typename detail::_apply_result<Fn, T const &&>::type>
    requires detail::_is_applicable<Fn, T const &&>::value
  {
    static_assert(some_just<::std::remove_cvref_t<typename detail::_apply_result<Fn, T const &&>::type>>);
    return detail::_apply(FWD(fn), ::std::move(v_));
  }

  /**
   * @brief Eliminates the payload through the callable
   *
   * The payload is handed over exactly as `fn::apply` would hand it - a `pack` or tuple-like
   * payload by elements, anything else whole - and trailing arguments follow the content.
   *
   * @param fn Callable applied on the payload
   * @param args Additional arguments, appended after the payload's content
   * @return The callable's result
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply(Fn &&fn, Args &&...args) & //
      noexcept(detail::_is_nothrow_applicable<Fn, T &, Args...>::value) -> decltype(auto)
    requires detail::_is_applicable<Fn, T &, Args...>::value
  {
    return detail::_apply(FWD(fn), v_, FWD(args)...);
  }

  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply(Fn &&fn, Args &&...args) const & //
      noexcept(detail::_is_nothrow_applicable<Fn, T const &, Args...>::value) -> decltype(auto)
    requires detail::_is_applicable<Fn, T const &, Args...>::value
  {
    return detail::_apply(FWD(fn), v_, FWD(args)...);
  }

  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply(Fn &&fn, Args &&...args) && //
      noexcept(detail::_is_nothrow_applicable<Fn, T &&, Args...>::value) -> decltype(auto)
    requires detail::_is_applicable<Fn, T &&, Args...>::value
  {
    return detail::_apply(FWD(fn), ::std::move(v_), FWD(args)...);
  }

  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply(Fn &&fn, Args &&...args) const && //
      noexcept(detail::_is_nothrow_applicable<Fn, T const &&, Args...>::value) -> decltype(auto)
    requires detail::_is_applicable<Fn, T const &&, Args...>::value
  {
    return detail::_apply(FWD(fn), ::std::move(v_), FWD(args)...);
  }

  /**
   * @brief Eliminates the payload through the callable, converting the result to `Ret`
   *
   * @tparam Ret Type the result converts to
   * @param fn Callable applied on the payload
   * @param args Additional arguments, appended after the payload's content
   * @return The callable's result, converted to `Ret`
   */
  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_r(Fn &&fn, Args &&...args) & //
      noexcept(detail::_is_nothrow_applicable_r<Ret, Fn, T &, Args...>::value) -> Ret
    requires detail::_is_applicable_r<Ret, Fn, T &, Args...>::value
  {
    return detail::_apply_r<Ret>(FWD(fn), v_, FWD(args)...);
  }

  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_r(Fn &&fn, Args &&...args) const & //
      noexcept(detail::_is_nothrow_applicable_r<Ret, Fn, T const &, Args...>::value) -> Ret
    requires detail::_is_applicable_r<Ret, Fn, T const &, Args...>::value
  {
    return detail::_apply_r<Ret>(FWD(fn), v_, FWD(args)...);
  }

  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_r(Fn &&fn, Args &&...args) && //
      noexcept(detail::_is_nothrow_applicable_r<Ret, Fn, T &&, Args...>::value) -> Ret
    requires detail::_is_applicable_r<Ret, Fn, T &&, Args...>::value
  {
    return detail::_apply_r<Ret>(FWD(fn), ::std::move(v_), FWD(args)...);
  }

  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_r(Fn &&fn, Args &&...args) const && //
      noexcept(detail::_is_nothrow_applicable_r<Ret, Fn, T const &&, Args...>::value) -> Ret
    requires detail::_is_applicable_r<Ret, Fn, T const &&, Args...>::value
  {
    return detail::_apply_r<Ret>(FWD(fn), ::std::move(v_), FWD(args)...);
  }

  /**
   * @brief Eliminates the payload through the callable, keyed by the payload's type
   *
   * The arm receives `std::in_place_type<T>` followed by the payload as `apply_type` hands it
   * over on `copack` and `choice` - a tuple-like payload's elements form is the row's one
   * signature - and trailing arguments follow the content.
   *
   * @param fn Callable applied on the tag and the payload
   * @param args Additional arguments, appended after the payload's content
   * @return The callable's result
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_type(Fn &&fn, Args &&...args) & //
      noexcept(noexcept(detail::_apply_tagged<::std::in_place_type_t<T>>(FWD(fn), v_, FWD(args)...))) -> decltype(auto)
    requires requires { detail::_apply_tagged<::std::in_place_type_t<T>>(FWD(fn), v_, FWD(args)...); }
  {
    return detail::_apply_tagged<::std::in_place_type_t<T>>(FWD(fn), v_, FWD(args)...);
  }

  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_type(Fn &&fn, Args &&...args) const & //
      noexcept(noexcept(detail::_apply_tagged<::std::in_place_type_t<T>>(FWD(fn), v_, FWD(args)...))) -> decltype(auto)
    requires requires { detail::_apply_tagged<::std::in_place_type_t<T>>(FWD(fn), v_, FWD(args)...); }
  {
    return detail::_apply_tagged<::std::in_place_type_t<T>>(FWD(fn), v_, FWD(args)...);
  }

  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_type(Fn &&fn, Args &&...args) && //
      noexcept(noexcept(detail::_apply_tagged<::std::in_place_type_t<T>>(FWD(fn), ::std::move(v_), FWD(args)...)))
          -> decltype(auto)
    requires requires { detail::_apply_tagged<::std::in_place_type_t<T>>(FWD(fn), ::std::move(v_), FWD(args)...); }
  {
    return detail::_apply_tagged<::std::in_place_type_t<T>>(FWD(fn), ::std::move(v_), FWD(args)...);
  }

  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_type(Fn &&fn, Args &&...args) const && //
      noexcept(noexcept(detail::_apply_tagged<::std::in_place_type_t<T>>(FWD(fn), ::std::move(v_), FWD(args)...)))
          -> decltype(auto)
    requires requires { detail::_apply_tagged<::std::in_place_type_t<T>>(FWD(fn), ::std::move(v_), FWD(args)...); }
  {
    return detail::_apply_tagged<::std::in_place_type_t<T>>(FWD(fn), ::std::move(v_), FWD(args)...);
  }

  /**
   * @brief Eliminates the payload through the callable, keyed by the payload's type, converting
   *        the result to `Ret`
   *
   * @tparam Ret Type the result converts to
   * @param fn Callable applied on the tag and the payload
   * @param args Additional arguments, appended after the payload's content
   * @return The callable's result, converted to `Ret`
   */
  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_type_r(Fn &&fn, Args &&...args) & //
      noexcept(noexcept(detail::_apply_tagged_r<Ret, ::std::in_place_type_t<T>>(FWD(fn), v_, FWD(args)...))) -> Ret
    requires requires { detail::_apply_tagged_r<Ret, ::std::in_place_type_t<T>>(FWD(fn), v_, FWD(args)...); }
  {
    return detail::_apply_tagged_r<Ret, ::std::in_place_type_t<T>>(FWD(fn), v_, FWD(args)...);
  }

  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_type_r(Fn &&fn, Args &&...args) const & //
      noexcept(noexcept(detail::_apply_tagged_r<Ret, ::std::in_place_type_t<T>>(FWD(fn), v_, FWD(args)...))) -> Ret
    requires requires { detail::_apply_tagged_r<Ret, ::std::in_place_type_t<T>>(FWD(fn), v_, FWD(args)...); }
  {
    return detail::_apply_tagged_r<Ret, ::std::in_place_type_t<T>>(FWD(fn), v_, FWD(args)...);
  }

  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_type_r(Fn &&fn, Args &&...args) && //
      noexcept(noexcept(detail::_apply_tagged_r<Ret, ::std::in_place_type_t<T>>(FWD(fn), ::std::move(v_),
                                                                                FWD(args)...))) -> Ret
    requires requires {
      detail::_apply_tagged_r<Ret, ::std::in_place_type_t<T>>(FWD(fn), ::std::move(v_), FWD(args)...);
    }
  {
    return detail::_apply_tagged_r<Ret, ::std::in_place_type_t<T>>(FWD(fn), ::std::move(v_), FWD(args)...);
  }

  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_type_r(Fn &&fn, Args &&...args) const && //
      noexcept(noexcept(detail::_apply_tagged_r<Ret, ::std::in_place_type_t<T>>(FWD(fn), ::std::move(v_),
                                                                                FWD(args)...))) -> Ret
    requires requires {
      detail::_apply_tagged_r<Ret, ::std::in_place_type_t<T>>(FWD(fn), ::std::move(v_), FWD(args)...);
    }
  {
    return detail::_apply_tagged_r<Ret, ::std::in_place_type_t<T>>(FWD(fn), ::std::move(v_), FWD(args)...);
  }

private:
  template <typename> friend struct just;

  template <typename Fn> constexpr explicit just(detail::_just_from_invoke_t, Fn &&make) : v_(FWD(make)()) {}
};

/**
 * @brief The identity carrier with nothing to carry: the family's unit as a value
 *
 * `just{}` (deduced) names it - a success with no payload, as `expected<void, copack<>>` in its
 * one state. Modeled on `expected<void, E>`: `value()` observes nothing, the callables of
 * `transform` and `and_then` are invoked without a value, and the `apply` family's one arm
 * receives the trailing arguments alone (`apply_type` prepends `std::in_place_type<void>`).
 */
template <> struct just<void> {
  using value_type = void;

  constexpr explicit just() = default;
  constexpr explicit just(::std::in_place_type_t<void>) noexcept {}

  [[nodiscard]] constexpr bool operator==(just const &) const noexcept = default;

  /**
   * @brief Observes the payload; there is nothing to observe
   */
  constexpr void value() const noexcept {}

  /**
   * @brief Maps through the callable, invoked with no arguments
   *
   * @param fn Callable to invoke
   * @return `just` of the callable's result; `just<void>` for a void result
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) const //
      noexcept(detail::_is_nothrow_applicable<Fn>::value) -> just<typename detail::_apply_result<Fn>::type>
    requires detail::_is_applicable<Fn>::value
  {
    using type = detail::_apply_result<Fn>::type;
    if constexpr (::std::is_void_v<type>) {
      detail::_apply(FWD(fn));
      return just<type>{};
    } else
      return just<type>{detail::_just_from_invoke, [&]() -> decltype(auto) { return detail::_apply(FWD(fn)); }};
  }

  /**
   * @brief Binds through the callable, invoked with no arguments; it must return a `just`
   *
   * @param fn Callable to invoke
   * @return The callable's own `just`
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto and_then(Fn &&fn) const //
      noexcept(detail::_is_nothrow_applicable<Fn>::value)
          -> ::std::remove_cvref_t<typename detail::_apply_result<Fn>::type>
    requires detail::_is_applicable<Fn>::value
  {
    // the member is the carrier's own bind; the cross-carrier bind lives in the and_then functor
    static_assert(some_just<::std::remove_cvref_t<typename detail::_apply_result<Fn>::type>>);
    return detail::_apply(FWD(fn));
  }

  /**
   * @brief Eliminates through the callable, invoked with the trailing arguments alone
   *
   * @param fn Callable to invoke
   * @param args Arguments the callable is invoked with
   * @return The callable's result
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply(Fn &&fn, Args &&...args) const //
      noexcept(detail::_is_nothrow_applicable<Fn, Args...>::value) -> decltype(auto)
    requires detail::_is_applicable<Fn, Args...>::value
  {
    return detail::_apply(FWD(fn), FWD(args)...);
  }

  /**
   * @brief Eliminates through the callable, converting the result to `Ret`
   *
   * @tparam Ret Type the result converts to
   * @param fn Callable to invoke
   * @param args Arguments the callable is invoked with
   * @return The callable's result, converted to `Ret`
   */
  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_r(Fn &&fn, Args &&...args) const //
      noexcept(detail::_is_nothrow_applicable_r<Ret, Fn, Args...>::value) -> Ret
    requires detail::_is_applicable_r<Ret, Fn, Args...>::value
  {
    return detail::_apply_r<Ret>(FWD(fn), FWD(args)...);
  }

  /**
   * @brief Eliminates through the callable, keyed by the payload's type
   *
   * @param fn Callable invoked with `std::in_place_type<void>` and the trailing arguments
   * @param args Arguments following the tag
   * @return The callable's result
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_type(Fn &&fn, Args &&...args) const //
      noexcept(detail::_is_nothrow_applicable<Fn, ::std::in_place_type_t<void>, Args &&...>::value) -> decltype(auto)
    requires detail::_is_applicable<Fn, ::std::in_place_type_t<void>, Args &&...>::value
  {
    return detail::_apply(FWD(fn), ::std::in_place_type_t<void>{}, FWD(args)...);
  }

  /**
   * @brief Eliminates through the callable, keyed by the payload's type, converting the result
   *        to `Ret`
   *
   * @tparam Ret Type the result converts to
   * @param fn Callable invoked with `std::in_place_type<void>` and the trailing arguments
   * @param args Arguments following the tag
   * @return The callable's result, converted to `Ret`
   */
  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_type_r(Fn &&fn, Args &&...args) const //
      noexcept(detail::_is_nothrow_applicable_r<Ret, Fn, ::std::in_place_type_t<void>, Args &&...>::value) -> Ret
    requires detail::_is_applicable_r<Ret, Fn, ::std::in_place_type_t<void>, Args &&...>::value
  {
    return detail::_apply_r<Ret>(FWD(fn), ::std::in_place_type_t<void>{}, FWD(args)...);
  }
};

/**
 * @brief Compares the payloads of two `just` values
 */
template <typename T, typename U>
[[nodiscard]] constexpr bool operator==(just<T> const &lh, just<U> const &rh) //
    noexcept(noexcept(lh.value() == rh.value()))
  requires requires {
    { lh.value() == rh.value() } -> ::std::convertible_to<bool>;
  }
{
  return lh.value() == rh.value();
}

/**
 * @brief Compares the payload with a value
 */
template <typename T, typename U>
[[nodiscard]] constexpr bool operator==(just<T> const &lh, U const &rh) //
    noexcept(noexcept(lh.value() == rh))
  requires(not some_just<U>) && requires {
    { lh.value() == rh } -> ::std::convertible_to<bool>;
  }
{
  return lh.value() == rh;
}

// CTAD for just, including the deduced spelling of the void carrier: just{}
template <typename T> just(T) -> just<T>;
template <typename T> explicit just(::std::in_place_type_t<T>, auto &&...) -> just<T>;
just() -> just<void>;

} // namespace fn

#endif // INCLUDE_FN_JUST
