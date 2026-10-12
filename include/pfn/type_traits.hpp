// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_PFN_TYPE_TRAITS
#define INCLUDE_PFN_TYPE_TRAITS

#include <libfn_version.hpp>

#include <type_traits>

namespace pfn {
inline namespace LIBFN_VERSION_BASE {

namespace detail {
template <class T, class U, bool Converts>
using _binding = ::std::conditional_t<Converts, ::std::is_convertible<U, T>, ::std::is_constructible<T, U>>;

template <class U>
constexpr inline bool _is_class_source
    = ::std::is_class_v<::std::remove_cvref_t<U>> || ::std::is_union_v<::std::remove_cvref_t<U>>;

// The standard asks about VAL<U>, which for a non-reference U of non-class, non-array type is a
// prvalue whose cv qualification is dropped: `const int` initializes as `int` does. Clang 19 and 20
// builtins answer for the cv-qualified type, so the builtins and the emulation both ask about this one.
template <class U>
using _val_t = ::std::conditional_t<::std::is_reference_v<U> || _is_class_source<U> || ::std::is_array_v<U>, U,
                                    ::std::remove_cv_t<U>>;

// The answer where the compiler lacks the builtin. A temporary is reported only where the binding
// rules make one certain, never otherwise: a false report would reject valid code on these
// compilers alone. A class source which is not reference-related to the referent may convert
// through a conversion function, which a library check cannot see, so it is reported as creating
// no temporary; so is any binding of a reference to an array.
template <class T, class U, bool Converts> struct _emulated_impl : ::std::false_type {};

// Without conversion functions, a binding goes through a temporary exactly when a reference to
// const volatile, which binds no rvalue, cannot make it. An xvalue binds directly, so the probe
// asks it as an lvalue of its type. A reference to an array is left out: compilers disagree on
// binding one to a temporary array.
template <class T, class U, bool Converts>
  requires ::std::is_reference_v<T> && (not ::std::is_array_v<::std::remove_reference_t<T>>)
           && _binding<T, U, Converts>::value && (not _is_class_source<U>)
struct _emulated_impl<T, U, Converts>
    : ::std::bool_constant<not _binding<
          ::std::remove_reference_t<T> const volatile &,
          ::std::conditional_t<::std::is_rvalue_reference_v<U>, ::std::remove_reference_t<U> &, U>, Converts>::value> {
};

// A reference-related source binds without consulting conversion functions: a glvalue directly, a
// prvalue through its materialized temporary.
template <class T, class U, bool Converts>
  requires ::std::is_reference_v<T> && _binding<T, U, Converts>::value && _is_class_source<U>
           && (::std::is_same_v<::std::remove_cvref_t<U>, ::std::remove_cvref_t<T>>
               || ::std::is_base_of_v<::std::remove_cvref_t<T>, ::std::remove_cvref_t<U>>)
struct _emulated_impl<T, U, Converts> : ::std::bool_constant<not ::std::is_reference_v<U>> {};

template <class T, class U, bool Converts> struct _emulated_from_temporary : _emulated_impl<T, _val_t<U>, Converts> {};

// MSVC's preprocessor rejects __has_builtin, so MSVC is gated by version first: Visual Studio 2026
// 18.6 (19.51.36246) is the earliest with both builtins.
#if defined(_MSC_VER) && not defined(__clang__) && _MSC_FULL_VER >= 195136246
template <class T, class U>
constexpr inline bool _constructs_from_temporary = __reference_constructs_from_temporary(T, _val_t<U>);
template <class T, class U>
constexpr inline bool _converts_from_temporary = __reference_converts_from_temporary(T, _val_t<U>);
#elif defined(_MSC_VER) && not defined(__clang__)
template <class T, class U>
constexpr inline bool _constructs_from_temporary = _emulated_from_temporary<T, U, false>::value;
template <class T, class U>
constexpr inline bool _converts_from_temporary = _emulated_from_temporary<T, U, true>::value;
#elif __has_builtin(__reference_constructs_from_temporary) && __has_builtin(__reference_converts_from_temporary)
template <class T, class U>
constexpr inline bool _constructs_from_temporary = __reference_constructs_from_temporary(T, _val_t<U>);
template <class T, class U>
constexpr inline bool _converts_from_temporary = __reference_converts_from_temporary(T, _val_t<U>);
#else
template <class T, class U>
constexpr inline bool _constructs_from_temporary = _emulated_from_temporary<T, U, false>::value;
template <class T, class U>
constexpr inline bool _converts_from_temporary = _emulated_from_temporary<T, U, true>::value;
#endif

template <class R, class Fn, class... Args> constexpr inline bool _invocable_r = false;
template <class R, class Fn, class... Args>
  requires ::std::is_invocable_r_v<R, Fn, Args...>
constexpr inline bool _invocable_r<R, Fn, Args...>
    = not _converts_from_temporary<R, ::std::invoke_result_t<Fn, Args...>>;
} // namespace detail

/**
 * @brief Checks if direct-initializing a `T` reference from a `U` binds it to a temporary
 *
 * `std::reference_constructs_from_temporary` as specified for C++23 ([meta.unary.prop]), for
 * C++20 compilers. Where the compiler provides `__reference_constructs_from_temporary` the answer
 * is the standard's; elsewhere (GCC 12, MSVC before Visual Studio 2026 18.6) it is a library
 * emulation which never reports a temporary that is not created, but misses one created by
 * converting a class source not reference-related to the referent, or one bound by a reference to
 * an array.
 *
 * @tparam T Reference type to initialize; a non-reference type answers `false`
 * @tparam U Type of the initializer; a non-reference type is a prvalue
 */
template <class T, class U>
struct reference_constructs_from_temporary : ::std::bool_constant<detail::_constructs_from_temporary<T, U>> {};
/**
 * @brief Variable form of `pfn::reference_constructs_from_temporary`
 */
template <class T, class U>
constexpr inline bool reference_constructs_from_temporary_v = reference_constructs_from_temporary<T, U>::value;

/**
 * @brief Checks if copy-initializing a `T` reference from a `U` binds it to a temporary
 *
 * `std::reference_converts_from_temporary` as specified for C++23 ([meta.unary.prop]), for C++20
 * compilers, with the same emulation as `pfn::reference_constructs_from_temporary` where the
 * compiler lacks `__reference_converts_from_temporary`.
 *
 * @tparam T Reference type to initialize; a non-reference type answers `false`
 * @tparam U Type of the initializer; a non-reference type is a prvalue
 */
template <class T, class U>
struct reference_converts_from_temporary : ::std::bool_constant<detail::_converts_from_temporary<T, U>> {};
/**
 * @brief Variable form of `pfn::reference_converts_from_temporary`
 */
template <class T, class U>
constexpr inline bool reference_converts_from_temporary_v = reference_converts_from_temporary<T, U>::value;

/**
 * @brief Checks if `Fn` is invocable with `Args...`, its result converting to `R`
 *
 * `std::is_invocable_r` as specified for C++23 ([meta.rel]): the call is not viable where the
 * result would bind a reference `R` to a temporary ([func.require]), which not every standard
 * library checks. Detected through `pfn::reference_converts_from_temporary`.
 *
 * @tparam R Type the result converts to; `void` accepts any result
 * @tparam Fn Callable to probe
 * @tparam Args Argument types
 */
template <class R, class Fn, class... Args>
struct is_invocable_r : ::std::bool_constant<detail::_invocable_r<R, Fn, Args...>> {};
/**
 * @brief Variable form of `pfn::is_invocable_r`
 */
template <class R, class Fn, class... Args>
constexpr inline bool is_invocable_r_v = is_invocable_r<R, Fn, Args...>::value;

/**
 * @brief Checks if `pfn::is_invocable_r` holds and neither the call nor the conversion can throw
 *
 * `std::is_nothrow_invocable_r` as specified for C++23 ([meta.rel]).
 *
 * @tparam R Type the result converts to
 * @tparam Fn Callable to probe
 * @tparam Args Argument types
 */
template <class R, class Fn, class... Args>
struct is_nothrow_invocable_r
    : ::std::bool_constant<is_invocable_r_v<R, Fn, Args...> && ::std::is_nothrow_invocable_r_v<R, Fn, Args...>> {};
/**
 * @brief Variable form of `pfn::is_nothrow_invocable_r`
 */
template <class R, class Fn, class... Args>
constexpr inline bool is_nothrow_invocable_r_v = is_nothrow_invocable_r<R, Fn, Args...>::value;

} // namespace LIBFN_VERSION_BASE
} // namespace pfn

#endif // INCLUDE_PFN_TYPE_TRAITS
