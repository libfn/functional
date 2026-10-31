// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/just.hpp>

namespace {
auto f(int i) -> int { return i; }
} // namespace

// Deduction holds a value; an array or a function is refused, never decayed to a pointer.

void deduce_array()
{
  int const x = 1;
  (void)fn::just{x};
#ifdef LIBFN_REJECT_deduce_array
  int const arr[3] = {1, 2, 3};
  (void)fn::just{arr};
#endif
}

void deduce_function()
{
  (void)fn::just{&f};
#ifdef LIBFN_REJECT_deduce_function
  (void)fn::just{f};
#endif
}
