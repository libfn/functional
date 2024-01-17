// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/detail/not_tuple.hpp"

#include <catch2/catch_all.hpp>

namespace fn::detail {

struct Foo final {};
static_assert([](auto &&v) constexpr -> bool {
  return requires { get<0>(v); } //
         && !requires { get<1>(v); };
}(not_tuple<Foo>{}));

static_assert([](auto &&v) constexpr -> bool {
  return requires { get<0>(v); }    //
         && requires { get<1>(v); } //
         && !requires { get<2>(v); };
  ;
}(not_tuple<Foo, Foo>{}));

// clang-format off
struct A { int v; };
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<A>>())), A &&>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<A> const>())), A const &&>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<A> &>())), A &>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<A> const &>())), A const &>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<A> &&>())), A &&>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<A> const &&>())), A const &&>);

static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<A &>>())), A &>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<A &> const>())), A const &>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<A &> &>())), A &>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<A &> const &>())), A const &>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<A &> &&>())), A &>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<A &> const &&>())), A const &>);

static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<A const &>>())), A const &>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<A const &> const>())), A const &>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<A const &> &>())), A const &>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<A const &> const &>())), A const &>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<A const &> &&>())), A const &>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<A const &> const &&>())), A const &>);

struct B { int v; };
static_assert(std::is_same_v<decltype(get<1>(std::declval<not_tuple<A, B> const>())), B const &&>);
static_assert(std::is_same_v<decltype(get<1>(std::declval<not_tuple<A, B &> const &&>())), B const &>);
static_assert(std::is_same_v<decltype(get<1>(std::declval<not_tuple<A, B> const &>())), B const &>);
static_assert(std::is_same_v<decltype(get<1>(std::declval<not_tuple<A, B const> &>())), B const &>);
// clang-format on
} // namespace fn::detail

TEST_CASE("detail::not_tuple", "[not_tuple]")
{
  using namespace fn::detail;
  WHEN("holding values")
  {
    using T = not_tuple<A, B const>;
    T t1 = {{1}, {2}};
    CHECK(get<0>(t1).v == 1);
    CHECK(get<1>(t1).v == 2);

    static_assert(std::is_same_v<decltype(get<0>(t1)), A &>);
    static_assert(std::is_same_v<decltype(get<1>(t1)), B const &>);
    static_assert(
        std::is_same_v<decltype(get<0>(std::as_const(t1))), A const &>);
    static_assert(
        std::is_same_v<decltype(get<1>(std::as_const(t1))), B const &>);
    static_assert(std::is_same_v<decltype(get<0>(std::move(t1))), A &&>);
    static_assert(std::is_same_v<decltype(get<1>(std::move(t1))), B const &&>);
    static_assert(std::is_same_v< //
                  decltype(get<0>(std::move(std::as_const(t1)))), A const &&>);
    static_assert(std::is_same_v< //
                  decltype(get<1>(std::move(std::as_const(t1)))), B const &&>);
    static_assert(std::is_same_v<decltype(get<0>(T{{}, {}})), A &&>);
    static_assert(std::is_same_v<decltype(get<1>(T{{}, {}})), B const &&>);
  }

  WHEN("holding references")
  {
    A a{3};
    B const b{4};
    using T = not_tuple<A &, B const &>;
    T t1 = {a, b};
    CHECK(get<0>(t1).v == 3);
    CHECK(get<1>(t1).v == 4);

    static_assert(std::is_same_v<decltype(get<0>(t1)), A &>);
    static_assert(std::is_same_v<decltype(get<1>(t1)), B const &>);
    static_assert(
        std::is_same_v<decltype(get<0>(std::as_const(t1))), A const &>);
    static_assert(
        std::is_same_v<decltype(get<1>(std::as_const(t1))), B const &>);
    static_assert(std::is_same_v<decltype(get<0>(std::move(t1))), A &>);
    static_assert(std::is_same_v<decltype(get<1>(std::move(t1))), B const &>);
    static_assert(std::is_same_v< //
                  decltype(get<0>(std::move(std::as_const(t1)))), A const &>);
    static_assert(std::is_same_v< //
                  decltype(get<1>(std::move(std::as_const(t1)))), B const &>);
    static_assert(std::is_same_v<decltype(get<0>(T{{a}, {b}})), A &>);
    static_assert(std::is_same_v<decltype(get<1>(T{{a}, {b}})), B const &>);
  }
}
