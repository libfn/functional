// Copyright (c) 2025 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <pfn/expected.hpp>

#include <catch2/catch_all.hpp>

#include <stdexcept>
#include <type_traits>

enum Error { unknown, secret = 142, mystery = 176 };

TEST_CASE("bad_expected_access", "[expected][polyfill][bad_expected_access]")
{
  SECTION("bad_expected_access<void>")
  {
    struct A : pfn::bad_expected_access<void> {};

    static_assert(noexcept(A{}));
    A a;
    static_assert(noexcept(A{a}));
    static_assert(noexcept(A{std::move(a)}));
    static_assert(noexcept(a.what()));
    static_assert(std::is_same_v<decltype(a.what()), char const *>);
    SECTION("constructors and assignment")
    {
      A a1 = [&]() -> A & { return a; }();
      CHECK(a.what() == a1.what());
      A a2 = [&]() -> A && { return std::move(a); }();
      CHECK(a.what() == a2.what());
      A a3 = [&]() -> A const & { return a; }();
      CHECK(a.what() == a3.what());
      A a4 = [&]() -> A const && { return std::move(a); }();
      CHECK(a.what() == a4.what());

      a = [&]() -> A & { return a; }();
      CHECK(A{}.what() == a.what());
      a = [&]() -> A && { return std::move(a); }();
      CHECK(A{}.what() == a.what());
      a = [&]() -> A const & { return a; }();
      CHECK(A{}.what() == a.what());
      a = [&]() -> A const && { return std::move(a); }();
      CHECK(A{}.what() == a.what());
    }
    CHECK(std::strcmp(a.what(), "bad access to expected without expected value") == 0);

    A const b;
    CHECK(&decltype(a)::what == &decltype(b)::what);
    CHECK(a.what() == b.what());

    static_assert(noexcept(A{b}));
    static_assert(noexcept(A{std::move(b)}));
    static_assert(noexcept(b.what()));
    static_assert(std::is_same_v<decltype(b.what()), char const *>);
  }

  SECTION("bad_expected_access<E>")
  {
    using A = pfn::bad_expected_access<Error>;
    static_assert(std::is_base_of_v<pfn::bad_expected_access<void>, A>);

    A a{secret};
    static_assert(noexcept(A{a}));
    static_assert(noexcept(A{std::move(a)}));
    static_assert(noexcept(a.what()));
    SECTION("constructors and assignment")
    {
      A a1 = [&]() -> A & { return a; }();
      CHECK(a1.error() == secret);
      A a2 = [&]() -> A && { return std::move(a); }();
      CHECK(a2.error() == secret);
      A a3 = [&]() -> A const & { return a; }();
      CHECK(a3.error() == secret);
      A a4 = [&]() -> A const && { return std::move(a); }();
      CHECK(a4.error() == secret);

      a = [&]() -> A & { return a; }();
      CHECK(a.error() == secret);
      a = [&]() -> A && { return std::move(a); }();
      CHECK(a.error() == secret);
      a = [&]() -> A const & { return a; }();
      CHECK(a.error() == secret);
      a = [&]() -> A const && { return std::move(a); }();
      CHECK(a.error() == secret);
    }
    static_assert(std::is_same_v<decltype(a.what()), char const *>);
    static_assert(std::is_same_v<decltype(a.error()), Error &>);
    static_assert(std::is_same_v<decltype(std::move(a).error()), Error &&>);
    CHECK(a.error() == secret);
    CHECK(std::move(a).error() == secret);

    A const b{mystery};
    static_assert(noexcept(A{b}));
    static_assert(noexcept(A{std::move(b)}));
    static_assert(noexcept(b.what()));
    static_assert(std::is_same_v<decltype(b.what()), char const *>);
    static_assert(std::is_same_v<decltype(b.error()), Error const &>);
    static_assert(std::is_same_v<decltype(std::move(b).error()), Error const &&>);
    CHECK(b.error() == mystery);
    CHECK(std::move(b).error() == mystery);

    CHECK(std::strcmp(a.what(), "bad access to expected without expected value") == 0);
    CHECK(a.what() == b.what());
    auto const c = []() {
      struct C : pfn::bad_expected_access<void> {};
      return C{};
    }();
    CHECK(a.what() == c.what());
    CHECK(&decltype(a)::what == &decltype(b)::what);
  }
}

namespace test {
template <auto E> struct A {};
} // namespace test

TEST_CASE("unexpect", "[expected][polyfill][unexpect]")
{
  SECTION("unexpect")
  {
    static_assert(std::is_empty_v<pfn::unexpect_t>);
    static_assert(noexcept(pfn::unexpect_t{}));
    static_assert(std::is_same_v<decltype(pfn::unexpect), pfn::unexpect_t const>);
    // pfn::unexpect can be used as a NTTP
    static_assert(std::is_empty_v<test::A<pfn::unexpect>>);
    static constexpr auto a = pfn::unexpect;
    static_assert(std::is_empty_v<test::A<a>>);
    static_assert(std::is_same_v<decltype(a), pfn::unexpect_t const>);
    static_assert(std::is_same_v<test::A<pfn::unexpect>, test::A<a>>);
  }
}

namespace unxp {
static int witness = 0;

struct Foo {
  int v = {};

  Foo() = delete;
  Foo(Foo &&) = default;

  Foo &operator=(Foo &o) noexcept
  {
    v = o.v;
    witness *= 53;
    return *this;
  }

  Foo &operator=(Foo const &o) noexcept
  {
    v = o.v;
    witness *= 59;
    return *this;
  }

  Foo &operator=(Foo &&o) noexcept
  {
    v = o.v;
    witness *= 61;
    return *this;
  }

  Foo &operator=(Foo const &&o) noexcept
  {
    v = o.v;
    witness *= 67;
    return *this;
  }

  Foo(int a) noexcept : v(a) { witness += a; }

  Foo(auto &&...a) noexcept
    requires(sizeof...(a) > 1 && (std::is_same_v<std::remove_cvref_t<decltype(a)>, int> && ...))
      : v((1 * ... * a))
  {
    witness += v;
  }

  Foo(std::initializer_list<double> l, auto... a) noexcept(false)
    requires(std::is_same_v<std::remove_cvref_t<decltype(a)>, int> && ...)
      : v(init(l, a...))
  {
    witness += v;
  }

  bool operator==(Foo const &) const noexcept = default;

  static int init(std::initializer_list<double> l, auto &&...a) noexcept(false)
  {
    double ret = (1 * ... * a);
    for (auto d : l) {
      if (d == 0.0)
        throw std::runtime_error("invalid input");
      ret *= d;
    }
    return static_cast<int>(ret);
  }
};

void swap(Foo &l, Foo &r)
{
  std::swap(l.v, r.v);
  witness *= 97;
}

} // namespace unxp

TEST_CASE("unexpected", "[expected][polyfill][unexpected]")
{
  using pfn::unexpected;
  using unxp::Foo;
  using unxp::witness;

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
      auto const before = witness;
      unexpected c{Foo{2}};
      CHECK(witness == before + 2);
      CHECK(c.error() == Foo{2});
      CHECK(c == unexpected<Foo>{2});
      static_assert(std::is_same_v<decltype(c), unexpected<Foo>>);
      static_assert(std::is_nothrow_constructible_v<decltype(c), Foo>);
    }

    SECTION("conversion, no CTAD")
    {
      auto const before = witness;
      unexpected<Foo> c(3);
      CHECK(witness == before + 3);
      CHECK(c.error().v == 3);
      CHECK(c == unexpected<Foo>{3});
      static_assert(std::is_nothrow_constructible_v<decltype(c), int>);
    }

    SECTION("in-place, no CTAD")
    {
      auto const before = witness;
      unexpected<Foo> c(std::in_place, 3, 5);
      CHECK(witness == before + 3 * 5);
      CHECK(c.error() == Foo{3, 5});
      CHECK(c == unexpected<Foo>{15});
      static_assert(std::is_nothrow_constructible_v<decltype(c), std::in_place_t, int, int>);
    }

    SECTION("in_place, not CTAD, initializer_list, noexcept(false)")
    {
      SECTION("forwarded args")
      {
        auto const before = witness;
        unexpected<Foo> c(std::in_place, {3.0, 5.0}, 7, 11);
        auto const d = 3 * 5 * 7 * 11;
        CHECK(witness == before + d);
        CHECK(c.error() == Foo{d});
        CHECK(c == unexpected{Foo{d}});
        static_assert(
            not std::is_nothrow_constructible_v<decltype(c), std::in_place_t, std::initializer_list<double>, int, int>);
        static_assert(std::is_constructible_v<decltype(c), std::in_place_t, std::initializer_list<double>, int, int>);
      }

      SECTION("no forwarded args")
      {
        auto const before = witness;
        unexpected<Foo> c(std::in_place, {2.0, 2.5});
        CHECK(witness == before + 5);
        CHECK(c.error() == Foo{5});
        CHECK(c == unexpected<Foo>{5});
        static_assert(not std::is_nothrow_constructible_v<decltype(c), std::in_place_t, std::initializer_list<double>>);
        static_assert(std::is_constructible_v<decltype(c), std::in_place_t, std::initializer_list<double>>);
      }

      SECTION("exception thrown")
      {
        unexpected<Foo> t{13};
        auto const before = witness;
        try {
          t = unexpected<Foo>{std::in_place, {2.0, 1.0, 0.0}, 5};
          FAIL();
        } catch (std::runtime_error const &) {
          SUCCEED();
        }
        CHECK(t.error().v == 13);
        CHECK(witness == before);
      }
    }
  }

  SECTION("accessor")
  {
    Foo v{1};

    SECTION("lval")
    {
      unexpected<Foo> t{13};
      auto before = witness;
      v = t.error();
      CHECK(witness == before * 53);
      CHECK(v == Foo{13});
    }

    SECTION("lval const")
    {
      unexpected<Foo> const t{17};
      auto before = witness;
      v = t.error();
      CHECK(witness == before * 59);
      CHECK(v == Foo{17});
    }

    SECTION("rval")
    {
      unexpected<Foo> t{19};
      auto before = witness;
      v = std::move(t).error();
      CHECK(witness == before * 61);
      CHECK(v == Foo{19});
    }

    SECTION("rval const")
    {
      unexpected<Foo> const t{23};
      auto before = witness;
      v = std::move(t).error();
      CHECK(witness == before * 67);
      CHECK(v == Foo{23});
    }
  }

  SECTION("assignment")
  {
    unexpected<Foo> v{0};

    SECTION("lval")
    {
      unexpected<Foo> t{13};
      auto before = witness;
      v = t; // t binds to unexpected<Foo> const &
      CHECK(witness == before * 59);
      CHECK(v.error() == Foo{13});
    }

    SECTION("lval const")
    {
      unexpected<Foo> const t{17};
      auto before = witness;
      v = t; // t binds to unexpected<Foo> const &
      CHECK(witness == before * 59);
      CHECK(v.error() == Foo{17});
    }

    SECTION("rval")
    {
      unexpected<Foo> t{19};
      auto before = witness;
      v = std::move(t); // t binds to unexpected<Foo> &&
      CHECK(witness == before * 61);
      CHECK(v.error() == Foo{19});
    }

    SECTION("rval const")
    {
      unexpected<Foo> const t{23};
      auto before = witness;
      v = std::move(t); // t binds to unexpected<Foo> const &
      CHECK(witness == before * 59);
      CHECK(v.error() == Foo{23});
    }
  }

  SECTION("swap")
  {
    unexpected<Foo> v{2};
    unexpected w{Foo{3}};
    auto before = witness;
    v.swap(w);
    CHECK(witness == before * 97);
    CHECK(v == unexpected{Foo{3}});
    CHECK(w == unexpected{Foo{2}});
    w.error() = Foo{11};
    before = witness;
    swap(v, w);
    CHECK(v.error() == Foo{11});
    CHECK(w.error() == Foo(3));
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
