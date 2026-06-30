// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/detail/traits.hpp>

#include <catch2/catch_all.hpp>

#include <concepts>

namespace {
struct empty_t {};
struct non_empty_t {
  int v;
};
} // namespace

TEST_CASE("_as_value", "[traits][as_value]")
{
  using fn::detail::_as_value;

  SECTION("scalar")
  {
    static_assert(std::same_as<decltype(_as_value<int>), int>);
    static_assert(std::same_as<decltype(_as_value<int const>), int const>);
    static_assert(std::same_as<decltype(_as_value<int &>), int &>);
    static_assert(std::same_as<decltype(_as_value<int const &>), int const &>);
    static_assert(std::same_as<decltype(_as_value<int &&>), int>);
    static_assert(std::same_as<decltype(_as_value<int const &&>), int const>);
    SUCCEED();
  }

  SECTION("non-empty class")
  {
    static_assert(std::same_as<decltype(_as_value<non_empty_t>), non_empty_t>);
    static_assert(std::same_as<decltype(_as_value<non_empty_t const>), non_empty_t const>);
    static_assert(std::same_as<decltype(_as_value<non_empty_t &>), non_empty_t &>);
    static_assert(std::same_as<decltype(_as_value<non_empty_t const &>), non_empty_t const &>);
    static_assert(std::same_as<decltype(_as_value<non_empty_t &&>), non_empty_t>);
    static_assert(std::same_as<decltype(_as_value<non_empty_t const &&>), non_empty_t const>);
    SUCCEED();
  }

  SECTION("empty class")
  {
    // Empty types: lvalue refs collapse to prvalue (the trait's reason to exist).
    static_assert(std::same_as<decltype(_as_value<empty_t>), empty_t>);
    static_assert(std::same_as<decltype(_as_value<empty_t const>), empty_t const>);
    static_assert(std::same_as<decltype(_as_value<empty_t &>), empty_t>);
    static_assert(std::same_as<decltype(_as_value<empty_t const &>), empty_t const>);
    static_assert(std::same_as<decltype(_as_value<empty_t &&>), empty_t>);
    static_assert(std::same_as<decltype(_as_value<empty_t const &&>), empty_t const>);
    SUCCEED();
  }
}

TEST_CASE("apply_const_lvalue_t", "[traits][apply_const][apply_lvalue][apply_const_lvalue_t]")
{
  using fn::detail::_apply_const;
  using fn::detail::_apply_lvalue;

  SECTION("_apply_const")
  {
    SECTION("non-const T leaves V unchanged")
    {
      static_assert(std::same_as<decltype(_apply_const<int, double>), double>);
      static_assert(std::same_as<decltype(_apply_const<int, double &>), double &>);
      static_assert(std::same_as<decltype(_apply_const<int, double &&>), double &&>);
      static_assert(std::same_as<decltype(_apply_const<int &, double>), double>);
      static_assert(std::same_as<decltype(_apply_const<int &&, double>), double>);
      SUCCEED();
    }

    SECTION("T = U const & propagates const onto V")
    {
      static_assert(std::same_as<decltype(_apply_const<int const &, double>), double const>);
      static_assert(std::same_as<decltype(_apply_const<int const &, double &>), double const &>);
      static_assert(std::same_as<decltype(_apply_const<int const &, double &&>), double const &&>);
      SUCCEED();
    }
  }

  SECTION("_apply_lvalue")
  {
    SECTION("non-lvalue-ref T leaves V unchanged")
    {
      static_assert(std::same_as<decltype(_apply_lvalue<int, double>), double>);
      static_assert(std::same_as<decltype(_apply_lvalue<int, double &>), double &>);
      static_assert(std::same_as<decltype(_apply_lvalue<int, double &&>), double &&>);
      static_assert(std::same_as<decltype(_apply_lvalue<int &&, double>), double>);
      SUCCEED();
    }

    SECTION("T = U & promotes V to V &")
    {
      static_assert(std::same_as<decltype(_apply_lvalue<int &, double>), double &>);
      static_assert(std::same_as<decltype(_apply_lvalue<int &, double &>), double &>);
      static_assert(std::same_as<decltype(_apply_lvalue<int &, double &&>), double &>);
      SUCCEED();
    }
  }

  SECTION("apply_const_lvalue_t composition")
  {
    // Exhaustive matrix lives in tests/fn/utility.cpp; anchor it here so the alias
    // and its composition cannot drift unnoticed in the detail header.
    using fn::apply_const_lvalue_t;
    static_assert(std::same_as<apply_const_lvalue_t<int, double>, double>);
    static_assert(std::same_as<apply_const_lvalue_t<int const, double>, double const>);
    static_assert(std::same_as<apply_const_lvalue_t<int &, double>, double &>);
    static_assert(std::same_as<apply_const_lvalue_t<int const &, double>, double const &>);
    SUCCEED();
  }
}
