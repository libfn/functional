// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_LIBFN_VERSION
#define INCLUDE_LIBFN_VERSION

// clang 15-18 miscompile this library silently: corrupted values at -O1 and
// above, no diagnostic. Refuse them rather than emit wrong code.
#if defined(__clang__) && __clang_major__ < 19
#error "libfn requires clang 19 or newer; for older toolchains use the 0.1.0 release"
#endif

// Mode-less version for pfn, which never uses C++26 features.
#define LIBFN_VERSION_BASE v0_1_dev

#ifdef LIBFN_CXX26
#define LIBFN_VERSION v0_1_dev_cxx26
#else
#define LIBFN_VERSION v0_1_dev
#endif

#endif // INCLUDE_LIBFN_VERSION
