// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/sum.hpp"
#include "functional/utility.hpp"

#include <catch2/catch_all.hpp>

#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

struct TestType final {
  static int count;
  TestType() noexcept { ++count; }
  ~TestType() noexcept { --count; }
};
int TestType::count = 0;

struct NonCopyable final {
  int v;

  constexpr operator int() const { return v; }
  constexpr NonCopyable(int i) noexcept : v(i) {}
  NonCopyable(NonCopyable const &) = delete;
  NonCopyable &operator=(NonCopyable const &) = delete;
};

struct MoveOnly final {
  int v;

  constexpr operator int() const { return v; }
  constexpr MoveOnly(int i) noexcept : v(i) {}
  MoveOnly(MoveOnly const &) = delete;
  constexpr MoveOnly(MoveOnly &&s) : v(s.v) { s.v = -1; }

  MoveOnly &operator=(MoveOnly const &) = delete;
  constexpr MoveOnly &operator=(MoveOnly &&s) = delete;
};

struct CopyOnly final {
  int v;

  constexpr operator int() const { return v; }
  constexpr CopyOnly(int i) noexcept : v(i) {}
  constexpr CopyOnly(CopyOnly const &) = default;
  constexpr CopyOnly(CopyOnly &&s) = delete;

  constexpr CopyOnly &operator=(CopyOnly const &) = delete;
  constexpr CopyOnly &operator=(CopyOnly &&s) = delete;
};

} // anonymous namespace

TEST_CASE("sum functions", "[sum][invoke]")
{
  // NOTE We have 5 different specializations, need to test each. This test is
  // ridiculously long to exercise the value-category preserving FWD(v) in apply_variadic_union

  using namespace fn;
  constexpr auto fn1 = [](auto i) noexcept -> std::size_t { return sizeof(i); };
  constexpr auto fn2 = [](auto, auto i) noexcept -> std::size_t { return sizeof(i); };

  WHEN("size 1")
  {
    sum<int> a{std::in_place_type<int>, 42};
    static_assert(decltype(a)::size == 1);
    CHECK(a.data.v0 == 42);

    WHEN("value only")
    {
      CHECK(a.invoke(fn1) == 4);
      CHECK(a.invoke(   //
          fn::overload( //
              [](auto) -> bool { throw 1; }, [](int &i) -> bool { return i == 42; },
              [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
              [](int const &&) -> bool { throw 0; })));
      CHECK(std::as_const(a).invoke( //
          fn::overload(              //
              [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
              [](int const &i) -> bool { return i == 42; }, [](int &&) -> bool { throw 0; },
              [](int const &&) -> bool { throw 0; })));
      CHECK(sum<int>{std::in_place_type<int>, 42}.invoke( //
          fn::overload(                                   //
              [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
              [](int &&i) -> bool { return i == 42; }, [](int const &&) -> bool { throw 0; })));
      CHECK(std::move(std::as_const(a))
                .invoke(          //
                    fn::overload( //
                        [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                        [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                        [](int const &&i) -> bool { return i == 42; })));
    }

    WHEN("tag and value")
    {
      CHECK(a.invoke(fn2) == 4);
      CHECK(a.invoke(   //
          fn::overload( //
              [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &i) -> bool { return i == 42; },
              [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
              [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
              [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
      CHECK(std::as_const(a).invoke( //
          fn::overload(              //
              [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
              [](std::in_place_type_t<int>, int const &i) -> bool { return i == 42; },
              [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
              [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
      CHECK(sum<int>{std::in_place_type<int>, 42}.invoke( //
          fn::overload(                                   //
              [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
              [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
              [](std::in_place_type_t<int>, int &&i) -> bool { return i == 42; },
              [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
      CHECK(std::move(std::as_const(a))
                .invoke(          //
                    fn::overload( //
                        [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &&i) -> bool { return i == 42; })));
    }
  }

  WHEN("size 2")
  {
    using type = sum<double, int>;
    static_assert(type::size == 2);

    WHEN("element v0 set")
    {
      type a{std::in_place_type<double>, 0.5};
      CHECK(a.data.v0 == 0.5);
      WHEN("value only")
      {
        CHECK(a.invoke(fn1) == 8);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](double &i) -> bool { return i == 0.5; },
                [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                [](double const &i) -> bool { return i == 0.5; }, [](double &&) -> bool { throw 0; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<double>, 0.5}.invoke( //
            fn::overload(                                   //
                [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                [](double const &) -> bool { throw 0; }, [](double &&i) -> bool { return i == 0.5; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                          [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                          [](double const &&i) -> bool { return i == 0.5; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 8);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<double>, double &i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<double>, 0.5}.invoke( //
            fn::overload(                                   //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double &&i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
                      fn::overload( //
                          [](auto, auto) -> bool { throw 1; },
                          [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double const &&i) -> bool { return i == 0.5; })));
      }
    }

    WHEN("element v1 set")
    {
      type a{std::in_place_type<int>, 42};
      CHECK(a.data.v1 == 42);

      WHEN("value only")
      {
        CHECK(a.invoke(fn1) == 4);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](int &i) -> bool { return i == 42; },
                [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                [](int const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                [](int const &i) -> bool { return i == 42; }, [](int &&) -> bool { throw 0; },
                [](int const &&) -> bool { throw 0; })));
        CHECK(sum<int>{std::in_place_type<int>, 42}.invoke( //
            fn::overload(                                   //
                [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                [](int &&i) -> bool { return i == 42; }, [](int const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                          [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                          [](int const &&i) -> bool { return i == 42; })));
      }

      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 4);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(sum<int>{std::in_place_type<int>, 42}.invoke( //
            fn::overload(                                   //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int &&i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke(          //
                    fn::overload( //
                        [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &&i) -> bool { return i == 42; })));
      }
    }
  }

  WHEN("size 3")
  {
    using type = sum<double, int, std::string_view>;
    static_assert(type::size == 3);

    WHEN("element v0 set")
    {
      type a{std::in_place_type<double>, 0.5};
      CHECK(a.data.v0 == 0.5);
      WHEN("value only")
      {
        CHECK(a.invoke(fn1) == 8);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](double &i) -> bool { return i == 0.5; },
                [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                [](double const &i) -> bool { return i == 0.5; }, [](double &&) -> bool { throw 0; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<double>, 0.5}.invoke( //
            fn::overload(                                   //
                [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                [](double const &) -> bool { throw 0; }, [](double &&i) -> bool { return i == 0.5; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                          [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                          [](double const &&i) -> bool { return i == 0.5; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 8);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<double>, double &i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<double>, 0.5}.invoke( //
            fn::overload(                                   //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double &&i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
                      fn::overload( //
                          [](auto, auto) -> bool { throw 1; },
                          [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double const &&i) -> bool { return i == 0.5; })));
      }
    }

    WHEN("element v1 set")
    {
      type a{std::in_place_type<int>, 42};
      CHECK(a.data.v1 == 42);

      WHEN("value only")
      {
        CHECK(a.invoke(fn1) == 4);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](int &i) -> bool { return i == 42; },
                [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                [](int const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                [](int const &i) -> bool { return i == 42; }, [](int &&) -> bool { throw 0; },
                [](int const &&) -> bool { throw 0; })));
        CHECK(sum<int>{std::in_place_type<int>, 42}.invoke( //
            fn::overload(                                   //
                [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                [](int &&i) -> bool { return i == 42; }, [](int const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                          [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                          [](int const &&i) -> bool { return i == 42; })));
      }

      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 4);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(sum<int>{std::in_place_type<int>, 42}.invoke( //
            fn::overload(                                   //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int &&i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke(          //
                    fn::overload( //
                        [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &&i) -> bool { return i == 42; })));
      }
    }

    WHEN("element v2 set")
    {
      type a{std::in_place_type<std::string_view>, "baz"};
      CHECK(a.data.v2 == "baz");
      WHEN("value only")
      {
        CHECK(a.invoke(fn1) == 16);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](std::string_view &i) -> bool { return i == "baz"; },
                [](std::string_view const &) -> bool { throw 0; }, [](std::string_view &&) -> bool { throw 0; },
                [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                [](std::string_view const &i) -> bool { return i == "baz"; },
                [](std::string_view &&) -> bool { throw 0; }, [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string_view>, "baz"}.invoke( //
            fn::overload(                                               //
                [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                [](std::string_view const &) -> bool { throw 0; },
                [](std::string_view &&i) -> bool { return i == "baz"; },
                [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke(          //
                    fn::overload( //
                        [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                        [](std::string_view const &) -> bool { throw 0; }, [](std::string_view &&) -> bool { throw 0; },
                        [](std::string_view const &&i) -> bool { return i == "baz"; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 16);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string_view>, "baz"}.invoke( //
            fn::overload(
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke( //
                      fn::overload(
                          [](auto, auto) -> bool { throw 1; },
                          [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                          [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                          [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                          [](std::in_place_type_t<std::string_view>, std::string_view const &&i) -> bool {
                            return i == "baz";
                          })));
      }
    }
  }

  WHEN("size 4")
  {
    using type = sum<double, int, std::string, std::string_view>;
    static_assert(type::size == 4);

    WHEN("element v0 set")
    {
      type a{std::in_place_type<double>, 0.5};
      CHECK(a.data.v0 == 0.5);
      WHEN("value only")
      {
        CHECK(a.invoke(fn1) == 8);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](double &i) -> bool { return i == 0.5; },
                [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                [](double const &i) -> bool { return i == 0.5; }, [](double &&) -> bool { throw 0; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<double>, 0.5}.invoke( //
            fn::overload(                                   //
                [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                [](double const &) -> bool { throw 0; }, [](double &&i) -> bool { return i == 0.5; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                          [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                          [](double const &&i) -> bool { return i == 0.5; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 8);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<double>, double &i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<double>, 0.5}.invoke( //
            fn::overload(                                   //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double &&i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
                      fn::overload( //
                          [](auto, auto) -> bool { throw 1; },
                          [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double const &&i) -> bool { return i == 0.5; })));
      }
    }

    WHEN("element v1 set")
    {
      type a{std::in_place_type<int>, 42};
      CHECK(a.data.v1 == 42);

      WHEN("value only")
      {
        CHECK(a.invoke(fn1) == 4);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](int &i) -> bool { return i == 42; },
                [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                [](int const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                [](int const &i) -> bool { return i == 42; }, [](int &&) -> bool { throw 0; },
                [](int const &&) -> bool { throw 0; })));
        CHECK(sum<int>{std::in_place_type<int>, 42}.invoke( //
            fn::overload(                                   //
                [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                [](int &&i) -> bool { return i == 42; }, [](int const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                          [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                          [](int const &&i) -> bool { return i == 42; })));
      }

      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 4);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(sum<int>{std::in_place_type<int>, 42}.invoke( //
            fn::overload(                                   //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int &&i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke(          //
                    fn::overload( //
                        [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &&i) -> bool { return i == 42; })));
      }
    }

    WHEN("element v2 set")
    {
      type a{std::in_place_type<std::string>, "bar"};
      CHECK(a.data.v2 == "bar");
      WHEN("value only")
      {
        CHECK(a.invoke(fn1) == 32);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](std::string &i) -> bool { return i == "bar"; },
                [](std::string const &) -> bool { throw 0; }, [](std::string &&) -> bool { throw 0; },
                [](std::string const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](std::string &) -> bool { throw 0; },
                [](std::string const &i) -> bool { return i == "bar"; }, [](std::string &&) -> bool { throw 0; },
                [](std::string const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string>, "bar"}.invoke( //
            fn::overload(                                          //
                [](auto) -> bool { throw 1; }, [](std::string &) -> bool { throw 0; },
                [](std::string const &) -> bool { throw 0; }, [](std::string &&i) -> bool { return i == "bar"; },
                [](std::string const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](std::string &) -> bool { throw 0; },
                          [](std::string const &) -> bool { throw 0; }, [](std::string &&) -> bool { throw 0; },
                          [](std::string const &&i) -> bool { return i == "bar"; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 32);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string>, std::string &i) -> bool { return i == "bar"; },
                [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string const &i) -> bool { return i == "bar"; },
                [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string>, "bar"}.invoke( //
            fn::overload(                                          //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string &&i) -> bool { return i == "bar"; },
                [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke(          //
                    fn::overload( //
                        [](auto, auto) -> bool { throw 1; },
                        [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                        [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                        [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                        [](std::in_place_type_t<std::string>, std::string const &&i) -> bool { return i == "bar"; })));
      }
    }

    WHEN("element v3 set")
    {
      type a{std::in_place_type<std::string_view>, "baz"};
      CHECK(a.data.v3 == "baz");
      WHEN("value only")
      {
        CHECK(a.invoke(fn1) == 16);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](std::string_view &i) -> bool { return i == "baz"; },
                [](std::string_view const &) -> bool { throw 0; }, [](std::string_view &&) -> bool { throw 0; },
                [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                [](std::string_view const &i) -> bool { return i == "baz"; },
                [](std::string_view &&) -> bool { throw 0; }, [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string_view>, "baz"}.invoke( //
            fn::overload(                                               //
                [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                [](std::string_view const &) -> bool { throw 0; },
                [](std::string_view &&i) -> bool { return i == "baz"; },
                [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke(          //
                    fn::overload( //
                        [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                        [](std::string_view const &) -> bool { throw 0; }, [](std::string_view &&) -> bool { throw 0; },
                        [](std::string_view const &&i) -> bool { return i == "baz"; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 16);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string_view>, "baz"}.invoke( //
            fn::overload(
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke( //
                      fn::overload(
                          [](auto, auto) -> bool { throw 1; },
                          [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                          [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                          [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                          [](std::in_place_type_t<std::string_view>, std::string_view const &&i) -> bool {
                            return i == "baz";
                          })));
      }
    }
  }

  WHEN("size 5")
  {
    using type = sum<double, int, std::string, std::string_view, std::vector<int>>;
    static_assert(type::size == 5);

    WHEN("element v0 set")
    {
      type a{std::in_place_type<double>, 0.5};
      CHECK(a.data.v0 == 0.5);
      WHEN("value only")
      {
        CHECK(a.invoke(fn1) == 8);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](double &i) -> bool { return i == 0.5; },
                [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                [](double const &i) -> bool { return i == 0.5; }, [](double &&) -> bool { throw 0; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<double>, 0.5}.invoke( //
            fn::overload(                                   //
                [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                [](double const &) -> bool { throw 0; }, [](double &&i) -> bool { return i == 0.5; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                          [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                          [](double const &&i) -> bool { return i == 0.5; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 8);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<double>, double &i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<double>, 0.5}.invoke( //
            fn::overload(                                   //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double &&i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
                      fn::overload( //
                          [](auto, auto) -> bool { throw 1; },
                          [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double const &&i) -> bool { return i == 0.5; })));
      }
    }

    WHEN("element v1 set")
    {
      type a{std::in_place_type<int>, 42};
      CHECK(a.data.v1 == 42);

      WHEN("value only")
      {
        CHECK(a.invoke(fn1) == 4);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](int &i) -> bool { return i == 42; },
                [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                [](int const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                [](int const &i) -> bool { return i == 42; }, [](int &&) -> bool { throw 0; },
                [](int const &&) -> bool { throw 0; })));
        CHECK(sum<int>{std::in_place_type<int>, 42}.invoke( //
            fn::overload(                                   //
                [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                [](int &&i) -> bool { return i == 42; }, [](int const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                          [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                          [](int const &&i) -> bool { return i == 42; })));
      }

      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 4);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(sum<int>{std::in_place_type<int>, 42}.invoke( //
            fn::overload(                                   //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int &&i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke(          //
                    fn::overload( //
                        [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &&i) -> bool { return i == 42; })));
      }
    }

    WHEN("element v2 set")
    {
      type a{std::in_place_type<std::string>, "bar"};
      CHECK(a.data.v2 == "bar");
      WHEN("value only")
      {
        CHECK(a.invoke(fn1) == 32);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](std::string &i) -> bool { return i == "bar"; },
                [](std::string const &) -> bool { throw 0; }, [](std::string &&) -> bool { throw 0; },
                [](std::string const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](std::string &) -> bool { throw 0; },
                [](std::string const &i) -> bool { return i == "bar"; }, [](std::string &&) -> bool { throw 0; },
                [](std::string const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string>, "bar"}.invoke( //
            fn::overload(                                          //
                [](auto) -> bool { throw 1; }, [](std::string &) -> bool { throw 0; },
                [](std::string const &) -> bool { throw 0; }, [](std::string &&i) -> bool { return i == "bar"; },
                [](std::string const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](std::string &) -> bool { throw 0; },
                          [](std::string const &) -> bool { throw 0; }, [](std::string &&) -> bool { throw 0; },
                          [](std::string const &&i) -> bool { return i == "bar"; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 32);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string>, std::string &i) -> bool { return i == "bar"; },
                [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string const &i) -> bool { return i == "bar"; },
                [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string>, "bar"}.invoke( //
            fn::overload(                                          //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string &&i) -> bool { return i == "bar"; },
                [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke(          //
                    fn::overload( //
                        [](auto, auto) -> bool { throw 1; },
                        [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                        [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                        [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                        [](std::in_place_type_t<std::string>, std::string const &&i) -> bool { return i == "bar"; })));
      }
    }

    WHEN("element v3 set")
    {
      type a{std::in_place_type<std::string_view>, "baz"};
      CHECK(a.data.v3 == "baz");
      WHEN("value only")
      {
        CHECK(a.invoke(fn1) == 16);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](std::string_view &i) -> bool { return i == "baz"; },
                [](std::string_view const &) -> bool { throw 0; }, [](std::string_view &&) -> bool { throw 0; },
                [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                [](std::string_view const &i) -> bool { return i == "baz"; },
                [](std::string_view &&) -> bool { throw 0; }, [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string_view>, "baz"}.invoke( //
            fn::overload(                                               //
                [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                [](std::string_view const &) -> bool { throw 0; },
                [](std::string_view &&i) -> bool { return i == "baz"; },
                [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke(          //
                    fn::overload( //
                        [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                        [](std::string_view const &) -> bool { throw 0; }, [](std::string_view &&) -> bool { throw 0; },
                        [](std::string_view const &&i) -> bool { return i == "baz"; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 16);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string_view>, "baz"}.invoke( //
            fn::overload(
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke( //
                      fn::overload(
                          [](auto, auto) -> bool { throw 1; },
                          [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                          [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                          [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                          [](std::in_place_type_t<std::string_view>, std::string_view const &&i) -> bool {
                            return i == "baz";
                          })));
      }
    }

    WHEN("more elements set")
    {
      std::vector<int> const foo{3, 14, 15, 92};
      type a{std::in_place_type<std::vector<int>>, foo};
      CHECK(a.data.more.v0 == foo);
      WHEN("value only")
      {
        CHECK(a.invoke(fn1) == 24);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [&](std::vector<int> &i) -> bool { return i == foo; },
                [](std::vector<int> const &) -> bool { throw 0; }, [](std::vector<int> &&) -> bool { throw 0; },
                [](std::vector<int> const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](std::vector<int> &) -> bool { throw 0; },
                [&](std::vector<int> const &i) -> bool { return i == foo; },
                [](std::vector<int> &&) -> bool { throw 0; }, [](std::vector<int> const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::vector<int>>, foo}.invoke( //
            fn::overload(                                             //
                [](auto) -> bool { throw 1; }, [](std::vector<int> &) -> bool { throw 0; },
                [](std::vector<int> const &) -> bool { throw 0; },
                [&](std::vector<int> &&i) -> bool { return i == foo; },
                [](std::vector<int> const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke(          //
                    fn::overload( //
                        [](auto) -> bool { throw 1; }, [](std::vector<int> &) -> bool { throw 0; },
                        [](std::vector<int> const &) -> bool { throw 0; }, [](std::vector<int> &&) -> bool { throw 0; },
                        [&](std::vector<int> const &&i) -> bool { return i == foo; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 24);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; },
                [&](std::in_place_type_t<std::vector<int>>, std::vector<int> &i) -> bool { return i == foo; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> &) -> bool { throw 0; },
                [&](std::in_place_type_t<std::vector<int>>, std::vector<int> const &i) -> bool { return i == foo; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::vector<int>>, foo}.invoke( //
            fn::overload(                                             //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> &) -> bool { throw 0; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> const &) -> bool { throw 0; },
                [&](std::in_place_type_t<std::vector<int>>, std::vector<int> &&i) -> bool { return i == foo; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke( //
                      fn::overload(
                          [](auto, auto) -> bool { throw 1; },
                          [](std::in_place_type_t<std::vector<int>>, std::vector<int> &) -> bool { throw 0; },
                          [](std::in_place_type_t<std::vector<int>>, std::vector<int> const &) -> bool { throw 0; },
                          [](std::in_place_type_t<std::vector<int>>, std::vector<int> &&) -> bool { throw 0; },
                          [&](std::in_place_type_t<std::vector<int>>, std::vector<int> const &&i) -> bool {
                            return i == foo;
                          })));
      }
    }
  }
}
