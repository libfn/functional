// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

// Run the whole pfn polyfill conformance suite against fn::expected. Bring the
// subject-under-test aliases into the global namespace, then include pfn/expected.cpp.
// The fn::expected sum/graded/pack behaviour is covered in fn/expected.cpp.

#include <fn/expected.hpp>

using fn::bad_expected_access;
using fn::expected;
using fn::unexpect;
using fn::unexpect_t;
using fn::unexpected;

#define PFN_TEST_NESTED
#include "pfn/expected.cpp"
