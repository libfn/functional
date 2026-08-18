// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_LIBFN_VERSION
#define INCLUDE_LIBFN_VERSION

// Mode-less version for pfn, which never uses C++26 features.
#define LIBFN_VERSION_BASE v0_1

#ifdef LIBFN_CXX26
#define LIBFN_VERSION v0_1_cxx26
#else
#define LIBFN_VERSION v0_1
#endif

#endif // INCLUDE_LIBFN_VERSION
