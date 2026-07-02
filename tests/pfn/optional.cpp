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
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

// Constructible from literally anything, but itself neither copyable nor movable -- so
// optional<greedy_t> can only be "constructible from an optional<greedy_t>&/&&/etc." via a
// hijacking converting ctor, never via the (unavailable) dedicated copy/move ctor. Mirrors
// pfn/expected.cpp's own greedy_t, used the same way for the same anti-hijack purpose.
struct greedy_t {
  greedy_t() = delete;
  greedy_t(greedy_t const &) = delete;
  greedy_t(greedy_t &&) = delete;
  template <class U> constexpr greedy_t(U &&) noexcept {}
};

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

    SECTION("converting")
    {
      using T = optional<helper>;
      static_assert(std::is_constructible_v<T, int>);
      static_assert(not std::is_nothrow_constructible_v<T, int>);
      static_assert(std::is_constructible_v<T, helper>);
      static_assert(not extension || std::is_nothrow_constructible_v<T, helper>);

      T const a(11);
      CHECK(a->v == 11);

      T const b(helper(13));
      CHECK(b->v == 13 * from_rval);

      // explicit-ness follows is_convertible_v<U, T>: helper(int) is an implicit ctor, so the
      // optional(U&&) ctor must be implicit too, but an explicit-only value type forces it explicit.
      static_assert(std::is_convertible_v<int, T>);
      struct explicit_only {
        int v;
        constexpr explicit explicit_only(int x) noexcept : v(x) {}
      };
      static_assert(std::is_constructible_v<optional<explicit_only>, int>);
      static_assert(not std::is_convertible_v<int, optional<explicit_only>>);

      // Self-hijack guard: greedy_t is constructible from literally anything but has no copy
      // or move ctor at all, so optional<greedy_t> can only spuriously look "constructible from
      // itself" if optional(U&&) (U = optional<greedy_t> and its ref-qualified variants) were
      // NOT excluded from the overload set.
      using H = optional<greedy_t>;
      static_assert(not std::is_copy_constructible_v<H>);
      static_assert(not std::is_move_constructible_v<H>);
      static_assert(not std::is_constructible_v<H, H &>);
      static_assert(not std::is_constructible_v<H, H const &>);
      static_assert(not std::is_constructible_v<H, H &&>);
      static_assert(not std::is_constructible_v<H, H const &&>);

      // nullopt_t/in_place_t must still select the dedicated ctors, never the converting one,
      // even for a value type that (unlike a normal type) is ALSO directly constructible from
      // both tags -- see the _can_convert exclusion in include/pfn/optional.hpp, an LWG4222-
      // style fix (that issue added the analogous unexpect_t exclusion to expected's converting
      // ctor; optional's own draft constraint list omits the equivalent nullopt_t exclusion).
      static_assert(std::is_constructible_v<greedy_t, std::nullopt_t>);  // prerequisite
      static_assert(std::is_constructible_v<greedy_t, std::in_place_t>); // prerequisite
      H c(std::nullopt);
      CHECK(not c.has_value());
      H d(std::in_place, 1);
      CHECK(d.has_value());
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

  SECTION("from other optional")
  {
    SECTION("rval")
    {
      SECTION("engaged")
      {
        using T = optional<helper>;
        static_assert(std::is_constructible_v<T, optional<int>>);
        static_assert(not std::is_nothrow_constructible_v<T, optional<int>>);
        static_assert(not extension || std::is_nothrow_constructible_v<T, optional<helper_list_t>>);
        static_assert(std::is_convertible_v<optional<int>, T>);

        constexpr optional<int> a{5};
        constexpr optional<double> b{a};
        static_assert(b.has_value() && *b == 5.0);

        T c((optional<int>(5)));
        CHECK(c.has_value());
        CHECK(c->v == 5);

#ifndef PFN_TEST_VALIDATION
        // Also converts from a *reference* optional<U&> (pfn-only: no released standard
        // library implements C++26's optional<T&> yet), exercising the fact that the
        // converting ctor reads its source through the public has_value()/operator*() API
        // rather than its private storage -- optional<int&> has a completely different
        // internal representation (a bare pointer, no union or set_ at all).
        static_assert(std::is_constructible_v<T, optional<int &>>);
        int x = 9;
        T d(optional<int &>(std::in_place, x));
        CHECK(d.has_value());
        CHECK(d->v == 9);
#endif
      }

      SECTION("disengaged")
      {
        using T = optional<helper>;
        constexpr optional<int> a;
        constexpr optional<double> b{a};
        static_assert(not b.has_value());

        T c((optional<int>(std::nullopt)));
        CHECK(not c.has_value());
      }
    }

    SECTION("lval const")
    {
      SECTION("engaged")
      {
        using T = optional<helper>;
        static_assert(std::is_constructible_v<T, optional<int> const &>);
        static_assert(not std::is_nothrow_constructible_v<T, optional<int> const &>);
        static_assert(not extension || std::is_nothrow_constructible_v<T, optional<helper_list_t> const &>);
        static_assert(std::is_convertible_v<optional<int> const &, T>);

        constexpr optional<bool> v{true};
        constexpr optional<int> a{v};
        static_assert(a.has_value() && *a == 1);

        optional<int> const w(11);
        T b(w);
        CHECK(b.has_value());
        CHECK(b->v == 11);
      }

      SECTION("disengaged")
      {
        using T = optional<helper>;
        optional<int> const a(std::nullopt);
        T b(a);
        CHECK(not b.has_value());
      }
    }

    SECTION("move-only and immovable value types")
    {
      // The converting ctor constructs T fresh from U's contained value -- it never
      // copies or moves a T, so it works even when T itself has neither ctor.
      static_assert(std::is_constructible_v<optional<helper_move_only>, optional<int>>);
      optional<int> a(std::in_place, 7);
      optional<helper_move_only> b(std::move(a));
      CHECK(b.has_value());
      CHECK(b->v == 7);

      static_assert(std::is_constructible_v<optional<helper_immovable>, optional<int>>);
      optional<int> c(std::in_place, 6);
      optional<helper_immovable> d(std::move(c));
      CHECK(d.has_value());
      CHECK(d->v == 6);
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

    SECTION("converting value")
    {
      using T = optional<double>;
      static_assert(std::is_assignable_v<T &, int>);
      // operator=(U&&)'s noexcept-specifier is a pfn extension (unspecified by the standard,
      // like the in_place ctor's), so this only holds for pfn itself, not real std::optional.
      static_assert(not extension || std::is_nothrow_assignable_v<T &, int>); // double(int) never throws

      T a;
      a = 5; // disengaged -> engaged, construct
      CHECK(a.has_value());
      CHECK(*a == 5.0);

      a = 7; // engaged -> engaged, plain T assignment
      CHECK(*a == 7.0);

      // Self-exclusion: operator=(U&&) must not compete with operator=(optional const&).
      // If _can_assign's exclusion were broken, this call would be ambiguous (a hard
      // compile error), not silently wrong.
      optional<int> b(std::in_place, 1);
      optional<int> const c(std::in_place, 2);
      b = c;
      CHECK(*b == 2);
    }

    SECTION("from other optional")
    {
      SECTION("rval")
      {
        SECTION("engaged to engaged")
        {
          using T = optional<double>;
          T a(std::in_place, 1.0);
          optional<int> b(std::in_place, 5);
          a = std::move(b); // both engaged: plain T assignment (double::operator=(int))
          CHECK(*a == 5.0);
        }

        SECTION("disengaged to engaged")
        {
          using T = optional<double>;
          T a;
          optional<int> b(std::in_place, 7);
          a = std::move(b); // construct
          CHECK(a.has_value());
          CHECK(*a == 7.0);
        }

        SECTION("engaged to disengaged")
        {
          using T = optional<double>;
          T a(std::in_place, 1.0);
          optional<int> b(std::nullopt);
          a = std::move(b); // destroy
          CHECK(not a.has_value());
        }

        SECTION("disengaged to disengaged")
        {
          using T = optional<double>;
          T a;
          optional<int> b(std::nullopt);
          a = std::move(b);
          CHECK(not a.has_value());
        }
      }

      SECTION("lval const")
      {
        SECTION("engaged to engaged")
        {
          using T = optional<double>;
          T a(std::in_place, 1.0);
          optional<int> const b(std::in_place, 5);
          a = b;
          CHECK(*a == 5.0);
        }

        SECTION("disengaged to engaged")
        {
          using T = optional<double>;
          T a;
          optional<int> const b(std::in_place, 7);
          a = b;
          CHECK(a.has_value());
          CHECK(*a == 7.0);
        }

        SECTION("engaged to disengaged")
        {
          using T = optional<double>;
          T a(std::in_place, 1.0);
          optional<int> const b(std::nullopt);
          a = b;
          CHECK(not a.has_value());
        }
      }

#ifndef PFN_TEST_VALIDATION
      SECTION("reference source")
      {
        // pfn-only (no released standard library implements C++26's optional<T&> yet).
        // Exercises _assign_from reading through has_value()/operator*() rather than
        // private storage -- optional<int&> has a completely different internal
        // representation (a bare pointer, no union or set_ at all).
        using T = optional<double>;
        int x = 9;
        optional<int &> const r(std::in_place, x);
        T a;
        a = r;
        CHECK(a.has_value());
        CHECK(*a == 9.0);
      }
#endif
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

  SECTION("accessors")
  {
    SECTION("value")
    {
      using T = optional<helper>;
      static_assert(not noexcept(std::declval<T &>().value()));
      static_assert(not noexcept(std::declval<T const &>().value()));
      static_assert(not noexcept(std::declval<T &&>().value()));
      static_assert(not noexcept(std::declval<T const &&>().value()));

      {
        T a(std::in_place, 11);
        CHECK(a.value().v == 11);
        CHECK(std::as_const(a).value().v == 11);
        CHECK(std::move(std::as_const(a)).value().v == 11);
        CHECK(std::move(a).value().v == 11);

        static_assert(std::is_same_v<decltype(a.value()), helper &>);
        static_assert(std::is_same_v<decltype(std::as_const(a).value()), helper const &>);
        static_assert(std::is_same_v<decltype(std::move(a).value()), helper &&>);
        static_assert(std::is_same_v<decltype(std::move(std::as_const(a)).value()), helper const &&>);
      }

      {
        T a(std::in_place, 13);
        CHECK(a);
        helper b{1};
        CHECK((b = a.value()).v == 13 * from_lval);
        CHECK((b = std::as_const(a).value()).v == 13 * from_lval_const);
        CHECK((b = std::move(std::as_const(a)).value()).v == 13 * from_rval_const);
        CHECK((b = std::move(a).value()).v == 13 * from_rval);
      }

      {
        // value() returns a reference, so it needs neither copy nor move of the value
        optional<helper_immovable> a(std::in_place, 6, 7);
        CHECK(a.value().v == 6 * 7);
        CHECK(std::as_const(a).value().v == 6 * 7);
        static_assert(std::is_same_v<decltype(std::move(a).value()), helper_immovable &&>);
      }

      SECTION("constexpr")
      {
        static_assert([] {
          T a{std::in_place, helper_list_t{}, 1};
          a.value().v = 13;
          return (                                //
              a.value().v == 13                   //
              && std::as_const(a).value().v == 13 //
              && std::move(a).value().v == 13     //
              && std::move(std::as_const(a)).value().v == 13);
        }());
        SUCCEED();
      }

      SECTION("throwing")
      {
        static_assert(std::is_base_of_v<std::exception, std::bad_optional_access>);

        T a(std::nullopt);
        CHECK(!a);
        REQUIRE_THROWS_AS(a.value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(std::as_const(a).value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(std::move(std::as_const(a)).value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(std::move(a).value(), std::bad_optional_access);

        // the exact type, not std::exception: gcc on macOS fails to match a base-class
        // handler for this exception (thrown by pfn and libstdc++ alike); the base
        // relationship is already static_assert'd above
        try {
          (void)a.value();
          FAIL();
        } catch (std::bad_optional_access const &e) {
          CHECK(e.what() != nullptr);
        }
      }
    }

    SECTION("value_or")
    {
      using T = optional<helper>;
      static_assert(std::is_same_v<decltype(std::declval<T &>().value_or(0)), helper>);
      static_assert(std::is_same_v<decltype(std::declval<T>().value_or(0)), helper>);
      static_assert(not noexcept(std::declval<T>().value_or(std::declval<int>())));
      static_assert(not noexcept(std::declval<T &>().value_or(std::declval<int>())));
      // value_or has no noexcept-specifier ([optional.observe] puts none, and unlike expected's
      // pfn adds no extension), so even nothrow arguments yield noexcept(false) -- asserted only
      // for pfn, since a standard library is free to strengthen its own.
      static_assert(not extension || not noexcept(std::declval<T>().value_or(std::declval<helper_list_t>())));
      static_assert(not extension || not noexcept(std::declval<T &>().value_or(std::declval<helper_list_t>())));

      SECTION("default template parameter")
      {
        // std::optional's value_or gained the `= remove_cv_t<T>` default late: libstdc++ in
        // GCC 15, libc++ in LLVM 22. Older implementations don't have this default, so we
        // can't test it there.
#if !defined(PFN_TEST_VALIDATION) || (defined(_LIBCPP_VERSION) && _LIBCPP_VERSION >= 220000)                           \
    || (defined(__GLIBCXX__) && _GLIBCXX_RELEASE >= 15)
        using D = optional<int>;
        static_assert(requires(D &o) { o.value_or({}); });             // const & overload
        static_assert(requires(D &&o) { std::move(o).value_or({}); }); // && overload
#endif
        SUCCEED();
      }

#ifndef _MSC_VER
      SECTION("engaged")
      {
        T a(std::in_place, 7);
        CHECK(a.value_or(0) == helper(7 * from_lval_const));
        CHECK(std::as_const(a).value_or(0) == helper(7 * from_lval_const));
        CHECK(std::move(std::as_const(a)).value_or(0) == helper(7 * from_lval_const));
        CHECK(std::move(a).value_or(0) == helper(7 * from_rval));
      }

      SECTION("disengaged")
      {
        {
          T a(std::nullopt);
          CHECK(a.value_or(13) == helper(13));
          CHECK(std::move(a).value_or(5) == helper(5));
        }

        {
          T const a(std::nullopt);
          helper b(11);
          CHECK(a.value_or(b) == helper(11 * from_lval));
          CHECK(a.value_or(std::as_const(b)) == helper(11 * from_lval_const));
          CHECK(a.value_or(std::move(std::as_const(b))) == helper(11 * from_rval_const));
          CHECK(a.value_or(std::move(b)) == helper(11 * from_rval));
        }
      }
#endif

      SECTION("conversion")
      {
        optional<double> a(std::in_place, 0.5);
        static_assert(std::is_same_v<decltype(a.value_or(3)), double>);
        CHECK(a.value_or(3) == 0.5);
        a = std::nullopt;
        CHECK(a.value_or(3) == 3.0);
      }

#ifndef _MSC_VER
      SECTION("move-only value type")
      {
        optional<helper_move_only> a(std::in_place, 7);
        CHECK(std::move(a).value_or(3) == helper_move_only(7 * from_rval));

        optional<helper_move_only> b(std::nullopt);
        CHECK(std::move(b).value_or(3) == helper_move_only(3));
      }
#endif

#ifndef _MSC_VER
      SECTION("constexpr")
      {
        constexpr helper c{helper_list_t(), 7};

        SECTION("lval const")
        {
          {
            constexpr T a(std::in_place, {3.0}, 5);
            static_assert(a.value_or(c).v == 3 * 5 * from_lval_const);
          }
          {
            constexpr T a(std::nullopt);
            static_assert(a.value_or(c).v == 7 * from_lval_const);
          }

          SUCCEED();
        }

        SECTION("rval")
        {
          static_assert(T{std::in_place, {3.0}, 5}.value_or(c).v == 3 * 5 * from_rval);
          static_assert(T{std::nullopt}.value_or(c).v == 7 * from_lval_const);
          static_assert(T{std::nullopt}.value_or(helper(helper_list_t{7.0}, 3)).v == 7 * 3 * from_rval);

          SUCCEED();
        }
      }
#endif
    }
  }
}

// No released standard library implements C++26's optional<T&> ([optional.optional.ref])
// yet, so this section is pfn-only and skipped when nested into optional_validation.cpp.
#ifndef PFN_TEST_VALIDATION

// Conversion-operator fixtures for optional<T&>'s convert-then-bind (_convert_ref_init_val)
// paths: T& can also be obtained through a user conversion operator -- implicitly or
// explicitly -- and that conversion (not any constructor of T) is what may throw.
struct ref_holder {
  int v;
  constexpr operator int &() noexcept { return v; }
};
struct explicit_ref_holder {
  int v;
  constexpr explicit operator int &() noexcept { return v; }
};
struct throwing_ref_holder {
  int v;
  constexpr operator int &() noexcept(false) { return v; }
};

TEST_CASE("optional reference", "[optional_ref][polyfill]")
{
  SECTION("type aliases")
  {
    static_assert(std::is_same_v<optional<int &>::value_type, int>);
    static_assert(std::is_same_v<optional<int const &>::value_type, int const>);
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

    SECTION("default and nullopt")
    {
      static_assert(std::is_nothrow_default_constructible_v<T>);
      static_assert(std::is_nothrow_constructible_v<T, std::nullopt_t>);

      constexpr T a;
      static_assert(not a.has_value());
      constexpr T b(std::nullopt);
      static_assert(not b.has_value());
      SUCCEED();
    }

    SECTION("in_place")
    {
      static_assert(std::is_constructible_v<T, std::in_place_t, int &>);
      static_assert(not std::is_constructible_v<T, std::in_place_t, int>);      // int& cannot bind an rvalue
      static_assert(not std::is_constructible_v<T, std::in_place_t, double &>); // nor a different lvalue type
      // the noexcept-specifier is a pfn extension ([optional.ref.ctor] leaves in_place without one)
      static_assert(std::is_nothrow_constructible_v<T, std::in_place_t, int &>);
      static_assert(not std::is_nothrow_constructible_v<T, std::in_place_t, throwing_ref_holder &>);

      int x = 5;
      T c(std::in_place, x); // binds the reference to x
      CHECK(c.has_value());
      CHECK(&*c == &x);

      // converts (through a user conversion operator), then binds
      ref_holder h{7};
      T d(std::in_place, h);
      CHECK(&*d == &h.v);
      CHECK(*d == 7);
    }

    SECTION("copy")
    {
      int x = 5;
      T c(std::in_place, x);
      T d = c; // trivial copy
      CHECK(&*d == &x);

      T const e(std::nullopt);
      T f = e;
      CHECK(not f.has_value());
    }

    SECTION("converting")
    {
      static_assert(std::is_constructible_v<T, int &>);
      static_assert(std::is_convertible_v<int &, T>);
      static_assert(not std::is_constructible_v<T, int>); // int& cannot bind an rvalue
      static_assert(not std::is_constructible_v<T, double &>);
      static_assert(std::is_nothrow_constructible_v<T, int &>); // standard-mandated, not an extension
      static_assert(not std::is_nothrow_constructible_v<T, throwing_ref_holder &>);

      int x = 5;
      T a = x;
      CHECK(&*a == &x);

      ref_holder h{7};
      T b = h;
      CHECK(&*b == &h.v);

      // explicit-ness follows is_convertible_v<U, T&>: an explicit-only conversion operator
      // still constructs, but no longer converts
      static_assert(std::is_constructible_v<T, explicit_ref_holder &>);
      static_assert(not std::is_convertible_v<explicit_ref_holder &, T>);
      explicit_ref_holder e{9};
      T c(e);
      CHECK(&*c == &e.v);
    }

    SECTION("from other optional")
    {
      SECTION("value source")
      {
        using S = optional<int>;
        static_assert(std::is_constructible_v<T, S &>);
        static_assert(std::is_convertible_v<S &, T>);
        static_assert(std::is_nothrow_constructible_v<T, S &>); // standard-mandated
        // int& can bind neither a const nor an rvalue source's contained value, nor one
        // whose contained type would require a conversion (i.e. a temporary)
        static_assert(not std::is_constructible_v<T, S const &>);
        static_assert(not std::is_constructible_v<T, S>);
        static_assert(not std::is_constructible_v<T, S const &&>);
        static_assert(not std::is_constructible_v<T, optional<double> &>);
        static_assert(not std::is_constructible_v<optional<long &>, S &>);

        S s(std::in_place, 5);
        T a = s;
        CHECK(a.has_value());
        CHECK(&*a == &*s); // a observes s's contained value...
        *a = 7;
        CHECK(*s == 7); // ...and writes through to it

        S d(std::nullopt);
        T b = d;
        CHECK(not b.has_value());
      }

      SECTION("const value source")
      {
        using C = optional<int const &>;
        static_assert(std::is_constructible_v<C, optional<int> &>);
        static_assert(std::is_constructible_v<C, optional<int> const &>);
        static_assert(std::is_convertible_v<optional<int> const &, C>);

        optional<int> const s(std::in_place, 5);
        C a = s;
        CHECK(a.has_value());
        CHECK(&*a == &*s);
      }

      SECTION("reference source")
      {
        // a differently-typed reference source: B& binds D& directly (derived-to-base), so
        // this is safe for any source value category -- the referent outlives the source
        struct B {
          int v;
        };
        struct D : B {};

        D d{{5}};
        optional<D &> s(std::in_place, d);
        optional<B &> a = s;
        CHECK(&*a == static_cast<B *>(&d));

        optional<B &> b = std::move(s); // rval source: the referent (d) is unaffected
        CHECK(&*b == static_cast<B *>(&d));

        optional<D &> const cs(std::in_place, d);
        optional<B &> g = std::move(cs); // const rval source, same directly-bound referent
        CHECK(&*g == static_cast<B *>(&d));

        optional<D &> const e(std::nullopt);
        optional<B &> f = e;
        CHECK(not f.has_value());
      }

      SECTION("constexpr")
      {
        static_assert([] {
          optional<int> s(std::in_place, 5);
          optional<int &> a(s);
          *a = 7;
          return &*a == &*s && *s == 7;
        }());
        static_assert([] {
          optional<int> s(std::nullopt);
          optional<int &> a(s);
          return not a.has_value();
        }());
        SUCCEED();
      }
    }
  }

  SECTION("assignment")
  {
    using T = optional<int &>;

    SECTION("nullopt_t")
    {
      int x = 5;
      T a(std::in_place, x);
      CHECK(a.has_value());
      a = std::nullopt;
      CHECK(not a.has_value());
      a = std::nullopt; // already disengaged: no-op path
      CHECK(not a.has_value());
    }

    SECTION("copy rebinds rather than assigning through")
    {
      int x = 5, y = 9;
      T a(std::in_place, x);
      T const b(std::in_place, y);
      a = b;
      CHECK(&*a == &y);
      CHECK(x == 5); // x untouched by the rebind
      y = 11;
      CHECK(*a == 11); // a now observes y, not a copy of its old value
    }

    SECTION("copy from disengaged")
    {
      int x = 5;
      T a(std::in_place, x);
      T const b(std::nullopt);
      a = b;
      CHECK(not a.has_value());
      CHECK(x == 5);
    }

    SECTION("from value rebinds via the converting constructor")
    {
      // [optional.ref.assign] has no operator=(U&&): assigning from a value goes through the
      // implicit converting constructor, then the trivial copy-assignment -- a rebind, never
      // an assign-through
      int x = 5, y = 9;
      T a(std::in_place, x);
      a = y;
      CHECK(&*a == &y);
      CHECK(x == 5);
    }
  }

  SECTION("emplace")
  {
    using T = optional<int &>;

    SECTION("from disengaged")
    {
      int x = 7;
      T a(std::nullopt);
      int &r = a.emplace(x);
      CHECK(a.has_value());
      CHECK(&*a == &x);
      CHECK(&r == &x);
    }

    SECTION("rebinds rather than assigning through")
    {
      int x = 5, y = 9;
      T a(std::in_place, x);
      a.emplace(y);
      CHECK(&*a == &y);
      CHECK(x == 5); // x untouched
    }

    SECTION("converts then binds")
    {
      ref_holder h{7};
      T a(std::nullopt);
      int &r = a.emplace(h);
      CHECK(&*a == &h.v);
      CHECK(&r == &h.v);
    }

    SECTION("SFINAE and noexcept")
    {
      static_assert(std::is_nothrow_constructible_v<int &, int &>);
      static_assert(noexcept(std::declval<T &>().emplace(std::declval<int &>())));
      static_assert(not noexcept(std::declval<T &>().emplace(std::declval<throwing_ref_holder &>())));

      // int&& cannot bind int&: emplace must be SFINAE'd out, not a hard error. Wrapped in a
      // generic lambda so the constraint failure is genuine SFINAE (substitution during the
      // lambda's own instantiation), rather than a hard error from a non-dependent call.
      static_assert(not std::is_constructible_v<int &, int &&>);
      constexpr auto fn = [](auto &&...args) constexpr -> bool {
        return requires(T &o) { o.emplace(std::forward<decltype(args)>(args)...); };
      };
      static_assert(not fn(5));
      SUCCEED();
    }

    SECTION("constexpr")
    {
      static_assert([] {
        int x = 1, y = 2;
        T o(std::in_place, x);
        o.emplace(y);
        return &*o == &y && x == 1;
      }());
      SUCCEED();
    }
  }

  SECTION("observers")
  {
    using T = optional<int &>;

    SECTION("const does not propagate to the referent")
    {
      int x = 5;
      T const a(std::in_place, x);
      static_assert(std::is_same_v<decltype(*a), int &>);             // not int const&
      static_assert(std::is_same_v<decltype(a.operator->()), int *>); // not int const*
      static_assert(noexcept(std::declval<T const &>().operator->()));
      static_assert(noexcept(*std::declval<T const &>()));
      static_assert(noexcept(std::declval<T const &>().has_value()));
      static_assert(noexcept(static_cast<bool>(std::declval<T const &>())));
      *a = 7;
      CHECK(x == 7); // mutated through a const optional<int&>
      CHECK(a.has_value());
      CHECK(static_cast<bool>(a));
    }

    SECTION("disengaged")
    {
      T const a(std::nullopt);
      CHECK(not a.has_value());
      CHECK(not static_cast<bool>(a));
    }

    SECTION("value")
    {
      // one overload only: T& regardless of the optional's constness or value category
      static_assert(std::is_same_v<decltype(std::declval<T &>().value()), int &>);
      static_assert(std::is_same_v<decltype(std::declval<T const &>().value()), int &>);
      static_assert(std::is_same_v<decltype(std::declval<T &&>().value()), int &>);
      static_assert(std::is_same_v<decltype(std::declval<T const &&>().value()), int &>);
      static_assert(not noexcept(std::declval<T const &>().value()));

      int x = 5;
      T const a(std::in_place, x);
      CHECK(&a.value() == &x);
      a.value() = 7;
      CHECK(x == 7); // mutated through a const optional<int&>

      SECTION("constexpr")
      {
        static_assert([] {
          int x = 5;
          T o(std::in_place, x);
          o.value() = 7;
          return x == 7 && &o.value() == &x;
        }());
        SUCCEED();
      }

      SECTION("throwing")
      {
        T b(std::nullopt);
        REQUIRE_THROWS_AS(b.value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(std::as_const(b).value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(std::move(b).value(), std::bad_optional_access);
      }
    }

    SECTION("value_or")
    {
      using O = optional<helper &>;

      // returns remove_cv_t<T> by value, and there is only the one const-qualified overload
      static_assert(std::is_same_v<decltype(std::declval<O &>().value_or(0)), helper>);
      static_assert(std::is_same_v<decltype(std::declval<O const &>().value_or(0)), helper>);
      static_assert(std::is_same_v<decltype(std::declval<optional<int const &> &>().value_or(0)), int>);
      static_assert(not noexcept(std::declval<O const &>().value_or(std::declval<int>())));

      SECTION("default template parameter")
      {
        static_assert(requires(optional<int &> &o) { o.value_or({}); });
        SUCCEED();
      }

#ifndef _MSC_VER
      SECTION("engaged")
      {
        // copies the referent through T&, NOT T const& like optional<T>'s const& overload:
        // the optional's constness does not propagate to the referent...
        helper x(13);
        O const a(std::in_place, x);
        CHECK(a.value_or(0) == helper(13 * from_lval));

        // ...unless the referent type itself is const
        optional<helper const &> const b(std::in_place, x);
        CHECK(b.value_or(0) == helper(13 * from_lval_const));
      }

      SECTION("disengaged")
      {
        O const a(std::nullopt);
        CHECK(a.value_or(11) == helper(11));

        helper b(11);
        CHECK(a.value_or(b) == helper(11 * from_lval));
        CHECK(a.value_or(std::as_const(b)) == helper(11 * from_lval_const));
        CHECK(a.value_or(std::move(std::as_const(b))) == helper(11 * from_rval_const));
        CHECK(a.value_or(std::move(b)) == helper(11 * from_rval));
      }
#endif

      SECTION("conversion")
      {
        optional<double &> a(std::nullopt);
        static_assert(std::is_same_v<decltype(a.value_or(3)), double>);
        CHECK(a.value_or(3) == 3.0);

        double d = 0.5;
        a.emplace(d);
        CHECK(a.value_or(3) == 0.5);
      }

#ifndef _MSC_VER
      SECTION("constexpr")
      {
        static_assert([] {
          helper c{helper_list_t(), 7};
          O o(std::in_place, c);
          return o.value_or(0).v == 7 * from_lval;
        }());
        static_assert([] {
          helper const c{helper_list_t(), 7};
          O o(std::nullopt);
          return o.value_or(c).v == 7 * from_lval_const //
                 && o.value_or(helper(helper_list_t{7.0}, 3)).v == 7 * 3 * from_rval;
        }());
        SUCCEED();
      }
#endif
    }
  }
}
#endif
