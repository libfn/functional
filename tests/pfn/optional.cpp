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
// When nested via PFN_TEST_NESTED (e.g optional_validation.cpp), the wrapper TU
// already includes the necessary header(s) and brings the relevant aliases into the
// global namespace to select right set of types expected as the subject under test.

#include <util/helper_types.hpp>

#include <catch2/catch_all.hpp>

#include <cstring>
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
      static_assert(std::is_nothrow_move_constructible_v<T>);

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

  SECTION("assignment")
  {
    // helper_t<V> fixtures used below (see helper_types.hpp for the full nothrow table)
    using M = helper_t<2>;  // nothrow move constructible, throwing copy constructible
    using E = helper_t<3>;  // may throw on move and copy
    using C = helper_t<4>;  // nothrow copy constructible, throwing move constructible
    using H = helper_t<40>; // nothrow copy/move constructible; throwing copy/move assignable
    static_assert(not std::is_nothrow_copy_constructible_v<M>);
    static_assert(std::is_nothrow_move_constructible_v<M>);
    static_assert(not std::is_nothrow_copy_constructible_v<E>);
    static_assert(not std::is_nothrow_move_constructible_v<E>);
    static_assert(std::is_nothrow_copy_constructible_v<C>);
    static_assert(not std::is_nothrow_move_constructible_v<C>);
    static_assert(std::is_nothrow_copy_constructible_v<H>);
    static_assert(std::is_nothrow_move_constructible_v<H>);
    static_assert(not std::is_nothrow_copy_assignable_v<H>);
    static_assert(not std::is_nothrow_move_assignable_v<H>);

    SECTION("nullopt_t")
    {
      optional<helper> a(std::in_place, 21);
      CHECK(a.has_value());
      a = std::nullopt;
      CHECK(not a.has_value());
      a = std::nullopt; // already disengaged: no-op path
      CHECK(not a.has_value());
    }

    SECTION("from rval")
    {
      SECTION("engaged to engaged")
      {
        using T = optional<helper>;
        static_assert(std::is_nothrow_assignable_v<T &, T &&>); // standard-mandated, not an extension

        T a(std::in_place, 17);
        a = T(std::in_place, 19);
        CHECK(a->v == 19 * from_rval);

        { // the move-assignment operator propagates a throwing T::operator=
          using T = optional<H>;
          static_assert(not std::is_nothrow_assignable_v<T &, T &&>);

          T b(std::in_place, 11);
          b = T(std::in_place, 13);
          CHECK(b->v == 13 * from_rval);

          try {
            b = T(std::in_place, 0);
            FAIL();
          } catch (std::runtime_error const &e) {
            CHECK(std::strcmp(e.what(), "invalid input") == 0);
            CHECK(b.has_value());
            CHECK(b->v == 13 * from_rval); // unchanged: H::operator= throws before mutating
          }
        }
      }

      SECTION("engaged to disengaged")
      {
        using T = optional<helper>;
        T a(std::in_place, 13);
        a = T(std::nullopt);
        CHECK(not a.has_value());
      }

      SECTION("disengaged to engaged")
      {
        SECTION("nothrow move")
        {
          using T = optional<M>;
          static_assert(std::is_nothrow_assignable_v<T &, T &&>);

          T a(std::nullopt);
          a = T(std::in_place, 11);
          CHECK(a->v == 11 * from_rval);
        }

        SECTION("throwing")
        {
          using T = optional<E>;
          static_assert(not std::is_nothrow_assignable_v<T &, T &&>);

          T a(std::nullopt);
          a = T(std::in_place, 5);
          CHECK(a->v == 5 * from_rval);

          T b(std::nullopt);
          try {
            // constructed via the initializer_list ctor (no V<8 throw-check there) to
            // get a stored 0 without the value ctor itself throwing first.
            b = T(std::in_place, {0.0});
            FAIL();
          } catch (std::runtime_error const &e) {
            CHECK(std::strcmp(e.what(), "invalid input") == 0);
            CHECK(not b.has_value());
          }
        }
      }

      SECTION("disengaged to disengaged")
      {
        using T = optional<helper>;
        T a(std::nullopt);
        a = T(std::nullopt);
        CHECK(not a.has_value());
      }

      SECTION("constexpr")
      {
        using T = optional<int>;
        constexpr auto fn = [](T &&v) constexpr -> T {
          T tmp{std::in_place, 1};
          tmp = std::move(v);
          return tmp;
        };

        constexpr T a = fn(T(std::in_place, 7));
        static_assert(a.has_value() && *a == 7);

        constexpr T b = fn(T(std::nullopt));
        static_assert(not b.has_value());

        SUCCEED();
      }
    }

    SECTION("from lval const")
    {
      SECTION("engaged to engaged")
      {
        using T = optional<helper>;
        static_assert(not extension || std::is_nothrow_assignable_v<T &, T const &>);

        T a(std::in_place, 3);
        T const b(std::in_place, 5);
        a = b;
        CHECK(a->v == 5 * from_lval_const);
      }

      SECTION("engaged to disengaged")
      {
        using T = optional<helper>;
        T a(std::in_place, 9);
        T const b(std::nullopt);
        a = b;
        CHECK(not a.has_value());
      }

      SECTION("disengaged to engaged")
      {
        SECTION("nothrow copy")
        {
          using T = optional<C>;
          static_assert(not extension || std::is_nothrow_assignable_v<T &, T const &>);

          T a(std::nullopt);
          T const b(std::in_place, 7);
          a = b;
          CHECK(a->v == 7 * from_lval_const);
        }

        SECTION("throwing")
        {
          using T = optional<E>;
          static_assert(not std::is_nothrow_assignable_v<T &, T const &>);

          T a(std::nullopt);
          T const b(std::in_place, 7);
          a = b;
          CHECK(a->v == 7 * from_lval_const);

          T c(std::nullopt);
          try {
            T const d(std::in_place, {0.0});
            c = d;
            FAIL();
          } catch (std::runtime_error const &e) {
            CHECK(std::strcmp(e.what(), "invalid input") == 0);
            CHECK(not c.has_value());
          }
        }
      }

      SECTION("disengaged to disengaged")
      {
        using T = optional<helper>;
        T a(std::nullopt);
        T const b(std::nullopt);
        a = b;
        CHECK(not a.has_value());
      }

      SECTION("constexpr")
      {
        using T = optional<int>;
        constexpr auto fn = [](T const &v) constexpr -> T {
          T tmp{std::in_place, 1};
          tmp = v;
          return tmp;
        };

        constexpr T a = fn(T(std::in_place, 7));
        static_assert(a.has_value() && *a == 7);

        constexpr T b = fn(T(std::nullopt));
        static_assert(not b.has_value());

        SUCCEED();
      }
    }

    SECTION("unavailable")
    {
      // Copy-assignment is deleted, so only rval (move) assignment is available.
      static_assert(not std::is_copy_assignable_v<optional<helper_move_only>>);
      static_assert(std::is_move_assignable_v<optional<helper_move_only>>);
      static_assert(std::is_nothrow_move_assignable_v<optional<helper_move_only>>);

      static_assert(not std::is_copy_assignable_v<optional<helper_immovable>>);
      static_assert(not std::is_move_assignable_v<optional<helper_immovable>>);
      SUCCEED();
    }
  }

  SECTION("emplace")
  {
    using T = optional<helper>;

    SECTION("engaged to engaged")
    {
      T a(std::in_place, 1);
      a.emplace(2, 3, 5);
      CHECK(a->v == 2 * 3 * 5);
    }

    SECTION("disengaged to engaged")
    {
      T a(std::nullopt);
      a.emplace(2, 3, 5);
      CHECK(a->v == 2 * 3 * 5);
    }

    SECTION("initializer_list")
    {
      T a(std::nullopt);
      a.emplace({7.0, 11.0});
      CHECK(a->v == 7 * 11);
    }

    SECTION("move-only value type")
    {
      // emplace constructs in place, so it needs neither copy nor move of the value.
      optional<helper_move_only> a(std::nullopt);
      a.emplace(7);
      CHECK(a->v == 7);
    }

    SECTION("immovable value type")
    {
      optional<helper_immovable> a(std::nullopt);
      a.emplace(6, 7);
      CHECK(a->v == 6 * 7);
    }

    SECTION("throwing constructor leaves the optional disengaged")
    {
      T a(std::in_place, 1);
      try {
        a.emplace(0);
        FAIL();
      } catch (std::runtime_error const &e) {
        CHECK(std::strcmp(e.what(), "invalid input") == 0);
        CHECK(not a.has_value());
      }
    }

    SECTION("no noexcept-specifier: unlike expected's, still callable for a throwing ctor")
    {
      // [optional.assign] leaves emplace's noexcept-specifier unspecified (unlike expected's
      // emplace, which pfn constrains to nothrow construction); libstdc++ and libc++ actually
      // disagree here (libstdc++ conditions it on is_nothrow_constructible_v, libc++ doesn't),
      // so only the pfn-specific choice (no noexcept-specifier at all) is asserted, and only
      // the direction that holds regardless of that choice -- a throwing ctor is never
      // noexcept -- is asserted unconditionally.
      static_assert(not std::is_nothrow_constructible_v<helper, int>);
      static_assert(not noexcept(std::declval<T &>().emplace(1)));

      static_assert(std::is_nothrow_constructible_v<helper_t<8>, int>);
      static_assert(not extension || not noexcept(std::declval<optional<helper_t<8>> &>().emplace(1)));
      SUCCEED();
    }

    SECTION("constexpr")
    {
      constexpr helper c{helper_list_t(), 5};

      SECTION("from disengaged")
      {
        constexpr auto fn = [](auto &&...args) constexpr -> T {
          T tmp{std::nullopt};
          tmp.emplace(std::forward<decltype(args)>(args)...);
          return tmp;
        };

        constexpr T a = fn(c);
        static_assert(a->v == 5 * from_lval_const * from_rval);

        constexpr T b = fn(helper_list_t{3.0, 11.0}, 7);
        static_assert(b->v == 3 * 11 * 7 * from_rval);

        SUCCEED();
      }

      SECTION("from engaged")
      {
        constexpr auto fn = [](auto &&...args) constexpr -> T {
          T tmp{std::in_place, helper_list_t(), 13};
          tmp.emplace(std::forward<decltype(args)>(args)...);
          return tmp;
        };

        constexpr T a = fn(c);
        static_assert(a->v == 5 * from_lval_const * from_rval);

        constexpr T b = fn(helper_list_t{3.0, 11.0}, 7);
        static_assert(b->v == 3 * 11 * 7 * from_rval);

        SUCCEED();
      }
    }
  }
}

// No released standard library implements C++26's optional<T&> ([optional.optional.ref])
// yet, so this section is pfn-only and skipped when nested into optional_validation.cpp.
#ifndef PFN_TEST_VALIDATION
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
#endif
