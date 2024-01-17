// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/utility.hpp"

#include <catch2/catch_all.hpp>

#include <optional>

namespace fn {
// clang-format off
static_assert(std::is_same_v<as_value_t<int>,          int>);
static_assert(std::is_same_v<as_value_t<int const>,    int const>);
static_assert(std::is_same_v<as_value_t<int &&>,       int>);
static_assert(std::is_same_v<as_value_t<int const &&>, int const>);
static_assert(std::is_same_v<as_value_t<int &>,        int &>);
static_assert(std::is_same_v<as_value_t<int const &>,  int const &>);

static_assert(std::is_same_v<as_value_t<std::nullopt_t>,          std::nullopt_t>);
static_assert(std::is_same_v<as_value_t<std::nullopt_t const>,    std::nullopt_t const>);
static_assert(std::is_same_v<as_value_t<std::nullopt_t &>,        std::nullopt_t>);
static_assert(std::is_same_v<as_value_t<std::nullopt_t const &>,  std::nullopt_t const>);
static_assert(std::is_same_v<as_value_t<std::nullopt_t &&>,       std::nullopt_t>);
static_assert(std::is_same_v<as_value_t<std::nullopt_t const &&>, std::nullopt_t const>);

static_assert(std::is_same_v<apply_const_t<float,          int>,    int>);
static_assert(std::is_same_v<apply_const_t<float const,    int>,    int const>);
static_assert(std::is_same_v<apply_const_t<float,          int &>,  int &>);
static_assert(std::is_same_v<apply_const_t<float const,    int &>,  int const &>);
static_assert(std::is_same_v<apply_const_t<float,          int &&>, int &&>);
static_assert(std::is_same_v<apply_const_t<float const,    int &&>, int const &&>);

static_assert(std::is_same_v<apply_const_t<float &,        int>,    int>);
static_assert(std::is_same_v<apply_const_t<float const &,  int>,    int const>);
static_assert(std::is_same_v<apply_const_t<float &,        int &>,  int &>);
static_assert(std::is_same_v<apply_const_t<float const &,  int &>,  int const &>);
static_assert(std::is_same_v<apply_const_t<float &,        int &&>, int &&>);
static_assert(std::is_same_v<apply_const_t<float const &,  int &&>, int const &&>);

static_assert(std::is_same_v<apply_const_t<float &&,       int>,    int>);
static_assert(std::is_same_v<apply_const_t<float const &&, int>,    int const>);
static_assert(std::is_same_v<apply_const_t<float &&,       int &>,  int &>);
static_assert(std::is_same_v<apply_const_t<float const &&, int &>,  int const &>);
static_assert(std::is_same_v<apply_const_t<float &&,       int &&>, int &&>);
static_assert(std::is_same_v<apply_const_t<float const &&, int &&>, int const &&>);

static_assert(std::is_same_v<decltype(apply_const<float>         (std::declval<int>())),    int &&>);
static_assert(std::is_same_v<decltype(apply_const<float const>   (std::declval<int>())),    int const &&>);
static_assert(std::is_same_v<decltype(apply_const<float>         (std::declval<int &>())),  int &>);
static_assert(std::is_same_v<decltype(apply_const<float const>   (std::declval<int &>())),  int const &>);
static_assert(std::is_same_v<decltype(apply_const<float>         (std::declval<int &&>())), int &&>);
static_assert(std::is_same_v<decltype(apply_const<float const>   (std::declval<int &&>())), int const &&>);
static_assert(std::is_same_v<decltype(apply_const<float &>       (std::declval<int>())),    int &&>);
static_assert(std::is_same_v<decltype(apply_const<float const &> (std::declval<int>())),    int const &&>);
static_assert(std::is_same_v<decltype(apply_const<float &>       (std::declval<int &>())),  int &>);
static_assert(std::is_same_v<decltype(apply_const<float const &> (std::declval<int &>())),  int const &>);
static_assert(std::is_same_v<decltype(apply_const<float &>       (std::declval<int &&>())), int &&>);
static_assert(std::is_same_v<decltype(apply_const<float const &> (std::declval<int &&>())), int const &&>);
static_assert(std::is_same_v<decltype(apply_const<float &&>      (std::declval<int>())),    int &&>);
static_assert(std::is_same_v<decltype(apply_const<float const &&>(std::declval<int>())),    int const &&>);
static_assert(std::is_same_v<decltype(apply_const<float &&>      (std::declval<int &>())),  int &>);
static_assert(std::is_same_v<decltype(apply_const<float const &&>(std::declval<int &>())),  int const &>);
static_assert(std::is_same_v<decltype(apply_const<float &&>      (std::declval<int &&>())), int &&>);
static_assert(std::is_same_v<decltype(apply_const<float const &&>(std::declval<int &&>())), int const &&>);
// clang-format on
} // namespace fn

extern int value;
extern int const const_value;
static_assert(                //
    std::is_same_v<           //
        decltype(FWD(value)), //
        decltype(std::forward<decltype(value)>(value))>);
static_assert(      //
    std::is_same_v< //
        decltype(FWD(const_value)),
        decltype(std::forward<decltype(const_value)>(const_value))>);

extern int &lvalue;
extern int const &const_lvalue;
static_assert(      //
    std::is_same_v< //
        decltype(FWD(lvalue)),
        decltype(std::forward<decltype(lvalue)>(lvalue))>);
static_assert(      //
    std::is_same_v< //
        decltype(FWD(const_lvalue)),
        decltype(std::forward<decltype(const_lvalue)>(const_lvalue))>);

extern int &&rvalue;
extern int const &&const_rvalue;
static_assert(      //
    std::is_same_v< //
        decltype(FWD(rvalue)),
        decltype(std::forward<decltype(rvalue)>(rvalue))>);
static_assert(      //
    std::is_same_v< //
        decltype(FWD(const_rvalue)),
        decltype(std::forward<decltype(const_rvalue)>(const_rvalue))>);

TEST_CASE("detail::apply_const", "[apply_const]")
{
  struct A final {};
  using namespace fn;
  CHECK(std::is_same_v<decltype(apply_const<int>(A{})), A &&>);
  CHECK(std::is_same_v<decltype(apply_const<int const>(A{})), A const &&>);

  A a = {};
  // NOTE std::move below do *not* leave us with moved-from objects
  CHECK(std::is_same_v< //
        decltype(apply_const<int>(std::move(a))), A &&>);
  CHECK(std::is_same_v< //
        decltype(apply_const<int const>(std::move(a))), A const &&>);
  CHECK(std::is_same_v< //
        decltype(apply_const<int>(a)), A &>);
  CHECK(std::is_same_v< //
        decltype(apply_const<int const>(a)), A const &>);

  CHECK(std::is_same_v< //
        decltype(apply_const<int>(std::move(std::as_const(a)))), A const &&>);
  CHECK(std::is_same_v< //
        decltype(apply_const<int const>(std::move(std::as_const(a)))),
        A const &&>);
  CHECK(std::is_same_v< //
        decltype(apply_const<int>(std::as_const(a))), A const &>);
  CHECK(std::is_same_v< //
        decltype(apply_const<int const>(std::as_const(a))), A const &>);
}
