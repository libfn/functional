// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/detail/fwd_macro.hpp"

#include <catch2/catch_all.hpp>

#include <type_traits>
#include <utility>

extern int value;
extern int const const_value;
static_assert(                //
    std::is_same_v<           //
        decltype(FWD(value)), //
        decltype(std::forward<decltype(value)>(value))>);
static_assert(      //
    std::is_same_v< //
        decltype(FWD(const_value)), decltype(std::forward<decltype(const_value)>(const_value))>);

extern int &lvalue;
extern int const &const_lvalue;
static_assert(      //
    std::is_same_v< //
        decltype(FWD(lvalue)), decltype(std::forward<decltype(lvalue)>(lvalue))>);
static_assert(      //
    std::is_same_v< //
        decltype(FWD(const_lvalue)), decltype(std::forward<decltype(const_lvalue)>(const_lvalue))>);

extern int &&rvalue;
extern int const &&const_rvalue;
static_assert(      //
    std::is_same_v< //
        decltype(FWD(rvalue)), decltype(std::forward<decltype(rvalue)>(rvalue))>);
static_assert(      //
    std::is_same_v< //
        decltype(FWD(const_rvalue)), decltype(std::forward<decltype(const_rvalue)>(const_rvalue))>);

TEST_CASE("Dummy") { SUCCEED(); }
