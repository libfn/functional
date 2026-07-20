// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

// Deliberately no include guard: this header and fn/detail/macro_end.hpp bracket the section of a
// header that uses the macros below; push_macro/pop_macro make the bracketing safe under nesting
// and preserve any prior user definition.

#pragma push_macro("FWD")
#undef FWD // NOSONAR cpp:S959 saved by push_macro above

// This FWD macro is a functional equivalent to std::forward<decltype(v)>(v),
// but it saves compilation time (and typing) when used frequently.
#define FWD(...) static_cast<decltype(__VA_ARGS__) &&>(__VA_ARGS__)

#pragma push_macro("DEDUCED_RETURN")
#undef DEDUCED_RETURN // NOSONAR cpp:S959 saved by push_macro above

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
