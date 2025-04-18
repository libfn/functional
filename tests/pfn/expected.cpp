// Copyright (c) 2025 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <pfn/expected.hpp>

#include <util/helper_types.hpp>

#include <catch2/catch_all.hpp>

#include <initializer_list>
#include <stdexcept>
#include <type_traits>
#include <utility>

enum Error { unknown = 1, file_not_found = 5 };

TEST_CASE("bad_expected_access", "[expected][polyfill][bad_expected_access]")
{
  SECTION("bad_expected_access<void>")
  {
    struct T : pfn::bad_expected_access<void> {};

    static_assert(noexcept(T{}));
    T a;
    static_assert(noexcept(T{a}));
    static_assert(noexcept(T{std::move(a)}));
    static_assert(noexcept(a.what()));
    static_assert(std::is_same_v<decltype(a.what()), char const *>);
    SECTION("constructors and assignment")
    {
      T a1 = [&]() -> T & { return a; }();
      CHECK(a.what() == a1.what());
      T a2 = [&]() -> T && { return std::move(a); }();
      CHECK(a.what() == a2.what());
      T a3 = [&]() -> T const & { return a; }();
      CHECK(a.what() == a3.what());
      T a4 = [&]() -> T const && { return std::move(a); }();
      CHECK(a.what() == a4.what());

      a = [&]() -> T & { return a; }();
      CHECK(T{}.what() == a.what());
      a = [&]() -> T && { return std::move(a); }();
      CHECK(T{}.what() == a.what());
      a = [&]() -> T const & { return a; }();
      CHECK(T{}.what() == a.what());
      a = [&]() -> T const && { return std::move(a); }();
      CHECK(T{}.what() == a.what());
    }
    CHECK(std::strcmp(a.what(), "bad access to expected without expected value") == 0);

    T const b;
    CHECK(&decltype(a)::what == &decltype(b)::what);
    CHECK(a.what() == b.what());

    static_assert(noexcept(T{b}));
    static_assert(noexcept(T{std::move(b)}));
    static_assert(noexcept(b.what()));
    static_assert(std::is_same_v<decltype(b.what()), char const *>);
  }

  SECTION("bad_expected_access<E>")
  {
    using T = pfn::bad_expected_access<helper>;
    static_assert(std::is_base_of_v<pfn::bad_expected_access<void>, T>);

    SECTION("type and noexcept")
    {
      T a{12};
      static_assert(noexcept(T{a}));
      static_assert(noexcept(T{std::move(a)}));
      static_assert(noexcept(a.what()));
      static_assert(std::is_same_v<decltype(a.what()), char const *>);
      static_assert(std::is_same_v<decltype(a.error()), helper &>);
      static_assert(std::is_same_v<decltype(std::move(a).error()), helper &&>);
    }

    SECTION("copy/move constructors")
    {
      T b{1};

      SECTION("lval")
      {
        b.error().v = 11;
        T c = b;
        CHECK(c.error().v == 11 * helper::from_lval_const);
      }

      SECTION("lval const")
      {
        b.error().v = 13;
        T c = std::as_const(b);
        CHECK(c.error().v == 13 * helper::from_lval_const);
      }

      SECTION("rval")
      {
        b.error().v = 17;
        T c = std::move(b);
        CHECK(c.error().v == 17 * helper::from_rval);
      }

      SECTION("rval cont")
      {
        b.error().v = 19;
        T c = std::move(std::as_const(b));
        CHECK(c.error().v == 19 * helper::from_lval_const);
      }
    }

    SECTION("assignment")
    {
      T a{12};
      T b{1};

      SECTION("lval")
      {
        b.error().v = 11;
        a = b;
        CHECK(a.error().v == 11 * helper::from_lval_const);
      }

      SECTION("lval const")
      {
        b.error().v = 13;
        a = std::as_const(b);
        CHECK(a.error().v == 13 * helper::from_lval_const);
      }

      SECTION("rval")
      {
        b.error().v = 17;
        a = std::move(b);
        CHECK(a.error().v == 17 * helper::from_rval);
      }

      SECTION("rval const")
      {
        b.error().v = 19;
        a = std::move(std::as_const(b));
        CHECK(a.error().v == 19 * helper::from_lval_const);
      }
    }

    SECTION("accessors")
    {
      helper c{1};
      T b{1};

      SECTION("lval")
      {
        b.error().v = 11;
        c = b.error();
        CHECK(c.v == 11 * helper::from_lval);
      }

      SECTION("lval const")
      {
        b.error().v = 13;
        c = std::as_const(b).error();
        CHECK(c.v == 13 * helper::from_lval_const);
      }

      SECTION("rval")
      {
        b.error().v = 17;
        c = std::move(b).error();
        CHECK(c.v == 17 * helper::from_rval);
      }

      SECTION("rval const")
      {
        b.error().v = 19;
        c = std::move(std::as_const(b)).error();
        CHECK(c.v == 19 * helper::from_rval_const);
      }
    }

    SECTION("bad_expected_access")
    {
      T a{12};
      CHECK(std::strcmp(a.what(), "bad access to expected without expected value") == 0);
      auto const c = []() {
        struct C : pfn::bad_expected_access<void> {};
        return C{};
      }();
      CHECK(a.what() == c.what());
    }
  }
}

template <auto V> struct dummy final {
  decltype(V) value = V;
};

TEST_CASE("unexpect", "[expected][polyfill][unexpect]")
{
  static_assert(std::is_empty_v<pfn::unexpect_t>);
  static_assert(noexcept(pfn::unexpect_t{}));
  static_assert(std::is_same_v<decltype(pfn::unexpect), pfn::unexpect_t const>);

  // pfn::unexpect can be used as a NTTP
  static_assert(not std::is_empty_v<dummy<pfn::unexpect>>);
  static constexpr auto a = pfn::unexpect;
  static_assert(not std::is_empty_v<dummy<a>>);
  static_assert(std::is_same_v<decltype(a), pfn::unexpect_t const>);
  static_assert(std::is_same_v<dummy<pfn::unexpect>, dummy<a>>);

  SUCCEED();
}

TEST_CASE("unexpected", "[expected][polyfill][unexpected]")
{
  using pfn::unexpected;

  SECTION("is_valid_unexpected")
  {
    using pfn::detail::_is_valid_unexpected;
    static_assert(not _is_valid_unexpected<void>);
    static_assert(not _is_valid_unexpected<void volatile>);
    static_assert(not _is_valid_unexpected<void const>);
    static_assert(not _is_valid_unexpected<void const volatile>);
    static_assert(not _is_valid_unexpected<int *()>);
    static_assert(not _is_valid_unexpected<void()>);
    static_assert(not _is_valid_unexpected<void(int) const>);
    static_assert(not _is_valid_unexpected<int const>);
    static_assert(not _is_valid_unexpected<int volatile>);
    static_assert(not _is_valid_unexpected<int const volatile>);
    static_assert(not _is_valid_unexpected<::pfn::unexpected<int>>);
    static_assert(not _is_valid_unexpected<::pfn::unexpected<Error>>);
    static_assert(_is_valid_unexpected<int>);
    static_assert(_is_valid_unexpected<Error>);
    static_assert(_is_valid_unexpected<std::optional<int>>);
    SUCCEED();
  }

  SECTION("constructors")
  {
    SECTION("CTAD")
    {
      constexpr unexpected c{Error::file_not_found};
      static_assert(c.error() == Error::file_not_found);
      static_assert(std::is_same_v<decltype(c), unexpected<Error> const>);
      static_assert(std::is_nothrow_constructible_v<decltype(c), Error>);
      SUCCEED();

      {
        unexpected c{helper{2}};
        CHECK(c.error().v == 2 * helper::from_rval);
        CHECK(c != unexpected{helper(3)});
        static_assert(std::is_same_v<decltype(c), unexpected<helper>>);
        static_assert(std::is_nothrow_constructible_v<decltype(c), helper>);
      }
    }

    SECTION("no CTAD")
    {
      constexpr unexpected<int> c{42};
      static_assert(c.error() == 42);
      static_assert(std::is_nothrow_constructible_v<decltype(c), int>);
      SUCCEED();

      {
        unexpected<helper> c(3);
        CHECK(c.error().v == 3);
        CHECK(c == unexpected<helper>(std::in_place, 3));
        static_assert(not std::is_nothrow_constructible_v<decltype(c), int>);
      }
    }

    SECTION("in-place, no CTAD")
    {
      unexpected<helper> c(std::in_place, 3, 5);
      CHECK(c.error().v == 3 * 5);
      static_assert(not std::is_nothrow_constructible_v<decltype(c), std::in_place_t, int, int>);

      c.error().v *= helper::from_rval;
      CHECK(c == unexpected<helper>(std::in_place, helper{15}));
    }

    SECTION("in_place, not CTAD, initializer_list, noexcept(false)")
    {
      SECTION("forwarded args")
      {
        unexpected<helper> c(std::in_place, {3.0, 5.0});
        auto const d = 3 * 5;
        CHECK(c.error().v == d);
        static_assert(std::is_nothrow_constructible_v<decltype(c), std::in_place_t, std::initializer_list<double>>);
      }

      SECTION("no forwarded args")
      {
        unexpected<helper> c(std::in_place, {2.0, 2.5});
        CHECK(c.error().v == 5);
        static_assert(std::is_nothrow_constructible_v<decltype(c), std::in_place_t, std::initializer_list<double>>);
      }

      SECTION("exception thrown")
      {
        unexpected<helper> t{13};
        try {
          t = unexpected<helper>{std::in_place, 1, 2, 0};
          FAIL();
        } catch (std::runtime_error const &) {
          SUCCEED();
        }
        CHECK(t.error().v == 13);
      }
    }
  }

  SECTION("accessors")
  {
    helper a{1};

    SECTION("lval")
    {
      unexpected<helper> t{13};
      a = t.error();
      CHECK(a.v == 13 * helper::from_lval);
    }

    SECTION("lval const")
    {
      unexpected<helper> const t{17};
      a = t.error();
      CHECK(a.v == 17 * helper::from_lval_const);
    }

    SECTION("rval")
    {
      unexpected<helper> t{19};
      a = std::move(t).error();
      CHECK(a.v == 19 * helper::from_rval);
    }

    SECTION("rval const")
    {
      unexpected<helper> const t{23};
      a = std::move(t).error();
      CHECK(a.v == 23 * helper::from_rval_const);
    }
  }

  SECTION("assignment")
  {
    unexpected<helper> a{1};

    SECTION("lval")
    {
      unexpected<helper> t{13};
      a = t;
      CHECK(a.error().v == 13 * helper::from_lval_const);
    }

    SECTION("lval const")
    {
      unexpected<helper> const t{17};
      a = t;
      CHECK(a.error().v == 17 * helper::from_lval_const);
    }

    SECTION("rval")
    {
      unexpected<helper> t{19};
      a = std::move(t);
      CHECK(a.error().v == 19 * helper::from_rval);
    }

    SECTION("rval const")
    {
      unexpected<helper> const t{23};
      a = std::move(t);
      CHECK(a.error().v == 23 * helper::from_lval_const);
    }
  }

  SECTION("swap")
  {
    unexpected<helper> a{1};
    a.error().v = 2;
    unexpected b{helper{1}};
    b.error().v = 3;
    a.swap(b);
    CHECK(a.error().v == 3 * helper::swapped);
    CHECK(b.error().v == 2 * helper::swapped);
    b.error() = helper{11};
    swap(a, b);
    CHECK(a.error().v == 11 * helper::from_rval * helper::swapped);
    CHECK(b.error().v == 3 * helper::swapped * helper::swapped);
  }

  SECTION("constexpr all, CTAD")
  {
    constexpr auto test = [](auto i) constexpr {
      unexpected a{i};
      unexpected b{i * 5};
      swap(a, b);
      unexpected c{1};
      c = b;
      b.swap(c);
      return unexpected{b.error() * a.error() * 7};
    };
    constexpr auto c = test(21);
    static_assert(std::is_same_v<decltype(c), unexpected<int> const>);
    static_assert(c.error() == 21 * 21 * 5 * 7);

    SUCCEED();
  }
}

TEST_CASE("expected", "[expected][polyfill]")
{
  using pfn::bad_expected_access;
  using pfn::expected;
  using pfn::unexpect;
  using pfn::unexpect_t;
  using pfn::unexpected;
  constexpr bool extension = true;

  SECTION("constructors")
  {
    SECTION("default unavailable")
    {
      static_assert(not std::is_default_constructible_v<helper>); // prerequisite
      static_assert(not std::is_default_constructible_v<expected<helper, Error>>);
      static_assert(std::is_default_constructible_v<expected<int, helper>>);
      SUCCEED();
    }

    SECTION("default trivial")
    {
      using T = expected<int, Error>;
      static_assert(std::is_default_constructible_v<T>);
      static_assert(not extension || std::is_nothrow_constructible_v<T>);

      constexpr T a;
      static_assert(a.has_value());
      static_assert(a.value() == 0);
      SUCCEED();
    }

    struct A {
      constexpr A() noexcept(true) {}
      constexpr bool operator==(A const &) const = default;

    private:
      int v = 12;
    };

    SECTION("default noexcept(true) from value type")
    {
      using T = expected<A, Error>;
      static_assert(std::is_default_constructible_v<T>);
      static_assert(not extension || std::is_nothrow_constructible_v<T>);

      constexpr T a;
      static_assert(a.has_value());
      static_assert(a.value() == A());

      T b;
      CHECK(b.has_value());
      CHECK(b.value() == A());
    }

    struct B {
      constexpr B() noexcept(false) {}
      int v = 42;
    };

    SECTION("default noexcept(false) from value type")
    {
      using T = expected<B, Error>;
      static_assert(std::is_default_constructible_v<T>);
      static_assert(not extension || not std::is_nothrow_constructible_v<T>);

      constexpr T a;
      static_assert(a.has_value());
      static_assert(a.value().v == 42);

      T b;
      CHECK(b.has_value());
      CHECK(b.value().v == 42);
    }

    SECTION("default ignore noexcept from error type")
    {
      using T = expected<int, B>;
      static_assert(std::is_default_constructible_v<T>);
      static_assert(not extension || std::is_nothrow_constructible_v<T>);
      SUCCEED();
    }

    struct C {
      C() noexcept(false) { throw 7; }
    };

    SECTION("default exception thrown")
    {
      try {
        expected<C, bool> b;
        FAIL();
      } catch (int i) {
        CHECK(i == 7);
      }

      // not a problem if error type ctor is throwing
      using T = expected<int, C>;
      static_assert(std::is_default_constructible_v<T>);
      static_assert(not extension || std::is_nothrow_constructible_v<T>);

      T b;
      CHECK(b.has_value());
      CHECK(b.value() == 0);
    }

    SECTION("value from other expected rval")
    {
      using T = expected<helper, Error>;
      static_assert(std::is_constructible_v<T, expected<int, Error>>);
      static_assert(std::is_constructible_v<T, expected<std::initializer_list<double>, Error>>);
      static_assert(not extension || not std::is_nothrow_constructible_v<T, expected<int, Error>>);
      static_assert(not extension
                    || std::is_nothrow_constructible_v<T, expected<std::initializer_list<double>, Error>>);

      constexpr T b(expected<int, Error>(unexpect, Error::unknown));
      static_assert(b.error() == Error::unknown);

      T c(expected<int, Error>(3));
      CHECK(c.value().v == 3);
    }

    SECTION("error from other expected rval")
    {
      using T = expected<int, helper>;
      static_assert(std::is_constructible_v<T, expected<double, helper>>);

      T d(expected<double, helper>(unexpect, 2));
      CHECK(d.error().v == 2 * helper::from_rval);
    }

    SECTION("value from other expected lval const")
    {
      using T = expected<helper, Error>;
      static_assert(std::is_constructible_v<T, expected<int, Error> const &>);
      static_assert(not extension || not std::is_nothrow_constructible_v<T, expected<int, Error> const &>);
      static_assert(not extension
                    || std::is_nothrow_constructible_v<T, expected<std::initializer_list<double>, Error> const &>);

      constexpr expected<int, Error> v(5);
      constexpr expected<int, Error> e(unexpect, Error::file_not_found);
      constexpr T b(e);
      static_assert(b.error() == Error::file_not_found);

      T c(v);
      CHECK(c.value().v == 5);
      T d(e);
      CHECK(d.error() == Error::file_not_found);
    }

    SECTION("error from other expected lval const")
    {
      using T = expected<int, helper>;
      static_assert(std::is_constructible_v<T, expected<double, helper>>);

      expected<double, helper> const e(unexpect, 3);
      T d(e);
      CHECK(d.error().v == 3 * helper::from_lval_const);
    }

    SECTION("converting")
    {
      using T = expected<helper, Error>;
      static_assert(std::is_constructible_v<T, int>);
      static_assert(not extension || not std::is_nothrow_constructible_v<T, int>);
      static_assert(std::is_constructible_v<T, helper>);
      static_assert(not extension || std::is_nothrow_constructible_v<T, helper>);
      static_assert(std::is_constructible_v<T, std::initializer_list<double>>);
      static_assert(not extension || std::is_nothrow_constructible_v<T, std::initializer_list<double>>);

      T const b(11);
      CHECK(b.value().v == 11);

      T const c(helper(13));
      CHECK(c.value().v == 13 * helper::from_rval);
    }

    SECTION("from unexpected rval")
    {
      using T = expected<int, helper>;
      static_assert(std::is_constructible_v<T, unexpected<int>>);
      static_assert(not extension || not std::is_nothrow_constructible_v<T, unexpected<int>>);
      static_assert(std::is_constructible_v<T, unexpected<std::initializer_list<double>>>);
      static_assert(not extension || std::is_nothrow_constructible_v<T, unexpected<std::initializer_list<double>>>);

      constexpr expected<std::byte, int> a(unexpected<bool>(true));
      static_assert(a.error() == 1);

      T const b(unexpected<int>(5));
      CHECK(b.error().v == 5);
    }

    SECTION("from unexpected lval const")
    {
      using T = expected<int, helper>;
      constexpr auto g1 = unexpected<int>(5);
      constexpr expected<std::byte, int> a(g1);
      static_assert(a.error() == 5);

      T const b(g1);
      CHECK(b.error().v == 5);
    }

    SECTION("with in_place")
    {
      using T = expected<helper, Error>;
      static_assert(std::is_constructible_v<T, std::in_place_t, int>);
      static_assert(not extension || not std::is_nothrow_constructible_v<T, std::in_place_t, int>);
      static_assert(std::is_constructible_v<T, std::in_place_t, helper>);
      static_assert(not extension || std::is_nothrow_constructible_v<T, std::in_place_t, helper>);
      static_assert(std::is_constructible_v<T, std::in_place_t, std::initializer_list<double>>);
      static_assert(not extension
                    || std::is_nothrow_constructible_v<T, std::in_place_t, std::initializer_list<double>>);

      T const b(std::in_place, 11, 13);
      CHECK(b.value().v == 11 * 13);

      T const c(std::in_place, {2.0, 3.0, 5.0});
      CHECK(c.value().v == 2 * 3 * 5.0);

      try {
        T const d(std::in_place, 1, 2, 0);
        FAIL();
      } catch (std::runtime_error const &e) {
        CHECK(std::strcmp(e.what(), "invalid input") == 0);
      }
    }

    SECTION("with unexpect")
    {
      using T = expected<int, helper>;
      static_assert(std::is_constructible_v<T, unexpect_t, int>);
      static_assert(not extension || not std::is_nothrow_constructible_v<T, unexpect_t, int>);
      static_assert(std::is_constructible_v<T, unexpect_t, helper>);
      static_assert(not extension || std::is_nothrow_constructible_v<T, unexpect_t, helper>);
      static_assert(std::is_constructible_v<T, unexpect_t, std::initializer_list<double>>);
      static_assert(not extension || std::is_nothrow_constructible_v<T, unexpect_t, std::initializer_list<double>>);

      T const b(unexpect, 11, 13);
      CHECK(b.error().v == 11 * 13);

      T const c(unexpect, {2.0, 3.0, 5.0});
      CHECK(c.error().v == 2 * 3 * 5.0);

      try {
        T const d(unexpect, 1, 2, 0);
        FAIL();
      } catch (std::runtime_error const &e) {
        CHECK(std::strcmp(e.what(), "invalid input") == 0);
      }
    }
  }

  SECTION("copy, move and dtor")
  {
    struct U {
      U() = default;
      U(U const &) = delete;
    };

    SECTION("unavailable")
    {
      static_assert(not std::is_copy_constructible_v<U>);           // prerequisite
      static_assert(not std::is_trivially_copy_constructible_v<U>); // prerequisite
      static_assert(not std::is_copy_constructible_v<expected<U, Error>>);
      static_assert(not std::is_copy_constructible_v<expected<int, U>>);
      static_assert(not std::is_move_constructible_v<expected<U, Error>>);
      static_assert(not std::is_move_constructible_v<expected<int, U>>);
      SUCCEED();
    }

    SECTION("trivial")
    {
      using T = expected<int, Error>;
      static_assert(std::is_copy_constructible_v<T>);
      static_assert(std::is_trivially_copy_constructible_v<T>);
      static_assert(not extension || std::is_nothrow_copy_constructible_v<T>);
      static_assert(std::is_move_constructible_v<T>);
      static_assert(std::is_trivially_move_constructible_v<T>);
      static_assert(not extension || std::is_nothrow_move_constructible_v<T>);
      static_assert(std::is_trivially_destructible_v<T>);
      static_assert(std::is_nothrow_destructible_v<T>);

      constexpr T a;
      constexpr T b = a;
      static_assert(b.has_value() && a.value() == b.value());

      {
        T a(std::in_place, 13);
        T b = a;
        CHECK(b.has_value());
        CHECK(b.value() == 13);

        T c = std::move(a);
        CHECK(c.has_value());
        CHECK(c.value() == 13);
      }
    }

    SECTION("non-trivial value type")
    {
      using T = expected<helper, Error>;
      static_assert(std::is_copy_constructible_v<T>);
      static_assert(not std::is_trivially_copy_constructible_v<T>);
      static_assert(not extension || std::is_nothrow_copy_constructible_v<T>);
      static_assert(std::is_move_constructible_v<T>);
      static_assert(not std::is_trivially_move_constructible_v<T>);
      static_assert(std::is_nothrow_move_constructible_v<T>); // required
      static_assert(not std::is_trivially_destructible_v<T>);
      static_assert(std::is_nothrow_destructible_v<T>);

      {
        T a(std::in_place, 13);
        T b = a; // no overload for lval
        CHECK(b.has_value());
        CHECK(b.value().v == 13 * helper::from_lval_const);

        T c = std::as_const(a);
        CHECK(b.has_value());
        CHECK(c.value().v == 13 * helper::from_lval_const);

        T d = std::move(std::as_const(a)); // no overload for lval const
        CHECK(b.has_value());
        CHECK(d.value().v == 13 * helper::from_lval_const);

        T e = std::move(a);
        CHECK(b.has_value());
        CHECK(e.value().v == 13 * helper::from_rval);
      }
    }

    SECTION("non-trivial error type")
    {
      using T = expected<int, helper>;
      static_assert(std::is_copy_constructible_v<T>);
      static_assert(not std::is_trivially_copy_constructible_v<T>);
      static_assert(not extension || std::is_nothrow_copy_constructible_v<T>);
      static_assert(std::is_move_constructible_v<T>);
      static_assert(not std::is_trivially_move_constructible_v<T>);
      static_assert(std::is_nothrow_move_constructible_v<T>); // required
      static_assert(not std::is_trivially_destructible_v<T>);
      static_assert(std::is_nothrow_destructible_v<T>);

      {
        T a(unexpect, 33);
        T b = a; // no overload for lval
        CHECK(not b.has_value());
        CHECK(b.error().v == 33 * helper::from_lval_const);

        T c = std::as_const(a);
        CHECK(not b.has_value());
        CHECK(c.error().v == 33 * helper::from_lval_const);

        T d = std::move(std::as_const(a)); // no overload for lval const
        CHECK(not b.has_value());
        CHECK(d.error().v == 33 * helper::from_lval_const);

        T e = std::move(a);
        CHECK(not b.has_value());
        CHECK(e.error().v == 33 * helper::from_rval);
      }
    }

    SECTION("non-trivial both")
    {
      using T = expected<helper, helper_t<1>>;
      static_assert(std::is_copy_constructible_v<T>);
      static_assert(not std::is_trivially_copy_constructible_v<T>);
      static_assert(not extension || std::is_nothrow_copy_constructible_v<T>);
      static_assert(std::is_move_constructible_v<T>);
      static_assert(not std::is_trivially_move_constructible_v<T>);
      static_assert(std::is_nothrow_move_constructible_v<T>); // required
      static_assert(not std::is_trivially_destructible_v<T>);
      static_assert(std::is_nothrow_destructible_v<T>);

      {
        T a(std::in_place, 41);
        T b = a; // no overload for lval
        CHECK(b.has_value());
        CHECK(b.value().v == 41 * helper::from_lval_const);
      }

      {
        T a(unexpect, 43);
        T b = a; // no overload for lval
        CHECK(not b.has_value());
        CHECK(b.error().v == 43 * helper::from_lval_const);
      }
    }

    struct B {
      int v;
      constexpr B(int v) : v(v) {}
      constexpr B(B const &s) noexcept(false) : v(s.v) {};
      constexpr B(B &&s) noexcept(false) : v(s.v) {};
    };

    SECTION("noexcept(false) from value type")
    {
      using T = expected<B, Error>;
      static_assert(std::is_copy_constructible_v<T>);
      static_assert(not std::is_trivially_copy_constructible_v<T>);
      static_assert(not extension || not std::is_nothrow_copy_constructible_v<T>);
      static_assert(std::is_move_constructible_v<T>);
      static_assert(not std::is_trivially_move_constructible_v<T>);
      static_assert(not std::is_nothrow_move_constructible_v<T>); // required
      static_assert(std::is_trivially_destructible_v<T>);
      static_assert(std::is_nothrow_destructible_v<T>);

      constexpr T a(std::in_place, 17);
      constexpr T b = a;
      static_assert(b.has_value() && a.value().v == b.value().v);

      {
        T const a(std::in_place, 19);
        T b = a;
        CHECK(b.has_value());
        CHECK(b.value().v == 19);

        T c = std::move(a);
        CHECK(b.has_value());
        CHECK(c.value().v == 19);
      }
    }

    SECTION("noexcept(false) from error type")
    {
      using T = expected<int, B>;
      static_assert(std::is_copy_constructible_v<T>);
      static_assert(not std::is_trivially_copy_constructible_v<T>);
      static_assert(not extension || not std::is_nothrow_copy_constructible_v<T>);
      static_assert(std::is_move_constructible_v<T>);
      static_assert(not std::is_trivially_move_constructible_v<T>);
      static_assert(not std::is_nothrow_move_constructible_v<T>); // required
      static_assert(std::is_trivially_destructible_v<T>);
      static_assert(std::is_nothrow_destructible_v<T>);

      constexpr T a(unexpect, 23);
      constexpr T b = a;
      static_assert(not b.has_value() && a.error().v == b.error().v);

      {
        T const a(unexpect, 29);
        T b = a;
        CHECK(not b.has_value());
        CHECK(b.error().v == 29);

        T c = std::move(a);
        CHECK(not c.has_value());
        CHECK(c.error().v == 29);
      }
    }

    struct C {
      constexpr C() noexcept(true) {};
      constexpr C(C const &) noexcept(true) {}; // WORKAROUND:MSVC
      constexpr ~C() noexcept(false) {};
    };

    SECTION("noexcept(false) dtor value type")
    {
      using T = expected<C, Error>;
      static_assert(std::is_copy_constructible_v<T>);
      static_assert(not std::is_trivially_copy_constructible_v<T>);
      static_assert(not extension || not std::is_nothrow_copy_constructible_v<T>);
      static_assert(std::is_move_constructible_v<T>);
      static_assert(not std::is_trivially_move_constructible_v<T>);
      static_assert(not std::is_nothrow_move_constructible_v<T>); // required
      static_assert(not std::is_trivially_destructible_v<T>);
      static_assert(not std::is_nothrow_destructible_v<T>);

      constexpr T a(std::in_place);
      constexpr T b = a;
      static_assert(b.has_value());

      {
        T const a(std::in_place);
        T b = a;
        CHECK(b.has_value());
      }
    }

    SECTION("noexcept(false) dtor error type")
    {
      using T = expected<int, C>;
      static_assert(std::is_copy_constructible_v<T>);
      static_assert(not std::is_trivially_copy_constructible_v<T>);
      static_assert(not extension || not std::is_nothrow_copy_constructible_v<T>);
      static_assert(std::is_move_constructible_v<T>);
      static_assert(not std::is_trivially_move_constructible_v<T>);
      static_assert(not std::is_nothrow_move_constructible_v<T>); // required
      static_assert(not std::is_trivially_destructible_v<T>);
      static_assert(not std::is_nothrow_destructible_v<T>);

      constexpr T a(unexpect);
      constexpr T b = a;
      static_assert(not b.has_value());

      {
        T const a(unexpect);
        T b = a;
        CHECK(not b.has_value());
      }
    }
  }

  SECTION("assign")
  {
    using M = helper_t<2>; // nothrow move constructible
    using E = helper_t<3>; // may throw on move and copy
    using C = helper_t<4>; // nothrow copy constructible
    static_assert(not std::is_nothrow_copy_constructible_v<M>);
    static_assert(std::is_nothrow_move_constructible_v<M>);
    static_assert(not std::is_nothrow_copy_constructible_v<E>);
    static_assert(not std::is_nothrow_move_constructible_v<E>);
    static_assert(std::is_nothrow_copy_constructible_v<C>);
    static_assert(not std::is_nothrow_move_constructible_v<C>);

    SECTION("from rval")
    {
      SECTION("value to value")
      {
        using T = expected<helper, Error>;

        {
          static_assert(std::is_nothrow_assignable_v<T &, T &&>); // required

          T a(std::in_place, 3);
          a = T(std::in_place, 5);
          CHECK(a.value().v == 5 * helper::from_rval);
        }

        {
          static_assert(not extension || std::is_nothrow_assignable_v<T &, helper &&>);

          T a(std::in_place, 3);
          a = helper(5);
          CHECK(a.value().v == 5 * helper::from_rval);
        }
      }

      SECTION("value to error")
      {
        SECTION("nothrow move")
        {
          using T = expected<int, M>;
          static_assert(std::is_nothrow_assignable_v<T &, T &&>); // required

          {
            T a(std::in_place, 3);
            a = T(unexpect, 5);
            CHECK(a.error().v == 5 * helper::from_rval);
          }

          {
            T a(std::in_place, 4);
            try {
              a = T(unexpect, {0.0});
              SUCCEED();
            } catch (std::runtime_error const &) {
              FAIL();
            }
          }

          {
            static_assert(not extension || std::is_nothrow_assignable_v<T &, unexpected<M> &&>);
            T a(std::in_place, 4);
            a = unexpected<M>(5);
            CHECK(a.error().v == 5 * helper::from_rval);
          }

          {
            T a(std::in_place, 4);
            try {
              a = unexpected<M>({0.0});
              SUCCEED();
            } catch (std::runtime_error const &e) {
              FAIL();
            }
          }
        }

        SECTION("throwing")
        {
          using T = expected<int, E>;
          static_assert(not std::is_nothrow_assignable_v<T &, T &&>); // required

          {
            T a(std::in_place, 3);
            a = T(unexpect, 5);
            CHECK(a.error().v == 5 * helper::from_rval);
          }

          {
            T a(std::in_place, 4);
            try {
              a = T(unexpect, {0.0});
              FAIL();
            } catch (std::runtime_error const &e) {
              CHECK(std::strcmp(e.what(), "invalid input") == 0);
              CHECK(a.value() == 4);
            }
          }

          {
            static_assert(not extension || not std::is_nothrow_assignable_v<T &, unexpected<E> &&>);
            T a(std::in_place, 4);
            a = unexpected<E>(5);
            CHECK(a.error().v == 5 * helper::from_rval);
          }

          {
            T a(std::in_place, 4);
            try {
              a = unexpected<E>({0.0});
              FAIL();
            } catch (std::runtime_error const &e) {
              CHECK(std::strcmp(e.what(), "invalid input") == 0);
              CHECK(a.value() == 4);
            }
          }
        }

        SECTION("nothrow copy")
        {
          using T = expected<int, C>;
          static_assert(not std::is_nothrow_assignable_v<T &, T &&>); // required

          {
            T a(std::in_place, 3);
            a = T(unexpect, 5);
            CHECK(a.error().v == 5 * helper::from_rval);
          }

          {
            T a(std::in_place, 4);
            try {
              a = T(unexpect, {0.0});
              FAIL();
            } catch (std::runtime_error const &e) {
              CHECK(std::strcmp(e.what(), "invalid input") == 0);
              CHECK(a.value() == 4);
            }
          }

          {
            static_assert(not extension || not std::is_nothrow_assignable_v<T &, unexpected<C> &&>);
            T a(std::in_place, 4);
            a = unexpected<C>(5);
            CHECK(a.error().v == 5 * helper::from_rval);
          }

          {
            T a(std::in_place, 4);
            try {
              a = unexpected<C>({0.0});
              FAIL();
            } catch (std::runtime_error const &e) {
              CHECK(std::strcmp(e.what(), "invalid input") == 0);
              CHECK(a.value() == 4);
            }
          }
        }
      }

      SECTION("error to value")
      {
        SECTION("nothrow move")
        {
          using T = expected<M, Error>;
          static_assert(std::is_nothrow_assignable_v<T &, T &&>); // required

          {
            T a(unexpect, Error::file_not_found);
            a = T(std::in_place, 5);
            CHECK(a.value().v == 5 * helper::from_rval);
          }

          {
            T a(unexpect, Error::file_not_found);
            try {
              a = T(std::in_place, {0.0});
              // expected<M, ...> must not use throwing copy constructor
              SUCCEED();
            } catch (std::runtime_error const &e) {
              FAIL();
            }
          }

          {
            static_assert(not extension || std::is_nothrow_assignable_v<T &, M>);
            T a(unexpect, Error::file_not_found);
            a = M(5);
            CHECK(a.value().v == 5 * helper::from_rval);
          }

          {
            T a(unexpect, Error::file_not_found);
            try {
              a = M({0.0});
              SUCCEED();
            } catch (std::runtime_error const &e) {
              FAIL();
            }
          }
        }

        SECTION("throwing")
        {
          using T = expected<E, Error>;
          static_assert(not std::is_nothrow_assignable_v<T &, T &&>); // required

          {
            T a(unexpect, Error::file_not_found);
            a = T(std::in_place, 5);
            CHECK(a.value().v == 5 * helper::from_rval);
          }

          {
            T a(unexpect, Error::file_not_found);
            try {
              a = T(std::in_place, {0.0});
              FAIL();
            } catch (std::runtime_error const &e) {
              CHECK(std::strcmp(e.what(), "invalid input") == 0);
              CHECK(a.error() == Error::file_not_found);
            }
          }

          {
            static_assert(not extension || not std::is_nothrow_assignable_v<T &, E>);
            T a(unexpect, Error::file_not_found);
            a = E(5);
            CHECK(a.value().v == 5 * helper::from_rval);
          }

          {
            T a(unexpect, Error::file_not_found);
            try {
              a = E({0.0});
              FAIL();
            } catch (std::runtime_error const &e) {
              CHECK(std::strcmp(e.what(), "invalid input") == 0);
              CHECK(a.error() == Error::file_not_found);
            }
          }
        }

        SECTION("nothrow copy")
        {
          using T = expected<C, Error>;
          static_assert(not std::is_nothrow_assignable_v<T &, T &&>); // required

          {
            T a(unexpect, Error::file_not_found);
            a = T(std::in_place, 5);
            CHECK(a.value().v == 5 * helper::from_rval);
          }

          {
            T a(unexpect, Error::file_not_found);
            try {
              a = T(std::in_place, {0.0});
              FAIL();
            } catch (std::runtime_error const &e) {
              CHECK(std::strcmp(e.what(), "invalid input") == 0);
              CHECK(a.error() == Error::file_not_found);
            }
          }

          {
            static_assert(not extension || not std::is_nothrow_assignable_v<T &, C>);
            T a(unexpect, Error::file_not_found);
            a = C(5);
            CHECK(a.value().v == 5 * helper::from_rval);
          }

          {
            T a(unexpect, Error::file_not_found);
            try {
              a = C({0.0});
              FAIL();
            } catch (std::runtime_error const &e) {
              CHECK(std::strcmp(e.what(), "invalid input") == 0);
              CHECK(a.error() == Error::file_not_found);
            }
          }
        }
      }

      SECTION("error to error")
      {
        using T = expected<int, helper>;
        static_assert(std::is_nothrow_assignable_v<T &, T &&>); // required

        T a(unexpect, 3);
        a = T(unexpect, 5);
        CHECK(a.error().v == 5 * helper::from_rval);

        a = unexpected<helper>(7);
        CHECK(a.error().v == 7 * helper::from_rval);
      }
    }

    SECTION("from lval const")
    {
      SECTION("value to value")
      {
        using T = expected<helper, Error>;
        {
          static_assert(not extension || std::is_nothrow_assignable_v<T &, T const &>);

          T a(std::in_place, 3);
          T const b(std::in_place, 5);
          a = b;
          CHECK(a.value().v == 5 * helper::from_lval_const);

          T c(std::in_place, 7);
          a = c;
          CHECK(a.value().v == 7 * helper::from_lval_const);

          T const d(std::in_place, 11);
          a = std::move(d);
          CHECK(a.value().v == 11 * helper::from_lval_const);
        }

        {
          static_assert(not extension || std::is_nothrow_assignable_v<T &, helper const &>);

          T a(std::in_place, 3);
          helper const b(5);
          a = b;
          CHECK(a.value().v == 5 * helper::from_lval_const);

          helper c(7);
          a = c;
          CHECK(a.value().v == 7 * helper::from_lval);

          helper const d(11);
          a = std::move(d);
          CHECK(a.value().v == 11 * helper::from_rval_const);
        }
      }

      SECTION("value to error")
      {
        SECTION("nothrow move")
        {
          using T = expected<int, M>;
          static_assert(not extension || not std::is_nothrow_assignable_v<T &, T const &>);

          {
            T a(std::in_place, 3);
            T const b(unexpect, 5);
            a = b;
            CHECK(a.error().v == 5 * helper::from_lval_const * helper::from_rval);
          }

          {
            T a(std::in_place, 4);
            try {
              T const b(unexpect, {0.0});
              a = b; // copy construction on a side of `New tmp` will throw
              FAIL();
            } catch (std::runtime_error const &e) {
              CHECK(std::strcmp(e.what(), "invalid input") == 0);
              CHECK(a.value() == 4);
            }
          }

          {
            static_assert(not extension || not std::is_nothrow_assignable_v<T &, unexpected<M> const &>);
            T a(std::in_place, 4);
            unexpected<M> const b(5);
            a = b;
            CHECK(a.error().v == 5 * helper::from_lval_const * helper::from_rval);
          }

          {
            T a(std::in_place, 4);
            try {
              unexpected<M> const b({0.0});
              a = b;
              FAIL();
            } catch (std::runtime_error const &e) {
              CHECK(std::strcmp(e.what(), "invalid input") == 0);
              CHECK(a.value() == 4);
            }
          }
        }

        SECTION("throwing")
        {
          using T = expected<int, E>;
          static_assert(not extension || not std::is_nothrow_assignable_v<T &, T const &>);

          {
            T a(std::in_place, 3);
            T const b(unexpect, 5);
            a = b;
            CHECK(a.error().v == 5 * helper::from_lval_const);
          }

          {
            T a(std::in_place, 4);
            try {
              T const b(unexpect, {0.0});
              a = b;
              FAIL();
            } catch (std::runtime_error const &e) {
              CHECK(std::strcmp(e.what(), "invalid input") == 0);
              CHECK(a.value() == 4);
            }
          }

          {
            static_assert(not extension || not std::is_nothrow_assignable_v<T &, unexpected<E> const &>);
            T a(std::in_place, 4);
            unexpected<E> const b(5);
            a = b;
            CHECK(a.error().v == 5 * helper::from_lval_const);
          }

          {
            T a(std::in_place, 4);
            try {
              unexpected<E> const b({0.0});
              a = b;
              FAIL();
            } catch (std::runtime_error const &e) {
              CHECK(std::strcmp(e.what(), "invalid input") == 0);
              CHECK(a.value() == 4);
            }
          }
        }

        SECTION("nothrow copy")
        {
          using T = expected<int, C>;
          static_assert(not extension || std::is_nothrow_assignable_v<T &, T const &>);

          {
            T a(std::in_place, 3);
            T const b(unexpect, 5);
            a = b;
            CHECK(a.error().v == 5 * helper::from_lval_const);
          }

          {
            T a(std::in_place, 4);
            try {
              T const b(unexpect, {0.0});
              a = b;
              SUCCEED();
            } catch (std::runtime_error const &e) {
              FAIL();
            }
          }

          {
            static_assert(not extension || std::is_nothrow_assignable_v<T &, unexpected<C> const &>);
            T a(std::in_place, 4);
            unexpected<C> const b(5);
            a = b;
            CHECK(a.error().v == 5 * helper::from_lval_const);
          }

          {
            T a(std::in_place, 4);
            try {
              unexpected<C> b(std::in_place, {0.0});
              a = b;
              SUCCEED();
            } catch (std::runtime_error const &e) {
              FAIL();
            }
          }
        }
      }

      SECTION("error to value")
      {
        SECTION("nothrow move")
        {
          using T = expected<M, Error>;
          static_assert(not extension || not std::is_nothrow_assignable_v<T &, T const &>);

          {
            T a(unexpect, Error::file_not_found);
            T const b(std::in_place, 5);
            a = b;
            CHECK(a.value().v == 5 * helper::from_lval_const * helper::from_rval);
          }

          {
            T a(unexpect, Error::file_not_found);
            try {
              T const b(std::in_place, {0.0});
              a = b; // copy construction on a side of `New tmp` will throw
              FAIL();
            } catch (std::runtime_error const &e) {
              CHECK(std::strcmp(e.what(), "invalid input") == 0);
              CHECK(a.error() == Error::file_not_found);
            }
          }

          {
            static_assert(not extension || not std::is_nothrow_assignable_v<T &, M const &>);
            T a(unexpect, Error::file_not_found);
            M const b(5);
            a = b;
            CHECK(a.value().v == 5 * helper::from_lval_const * helper::from_rval);
          }

          {
            T a(unexpect, Error::file_not_found);
            try {
              M const b({0.0});
              a = b; // copy construction on a side of `New tmp` will throw
              FAIL();
            } catch (std::runtime_error const &e) {
              CHECK(std::strcmp(e.what(), "invalid input") == 0);
              CHECK(a.error() == Error::file_not_found);
            }
          }
        }

        SECTION("throwing")
        {
          using T = expected<E, Error>;
          static_assert(not extension || not std::is_nothrow_assignable_v<T &, T const &>);

          {
            T a(unexpect, Error::file_not_found);
            T const b(std::in_place, 5);
            a = b;
            CHECK(a.value().v == 5 * helper::from_lval_const);
          }

          {
            T a(unexpect, Error::file_not_found);
            try {
              T const b(std::in_place, {0.0});
              a = b;
              FAIL();
            } catch (std::runtime_error const &e) {
              CHECK(std::strcmp(e.what(), "invalid input") == 0);
              CHECK(a.error() == Error::file_not_found);
            }
          }

          {
            static_assert(not extension || not std::is_nothrow_assignable_v<T &, E const &>);
            T a(unexpect, Error::file_not_found);
            E const b(5);
            a = b;
            CHECK(a.value().v == 5 * helper::from_lval_const);
          }

          {
            T a(unexpect, Error::file_not_found);
            try {
              E const b({0.0});
              a = b;
              FAIL();
            } catch (std::runtime_error const &e) {
              CHECK(std::strcmp(e.what(), "invalid input") == 0);
              CHECK(a.error() == Error::file_not_found);
            }
          }
        }

        SECTION("nothrow copy")
        {
          using T = expected<C, Error>;
          static_assert(not extension || std::is_nothrow_assignable_v<T &, T const &>);

          {
            T a(unexpect, Error::file_not_found);
            T const b(std::in_place, 5);
            a = b;
            CHECK(a.value().v == 5 * helper::from_lval_const);
          }

          {
            T a(unexpect, Error::file_not_found);
            try {
              T const b(std::in_place, {0.0});
              a = b;
              SUCCEED();
            } catch (std::runtime_error const &e) {
              FAIL();
            }
          }

          {
            static_assert(not extension || std::is_nothrow_assignable_v<T &, C const &>);
            T a(unexpect, Error::file_not_found);
            C const b(5);
            a = b;
            CHECK(a.value().v == 5 * helper::from_lval_const);
          }

          {
            T a(unexpect, Error::file_not_found);
            try {
              C const b({0.0});
              a = b;
              SUCCEED();
            } catch (std::runtime_error const &e) {
              FAIL();
            }
          }
        }
      }

      SECTION("error to error")
      {
        using T = expected<int, helper>;
        static_assert(not extension || std::is_nothrow_assignable_v<T &, T const &>);

        T a(unexpect, 3);
        T const b(unexpect, 5);
        a = b;
        CHECK(a.error().v == 5 * helper::from_lval_const);

        unexpected<helper> const c(7);
        a = c;
        CHECK(a.error().v == 7 * helper::from_lval_const);
      }
    }
  }

  SECTION("accessors")
  {
    SECTION("value")
    {
      using T = expected<helper, Error>;

      T a = {11};
      CHECK(a.value().v == 11);
      CHECK(std::as_const(a).value().v == 11);
      CHECK(std::move(std::as_const(a)).value().v == 11);
      CHECK(std::move(a).value().v == 11);

      {
        helper b{1};
        CHECK((b = a.value()).v == 11 * helper::from_lval);
        CHECK((b = std::as_const(a).value()).v == 11 * helper::from_lval_const);
        CHECK((b = std::move(std::as_const(a)).value()).v == 11 * helper::from_rval_const);
        CHECK((b = std::move(a).value()).v == 11 * helper::from_rval);
      }

      {
        T a{unexpect, Error::file_not_found};

        try {
          auto _ = a.value();
          FAIL();
        } catch (bad_expected_access<Error> const &e) {
          CHECK(e.error() == Error::file_not_found);
        }

        try {
          auto _ = std::as_const(a).value();
          FAIL();
        } catch (bad_expected_access<Error> const &e) {
          CHECK(e.error() == Error::file_not_found);
        }

        try {
          auto _ = std::move(std::as_const(a)).value();
          FAIL();
        } catch (bad_expected_access<Error> const &e) {
          CHECK(e.error() == Error::file_not_found);
        }

        try {
          auto _ = std::move(a).value();
          FAIL();
        } catch (bad_expected_access<Error> const &e) {
          CHECK(e.error() == Error::file_not_found);
        }
      }
    }

    SECTION("error")
    {
      using T = expected<int, helper>;

      T a{unexpect, 17};
      CHECK(a.error().v == 17);
      CHECK(std::as_const(a).error().v == 17);
      CHECK(std::move(std::as_const(a)).error().v == 17);
      CHECK(std::move(a).error().v == 17);

      {
        helper b{1};
        CHECK((b = a.error()).v == 17 * helper::from_lval);
        CHECK((b = std::as_const(a).error()).v == 17 * helper::from_lval_const);
        CHECK((b = std::move(std::as_const(a)).error()).v == 17 * helper::from_rval_const);
        CHECK((b = std::move(a).error()).v == 17 * helper::from_rval);
      }
    }
  }
}
