// Copyright (c) 2025 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <pfn/expected.hpp>

#include <util/helper_types.hpp>

#include <catch2/catch_all.hpp>

#include <stdexcept>
#include <type_traits>

enum Error { unknown, secret = 142, mystery = 176 };

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

    T a{12};
    static_assert(noexcept(T{a}));
    static_assert(noexcept(T{std::move(a)}));
    static_assert(noexcept(a.what()));
    static_assert(std::is_same_v<decltype(a.what()), char const *>);
    static_assert(std::is_same_v<decltype(a.error()), helper &>);
    static_assert(std::is_same_v<decltype(std::move(a).error()), helper &&>);

    SECTION("copy/move constructors")
    {
      T b{0};

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
      T b{0};

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
      helper c{0};
      T b{0};

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

    CHECK(std::strcmp(a.what(), "bad access to expected without expected value") == 0);
    auto const c = []() {
      struct C : pfn::bad_expected_access<void> {};
      return C{};
    }();
    CHECK(a.what() == c.what());
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
    SECTION("constexpr, CTAD")
    {
      constexpr unexpected c{Error::mystery};
      static_assert(c.error() == Error::mystery);
      static_assert(std::is_same_v<decltype(c), unexpected<Error> const>);
      static_assert(std::is_nothrow_constructible_v<decltype(c), Error>);
      SUCCEED();
    }

    SECTION("constexpr, no CTAD")
    {
      constexpr unexpected<int> c{42};
      static_assert(c.error() == 42);
      static_assert(std::is_nothrow_constructible_v<decltype(c), int>);
      SUCCEED();
    }

    SECTION("no conversion, CTAD")
    {
      unexpected c{helper{2}};
      CHECK(c.error().v == 2 * helper::from_rval);
      static_assert(std::is_same_v<decltype(c), unexpected<helper>>);
      static_assert(std::is_nothrow_constructible_v<decltype(c), helper>);
    }

    SECTION("conversion, no CTAD")
    {
      unexpected<helper> c(3);
      CHECK(c.error().v == 3);
      static_assert(std::is_nothrow_constructible_v<decltype(c), int>);
    }

    SECTION("in-place, no CTAD")
    {
      unexpected<helper> c(std::in_place, 3, 5);
      CHECK(c.error().v == 3 * 5);
      static_assert(std::is_nothrow_constructible_v<decltype(c), std::in_place_t, int, int>);
    }

    SECTION("in_place, not CTAD, initializer_list, noexcept(false)")
    {
      SECTION("forwarded args")
      {
        unexpected<helper> c(std::in_place, {3.0, 5.0}, 7, 11);
        auto const d = 3 * 5 * 7 * 11;
        CHECK(c.error().v == d);
        static_assert(
            not std::is_nothrow_constructible_v<decltype(c), std::in_place_t, std::initializer_list<double>, int, int>);
        static_assert(std::is_constructible_v<decltype(c), std::in_place_t, std::initializer_list<double>, int, int>);
      }

      SECTION("no forwarded args")
      {
        unexpected<helper> c(std::in_place, {2.0, 2.5});
        CHECK(c.error().v == 5);
        static_assert(not std::is_nothrow_constructible_v<decltype(c), std::in_place_t, std::initializer_list<double>>);
        static_assert(std::is_constructible_v<decltype(c), std::in_place_t, std::initializer_list<double>>);
      }

      SECTION("exception thrown")
      {
        unexpected<helper> t{13};
        try {
          t = unexpected<helper>{std::in_place, {2.0, 1.0, 0.0}, 5};
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
    unexpected<helper> a{0};

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
    unexpected<helper> a{0};
    a.error().v = 2;
    unexpected b{helper{0}};
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
      unexpected c{0};
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
