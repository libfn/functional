// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_PFN_UTILITY
#define INCLUDE_PFN_UTILITY

namespace pfn {

[[noreturn]] inline void unreachable()
{
#if defined(__GNUC__) || defined(__clang__)
  __builtin_unreachable();
#elif defined(_MSC_VER)
  __assume(false);
#endif
}

} // namespace pfn

#endif // INCLUDE_PFN_UTILITY
