// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_FWD
#define INCLUDE_FUNCTIONAL_FWD

namespace fn {
// NOTE Some forward declarations can lead to hard to troubleshoot compilation
//      errors. Only declare select, useful monostate datatypes here, e.g.
//      monadic operators.
struct and_then_t;
struct transform_t;
struct transform_error_t;
struct or_else_t;
struct recover_t;
struct fail_t;
struct inspect_t;
struct filter_t; // TODO (a.k.a allow-filter)
struct remove_t; // TODO (reverse of filter)
struct match_t;  // TODO (conversion to arbitrary type)
} // namespace fn

template <typename T> struct incomplete;

#endif // INCLUDE_FUNCTIONAL_FWD
