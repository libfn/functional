// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/detail/macro_deduced_return.hpp>

#include <catch2/catch_all.hpp>

#include <type_traits>

// Defined, not just declared: the helpers below reference it in their bodies, and some compilers
// emit those (otherwise unused) functions, so the reference must resolve at link time.
int var{};

namespace {

// DEDUCED_RETURN(EXPR) names exactly decltype(EXPR) on either macro branch: the explicit
// `decltype(EXPR)`, or `decltype(auto)` deducing the same from `return EXPR;`. So the spelled
// return type round-trips each value category identically, whether or not the guard is active.
[[maybe_unused]] auto returns_prvalue() -> DEDUCED_RETURN(int{}) { return int{}; }
[[maybe_unused]] auto returns_lvalue() -> DEDUCED_RETURN((var)) { return (var); }
[[maybe_unused]] auto returns_xvalue() -> DEDUCED_RETURN(static_cast<int &&>(var)) { return static_cast<int &&>(var); }

static_assert(std::is_same_v<decltype(returns_prvalue()), int>);
static_assert(std::is_same_v<decltype(returns_lvalue()), int &>);
static_assert(std::is_same_v<decltype(returns_xvalue()), int &&>);

} // namespace

TEST_CASE("Dummy") { SUCCEED(); }
