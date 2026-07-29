// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_PFN_UTILITY
#define INCLUDE_PFN_UTILITY

#include <libfn_version.hpp>

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

} // namespace LIBFN_VERSION_BASE
} // namespace pfn

#endif // INCLUDE_PFN_UTILITY
