// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_DETAIL_MACRO_DEDUCED_RETURN
#define INCLUDE_FN_DETAIL_MACRO_DEDUCED_RETURN

// Spell a function's deduced return type explicitly on MSVC only. MSVC mis-resolves a deduced
// (`auto`/`decltype(auto)`) return whose type derives from a body-local alias, leaking it as an
// opaque dependent type; an explicit trailing return type is the fix. Every other compiler keeps
// `decltype(auto)` — the natural spelling — which by construction deduces `decltype(EXPR)` on
// `return EXPR;`, so the two are the same type and this is a no-op off MSVC.
//
// Confining `decltype(EXPR)` to MSVC also keeps it off clang <= 20, which in C++23 substitutes a
// non-viable constrained overload's trailing return type BEFORE its requires-clause rejects it
// (fixed clang 21), so the explicit form would fail to compile instead of removing the non-viable
// function from the overload set.
#ifndef _MSC_VER
#define DEDUCED_RETURN(...) decltype(auto)
#else
#define DEDUCED_RETURN(...) decltype(__VA_ARGS__)
#endif

#endif // INCLUDE_FN_DETAIL_MACRO_DEDUCED_RETURN
