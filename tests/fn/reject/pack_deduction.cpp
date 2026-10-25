// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/pack.hpp>

namespace {
auto f(int i) -> int { return i; }
} // namespace

// Deduction holds a value; a function is refused, never decayed to a pointer.

void deduce_function()
{
  (void)fn::pack{&f};
#ifdef LIBFN_REJECT_deduce_function
  (void)fn::pack{f};
#endif
}
