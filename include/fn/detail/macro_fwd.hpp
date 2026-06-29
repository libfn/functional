// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_DETAIL_MACRO_FWD
#define INCLUDE_FN_DETAIL_MACRO_FWD

// This FWD macro is a functional equivalent to std::forward<decltype(v)>(v),
// but it saves compilation time (and typing) when used frequently.
#define FWD(...) static_cast<decltype(__VA_ARGS__) &&>(__VA_ARGS__)

#endif // INCLUDE_FN_DETAIL_MACRO_FWD
