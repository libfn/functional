// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_DETAIL_MACRO_DEDUCED_RETURN
#define INCLUDE_FN_DETAIL_MACRO_DEDUCED_RETURN

// Spell a function's deduced return type explicitly. MSVC mis-resolves a deduced
// (`auto`/`decltype(auto)`) return whose type derives from a body-local alias, leaking it as an
// opaque dependent type; an explicit trailing return type is the fix. But clang <= 20 in C++23
// substitutes a non-viable constrained overload's trailing return type before its requires-clause
// rejects it (fixed clang 21), so an explicit return there can hard-error. We therefore spell it
// explicitly everywhere except clang <= 20, which keeps `decltype(auto)`. The two are the same type
// by construction (`decltype(auto)` on `return EXPR;` deduces `decltype(EXPR)`), so this is a no-op
// on conforming compilers and only changes what MSVC and clang <= 20 each see.
#if defined(__clang__) && __clang_major__ <= 20
#define DEDUCED_RETURN(...) decltype(auto)
#else
#define DEDUCED_RETURN(...) decltype(__VA_ARGS__)
#endif

#endif // INCLUDE_FN_DETAIL_MACRO_DEDUCED_RETURN
