// Copyright (c) 2025 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <pfn/expected.hpp>

#include <catch2/catch_all.hpp>

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

TEST_CASE("unexpected", "[expected][polyfill][unexpected]")
{
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
    using pfn::unexpected;

    SECTION("explicit single parameter")
    {
      SECTION("constexpr, CTAD")
      {
        constexpr unexpected c1{Error::mystery};
        static_assert(std::is_same_v<decltype(c1), unexpected<Error> const>);
        static_assert(c1.error() == Error::mystery);
      }

      SECTION("constexpr, no CTAD")
      {
        constexpr unexpected<int> c1{42};
        static_assert(std::is_same_v<decltype(c1), unexpected<int> const>);
        static_assert(c1.error() == 42);
      }

      SECTION("no conversion, CTAD")
      {
        unexpected const c2{Error::secret};
        static_assert(std::is_same_v<decltype(c2), unexpected<Error> const>);
        CHECK(c2.error() == Error::secret);
      }

      SECTION("conversion, no CTAD")
      {
        unexpected<double> c3(12);
        static_assert(std::is_same_v<decltype(c3), unexpected<double>>);
        CHECK(c3.error() == 12.0);
      }
    }

    SECTION("in_place") {}
  }
}
