// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_PFN_UTILITY
#define INCLUDE_PFN_UTILITY

#include <libfn_version.hpp>

#include <type_traits>

namespace pfn {
inline namespace LIBFN_VERSION_BASE {

/**
 * @brief Marks a point of provably unreachable control flow: `std::unreachable` as specified
 *        for C++23 ([utility.undefined]), for C++20 compilers
 *
 * Reaching a call is undefined behaviour; the implementation is the compiler's own
 * unreachability intrinsic, so the optimizer may assume every path to the call site is dead.
 */
// LCOV_EXCL_START unreachable by design
[[noreturn]] inline void unreachable()
{
#if defined(__GNUC__) || defined(__clang__)
  __builtin_unreachable();
#elif defined(_MSC_VER)
  __assume(false);
#else
#error "No 'unreachable' intrinsic for this compiler"
#endif
}
// LCOV_EXCL_STOP

namespace detail {
// The type of `std::forward_like<S>(x)` for an `x` of non-reference type X
template <class S, class X>
using _forward_like_t
    = ::std::conditional_t<::std::is_lvalue_reference_v<S>,
                           ::std::conditional_t<::std::is_const_v<::std::remove_reference_t<S>>, X const, X> &,
                           ::std::conditional_t<::std::is_const_v<::std::remove_reference_t<S>>, X const, X> &&>;

// Trivially copyable storage for a side that Policy declares uninhabited. Typed accessors preserve
// the public value/error type and mark access to that side unreachable. The variadic constructor
// lets generic storage operations compile; reaching it is undefined behaviour.
struct _uninhabited_t final {
  template <class... Args> [[noreturn]] _uninhabited_t(Args &&...) noexcept { ::pfn::unreachable(); } // LCOV_EXCL_LINE
};

template <class X, class Policy>
using _stored_t = ::std::conditional_t<Policy::template is_uninhabited<X>, _uninhabited_t, X>;
} // namespace detail

} // namespace LIBFN_VERSION_BASE
} // namespace pfn

#endif // INCLUDE_PFN_UTILITY
