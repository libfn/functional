// Copyright (c) 2026 Bronek Kozicki
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
#include <functional>
#include <initializer_list>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <version>

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

namespace {

template <typename T> struct non_swappable {
  friend void swap(T &, T &) = delete;
};

template <typename T> struct swappable {
  friend void swap(T &, T &) noexcept(false) {}
};

template <typename T> struct nothrow_swappable {
  friend void swap(T &, T &) noexcept(true) {}
};

// NOTE: not the same as std::is_swappable https://eel.is/c++draft/swappable.requirements
// because std::is_swappable brings std::swap https://eel.is/c++draft/utility.swap#lib:swap
// into scope, which we do not want for this check.
template <typename T>
concept is_swappable = requires { swap(std::declval<T &>(), std::declval<T &>()); };

template <typename T>
concept is_nothrow_swappable = requires {
  { swap(std::declval<T &>(), std::declval<T &>()) } noexcept;
};

// SFINAE probes for the comparison operators' constraints; a generic (dependent) context, so
// a non-viable comparison yields false instead of a hard error.
template <typename L, typename R = L>
concept has_eq = requires(L const &l, R const &r) { l == r; };
template <typename L, typename R = L>
concept has_ne = requires(L const &l, R const &r) { l != r; };
template <typename L, typename R = L>
concept has_lt = requires(L const &l, R const &r) { l < r; };
template <typename L, typename R = L>
concept has_gt = requires(L const &l, R const &r) { l > r; };
template <typename L, typename R = L>
concept has_le = requires(L const &l, R const &r) { l <= r; };
template <typename L, typename R = L>
concept has_ge = requires(L const &l, R const &r) { l >= r; };
template <typename L, typename R = L>
concept has_spaceship = requires(L const &l, R const &r) { l <=> r; };

// SFINAE probe for or_else's constraints (the only constrained monadic operation)
template <typename O, typename F>
concept has_or_else = requires(O &&o, F &&f) { std::forward<O>(o).or_else(std::forward<F>(f)); };

} // namespace

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

#ifndef PFN_TEST_VALIDATION
    SECTION("from-invoke tag ctor is private")
    {
      // is_constructible_v cannot see inaccessible ctors, so this pins the detail tag
      // ctor (used by transform) out of the public interface
      static_assert(not std::is_constructible_v<optional<int>, pfn::detail::_optional_from_invoke_t, int (*)()>);
      SUCCEED();
    }
#endif
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

  SECTION("swap")
  {
    SECTION("non-swappable")
    {
      struct A : non_swappable<A> {};
      static_assert(not std::is_swappable_v<A>);
      static_assert(std::is_move_constructible_v<A>);

      static_assert(not extension || not is_swappable<optional<A>>);

      SUCCEED();
    }

    SECTION("non-move-constructible")
    {
      struct A : swappable<A> {
        A(A &&) = delete;
      };
      static_assert(std::is_swappable_v<A>);
      static_assert(not std::is_move_constructible_v<A>);

      static_assert(not is_swappable<optional<A>>);

#ifndef PFN_TEST_VALIDATION
      // [optional.swap] Mandates (rather than constrains) move-constructibility and only
      // preconditions swappability, unlike [expected.object.swap]'s constraints: the member
      // stays declared even when the namespace-scope swap is constrained away
      static_assert(requires(optional<A> &a, optional<A> &b) { a.swap(b); });
#endif
      SUCCEED();
    }

    SECTION("swappable, nothrow-move-constructible")
    {
      struct A : swappable<A> {
        A(A &&) noexcept(true) = default;
      };
      static_assert(std::is_swappable_v<A>);
      static_assert(not std::is_nothrow_swappable_v<A>);
      static_assert(std::is_nothrow_move_constructible_v<A>);

      static_assert(is_swappable<optional<A>>);
      static_assert(not is_nothrow_swappable<optional<A>>);

      SUCCEED();
    }

    SECTION("nothrow-swappable, no-nothrow-move-constructible")
    {
      struct A : nothrow_swappable<A> {
        A(A &&) noexcept(false) {}
      };
      static_assert(std::is_nothrow_swappable_v<A>);
      static_assert(std::is_move_constructible_v<A>);
      static_assert(not std::is_nothrow_move_constructible_v<A>);

      // unlike expected's swap, a throwing move alone does not disable swap -- the
      // cross-state transfer may throw, with the engagement states left unchanged
      static_assert(is_swappable<optional<A>>);
      static_assert(not is_nothrow_swappable<optional<A>>);

      SUCCEED();
    }

    SECTION("nothrow-swappable, nothrow-move-constructible")
    {
      struct A : nothrow_swappable<A> {
        A(A &&) noexcept(true) = default;
      };
      static_assert(std::is_nothrow_swappable_v<A>);
      static_assert(std::is_nothrow_move_constructible_v<A>);

      static_assert(is_swappable<optional<A>>);
      static_assert(is_nothrow_swappable<optional<A>>);

      static_assert(is_nothrow_swappable<optional<int>>);

      SUCCEED();
    }

    SECTION("both engaged")
    {
      using T = optional<helper>;
      T a(std::in_place, 7);
      T b(std::in_place, 13);
      a.swap(b);
      CHECK(a->v == 13 * swapped);
      CHECK(b->v == 7 * swapped);

      swap(a, b); // the namespace-scope swap, found by ADL
      CHECK(a->v == 7 * swapped * swapped);
      CHECK(b->v == 13 * swapped * swapped);
    }

    SECTION("engaged/disengaged")
    {
      using T = optional<helper>;
      T a(std::in_place, 19);
      T b(std::nullopt);
      a.swap(b);
      CHECK(not a.has_value());
      CHECK(b.has_value());
      CHECK(b->v == 19 * from_rval);

      a.swap(b); // mirrored: *this disengaged, rhs engaged
      CHECK(a.has_value());
      CHECK(a->v == 19 * from_rval * from_rval);
      CHECK(not b.has_value());
    }

    SECTION("both disengaged")
    {
      using T = optional<helper>;
      T a(std::nullopt);
      T b(std::nullopt);
      a.swap(b);
      CHECK(not a.has_value());
      CHECK(not b.has_value());
    }

    SECTION("exception")
    {
      // a throwing move during the cross-state transfer: engagement states unchanged
      using T = optional<helper_t<3>>;
      static_assert(not std::is_nothrow_move_constructible_v<helper_t<3>>);

      // constructed via the initializer_list ctor (no V<8 throw-check there) to get a
      // stored 0 without the value ctor itself throwing first
      T a(std::in_place, {0.0});
      T b(std::nullopt);
      try {
        a.swap(b);
        FAIL();
      } catch (std::runtime_error const &e) {
        CHECK(std::strcmp(e.what(), "invalid input") == 0);
        CHECK(a.has_value());
        CHECK(a->v == 0);
        CHECK(not b.has_value());
      }

      try {
        b.swap(a); // mirrored orientation takes the delegating branch
        FAIL();
      } catch (std::runtime_error const &) {
        CHECK(a.has_value());
        CHECK(not b.has_value());
      }
    }

    SECTION("constexpr")
    {
      using T = optional<int>;

      // in-lambda asserts rather than the fn-return idiom of the sibling sections: copying a
      // swapped-to-disengaged std::optional out of the lambda is not a constant expression on
      // libstdc++ 14 (its swap ends the donor payload's lifetime; the trivial copy reads it)
      SECTION("cross-state")
      {
        static_assert([] {
          T a(std::in_place, 12);
          T b(std::nullopt);
          swap(a, b);
          return not a.has_value() && b.has_value() && *b == 12;
        }());
        static_assert([] {
          T a(std::nullopt);
          T b(std::in_place, 12);
          swap(a, b);
          return a.has_value() && *a == 12 && not b.has_value();
        }());
        SUCCEED();
      }

      SECTION("same-state")
      {
        static_assert([] {
          T a(std::in_place, 7);
          T b(std::in_place, 12);
          swap(a, b);
          return *a == 12 && *b == 7;
        }());
        static_assert([] {
          T a(std::nullopt);
          T b(std::nullopt);
          swap(a, b);
          return not a.has_value() && not b.has_value();
        }());
        SUCCEED();
      }
    }
  }

#if !defined(PFN_TEST_VALIDATION) || defined(__cpp_lib_optional_range_support)
  SECTION("iterator support")
  {
    static_assert(std::contiguous_iterator<optional<int>::iterator>);
    static_assert(std::contiguous_iterator<optional<int>::const_iterator>);
    static_assert(std::is_same_v<std::iter_value_t<optional<int const>::iterator>, int>);
    static_assert(std::is_same_v<std::iter_reference_t<optional<int>::iterator>, int &>);
    static_assert(std::is_same_v<std::iter_reference_t<optional<int>::const_iterator>, int const &>);
    static_assert(std::ranges::contiguous_range<optional<int>>);
    static_assert(std::ranges::enable_view<optional<int>>);
#if defined(__cpp_lib_format_ranges)
    static_assert(std::format_kind<optional<int>> == std::range_format::disabled);
#endif

    SECTION("engaged")
    {
      optional<int> o{std::in_place, 42};

      SECTION("mutable")
      {
        CHECK(o.end() - o.begin() == 1);
        CHECK(std::addressof(*o.begin()) == std::addressof(*o));

        SECTION("write-through")
        {
          *o.begin() += 1;
          CHECK(*o == 43);
        }
      }

      SECTION("const")
      {
        auto const &c = o;
        static_assert(std::is_same_v<decltype(*c.begin()), int const &>);
        CHECK(c.end() - c.begin() == 1);
        CHECK(std::addressof(*c.begin()) == std::addressof(*o));
      }

      SECTION("range-based for")
      {
        int count = 0;
        for (auto &v : o) {
          CHECK(v == 42);
          count += 1;
        }
        CHECK(count == 1);
      }

      SECTION("iterator operations")
      {
        auto it = o.begin();
        auto const e = o.end();
        CHECK(*it == 42);
        CHECK(it[0] == 42);
        CHECK(it != e);
        CHECK(it < e);
        CHECK(it <= e);
        CHECK(e > it);
        CHECK(e >= it);
        CHECK(++it == e);
        CHECK(--it == o.begin());
        CHECK(it++ == o.begin());
        CHECK(it-- == e);
        CHECK(it + 1 == e);
        CHECK(1 + it == e);
        CHECK(e - 1 == it);
        CHECK(e - it == 1);
        it += 1;
        CHECK(it == e);
        it -= 1;
        CHECK(it == o.begin());

        SECTION("conversion to const_iterator")
        {
          optional<int>::const_iterator cit = it;
          CHECK(cit == it);
          CHECK(it == cit);
          CHECK(it - cit == 0);
        }

        SECTION("member access")
        {
          struct pt {
            int x;
          };
          optional<pt> p{std::in_place, pt{7}};
          CHECK(p.begin()->x == 7);
        }

        SECTION("constexpr")
        {
          static_assert([] {
            optional<int> a{std::in_place, 42};
            auto i = a.begin();
            auto const s = a.end();
            bool ok = *i == 42 && i[0] == 42 && i != s && i < s && i <= s && s > i && s >= i;
            ok = ok && ++i == s;
            ok = ok && --i == a.begin();
            ok = ok && i++ == a.begin();
            ok = ok && i-- == s;
            ok = ok && i + 1 == s && 1 + i == s && s - 1 == i && s - i == 1;
            i += 1;
            ok = ok && i == s;
            i -= 1;
            ok = ok && i == a.begin();
            optional<int>::const_iterator ci = i;
            return ok && ci == i && i == ci && i - ci == 0;
          }());
          static_assert([] {
            struct pt {
              int x;
            };
            optional<pt> p{std::in_place, pt{7}};
            return p.begin()->x == 7;
          }());
          SUCCEED();
        }
      }
    }

    SECTION("disengaged")
    {
      optional<int> o{};

      SECTION("mutable") { CHECK(o.begin() == o.end()); }

      SECTION("const")
      {
        auto const &c = o;
        CHECK(c.begin() == c.end());
      }

      SECTION("range-based for")
      {
        int count = 0;
        for ([[maybe_unused]] auto &v : o)
          count += 1;
        CHECK(count == 0);
      }
    }

    SECTION("constexpr")
    {
      static_assert([] {
        optional<int> o{std::in_place, 7};
        int n = 0;
        for (auto v : o)
          n += v;
        return n == 7;
      }());
      static_assert([] {
        optional<int> o{};
        return o.begin() == o.end();
      }());
      SUCCEED();
    }
  }
#endif

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

// C++23 members: gated away when validating against a pre-C++23 std::optional
#if !defined(PFN_TEST_VALIDATION) || (defined(__cpp_lib_optional) && __cpp_lib_optional >= 202110L)
  SECTION("monadic functions")
  {
    SECTION("and_then")
    {
      SECTION("value")
      {
        using T = optional<helper>;
        constexpr auto fn
            = [](auto &&a) constexpr -> optional<int> { return helper(std::forward<decltype(a)>(a)).v * 2; };

        T a(std::in_place, 7);
        static_assert(std::is_same_v<decltype(a.and_then(fn)), optional<int>>);

        // extension: conditional noexcept, keyed on the callable
        constexpr auto nx = [](auto &&) noexcept -> optional<int> { return 1; };
        static_assert(not extension || noexcept(a.and_then(nx)));
        static_assert(not extension || not noexcept(a.and_then(fn)));

        CHECK(a.and_then(fn).value() == 7 * 2 * from_lval);
        CHECK(std::as_const(a).and_then(fn).value() == 7 * 2 * from_lval_const);
        CHECK(std::move(std::as_const(a)).and_then(fn).value() == 7 * 2 * from_rval_const);
        CHECK(std::move(a).and_then(fn).value() == 7 * 2 * from_rval);
      }

      SECTION("disengaged")
      {
        using T = optional<helper>;
        constexpr auto fn = [](auto &&) constexpr -> optional<int> { return 1; };
        T a(std::nullopt);
        CHECK(not a.and_then(fn).has_value());
        CHECK(not std::as_const(a).and_then(fn).has_value());
        CHECK(not std::move(std::as_const(a)).and_then(fn).has_value());
        CHECK(not std::move(a).and_then(fn).has_value());
      }

      SECTION("constexpr")
      {
        using T = optional<helper>;
        constexpr auto fn
            = [](auto &&a) constexpr -> optional<int> { return helper(std::forward<decltype(a)>(a)).v * 3; };

        SECTION("lval const")
        {
          {
            constexpr T a(std::in_place, {3.0}, 5);
            static_assert(a.and_then(fn).value() == 3 * 3 * 5 * from_lval_const);
          }

          {
            constexpr T a(std::nullopt);
            static_assert(not a.and_then(fn).has_value());
          }

          SUCCEED();
        }

        SECTION("rval")
        {
          static_assert(T{std::in_place, {3.0}, 5}.and_then(fn) == 3 * 3 * 5 * from_rval);
          static_assert(not T{std::nullopt}.and_then(fn).has_value());

          SUCCEED();
        }

        SECTION("lval")
        {
          static_assert([&fn] {
            T a{std::in_place, {3.0}, 5};
            T b{std::nullopt};
            return a.and_then(fn).value() == 3 * 3 * 5 * from_lval //
                   && not b.and_then(fn).has_value();
          }());

          SUCCEED();
        }

        SECTION("rval const")
        {
          static_assert([&fn] {
            T a{std::in_place, {3.0}, 5};
            T b{std::nullopt};
            return std::move(std::as_const(a)).and_then(fn).value() == 3 * 3 * 5 * from_rval_const //
                   && not std::move(std::as_const(b)).and_then(fn).has_value();
          }());

          SUCCEED();
        }
      }

      SECTION("move-only type")
      {
        optional<std::unique_ptr<int>> o(std::in_place, std::make_unique<int>(42));
        auto res = std::move(o).and_then([](std::unique_ptr<int> p) -> optional<int> { return *p + 1; });
        CHECK(res.value() == 43);
      }
    }

    SECTION("or_else")
    {
      SECTION("engaged")
      {
        using T = optional<helper>;
        constexpr auto fn = []() constexpr -> T { return T(std::in_place, 1); };

        T a(std::in_place, 13);

        // extension: conditional noexcept, keyed on the callable and on copying *this (helper's
        // copy constructor is noexcept; a throwing copy makes the second conjunct false)
        constexpr auto nx = []() noexcept -> T { return T(std::nullopt); };
        static_assert(not extension || noexcept(a.or_else(nx)));
        static_assert(not extension || not noexcept(a.or_else(fn)));
        struct throwing_copy_t {
          throwing_copy_t() = default;
          throwing_copy_t(throwing_copy_t const &) {}
        };
        constexpr auto nxt = []() noexcept -> optional<throwing_copy_t> { return {std::nullopt}; };
        static_assert(not extension || not noexcept(std::declval<optional<throwing_copy_t> const &>().or_else(nxt)));

        // or_else has only const& and && overloads; the engaged path returns a copy of *this
        // (the contained value through helper's const& copy ctor) or a move of it
        CHECK(a.or_else(fn).value().v == 13 * from_lval_const);
        CHECK(std::as_const(a).or_else(fn).value().v == 13 * from_lval_const);
        CHECK(std::move(std::as_const(a)).or_else(fn).value().v == 13 * from_lval_const);
        CHECK(std::move(a).or_else(fn).value().v == 13 * from_rval);
      }

      SECTION("disengaged")
      {
        using T = optional<helper>;
        constexpr auto fn = []() constexpr -> T { return T(std::in_place, 5); };
        T a(std::nullopt);
        // the callable's result is returned as-is: no witness factor multiplied in
        CHECK(a.or_else(fn).value().v == 5);
        CHECK(std::move(a).or_else(fn).value().v == 5);
        CHECK(not T(std::nullopt).or_else([]() -> T { return T(std::nullopt); }).has_value());
      }

      SECTION("constraints")
      {
        // [optional.monadic] or_else Constraints: invocable F and, per overload, copy- or
        // move-constructible T
        constexpr auto fn = []() -> optional<helper_move_only> { return {}; };
        static_assert(has_or_else<optional<helper_move_only> &&, decltype(fn)>);
        static_assert(not has_or_else<optional<helper_move_only> &, decltype(fn)>);
        static_assert(not has_or_else<optional<helper_move_only> const &, decltype(fn)>);
        static_assert(not has_or_else<optional<int> &&, int>); // not invocable
        SUCCEED();
      }

      SECTION("constexpr")
      {
        using T = optional<int>;
        constexpr auto fn = []() constexpr -> T { return T(99); };
        static_assert(T(5).or_else(fn).value() == 5);
        static_assert(T(std::nullopt).or_else(fn).value() == 99);
        static_assert([&fn] {
          T a(7);
          T e(std::nullopt);
          return a.or_else(fn).value() == 7 && e.or_else(fn).value() == 99;
        }());
        SUCCEED();
      }
    }

    SECTION("transform")
    {
      SECTION("value")
      {
        using T = optional<helper>;
        constexpr auto fn = [](auto &&a) constexpr { return helper(std::forward<decltype(a)>(a)).v * 2; };

        T a(std::in_place, 7);
        static_assert(std::is_same_v<decltype(a.transform(fn)), optional<int>>);

        // extension: conditional noexcept, keyed on the callable alone -- the result is
        // direct-initialized from the invoke expression (guaranteed elision), so even an
        // immovable result type keeps it
        constexpr auto nx = [](auto &&) noexcept { return 1; };
        static_assert(not extension || noexcept(a.transform(nx)));
        static_assert(not extension || not noexcept(a.transform(fn)));
        constexpr auto nxi = [](auto &&) noexcept { return helper_immovable(3, 4); };
        static_assert(not extension || noexcept(a.transform(nxi)));

        CHECK(a.transform(fn).value() == 7 * 2 * from_lval);
        CHECK(std::as_const(a).transform(fn).value() == 7 * 2 * from_lval_const);
        CHECK(std::move(std::as_const(a)).transform(fn).value() == 7 * 2 * from_rval_const);
        CHECK(std::move(a).transform(fn).value() == 7 * 2 * from_rval);
      }

      SECTION("direct initialization")
      {
        // the contained value is direct-non-list-initialized from the invoke result:
        // guaranteed elision, so no copy/move witness factor and an immovable type works
        optional<int> a(7);
        auto r = a.transform([](int v) { return helper(v); });
        static_assert(std::is_same_v<decltype(r), optional<helper>>);
        CHECK(r.value().v == 7);

        static_assert(not std::is_move_constructible_v<helper_immovable>);
        auto ri = a.transform([](int v) { return helper_immovable(v, 3); });
        static_assert(std::is_same_v<decltype(ri), optional<helper_immovable>>);
        CHECK(ri.value().v == 7 * 3);
      }

      SECTION("disengaged")
      {
        using T = optional<helper>;
        constexpr auto fn = [](auto &&) constexpr { return 1; };
        T a(std::nullopt);
        CHECK(not a.transform(fn).has_value());
        CHECK(not std::as_const(a).transform(fn).has_value());
        CHECK(not std::move(std::as_const(a)).transform(fn).has_value());
        CHECK(not std::move(a).transform(fn).has_value());
      }

#ifndef PFN_TEST_VALIDATION
      SECTION("to reference")
      {
        // C++26 via P2988: remove_cv_t<invoke_result_t<F, ...>> does not strip references and
        // T& is now a valid contained type, so a reference-returning callable produces an
        // optional<X&> bound to the returned lvalue
        int x = 5;
        optional<int> a(1);
        auto r = a.transform([&x](int) -> int & { return x; });
        static_assert(std::is_same_v<decltype(r), optional<int &>>);
        CHECK(&*r == &x);
      }
#endif

      SECTION("constexpr")
      {
        using T = optional<helper>;
        constexpr auto fn = [](auto &&a) constexpr { return helper(std::forward<decltype(a)>(a)).v * 3; };

        SECTION("lval const")
        {
          constexpr T a(std::in_place, {3.0}, 5);
          constexpr T e(std::nullopt);
          static_assert(a.transform(fn).value() == 3 * 3 * 5 * from_lval_const);
          static_assert(not e.transform(fn).has_value());
          SUCCEED();
        }

        SECTION("rval")
        {
          static_assert(T{std::in_place, {3.0}, 5}.transform(fn) == 3 * 3 * 5 * from_rval);
          SUCCEED();
        }

        SECTION("lval")
        {
          static_assert([&fn] {
            T a{std::in_place, {3.0}, 5};
            return a.transform(fn).value() == 3 * 3 * 5 * from_lval;
          }());
          SUCCEED();
        }

        SECTION("rval const")
        {
          static_assert([&fn] {
            T a{std::in_place, {3.0}, 5};
            return std::move(std::as_const(a)).transform(fn).value() == 3 * 3 * 5 * from_rval_const;
          }());
          SUCCEED();
        }
      }
    }
  }
#endif

  SECTION("reset")
  {
    using T = optional<helper>;
    static_assert(noexcept(std::declval<T &>().reset()));

    SECTION("engaged")
    {
      // dtor witness: the contained value flips a flag when destroyed
      struct D {
        bool *flag;
        constexpr explicit D(bool *f) noexcept : flag(f) {}
        constexpr ~D() { *flag = true; }
      };
      bool destroyed = false;
      optional<D> a(std::in_place, &destroyed);
      CHECK(a.has_value());
      a.reset();
      CHECK(not a.has_value());
      CHECK(destroyed);
    }

    SECTION("disengaged")
    {
      T a(std::nullopt);
      a.reset(); // no effect
      CHECK(not a.has_value());
    }

    SECTION("constexpr")
    {
      static_assert([] {
        T a{std::in_place, helper_list_t{}, 5};
        a.reset();
        bool const disengaged = not a.has_value();
        a.emplace(helper_list_t{3.0}, 7);
        return disengaged && a.has_value() && a->v == 3 * 7;
      }());
      SUCCEED();
    }
  }

  SECTION("relational operators")
  {
    SECTION("constraints")
    {
      struct A {};
      struct B {
        constexpr bool operator==(B const &) const noexcept = default;
      };
      // each operator is individually constrained on its own comparison expression
      static_assert(not extension || not has_eq<optional<A>>);
      static_assert(not extension || not has_ne<optional<A>>);
      static_assert(not extension || not has_lt<optional<A>>);
      static_assert(not extension || not has_gt<optional<A>>);
      static_assert(not extension || not has_le<optional<A>>);
      static_assert(not extension || not has_ge<optional<A>>);
      static_assert(has_eq<optional<B>>);
      static_assert(has_ne<optional<B>>); // *x != *y is well-formed through B's rewritten ==
      static_assert(not has_spaceship<optional<A>>);
      static_assert(not has_spaceship<optional<B>>); // == alone does not satisfy three_way_comparable_with
// libc++ 16 has no operator<=> for std::optional (P1614 incomplete there);
// __cpp_lib_three_way_comparison tracks the stdlib's library-wide <=> support
#if !defined(PFN_TEST_VALIDATION) || defined(__cpp_lib_three_way_comparison)
      static_assert(has_spaceship<optional<int>, optional<long>>);
#endif
      SUCCEED();
    }

    SECTION("equality")
    {
      SECTION("same type")
      {
        using T = optional<helper>;
        T const e1(std::nullopt);
        T const e2(std::nullopt);
        T const v1(std::in_place, 12);
        T const v2(std::in_place, {3.0}, 4);
        T const v3(std::in_place, 5);
        CHECK((e1 == e2));
        CHECK(not(e1 != e2));
        CHECK(not(e1 == v1));
        CHECK((e1 != v1));
        CHECK(not(v1 == e1)); // both argument orders go through the one [optional.relops] overload
        CHECK((v1 != e1));
        CHECK((v1 == v2));
        CHECK(not(v1 != v2));
        CHECK(not(v1 == v3));
        CHECK((v1 != v3));
      }

      SECTION("different types")
      {
        using V = optional<int>;
        using W = optional<double>;
        static_assert(V{12} == W{12.0});
        static_assert(V{12} != W{12.5});
        static_assert(V{std::nullopt} == W{std::nullopt});
        static_assert(V{12} != W{std::nullopt});
        SUCCEED();
      }
    }

    SECTION("ordering")
    {
      using V = optional<int>;
      constexpr V e(std::nullopt);
      constexpr V a(1);
      constexpr V b(2);
      // a disengaged optional orders before every engaged one
      static_assert(e < a);
      static_assert(not(a < e));
      static_assert(e <= a);
      static_assert(e <= e);
      static_assert(not(e < e));
      static_assert(not(e > e));
      static_assert(e >= e);
      static_assert(a < b);
      static_assert(a <= b);
      static_assert(b > a);
      static_assert(b >= a);
      static_assert(a > e);
      static_assert(a >= e);

      SECTION("different types")
      {
        static_assert(optional<int>(1) < optional<double>(1.5));
        static_assert(optional<double>(0.5) < optional<int>(1));
        SUCCEED();
      }

      SECTION("runtime")
      {
        V const e1(std::nullopt);
        V const v1(3);
        V const v2(7);
        CHECK((e1 < v1));
        CHECK(not(v1 < e1));
        CHECK((v1 < v2));
        CHECK((v2 > v1));
        CHECK((v1 <= v1));
        CHECK((v1 >= v1));
        // disengaged branches again, at runtime: the constexpr matrix above earns no coverage
        CHECK(not(e1 > v1));
        CHECK((v1 > e1));
        CHECK((e1 <= v1));
        CHECK(not(v1 <= e1));
        CHECK((v1 >= e1));
        CHECK(not(e1 >= v1));
      }
    }

#if !defined(PFN_TEST_VALIDATION) || defined(__cpp_lib_three_way_comparison)
    SECTION("three-way") // no std::optional operator<=> in libc++ 16
    {
      using V = optional<int>;
      static_assert(
          std::is_same_v<decltype(std::declval<V const &>() <=> std::declval<V const &>()), std::strong_ordering>);
      static_assert(std::is_same_v<decltype(std::declval<V const &>() <=> std::declval<optional<double> const &>()),
                                   std::partial_ordering>);
      constexpr V e(std::nullopt);
      constexpr V a(1);
      constexpr V b(2);
      static_assert((e <=> e) == std::strong_ordering::equal);
      static_assert((e <=> a) == std::strong_ordering::less);
      static_assert((a <=> e) == std::strong_ordering::greater);
      static_assert((a <=> b) == std::strong_ordering::less);
      static_assert((a <=> a) == std::strong_ordering::equal);

      SECTION("partial ordering")
      {
        optional<double> const n(std::numeric_limits<double>::quiet_NaN());
        optional<double> const e1(std::nullopt);
        optional<double> const v(1.0);
        CHECK((n <=> v) == std::partial_ordering::unordered);
        CHECK((n <=> n) == std::partial_ordering::unordered); // engaged NaNs compare unordered...
        CHECK((e1 <=> n) == std::partial_ordering::less);     // ...but engagement still orders first
      }
    }
#endif
  }

  SECTION("comparison with nullopt")
  {
    using V = optional<int>;
    // [optional.nullops] both operators are noexcept, and <=> returns strong_ordering for any T
    static_assert(noexcept(std::declval<V const &>() == std::nullopt));
    static_assert(noexcept(std::nullopt == std::declval<V const &>()));
#if !defined(PFN_TEST_VALIDATION) || defined(__cpp_lib_three_way_comparison)
    // no std::optional operator<=> in libc++ 16
    static_assert(noexcept(std::declval<V const &>() <=> std::nullopt));
    static_assert(
        std::is_same_v<decltype(std::declval<optional<double> const &>() <=> std::nullopt), std::strong_ordering>);
#endif

    constexpr V e(std::nullopt);
    constexpr V v(5);
    static_assert(e == std::nullopt);
    static_assert(std::nullopt == e);
    static_assert(v != std::nullopt);
    static_assert(std::nullopt != v);
#if !defined(PFN_TEST_VALIDATION) || defined(__cpp_lib_three_way_comparison)
    static_assert((e <=> std::nullopt) == std::strong_ordering::equal);
    static_assert((v <=> std::nullopt) == std::strong_ordering::greater);
#endif
    // the ordering relations are rewritten from <=> (libc++ 16 falls back to its legacy operators)
    static_assert(std::nullopt < v);
    static_assert(not(std::nullopt < e));
    static_assert(e <= std::nullopt);
    static_assert(v > std::nullopt);
    static_assert(v >= std::nullopt);
    static_assert(not(std::nullopt >= v));

    V const r(7);
    CHECK(not(r == std::nullopt));
    CHECK((r != std::nullopt));
    CHECK((std::nullopt < r));
  }

  SECTION("comparison with value")
  {
    SECTION("constraints")
    {
      struct A {};
      static_assert(not extension || not has_eq<optional<A>, A>);
      static_assert(not extension || not has_eq<A, optional<A>>);
      static_assert(not extension || not has_lt<optional<A>, A>);
      static_assert(not extension || not has_lt<A, optional<A>>);

      // LWG4072: the value operand must not itself be a specialization of optional. weird_t
      // compares only with a whole optional<move_only_t> (its == takes exactly that), and with
      // the value-operand overloads constrained away no comparison is viable at all: the
      // [optional.relops] overload would need weird_t == move_only_t, which does not exist
      // (move_only_t is not copyable, so it does not implicitly convert to optional<move_only_t>)
      struct move_only_t {
        move_only_t() = default;
        move_only_t(move_only_t &&) = default;
      };
      struct weird_t {
        constexpr bool operator==(optional<move_only_t> const &) const noexcept { return true; }
      };
      static_assert(not extension || not has_eq<optional<weird_t>, optional<move_only_t>>);
      // ...while as a direct value operand weird_t's own member operator== still applies
      static_assert(has_eq<weird_t, optional<move_only_t>>);
      SUCCEED();
    }

    SECTION("equality")
    {
      using V = optional<int>;
      constexpr V e(std::nullopt);
      constexpr V v(5);
      static_assert(v == 5);
      static_assert(5 == v);
      static_assert(v != 7);
      static_assert(7 != v);
      static_assert(not(e == 5));
      static_assert(e != 5);
      static_assert(not(5 == e));
      static_assert(5 != e);
      static_assert(v == 5.0); // heterogeneous
      static_assert(v != 5.5);

      using T = optional<helper>;
      T const t(std::in_place, {3.0}, 4);
      CHECK((t == helper(12)));
      CHECK((helper(12) == t));
      CHECK((t != helper(7)));
      CHECK((helper(7) != t));
      CHECK(not(T(std::nullopt) == helper(12)));
    }

    SECTION("ordering")
    {
      using V = optional<int>;
      constexpr V e(std::nullopt);
      constexpr V v(5);
      // a disengaged optional orders before any value
      static_assert(e < 0);
      static_assert(not(0 < e));
      static_assert(e <= 0);
      static_assert(not(0 <= e));
      static_assert(0 > e);
      static_assert(not(e > 0));
      static_assert(0 >= e);
      static_assert(not(e >= 0));
      static_assert(v < 6);
      static_assert(4 < v);
      static_assert(v <= 5);
      static_assert(5 <= v);
      static_assert(v > 4);
      static_assert(6 > v);
      static_assert(v >= 5);
      static_assert(5 >= v);
      static_assert(v < 5.5); // heterogeneous

      V const r(3);
      CHECK((r < 5));
      CHECK((5 > r));
      CHECK((V(std::nullopt) < 5));
    }

#if !defined(PFN_TEST_VALIDATION) || defined(__cpp_lib_three_way_comparison)
    SECTION("three-way") // no std::optional operator<=> in libc++ 16
    {
      using V = optional<int>;
      static_assert(std::is_same_v<decltype(std::declval<V const &>() <=> 5), std::strong_ordering>);
      static_assert(std::is_same_v<decltype(std::declval<V const &>() <=> 5.0), std::partial_ordering>);
      constexpr V e(std::nullopt);
      constexpr V v(5);
      static_assert((v <=> 5) == std::strong_ordering::equal);
      static_assert((v <=> 7) == std::strong_ordering::less);
      static_assert((e <=> 5) == std::strong_ordering::less);  // disengaged: strong_ordering::less
      static_assert((5 <=> v) == std::strong_ordering::equal); // reversed, synthesized by rewriting
      static_assert((5 <=> e) == std::strong_ordering::greater);

      // a type derived from optional is compared by [optional.relops], not [optional.comp.with.t]
      struct derived_t : optional<int> {
        using optional<int>::optional;
      };
      static_assert(has_spaceship<optional<int>, derived_t>);
      CHECK((V(5) <=> derived_t(3)) == std::strong_ordering::greater);
      CHECK((V(5) <=> derived_t(std::nullopt)) == std::strong_ordering::greater); // compared as optionals
    }
#endif
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

    SECTION("from-invoke tag ctor is private")
    {
      // is_constructible_v cannot see inaccessible ctors, so this pins the detail tag
      // ctor (used by transform) out of the public interface
      static_assert(not std::is_constructible_v<optional<int &>, pfn::detail::_optional_from_invoke_t, int &(*)()>);
      SUCCEED();
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

  SECTION("swap")
  {
    using T = optional<int &>;
    static_assert(noexcept(std::declval<T &>().swap(std::declval<T &>())));
    // the namespace-scope swap accepts optional<T&> through the is_reference_v arm of its
    // constraint, and is unconditionally noexcept here
    static_assert(is_swappable<T>);
    static_assert(is_nothrow_swappable<T>);

    SECTION("rebinds both sides")
    {
      int x = 5, y = 9;
      T a(std::in_place, x);
      T b(std::in_place, y);
      a.swap(b);
      CHECK(&*a == &y);
      CHECK(&*b == &x);
      CHECK(x == 5); // a pointer swap, never a value swap: referents untouched
      CHECK(y == 9);

      swap(a, b);
      CHECK(&*a == &x);
      CHECK(&*b == &y);
    }

    SECTION("engaged/disengaged")
    {
      int x = 5;
      T a(std::in_place, x);
      T b(std::nullopt);
      a.swap(b);
      CHECK(not a.has_value());
      CHECK(&*b == &x);
    }

    SECTION("constexpr")
    {
      static_assert([] {
        int x = 1, y = 2;
        T a(std::in_place, x);
        T b(std::in_place, y);
        a.swap(b);
        return &*a == &y && &*b == &x && x == 1 && y == 2;
      }());
      SUCCEED();
    }
  }

  SECTION("iterator support")
  {
    static_assert(std::contiguous_iterator<optional<int &>::iterator>);
    static_assert(std::is_same_v<std::iter_value_t<optional<int const &>::iterator>, int>);
    static_assert(std::is_same_v<std::iter_reference_t<optional<int &>::iterator>, int &>);
    static_assert(std::ranges::contiguous_range<optional<int &>>);
    static_assert(std::ranges::enable_view<optional<int &>>);
    // const-only single overload, like the observers: const does not propagate to the referent
    static_assert(std::is_same_v<decltype(std::declval<optional<int &> const &>().begin()), //
                                 optional<int &>::iterator>);

    SECTION("engaged")
    {
      int x = 42;
      optional<int &> const o{std::in_place, x};

      SECTION("referent identity")
      {
        CHECK(o.end() - o.begin() == 1);
        CHECK(std::addressof(*o.begin()) == std::addressof(x));
      }

      SECTION("write-through")
      {
        for (auto &v : o)
          v += 1;
        CHECK(x == 43);
      }
    }

    SECTION("disengaged")
    {
      optional<int &> const o{};
      CHECK(o.begin() == o.end());
      int count = 0;
      for ([[maybe_unused]] auto &v : o)
        count += 1;
      CHECK(count == 0);
    }

    SECTION("constexpr")
    {
      static_assert([] {
        int x = 3;
        optional<int &> const o{std::in_place, x};
        for (auto &v : o)
          v += 4;
        return x == 7 && std::addressof(*o.begin()) == std::addressof(x);
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

  SECTION("monadic functions")
  {
    using T = optional<int &>;

    SECTION("and_then")
    {
      int x = 5;
      T const a(std::in_place, x);
      T const e(std::nullopt);
      // the callable always receives plain int&, even through a const optional<int&>
      constexpr auto fn = [](int &v) -> optional<double> { return v * 2.0; };
      static_assert(std::is_same_v<decltype(a.and_then(fn)), optional<double>>);

      // extension: conditional noexcept, keyed on the callable
      constexpr auto nx = [](int &) noexcept -> optional<int> { return 1; };
      static_assert(noexcept(a.and_then(nx)));
      static_assert(not noexcept(a.and_then(fn)));

      CHECK(a.and_then(fn).value() == 10.0);
      CHECK(not e.and_then(fn).has_value());

      // the referent itself is passed, not a copy
      auto same = [&x](int &v) -> optional<bool> { return &v == &x; };
      CHECK(a.and_then(same).value());

      SECTION("constexpr")
      {
        static_assert([] {
          int y = 5;
          T const o(std::in_place, y);
          T const d(std::nullopt);
          auto f = [](int &v) -> optional<int> { return v + 1; };
          return o.and_then(f).value() == 6 && not d.and_then(f).has_value();
        }());
        SUCCEED();
      }
    }

    SECTION("or_else")
    {
      int x = 5;
      int y = 9;
      T const a(std::in_place, x);
      T const e(std::nullopt);
      auto fn = [&y]() -> T { return T(std::in_place, y); };

      // extension: conditional noexcept, keyed on the callable alone (no copy conjunct:
      // the engaged path only rebinds a pointer, which cannot throw)
      constexpr auto nx = []() noexcept -> T { return T(std::nullopt); };
      static_assert(noexcept(a.or_else(nx)));
      static_assert(not noexcept(a.or_else(fn)));

      // engaged: rebinds to the same referent; disengaged: the callable's result
      auto r1 = a.or_else(fn);
      CHECK(&*r1 == &x);
      auto r2 = e.or_else(fn);
      CHECK(&*r2 == &y);
      // [optional.ref.monadic] Constraints: only invocable F, no copy-constructible arm
      static_assert(has_or_else<T const &, decltype(fn)>);
      static_assert(not has_or_else<T const &, int>);

      SECTION("constexpr")
      {
        static_assert([] {
          int v = 1;
          int w = 2;
          T const o(std::in_place, v);
          T const d(std::nullopt);
          auto f = [&w]() -> T { return T(std::in_place, w); };
          return &*o.or_else(f) == &v && &*d.or_else(f) == &w;
        }());
        SUCCEED();
      }
    }

    SECTION("transform")
    {
      int x = 5;
      T const a(std::in_place, x);
      T const e(std::nullopt);

      SECTION("to value")
      {
        constexpr auto fn = [](int &v) { return v * 3; };
        static_assert(std::is_same_v<decltype(a.transform(fn)), optional<int>>);

        // extension: conditional noexcept, keyed on the callable alone -- the result is
        // direct-initialized from the invoke expression (guaranteed elision), so even an
        // immovable result type keeps it
        constexpr auto nx = [](int &) noexcept { return 1; };
        static_assert(noexcept(a.transform(nx)));
        static_assert(not noexcept(a.transform(fn)));
        constexpr auto nxi = [](int &) noexcept { return helper_immovable(3, 4); };
        static_assert(noexcept(a.transform(nxi)));

        CHECK(a.transform(fn).value() == 15);
        CHECK(not e.transform(fn).has_value());

        // direct-non-list-initialized from the invoke result: no witness factor
        auto r = a.transform([](int &v) { return helper(v); });
        CHECK(r.value().v == 5);
      }

      SECTION("to reference")
      {
        // a reference-returning callable yields optional<X&>, here still bound to x
        constexpr auto fn = [](int &v) -> int & { return v; };
        auto r = a.transform(fn);
        static_assert(std::is_same_v<decltype(r), optional<int &>>);
        CHECK(&*r == &x);
        CHECK(not e.transform(fn).has_value());
      }

      SECTION("constexpr")
      {
        static_assert([] {
          int y = 7;
          T const o(std::in_place, y);
          auto f = [](int &v) { return v * 2; };
          return o.transform(f).value() == 14;
        }());
        SUCCEED();
      }
    }
  }

  SECTION("reset")
  {
    using T = optional<int &>;
    static_assert(noexcept(std::declval<T &>().reset()));

    int x = 5;
    T a(std::in_place, x);
    a.reset();
    CHECK(not a.has_value());
    CHECK(x == 5); // referent untouched
    a.reset();     // no effect when already disengaged
    CHECK(not a.has_value());

    SECTION("constexpr")
    {
      static_assert([] {
        int x = 5;
        T o(std::in_place, x);
        o.reset();
        return not o.has_value() && x == 5;
      }());
      SUCCEED();
    }
  }

  SECTION("relational operators")
  {
    using T = optional<int &>;

    SECTION("with optional")
    {
      // compares the referents' values, never their addresses
      int x = 5;
      int y = 5;
      int z = 7;
      T const a(std::in_place, x);
      T const b(std::in_place, y);
      T const c(std::in_place, z);
      T const e(std::nullopt);
      CHECK((a == b));
      CHECK(not(a != b));
      CHECK((a != c));
      CHECK((a < c));
      CHECK((c > a));
      CHECK((a <= b));
      CHECK((a >= b));
      CHECK((e < a));
      CHECK((e == e));
      static_assert(std::is_same_v<decltype(std::declval<T const &>() <=> std::declval<T const &>()), //
                                   std::strong_ordering>);
      CHECK((a <=> b) == std::strong_ordering::equal);
      CHECK((e <=> a) == std::strong_ordering::less);

      SECTION("with optional<U>")
      {
        // mixed with a value optional, in both argument orders
        optional<int> const v(5);
        optional<int> const w(9);
        CHECK((a == v));
        CHECK((v == a));
        CHECK((a < w));
        CHECK((w > a));
        CHECK((a <=> v) == std::strong_ordering::equal);
      }

      SECTION("constexpr")
      {
        static_assert([] {
          int x = 1;
          int y = 2;
          T const a(std::in_place, x);
          T const b(std::in_place, y);
          T const e(std::nullopt);
          return a != b && a < b && e < a && (a <=> b) == std::strong_ordering::less;
        }());
        SUCCEED();
      }
    }

    SECTION("with nullopt")
    {
      static_assert(noexcept(std::declval<T const &>() == std::nullopt));
      static_assert(noexcept(std::declval<T const &>() <=> std::nullopt));
      int x = 5;
      T const a(std::in_place, x);
      T const e(std::nullopt);
      CHECK((e == std::nullopt));
      CHECK((a != std::nullopt));
      CHECK((std::nullopt < a));
      CHECK((a <=> std::nullopt) == std::strong_ordering::greater);
    }

    SECTION("with value")
    {
      int x = 5;
      T const a(std::in_place, x);
      T const e(std::nullopt);
      CHECK((a == 5));
      CHECK((5 == a));
      CHECK((a != 7));
      CHECK((a < 9));
      CHECK((9 > a));
      CHECK((e < 5)); // disengaged orders before any value
      CHECK(not(5 < e));
      CHECK((a <=> 5) == std::strong_ordering::equal);
      CHECK((e <=> 5) == std::strong_ordering::less);
    }
  }
}
#endif

TEST_CASE("make_optional", "[optional][polyfill][make_optional]")
{
#ifndef PFN_TEST_VALIDATION
  constexpr bool extension = true;
#else
  constexpr bool extension = false;
#endif

  SECTION("deduced")
  {
    // decay-copies its argument into an optional<decay_t<T>>
    static_assert(std::is_same_v<decltype(make_optional(5)), optional<int>>);
    static_assert(std::is_same_v<decltype(make_optional("abc")), optional<char const *>>);
    static_assert(not extension || noexcept(make_optional(5)));

    auto const o = make_optional(42);
    static_assert(std::is_same_v<decltype(o), optional<int> const>);
    CHECK((o == 42));

    helper x(13);
    static_assert(std::is_same_v<decltype(make_optional(x)), optional<helper>>); // decayed, no reference
    CHECK(make_optional(x)->v == 13 * from_lval);
    CHECK(make_optional(std::move(x))->v == 13 * from_rval);

    SECTION("constexpr")
    {
      static_assert(*make_optional(42) == 42);
      static_assert(make_optional(1.5) == optional<double>(1.5));
      SUCCEED();
    }
  }

  SECTION("in_place")
  {
    // an explicit type template-argument always selects the in_place overloads: [optional.specalg]
    // constrains the deducing overload away from any such call
    static_assert(std::is_same_v<decltype(make_optional<helper>(3, 4)), optional<helper>>);
    auto const o = make_optional<helper>(3, 4);
    CHECK(o->v == 3 * 4);                    // direct construction: no copy/move witness factor
    CHECK(make_optional<helper>(5)->v == 5); // even with a single argument

    SECTION("initializer_list")
    {
      static_assert(std::is_same_v<decltype(make_optional<helper>({3.0}, 4)), optional<helper>>);
      CHECK(make_optional<helper>({3.0}, 4)->v == 3 * 4);
    }

    SECTION("constexpr")
    {
      static_assert(*make_optional<int>(7) == 7);
      static_assert(make_optional<helper>({3.0}, 4)->v == 12);
      SUCCEED();
    }
  }

#ifndef PFN_TEST_VALIDATION
  SECTION("reference")
  {
    // make_optional<U&>(u) binds instead of decaying: the [optional.specalg] constraint on the
    // deducing overload is what routes an explicit type template-argument to in_place
    int i = 5;
    static_assert(std::is_same_v<decltype(make_optional<int &>(i)), optional<int &>>);
    auto const o = make_optional<int &>(i);
    CHECK(&*o == &i);

    static_assert(std::is_same_v<decltype(make_optional<int const &>(i)), optional<int const &>>);
    auto const c = make_optional<int const &>(i);
    CHECK(&*c == &i);

    SECTION("constexpr")
    {
      static_assert([] {
        int i = 5;
        auto const o = make_optional<int &>(i);
        return &*o == &i && *o == 5;
      }());
      SUCCEED();
    }
  }
#endif
}

TEST_CASE("optional hash", "[optional][polyfill][hash]")
{
#ifndef PFN_TEST_VALIDATION
  constexpr bool extension = true;
#else
  constexpr bool extension = false;
#endif

  SECTION("enabled")
  {
    using T = optional<int>;
    static_assert(std::is_default_constructible_v<std::hash<T>>);
    static_assert(std::is_invocable_r_v<std::size_t, std::hash<T> const &, T const &>);
    static_assert(not extension || noexcept(std::hash<T>{}(std::declval<T const &>())));

    // engaged: hashes to the same value as the underlying hash ([optional.hash])
    T const a(42);
    CHECK(std::hash<T>{}(a) == std::hash<int>{}(42));

    SECTION("remove_const")
    {
      // enablement and value are both keyed on hash<remove_const_t<T>>
      optional<int const> const b(std::in_place, 42);
      static_assert(std::is_default_constructible_v<std::hash<optional<int const>>>);
      CHECK(std::hash<optional<int const>>{}(b) == std::hash<int>{}(42));
    }

    SECTION("disengaged")
    {
      // an unspecified but consistent value
      T const d1(std::nullopt);
      T const d2(std::nullopt);
      CHECK(std::hash<T>{}(d1) == std::hash<T>{}(d2));
    }
  }

  SECTION("disabled")
  {
    struct no_hash {};
    using T = optional<no_hash>;
    // [unord.hash]: a disabled specialization is not a function object and not constructible
    static_assert(not std::is_default_constructible_v<std::hash<T>>);
    static_assert(not std::is_copy_constructible_v<std::hash<T>>);
    static_assert(not std::is_move_constructible_v<std::hash<T>>);
    static_assert(not std::is_copy_assignable_v<std::hash<T>>);
    static_assert(not std::is_move_assignable_v<std::hash<T>>);
    static_assert(not std::is_invocable_v<std::hash<T>, T const &>);
    SUCCEED();
  }

#ifndef PFN_TEST_VALIDATION
  SECTION("reference")
  {
    // keyed on hash<remove_const_t<T>> with T = int&, and no std::hash for a reference type is
    // ever enabled -- so hash<optional<int&>> is disabled, even though hash<int> is enabled
    using T = optional<int &>;
    static_assert(not std::is_default_constructible_v<std::hash<T>>);
    static_assert(not std::is_invocable_v<std::hash<T>, T const &>);
    SUCCEED();
  }
#endif
}
