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

TEST_CASE("unexpect", "[expected][polyfill][unexpect]")
{
  static_assert(std::is_empty_v<pfn::unexpect_t>);
  static_assert(noexcept(pfn::unexpect_t{}));
  static_assert(std::is_same_v<decltype(pfn::unexpect), pfn::unexpect_t const>);

  // pfn::unexpect can be used as a NTTP
  static_assert(not std::is_empty_v<helper_t<pfn::unexpect>>);
  static constexpr auto a = pfn::unexpect;
  static_assert(not std::is_empty_v<helper_t<a>>);
  static_assert(std::is_same_v<decltype(a), pfn::unexpect_t const>);
  static_assert(std::is_same_v<helper_t<pfn::unexpect>, helper_t<a>>);

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
      static_assert(extension && std::is_nothrow_constructible_v<T>);

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
      static_assert(extension && std::is_nothrow_constructible_v<T>);

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
      static_assert(not std::is_nothrow_constructible_v<T>);

      constexpr T a;
      static_assert(a.has_value());
      static_assert(a.value().v == 42);

      T b;
      CHECK(b.has_value());
      CHECK(b.value().v == 42);
    }

    SECTION("ignore noexcept from error type")
    {
      using T = expected<int, B>;
      static_assert(std::is_default_constructible_v<T>);
      static_assert(extension && std::is_nothrow_constructible_v<T>);
      SUCCEED();
    }

    struct C {
      C() noexcept(false) { throw 7; }
    };

    SECTION("exception thrown")
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
      static_assert(extension && std::is_nothrow_constructible_v<T>);

      T b;
      CHECK(b.has_value());
      CHECK(b.value() == 0);
    }

    SECTION("from other expected rval")
    {
      using T = expected<helper, Error>;
      static_assert(std::is_constructible_v<T, expected<int, Error>>);
      static_assert(std::is_constructible_v<T, expected<std::initializer_list<double>, Error>>);
      static_assert(not std::is_nothrow_constructible_v<T, expected<int, Error>>);                       // extension
      static_assert(std::is_nothrow_constructible_v<T, expected<std::initializer_list<double>, Error>>); // extension

      constexpr T b(expected<int, Error>(unexpect, Error::unknown));
      static_assert(b.error() == Error::unknown);

      T c(expected<int, Error>(3));
      CHECK(c.value().v == 3);
      T d(expected<int, Error>(unexpect, Error::file_not_found));
      CHECK(d.error() == Error::file_not_found);
    }

    SECTION("from other expected lval const")
    {
      using T = expected<helper, Error>;
      static_assert(std::is_constructible_v<T, expected<int, Error> const &>);
      static_assert(not std::is_nothrow_constructible_v<T, expected<int, Error> const &>); // extension
      static_assert(
          std::is_nothrow_constructible_v<T, expected<std::initializer_list<double>, Error> const &>); // extension

      constexpr expected<int, Error> v(5);
      constexpr expected<int, Error> e(unexpect, Error::file_not_found);
      constexpr T b(e);
      static_assert(b.error() == Error::file_not_found);

      T c(v);
      CHECK(c.value().v == 5);
      T d(e);
      CHECK(d.error() == Error::file_not_found);
    }

    SECTION("converting")
    {
      using T = expected<helper, Error>;
      static_assert(std::is_constructible_v<T, int>);
      static_assert(not std::is_nothrow_constructible_v<T, int>); // extension
      static_assert(std::is_constructible_v<T, helper>);
      static_assert(std::is_nothrow_constructible_v<T, helper>); // extension
      static_assert(std::is_constructible_v<T, std::initializer_list<double>>);
      static_assert(std::is_nothrow_constructible_v<T, std::initializer_list<double>>); // extension

      T const b(11);
      CHECK(b.value().v == 11);

      T const c(helper(13));
      CHECK(c.value().v == 13 * helper::from_rval);
    }

    SECTION("with in_place")
    {
      using T = expected<helper, Error>;
      static_assert(std::is_constructible_v<T, std::in_place_t, int>);
      static_assert(not std::is_nothrow_constructible_v<T, std::in_place_t, int>); // extension
      static_assert(std::is_constructible_v<T, std::in_place_t, helper>);
      static_assert(std::is_nothrow_constructible_v<T, std::in_place_t, helper>); // extension
      static_assert(std::is_constructible_v<T, std::in_place_t, std::initializer_list<double>>);
      static_assert(std::is_nothrow_constructible_v<T, std::in_place_t, std::initializer_list<double>>); // extension

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
      static_assert(not std::is_nothrow_constructible_v<T, unexpect_t, int>); // extension
      static_assert(std::is_constructible_v<T, unexpect_t, helper>);
      static_assert(std::is_nothrow_constructible_v<T, unexpect_t, helper>); // extension
      static_assert(std::is_constructible_v<T, unexpect_t, std::initializer_list<double>>);
      static_assert(std::is_nothrow_constructible_v<T, unexpect_t, std::initializer_list<double>>); // extension

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
      static_assert(extension && std::is_nothrow_copy_constructible_v<T>);
      static_assert(std::is_move_constructible_v<T>);
      static_assert(std::is_trivially_move_constructible_v<T>);
      static_assert(extension && std::is_nothrow_move_constructible_v<T>);
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
      static_assert(extension && std::is_nothrow_copy_constructible_v<T>);
      static_assert(std::is_move_constructible_v<T>);
      static_assert(not std::is_trivially_move_constructible_v<T>);
      static_assert(extension && std::is_nothrow_move_constructible_v<T>);
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
      static_assert(extension && std::is_nothrow_copy_constructible_v<T>);
      static_assert(std::is_move_constructible_v<T>);
      static_assert(not std::is_trivially_move_constructible_v<T>);
      static_assert(extension && std::is_nothrow_move_constructible_v<T>);
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
      static_assert(extension && std::is_nothrow_copy_constructible_v<T>);
      static_assert(std::is_move_constructible_v<T>);
      static_assert(not std::is_trivially_move_constructible_v<T>);
      static_assert(extension && std::is_nothrow_move_constructible_v<T>);
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
      static_assert(extension && not std::is_nothrow_copy_constructible_v<T>);
      static_assert(std::is_move_constructible_v<T>);
      static_assert(not std::is_trivially_move_constructible_v<T>);
      static_assert(extension && not std::is_nothrow_move_constructible_v<T>);
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
      static_assert(extension && not std::is_nothrow_copy_constructible_v<T>);
      static_assert(std::is_move_constructible_v<T>);
      static_assert(not std::is_trivially_move_constructible_v<T>);
      static_assert(extension && not std::is_nothrow_move_constructible_v<T>);
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
      static_assert(extension && not std::is_nothrow_copy_constructible_v<T>);
      static_assert(std::is_move_constructible_v<T>);
      static_assert(not std::is_trivially_move_constructible_v<T>);
      static_assert(extension && not std::is_nothrow_move_constructible_v<T>);
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
      static_assert(extension && not std::is_nothrow_copy_constructible_v<T>);
      static_assert(std::is_move_constructible_v<T>);
      static_assert(not std::is_trivially_move_constructible_v<T>);
      static_assert(extension && not std::is_nothrow_move_constructible_v<T>);
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
