// Copyright (c) 2025 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "catch2/catch_test_macros.hpp"

#ifndef PFN_TEST_NESTED

#include <pfn/optional.hpp>

using pfn::make_optional;
using pfn::optional;

#endif
// When nested via PFN_TEST_NESTED (e.g expected_validation.cpp), the wrapper TU
// already includes the necessary header(s) and brings the relevant aliases into the
// global namespace to select right set of types expected as the subject under test.

#include <util/helper_types.hpp>

#include <catch2/catch_all.hpp>

#include <initializer_list>
#include <stdexcept>
#include <type_traits>
#include <utility>

TEST_CASE("optional", "[optional][polyfill]")
{
#ifndef PFN_TEST_VALIDATION
  constexpr bool extension = true;
#else
  constexpr bool extension = false;
#endif

  SECTION("type aliases")
  {
    static_assert(std::is_same_v<optional<int>::value_type, int>);
    static_assert(std::is_same_v<optional<helper>::value_type, helper>);
    SUCCEED();
  }

  SECTION("constructors")
  {
    SECTION("default and nullopt")
    {
      // optional<T> is default constructible (disengaged) even when T is not.
      static_assert(not std::is_default_constructible_v<helper>); // prerequisite
      static_assert(std::is_default_constructible_v<optional<helper>>);
      static_assert(std::is_nothrow_default_constructible_v<optional<helper>>);
      static_assert(std::is_nothrow_constructible_v<optional<helper>, std::nullopt_t>);

      using T = optional<int>;
      static_assert(std::is_nothrow_default_constructible_v<T>);
      static_assert(std::is_trivially_destructible_v<T>);

      constexpr T a;
      constexpr T b(std::nullopt);
      (void)a;
      (void)b;
      SUCCEED();
    }

    SECTION("in_place")
    {
      using T = optional<helper>;
      static_assert(std::is_constructible_v<T, std::in_place_t, int>);
      static_assert(not std::is_constructible_v<T, std::in_place_t>); // helper has no default ctor
      static_assert(not extension || not std::is_nothrow_constructible_v<T, std::in_place_t, int>);
      static_assert(not extension || std::is_nothrow_constructible_v<optional<helper_t<8>>, std::in_place_t, int>);

      // value ctor path is witnessed by helper_t::state
      int const s0 = helper::state;
      T a(std::in_place, 11);
      CHECK(helper::state - s0 == 11);

      // throwing value ctor propagates
      REQUIRE_THROWS_AS(T(std::in_place, 0), std::runtime_error);
    }

    SECTION("in_place initializer_list")
    {
      using T = optional<helper>;
      static_assert(std::is_constructible_v<T, std::in_place_t, helper_list_t>);
      static_assert(std::is_constructible_v<T, std::in_place_t, helper_list_t, int>);

      int const s0 = helper::state;
      T a(std::in_place, {1.0, 2.0, 3.0}); // helper(list): state += 1*2*3
      CHECK(helper::state - s0 == 6);
    }
  }

  SECTION("copy, move and dtor")
  {
    SECTION("unavailable")
    {
      static_assert(not std::is_copy_constructible_v<optional<helper_move_only>>);
      static_assert(std::is_move_constructible_v<optional<helper_move_only>>);

      // An immovable value type cannot be copied or moved, but the optional can
      // still be built in place.
      static_assert(not std::is_copy_constructible_v<optional<helper_immovable>>);
      static_assert(not std::is_move_constructible_v<optional<helper_immovable>>);
      optional<helper_immovable> a(std::in_place, 6, 7);
      (void)a;
      SUCCEED();
    }

    SECTION("move-only value type")
    {
      using T = optional<helper_move_only>;
      static_assert(not std::is_copy_constructible_v<T>);
      static_assert(std::is_move_constructible_v<T>);
      static_assert(not std::is_trivially_move_constructible_v<T>);
      static_assert(not extension || std::is_nothrow_move_constructible_v<T>);

      T a(std::in_place, 7);
      T b = std::move(a);
      (void)b;
      SUCCEED();
    }

    SECTION("trivial")
    {
      using T = optional<int>;
      static_assert(std::is_copy_constructible_v<T>);
      static_assert(std::is_trivially_copy_constructible_v<T>);
      static_assert(not extension || std::is_nothrow_copy_constructible_v<T>);
      static_assert(std::is_move_constructible_v<T>);
      static_assert(std::is_trivially_move_constructible_v<T>);
      static_assert(std::is_trivially_destructible_v<T>);
      static_assert(std::is_nothrow_destructible_v<T>);

      constexpr T a(std::in_place, 13);
      constexpr T b = a;
      constexpr T c = std::move(a);
      (void)b;
      (void)c;
      SUCCEED();
    }

    SECTION("non-trivial value type")
    {
      using T = optional<helper>;
      static_assert(std::is_copy_constructible_v<T>);
      static_assert(not std::is_trivially_copy_constructible_v<T>);
      static_assert(not extension || std::is_nothrow_copy_constructible_v<T>);
      static_assert(std::is_move_constructible_v<T>);
      static_assert(not std::is_trivially_move_constructible_v<T>);
      static_assert(std::is_nothrow_move_constructible_v<T>);
      static_assert(not std::is_trivially_destructible_v<T>);
      static_assert(std::is_nothrow_destructible_v<T>);

      // copy/move of the disengaged state invokes no value ctor
      T a(std::nullopt);
      T b = a;
      T c = std::move(a);
      (void)b;
      (void)c;

      // move of the engaged state is witnessed via helper_t<30>::state
      using H = helper_t<30>;
      static_assert(not std::is_trivially_move_constructible_v<optional<H>>);
      int const s0 = H::state;
      optional<H> d(std::in_place, 7); // value ctor: state += 7
      optional<H> e = std::move(d);    // move ctor (V>=30): state += 7*from_rval
      CHECK(H::state - s0 == 7 + 7 * from_rval);
    }

    SECTION("noexcept(false) from value type")
    {
      using T = optional<helper_t<2>>; // throwing copy ctor, nothrow move ctor
      static_assert(std::is_copy_constructible_v<T>);
      static_assert(not std::is_trivially_copy_constructible_v<T>);
      static_assert(not std::is_nothrow_copy_constructible_v<T>);
      static_assert(std::is_move_constructible_v<T>);
      static_assert(std::is_nothrow_move_constructible_v<T>);
      SUCCEED();
    }
  }
}

TEST_CASE("optional reference", "[optional_ref][polyfill]")
{
  SECTION("type aliases")
  {
    static_assert(std::is_same_v<optional<int &>::value_type, int>);
    SUCCEED();
  }

  SECTION("trivial")
  {
    using T = optional<int &>;
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(std::is_trivially_destructible_v<T>);
    static_assert(std::is_nothrow_default_constructible_v<T>);
    static_assert(std::is_copy_constructible_v<T>);
    static_assert(std::is_trivially_copy_constructible_v<T>);
    static_assert(std::is_nothrow_constructible_v<T, std::nullopt_t>);
    SUCCEED();
  }

  SECTION("constructors")
  {
    using T = optional<int &>;
    constexpr T a;
    constexpr T b(std::nullopt);
    (void)a;
    (void)b;

    int x = 5;
    T c(std::in_place, x); // binds the reference to x
    T d = c;               // trivial copy
    (void)c;
    (void)d;
    SUCCEED();
  }
}
