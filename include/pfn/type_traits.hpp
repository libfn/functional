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
// The answer where the compiler lacks the builtin. A temporary is reported only where the binding
// rules make one certain, never otherwise: a false report would reject valid code on these
// compilers alone. A class source which is not reference-related to the referent may convert
// through a conversion function, which a library check cannot see, so it is reported as creating
// no temporary.
template <class T, class U, bool Converts> consteval auto _emulated_from_temporary() noexcept -> bool
{
  using referent = ::std::remove_reference_t<T>;
  using source = ::std::remove_cvref_t<U>;
  if constexpr (not ::std::is_reference_v<T>
                || (Converts ? not ::std::is_convertible_v<U, T> : not ::std::is_constructible_v<T, U>)) {
    return false;
  } else if constexpr (not ::std::is_class_v<source> && not ::std::is_union_v<source>) {
    // Without conversion functions, a binding goes through a temporary exactly when a reference to
    // const volatile, which binds no rvalue, cannot make it. An xvalue binds directly, so it is
    // asked as an lvalue of its type.
    using asked = ::std::conditional_t<::std::is_rvalue_reference_v<U>, ::std::remove_reference_t<U> &, U>;
    if constexpr (Converts)
      return not ::std::is_convertible_v<asked, referent const volatile &>;
    else
      return not ::std::is_constructible_v<referent const volatile &, asked>;
  } else if constexpr (::std::is_same_v<source, ::std::remove_cv_t<referent>>
                       || ::std::is_base_of_v<::std::remove_cv_t<referent>, source>) {
    // A reference-related source binds without consulting conversion functions: a glvalue
    // directly, a prvalue through its materialized temporary.
    return not ::std::is_reference_v<U>;
  } else {
    return false;
  }
}

template <class T, class U> consteval auto _constructs_from_temporary() noexcept -> bool
{
// MSVC's preprocessor rejects __has_builtin, so gate _MSC_VER first.
#if defined(_MSC_VER)
  return _emulated_from_temporary<T, U, false>();
#elif __has_builtin(__reference_constructs_from_temporary)
  return __reference_constructs_from_temporary(T, U);
#else
  return _emulated_from_temporary<T, U, false>();
#endif
}

template <class T, class U> consteval auto _converts_from_temporary() noexcept -> bool
{
#if defined(_MSC_VER)
  return _emulated_from_temporary<T, U, true>();
#elif __has_builtin(__reference_converts_from_temporary)
  return __reference_converts_from_temporary(T, U);
#else
  return _emulated_from_temporary<T, U, true>();
#endif
}
} // namespace detail

/**
 * @brief Checks if direct-initializing a `T` reference from a `U` binds it to a temporary
 *
 * `std::reference_constructs_from_temporary` as specified for C++23 ([meta.unary.prop]), for
 * C++20 compilers. Where the compiler provides `__reference_constructs_from_temporary` the answer
 * is the standard's; elsewhere (GCC 12, MSVC) it is a library emulation which never
 * reports a temporary that is not created, but misses one created by converting a class source
 * not reference-related to the referent.
 *
 * @tparam T Reference type to initialize; a non-reference type answers `false`
 * @tparam U Type of the initializer; a non-reference type is a prvalue
 */
template <class T, class U>
struct reference_constructs_from_temporary : ::std::bool_constant<detail::_constructs_from_temporary<T, U>()> {};
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
struct reference_converts_from_temporary : ::std::bool_constant<detail::_converts_from_temporary<T, U>()> {};
/**
 * @brief Variable form of `pfn::reference_converts_from_temporary`
 */
template <class T, class U>
constexpr inline bool reference_converts_from_temporary_v = reference_converts_from_temporary<T, U>::value;

} // namespace LIBFN_VERSION_BASE
} // namespace pfn

#endif // INCLUDE_PFN_TYPE_TRAITS
