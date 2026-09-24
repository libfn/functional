// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_JUST
#define INCLUDE_FN_JUST

#include <libfn_version.hpp>
#include <pfn/utility.hpp>

#include <fn/copack.hpp>
#include <fn/detail/functional.hpp>
#include <fn/detail/fwd.hpp>
#include <fn/detail/meta.hpp>
#include <fn/detail/variadic_union.hpp>
#include <fn/pack.hpp>

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include <fn/detail/macro_begin.hpp>

namespace fn {
inline namespace LIBFN_VERSION {

/**
 * @brief Checks if a type is a `just` (with any payload)
 *
 * @tparam T Type to check, possibly cv-ref qualified
 */
template <typename T>
concept some_just = detail::_some_just<T>;

/**
 * @brief Checks if a type is a `choice` - a `just` over a copack
 *
 * @tparam T Type to check, possibly cv-ref qualified
 */
template <typename T>
concept some_choice = detail::_some_choice<T>;

namespace detail {
// Supported referents for just<T&>. Copacks are excluded because dispatch would depend on
// the referent's active alternative, state the carrier does not own (issue #434).
template <typename T>
concept _just_referent = ::std::is_object_v<T> && (not ::std::is_array_v<T>)
                         && (not _some_in_place_type<::std::remove_cv_t<T>>) && (not _some_copack<T>);

// The payload just admits - the class mandates below assert the same set, and the transform verb
// asks this before naming just<result> anywhere, so an inadmissible result answers instead of
// firing a mandate inside the probe. The empty copack is out: just<copack<>> is incomplete.
template <typename T>
concept _just_payload
    = (::std::is_lvalue_reference_v<T> && _just_referent<::std::remove_reference_t<T>>)
      || ((not ::std::is_same_v<T, copack<>>) && (not ::std::is_reference_v<T>) && (not ::std::is_array_v<T>)
          && (not _some_in_place_type<T>) && ::std::is_same_v<T, ::std::remove_cv_t<T>>);

// What the callback's result may become: void and admissible payloads make a just; anything else
// keeps the member viable-but-loud below
template <typename Fn, typename... V>
concept _just_admissible_result
    = ::std::is_void_v<typename _apply_result<Fn, V...>::type> || _just_payload<typename _apply_result<Fn, V...>::type>;

// For an admissible result, transform returns just<result>. Otherwise expose the raw result
// type (including empty copacks, references and arrays) so probing does not instantiate an
// invalid just. Calling the member still triggers its static_assert. An inapplicable callback
// leaves no type.
template <typename Fn, typename... V> struct _just_transform_result {};
template <typename Fn, typename... V>
  requires(_is_applicable<Fn, V...>::value) && (not _just_admissible_result<Fn, V...>)
struct _just_transform_result<Fn, V...> {
  using type = typename _apply_result<Fn, V...>::type;
};
template <typename Fn, typename... V>
  requires(_is_applicable<Fn, V...>::value) && _just_admissible_result<Fn, V...>
struct _just_transform_result<Fn, V...> {
  using type = ::fn::just<typename _apply_result<Fn, V...>::type>;
};

// transform's result is direct-non-list-initialized from the thunk's invocation: no extra move,
// and immovable result types work (the pattern of pfn's _expected_from_invoke)
constexpr inline struct _just_from_invoke_t {
  explicit _just_from_invoke_t() = default;
} _just_from_invoke{};
} // namespace detail

// Declare the specializations before the primary can instantiate them through transform.
// just<copack<>> remains incomplete because there is no alternative to hold; just<void> is the unit.
template <typename T> struct just;
template <typename T> struct just<T &>;
template <> struct just<void>;
template <typename... Ts> struct just<copack<Ts...>>;
template <> struct just<copack<>>;

/**
 * @brief The identity carrier over a coproduct, `just<copack<Ts...>>`
 *
 * @tparam Ts The alternatives - flat, unique and sorted in the canonical order
 */
template <typename... Ts> using choice = just<copack<Ts...>>;

/**
 * @brief The identity carrier: holds exactly one value of `T`, always present
 *
 * The unit of the monadic family as a value: no error channel, no empty state, one payload. As
 * trivial as `T` permits in copying, moving, assignment and destruction; a structural type when
 * `T` is one. Dispatch granularity belongs to the engine, uniform per payload: a copack payload
 * selects the branch-wise specialization below, `choice`, while a `choice` payload is an atom
 * here as everywhere.
 *
 * @tparam T Payload type
 */
template <typename T> struct just {
  static_assert(not ::std::is_reference_v<T>);
  static_assert(not ::std::is_array_v<T>);
  static_assert(not detail::_some_in_place_type<T>);
  static_assert(::std::is_same_v<T, ::std::remove_cv_t<T>>);

  /**
   * @brief The type of the value side
   */
  using value_type = T;

  /**
   * @brief The payload
   */
  T v_{};

  /**
   * @brief Default constructor
   */
  constexpr just() = default;
  /**
   * @brief Copy constructor
   */
  constexpr just(just const &) = default;
  /**
   * @brief Move constructor
   */
  constexpr just(just &&) = default;
  /**
   * @brief Copy assignment
   */
  constexpr just &operator=(just const &) = default;
  /**
   * @brief Move assignment
   */
  constexpr just &operator=(just &&) = default;
  /**
   * @brief Destructor
   */
  constexpr ~just() = default;

  /**
   * @brief Constructs the payload from a value
   *
   * Explicit exactly where the conversion to `T` is. A `just` argument is accepted here only
   * when its type is `T`, so it becomes the payload rather than being unwrapped.
   *
   * @param v Value to initialize the payload from
   */
  // Keep these constraints in leading requires-clauses to avoid ambiguous deduction for
  // `choice{x}` on Clang 22 when it derives deduction guides for the alias.
  template <typename U>
    requires((not some_just<::std::remove_cvref_t<U>>) || ::std::is_same_v<::std::remove_cvref_t<U>, T>)
            && (not detail::_some_in_place_type<::std::remove_cvref_t<U>>)
            && ::std::is_constructible_v<T, U &&> && ::std::is_convertible_v<U &&, T>
  constexpr just(U &&v) // NOSONAR cpp:S1709 implicit arm of the explicit pair
      noexcept(::std::is_nothrow_constructible_v<T, U &&>)
      : v_(FWD(v))
  {
  }

  template <typename U>
    requires((not some_just<::std::remove_cvref_t<U>>) || ::std::is_same_v<::std::remove_cvref_t<U>, T>)
            && (not detail::_some_in_place_type<::std::remove_cvref_t<U>>)
            && ::std::is_constructible_v<T, U &&> && (not ::std::is_convertible_v<U &&, T>)
  constexpr explicit just(U &&v) // NOSONAR cpp:S6458 just sources must match the payload type
      noexcept(::std::is_nothrow_constructible_v<T, U &&>)
      : v_(FWD(v))
  {
  }

  /**
   * @brief Constructs the payload in place from the arguments
   *
   * @param args Arguments to construct the payload from
   */
  template <typename... Args>
    requires ::std::is_constructible_v<T, Args &&...>
  constexpr explicit just(::std::in_place_type_t<T>, Args &&...args) //
      noexcept(::std::is_nothrow_constructible_v<T, Args &&...>)
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
    requires((not some_just<::std::remove_cvref_t<U>>) || ::std::is_same_v<::std::remove_cvref_t<U>, T>)
            && ::std::is_assignable_v<T &, decltype(v)>
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
      noexcept(detail::_is_nothrow_applicable<Fn, T &>::value) -> typename detail::_just_transform_result<Fn, T &>::type
    requires detail::_is_applicable<Fn, T &>::value
  {
    using type = detail::_apply_result<Fn, T &>::type;
    static_assert(detail::_just_admissible_result<Fn, T &>);
    if constexpr (::std::is_void_v<type>) {
      detail::_apply(FWD(fn), v_);
      return just<type>{};
    } else if constexpr (detail::_just_payload<type>)
      return just<type>{detail::_just_from_invoke,
                        [&fn, this]() -> decltype(auto) { return detail::_apply(FWD(fn), v_); }};
    else
      ::pfn::unreachable(); // LCOV_EXCL_LINE - rejected by the static_assert above
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) const & //
      noexcept(detail::_is_nothrow_applicable<Fn, T const &>::value) ->
      typename detail::_just_transform_result<Fn, T const &>::type
    requires detail::_is_applicable<Fn, T const &>::value
  {
    using type = detail::_apply_result<Fn, T const &>::type;
    static_assert(detail::_just_admissible_result<Fn, T const &>);
    if constexpr (::std::is_void_v<type>) {
      detail::_apply(FWD(fn), v_);
      return just<type>{};
    } else if constexpr (detail::_just_payload<type>)
      return just<type>{detail::_just_from_invoke,
                        [&fn, this]() -> decltype(auto) { return detail::_apply(FWD(fn), v_); }};
    else
      ::pfn::unreachable(); // LCOV_EXCL_LINE - rejected by the static_assert above
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) && //
      noexcept(detail::_is_nothrow_applicable<Fn, T &&>::value) ->
      typename detail::_just_transform_result<Fn, T &&>::type
    requires detail::_is_applicable<Fn, T &&>::value
  {
    using type = detail::_apply_result<Fn, T &&>::type;
    static_assert(detail::_just_admissible_result<Fn, T &&>);
    if constexpr (::std::is_void_v<type>) {
      detail::_apply(FWD(fn), ::std::move(v_));
      return just<type>{};
    } else if constexpr (detail::_just_payload<type>)
      return just<type>{detail::_just_from_invoke,
                        [&fn, this]() -> decltype(auto) { return detail::_apply(FWD(fn), ::std::move(v_)); }};
    else
      ::pfn::unreachable(); // LCOV_EXCL_LINE - rejected by the static_assert above
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) const && //
      noexcept(detail::_is_nothrow_applicable<Fn, T const &&>::value) ->
      typename detail::_just_transform_result<Fn, T const &&>::type
    requires detail::_is_applicable<Fn, T const &&>::value
  {
    using type = detail::_apply_result<Fn, T const &&>::type;
    static_assert(detail::_just_admissible_result<Fn, T const &&>);
    if constexpr (::std::is_void_v<type>) {
      detail::_apply(FWD(fn), ::std::move(v_));
      return just<type>{};
    } else if constexpr (detail::_just_payload<type>)
      return just<type>{detail::_just_from_invoke,
                        [&fn, this]() -> decltype(auto) { return detail::_apply(FWD(fn), ::std::move(v_)); }};
    else
      ::pfn::unreachable(); // LCOV_EXCL_LINE - rejected by the static_assert above
  }

  /**
   * @brief Binds the payload through the callable, which returns a `just` of any payload
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
    static_assert(some_just<typename detail::_apply_result<Fn, T &>::type>);
    return detail::_apply(FWD(fn), v_);
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto and_then(Fn &&fn) const & //
      noexcept(detail::_is_nothrow_applicable<Fn, T const &>::value)
          -> ::std::remove_cvref_t<typename detail::_apply_result<Fn, T const &>::type>
    requires detail::_is_applicable<Fn, T const &>::value
  {
    static_assert(some_just<typename detail::_apply_result<Fn, T const &>::type>);
    return detail::_apply(FWD(fn), v_);
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto and_then(Fn &&fn) && //
      noexcept(detail::_is_nothrow_applicable<Fn, T &&>::value)
          -> ::std::remove_cvref_t<typename detail::_apply_result<Fn, T &&>::type>
    requires detail::_is_applicable<Fn, T &&>::value
  {
    static_assert(some_just<typename detail::_apply_result<Fn, T &&>::type>);
    return detail::_apply(FWD(fn), ::std::move(v_));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto and_then(Fn &&fn) const && //
      noexcept(detail::_is_nothrow_applicable<Fn, T const &&>::value)
          -> ::std::remove_cvref_t<typename detail::_apply_result<Fn, T const &&>::type>
    requires detail::_is_applicable<Fn, T const &&>::value
  {
    static_assert(some_just<typename detail::_apply_result<Fn, T const &&>::type>);
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
  /**
   * @brief The type of the value side
   */
  using value_type = void;

  /**
   * @brief Default constructor
   */
  constexpr just() = default;
  /**
   * @brief Constructs the alternative named by the tag, in place from the arguments
   */
  constexpr explicit just(::std::in_place_type_t<void>) noexcept {}
  /**
   * @brief Constructs the value in place from the arguments
   */
  constexpr explicit just(::std::in_place_t) noexcept {}

  /**
   * @brief Equality; every `just` compares equal to every other
   */
  [[nodiscard]] constexpr bool operator==(just const &) const noexcept = default;

  /**
   * @brief Observes the payload; there is nothing to observe
   */
  constexpr void value() const noexcept {} // NOSONAR cpp:S1186 void payload

  /**
   * @brief Maps through the callable, invoked with no arguments
   *
   * @param fn Callable to invoke
   * @return `just` of the callable's result; `just<void>` for a void result
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) const //
      noexcept(detail::_is_nothrow_applicable<Fn>::value) -> typename detail::_just_transform_result<Fn>::type
    requires detail::_is_applicable<Fn>::value
  {
    using type = detail::_apply_result<Fn>::type;
    static_assert(detail::_just_admissible_result<Fn>);
    if constexpr (::std::is_void_v<type>) {
      detail::_apply(FWD(fn));
      return just<type>{};
    } else if constexpr (detail::_just_payload<type>)
      return just<type>{detail::_just_from_invoke, [&fn]() -> decltype(auto) { return detail::_apply(FWD(fn)); }};
    else
      ::pfn::unreachable(); // LCOV_EXCL_LINE - rejected by the static_assert above
  }

  /**
   * @brief Binds through the callable, invoked with no arguments; it returns a `just` of any payload
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
    static_assert(some_just<typename detail::_apply_result<Fn>::type>);
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
 * @brief The identity carrier over an lvalue reference: always bound to one `T`
 *
 * The always-engaged counterpart of `optional<T&>`. Copy assignment and `emplace` rebind the
 * reference without assigning to the referent. `value()` returns `T&` regardless of the carrier's
 * value category or constness. Callable operations use the same reference, expanding packs and
 * tuple-like referents as `fn::apply` does.
 *
 * There is no default constructor. Deduction from a value produces an owning carrier; use
 * `just<T&>` or `std::in_place_type<T&>` to request a reference. Referents must be object types
 * other than arrays, in-place type tags, or copacks. The carrier is trivially copyable and a
 * structural type.
 *
 * The caller must keep the referent alive. As with the library's `optional<T&>`, binding to a
 * temporary is not rejected and can leave a dangling reference. Detecting such bindings requires
 * the C++23 trait `std::reference_constructs_from_temporary`.
 *
 * @tparam T Referent type
 */
template <typename T> struct just<T &> {
  static_assert(detail::_just_referent<T>);

  /**
   * @brief The referent type, including any cv-qualification
   */
  using value_type = T;

  /**
   * @brief The referent's address
   */
  T *p_;

  /**
   * @brief Copy constructor; binds the same referent
   */
  constexpr just(just const &) = default;
  /**
   * @brief Copy assignment; rebinds to the other's referent
   */
  constexpr just &operator=(just const &) = default;
  /**
   * @brief Destructor
   */
  constexpr ~just() = default;

  /**
   * @brief Binds the reference to the argument
   *
   * Explicit unless the argument is implicitly convertible to `T&`.
   *
   * @param u The referent, or a value whose conversion yields it
   */
  template <typename U>
    requires(not ::std::is_same_v<::std::remove_cvref_t<U>, just>)
            && (not detail::_some_in_place_type<::std::remove_cvref_t<U>>) && ::std::is_constructible_v<T &, U>
  constexpr explicit(not ::std::is_convertible_v<U, T &>) just(U &&u) // NOSONAR cpp:S6458 the constraint excludes self
      noexcept(::std::is_nothrow_constructible_v<T &, U>)
      : p_(_address(FWD(u)))
  {
  }

  /**
   * @brief Binds the reference to the argument
   *
   * @param u The referent, or a value whose conversion yields it
   */
  template <typename U>
    requires ::std::is_constructible_v<T &, U>
  constexpr explicit just(::std::in_place_type_t<T &>, U &&u) //
      noexcept(::std::is_nothrow_constructible_v<T &, U>)
      : p_(_address(FWD(u)))
  {
  }

  /**
   * @brief Rebinds the reference to the argument
   *
   * @param u The new referent, or a value whose conversion yields it
   * @return Reference to the new referent
   */
  template <typename U>
  constexpr T &emplace(U &&u) noexcept(::std::is_nothrow_constructible_v<T &, U>)
    requires ::std::is_constructible_v<T &, U>
  {
    p_ = _address(FWD(u));
    return *p_;
  }

  /**
   * @brief Accesses the referent
   *
   * @return The referent, whatever the value category of `*this`
   */
  [[nodiscard]] constexpr T &value() const noexcept { return *p_; }

  /**
   * @brief Maps the referent through the callable, wrapping the result
   *
   * A supported lvalue-reference result produces a non-owning `just<U&>`; a value result
   * produces an owning carrier.
   *
   * @param fn Callable applied on the referent
   * @return `just` of the callable's result; `just<void>` for a void result
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) const //
      noexcept(detail::_is_nothrow_applicable<Fn, T &>::value) -> typename detail::_just_transform_result<Fn, T &>::type
    requires detail::_is_applicable<Fn, T &>::value
  {
    using type = detail::_apply_result<Fn, T &>::type;
    static_assert(detail::_just_admissible_result<Fn, T &>);
    if constexpr (::std::is_void_v<type>) {
      detail::_apply(FWD(fn), *p_);
      return just<type>{};
    } else if constexpr (detail::_just_payload<type>)
      return just<type>{detail::_just_from_invoke,
                        [&fn, this]() -> decltype(auto) { return detail::_apply(FWD(fn), *p_); }};
    else
      ::pfn::unreachable(); // LCOV_EXCL_LINE - rejected by the static_assert above
  }

  /**
   * @brief Binds the referent through the callable, which returns a `just` of any payload
   *
   * @param fn Callable applied on the referent
   * @return The callable's `just` result, returned by value
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto and_then(Fn &&fn) const //
      noexcept(detail::_is_nothrow_applicable<Fn, T &>::value)
          -> ::std::remove_cvref_t<typename detail::_apply_result<Fn, T &>::type>
    requires detail::_is_applicable<Fn, T &>::value
  {
    // the member is the carrier's own bind; the cross-carrier bind lives in the and_then functor
    static_assert(some_just<typename detail::_apply_result<Fn, T &>::type>);
    return detail::_apply(FWD(fn), *p_);
  }

  /**
   * @brief Eliminates the referent through the callable
   *
   * As with `fn::apply`, packs and tuple-like referents are expanded into their elements; other
   * referents are passed whole. Additional arguments follow the referent or its elements.
   *
   * @param fn Callable applied on the referent
   * @param args Additional arguments, appended after the referent's content
   * @return The callable's result
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply(Fn &&fn, Args &&...args) const //
      noexcept(detail::_is_nothrow_applicable<Fn, T &, Args...>::value) -> decltype(auto)
    requires detail::_is_applicable<Fn, T &, Args...>::value
  {
    return detail::_apply(FWD(fn), *p_, FWD(args)...);
  }

  /**
   * @brief Eliminates the referent through the callable, converting the result to `Ret`
   *
   * @tparam Ret Type the result converts to
   * @param fn Callable applied on the referent
   * @param args Additional arguments, appended after the referent's content
   * @return The callable's result, converted to `Ret`
   */
  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_r(Fn &&fn, Args &&...args) const //
      noexcept(detail::_is_nothrow_applicable_r<Ret, Fn, T &, Args...>::value) -> Ret
    requires detail::_is_applicable_r<Ret, Fn, T &, Args...>::value
  {
    return detail::_apply_r<Ret>(FWD(fn), *p_, FWD(args)...);
  }

  /**
   * @brief Eliminates the referent through the callable, keyed by the payload's type
   *
   * The callable receives `std::in_place_type<T&>`, then the referent or its elements as in
   * `apply`, then any additional arguments.
   *
   * @param fn Callable applied on the tag and the referent
   * @param args Additional arguments, appended after the referent's content
   * @return The callable's result
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_type(Fn &&fn, Args &&...args) const //
      noexcept(noexcept(detail::_apply_tagged<::std::in_place_type_t<T &>>(FWD(fn), *p_, FWD(args)...)))
          -> decltype(auto)
    requires requires { detail::_apply_tagged<::std::in_place_type_t<T &>>(FWD(fn), *p_, FWD(args)...); }
  {
    return detail::_apply_tagged<::std::in_place_type_t<T &>>(FWD(fn), *p_, FWD(args)...);
  }

  /**
   * @brief Eliminates the referent through the callable, keyed by the payload's type, converting
   *        the result to `Ret`
   *
   * @tparam Ret Type the result converts to
   * @param fn Callable applied on the tag and the referent
   * @param args Additional arguments, appended after the referent's content
   * @return The callable's result, converted to `Ret`
   */
  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_type_r(Fn &&fn, Args &&...args) const //
      noexcept(noexcept(detail::_apply_tagged_r<Ret, ::std::in_place_type_t<T &>>(FWD(fn), *p_, FWD(args)...))) -> Ret
    requires requires { detail::_apply_tagged_r<Ret, ::std::in_place_type_t<T &>>(FWD(fn), *p_, FWD(args)...); }
  {
    return detail::_apply_tagged_r<Ret, ::std::in_place_type_t<T &>>(FWD(fn), *p_, FWD(args)...);
  }

private:
  template <typename> friend struct just;

  // Same semantics as `T &r(FWD(u));`, spelled as a cast for MSVC (error C2440)
  template <typename U> static constexpr T *_address(U &&u) noexcept(::std::is_nothrow_constructible_v<T &, U>)
  {
    return ::std::addressof(static_cast<T &>(FWD(u)));
  }

  template <typename Fn> constexpr explicit just(detail::_just_from_invoke_t, Fn &&make) : p_(_address(FWD(make)())) {}
};

namespace detail {
// Inject each branch's payload into the joined choice: a choice contributes its copack,
// an ordinary just contributes its value, and just<void> contributes pack<>. Constructing from
// the whole carrier could instead select that carrier as an alternative and keep an extra layer.
// Return the callback result directly when it already has the joined type or the destination
// is not a choice. The latter also preserves invalid convergent results for the member's
// static_assert, without assuming they have a payload while probing noexcept.
template <typename To, typename Fn, typename... Args> struct _nothrow_just_inject : ::std::false_type {};
template <typename To, typename Fn, typename... Args>
  requires ::std::is_invocable_v<Fn, Args...>
struct _nothrow_just_inject<To, Fn, Args...> {
  static constexpr bool value = [] {
    using from = ::std::remove_cvref_t<::std::invoke_result_t<Fn, Args...>>;
    if constexpr (::std::is_same_v<from, To> || not _some_choice<To>)
      return ::std::is_nothrow_invocable_r_v<To, Fn, Args...>;
    else if constexpr (::std::is_void_v<typename from::value_type>)
      return ::std::is_nothrow_invocable_v<Fn, Args...>
             && ::std::is_nothrow_constructible_v<To, ::std::in_place_type_t<pack<>>>;
    else
      return ::std::is_nothrow_invocable_v<Fn, Args...>
             && ::std::is_nothrow_constructible_v<To, ::std::in_place_type_t<typename from::value_type>,
                                                  decltype((::std::declval<::std::invoke_result_t<Fn, Args...>>().v_))>;
  }();
};

template <typename To, typename Fn> struct _just_injector final {
  Fn fn;

  template <typename... Args>
  constexpr auto operator()(Args &&...args) const noexcept(_nothrow_just_inject<To, Fn, Args...>::value) -> To
    requires ::std::is_invocable_v<Fn, Args...>
  {
    using from = ::std::remove_cvref_t<::std::invoke_result_t<Fn, Args...>>;
    if constexpr (::std::is_same_v<from, To> || not _some_choice<To>)
      return ::std::invoke(FWD(fn), FWD(args)...);
    else if constexpr (::std::is_void_v<typename from::value_type>) {
      static_cast<void>(::std::invoke(FWD(fn), FWD(args)...));
      return To{::std::in_place_type<pack<>>};
    } else
      return To{::std::in_place_type<typename from::value_type>, ::std::invoke(FWD(fn), FWD(args)...).v_};
  }
};

// The join-mode engine entry for the bind over a copack: the tag announces the joined just, and
// every branch's result enters it through the injection above
template <typename Tag, typename Cp, typename Fn>
  requires _some_copack<::std::remove_cvref_t<Cp>>
[[nodiscard]] constexpr auto _join_just_apply(Cp &&cp, Fn &&fn) //
    noexcept(_is_nothrow_rts_applicable<typename _copack_apply_result<Tag, Fn &&, Cp &&>::type,
                                        _just_injector<typename _copack_apply_result<Tag, Fn &&, Cp &&>::type, Fn &&>,
                                        Cp &&>) -> typename _copack_apply_result<Tag, Fn &&, Cp &&>::type
{
  using type = _copack_apply_result<Tag, Fn &&, Cp &&>::type;
  using data_t = ::std::remove_cvref_t<Cp>::data_t;
  return apply_variadic_union<type, data_t>(FWD(cp).data, cp.index, _just_injector<type, Fn &&>{FWD(fn)});
}
} // namespace detail

/**
 * @brief The identity carrier over a coproduct: always holds one of the alternatives
 *
 * `choice<Ts...>` names it: a never-failing computation whose result is one of `Ts...`. The
 * payload is the `copack` of the alternatives, and the copack's alternative-wise surface -
 * construction, assignment, `emplace`, the queries and the `apply` family - is forwarded to it.
 * Where a bare `copack` is self-flattening data, a choice is an atom: mapping keeps a returned
 * choice whole, and only `and_then` joins the branches' justs into one. The alternatives obey the
 * same canonical form as `copack`'s - flat, unique, sorted -
 * so spell `choice_for`. A structural type when the alternatives are.
 *
 * @tparam Ts The alternatives - flat, unique and sorted in the canonical order
 */
template <typename... Ts> struct just<copack<Ts...>> {
  /**
   * @brief The copack of alternatives this choice carries
   */
  using value_type = copack<Ts...>;

  /**
   * @brief The payload
   */
  value_type v_;

  /**
   * @brief The number of alternatives
   */
  static constexpr ::std::size_t size = sizeof...(Ts);
  /**
   * @brief The I-th alternative in the canonical order
   */
  template <::std::size_t I> using select_nth = detail::select_nth_t<I, Ts...>;
  /**
   * @brief Whether `T` is one of the alternatives
   */
  template <typename T> static constexpr bool has_type = value_type::template has_type<T>;

  /**
   * @brief Constructs the alternative matching the value's type after removing cv/ref qualifiers
   *
   * Explicit exactly where the conversion to that alternative is.
   *
   * @param v Value of one alternative
   */
  template <typename T>
  constexpr just(T &&v) // NOSONAR cpp:S1709,S6458 implicit arm of the explicit pair; has_type excludes self
      noexcept(::std::is_nothrow_constructible_v<value_type, ::std::in_place_type_t<::std::remove_cvref_t<T>>, T &&>)
    requires has_type<::std::remove_cvref_t<T>>
             && ::std::is_constructible_v<value_type, ::std::in_place_type_t<::std::remove_cvref_t<T>>, T &&>
             && ::std::is_convertible_v<T &&, ::std::remove_cvref_t<T>>
      : v_(::std::in_place_type<::std::remove_cvref_t<T>>, FWD(v))
  {
  }

  template <typename T>
  constexpr explicit just(T &&v) // NOSONAR cpp:S6458 has_type excludes self
      noexcept(::std::is_nothrow_constructible_v<value_type, ::std::in_place_type_t<::std::remove_cvref_t<T>>, T &&>)
    requires has_type<::std::remove_cvref_t<T>>
             && ::std::is_constructible_v<value_type, ::std::in_place_type_t<::std::remove_cvref_t<T>>, T &&>
             && (not ::std::is_convertible_v<T &&, ::std::remove_cvref_t<T>>)
      : v_(::std::in_place_type<::std::remove_cvref_t<T>>, FWD(v))
  {
  }

  /**
   * @brief Constructs the alternative `T` in place from the arguments
   *
   * @tparam T The alternative to construct
   * @param d Tag naming the alternative
   * @param args Arguments to construct the alternative from
   */
  template <typename T>
  constexpr explicit just(::std::in_place_type_t<T> d, auto &&...args) //
      noexcept(::std::is_nothrow_constructible_v<value_type, ::std::in_place_type_t<T>, decltype(args)...>)
    requires has_type<T> && ::std::is_constructible_v<value_type, ::std::in_place_type_t<T>, decltype(args)...>
      : v_(d, FWD(args)...)
  {
  }

  /**
   * @brief Widening constructor from a copack over a subset of the alternatives
   *
   * The destination must contain every source alternative, and its payload must be constructible
   * from the source copack.
   *
   * @param v The copack to lift
   */
  template <typename... Tx>
  constexpr just(copack<Tx...> const &v) // NOSONAR cpp:S1709 implicit widening by design
      noexcept(::std::is_nothrow_constructible_v<value_type, copack<Tx...> const &>)
    requires detail::is_superset_of<value_type, copack<Tx...>>
             && ::std::is_constructible_v<value_type, copack<Tx...> const &>
      : v_(v)
  {
  }

  /**
   * @brief Widening constructor from a copack over a subset of the alternatives
   */
  template <typename... Tx>
  constexpr just(copack<Tx...> &&v) // NOSONAR cpp:S1709 implicit widening by design
      noexcept(::std::is_nothrow_constructible_v<value_type, copack<Tx...>>)
    requires detail::is_superset_of<value_type, copack<Tx...>> && ::std::is_constructible_v<value_type, copack<Tx...>>
      : v_(::std::move(v))
  {
  }

  /**
   * @brief Widening constructor from a copack whose type is spelled as a tag
   *
   * @param d Tag naming the copack's type
   * @param v The copack to lift
   */
  template <typename... Tx, some_copack V>
  constexpr just(::std::in_place_type_t<copack<Tx...>> d, V &&v) //
      noexcept(::std::is_nothrow_constructible_v<value_type, ::std::in_place_type_t<copack<Tx...>>, V &&>)
    requires ::std::is_constructible_v<value_type, ::std::in_place_type_t<copack<Tx...>>, V &&>
      : v_(d, FWD(v))
  {
  }

  // An alternative match wins over widening: where a narrower choice is itself one of the
  // alternatives, the value constructor above boxes it whole.
  /**
   * @brief Widening constructor from a choice over a subset of the alternatives
   *
   * @param other The narrower choice
   */
  template <typename... Tx>
  constexpr just(just<copack<Tx...>> const &other) // NOSONAR cpp:S1709 implicit widening by design
      noexcept(::std::is_nothrow_constructible_v<value_type, copack<Tx...> const &>)
    requires(not ::std::is_same_v<value_type, copack<Tx...>>) && (not has_type<just<copack<Tx...>>>)
            && detail::is_superset_of<value_type, copack<Tx...>>
            && ::std::is_constructible_v<value_type, copack<Tx...> const &>
      : v_(other.v_)
  {
  }

  /**
   * @brief Widening constructor from a choice over a subset of the alternatives
   */
  template <typename... Tx>
  constexpr just(just<copack<Tx...>> &&other) // NOSONAR cpp:S1709 implicit widening by design
      noexcept(::std::is_nothrow_constructible_v<value_type, copack<Tx...>>)
    requires(not ::std::is_same_v<value_type, copack<Tx...>>) && (not has_type<just<copack<Tx...>>>)
            && detail::is_superset_of<value_type, copack<Tx...>> && ::std::is_constructible_v<value_type, copack<Tx...>>
      : v_(::std::move(other.v_))
  {
  }

  /**
   * @brief Copy constructor; trivial where every alternative's is
   */
  constexpr just(just const &) = default;
  /**
   * @brief Move constructor; trivial where every alternative's is
   */
  constexpr just(just &&) = default;
  /**
   * @brief Copy assignment, with the strong exception guarantee
   */
  constexpr just &operator=(just const &) = default;
  /**
   * @brief Move assignment, with the strong exception guarantee
   */
  constexpr just &operator=(just &&) = default;
  /**
   * @brief Destructor
   */
  constexpr ~just() = default;

  /**
   * @brief Widening assignment from a copack over a subset of the alternatives
   *
   * @param arg The narrower copack
   * @return Reference to `*this`
   */
  template <typename... Tx>
  constexpr just &operator=(copack<Tx...> const &arg) //
      noexcept(::std::is_nothrow_assignable_v<value_type &, copack<Tx...> const &>)
    requires ::std::is_assignable_v<value_type &, copack<Tx...> const &>
  {
    v_ = arg;
    return *this;
  }

  /**
   * @brief Widening assignment from a copack over a subset of the alternatives
   */
  template <typename... Tx>
  constexpr just &operator=(copack<Tx...> &&arg) //
      noexcept(::std::is_nothrow_assignable_v<value_type &, copack<Tx...>>)
    requires ::std::is_assignable_v<value_type &, copack<Tx...>>
  {
    v_ = ::std::move(arg);
    return *this;
  }

  /**
   * @brief Widening assignment from a choice over a subset of the alternatives
   *
   * @param arg The narrower choice
   * @return Reference to `*this`
   */
  template <typename... Tx>
  constexpr just &operator=(just<copack<Tx...>> const &arg) //
      noexcept(::std::is_nothrow_assignable_v<value_type &, copack<Tx...> const &>)
    requires(not ::std::is_same_v<value_type, copack<Tx...>>) && (not has_type<just<copack<Tx...>>>)
            && ::std::is_assignable_v<value_type &, copack<Tx...> const &>
  {
    v_ = arg.v_;
    return *this;
  }

  /**
   * @brief Widening assignment from a choice over a subset of the alternatives
   */
  template <typename... Tx>
  constexpr just &operator=(just<copack<Tx...>> &&arg) //
      noexcept(::std::is_nothrow_assignable_v<value_type &, copack<Tx...>>)
    requires(not ::std::is_same_v<value_type, copack<Tx...>>) && (not has_type<just<copack<Tx...>>>)
            && ::std::is_assignable_v<value_type &, copack<Tx...>>
  {
    v_ = ::std::move(arg.v_);
    return *this;
  }

  // Copack- and choice-typed sources are left to the assignments above: for a non-const lvalue
  // source a forwarding reference would otherwise outrank their `const &` bindings.
  /**
   * @brief Assignment from a value of one alternative, with the strong exception guarantee
   *
   * @param v Value of one alternative
   * @return Reference to `*this`
   */
  template <typename U>
  constexpr just &operator=(U &&v) //
      noexcept(::std::is_nothrow_assignable_v<value_type &, U &&>)
    requires(not some_copack<U>) && (not some_choice<U>) && ::std::is_assignable_v<value_type &, U &&>
  {
    v_ = FWD(v);
    return *this;
  }

  /**
   * @brief Replaces the active alternative with a `T`, preserving the original value if construction throws
   *
   * @tparam T The alternative to construct
   * @param args Arguments to construct the alternative from
   * @return Reference to the new alternative
   */
  template <typename T>
  constexpr T &emplace(auto &&...args) //
      noexcept(noexcept(v_.template emplace<T>(FWD(args)...)))
    requires requires { v_.template emplace<T>(FWD(args)...); }
  {
    return v_.template emplace<T>(FWD(args)...);
  }

  /**
   * @brief Checks if `T` is the active alternative
   *
   * @tparam T The alternative to ask about
   * @return Whether `T` is the alternative held
   */
  template <typename T>
    requires has_type<T>
  [[nodiscard]] constexpr bool has_value(::std::in_place_type_t<T> d = ::std::in_place_type<T>) const noexcept
  {
    return v_.has_value(d);
  }

  /**
   * @brief Pointer to the alternative `T`, or `nullptr` where it is not the one held
   *
   * @tparam T The alternative to access
   * @return Pointer to the alternative, or `nullptr`
   */
  template <typename T>
    requires has_type<T>
  [[nodiscard]] constexpr T *get_ptr(::std::in_place_type_t<T> d = ::std::in_place_type<T>) noexcept
  {
    return v_.get_ptr(d);
  }

  template <typename T>
    requires has_type<T>
  [[nodiscard]] constexpr T const *get_ptr(::std::in_place_type_t<T> d = ::std::in_place_type<T>) const noexcept
  {
    return v_.get_ptr(d);
  }

  /**
   * @brief Accesses the alternatives as the `copack` payload
   *
   * Always present - a choice cannot fail - so the access is total, never throwing.
   *
   * @return Reference to the payload, in `*this`'s value category
   */
  [[nodiscard]] constexpr value_type &value() & noexcept { return v_; }
  [[nodiscard]] constexpr value_type const &value() const & noexcept { return v_; }
  [[nodiscard]] constexpr value_type &&value() && noexcept { return ::std::move(v_); }
  [[nodiscard]] constexpr value_type const &&value() const && noexcept { return ::std::move(v_); }

  /**
   * @brief Eliminates the choice: the active alternative routes into the callable
   *
   * Exactly `copack`'s `apply`, over the alternatives: exhaustive dispatch by ordinary overload
   * resolution, one deduced result type, a tuple-like alternative unpacked one level into its
   * elements, trailing arguments after the content.
   *
   * @param fn Callable applied on the active alternative; `fn::overload` fuses arms into one
   * @param args Additional arguments, appended after the alternative's content
   * @return The callable's result
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply(Fn &&fn, Args &&...args) & //
      noexcept(noexcept(v_.apply(FWD(fn), FWD(args)...))) -> decltype(auto)
    requires requires { v_.apply(FWD(fn), FWD(args)...); }
  {
    return v_.apply(FWD(fn), FWD(args)...);
  }

  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply(Fn &&fn, Args &&...args) const & //
      noexcept(noexcept(v_.apply(FWD(fn), FWD(args)...))) -> decltype(auto)
    requires requires { v_.apply(FWD(fn), FWD(args)...); }
  {
    return v_.apply(FWD(fn), FWD(args)...);
  }

  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply(Fn &&fn, Args &&...args) && //
      noexcept(noexcept(::std::move(v_).apply(FWD(fn), FWD(args)...))) -> decltype(auto)
    requires requires { ::std::move(v_).apply(FWD(fn), FWD(args)...); }
  {
    return ::std::move(v_).apply(FWD(fn), FWD(args)...);
  }

  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply(Fn &&fn, Args &&...args) const && //
      noexcept(noexcept(::std::move(v_).apply(FWD(fn), FWD(args)...))) -> decltype(auto)
    requires requires { ::std::move(v_).apply(FWD(fn), FWD(args)...); }
  {
    return ::std::move(v_).apply(FWD(fn), FWD(args)...);
  }

  /**
   * @brief Eliminates the choice, converting each branch's result to `Ret`
   *
   * @tparam Ret Type the results convert to
   * @param fn Callable applied on the active alternative
   * @param args Additional arguments, appended after the alternative's content
   * @return The callable's result, converted to `Ret`
   */
  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_r(Fn &&fn, Args &&...args) & //
      noexcept(noexcept(v_.template apply_r<Ret>(FWD(fn), FWD(args)...))) -> Ret
    requires requires { v_.template apply_r<Ret>(FWD(fn), FWD(args)...); }
  {
    return v_.template apply_r<Ret>(FWD(fn), FWD(args)...);
  }

  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_r(Fn &&fn, Args &&...args) const & //
      noexcept(noexcept(v_.template apply_r<Ret>(FWD(fn), FWD(args)...))) -> Ret
    requires requires { v_.template apply_r<Ret>(FWD(fn), FWD(args)...); }
  {
    return v_.template apply_r<Ret>(FWD(fn), FWD(args)...);
  }

  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_r(Fn &&fn, Args &&...args) && //
      noexcept(noexcept(::std::move(v_).template apply_r<Ret>(FWD(fn), FWD(args)...))) -> Ret
    requires requires { ::std::move(v_).template apply_r<Ret>(FWD(fn), FWD(args)...); }
  {
    return ::std::move(v_).template apply_r<Ret>(FWD(fn), FWD(args)...);
  }

  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_r(Fn &&fn, Args &&...args) const && //
      noexcept(noexcept(::std::move(v_).template apply_r<Ret>(FWD(fn), FWD(args)...))) -> Ret
    requires requires { ::std::move(v_).template apply_r<Ret>(FWD(fn), FWD(args)...); }
  {
    return ::std::move(v_).template apply_r<Ret>(FWD(fn), FWD(args)...);
  }

  /**
   * @brief Eliminates the choice, keyed by the alternative's type
   *
   * The active arm receives `std::in_place_type<T>` for the alternative held, followed by its
   * content as `copack`'s `apply_type` hands it over.
   *
   * @param fn Callable applied on the tag and the alternative's content
   * @param args Additional arguments, appended after the content
   * @return The callable's result
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_type(Fn &&fn, Args &&...args) & //
      noexcept(noexcept(v_.apply_type(FWD(fn), FWD(args)...))) -> decltype(auto)
    requires requires { v_.apply_type(FWD(fn), FWD(args)...); }
  {
    return v_.apply_type(FWD(fn), FWD(args)...);
  }

  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_type(Fn &&fn, Args &&...args) const & //
      noexcept(noexcept(v_.apply_type(FWD(fn), FWD(args)...))) -> decltype(auto)
    requires requires { v_.apply_type(FWD(fn), FWD(args)...); }
  {
    return v_.apply_type(FWD(fn), FWD(args)...);
  }

  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_type(Fn &&fn, Args &&...args) && //
      noexcept(noexcept(::std::move(v_).apply_type(FWD(fn), FWD(args)...))) -> decltype(auto)
    requires requires { ::std::move(v_).apply_type(FWD(fn), FWD(args)...); }
  {
    return ::std::move(v_).apply_type(FWD(fn), FWD(args)...);
  }

  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_type(Fn &&fn, Args &&...args) const && //
      noexcept(noexcept(::std::move(v_).apply_type(FWD(fn), FWD(args)...))) -> decltype(auto)
    requires requires { ::std::move(v_).apply_type(FWD(fn), FWD(args)...); }
  {
    return ::std::move(v_).apply_type(FWD(fn), FWD(args)...);
  }

  /**
   * @brief Eliminates the choice, keyed by the alternative's type, converting the result to `Ret`
   *
   * @tparam Ret Type the results convert to
   * @param fn Callable applied on the tag and the alternative's content
   * @param args Additional arguments, appended after the content
   * @return The callable's result, converted to `Ret`
   */
  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_type_r(Fn &&fn, Args &&...args) & //
      noexcept(noexcept(v_.template apply_type_r<Ret>(FWD(fn), FWD(args)...))) -> Ret
    requires requires { v_.template apply_type_r<Ret>(FWD(fn), FWD(args)...); }
  {
    return v_.template apply_type_r<Ret>(FWD(fn), FWD(args)...);
  }

  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_type_r(Fn &&fn, Args &&...args) const & //
      noexcept(noexcept(v_.template apply_type_r<Ret>(FWD(fn), FWD(args)...))) -> Ret
    requires requires { v_.template apply_type_r<Ret>(FWD(fn), FWD(args)...); }
  {
    return v_.template apply_type_r<Ret>(FWD(fn), FWD(args)...);
  }

  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_type_r(Fn &&fn, Args &&...args) && //
      noexcept(noexcept(::std::move(v_).template apply_type_r<Ret>(FWD(fn), FWD(args)...))) -> Ret
    requires requires { ::std::move(v_).template apply_type_r<Ret>(FWD(fn), FWD(args)...); }
  {
    return ::std::move(v_).template apply_type_r<Ret>(FWD(fn), FWD(args)...);
  }

  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto apply_type_r(Fn &&fn, Args &&...args) const && //
      noexcept(noexcept(::std::move(v_).template apply_type_r<Ret>(FWD(fn), FWD(args)...))) -> Ret
    requires requires { ::std::move(v_).template apply_type_r<Ret>(FWD(fn), FWD(args)...); }
  {
    return ::std::move(v_).template apply_type_r<Ret>(FWD(fn), FWD(args)...);
  }

  /**
   * @brief Maps the alternatives, the branch results forming a new normalized choice
   *
   * As `copack`'s `transform`, staying a carrier: the branch results flatten, deduplicate and
   * sort. A returned `choice` stays whole - an atom, nested as one alternative - where a returned
   * bare `copack` dissolves into the set.
   *
   * @param fn Callable applied on the active alternative; `fn::overload` fuses arms into one
   * @return A choice of the normalized branch-result set, holding the active branch's result
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) & //
      noexcept(noexcept(v_.transform(FWD(fn))))
          -> just<typename detail::_copack_apply_result<detail::_collapsing_copack_tag, Fn &&, value_type &>::type>
    requires typelist_applicable<Fn, value_type &>
  {
    using type = detail::_copack_apply_result<detail::_collapsing_copack_tag, Fn &&, value_type &>::type;
    return just<type>{detail::_just_from_invoke, [&fn, this]() -> decltype(auto) { return v_.transform(FWD(fn)); }};
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) const & //
      noexcept(noexcept(v_.transform(FWD(fn)))) -> just<
          typename detail::_copack_apply_result<detail::_collapsing_copack_tag, Fn &&, value_type const &>::type>
    requires typelist_applicable<Fn, value_type const &>
  {
    using type = detail::_copack_apply_result<detail::_collapsing_copack_tag, Fn &&, value_type const &>::type;
    return just<type>{detail::_just_from_invoke, [&fn, this]() -> decltype(auto) { return v_.transform(FWD(fn)); }};
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) && //
      noexcept(noexcept(::std::move(v_).transform(FWD(fn))))
          -> just<typename detail::_copack_apply_result<detail::_collapsing_copack_tag, Fn &&, value_type &&>::type>
    requires typelist_applicable<Fn, value_type &&>
  {
    using type = detail::_copack_apply_result<detail::_collapsing_copack_tag, Fn &&, value_type &&>::type;
    return just<type>{detail::_just_from_invoke,
                      [&fn, this]() -> decltype(auto) { return ::std::move(v_).transform(FWD(fn)); }};
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) const && //
      noexcept(noexcept(::std::move(v_).transform(FWD(fn)))) -> just<
          typename detail::_copack_apply_result<detail::_collapsing_copack_tag, Fn &&, value_type const &&>::type>
    requires typelist_applicable<Fn, value_type const &&>
  {
    using type = detail::_copack_apply_result<detail::_collapsing_copack_tag, Fn &&, value_type const &&>::type;
    return just<type>{detail::_just_from_invoke,
                      [&fn, this]() -> decltype(auto) { return ::std::move(v_).transform(FWD(fn)); }};
  }

  /**
   * @brief Binds the alternatives: every branch returns a `just`, joined into one
   *
   * If branch results have the same type after cv/ref removal, the result keeps that `just` type.
   * Otherwise their payloads form a normalized choice: a returned choice contributes its
   * alternatives, an ordinary `just<T>` contributes `T`, and `just<void>` contributes `pack<>`.
   * Use `transform` for bare results and pipeline `fn::and_then` for supported transitions to
   * another carrier family.
   *
   * @param fn Callable applied on the active alternative; `fn::overload` fuses arms into one
   * @return The joined `just` of the branches' results
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto and_then(Fn &&fn) & //
      noexcept(noexcept(detail::_join_just_apply<detail::_joining_superset_tag>(v_, FWD(fn)))) ->
      typename detail::_copack_apply_result<detail::_joining_superset_tag, Fn &&, value_type &>::type
    requires typelist_applicable<Fn, value_type &>
  {
    static_assert(
        some_just<typename detail::_copack_apply_result<detail::_joining_superset_tag, Fn &&, value_type &>::type>);
    return detail::_join_just_apply<detail::_joining_superset_tag>(v_, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto and_then(Fn &&fn) const & //
      noexcept(noexcept(detail::_join_just_apply<detail::_joining_superset_tag>(v_, FWD(fn)))) ->
      typename detail::_copack_apply_result<detail::_joining_superset_tag, Fn &&, value_type const &>::type
    requires typelist_applicable<Fn, value_type const &>
  {
    static_assert(
        some_just<
            typename detail::_copack_apply_result<detail::_joining_superset_tag, Fn &&, value_type const &>::type>);
    return detail::_join_just_apply<detail::_joining_superset_tag>(v_, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto and_then(Fn &&fn) && //
      noexcept(noexcept(detail::_join_just_apply<detail::_joining_superset_tag>(::std::move(v_), FWD(fn)))) ->
      typename detail::_copack_apply_result<detail::_joining_superset_tag, Fn &&, value_type &&>::type
    requires typelist_applicable<Fn, value_type &&>
  {
    static_assert(
        some_just<typename detail::_copack_apply_result<detail::_joining_superset_tag, Fn &&, value_type &&>::type>);
    return detail::_join_just_apply<detail::_joining_superset_tag>(::std::move(v_), FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto and_then(Fn &&fn) const && //
      noexcept(noexcept(detail::_join_just_apply<detail::_joining_superset_tag>(::std::move(v_), FWD(fn)))) ->
      typename detail::_copack_apply_result<detail::_joining_superset_tag, Fn &&, value_type const &&>::type
    requires typelist_applicable<Fn, value_type const &&>
  {
    static_assert(
        some_just<
            typename detail::_copack_apply_result<detail::_joining_superset_tag, Fn &&, value_type const &&>::type>);
    return detail::_join_just_apply<detail::_joining_superset_tag>(::std::move(v_), FWD(fn));
  }

private:
  template <typename> friend struct just;

  template <typename Fn> constexpr explicit just(detail::_just_from_invoke_t, Fn &&make) : v_(FWD(make)()) {}
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

namespace detail {
template <typename Lh, typename Rh>
using _just_fold_t = decltype(_fold_detail::fold<typename ::std::remove_cvref_t<Lh>::value_type,
                                                 typename ::std::remove_cvref_t<Rh>::value_type>(
    ::std::declval<Lh>().value(), ::std::declval<Rh>().value()));
template <typename Lh, typename Rh>
constexpr inline bool _nothrow_just_fold
    = noexcept(_fold_detail::fold<typename ::std::remove_cvref_t<Lh>::value_type,
                                  typename ::std::remove_cvref_t<Rh>::value_type>(::std::declval<Lh>().value(),
                                                                                  ::std::declval<Rh>().value()))
      && ::std::is_nothrow_constructible_v<just<_just_fold_t<Lh, Rh>>, _just_fold_t<Lh, Rh>>;
} // namespace detail

// The conjunction inside the cluster: just & just folds the payloads and stays just, and
// just<void> is the product's unit - it elides, whatever the other operand.
template <typename Lh, typename Rh>
  requires detail::_some_just<Lh> && detail::_some_just<Rh>
           && (not ::std::is_void_v<typename ::std::remove_cvref_t<Lh>::value_type>)
           && (not ::std::is_void_v<typename ::std::remove_cvref_t<Rh>::value_type>)
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) //
    noexcept(detail::_nothrow_just_fold<Lh, Rh>)
{
  using VL = ::std::remove_cvref_t<Lh>::value_type;
  using VR = ::std::remove_cvref_t<Rh>::value_type;
  return just<detail::_just_fold_t<Lh, Rh>>{::fn::detail::_fold_detail::fold<VL, VR>(FWD(lh).value(), FWD(rh).value())};
}

template <typename Lh, typename Rh>
  requires detail::_some_just<Lh> && ::std::is_void_v<typename ::std::remove_cvref_t<Lh>::value_type>
           && detail::_some_just<Rh>
[[nodiscard]] constexpr auto operator&(Lh &&, Rh &&rh) //
    noexcept(::std::is_nothrow_constructible_v<::std::remove_cvref_t<Rh>, Rh>) -> ::std::remove_cvref_t<Rh>
{
  return ::std::remove_cvref_t<Rh>{FWD(rh)};
}

template <typename Lh, typename Rh>
  requires detail::_some_just<Lh> && (not ::std::is_void_v<typename ::std::remove_cvref_t<Lh>::value_type>)
           && detail::_some_just<Rh> && ::std::is_void_v<typename ::std::remove_cvref_t<Rh>::value_type>
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&) //
    noexcept(::std::is_nothrow_constructible_v<::std::remove_cvref_t<Lh>, Lh>) -> ::std::remove_cvref_t<Lh>
{
  return ::std::remove_cvref_t<Lh>{FWD(lh)};
}

namespace detail {
template <typename T>
concept _identity_expected = _some_expected<T> && empty_copack<typename ::std::remove_cvref_t<T>::error_type>;
template <typename T>
concept _cluster_operand = _some_just<T> || _identity_expected<T>;
template <typename T>
concept _some_carrier = _some_expected<T> || _some_optional<T> || _some_just<T>;

// How a side enters the total disjunction's result: not at all (dead), as the unit pack<>, or as
// its value - the three-way split keeps the dead and void conjuncts from naming an accessor.
template <typename T>
constexpr inline int _inject_kind
    = ::std::is_void_v<typename ::std::remove_cvref_t<T>::value_type> ? 1 : (::fn::detail::_dead_value<T> ? 0 : 2);
template <int Kind, typename Type, typename Side> struct _nothrow_total_inject {
  static constexpr bool value = true;
};
template <typename Type, typename Side> struct _nothrow_total_inject<1, Type, Side> {
  static constexpr bool value = ::std::is_nothrow_constructible_v<Type, pack<>>;
};
template <typename Type, typename Side> struct _nothrow_total_inject<2, Type, Side> {
  static constexpr bool value = ::std::is_nothrow_constructible_v<Type, decltype(::std::declval<Side>().value())>;
};

// Type stays a template parameter so both branches are dependent: a non-dependent discarded
// statement would still be checked against choices without a pack<> alternative.
template <typename Type, typename Side>
[[nodiscard]] constexpr auto _total_inject(Side &&side) //
    noexcept(_nothrow_total_inject<_inject_kind<Side>, Type, Side>::value) -> Type
{
  if constexpr (::std::is_void_v<typename ::std::remove_cvref_t<Side>::value_type>)
    return Type{pack<>{}};
  else
    return Type{FWD(side).value()};
}
} // namespace detail

// The total disjunction: a cluster operand - just, choice, or the identity expected - puts an
// uninhabited factor into the error product, so the result never fails and collapses into the
// cluster: just of the value sum, which is a choice when the union is genuine. The leftmost
// engaged operand wins; a cluster operand is always engaged.
template <typename Lh, typename Rh>
  requires(detail::_cluster_operand<Lh> || detail::_cluster_operand<Rh>) //
          && detail::_some_carrier<Lh> && detail::_some_carrier<Rh>
          && (not ::std::is_void_v<typename ::std::remove_cvref_t<Lh>::value_type>)
          && ::std::is_same_v<typename ::std::remove_cvref_t<Lh>::value_type,
                              typename ::std::remove_cvref_t<Rh>::value_type>
[[nodiscard]] constexpr auto operator|(Lh &&lh, Rh &&rh) //
    noexcept(detail::_nothrow_total_inject<detail::_inject_kind<Lh>,
                                           ::fn::just<typename ::std::remove_cvref_t<Lh>::value_type>, Lh>::value
             && detail::_nothrow_total_inject<detail::_inject_kind<Rh>,
                                              ::fn::just<typename ::std::remove_cvref_t<Lh>::value_type>, Rh>::value)
{
  using type = ::fn::just<typename ::std::remove_cvref_t<Lh>::value_type>;
  if constexpr (detail::_cluster_operand<Lh>) {
    return type{FWD(lh).value()};
  } else {
    if (lh.has_value())
      return type{FWD(lh).value()};
    return type{FWD(rh).value()};
  }
}

template <typename Lh, typename Rh>
  requires(detail::_cluster_operand<Lh> || detail::_cluster_operand<Rh>) //
          && detail::_some_carrier<Lh> && detail::_some_carrier<Rh>
          && ::std::is_void_v<typename ::std::remove_cvref_t<Lh>::value_type>
          && ::std::is_void_v<typename ::std::remove_cvref_t<Rh>::value_type>
[[nodiscard]] constexpr auto operator|(Lh &&, Rh &&) noexcept -> ::fn::just<void>
{
  return ::fn::just<void>{};
}

template <typename Lh, typename Rh>
  requires(detail::_cluster_operand<Lh> || detail::_cluster_operand<Rh>) //
          && detail::_some_carrier<Lh> && detail::_some_carrier<Rh>
          && (not ::std::is_same_v<typename ::std::remove_cvref_t<Lh>::value_type,
                                   typename ::std::remove_cvref_t<Rh>::value_type>)
[[nodiscard]] constexpr auto operator|(Lh &&lh, Rh &&rh) //
    noexcept(detail::_nothrow_total_inject<detail::_inject_kind<Lh>, ::fn::just<::fn::detail::_disjoined_t<Lh, Rh>>,
                                           Lh>::value
             && detail::_nothrow_total_inject<detail::_inject_kind<Rh>, ::fn::just<::fn::detail::_disjoined_t<Lh, Rh>>,
                                              Rh>::value)
{
  using type = ::fn::just<::fn::detail::_disjoined_t<Lh, Rh>>;
  if constexpr (detail::_cluster_operand<Lh>) {
    return detail::_total_inject<type>(FWD(lh));
  } else {
    if constexpr (not ::fn::detail::_dead_value<Lh>) {
      if (lh.has_value())
        return detail::_total_inject<type>(FWD(lh));
    }
    return detail::_total_inject<type>(FWD(rh));
  }
}

// CTAD for just, including the deduced spelling of the void carrier: just{}
template <typename T> just(T) -> just<T>;
template <typename T> explicit just(::std::in_place_type_t<T>, auto &&...) -> just<T>;
just() -> just<void>;
explicit just(::std::in_place_t) -> just<void>;

namespace detail {
template <typename... Ts>
constexpr inline bool _deducible_choice_alternative = sizeof...(Ts) == 1 && (_is_valid_copack_subtype<Ts> && ...);
} // namespace detail

// Value and in-place deduction for the choice alias use guides declared on just. The packs
// allow the ordinary just guides above to remain more specialized for just deduction.
// enable_if rather than a requires-clause: Clang 19 crashes deriving the alias guides from a
// constraint on a pack.
template <typename... Ts, ::std::enable_if_t<detail::_deducible_choice_alternative<Ts...>, int> = 0>
explicit just(Ts...) -> just<copack<Ts...>>;
template <typename... Ts, ::std::enable_if_t<detail::_deducible_choice_alternative<Ts...>, int> = 0>
explicit just(::std::in_place_type_t<Ts...>, auto &&...) -> just<copack<Ts...>>;
// Deduces what copy deduction does. GCC 12 and 13 find copy deduction through the alias
// ambiguous with the guides derived from the copy and move constructors.
template <typename... Ts> just(just<copack<Ts...>>) -> just<copack<Ts...>>;

/**
 * @brief Builds the canonical `choice` for any list of types
 *
 * The construction alias over `choice`, exactly as `copack_for` stands to `copack`: flattens,
 * deduplicates and sorts into the canonical order. Spell `choice_for` rather than `choice`, so
 * that no spelling in your project is tied to one compiler's alternative order.
 *
 * @tparam Ts Types to combine - alternatives and copacks of them, in any order, duplicates allowed
 */
template <typename... Ts> using choice_for = just<copack_for<Ts...>>;

namespace detail {
// Validate the alternative before naming choice<T>, which can trigger a payload static_assert.
// Guard the noexcept helpers separately as well, following _nothrow_copack_lift for MSVC.
template <typename Src>
concept _choice_liftable = _is_valid_copack_subtype<::std::decay_t<Src>>;

template <typename Src> constexpr inline bool _nothrow_choice_lift = false;
template <typename Src>
  requires _choice_liftable<Src>
constexpr inline bool _nothrow_choice_lift<Src>
    = ::std::is_nothrow_constructible_v<choice<::std::decay_t<Src>>, ::std::in_place_type_t<::std::decay_t<Src>>, Src>;

template <typename Src> constexpr inline bool _nothrow_copack_choice_lift = false;
template <typename Src>
  requires _some_copack<::std::remove_cvref_t<Src>> && (not ::std::is_same_v<::std::remove_cvref_t<Src>, copack<>>)
constexpr inline bool _nothrow_copack_choice_lift<Src>
    = ::std::is_nothrow_constructible_v<just<::std::remove_cvref_t<Src>>, Src>;

template <typename T, typename... Args> constexpr inline bool _nothrow_choice_emplace = false;
template <typename T, typename... Args>
  requires _is_valid_copack_subtype<T>
constexpr inline bool _nothrow_choice_emplace<T, Args...>
    = ::std::is_nothrow_constructible_v<choice<T>, ::std::in_place_type_t<T>, Args...>;
} // namespace detail

// Lifts
/**
 * @brief Constructs a single-alternative choice from a value
 *
 * The alternative is the decayed source type: cv/ref qualifiers are removed, and arrays and
 * functions become pointers. Unlike `choice{x}`, this overload constructs from arrays and
 * functions through their pointer conversions, and wraps an existing choice as an alternative.
 *
 * @param src Value to lift
 * @return A `choice` over the decayed type of `src`, holding the constructed alternative
 */
[[nodiscard]] constexpr auto as_choice(auto &&src) //
    noexcept(detail::_nothrow_choice_lift<decltype(src)>) -> decltype(auto)
  requires detail::_choice_liftable<decltype(src)>
           && ::std::is_constructible_v<choice<::std::decay_t<decltype(src)>>,
                                        ::std::in_place_type_t<::std::decay_t<decltype(src)>>, decltype(src)>
{
  using type = ::std::decay_t<decltype(src)>;
  return choice<type>(::std::in_place_type<type>, FWD(src));
}

/**
 * @brief Lifts a copack into the choice over its alternatives
 *
 * The copack is the payload itself, so the choice is the one `just{copack}` deduces.
 *
 * @param src Copack to lift
 * @return The `choice` over the alternatives of `src`, holding its value
 */
template <typename Src>
  requires some_copack<::std::remove_cvref_t<Src>> && (not ::std::is_same_v<::std::remove_cvref_t<Src>, copack<>>)
           && ::std::is_constructible_v<just<::std::remove_cvref_t<Src>>, Src>
[[nodiscard]] constexpr auto as_choice(Src &&src) //
    noexcept(detail::_nothrow_copack_choice_lift<Src>) -> decltype(auto)
{
  return just<::std::remove_cvref_t<Src>>(FWD(src));
}

/**
 * @brief Lifts arguments into a singular choice of `T`, constructed in place
 *
 * @tparam T The sole alternative
 * @param args Arguments to construct the alternative from
 * @return `choice<T>` holding the alternative
 */
template <typename T>
[[nodiscard]] constexpr auto as_choice(::std::in_place_type_t<T>, auto &&...args) //
    noexcept(detail::_nothrow_choice_emplace<T, decltype(args)...>) -> decltype(auto)
  requires detail::_is_valid_copack_subtype<T>
           && ::std::is_constructible_v<choice<T>, ::std::in_place_type_t<T>, decltype(args)...>
{
  return choice<T>(::std::in_place_type<T>, FWD(args)...);
}

} // namespace LIBFN_VERSION
} // namespace fn

#include <fn/detail/macro_end.hpp>

#endif // INCLUDE_FN_JUST
