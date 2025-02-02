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
      SECTION("lval")
      {
        auto const before = helper::witness;
        T c = a;
        CHECK(helper::witness == before * helper::from_lval_const);
        CHECK(c.error() == 12);
      }

      SECTION("lval const")
      {
        T const b{13};
        auto const before = helper::witness;
        T c = b;
        CHECK(helper::witness == before * helper::from_lval_const);
        CHECK(c.error() == 13);
      }

      SECTION("rval")
      {
        auto const before = helper::witness;
        T c = std::move(a);
        CHECK(helper::witness == before * helper::from_rval);
        CHECK(c.error() == 12);
      }

      SECTION("rval cont")
      {
        T const b{17};
        auto const before = helper::witness;
        T c = std::move(b);
        CHECK(helper::witness == before * helper::from_lval_const);
        CHECK(c.error() == 17);
      }
    }

    SECTION("assignment")
    {
      SECTION("lval")
      {
        T b{11};
        auto const before = helper::witness;
        a = b;
        CHECK(helper::witness == before * helper::from_lval_const);
        CHECK(a.error() == 11);
      }

      SECTION("lval const")
      {
        T const b{11};
        auto const before = helper::witness;
        a = b;
        CHECK(helper::witness == before * helper::from_lval_const);
        CHECK(a.error() == 11);
      }

      SECTION("rval")
      {
        T b{11};
        auto const before = helper::witness;
        a = std::move(b);
        CHECK(helper::witness == before * helper::from_rval);
        CHECK(a.error() == 11);
      }

      SECTION("rval const")
      {
        T const b{11};
        auto const before = helper::witness;
        a = std::move(b);
        CHECK(helper::witness == before * helper::from_lval_const);
        CHECK(a.error() == 11);
      }
    }

    SECTION("accessors")
    {
      helper c{0};

      SECTION("lval")
      {
        T b{11};
        auto const before = helper::witness;
        c = b.error();
        CHECK(helper::witness == before * helper::from_lval);
        CHECK(c == 11);
      }

      SECTION("lval const")
      {
        T const b{13};
        auto const before = helper::witness;
        c = b.error();
        CHECK(helper::witness == before * helper::from_lval_const);
        CHECK(c == 13);
      }

      SECTION("rval")
      {
        T b{17};
        auto const before = helper::witness;
        c = std::move(b).error();
        CHECK(helper::witness == before * helper::from_rval);
        CHECK(c == 17);
      }

      SECTION("rval const")
      {
        T const b{19};
        auto const before = helper::witness;
        c = std::move(b).error();
        CHECK(helper::witness == before * helper::from_rval_const);
        CHECK(c == 19);
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
      auto const before = helper::witness;
      unexpected c{helper{2}};
      CHECK(helper::witness == (before + 2) * helper::from_rval);
      CHECK(c.error() == helper{2});
      CHECK(c == unexpected<helper>{2});
      static_assert(std::is_same_v<decltype(c), unexpected<helper>>);
      static_assert(std::is_nothrow_constructible_v<decltype(c), helper>);
    }

    SECTION("conversion, no CTAD")
    {
      auto const before = helper::witness;
      unexpected<helper> c(3);
      CHECK(helper::witness == before + 3);
      CHECK(c.error().v == 3);
      CHECK(c == unexpected<helper>{3});
      static_assert(std::is_nothrow_constructible_v<decltype(c), int>);
    }

    SECTION("in-place, no CTAD")
    {
      auto const before = helper::witness;
      unexpected<helper> c(std::in_place, 3, 5);
      CHECK(helper::witness == before + 3 * 5);
      CHECK(c.error() == helper{3, 5});
      CHECK(c == unexpected<helper>{15});
      static_assert(std::is_nothrow_constructible_v<decltype(c), std::in_place_t, int, int>);
    }

    SECTION("in_place, not CTAD, initializer_list, noexcept(false)")
    {
      SECTION("forwarded args")
      {
        auto const before = helper::witness;
        unexpected<helper> c(std::in_place, {3.0, 5.0}, 7, 11);
        auto const d = 3 * 5 * 7 * 11;
        CHECK(helper::witness == before + d);
        CHECK(c.error() == helper{d});
        CHECK(c == unexpected{helper{d}});
        static_assert(
            not std::is_nothrow_constructible_v<decltype(c), std::in_place_t, std::initializer_list<double>, int, int>);
        static_assert(std::is_constructible_v<decltype(c), std::in_place_t, std::initializer_list<double>, int, int>);
      }

      SECTION("no forwarded args")
      {
        auto const before = helper::witness;
        unexpected<helper> c(std::in_place, {2.0, 2.5});
        CHECK(helper::witness == before + 5);
        CHECK(c.error() == helper{5});
        CHECK(c == unexpected<helper>{5});
        static_assert(not std::is_nothrow_constructible_v<decltype(c), std::in_place_t, std::initializer_list<double>>);
        static_assert(std::is_constructible_v<decltype(c), std::in_place_t, std::initializer_list<double>>);
      }

      SECTION("exception thrown")
      {
        unexpected<helper> t{13};
        auto const before = helper::witness;
        try {
          t = unexpected<helper>{std::in_place, {2.0, 1.0, 0.0}, 5};
          FAIL();
        } catch (std::runtime_error const &) {
          SUCCEED();
        }
        CHECK(t.error().v == 13);
        CHECK(helper::witness == before);
      }
    }
  }

  SECTION("accessors")
  {
    helper v{1};

    SECTION("lval")
    {
      unexpected<helper> t{13};
      auto before = helper::witness;
      v = t.error();
      CHECK(helper::witness == before * helper::from_lval);
      CHECK(v == helper{13});
    }

    SECTION("lval const")
    {
      unexpected<helper> const t{17};
      auto before = helper::witness;
      v = t.error();
      CHECK(helper::witness == before * helper::from_lval_const);
      CHECK(v == helper{17});
    }

    SECTION("rval")
    {
      unexpected<helper> t{19};
      auto before = helper::witness;
      v = std::move(t).error();
      CHECK(helper::witness == before * helper::from_rval);
      CHECK(v == helper{19});
    }

    SECTION("rval const")
    {
      unexpected<helper> const t{23};
      auto before = helper::witness;
      v = std::move(t).error();
      CHECK(helper::witness == before * helper::from_rval_const);
      CHECK(v == helper{23});
    }
  }

  SECTION("assignment")
  {
    unexpected<helper> v{0};

    SECTION("lval")
    {
      unexpected<helper> t{13};
      auto before = helper::witness;
      v = t;
      CHECK(helper::witness == before * helper::from_lval_const);
      CHECK(v.error() == helper{13});
    }

    SECTION("lval const")
    {
      unexpected<helper> const t{17};
      auto before = helper::witness;
      v = t;
      CHECK(helper::witness == before * helper::from_lval_const);
      CHECK(v.error() == helper{17});
    }

    SECTION("rval")
    {
      unexpected<helper> t{19};
      auto before = helper::witness;
      v = std::move(t);
      CHECK(helper::witness == before * helper::from_rval);
      CHECK(v.error() == helper{19});
    }

    SECTION("rval const")
    {
      unexpected<helper> const t{23};
      auto before = helper::witness;
      v = std::move(t);
      CHECK(helper::witness == before * helper::from_lval_const);
      CHECK(v.error() == helper{23});
    }
  }

  SECTION("swap")
  {
    unexpected<helper> v{2};
    unexpected w{helper{3}};
    auto before = helper::witness;
    v.swap(w);
    CHECK(helper::witness == before * helper::swapped);
    CHECK(v == unexpected{helper{3}});
    CHECK(w == unexpected{helper{2}});
    w.error() = helper{11};
    before = helper::witness;
    swap(v, w);
    CHECK(v.error() == helper{11});
    CHECK(w.error() == helper(3));
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
