// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_TESTS_UTIL_ALLOW_DEPRECATED
#define INCLUDE_TESTS_UTIL_ALLOW_DEPRECATED

// Brackets the tests of a deprecated entity, so that only its uses elsewhere warn.
#if defined(_MSC_VER) && not defined(__clang__)
#define ALLOW_DEPRECATED_BEGIN __pragma(warning(push)) __pragma(warning(disable : 4996))
#define ALLOW_DEPRECATED_END __pragma(warning(pop))
#else
#define ALLOW_DEPRECATED_BEGIN                                                                                         \
  _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#define ALLOW_DEPRECATED_END _Pragma("GCC diagnostic pop")
#endif

#endif // INCLUDE_TESTS_UTIL_ALLOW_DEPRECATED
