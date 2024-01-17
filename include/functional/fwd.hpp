// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_FWD
#define INCLUDE_FUNCTIONAL_FWD

namespace fn {
// NOTE Some forward declarations can lead to hard to troubleshoot compilation
//      errors. Only declare select, useful monostate datatypes here, e.g.
//      monadic operations.
struct and_then_t;
struct transform_t;
struct transform_error_t;
struct or_else_t;
struct recover_t;
struct fail_t;
struct inspect_t;
} // namespace fn

#endif // INCLUDE_FUNCTIONAL_FWD
