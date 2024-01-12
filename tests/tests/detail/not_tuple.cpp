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
}(not_tuple<1, Foo>{}));

static_assert([](auto &&v) constexpr -> bool {
  return requires { get<0>(v); }    //
         && requires { get<1>(v); } //
         && !requires { get<2>(v); };
  ;
}(not_tuple<2, Foo, Foo>{}));

static_assert([](auto &&v) constexpr -> bool {
  return requires { get<0>(v); }    //
         && requires { get<1>(v); } //
         && requires { get<2>(v); } //
         && !requires { get<3>(v); };
  ;
}(not_tuple<3, Foo, Foo, Foo>{}));

static_assert([](auto &&v) constexpr -> bool {
  return requires { get<0>(v); }    //
         && requires { get<1>(v); } //
         && requires { get<2>(v); } //
         && requires { get<3>(v); } //
         && !requires { get<4>(v); };
  ;
}(not_tuple<4, Foo, Foo, Foo, Foo>{}));

// clang-format off
struct A { int v; };
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<1, A>>())), A &&>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<1, A> const>())), A const &&>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<1, A> &>())), A &>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<1, A> const &>())), A const &>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<1, A> &&>())), A &&>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<1, A> const &&>())), A const &&>);

static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<1, A &>>())), A &>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<1, A &> const>())), A const &>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<1, A &> &>())), A &>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<1, A &> const &>())), A const &>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<1, A &> &&>())), A &>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<1, A &> const &&>())), A const &>);

static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<1, A const &>>())), A const &>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<1, A const &> const>())), A const &>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<1, A const &> &>())), A const &>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<1, A const &> const &>())), A const &>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<1, A const &> &&>())), A const &>);
static_assert(std::is_same_v<decltype(get<0>(std::declval<not_tuple<1, A const &> const &&>())), A const &>);

struct B { int v; };
static_assert(std::is_same_v<decltype(get<1>(std::declval<not_tuple<2, A, B> const>())), B const &&>);
static_assert(std::is_same_v<decltype(get<1>(std::declval<not_tuple<2, A, B &> const &&>())), B const &>);
static_assert(std::is_same_v<decltype(get<1>(std::declval<not_tuple<2, A, B> const &>())), B const &>);
static_assert(std::is_same_v<decltype(get<1>(std::declval<not_tuple<2, A, B const> &>())), B const &>);

struct C { int v; };
static_assert(std::is_same_v<decltype(get<2>(std::declval<not_tuple<3, A, B, C> const>())), C const &&>);
static_assert(std::is_same_v<decltype(get<2>(std::declval<not_tuple<3, A, B, C &> const &&>())), C const &>);
static_assert(std::is_same_v<decltype(get<2>(std::declval<not_tuple<3, A, B, C> const &>())), C const &>);
static_assert(std::is_same_v<decltype(get<2>(std::declval<not_tuple<3, A, B, C const> &>())), C const &>);

struct D { int v; };
static_assert(std::is_same_v<decltype(get<3>(std::declval<not_tuple<4, A, B, C, D> const>())), D const &&>);
static_assert(std::is_same_v<decltype(get<3>(std::declval<not_tuple<4, A, B, C, D &> const &&>())), D const &>);
static_assert(std::is_same_v<decltype(get<3>(std::declval<not_tuple<4, A, B, C, D> const &>())), D const &>);
static_assert(std::is_same_v<decltype(get<3>(std::declval<not_tuple<4, A, B, C, D const> &>())), D const &>);
// clang-format on
} // namespace fn::detail

TEST_CASE("detail::not_tuple", "[not_tuple]")
{
  using namespace fn::detail;
  C c{3};
  D d{4};
  using T1 = not_tuple<4, A, B const, C &, D const &>;
  T1 t1 = {{1}, {2}, c, d};
  CHECK(get<0>(t1).v == 1);
  CHECK(get<1>(t1).v == 2);
  CHECK(get<2>(t1).v == 3);
  CHECK(get<3>(t1).v == 4);

  CHECK(std::is_same_v<decltype(get<0>(t1)), A &>);
  CHECK(std::is_same_v<decltype(get<1>(t1)), B const &>);
  CHECK(std::is_same_v<decltype(get<2>(t1)), C &>);
  CHECK(std::is_same_v<decltype(get<3>(t1)), D const &>);

  // NOTE std::move below do *not* leave us with moved-from objects
  CHECK(std::is_same_v<decltype(get<0>(std::move(t1))), A &&>);
  CHECK(std::is_same_v<decltype(get<1>(std::move(t1))), B const &&>);
  CHECK(std::is_same_v<decltype(get<2>(std::move(t1))), C &>);
  CHECK(std::is_same_v<decltype(get<3>(std::move(t1))), D const &>);

  CHECK(std::is_same_v< //
        decltype(get<0>(std::move(std::as_const(t1)))), A const &&>);
  CHECK(std::is_same_v< //
        decltype(get<1>(std::move(std::as_const(t1)))), B const &&>);
  CHECK(std::is_same_v< //
        decltype(get<2>(std::move(std::as_const(t1)))), C const &>);
  CHECK(std::is_same_v< //
        decltype(get<3>(std::move(std::as_const(t1)))), D const &>);

  CHECK(std::is_same_v<decltype(get<0>(T1{{}, {}, {c}, {d}})), A &&>);
  CHECK(std::is_same_v<decltype(get<1>(T1{{}, {}, {c}, {d}})), B const &&>);
  CHECK(std::is_same_v<decltype(get<2>(T1{{}, {}, {c}, {d}})), C &>);
  CHECK(std::is_same_v<decltype(get<3>(T1{{}, {}, {c}, {d}})), D const &>);
}
