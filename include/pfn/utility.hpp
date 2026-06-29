// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_PFN_UTILITY
#define INCLUDE_PFN_UTILITY

namespace pfn {

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

} // namespace pfn

#endif // INCLUDE_PFN_UTILITY
