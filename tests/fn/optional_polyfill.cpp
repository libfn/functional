// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

// Run the whole pfn polyfill conformance suite against fn::optional. Bring the
// subject-under-test aliases into the global namespace, then include pfn/optional.cpp.
// The fn::optional copack/graded/pack behaviour is covered in fn/optional.cpp.

#include <fn/optional.hpp>

using fn::make_optional;
using fn::optional;

#define PFN_TEST_NESTED
#include "pfn/optional.cpp"
