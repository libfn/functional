// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/expected.hpp"
#include "functional/utility.hpp"

#include <catch2/catch_all.hpp>

#include <utility>

namespace {
enum Error { Unknown, FileNotFound };

struct Xint {
  int value;
  constexpr bool operator==(Xint const &) const noexcept = default;
};
} // namespace

TEST_CASE("graded monad", "[expected][sum][graded][and_then][or_else][sum_value][sum_error]")
{
  WHEN("unit")
  {
    constexpr fn::expected<void, fn::sum<>> unit{};
    static_assert(unit.has_value());

    WHEN("constexpr")
    {
      WHEN("and_then to value/sum<>")
      {
        constexpr auto fn = []() -> fn::expected<int, fn::sum<>> { return {7}; };
        constexpr auto a = unit.and_then(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<int, fn::sum<>> const>);
        static_assert(a.value() == 7);
      }

      WHEN("and_then to value")
      {
        constexpr auto fn = []() -> fn::expected<int, Error> { return {12}; };
        constexpr auto a = unit.and_then(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<int, fn::sum<Error>> const>);
        static_assert(a.value() == 12);
      }

      WHEN("and_then to error")
      {
        constexpr auto fn = []() -> fn::expected<int, Error> { return std::unexpected<Error>(FileNotFound); };
        constexpr auto a = unit.and_then(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<int, fn::sum<Error>> const>);
        static_assert(a.error() == fn::sum{FileNotFound});
      }

      WHEN("transform to int")
      {
        constexpr auto fn = []() -> int { return 144'000; };
        constexpr auto a = unit.transform(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<int, fn::sum<>> const>);
        static_assert(a.value() == 144'000);
      }
    }

    WHEN("runtime")
    {
      WHEN("and_then to value/sum<>")
      {
        constexpr auto fn = []() -> fn::expected<int, fn::sum<>> { return {7}; };
        auto a = unit.and_then(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<int, fn::sum<>>>);
        CHECK(a.value() == 7);
      }

      WHEN("and_then to value")
      {
        constexpr auto fn = []() -> fn::expected<int, Error> { return {12}; };
        auto a = unit.and_then(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<int, fn::sum<Error>>>);
        CHECK(a.value() == 12);
      }

      WHEN("and_then to error")
      {
        constexpr auto fn = []() -> fn::expected<int, Error> { return std::unexpected<Error>(FileNotFound); };
        auto a = unit.and_then(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<int, fn::sum<Error>>>);
        CHECK(a.error() == fn::sum{FileNotFound});
      }

      WHEN("transform to int")
      {
        constexpr auto fn = []() -> int { return 144'000; };
        auto a = unit.transform(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<int, fn::sum<>>>);
        CHECK(a.value() == 144'000);
      }
    }
  }

  WHEN("sum_error from sum")
  {
    using T = fn::expected<int, fn::sum<Error>>;
    T s{12};
    static_assert(std::is_same_v<decltype(s.sum_error()), T &>);
    static_assert(std::is_same_v<decltype(std::as_const(s).sum_error()), T const &>);
    static_assert(std::is_same_v<decltype(std::move(std::as_const(s)).sum_error()), T const &&>);
    static_assert(std::is_same_v<decltype(std::move(s).sum_error()), T &&>);
    WHEN("value")
    {
      CHECK(s.sum_error().value() == 12);
      CHECK(std::as_const(s).sum_error().value() == 12);
      CHECK(std::move(std::as_const(s)).sum_error().value() == 12);
      CHECK(std::move(s).sum_error().value() == 12);
    }
    WHEN("error")
    {
      T s{std::unexpect, Unknown};
      CHECK(s.sum_error().error() == fn::sum{Unknown});
      CHECK(std::as_const(s).sum_error().error() == fn::sum{Unknown});
      CHECK(std::move(std::as_const(s)).sum_error().error() == fn::sum{Unknown});
      CHECK(std::move(s).sum_error().error() == fn::sum{Unknown});
    }

    static_assert(std::is_same_v<decltype(fn::sum_error(s)), T &>);
  }

  WHEN("sum_error from non-sum")
  {
    using T = fn::expected<int, Error>;
    T s{12};
    static_assert(std::is_same_v<decltype(s.sum_error()), fn::expected<int, fn::sum<Error>>>);
    static_assert(std::is_same_v<decltype(std::as_const(s).sum_error()), fn::expected<int, fn::sum<Error>>>);
    static_assert(std::is_same_v<decltype(std::move(std::as_const(s)).sum_error()), fn::expected<int, fn::sum<Error>>>);
    static_assert(std::is_same_v<decltype(std::move(s).sum_error()), fn::expected<int, fn::sum<Error>>>);
    WHEN("value")
    {
      CHECK(s.sum_error().value() == 12);
      CHECK(std::as_const(s).sum_error().value() == 12);
      CHECK(std::move(std::as_const(s)).sum_error().value() == 12);
      CHECK(std::move(s).sum_error().value() == 12);
    }
    WHEN("error")
    {
      T s{std::unexpect, Unknown};
      CHECK(s.sum_error().error() == fn::sum{Unknown});
      CHECK(std::as_const(s).sum_error().error() == fn::sum{Unknown});
      CHECK(std::move(std::as_const(s)).sum_error().error() == fn::sum{Unknown});
      CHECK(std::move(s).sum_error().error() == fn::sum{Unknown});
    }

    static_assert(std::is_same_v<decltype(fn::sum_error(s)), fn::expected<int, fn::sum<Error>>>);
  }

  WHEN("sum_value from sum")
  {
    using T = fn::expected<fn::sum<int>, Error>;
    T s{12};
    static_assert(std::is_same_v<decltype(s.sum_value()), T &>);
    static_assert(std::is_same_v<decltype(std::as_const(s).sum_value()), T const &>);
    static_assert(std::is_same_v<decltype(std::move(std::as_const(s)).sum_value()), T const &&>);
    static_assert(std::is_same_v<decltype(std::move(s).sum_value()), T &&>);
    WHEN("value")
    {
      CHECK(s.sum_value().value() == fn::sum{12});
      CHECK(std::as_const(s).sum_value().value() == fn::sum{12});
      CHECK(std::move(std::as_const(s)).sum_value().value() == fn::sum{12});
      CHECK(std::move(s).sum_value().value() == fn::sum{12});
    }
    WHEN("error")
    {
      T s{std::unexpect, Unknown};
      CHECK(s.sum_value().error() == Unknown);
      CHECK(std::as_const(s).sum_value().error() == Unknown);
      CHECK(std::move(std::as_const(s)).sum_value().error() == Unknown);
      CHECK(std::move(s).sum_value().error() == Unknown);
    }

    static_assert(std::is_same_v<decltype(fn::sum_value(s)), T &>);
  }

  WHEN("sum_value from non-sum")
  {
    using T = fn::expected<int, Error>;
    T s{12};
    static_assert(std::is_same_v<decltype(s.sum_value()), fn::expected<fn::sum<int>, Error>>);
    static_assert(std::is_same_v<decltype(std::as_const(s).sum_value()), fn::expected<fn::sum<int>, Error>>);
    static_assert(std::is_same_v<decltype(std::move(std::as_const(s)).sum_value()), fn::expected<fn::sum<int>, Error>>);
    static_assert(std::is_same_v<decltype(std::move(s).sum_value()), fn::expected<fn::sum<int>, Error>>);
    WHEN("value")
    {
      CHECK(s.sum_value().value() == fn::sum{12});
      CHECK(std::as_const(s).sum_value().value() == fn::sum{12});
      CHECK(std::move(std::as_const(s)).sum_value().value() == fn::sum{12});
      CHECK(std::move(s).sum_value().value() == fn::sum{12});
    }
    WHEN("error")
    {
      T s{std::unexpect, Unknown};
      CHECK(s.sum_value().error() == Unknown);
      CHECK(std::as_const(s).sum_value().error() == Unknown);
      CHECK(std::move(std::as_const(s)).sum_value().error() == Unknown);
      CHECK(std::move(s).sum_value().error() == Unknown);
    }

    static_assert(std::is_same_v<decltype(fn::sum_value(s)), fn::expected<fn::sum<int>, Error>>);
  }

  WHEN("and_then")
  {
    WHEN("value to value")
    {
      fn::expected<int, fn::sum<Error>> s{12};

      constexpr auto fn1 = [](int) -> fn::expected<int, bool> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn1)), fn::expected<int, fn::sum<Error, bool>>>);
      constexpr auto fn2 = [](int) -> fn::expected<int, Error> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn2)), fn::expected<int, fn::sum<Error>>>);
      constexpr auto fn3 = [](int) -> fn::expected<int, fn::sum<Error>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn3)), fn::expected<int, fn::sum<Error>>>);
      constexpr auto fn4 = [](int) -> fn::expected<int, fn::sum<bool>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn4)), fn::expected<int, fn::sum<Error, bool>>>);
      constexpr auto fn5 = [](int) -> fn::expected<int, fn::sum<Error, bool>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn5)), fn::expected<int, fn::sum<Error, bool>>>);
      constexpr auto fn6 = [](int) -> fn::expected<int, fn::sum<bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn6)), fn::expected<int, fn::sum<Error, bool, int>>>);
      constexpr auto fn7 = [](int) -> fn::expected<int, fn::sum<Error, bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn7)), fn::expected<int, fn::sum<Error, bool, int>>>);
      constexpr auto fn8 = [](int) -> fn::expected<Xint, fn::sum<Error, bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn8)), fn::expected<Xint, fn::sum<Error, bool, int>>>);

      WHEN("value to value")
      {
        constexpr auto fn = [](int i) -> fn::expected<int, bool> { return {i + 12}; };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<int, fn::sum<Error, bool>>>);
        CHECK(s.and_then(fn).value() == 24);
        CHECK(std::as_const(s).and_then(fn).value() == 24);
        CHECK(std::move(std::as_const(s)).and_then(fn).value() == 24);
        CHECK(std::move(s).and_then(fn).value() == 24);
      }

      WHEN("value to error")
      {
        constexpr auto fn = [](int i) -> fn::expected<int, bool> { return std::unexpected<bool>(i >= 1); };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<int, fn::sum<Error, bool>>>);
        CHECK(s.and_then(fn).error() == fn::sum{true});
        CHECK(std::as_const(s).and_then(fn).error() == fn::sum{true});
        CHECK(std::move(std::as_const(s)).and_then(fn).error() == fn::sum{true});
        CHECK(std::move(s).and_then(fn).error() == fn::sum{true});
      }

      WHEN("error")
      {
        fn::expected<int, fn::sum<Error>> s{std::unexpect, fn::sum{FileNotFound}};
        constexpr auto fn = [](int) -> fn::expected<int, bool> { throw 0; };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<int, fn::sum<Error, bool>>>);
        CHECK(s.and_then(fn).error() == fn::sum{FileNotFound});
        CHECK(s.and_then(fn).error() != fn::sum{false});
        CHECK(s.and_then(fn).error() != fn::sum{true});
        CHECK(std::as_const(s).and_then(fn).error() == fn::sum{FileNotFound});
        CHECK(std::move(std::as_const(s)).and_then(fn).error() == fn::sum{FileNotFound});
        CHECK(std::move(s).and_then(fn).error() == fn::sum{FileNotFound});
      }
    }

    WHEN("void to value")
    {
      fn::expected<void, fn::sum<Error>> s{};

      constexpr auto fn1 = []() -> fn::expected<int, bool> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn1)), fn::expected<int, fn::sum<Error, bool>>>);
      constexpr auto fn2 = []() -> fn::expected<int, Error> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn2)), fn::expected<int, fn::sum<Error>>>);
      constexpr auto fn3 = []() -> fn::expected<int, fn::sum<Error>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn3)), fn::expected<int, fn::sum<Error>>>);
      constexpr auto fn4 = []() -> fn::expected<int, fn::sum<bool>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn4)), fn::expected<int, fn::sum<Error, bool>>>);
      constexpr auto fn5 = []() -> fn::expected<int, fn::sum<Error, bool>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn5)), fn::expected<int, fn::sum<Error, bool>>>);
      constexpr auto fn6 = []() -> fn::expected<int, fn::sum<bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn6)), fn::expected<int, fn::sum<Error, bool, int>>>);
      constexpr auto fn7 = []() -> fn::expected<int, fn::sum<Error, bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn7)), fn::expected<int, fn::sum<Error, bool, int>>>);

      WHEN("value to value")
      {
        constexpr auto fn = []() -> fn::expected<int, bool> { return {12}; };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<int, fn::sum<Error, bool>>>);
        CHECK(s.and_then(fn).value() == 12);
        CHECK(std::as_const(s).and_then(fn).value() == 12);
        CHECK(std::move(std::as_const(s)).and_then(fn).value() == 12);
        CHECK(std::move(s).and_then(fn).value() == 12);
      }

      WHEN("value to error")
      {
        constexpr auto fn = []() -> fn::expected<int, bool> { return std::unexpected<bool>(true); };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<int, fn::sum<Error, bool>>>);
        CHECK(s.and_then(fn).error() == fn::sum{true});
        CHECK(std::as_const(s).and_then(fn).error() == fn::sum{true});
        CHECK(std::move(std::as_const(s)).and_then(fn).error() == fn::sum{true});
        CHECK(std::move(s).and_then(fn).error() == fn::sum{true});
      }

      WHEN("error")
      {
        fn::expected<void, fn::sum<Error>> s{std::unexpect, fn::sum{FileNotFound}};
        constexpr auto fn = []() -> fn::expected<int, bool> { throw 0; };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<int, fn::sum<Error, bool>>>);
        CHECK(s.and_then(fn).error() == fn::sum{FileNotFound});
        CHECK(s.and_then(fn).error() != fn::sum{false});
        CHECK(s.and_then(fn).error() != fn::sum{true});
        CHECK(std::as_const(s).and_then(fn).error() == fn::sum{FileNotFound});
        CHECK(std::move(std::as_const(s)).and_then(fn).error() == fn::sum{FileNotFound});
        CHECK(std::move(s).and_then(fn).error() == fn::sum{FileNotFound});
      }
    }

    WHEN("value to void")
    {
      fn::expected<int, fn::sum<Error>> s{12};

      constexpr auto fn1 = [](int) -> fn::expected<void, bool> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn1)), fn::expected<void, fn::sum<Error, bool>>>);
      constexpr auto fn2 = [](int) -> fn::expected<void, Error> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn2)), fn::expected<void, fn::sum<Error>>>);
      constexpr auto fn3 = [](int) -> fn::expected<void, fn::sum<Error>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn3)), fn::expected<void, fn::sum<Error>>>);
      constexpr auto fn4 = [](int) -> fn::expected<void, fn::sum<bool>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn4)), fn::expected<void, fn::sum<Error, bool>>>);
      constexpr auto fn5 = [](int) -> fn::expected<void, fn::sum<Error, bool>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn5)), fn::expected<void, fn::sum<Error, bool>>>);
      constexpr auto fn6 = [](int) -> fn::expected<void, fn::sum<bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn6)), fn::expected<void, fn::sum<Error, bool, int>>>);
      constexpr auto fn7 = [](int) -> fn::expected<void, fn::sum<Error, bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn7)), fn::expected<void, fn::sum<Error, bool, int>>>);

      WHEN("value to value")
      {
        constexpr auto fn = [](int) -> fn::expected<void, bool> { return {}; };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<void, fn::sum<Error, bool>>>);
        CHECK(s.and_then(fn).has_value());
        CHECK(std::as_const(s).and_then(fn).has_value());
        CHECK(std::move(std::as_const(s)).and_then(fn).has_value());
        CHECK(std::move(s).and_then(fn).has_value());
      }

      WHEN("value to error")
      {
        constexpr auto fn = [](int i) -> fn::expected<void, bool> { return std::unexpected<bool>(i >= 1); };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<void, fn::sum<Error, bool>>>);
        CHECK(s.and_then(fn).error() == fn::sum{true});
        CHECK(std::as_const(s).and_then(fn).error() == fn::sum{true});
        CHECK(std::move(std::as_const(s)).and_then(fn).error() == fn::sum{true});
        CHECK(std::move(s).and_then(fn).error() == fn::sum{true});
      }

      WHEN("error")
      {
        fn::expected<int, fn::sum<Error>> s{std::unexpect, fn::sum{FileNotFound}};
        constexpr auto fn = [](int) -> fn::expected<void, bool> { throw 0; };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<void, fn::sum<Error, bool>>>);
        CHECK(s.and_then(fn).error() == fn::sum{FileNotFound});
        CHECK(s.and_then(fn).error() != fn::sum{false});
        CHECK(s.and_then(fn).error() != fn::sum{true});
        CHECK(std::as_const(s).and_then(fn).error() == fn::sum{FileNotFound});
        CHECK(std::move(std::as_const(s)).and_then(fn).error() == fn::sum{FileNotFound});
        CHECK(std::move(s).and_then(fn).error() == fn::sum{FileNotFound});
      }
    }

    WHEN("void to void")
    {
      fn::expected<void, fn::sum<Error>> s{};

      constexpr auto fn1 = []() -> fn::expected<void, bool> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn1)), fn::expected<void, fn::sum<Error, bool>>>);
      constexpr auto fn2 = []() -> fn::expected<void, Error> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn2)), fn::expected<void, fn::sum<Error>>>);
      constexpr auto fn3 = []() -> fn::expected<void, fn::sum<Error>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn3)), fn::expected<void, fn::sum<Error>>>);
      constexpr auto fn4 = []() -> fn::expected<void, fn::sum<bool>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn4)), fn::expected<void, fn::sum<Error, bool>>>);
      constexpr auto fn5 = []() -> fn::expected<void, fn::sum<Error, bool>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn5)), fn::expected<void, fn::sum<Error, bool>>>);
      constexpr auto fn6 = []() -> fn::expected<void, fn::sum<bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn6)), fn::expected<void, fn::sum<Error, bool, int>>>);
      constexpr auto fn7 = []() -> fn::expected<void, fn::sum<Error, bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn7)), fn::expected<void, fn::sum<Error, bool, int>>>);

      WHEN("value to value")
      {
        constexpr auto fn = []() -> fn::expected<void, bool> { return {}; };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<void, fn::sum<Error, bool>>>);
        CHECK(s.and_then(fn).has_value());
        CHECK(std::as_const(s).and_then(fn).has_value());
        CHECK(std::move(std::as_const(s)).and_then(fn).has_value());
        CHECK(std::move(s).and_then(fn).has_value());
      }

      WHEN("value to error")
      {
        constexpr auto fn = []() -> fn::expected<void, bool> { return std::unexpected<bool>(true); };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<void, fn::sum<Error, bool>>>);
        CHECK(s.and_then(fn).error() == fn::sum{true});
        CHECK(std::as_const(s).and_then(fn).error() == fn::sum{true});
        CHECK(std::move(std::as_const(s)).and_then(fn).error() == fn::sum{true});
        CHECK(std::move(s).and_then(fn).error() == fn::sum{true});
      }

      WHEN("error")
      {
        fn::expected<void, fn::sum<Error>> s{std::unexpect, fn::sum{FileNotFound}};
        constexpr auto fn = []() -> fn::expected<void, bool> { throw 0; };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<void, fn::sum<Error, bool>>>);
        CHECK(s.and_then(fn).error() == fn::sum{FileNotFound});
        CHECK(s.and_then(fn).error() != fn::sum{false});
        CHECK(s.and_then(fn).error() != fn::sum{true});
        CHECK(std::as_const(s).and_then(fn).error() == fn::sum{FileNotFound});
        CHECK(std::move(std::as_const(s)).and_then(fn).error() == fn::sum{FileNotFound});
        CHECK(std::move(s).and_then(fn).error() == fn::sum{FileNotFound});
      }
    }
  }

  WHEN("or_else")
  {
    fn::expected<fn::sum<int>, Error> s{std::unexpect, FileNotFound};

    constexpr auto fn1 = [](int) -> fn::expected<Xint, Error> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn1)), fn::expected<fn::sum<Xint, int>, Error>>);
    constexpr auto fn2 = [](int) -> fn::expected<int, Error> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn2)), fn::expected<fn::sum<int>, Error>>);
    constexpr auto fn3 = [](int) -> fn::expected<fn::sum<int>, Error> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn3)), fn::expected<fn::sum<int>, Error>>);
    constexpr auto fn4 = [](int) -> fn::expected<fn::sum<Xint>, Error> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn4)), fn::expected<fn::sum<Xint, int>, Error>>);
    constexpr auto fn5 = [](int) -> fn::expected<fn::sum<Xint, int>, Error> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn5)), fn::expected<fn::sum<Xint, int>, Error>>);
    constexpr auto fn6 = [](int) -> fn::expected<fn::sum<Xint, long>, Error> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn6)), fn::expected<fn::sum<Xint, int, long>, Error>>);
    constexpr auto fn7 = [](int) -> fn::expected<fn::sum<Xint, int, long>, Error> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn7)), fn::expected<fn::sum<Xint, int, long>, Error>>);
    constexpr auto fn8 = [](int) -> fn::expected<fn::sum<Xint, int, long>, std::string> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn8)), fn::expected<fn::sum<Xint, int, long>, std::string>>);

    WHEN("error to value")
    {
      constexpr auto fn = [](Error) -> fn::expected<Xint, std::string> { return {Xint{12}}; };
      static_assert(std::is_same_v<decltype(s.or_else(fn)), fn::expected<fn::sum<Xint, int>, std::string>>);
      CHECK(s.or_else(fn).value() == fn::sum{Xint{12}});
      CHECK(std::as_const(s).or_else(fn).value() == fn::sum{Xint{12}});
      CHECK(std::move(std::as_const(s)).or_else(fn).value() == fn::sum{Xint{12}});
      CHECK(std::move(s).or_else(fn).value() == fn::sum{Xint{12}});
    }

    WHEN("error to error")
    {
      constexpr auto fn = [](Error) -> fn::expected<Xint, std::string> { return std::unexpected<std::string>("Boo"); };
      static_assert(std::is_same_v<decltype(s.or_else(fn)), fn::expected<fn::sum<Xint, int>, std::string>>);
      CHECK(s.or_else(fn).error() == "Boo");
      CHECK(std::as_const(s).or_else(fn).error() == "Boo");
      CHECK(std::move(std::as_const(s)).or_else(fn).error() == "Boo");
      CHECK(std::move(s).or_else(fn).error() == "Boo");
    }

    WHEN("value")
    {
      fn::expected<fn::sum<int>, Error> s{fn::sum{12}};
      constexpr auto fn = [](int) -> fn::expected<Xint, std::string> { throw 0; };
      static_assert(std::is_same_v<decltype(s.or_else(fn)), fn::expected<fn::sum<Xint, int>, std::string>>);
      CHECK(s.or_else(fn).value() == fn::sum{12});
      CHECK(std::as_const(s).or_else(fn).value() == fn::sum{12});
      CHECK(std::move(std::as_const(s)).or_else(fn).value() == fn::sum{12});
      CHECK(std::move(s).or_else(fn).value() == fn::sum{12});
    }
  }
}

TEST_CASE("graded monad constexpr and runtime", "[constexpr][and_then][or_else][expected][graded][sum]")
{
  enum class Error : int { Unknown, InvalidValue };
  using T = fn::expected<int, fn::sum<Error>>;

  WHEN("and_then constexpr")
  {
    WHEN("same error type")
    {
      constexpr auto fn1 = [](int i) -> fn::expected<int, int> {
        if (i < 2)
          return {i + 1};
        return std::unexpected<int>{i};
      };

      constexpr auto r1 = T{0}.and_then(fn1);
      static_assert(std::is_same_v<decltype(r1), fn::expected<int, fn::sum<Error, int>> const>);
      static_assert(r1.value() == 1);
      constexpr auto r2 = r1.and_then(fn1);
      static_assert(r2.value() == 2);
      constexpr auto r3 = r2.and_then(fn1);
      static_assert(r3.error() == fn::sum{2});
      constexpr auto r4 = r3.and_then(fn1);
      static_assert(r4.error() == fn::sum{2});

      SUCCEED();
    }

    WHEN("accummulate errors")
    {
      constexpr auto fn2 = [](int i) -> fn::expected<bool, Error> {
        if (i < 0 || i > 1)
          return std::unexpected<Error>{Error::InvalidValue};
        return {i == 1};
      };

      constexpr auto r2 = T{1}.and_then(fn2);
      static_assert(std::is_same_v<decltype(r2), fn::expected<bool, fn::sum<Error>> const>);
      static_assert(r2.value());

      constexpr auto r3 = T{2}.and_then(fn2);
      static_assert(std::is_same_v<decltype(r3), fn::expected<bool, fn::sum<Error>> const>);
      static_assert(r3.error() == fn::sum{Error::InvalidValue});

      constexpr auto fn3 = [](int i) -> fn::expected<int, int> { return {i + 1}; };
      constexpr auto r4 = r3.and_then(fn3);
      static_assert(std::is_same_v<decltype(r4), fn::expected<int, fn::sum<Error, int>> const>);
      static_assert(r4.error() == fn::sum{Error::InvalidValue});

      constexpr auto r5 = T{2}.and_then(fn3);
      static_assert(std::is_same_v<decltype(r5), fn::expected<int, fn::sum<Error, int>> const>);
      static_assert(r5.value() == 3);

      SUCCEED();
    }
  }

  WHEN("and_then runtime")
  {
    WHEN("same error type")
    {
      constexpr auto fn1 = [](int i) -> fn::expected<int, int> {
        if (i < 2)
          return {i + 1};
        return std::unexpected<int>{i};
      };

      auto const r1 = T{0}.and_then(fn1);
      static_assert(std::is_same_v<decltype(r1), fn::expected<int, fn::sum<Error, int>> const>);
      CHECK(r1.value() == 1);
      auto const r2 = r1.and_then(fn1);
      CHECK(r2.value() == 2);
      auto const r3 = r2.and_then(fn1);
      CHECK(r3.error() == fn::sum{2});
      auto const r4 = r3.and_then(fn1);
      CHECK(r4.error() == fn::sum{2});
    }

    WHEN("accummulate errors")
    {
      constexpr auto fn2 = [](int i) -> fn::expected<bool, Error> {
        if (i < 0 || i > 1)
          return std::unexpected<Error>{Error::InvalidValue};
        return {i == 1};
      };

      auto const r2 = T{1}.and_then(fn2);
      static_assert(std::is_same_v<decltype(r2), fn::expected<bool, fn::sum<Error>> const>);
      CHECK(r2.value());
      auto const r3 = T{2}.and_then(fn2);
      CHECK(r3.error() == fn::sum{Error::InvalidValue});

      auto const fn3 = [](int i) -> fn::expected<int, int> { return {i + 1}; };
      auto const r4 = r3.and_then(fn3);
      static_assert(std::is_same_v<decltype(r4), fn::expected<int, fn::sum<Error, int>> const>);
      CHECK(r4.error() == fn::sum{Error::InvalidValue});
      auto const r5 = T{2}.and_then(fn3);
      CHECK(r5.value() == 3);
    }
  }

  WHEN("or_else constexpr")
  {
    using T = fn::expected<fn::sum<int>, Error>;

    constexpr auto fn1 = [](Error i) -> fn::expected<int, int> {
      if (i == Error::Unknown)
        return {0};
      return std::unexpected<int>{(int)i};
    };

    constexpr auto r1 = T{14}.or_else(fn1);
    static_assert(std::is_same_v<decltype(r1), fn::expected<fn::sum<int>, int> const>);
    static_assert(r1.value() == fn::sum{14});
    constexpr auto r2 = T{std::unexpect, Error::InvalidValue}.or_else(fn1);
    static_assert(r2.error() == 1);
    constexpr auto r3 = T{std::unexpect, Error::Unknown}.or_else(fn1);
    static_assert(r3.value() == fn::sum{0});

    SUCCEED();
  }

  WHEN("or_else runtime")
  {
    using T = fn::expected<fn::sum<int>, Error>;

    constexpr auto fn1 = [](Error i) -> fn::expected<int, int> {
      if (i == Error::Unknown)
        return {0};
      return std::unexpected<int>{(int)i};
    };

    auto const r1 = T{14}.or_else(fn1);
    static_assert(std::is_same_v<decltype(r1), fn::expected<fn::sum<int>, int> const>);
    CHECK(r1.value() == fn::sum{14});
    auto const r2 = T{std::unexpect, Error::InvalidValue}.or_else(fn1);
    CHECK(r2.error() == 1);
    auto const r3 = T{std::unexpect, Error::Unknown}.or_else(fn1);
    CHECK(r3.value() == fn::sum{0});
  }
}

TEST_CASE("expected pack support", "[expected][pack][and_then][transform][operator_and][graded][sum]")
{
  WHEN("and_then")
  {
    WHEN("value")
    {
      fn::expected<fn::pack<int, std::string_view>, Error> s{
          fn::pack<int>{12}.append(std::in_place_type<std::string_view>, "bar")};

      CHECK(s.and_then( //
                 fn::overload([](int &i, auto &&...) -> fn::expected<bool, Error> { return i == 12; },
                              [](int const &, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                              [](int &&, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                              [](int const &&, auto &&...) -> fn::expected<bool, Error> { throw 0; })) //
                .value());
      CHECK(std::as_const(s)
                .and_then( //
                    fn::overload([](int &, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                                 [](int const &i, auto &&...) -> fn::expected<bool, Error> { return i == 12; },
                                 [](int &&, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                                 [](int const &&, auto &&...) -> fn::expected<bool, Error> { throw 0; })) //
                .value());
      CHECK(std::move(std::as_const(s))
                .and_then( //
                    fn::overload([](int &, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                                 [](int const &, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                                 [](int &&, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                                 [](int const &&i, auto &&...) -> fn::expected<bool, Error> { return i == 12; })) //
                .value());
      CHECK(std::move(s)
                .and_then( //
                    fn::overload([](int &, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                                 [](int const &, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                                 [](int &&i, auto &&...) -> fn::expected<bool, Error> { return i == 12; },
                                 [](int const &&, auto &&...) -> fn::expected<bool, Error> { throw 0; })) //
                .value());
    }

    WHEN("error")
    {
      fn::expected<fn::pack<int, std::string_view>, Error> s{std::unexpect, FileNotFound};
      CHECK(s.and_then( //
                 [](auto...) -> fn::expected<bool, Error> { throw 0; })
                .error()
            == FileNotFound);
      CHECK(std::as_const(s)
                .and_then( //
                    [](auto...) -> fn::expected<bool, Error> { throw 0; })
                .error()
            == FileNotFound);
      CHECK(std::move(std::as_const(s))
                .and_then( //
                    [](auto...) -> fn::expected<bool, Error> { throw 0; })
                .error()
            == FileNotFound);
      CHECK(std::move(s)
                .and_then( //
                    [](auto...) -> fn::expected<bool, Error> { throw 0; })
                .error()
            == FileNotFound);
    }
  }

  WHEN("transform")
  {
    WHEN("value")
    {
      fn::expected<fn::pack<int, std::string_view>, Error> s{
          fn::pack<int>{12}.append(std::in_place_type<std::string_view>, "bar")};

      CHECK(s.transform( //
                 fn::overload([](int &i, auto &&...) -> bool { return i == 12; },
                              [](int const &, auto &&...) -> bool { throw 0; },
                              [](int &&, auto &&...) -> bool { throw 0; },
                              [](int const &&, auto &&...) -> bool { throw 0; })) //
                .value());
      CHECK(std::as_const(s)
                .transform( //
                    fn::overload([](int &, auto &&...) -> bool { throw 0; },
                                 [](int const &i, auto &&...) -> bool { return i == 12; },
                                 [](int &&, auto &&...) -> bool { throw 0; },
                                 [](int const &&, auto &&...) -> bool { throw 0; })) //
                .value());
      CHECK(std::move(std::as_const(s))
                .transform( //
                    fn::overload([](int &, auto &&...) -> bool { throw 0; },
                                 [](int const &, auto &&...) -> bool { throw 0; },
                                 [](int &&, auto &&...) -> bool { throw 0; },
                                 [](int const &&i, auto &&...) -> bool { return i == 12; })) //
                .value());
      CHECK(std::move(s)
                .transform( //
                    fn::overload([](int &, auto &&...) -> bool { throw 0; },
                                 [](int const &, auto &&...) -> bool { throw 0; },
                                 [](int &&i, auto &&...) -> bool { return i == 12; },
                                 [](int const &&, auto &&...) -> bool { throw 0; })) //
                .value());
    }

    WHEN("void result")
    {
      fn::expected<fn::pack<int, std::string_view>, Error> s{
          fn::pack<int>{12}.append(std::in_place_type<std::string_view>, "bar")};

      CHECK(s.transform( //
                 fn::overload([](int &, auto &&...) -> void {}, [](int const &, auto &&...) -> void { throw 0; },
                              [](int &&, auto &&...) -> void { throw 0; },
                              [](int const &&, auto &&...) -> void { throw 0; })) //
                .has_value());
      CHECK(std::as_const(s)
                .transform( //
                    fn::overload([](int &, auto &&...) -> void { throw 0; }, [](int const &, auto &&...) -> void {},
                                 [](int &&, auto &&...) -> void { throw 0; },
                                 [](int const &&, auto &&...) -> void { throw 0; })) //
                .has_value());
      CHECK(std::move(std::as_const(s))
                .transform( //
                    fn::overload(
                        [](int &, auto &&...) -> void { throw 0; }, [](int const &, auto &&...) -> void { throw 0; },
                        [](int &&, auto &&...) -> void { throw 0; }, [](int const &&, auto &&...) -> void {})) //
                .has_value());
      CHECK(std::move(s)
                .transform( //
                    fn::overload([](int &, auto &&...) -> void { throw 0; },
                                 [](int const &, auto &&...) -> void { throw 0; }, [](int &&, auto &&...) -> void {},
                                 [](int const &&, auto &&...) -> void { throw 0; })) //
                .has_value());
    }

    WHEN("error")
    {
      fn::expected<fn::pack<int, std::string_view>, Error> s{std::unexpect, FileNotFound};
      CHECK(s.transform([](auto...) -> bool { throw 0; }).error() == FileNotFound);
      CHECK(std::as_const(s).transform([](auto...) -> bool { throw 0; }).error() == FileNotFound);
      CHECK(fn::expected<fn::pack<int, std::string_view>, Error>{std::unexpect, FileNotFound}
                .transform([](auto...) -> bool { throw 0; })
                .error()
            == FileNotFound);
      CHECK(std::move(std::as_const(s)).transform([](auto...) -> bool { throw 0; }).error() == FileNotFound);
    }
  }

  WHEN("operator &")
  {
    WHEN("same error type")
    {
      WHEN("value & void yield value")
      {
        static_assert(
            std::same_as<decltype(std::declval<fn::expected<int, Error>>() & std::declval<fn::expected<void, Error>>()),
                         fn::expected<int, Error>>);

        CHECK((fn::expected<int, Error>{42} //
               & fn::expected<void, Error>{})
                  .value()
              == 42);
        CHECK((fn::expected<int, Error>{std::unexpect, FileNotFound} //
               & fn::expected<void, Error>{})
                  .error()
              == FileNotFound);
        CHECK((fn::expected<int, Error>{42} //
               & fn::expected<void, Error>{std::unexpect, Unknown})
                  .error()
              == Unknown);
        CHECK((fn::expected<int, Error>{std::unexpect, FileNotFound} //
               & fn::expected<void, Error>{std::unexpect, Unknown})
                  .error()
              == FileNotFound);
      }

      WHEN("void & value yield value")
      {
        static_assert(
            std::same_as<decltype(std::declval<fn::expected<void, Error>>() & std::declval<fn::expected<int, Error>>()),
                         fn::expected<int, Error>>);

        CHECK((fn::expected<void, Error>{} //
               & fn::expected<int, Error>{12})
                  .value()
              == 12);
        CHECK((fn::expected<void, Error>{std::unexpect, FileNotFound} //
               & fn::expected<int, Error>{12})
                  .error()
              == FileNotFound);
        CHECK((fn::expected<void, Error>{} //
               & fn::expected<int, Error>{std::unexpect, Unknown})
                  .error()
              == Unknown);
        CHECK((fn::expected<void, Error>{std::unexpect, FileNotFound} //
               & fn::expected<int, Error>{std::unexpect, Unknown})
                  .error()
              == FileNotFound);
      }

      WHEN("void & void yield void")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<void, Error>>()
                                            & std::declval<fn::expected<void, Error>>()),
                                   fn::expected<void, Error>>);

        CHECK((fn::expected<void, Error>{} //
               & fn::expected<void, Error>{})
                  .has_value());
        CHECK((fn::expected<void, Error>{std::unexpect, FileNotFound} //
               & fn::expected<void, Error>{})
                  .error()
              == FileNotFound);
        CHECK((fn::expected<void, Error>{} //
               & fn::expected<void, Error>{std::unexpect, Unknown})
                  .error()
              == Unknown);
        CHECK((fn::expected<void, Error>{std::unexpect, FileNotFound} //
               & fn::expected<void, Error>{std::unexpect, Unknown})
                  .error()
              == FileNotFound);
      }

      WHEN("value & value yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<int, Error>>()
                                            & std::declval<fn::expected<double, Error>>()),
                                   fn::expected<fn::pack<int, double>, Error>>);

        CHECK((fn::expected<double, Error>{0.5} //
               & fn::expected<int, Error>{12})
                  .transform([](double d, int i) constexpr -> bool { return d == 0.5 && i == 12; })
                  .value());
        CHECK((fn::expected<double, Error>{std::unexpect, FileNotFound} //
               & fn::expected<int, Error>{12})
                  .error()
              == FileNotFound);
        CHECK((fn::expected<double, Error>{} //
               & fn::expected<int, Error>{std::unexpect, Unknown})
                  .error()
              == Unknown);
        CHECK((fn::expected<double, Error>{std::unexpect, FileNotFound} //
               & fn::expected<int, Error>{std::unexpect, Unknown})
                  .error()
              == FileNotFound);
      }

      WHEN("pack & value yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<fn::pack<double, bool>, Error>>()
                                            & std::declval<fn::expected<int, Error>>()),
                                   fn::expected<fn::pack<double, bool, int>, Error>>);

        CHECK((fn::expected<fn::pack<double, bool>, Error>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<int, Error>{12})
                  .transform([](double d, bool b, int i) constexpr -> bool { return d == 0.5 && b && i == 12; })
                  .value());
        CHECK((fn::expected<fn::pack<double, bool>, Error>{std::unexpect, FileNotFound} //
               & fn::expected<int, Error>{12})
                  .error()
              == FileNotFound);
        CHECK((fn::expected<fn::pack<double, bool>, Error>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<int, Error>{std::unexpect, Unknown})
                  .error()
              == Unknown);
        CHECK((fn::expected<fn::pack<double, bool>, Error>{std::unexpect, FileNotFound} //
               & fn::expected<int, Error>{std::unexpect, Unknown})
                  .error()
              == FileNotFound);
      }

      WHEN("pack & void yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<fn::pack<double, bool>, Error>>()
                                            & std::declval<fn::expected<void, Error>>()),
                                   fn::expected<fn::pack<double, bool>, Error>>);

        CHECK((fn::expected<fn::pack<double, bool>, Error>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<void, Error>{})
                  .transform([](double d, bool b) constexpr -> bool { return d == 0.5 && b; })
                  .value());
        CHECK((fn::expected<fn::pack<double, bool>, Error>{std::unexpect, FileNotFound} //
               & fn::expected<void, Error>{})
                  .error()
              == FileNotFound);
        CHECK((fn::expected<fn::pack<double, bool>, Error>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<void, Error>{std::unexpect, Unknown})
                  .error()
              == Unknown);
        CHECK((fn::expected<fn::pack<double, bool>, Error>{std::unexpect, FileNotFound} //
               & fn::expected<void, Error>{std::unexpect, Unknown})
                  .error()
              == FileNotFound);
      }

      WHEN("void & pack yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<void, Error>>()
                                            & std::declval<fn::expected<fn::pack<double, bool>, Error>>()),
                                   fn::expected<fn::pack<double, bool>, Error>>);

        CHECK((fn::expected<void, Error>{} //
               & fn::expected<fn::pack<double, bool>, Error>{std::in_place, fn::pack<double, bool>{0.5, true}})
                  .transform([](double d, bool b) constexpr -> bool { return d == 0.5 && b; })
                  .value());
        CHECK((fn::expected<void, Error>{std::unexpect, FileNotFound} //
               & fn::expected<fn::pack<double, bool>, Error>{std::in_place, fn::pack<double, bool>{0.5, true}})
                  .error()
              == FileNotFound);
        CHECK((fn::expected<void, Error>{} //
               & fn::expected<fn::pack<double, bool>, Error>{std::unexpect, Unknown})
                  .error()
              == Unknown);
        CHECK((fn::expected<void, Error>{std::unexpect, FileNotFound} //
               & fn::expected<fn::pack<double, bool>, Error>{std::unexpect, Unknown})
                  .error()
              == FileNotFound);
      }

      WHEN("value & pack is unsupported")
      {
        static_assert(
            [](auto &&lh,                                                          //
               auto &&rh) constexpr -> bool { return not requires { lh & rh; }; }( //
                                        fn::expected<int, Error>{12}, fn::expected<fn::pack<double, bool>, Error>{
                                                                          fn::pack<double, bool>{0.5, true}}));
      }
      WHEN("pack & pack is unsupported")
      {
        static_assert([](auto &&lh, auto &&rh) constexpr
                      -> bool { return not requires { lh & rh; }; }( //
                          fn::expected<fn::pack<double, bool>, Error>{fn::pack<double, bool>{0.5, true}},
                          fn::expected<fn::pack<double, bool>, Error>{fn::pack<double, bool>{0.5, true}}));
      }

      WHEN("sum on both sides")
      {
        using Lh = fn::expected<fn::sum<double, int>, Error>;
        using Rh = fn::expected<fn::sum<bool, int>, Error>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::sum< //
                                                    fn::pack<double, bool>, fn::pack<double, int>, fn::pack<int, bool>,
                                                    fn::pack<int, int>>,
                                                Error>>);

        CHECK((Lh{fn::sum{0.5}} & Rh{fn::sum{12}})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::sum{true});
        CHECK((Lh{std::unexpect, FileNotFound} & Rh{fn::sum{12}}).error() == FileNotFound);
        CHECK((Lh{fn::sum{0.5}} & Rh{std::unexpect, Unknown}).error() == Unknown);
        CHECK((Lh{std::unexpect, FileNotFound} & Rh{std::unexpect, Unknown}).error() == FileNotFound);

        WHEN("sum of packs on left")
        {
          using Lh = fn::expected<fn::sum<fn::pack<double, bool>, fn::pack<double, int>>, Error>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::sum< //
                                                      fn::pack<double, bool, bool>, fn::pack<double, bool, int>,
                                                      fn::pack<double, int, bool>, fn::pack<double, int, int>>,
                                                  Error>>);

          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{fn::sum{12}})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                    })
                    .value()
                == fn::sum{true});
          CHECK((Lh{std::unexpect, FileNotFound} & Rh{fn::sum{12}}).error() == FileNotFound);
          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{std::unexpect, Unknown}).error() == Unknown);
          CHECK((Lh{std::unexpect, FileNotFound} & Rh{std::unexpect, Unknown}).error() == FileNotFound);
        }
      }

      WHEN("sum on left side only")
      {
        using Lh = fn::expected<fn::sum<double, int>, Error>;
        using Rh = fn::expected<int, Error>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::sum< //
                                                    fn::pack<double, int>, fn::pack<int, int>>,
                                                Error>>);

        CHECK((Lh{fn::sum{0.5}} & Rh{12})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::sum{true});
        CHECK((Lh{std::unexpect, FileNotFound} & Rh{12}).error() == FileNotFound);
        CHECK((Lh{fn::sum{0.5}} & Rh{std::unexpect, Unknown}).error() == Unknown);
        CHECK((Lh{std::unexpect, FileNotFound} & Rh{std::unexpect, Unknown}).error() == FileNotFound);

        WHEN("sum of packs on left")
        {
          using Lh = fn::expected<fn::sum<fn::pack<double, bool>, fn::pack<double, int>>, Error>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::sum< //
                                                      fn::pack<double, bool, int>, fn::pack<double, int, int>>,
                                                  Error>>);

          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{12})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                    })
                    .value()
                == fn::sum{true});
          CHECK((Lh{std::unexpect, FileNotFound} & Rh{12}).error() == FileNotFound);
          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{std::unexpect, Unknown}).error() == Unknown);
          CHECK((Lh{std::unexpect, FileNotFound} & Rh{std::unexpect, Unknown}).error() == FileNotFound);
        }
      }

      WHEN("sum on right side only")
      {
        using Lh = fn::expected<double, Error>;
        using Rh = fn::expected<fn::sum<bool, int>, Error>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::sum< //
                                                    fn::pack<double, bool>, fn::pack<double, int>>,
                                                Error>>);

        CHECK((Lh{0.5} & Rh{fn::sum{12}})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::sum{true});
        CHECK((Lh{std::unexpect, FileNotFound} & Rh{fn::sum{12}}).error() == FileNotFound);
        CHECK((Lh{0.5} & Rh{std::unexpect, Unknown}).error() == Unknown);
        CHECK((Lh{std::unexpect, FileNotFound} & Rh{std::unexpect, Unknown}).error() == FileNotFound);

        WHEN("pack on left")
        {
          using Lh = fn::expected<fn::pack<double, int>, Error>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::sum< //
                                                      fn::pack<double, int, bool>, fn::pack<double, int, int>>,
                                                  Error>>);

          CHECK((Lh{fn::pack{0.5, 3}} & Rh{fn::sum{12}})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                    })
                    .value()
                == fn::sum{true});
          CHECK((Lh{std::unexpect, FileNotFound} & Rh{fn::sum{12}}).error() == FileNotFound);
          CHECK((Lh{fn::pack{0.5, 3}} & Rh{std::unexpect, Unknown}).error() == Unknown);
          CHECK((Lh{std::unexpect, FileNotFound} & Rh{std::unexpect, Unknown}).error() == FileNotFound);
        }
      }
    }

    WHEN("graded monad as left operand")
    {
      static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::sum<Error>>>()
                                          & std::declval<fn::expected<void, Error>>()),
                                 fn::expected<int, fn::sum<Error>>>);

      static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::sum<Error>>>()
                                          & std::declval<fn::expected<void, fn::sum<Error>>>()),
                                 fn::expected<int, fn::sum<Error>>>);

      static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::sum<Error>>>()
                                          & std::declval<fn::expected<void, fn::sum<int>>>()),
                                 fn::expected<int, fn::sum<Error, int>>>);

      static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::sum<Error>>>()
                                          & std::declval<fn::expected<void, fn::sum<bool, int>>>()),
                                 fn::expected<int, fn::sum<Error, bool, int>>>);

      static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::sum<bool, int>>>()
                                          & std::declval<fn::expected<void, fn::sum<Error>>>()),
                                 fn::expected<int, fn::sum<Error, bool, int>>>);

      WHEN("value & void yield value")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::sum<Error>>>()
                                            & std::declval<fn::expected<void, int>>()),
                                   fn::expected<int, fn::sum<Error, int>>>);

        CHECK((fn::expected<int, fn::sum<Error>>{42} //
               & fn::expected<void, int>{})
                  .value()
              == 42);
        CHECK((fn::expected<int, fn::sum<Error>>{std::unexpect, fn::sum{FileNotFound}} //
               & fn::expected<void, int>{})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<int, fn::sum<Error>>{42} //
               & fn::expected<void, int>{std::unexpect, 13})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<int, fn::sum<Error>>{std::unexpect, FileNotFound} //
               & fn::expected<void, int>{std::unexpect, 13})
                  .error()
              == fn::sum{FileNotFound});
      }

      WHEN("void & value yield value")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<void, fn::sum<Error>>>()
                                            & std::declval<fn::expected<int, int>>()),
                                   fn::expected<int, fn::sum<Error, int>>>);

        CHECK((fn::expected<void, fn::sum<Error>>{} //
               & fn::expected<int, int>{12})
                  .value()
              == 12);
        CHECK((fn::expected<void, fn::sum<Error>>{std::unexpect, FileNotFound} //
               & fn::expected<int, int>{12})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<void, fn::sum<Error>>{} //
               & fn::expected<int, int>{std::unexpect, 13})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<void, fn::sum<Error>>{std::unexpect, FileNotFound} //
               & fn::expected<int, int>{std::unexpect, 13})
                  .error()
              == fn::sum{FileNotFound});
      }

      WHEN("void & void yield void")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<void, fn::sum<Error>>>()
                                            & std::declval<fn::expected<void, int>>()),
                                   fn::expected<void, fn::sum<Error, int>>>);

        CHECK((fn::expected<void, fn::sum<Error>>{} //
               & fn::expected<void, int>{})
                  .has_value());
        CHECK((fn::expected<void, fn::sum<Error>>{std::unexpect, FileNotFound} //
               & fn::expected<void, int>{})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<void, fn::sum<Error>>{} //
               & fn::expected<void, int>{std::unexpect, 13})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<void, fn::sum<Error>>{std::unexpect, FileNotFound} //
               & fn::expected<void, int>{std::unexpect, 13})
                  .error()
              == fn::sum{FileNotFound});
      }

      WHEN("value & value yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::sum<Error>>>()
                                            & std::declval<fn::expected<double, int>>()),
                                   fn::expected<fn::pack<int, double>, fn::sum<Error, int>>>);

        CHECK((fn::expected<double, fn::sum<Error>>{0.5} //
               & fn::expected<int, int>{12})
                  .transform([](double d, int i) constexpr -> bool { return d == 0.5 && i == 12; })
                  .value());
        CHECK((fn::expected<double, fn::sum<Error>>{std::unexpect, FileNotFound} //
               & fn::expected<int, int>{12})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<double, fn::sum<Error>>{} //
               & fn::expected<int, int>{std::unexpect, 13})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<double, fn::sum<Error>>{std::unexpect, FileNotFound} //
               & fn::expected<int, int>{std::unexpect, 13})
                  .error()
              == fn::sum{FileNotFound});
      }

      WHEN("pack & value yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<fn::pack<double, bool>, fn::sum<Error>>>()
                                            & std::declval<fn::expected<int, int>>()),
                                   fn::expected<fn::pack<double, bool, int>, fn::sum<Error, int>>>);

        CHECK((fn::expected<fn::pack<double, bool>, fn::sum<Error>>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<int, int>{12})
                  .transform([](double d, bool b, int i) constexpr -> bool { return d == 0.5 && b && i == 12; })
                  .value());
        CHECK((fn::expected<fn::pack<double, bool>, fn::sum<Error>>{std::unexpect, FileNotFound} //
               & fn::expected<int, int>{12})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<fn::pack<double, bool>, fn::sum<Error>>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<int, int>{std::unexpect, 13})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<fn::pack<double, bool>, fn::sum<Error>>{std::unexpect, FileNotFound} //
               & fn::expected<int, int>{std::unexpect, 13})
                  .error()
              == fn::sum{FileNotFound});
      }

      WHEN("pack & void yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<fn::pack<double, bool>, fn::sum<Error>>>()
                                            & std::declval<fn::expected<void, int>>()),
                                   fn::expected<fn::pack<double, bool>, fn::sum<Error, int>>>);

        CHECK((fn::expected<fn::pack<double, bool>, fn::sum<Error>>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<void, int>{})
                  .transform([](double d, bool b) constexpr -> bool { return d == 0.5 && b; })
                  .value());
        CHECK((fn::expected<fn::pack<double, bool>, fn::sum<Error>>{std::unexpect, FileNotFound} //
               & fn::expected<void, int>{})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<fn::pack<double, bool>, fn::sum<Error>>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<void, int>{std::unexpect, 13})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<fn::pack<double, bool>, fn::sum<Error>>{std::unexpect, FileNotFound} //
               & fn::expected<void, int>{std::unexpect, 13})
                  .error()
              == fn::sum{FileNotFound});
      }

      WHEN("void & pack yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<void, fn::sum<Error>>>()
                                            & std::declval<fn::expected<fn::pack<double, bool>, int>>()),
                                   fn::expected<fn::pack<double, bool>, fn::sum<Error, int>>>);

        CHECK((fn::expected<void, fn::sum<Error>>{} //
               & fn::expected<fn::pack<double, bool>, int>{std::in_place, fn::pack<double, bool>{0.5, true}})
                  .transform([](double d, bool b) constexpr -> bool { return d == 0.5 && b; })
                  .value());
        CHECK((fn::expected<void, fn::sum<Error>>{std::unexpect, FileNotFound} //
               & fn::expected<fn::pack<double, bool>, int>{std::in_place, fn::pack<double, bool>{0.5, true}})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<void, fn::sum<Error>>{} //
               & fn::expected<fn::pack<double, bool>, int>{std::unexpect, 13})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<void, fn::sum<Error>>{std::unexpect, FileNotFound} //
               & fn::expected<fn::pack<double, bool>, int>{std::unexpect, 13})
                  .error()
              == fn::sum{FileNotFound});
      }

      WHEN("sum on both sides")
      {
        using Lh = fn::expected<fn::sum<double, int>, fn::sum<Error>>;
        using Rh = fn::expected<fn::sum<bool, int>, int>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::sum< //
                                                    fn::pack<double, bool>, fn::pack<double, int>, fn::pack<int, bool>,
                                                    fn::pack<int, int>>,
                                                fn::sum<Error, int>>>);

        CHECK((Lh{fn::sum{0.5}} & Rh{fn::sum{12}})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::sum{true});
        CHECK((Lh{std::unexpect, fn::sum{FileNotFound}} & Rh{fn::sum{12}}).error() == fn::sum{FileNotFound});
        CHECK((Lh{fn::sum{0.5}} & Rh{std::unexpect, 13}).error() == fn::sum{13});
        CHECK((Lh{std::unexpect, fn::sum{FileNotFound}} & Rh{std::unexpect, 13}).error() == fn::sum{FileNotFound});

        WHEN("sum of packs on left")
        {
          using Lh = fn::expected<fn::sum<fn::pack<double, bool>, fn::pack<double, int>>, fn::sum<Error>>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::sum< //
                                                      fn::pack<double, bool, bool>, fn::pack<double, bool, int>,
                                                      fn::pack<double, int, bool>, fn::pack<double, int, int>>,
                                                  fn::sum<Error, int>>>);

          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{fn::sum{12}})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                    })
                    .value()
                == fn::sum{true});
          CHECK((Lh{std::unexpect, fn::sum{FileNotFound}} & Rh{fn::sum{12}}).error() == fn::sum{FileNotFound});
          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{std::unexpect, 13}).error() == fn::sum{13});
          CHECK((Lh{std::unexpect, fn::sum{FileNotFound}} & Rh{std::unexpect, 13}).error() == fn::sum{FileNotFound});
        }
      }

      WHEN("sum on left side only")
      {
        using Lh = fn::expected<fn::sum<double, int>, fn::sum<Error>>;
        using Rh = fn::expected<int, int>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::sum< //
                                                    fn::pack<double, int>, fn::pack<int, int>>,
                                                fn::sum<Error, int>>>);

        CHECK((Lh{fn::sum{0.5}} & Rh{12})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::sum{true});
        CHECK((Lh{std::unexpect, fn::sum{FileNotFound}} & Rh{12}).error() == fn::sum{FileNotFound});
        CHECK((Lh{fn::sum{0.5}} & Rh{std::unexpect, 13}).error() == fn::sum{13});
        CHECK((Lh{std::unexpect, fn::sum{FileNotFound}} & Rh{std::unexpect, 13}).error() == fn::sum{FileNotFound});

        WHEN("sum of packs on left")
        {
          using Lh = fn::expected<fn::sum<fn::pack<double, bool>, fn::pack<double, int>>, fn::sum<Error>>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::sum< //
                                                      fn::pack<double, bool, int>, fn::pack<double, int, int>>,
                                                  fn::sum<Error, int>>>);

          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{12})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                    })
                    .value()
                == fn::sum{true});
          CHECK((Lh{std::unexpect, fn::sum{FileNotFound}} & Rh{12}).error() == fn::sum{FileNotFound});
          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{std::unexpect, 13}).error() == fn::sum{13});
          CHECK((Lh{std::unexpect, fn::sum{FileNotFound}} & Rh{std::unexpect, 13}).error() == fn::sum{FileNotFound});
        }
      }

      WHEN("sum on right side only")
      {
        using Lh = fn::expected<double, fn::sum<Error>>;
        using Rh = fn::expected<fn::sum<bool, int>, int>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::sum< //
                                                    fn::pack<double, bool>, fn::pack<double, int>>,
                                                fn::sum<Error, int>>>);

        CHECK((Lh{0.5} & Rh{fn::sum{12}})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::sum{true});
        CHECK((Lh{std::unexpect, fn::sum{FileNotFound}} & Rh{fn::sum{12}}).error() == fn::sum{FileNotFound});
        CHECK((Lh{0.5} & Rh{std::unexpect, 13}).error() == fn::sum{13});
        CHECK((Lh{std::unexpect, fn::sum{FileNotFound}} & Rh{std::unexpect, 13}).error() == fn::sum{FileNotFound});

        WHEN("pack on left")
        {
          using Lh = fn::expected<fn::pack<double, int>, fn::sum<Error>>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::sum< //
                                                      fn::pack<double, int, bool>, fn::pack<double, int, int>>,
                                                  fn::sum<Error, int>>>);

          CHECK((Lh{fn::pack{0.5, 3}} & Rh{fn::sum{12}})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                    })
                    .value()
                == fn::sum{true});
          CHECK((Lh{std::unexpect, fn::sum{FileNotFound}} & Rh{fn::sum{12}}).error() == fn::sum{FileNotFound});
          CHECK((Lh{fn::pack{0.5, 3}} & Rh{std::unexpect, 13}).error() == fn::sum{13});
          CHECK((Lh{std::unexpect, fn::sum{FileNotFound}} & Rh{std::unexpect, 13}).error() == fn::sum{FileNotFound});
        }
      }
    }

    WHEN("graded monad as right operand")
    {
      static_assert(std::same_as<decltype(std::declval<fn::expected<void, Error>>()
                                          & std::declval<fn::expected<int, fn::sum<Error>>>()),
                                 fn::expected<int, fn::sum<Error>>>);

      static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::sum<Error>>>()
                                          & std::declval<fn::expected<void, fn::sum<Error>>>()),
                                 fn::expected<int, fn::sum<Error>>>);

      static_assert(std::same_as<decltype(std::declval<fn::expected<void, fn::sum<int>>>()
                                          & std::declval<fn::expected<int, fn::sum<Error>>>()),
                                 fn::expected<int, fn::sum<Error, int>>>);

      static_assert(std::same_as<decltype(std::declval<fn::expected<void, fn::sum<bool, int>>>()
                                          & std::declval<fn::expected<int, fn::sum<Error>>>()),
                                 fn::expected<int, fn::sum<Error, bool, int>>>);

      static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::sum<bool, int>>>()
                                          & std::declval<fn::expected<void, fn::sum<Error>>>()),
                                 fn::expected<int, fn::sum<Error, bool, int>>>);

      WHEN("value & void & yield value")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<int, int>>()
                                            & std::declval<fn::expected<void, fn::sum<Error>>>()),
                                   fn::expected<int, fn::sum<Error, int>>>);

        CHECK((fn::expected<int, int>{12} //
               & fn::expected<void, fn::sum<Error>>{})
                  .value()
              == 12);
        CHECK((fn::expected<int, int>{12} //
               & fn::expected<void, fn::sum<Error>>{std::unexpect, FileNotFound})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<int, int>{std::unexpect, 13} //
               & fn::expected<void, fn::sum<Error>>{})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<int, int>{std::unexpect, 13} //
               & fn::expected<void, fn::sum<Error>>{std::unexpect, FileNotFound})
                  .error()
              == fn::sum{13});
      }

      WHEN("void & value yield value")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<void, int>>()
                                            & std::declval<fn::expected<int, fn::sum<Error>>>()),
                                   fn::expected<int, fn::sum<Error, int>>>);

        CHECK((fn::expected<void, int>{} //
               & fn::expected<int, fn::sum<Error>>{42})
                  .value()
              == 42);
        CHECK((fn::expected<void, int>{} //
               & fn::expected<int, fn::sum<Error>>{std::unexpect, fn::sum{FileNotFound}})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<void, int>{std::unexpect, 13} //
               & fn::expected<int, fn::sum<Error>>{42})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<void, int>{std::unexpect, 13} //
               & fn::expected<int, fn::sum<Error>>{std::unexpect, FileNotFound})
                  .error()
              == fn::sum{13});
      }

      WHEN("void & void yield void")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<void, int>>()
                                            & std::declval<fn::expected<void, fn::sum<Error>>>()),
                                   fn::expected<void, fn::sum<Error, int>>>);

        CHECK((fn::expected<void, int>{} //
               & fn::expected<void, fn::sum<Error>>{})
                  .has_value());
        CHECK((fn::expected<void, int>{} //
               & fn::expected<void, fn::sum<Error>>{std::unexpect, FileNotFound})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<void, int>{std::unexpect, 13} //
               & fn::expected<void, fn::sum<Error>>{})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<void, int>{std::unexpect, 13} //
               & fn::expected<void, fn::sum<Error>>{std::unexpect, FileNotFound})
                  .error()
              == fn::sum{13});
      }

      WHEN("value & value yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<double, int>>()
                                            & std::declval<fn::expected<int, fn::sum<Error>>>()),
                                   fn::expected<fn::pack<double, int>, fn::sum<Error, int>>>);

        CHECK((fn::expected<double, int>{0.5} //
               & fn::expected<int, fn::sum<Error>>{12})
                  .transform([](double d, int i) constexpr -> bool { return d == 0.5 && i == 12; })
                  .value());
        CHECK((fn::expected<double, int>{0.5} //
               & fn::expected<int, fn::sum<Error>>{std::unexpect, FileNotFound})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<double, int>{std::unexpect, 13} //
               & fn::expected<int, fn::sum<Error>>{12})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<double, int>{std::unexpect, 13} //
               & fn::expected<int, fn::sum<Error>>{std::unexpect, FileNotFound})
                  .error()
              == fn::sum{13});
      }

      WHEN("pack & value yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<fn::pack<double, bool>, int>>()
                                            & std::declval<fn::expected<int, fn::sum<Error>>>()),
                                   fn::expected<fn::pack<double, bool, int>, fn::sum<Error, int>>>);

        CHECK((fn::expected<fn::pack<double, bool>, int>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<int, fn::sum<Error>>{12})
                  .transform([](double d, bool b, int i) constexpr -> bool { return d == 0.5 && b && i == 12; })
                  .value());
        CHECK((fn::expected<fn::pack<double, bool>, int>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<int, fn::sum<Error>>{std::unexpect, FileNotFound})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<fn::pack<double, bool>, int>{std::unexpect, 13} //
               & fn::expected<int, fn::sum<Error>>{12})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<fn::pack<double, bool>, int>{std::unexpect, 13} //
               & fn::expected<int, fn::sum<Error>>{std::unexpect, FileNotFound})
                  .error()
              == fn::sum{13});
      }

      WHEN("pack & void yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<fn::pack<double, bool>, int>>()
                                            & std::declval<fn::expected<void, fn::sum<Error>>>()),
                                   fn::expected<fn::pack<double, bool>, fn::sum<Error, int>>>);

        CHECK((fn::expected<fn::pack<double, bool>, int>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<void, fn::sum<Error>>{})
                  .transform([](double d, bool b) constexpr -> bool { return d == 0.5 && b; })
                  .value());
        CHECK((fn::expected<fn::pack<double, bool>, int>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<void, fn::sum<Error>>{std::unexpect, FileNotFound})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<fn::pack<double, bool>, int>{std::unexpect, 13} //
               & fn::expected<void, fn::sum<Error>>{})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<fn::pack<double, bool>, int>{std::unexpect, 13} //
               & fn::expected<void, fn::sum<Error>>{std::unexpect, FileNotFound})
                  .error()
              == fn::sum{13});
      }

      WHEN("void & pack yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<void, int>>()
                                            & std::declval<fn::expected<fn::pack<double, bool>, fn::sum<Error>>>()),
                                   fn::expected<fn::pack<double, bool>, fn::sum<Error, int>>>);

        CHECK((fn::expected<void, int>{} //
               & fn::expected<fn::pack<double, bool>, fn::sum<Error>>{std::in_place, fn::pack<double, bool>{0.5, true}})
                  .transform([](double d, bool b) constexpr -> bool { return d == 0.5 && b; })
                  .value());
        CHECK((fn::expected<void, int>{} //
               & fn::expected<fn::pack<double, bool>, fn::sum<Error>>{std::unexpect, FileNotFound})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<void, int>{std::unexpect, 13} //
               & fn::expected<fn::pack<double, bool>, fn::sum<Error>>{std::in_place, fn::pack<double, bool>{0.5, true}})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<void, int>{std::unexpect, 13} //
               & fn::expected<fn::pack<double, bool>, fn::sum<Error>>{std::unexpect, FileNotFound})
                  .error()
              == fn::sum{13});
      }

      WHEN("sum on both sides")
      {
        using Lh = fn::expected<fn::sum<double, int>, Error>;
        using Rh = fn::expected<fn::sum<bool, int>, fn::sum<int>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::sum< //
                                                    fn::pack<double, bool>, fn::pack<double, int>, fn::pack<int, bool>,
                                                    fn::pack<int, int>>,
                                                fn::sum<Error, int>>>);

        CHECK((Lh{fn::sum{0.5}} & Rh{fn::sum{12}})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::sum{true});
        CHECK((Lh{std::unexpect, FileNotFound} & Rh{fn::sum{12}}).error() == fn::sum{FileNotFound});
        CHECK((Lh{fn::sum{0.5}} & Rh{std::unexpect, fn::sum{13}}).error() == fn::sum{13});
        CHECK((Lh{std::unexpect, FileNotFound} & Rh{std::unexpect, fn::sum{13}}).error() == fn::sum{FileNotFound});

        WHEN("sum of packs on left")
        {
          using Lh = fn::expected<fn::sum<fn::pack<double, bool>, fn::pack<double, int>>, Error>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::sum< //
                                                      fn::pack<double, bool, bool>, fn::pack<double, bool, int>,
                                                      fn::pack<double, int, bool>, fn::pack<double, int, int>>,
                                                  fn::sum<Error, int>>>);

          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{fn::sum{12}})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                    })
                    .value()
                == fn::sum{true});
          CHECK((Lh{std::unexpect, FileNotFound} & Rh{fn::sum{12}}).error() == fn::sum{FileNotFound});
          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{std::unexpect, fn::sum{13}}).error() == fn::sum{13});
          CHECK((Lh{std::unexpect, FileNotFound} & Rh{std::unexpect, fn::sum{13}}).error() == fn::sum{FileNotFound});
        }
      }

      WHEN("sum on left side only")
      {
        using Lh = fn::expected<fn::sum<double, int>, Error>;
        using Rh = fn::expected<int, fn::sum<int>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::sum< //
                                                    fn::pack<double, int>, fn::pack<int, int>>,
                                                fn::sum<Error, int>>>);

        CHECK((Lh{fn::sum{0.5}} & Rh{12})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::sum{true});
        CHECK((Lh{std::unexpect, FileNotFound} & Rh{12}).error() == fn::sum{FileNotFound});
        CHECK((Lh{fn::sum{0.5}} & Rh{std::unexpect, 13}).error() == fn::sum{13});
        CHECK((Lh{std::unexpect, FileNotFound} & Rh{std::unexpect, 13}).error() == fn::sum{FileNotFound});

        WHEN("sum of packs on left")
        {
          using Lh = fn::expected<fn::sum<fn::pack<double, bool>, fn::pack<double, int>>, Error>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::sum< //
                                                      fn::pack<double, bool, int>, fn::pack<double, int, int>>,
                                                  fn::sum<Error, int>>>);

          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{12})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                    })
                    .value()
                == fn::sum{true});
          CHECK((Lh{std::unexpect, FileNotFound} & Rh{12}).error() == fn::sum{FileNotFound});
          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{std::unexpect, fn::sum{13}}).error() == fn::sum{13});
          CHECK((Lh{std::unexpect, FileNotFound} & Rh{std::unexpect, fn::sum{13}}).error() == fn::sum{FileNotFound});
        }
      }

      WHEN("sum on right side only")
      {
        using Lh = fn::expected<double, Error>;
        using Rh = fn::expected<fn::sum<bool, int>, fn::sum<int>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::sum< //
                                                    fn::pack<double, bool>, fn::pack<double, int>>,
                                                fn::sum<Error, int>>>);

        CHECK((Lh{0.5} & Rh{fn::sum{12}})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::sum{true});
        CHECK((Lh{std::unexpect, FileNotFound} & Rh{fn::sum{12}}).error() == fn::sum{FileNotFound});
        CHECK((Lh{0.5} & Rh{std::unexpect, fn::sum{13}}).error() == fn::sum{13});
        CHECK((Lh{std::unexpect, FileNotFound} & Rh{std::unexpect, fn::sum{13}}).error() == fn::sum{FileNotFound});

        WHEN("pack on left")
        {
          using Lh = fn::expected<fn::pack<double, int>, Error>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::sum< //
                                                      fn::pack<double, int, bool>, fn::pack<double, int, int>>,
                                                  fn::sum<Error, int>>>);

          CHECK((Lh{fn::pack{0.5, 3}} & Rh{fn::sum{12}})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                    })
                    .value()
                == fn::sum{true});
          CHECK((Lh{std::unexpect, FileNotFound} & Rh{fn::sum{12}}).error() == fn::sum{FileNotFound});
          CHECK((Lh{fn::pack{0.5, 3}} & Rh{std::unexpect, fn::sum{13}}).error() == fn::sum{13});
          CHECK((Lh{std::unexpect, FileNotFound} & Rh{std::unexpect, fn::sum{13}}).error() == fn::sum{FileNotFound});
        }
      }
    }

    WHEN("graded monad on both ides")
    {
      static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::sum<bool, int>>>()
                                          & std::declval<fn::expected<void, fn::sum<Error>>>()),
                                 fn::expected<int, fn::sum<Error, bool, int>>>);

      WHEN("value & void & yield value")
      {
        using Lh = fn::expected<int, fn::sum<bool, int>>;
        using Rh = fn::expected<void, fn::sum<Error>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<int, fn::sum<Error, bool, int>>>);

        CHECK((Lh{12} & Rh{}).value() == 12);
        CHECK((Lh{12} & Rh{std::unexpect, fn::sum{FileNotFound}}).error() == fn::sum{FileNotFound});
        CHECK((Lh{std::unexpect, fn::sum{13}} & Rh{}).error() == fn::sum{13});
        CHECK((Lh{std::unexpect, fn::sum{13}} & Rh{std::unexpect, fn::sum{FileNotFound}}).error() == fn::sum{13});
      }

      WHEN("void & value yield value")
      {
        using Lh = fn::expected<void, fn::sum<bool, int>>;
        using Rh = fn::expected<int, fn::sum<Error>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<int, fn::sum<Error, bool, int>>>);

        CHECK((Lh{} & Rh{42}).value() == 42);
        CHECK((Lh{} & Rh{std::unexpect, fn::sum{FileNotFound}}).error() == fn::sum{FileNotFound});
        CHECK((Lh{std::unexpect, fn::sum{13}} & Rh{42}).error() == fn::sum{13});
        CHECK((Lh{std::unexpect, fn::sum{13}} & Rh{std::unexpect, fn::sum{FileNotFound}}).error() == fn::sum{13});
      }

      WHEN("void & void yield void")
      {
        using Lh = fn::expected<void, fn::sum<bool, int>>;
        using Rh = fn::expected<void, fn::sum<Error>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<void, fn::sum<Error, bool, int>>>);

        CHECK((Lh{} & Rh{}).has_value());
        CHECK((Lh{} & Rh{std::unexpect, fn::sum{FileNotFound}}).error() == fn::sum{FileNotFound});
        CHECK((Lh{std::unexpect, fn::sum{13}} & Rh{}).error() == fn::sum{13});
        CHECK((Lh{std::unexpect, fn::sum{13}} & Rh{std::unexpect, fn::sum{FileNotFound}}).error() == fn::sum{13});
      }

      WHEN("value & value yield pack")
      {
        using Lh = fn::expected<double, fn::sum<bool, int>>;
        using Rh = fn::expected<int, fn::sum<Error>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::pack<double, int>, fn::sum<Error, bool, int>>>);

        CHECK((Lh{0.5} & Rh{12})
                  .transform([](double d, int i) constexpr -> bool { return d == 0.5 && i == 12; })
                  .value());
        CHECK((Lh{0.5} & Rh{std::unexpect, fn::sum{FileNotFound}}).error() == fn::sum{FileNotFound});
        CHECK((Lh{std::unexpect, fn::sum{13}} & Rh{12}).error() == fn::sum{13});
        CHECK((Lh{std::unexpect, fn::sum{13}} & Rh{std::unexpect, fn::sum{FileNotFound}}).error() == fn::sum{13});
      }

      WHEN("pack & value yield pack")
      {
        using Lh = fn::expected<fn::pack<double, bool>, fn::sum<bool, int>>;
        using Rh = fn::expected<int, fn::sum<Error>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::pack<double, bool, int>, fn::sum<Error, bool, int>>>);

        CHECK((Lh{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & Rh{12})
                  .transform([](double d, bool b, int i) constexpr -> bool { return d == 0.5 && b && i == 12; })
                  .value());
        CHECK((Lh{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & Rh{std::unexpect, fn::sum{FileNotFound}})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((Lh{std::unexpect, fn::sum{13}} //
               & Rh{12})
                  .error()
              == fn::sum{13});
        CHECK((Lh{std::unexpect, fn::sum{13}} //
               & Rh{std::unexpect, fn::sum{FileNotFound}})
                  .error()
              == fn::sum{13});
      }

      WHEN("pack & void yield pack")
      {
        using Lh = fn::expected<fn::pack<double, bool>, fn::sum<bool, int>>;
        using Rh = fn::expected<void, fn::sum<Error>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::pack<double, bool>, fn::sum<Error, bool, int>>>);

        CHECK((Lh{std::in_place, fn::pack<double, bool>{0.5, true}} & Rh{})
                  .transform([](double d, bool b) constexpr -> bool { return d == 0.5 && b; })
                  .value());
        CHECK((Lh{std::in_place, fn::pack<double, bool>{0.5, true}} & Rh{std::unexpect, fn::sum{FileNotFound}}).error()
              == fn::sum{FileNotFound});
        CHECK((Lh{std::unexpect, fn::sum{13}} & Rh{}).error() == fn::sum{13});
        CHECK((Lh{std::unexpect, fn::sum{13}} & Rh{std::unexpect, fn::sum{FileNotFound}}).error() == fn::sum{13});
      }

      WHEN("void & pack yield pack")
      {
        using Lh = fn::expected<void, fn::sum<bool, int>>;
        using Rh = fn::expected<fn::pack<double, bool>, fn::sum<Error>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::pack<double, bool>, fn::sum<Error, bool, int>>>);

        CHECK((Lh{} & Rh{std::in_place, fn::pack<double, bool>{0.5, true}})
                  .transform([](double d, bool b) constexpr -> bool { return d == 0.5 && b; })
                  .value());
        CHECK((Lh{} & Rh{std::unexpect, fn::sum{FileNotFound}}).error() == fn::sum{FileNotFound});
        CHECK((Lh{std::unexpect, fn::sum{13}} & Rh{std::in_place, fn::pack<double, bool>{0.5, true}}).error()
              == fn::sum{13});
        CHECK((Lh{std::unexpect, fn::sum{13}} & Rh{std::unexpect, fn::sum{FileNotFound}}).error() == fn::sum{13});
      }

      WHEN("sum on both sides")
      {
        using Lh = fn::expected<fn::sum<double, int>, fn::sum<Error>>;
        using Rh = fn::expected<fn::sum<bool, int>, fn::sum<bool, int>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::sum< //
                                                    fn::pack<double, bool>, fn::pack<double, int>, fn::pack<int, bool>,
                                                    fn::pack<int, int>>,
                                                fn::sum<Error, bool, int>>>);

        CHECK((Lh{fn::sum{0.5}} & Rh{fn::sum{12}})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::sum{true});
        CHECK((Lh{std::unexpect, fn::sum{FileNotFound}} & Rh{fn::sum{12}}).error() == fn::sum{FileNotFound});
        CHECK((Lh{fn::sum{0.5}} & Rh{std::unexpect, fn::sum{13}}).error() == fn::sum{13});
        CHECK((Lh{std::unexpect, fn::sum{FileNotFound}} & Rh{std::unexpect, fn::sum{13}}).error()
              == fn::sum{FileNotFound});

        WHEN("sum of packs on left")
        {
          using Lh = fn::expected<fn::sum<fn::pack<double, bool>, fn::pack<double, int>>, fn::sum<Error>>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::sum< //
                                                      fn::pack<double, bool, bool>, fn::pack<double, bool, int>,
                                                      fn::pack<double, int, bool>, fn::pack<double, int, int>>,
                                                  fn::sum<Error, bool, int>>>);

          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{fn::sum{12}})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      if constexpr (std::is_same_v<decltype(i), double> //
                                    && std::is_same_v<decltype(j), int> //
                                    && std::is_same_v<decltype(k), int>) {
                        return 0.5 == i && 3 == j && 12 == k;
                      } else {
                        throw 0;
                      }
                    })
                    .value()
                == fn::sum{true});
          CHECK((Lh{std::unexpect, fn::sum{FileNotFound}} & Rh{fn::sum{12}}).error() == fn::sum{FileNotFound});
          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{std::unexpect, fn::sum{13}}).error() == fn::sum{13});
          CHECK((Lh{std::unexpect, fn::sum{FileNotFound}} & Rh{std::unexpect, fn::sum{13}}).error()
                == fn::sum{FileNotFound});
        }
      }

      WHEN("sum on left side only")
      {
        using Lh = fn::expected<fn::sum<double, int>, fn::sum<Error>>;
        using Rh = fn::expected<int, fn::sum<bool, int>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::sum< //
                                                    fn::pack<double, int>, fn::pack<int, int>>,
                                                fn::sum<Error, bool, int>>>);

        CHECK((Lh{fn::sum{0.5}} & Rh{12})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::sum{true});
        CHECK((Lh{std::unexpect, fn::sum{FileNotFound}} & Rh{12}).error() == fn::sum{FileNotFound});
        CHECK((Lh{fn::sum{0.5}} & Rh{std::unexpect, 13}).error() == fn::sum{13});
        CHECK((Lh{std::unexpect, fn::sum{FileNotFound}} & Rh{std::unexpect, 13}).error() == fn::sum{FileNotFound});

        WHEN("sum of packs on left")
        {
          using Lh = fn::expected<fn::sum<fn::pack<double, bool>, fn::pack<double, int>>, fn::sum<Error>>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::sum< //
                                                      fn::pack<double, bool, int>, fn::pack<double, int, int>>,
                                                  fn::sum<Error, bool, int>>>);

          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{12})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      if constexpr (std::is_same_v<decltype(i), double> //
                                    && std::is_same_v<decltype(j), int> //
                                    && std::is_same_v<decltype(k), int>) {
                        return 0.5 == i && 3 == j && 12 == k;
                      } else {
                        throw 0;
                      }
                    })
                    .value()
                == fn::sum{true});
          CHECK((Lh{std::unexpect, fn::sum{FileNotFound}} & Rh{12}).error() == fn::sum{FileNotFound});
          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{std::unexpect, fn::sum{13}}).error() == fn::sum{13});
          CHECK((Lh{std::unexpect, fn::sum{FileNotFound}} & Rh{std::unexpect, fn::sum{13}}).error()
                == fn::sum{FileNotFound});
        }
      }

      WHEN("sum on right side only")
      {
        using Lh = fn::expected<double, fn::sum<Error>>;
        using Rh = fn::expected<fn::sum<bool, int>, fn::sum<bool, int>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::sum< //
                                                    fn::pack<double, bool>, fn::pack<double, int>>,
                                                fn::sum<Error, bool, int>>>);

        CHECK((Lh{0.5} & Rh{fn::sum{12}})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::sum{true});
        CHECK((Lh{std::unexpect, FileNotFound} & Rh{fn::sum{12}}).error() == fn::sum{FileNotFound});
        CHECK((Lh{0.5} & Rh{std::unexpect, fn::sum{13}}).error() == fn::sum{13});
        CHECK((Lh{std::unexpect, FileNotFound} & Rh{std::unexpect, fn::sum{13}}).error() == fn::sum{FileNotFound});

        WHEN("pack on left")
        {
          using Lh = fn::expected<fn::pack<double, int>, fn::sum<Error>>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::sum< //
                                                      fn::pack<double, int, bool>, fn::pack<double, int, int>>,
                                                  fn::sum<Error, bool, int>>>);

          CHECK((Lh{fn::pack{0.5, 3}} & Rh{fn::sum{12}})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      if constexpr (std::is_same_v<decltype(i), double> //
                                    && std::is_same_v<decltype(j), int> //
                                    && std::is_same_v<decltype(k), int>) {
                        return 0.5 == i && 3 == j && 12 == k;
                      } else {
                        throw 0;
                      }
                    })
                    .value()
                == fn::sum{true});
          CHECK((Lh{std::unexpect, fn::sum{FileNotFound}} & Rh{fn::sum{12}}).error() == fn::sum{FileNotFound});
          CHECK((Lh{fn::pack{0.5, 3}} & Rh{std::unexpect, fn::sum{13}}).error() == fn::sum{13});
          CHECK((Lh{std::unexpect, fn::sum{FileNotFound}} & Rh{std::unexpect, fn::sum{13}}).error()
                == fn::sum{FileNotFound});
        }
      }
    }
  }
}

TEST_CASE("expected sum support and_then", "[expected][sum][and_then]")
{
  WHEN("value")
  {
    fn::expected<fn::sum<int, std::string_view>, Error> s{fn::sum{12}};

    CHECK(s.and_then( //
               fn::overload([](int &i) -> fn::expected<bool, Error> { return i == 12; },
                            [](int const &) -> fn::expected<bool, Error> { throw 0; },
                            [](int &&) -> fn::expected<bool, Error> { throw 0; },
                            [](int const &&) -> fn::expected<bool, Error> { throw 0; },
                            [](std::string_view &) -> fn::expected<bool, Error> { throw 0; },
                            [](std::string_view const &) -> fn::expected<bool, Error> { throw 0; },
                            [](std::string_view &&) -> fn::expected<bool, Error> { throw 0; },
                            [](std::string_view const &&) -> fn::expected<bool, Error> { throw 0; }))
              .value());

    CHECK(std::as_const(s)
              .and_then( //
                  fn::overload([](int &) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &i) -> fn::expected<bool, Error> { return i == 12; },
                               [](int &&) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &&) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view &) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view const &) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view &&) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view const &&) -> fn::expected<bool, Error> { throw 0; }))
              .value());

    CHECK(std::move(std::as_const(s))
              .and_then( //
                  fn::overload([](int &) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &) -> fn::expected<bool, Error> { throw 0; },
                               [](int &&) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &&i) -> fn::expected<bool, Error> { return i == 12; },
                               [](std::string_view &) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view const &) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view &&) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view const &&) -> fn::expected<bool, Error> { throw 0; }))
              .value());

    CHECK(std::move(s)
              .and_then( //
                  fn::overload([](int &) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &) -> fn::expected<bool, Error> { throw 0; },
                               [](int &&i) -> fn::expected<bool, Error> { return i == 12; },
                               [](int const &&) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view &) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view const &) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view &&) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view const &&) -> fn::expected<bool, Error> { throw 0; }))
              .value());
  }

  WHEN("error")
  {
    fn::expected<fn::sum<int, std::string_view>, Error> s{std::unexpect, FileNotFound};
    CHECK(s.and_then( //
               [](auto...) -> fn::expected<bool, Error> { return {true}; })
              .error()
          == FileNotFound);
    CHECK(std::as_const(s)
              .and_then( //
                  [](auto...) -> fn::expected<bool, Error> { return {true}; })
              .error()
          == FileNotFound);
    CHECK(fn::expected<fn::sum<int, std::string_view>, Error>{std::unexpect, FileNotFound}
              .and_then( //
                  [](auto...) -> fn::expected<bool, Error> { return {true}; })
              .error()
          == FileNotFound);
    CHECK(std::move(std::as_const(s))
              .and_then( //
                  [](auto...) -> fn::expected<bool, Error> { return {true}; })
              .error()
          == FileNotFound);
  }

  WHEN("constexpr")
  {
    constexpr auto fn = fn::overload([](int &) -> fn::expected<bool, Error> { throw 0; },
                                     [](int const &i) -> fn::expected<bool, Error> { return i == 42; },
                                     [](int &&) -> fn::expected<bool, Error> { throw 0; },
                                     [](int const &&) -> fn::expected<bool, Error> { throw 0; },
                                     [](std::string_view &) -> fn::expected<bool, Error> { throw 0; },
                                     [](std::string_view const &) -> fn::expected<bool, Error> { throw 0; },
                                     [](std::string_view &&) -> fn::expected<bool, Error> { throw 0; },
                                     [](std::string_view const &&) -> fn::expected<bool, Error> { throw 0; });
    constexpr fn::expected<fn::sum<int, std::string_view>, Error> a{fn::sum{42}};
    static_assert(std::is_same_v<decltype(a.and_then(fn)), fn::expected<bool, Error>>);
    static_assert(a.and_then(fn).value());
  }
}

TEST_CASE("expected sum support or_else", "[expected][sum][or_else]")
{
  WHEN("value")
  {
    fn::expected<double, fn::sum<int, std::string_view>> s{std::unexpect, fn::sum{12}};

    CHECK(s.or_else( //
               fn::overload([](int &i) -> fn::expected<double, Error> { return {i}; },
                            [](int const &) -> fn::expected<double, Error> { throw 0; },
                            [](int &&) -> fn::expected<double, Error> { throw 0; },
                            [](int const &&) -> fn::expected<double, Error> { throw 0; },
                            [](std::string_view &) -> fn::expected<double, Error> { throw 0; },
                            [](std::string_view const &) -> fn::expected<double, Error> { throw 0; },
                            [](std::string_view &&) -> fn::expected<double, Error> { throw 0; },
                            [](std::string_view const &&) -> fn::expected<double, Error> { throw 0; }))
              .value()
          == 12);

    CHECK(std::as_const(s)
              .or_else( //
                  fn::overload([](int &) -> fn::expected<double, Error> { throw 0; },
                               [](int const &i) -> fn::expected<double, Error> { return {i}; },
                               [](int &&) -> fn::expected<double, Error> { throw 0; },
                               [](int const &&) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view &) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view const &) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view &&) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view const &&) -> fn::expected<double, Error> { throw 0; }))
              .value()
          == 12);

    CHECK(std::move(std::as_const(s))
              .or_else( //
                  fn::overload([](int &) -> fn::expected<double, Error> { throw 0; },
                               [](int const &) -> fn::expected<double, Error> { throw 0; },
                               [](int &&) -> fn::expected<double, Error> { throw 0; },
                               [](int const &&i) -> fn::expected<double, Error> { return {i}; },
                               [](std::string_view &) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view const &) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view &&) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view const &&) -> fn::expected<double, Error> { throw 0; }))
              .value()
          == 12);

    CHECK(std::move(s)
              .or_else( //
                  fn::overload([](int &) -> fn::expected<double, Error> { throw 0; },
                               [](int const &) -> fn::expected<double, Error> { throw 0; },
                               [](int &&i) -> fn::expected<double, Error> { return {i}; },
                               [](int const &&) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view &) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view const &) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view &&) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view const &&) -> fn::expected<double, Error> { throw 0; }))
              .value()
          == 12);

    WHEN("value")
    {
      fn::expected<double, fn::sum<int, std::string_view>> s{1.5};
      CHECK(s.or_else( //
                 [](auto...) -> fn::expected<double, Error> { throw 0; })
                .value()
            == 1.5);
      CHECK(std::as_const(s)
                .or_else( //
                    [](auto...) -> fn::expected<double, Error> { throw 0; })
                .value()
            == 1.5);
      CHECK(std::move(std::as_const(s))
                .or_else( //
                    [](auto...) -> fn::expected<double, Error> { throw 0; })
                .value()
            == 1.5);
      CHECK(std::move(s)
                .or_else( //
                    [](auto...) -> fn::expected<double, Error> { throw 0; })
                .value()
            == 1.5);
    }

    WHEN("constexpr")
    {
      constexpr auto fn = fn::overload([](int &) -> fn::expected<double, Error> { throw 0; },
                                       [](int const &i) -> fn::expected<double, Error> { return {i}; },
                                       [](int &&) -> fn::expected<double, Error> { throw 0; },
                                       [](int const &&) -> fn::expected<double, Error> { throw 0; },
                                       [](std::string_view &) -> fn::expected<double, Error> { throw 0; },
                                       [](std::string_view const &) -> fn::expected<double, Error> { throw 0; },
                                       [](std::string_view &&) -> fn::expected<double, Error> { throw 0; },
                                       [](std::string_view const &&) -> fn::expected<double, Error> { throw 0; });
      constexpr fn::expected<double, fn::sum<int, std::string_view>> a{std::unexpect, fn::sum{42}};
      static_assert(std::is_same_v<decltype(a.or_else(fn)), fn::expected<double, Error>>);
      static_assert(a.or_else(fn).value() == 42);
    }
  }

  WHEN("void")
  {
    fn::expected<void, fn::sum<int, std::string_view>> s{std::unexpect, fn::sum{12}};

    CHECK(s.or_else( //
               fn::overload([](int &) -> fn::expected<void, Error> { return std::unexpected<Error>{FileNotFound}; },
                            [](int const &) -> fn::expected<void, Error> { throw 0; },
                            [](int &&) -> fn::expected<void, Error> { throw 0; },
                            [](int const &&) -> fn::expected<void, Error> { throw 0; },
                            [](std::string_view &) -> fn::expected<void, Error> { throw 0; },
                            [](std::string_view const &) -> fn::expected<void, Error> { throw 0; },
                            [](std::string_view &&) -> fn::expected<void, Error> { throw 0; },
                            [](std::string_view const &&) -> fn::expected<void, Error> { throw 0; }))
              .error()
          == FileNotFound);

    CHECK(std::as_const(s)
              .or_else( //
                  fn::overload(
                      [](int &) -> fn::expected<void, Error> { throw 0; },
                      [](int const &) -> fn::expected<void, Error> { return std::unexpected<Error>{FileNotFound}; },
                      [](int &&) -> fn::expected<void, Error> { throw 0; },
                      [](int const &&) -> fn::expected<void, Error> { throw 0; },
                      [](std::string_view &) -> fn::expected<void, Error> { throw 0; },
                      [](std::string_view const &) -> fn::expected<void, Error> { throw 0; },
                      [](std::string_view &&) -> fn::expected<void, Error> { throw 0; },
                      [](std::string_view const &&) -> fn::expected<void, Error> { throw 0; }))
              .error()
          == FileNotFound);

    CHECK(std::move(std::as_const(s))
              .or_else( //
                  fn::overload(
                      [](int &) -> fn::expected<void, Error> { throw 0; },
                      [](int const &) -> fn::expected<void, Error> { throw 0; },
                      [](int &&) -> fn::expected<void, Error> { throw 0; },
                      [](int const &&) -> fn::expected<void, Error> { return std::unexpected<Error>{FileNotFound}; },
                      [](std::string_view &) -> fn::expected<void, Error> { throw 0; },
                      [](std::string_view const &) -> fn::expected<void, Error> { throw 0; },
                      [](std::string_view &&) -> fn::expected<void, Error> { throw 0; },
                      [](std::string_view const &&) -> fn::expected<void, Error> { throw 0; }))
              .error()
          == FileNotFound);

    CHECK(std::move(s)
              .or_else( //
                  fn::overload([](int &) -> fn::expected<void, Error> { throw 0; },
                               [](int const &) -> fn::expected<void, Error> { throw 0; },
                               [](int &&) -> fn::expected<void, Error> { return std::unexpected<Error>{FileNotFound}; },
                               [](int const &&) -> fn::expected<void, Error> { throw 0; },
                               [](std::string_view &) -> fn::expected<void, Error> { throw 0; },
                               [](std::string_view const &) -> fn::expected<void, Error> { throw 0; },
                               [](std::string_view &&) -> fn::expected<void, Error> { throw 0; },
                               [](std::string_view const &&) -> fn::expected<void, Error> { throw 0; }))
              .error()
          == FileNotFound);

    WHEN("value")
    {
      fn::expected<void, fn::sum<int, std::string_view>> s{};
      CHECK(s.or_else( //
                 [](auto...) -> fn::expected<void, Error> { throw 0; })
                .has_value());
      CHECK(std::as_const(s)
                .or_else( //
                    [](auto...) -> fn::expected<void, Error> { throw 0; })
                .has_value());
      CHECK(std::move(std::as_const(s))
                .or_else( //
                    [](auto...) -> fn::expected<void, Error> { throw 0; })
                .has_value());
      CHECK(std::move(s)
                .or_else( //
                    [](auto...) -> fn::expected<void, Error> { throw 0; })
                .has_value());
    }

    WHEN("constexpr")
    {
      constexpr auto fn
          = fn::overload([](int &) -> fn::expected<void, Error> { throw 0; },
                         [](int const &) -> fn::expected<void, Error> { return std::unexpected<Error>{FileNotFound}; },
                         [](int &&) -> fn::expected<void, Error> { throw 0; },
                         [](int const &&) -> fn::expected<void, Error> { throw 0; },
                         [](std::string_view &) -> fn::expected<void, Error> { throw 0; },
                         [](std::string_view const &) -> fn::expected<void, Error> { throw 0; },
                         [](std::string_view &&) -> fn::expected<void, Error> { throw 0; },
                         [](std::string_view const &&) -> fn::expected<void, Error> { throw 0; });
      constexpr fn::expected<void, fn::sum<int, std::string_view>> a{std::unexpect, fn::sum{42}};
      static_assert(std::is_same_v<decltype(a.or_else(fn)), fn::expected<void, Error>>);
      static_assert(a.or_else(fn).error() == FileNotFound);
    }
  }
}

TEST_CASE("expected sum support transform", "[expected][sum][transform]")
{
  WHEN("value")
  {
    fn::expected<fn::sum<int, std::string_view>, Error> s{fn::sum{12}};

    CHECK(s.transform( //
               fn::overload(
                   [](int &) -> std::monostate { return {}; }, [](int const &) -> std::monostate { throw 0; },
                   [](int &&) -> std::monostate { throw 0; }, [](int const &&) -> std::monostate { throw 0; },
                   [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                   [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }))
              .value()
              .has_value<std::monostate>());

    CHECK(std::as_const(s)
              .transform( //
                  fn::overload(
                      [](int &) -> std::monostate { throw 0; }, [](int const &) -> std::monostate { return {}; },
                      [](int &&) -> std::monostate { throw 0; }, [](int const &&) -> std::monostate { throw 0; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }))
              .value()
              .has_value<std::monostate>());

    CHECK(std::move(std::as_const(s))
              .transform( //
                  fn::overload(
                      [](int &) -> std::monostate { throw 0; }, [](int const &) -> std::monostate { throw 0; },
                      [](int &&) -> std::monostate { throw 0; }, [](int const &&) -> std::monostate { return {}; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }))
              .value()
              .has_value<std::monostate>());

    CHECK(std::move(s)
              .transform( //
                  fn::overload(
                      [](int &) -> std::monostate { throw 0; }, [](int const &) -> std::monostate { throw 0; },
                      [](int &&) -> std::monostate { return {}; }, [](int const &&) -> std::monostate { throw 0; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }))
              .value()
              .has_value<std::monostate>());
  }

  WHEN("error")
  {
    fn::expected<fn::sum<int, std::string_view>, Error> s{std::unexpect, FileNotFound};
    CHECK(s.transform( //
               [](auto...) -> std::monostate { throw 0; })
              .error()
          == FileNotFound);
    CHECK(std::as_const(s)
              .transform( //
                  [](auto...) -> std::monostate { throw 0; })
              .error()
          == FileNotFound);
    CHECK(std::move(std::as_const(s))
              .transform( //
                  [](auto...) -> std::monostate { throw 0; })
              .error()
          == FileNotFound);
    CHECK(std::move(s)
              .transform( //
                  [](auto...) -> std::monostate { throw 0; })
              .error()
          == FileNotFound);
  }

  WHEN("constexpr")
  {
    constexpr auto fn
        = fn::overload([](int &) -> bool { throw 0; }, [](int const &) -> bool { return true; },
                       [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; },
                       [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                       [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; });
    constexpr fn::expected<fn::sum<int, std::string_view>, Error> a{fn::sum{42}};
    static_assert(std::is_same_v<decltype(a.transform(fn)), fn::expected<fn::sum<bool, int>, Error>>);
    // TODO Switch bool to std::monostate or similar user-defined type
    static_assert(a.transform(fn).value().has_value<bool>());
  }
}

TEST_CASE("expected sum support transform_error", "[expected][sum][transform_error]")
{
  WHEN("value")
  {
    fn::expected<double, fn::sum<int, std::string_view>> s{std::unexpect, fn::sum{12}};

    CHECK(s.transform_error( //
               fn::overload(
                   [](int &i) -> bool { return i == 12; }, [](int const &) -> bool { throw 0; },
                   [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; },
                   [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                   [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }))
              .error()
          == fn::sum{true});

    CHECK(std::as_const(s)
              .transform_error( //
                  fn::overload(
                      [](int &) -> bool { throw 0; }, [](int const &i) -> bool { return i == 12; },
                      [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }))
              .error()
          == fn::sum{true});

    CHECK(std::move(std::as_const(s))
              .transform_error( //
                  fn::overload(
                      [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                      [](int &&) -> bool { throw 0; }, [](int const &&i) -> bool { return i == 12; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }))
              .error()
          == fn::sum{true});

    CHECK(std::move(s)
              .transform_error( //
                  fn::overload(
                      [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                      [](int &&i) -> bool { return i == 12; }, [](int const &&) -> bool { throw 0; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }))
              .error()
          == fn::sum{true});

    WHEN("value")
    {
      fn::expected<double, fn::sum<int, std::string_view>> s{1.5};
      CHECK(s.transform_error( //
                 [](auto...) -> bool { throw 0; })
                .value()
            == 1.5);
      CHECK(std::as_const(s)
                .transform_error( //
                    [](auto...) -> bool { throw 0; })
                .value()
            == 1.5);
      CHECK(std::move(std::as_const(s))
                .transform_error( //
                    [](auto...) -> bool { throw 0; })
                .value()
            == 1.5);
      CHECK(std::move(s)
                .transform_error( //
                    [](auto...) -> bool { throw 0; })
                .value()
            == 1.5);
    }

    WHEN("constexpr")
    {
      constexpr auto fn = fn::overload(
          [](int &) -> bool { throw 0; }, [](int const &i) -> bool { return i == 42; }, [](int &&) -> bool { throw 0; },
          [](int const &&) -> bool { throw 0; }, [](std::string_view &) -> int { throw 0; },
          [](std::string_view const &) -> int { throw 0; }, [](std::string_view &&) -> int { throw 0; },
          [](std::string_view const &&) -> int { throw 0; });
      constexpr fn::expected<double, fn::sum<int, std::string_view>> a{std::unexpect, fn::sum{42}};
      static_assert(std::is_same_v<decltype(a.transform_error(fn)), fn::expected<double, fn::sum<bool, int>>>);
      static_assert(a.transform_error(fn).error() == fn::sum{true});
    }
  }

  WHEN("void")
  {
    fn::expected<void, fn::sum<int, std::string_view>> s{std::unexpect, fn::sum{12}};

    CHECK(s.transform_error( //
               fn::overload(
                   [](int &i) -> int { return i; }, [](int const &) -> int { throw 0; }, [](int &&) -> int { throw 0; },
                   [](int const &&) -> int { throw 0; }, [](std::string_view &) -> int { throw 0; },
                   [](std::string_view const &) -> int { throw 0; }, [](std::string_view &&) -> int { throw 0; },
                   [](std::string_view const &&) -> int { throw 0; }))
              .error()
          == fn::sum{12});

    CHECK(std::as_const(s)
              .transform_error( //
                  fn::overload(
                      [](int &) -> int { throw 0; }, [](int const &i) -> int { return i; },
                      [](int &&) -> int { throw 0; }, [](int const &&) -> int { throw 0; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }))
              .error()
          == fn::sum{12});

    CHECK(std::move(std::as_const(s))
              .transform_error( //
                  fn::overload(
                      [](int &) -> int { throw 0; }, [](int const &) -> int { throw 0; },
                      [](int &&) -> int { throw 0; }, [](int const &&i) -> int { return i; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }))
              .error()
          == fn::sum{12});

    CHECK(std::move(s)
              .transform_error( //
                  fn::overload(
                      [](int &) -> int { throw 0; }, [](int const &) -> int { throw 0; },
                      [](int &&i) -> int { return i; }, [](int const &&) -> int { throw 0; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }))
              .error()
          == fn::sum{12});

    WHEN("value")
    {
      fn::expected<void, fn::sum<int, std::string_view>> s{};
      CHECK(s.transform_error( //
                 [](auto...) -> int { throw 0; })
                .has_value());
      CHECK(std::as_const(s)
                .transform_error( //
                    [](auto...) -> int { throw 0; })
                .has_value());
      CHECK(std::move(std::as_const(s))
                .transform_error( //
                    [](auto...) -> int { throw 0; })
                .has_value());
      CHECK(std::move(s)
                .transform_error( //
                    [](auto...) -> int { throw 0; })
                .has_value());
    }

    WHEN("constexpr")
    {
      constexpr auto fn = fn::overload(
          [](int &) -> int { throw 0; }, [](int const &i) -> int { return i; }, [](int &&) -> int { throw 0; },
          [](int const &&) -> int { throw 0; }, [](std::string_view &) -> int { throw 0; },
          [](std::string_view const &) -> int { throw 0; }, [](std::string_view &&) -> int { throw 0; },
          [](std::string_view const &&) -> int { throw 0; });
      constexpr fn::expected<void, fn::sum<int, std::string_view>> a{std::unexpect, fn::sum{42}};
      static_assert(std::is_same_v<decltype(a.transform_error(fn)), fn::expected<void, fn::sum<int>>>);
      static_assert(a.transform_error(fn).error() == fn::sum{42});
    }
  }
}

TEST_CASE("expected polyfills and_then", "[expected][polyfill][and_then]")
{
  WHEN("value")
  {
    fn::expected<int, Error> s{12};
    CHECK(s.and_then( //
               fn::overload([](int &i) -> fn::expected<bool, Error> { return i == 12; },
                            [](int const &) -> fn::expected<bool, Error> { throw 0; },
                            [](int &&) -> fn::expected<bool, Error> { throw 0; },
                            [](int const &&) -> fn::expected<bool, Error> { throw 0; })) //
              .value());
    CHECK(std::as_const(s)
              .and_then( //
                  fn::overload([](int &) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &i) -> fn::expected<bool, Error> { return i == 12; },
                               [](int &&) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &&) -> fn::expected<bool, Error> { throw 0; })) //
              .value());
    CHECK(std::move(std::as_const(s))
              .and_then( //
                  fn::overload([](int &) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &) -> fn::expected<bool, Error> { throw 0; },
                               [](int &&) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &&i) -> fn::expected<bool, Error> { return i == 12; })) //
              .value());
    CHECK(std::move(s)
              .and_then( //
                  fn::overload([](int &) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &) -> fn::expected<bool, Error> { throw 0; },
                               [](int &&i) -> fn::expected<bool, Error> { return i == 12; },
                               [](int const &&) -> fn::expected<bool, Error> { throw 0; })) //
              .value());

    WHEN("error")
    {
      fn::expected<int, Error> s{std::unexpect, Unknown};
      CHECK(s.and_then( //
                 [](auto) -> fn::expected<bool, Error> { throw 0; })
                .error()
            == Unknown);
      CHECK(std::as_const(s)
                .and_then( //
                    [](auto) -> fn::expected<bool, Error> { throw 0; })
                .error()
            == Unknown);
      CHECK(std::move(std::as_const(s))
                .and_then( //
                    [](auto) -> fn::expected<bool, Error> { throw 0; })
                .error()
            == Unknown);
      CHECK(std::move(s)
                .and_then( //
                    [](auto) -> fn::expected<bool, Error> { throw 0; })
                .error()
            == Unknown);
    }
  }

  WHEN("void")
  {
    fn::expected<void, Error> s{};
    CHECK(s.and_then([]() -> fn::expected<bool, Error> { return true; }).value());
    CHECK(std::as_const(s).and_then([]() -> fn::expected<bool, Error> { return true; }).value());
    CHECK(fn::expected<void, Error>{}.and_then([]() -> fn::expected<bool, Error> { return true; }).value());
    CHECK(std::move(std::as_const(s)).and_then([]() -> fn::expected<bool, Error> { return true; }).value());

    WHEN("error")
    {
      fn::expected<void, Error> s{std::unexpect, Unknown};
      CHECK(s.and_then([]() -> fn::expected<bool, Error> { throw 0; }).error() == Unknown);
      CHECK(std::as_const(s).and_then([]() -> fn::expected<bool, Error> { throw 0; }).error() == Unknown);
      CHECK(std::move(std::as_const(s)).and_then([]() -> fn::expected<bool, Error> { throw 0; }).error() == Unknown);
      CHECK(std::move(s).and_then([]() -> fn::expected<bool, Error> { throw 0; }).error() == Unknown);
    }
  }
}

TEST_CASE("expected polyfills or_else", "[expected][polyfill][or_else][pack]")
{
  WHEN("value")
  {
    fn::expected<int, Error> s{1};
    CHECK(s.or_else([](auto) -> fn::expected<int, Error> { throw 0; }).value());
    CHECK(std::as_const(s).or_else([](auto) -> fn::expected<int, Error> { throw 0; }).value());
    CHECK(fn::expected<int, Error>{1}.or_else([](auto) -> fn::expected<int, Error> { throw 0; }).value());
    CHECK(std::move(std::as_const(s)).or_else([](auto) -> fn::expected<int, Error> { throw 0; }).value());

    WHEN("error")
    {
      fn::expected<int, Error> s{std::unexpect, FileNotFound};
      CHECK(s.or_else( //
                 fn::overload([](Error &e) -> fn::expected<int, Error> { return e == FileNotFound; },
                              [](Error const &) -> fn::expected<int, Error> { throw 0; },
                              [](Error &&) -> fn::expected<int, Error> { throw 0; },
                              [](Error const &&) -> fn::expected<int, Error> { throw 0; })) //
                .value());
      CHECK(std::as_const(s)
                .or_else( //
                    fn::overload([](Error &) -> fn::expected<int, Error> { throw 0; },
                                 [](Error const &e) -> fn::expected<int, Error> { return e == FileNotFound; },
                                 [](Error &&) -> fn::expected<int, Error> { throw 0; },
                                 [](Error const &&) -> fn::expected<int, Error> { throw 0; })) //
                .value());
      CHECK(std::move(std::as_const(s))
                .or_else( //
                    fn::overload([](Error &) -> fn::expected<int, Error> { throw 0; },
                                 [](Error const &) -> fn::expected<int, Error> { throw 0; },
                                 [](Error &&) -> fn::expected<int, Error> { throw 0; },
                                 [](Error const &&e) -> fn::expected<int, Error> { return e == FileNotFound; })) //
                .value());
      CHECK(std::move(s)
                .or_else( //
                    fn::overload([](Error &) -> fn::expected<int, Error> { throw 0; },
                                 [](Error const &) -> fn::expected<int, Error> { throw 0; },
                                 [](Error &&e) -> fn::expected<int, Error> { return e == FileNotFound; },
                                 [](Error const &&) -> fn::expected<int, Error> { throw 0; })) //
                .value());
    }

    WHEN("pack error")
    {
      fn::expected<int, fn::pack<int, Error>> s{std::unexpect, fn::pack{12, FileNotFound}};
      CHECK(s.or_else( //
                 fn::overload([](int, Error &e) -> fn::expected<int, Error> { return e == FileNotFound; },
                              [](int, Error const &) -> fn::expected<int, Error> { throw 0; },
                              [](int, Error &&) -> fn::expected<int, Error> { throw 0; },
                              [](int, Error const &&) -> fn::expected<int, Error> { throw 0; })) //
                .value());
      CHECK(std::as_const(s)
                .or_else( //
                    fn::overload([](int, Error &) -> fn::expected<int, Error> { throw 0; },
                                 [](int, Error const &e) -> fn::expected<int, Error> { return e == FileNotFound; },
                                 [](int, Error &&) -> fn::expected<int, Error> { throw 0; },
                                 [](int, Error const &&) -> fn::expected<int, Error> { throw 0; })) //
                .value());
      CHECK(std::move(std::as_const(s))
                .or_else( //
                    fn::overload([](int, Error &) -> fn::expected<int, Error> { throw 0; },
                                 [](int, Error const &) -> fn::expected<int, Error> { throw 0; },
                                 [](int, Error &&) -> fn::expected<int, Error> { throw 0; },
                                 [](int, Error const &&e) -> fn::expected<int, Error> { return e == FileNotFound; })) //
                .value());
      CHECK(std::move(s)
                .or_else( //
                    fn::overload([](int, Error &) -> fn::expected<int, Error> { throw 0; },
                                 [](int, Error const &) -> fn::expected<int, Error> { throw 0; },
                                 [](int, Error &&e) -> fn::expected<int, Error> { return e == FileNotFound; },
                                 [](int, Error const &&) -> fn::expected<int, Error> { throw 0; })) //
                .value());
    }
  }

  WHEN("void")
  {
    fn::expected<void, Error> s{};
    CHECK(s.or_else([](auto) -> fn::expected<void, Error> { throw 0; }).has_value());
    CHECK(std::as_const(s).or_else([](auto) -> fn::expected<void, Error> { throw 0; }).has_value());
    CHECK(fn::expected<void, Error>{}.or_else([](auto) -> fn::expected<void, Error> { throw 0; }).has_value());
    CHECK(std::move(std::as_const(s)).or_else([](auto) -> fn::expected<void, Error> { throw 0; }).has_value());

    WHEN("error")
    {
      fn::expected<void, Error> s{std::unexpect, FileNotFound};
      CHECK(s.or_else( //
                 fn::overload([](Error &) -> fn::expected<void, Error> { return {}; },
                              [](Error const &) -> fn::expected<void, Error> { throw 0; },
                              [](Error &&) -> fn::expected<void, Error> { throw 0; },
                              [](Error const &&) -> fn::expected<void, Error> { throw 0; }))
                .has_value());
      CHECK(std::as_const(s)
                .or_else( //
                    fn::overload([](Error &) -> fn::expected<void, Error> { throw 0; },
                                 [](Error const &) -> fn::expected<void, Error> { return {}; },
                                 [](Error &&) -> fn::expected<void, Error> { throw 0; },
                                 [](Error const &&) -> fn::expected<void, Error> { throw 0; }))
                .has_value());
      CHECK(std::move(std::as_const(s))
                .or_else( //
                    fn::overload([](Error &) -> fn::expected<void, Error> { throw 0; },
                                 [](Error const &) -> fn::expected<void, Error> { throw 0; },
                                 [](Error &&) -> fn::expected<void, Error> { throw 0; },
                                 [](Error const &&) -> fn::expected<void, Error> { return {}; }))
                .has_value());
      CHECK(std::move(s)
                .or_else( //
                    fn::overload([](Error &) -> fn::expected<void, Error> { throw 0; },
                                 [](Error const &) -> fn::expected<void, Error> { throw 0; },
                                 [](Error &&) -> fn::expected<void, Error> { return {}; },
                                 [](Error const &&) -> fn::expected<void, Error> { throw 0; }))
                .has_value());
    }

    WHEN("pack error")
    {
      fn::expected<void, fn::pack<int, Error>> s{std::unexpect, fn::pack{12, FileNotFound}};
      CHECK(s.or_else( //
                 fn::overload([](int, Error &) -> fn::expected<void, Error> { return {}; },
                              [](int, Error const &) -> fn::expected<void, Error> { throw 0; },
                              [](int, Error &&) -> fn::expected<void, Error> { throw 0; },
                              [](int, Error const &&) -> fn::expected<void, Error> { throw 0; }))
                .has_value());
      CHECK(std::as_const(s)
                .or_else( //
                    fn::overload([](int, Error &) -> fn::expected<void, Error> { throw 0; },
                                 [](int, Error const &) -> fn::expected<void, Error> { return {}; },
                                 [](int, Error &&) -> fn::expected<void, Error> { throw 0; },
                                 [](int, Error const &&) -> fn::expected<void, Error> { throw 0; }))
                .has_value());
      CHECK(std::move(std::as_const(s))
                .or_else( //
                    fn::overload([](int, Error &) -> fn::expected<void, Error> { throw 0; },
                                 [](int, Error const &) -> fn::expected<void, Error> { throw 0; },
                                 [](int, Error &&) -> fn::expected<void, Error> { throw 0; },
                                 [](int, Error const &&) -> fn::expected<void, Error> { return {}; }))
                .has_value());
      CHECK(std::move(s)
                .or_else( //
                    fn::overload([](int, Error &) -> fn::expected<void, Error> { throw 0; },
                                 [](int, Error const &) -> fn::expected<void, Error> { throw 0; },
                                 [](int, Error &&) -> fn::expected<void, Error> { return {}; },
                                 [](int, Error const &&) -> fn::expected<void, Error> { throw 0; }))
                .has_value());
    }
  }
}

TEST_CASE("expected polyfills transform", "[expected][polyfill][transform]")
{
  WHEN("value")
  {
    fn::expected<int, Error> s{12};
    CHECK(s.transform( //
               fn::overload([](int &i) -> bool { return i == 12; }, [](int const &) -> bool { throw 0; },
                            [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; })) //
              .value());
    CHECK(std::as_const(s)
              .transform( //
                  fn::overload([](int &) -> bool { throw 0; }, [](int const &i) -> bool { return i == 12; },
                               [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; })) //
              .value());
    CHECK(std::move(std::as_const(s))
              .transform( //
                  fn::overload([](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                               [](int &&) -> bool { throw 0; }, [](int const &&i) -> bool { return i == 12; })) //
              .value());
    CHECK(std::move(s)
              .transform( //
                  fn::overload([](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                               [](int &&i) -> bool { return i == 12; }, [](int const &&) -> bool { throw 0; })) //
              .value());

    WHEN("void result")
    {
      fn::expected<int, Error> s{12};
      CHECK(s.transform( //
                 fn::overload([](int &) -> void {}, [](int const &) -> void { throw 0; },
                              [](int &&) -> void { throw 0; }, [](int const &&) -> void { throw 0; })) //
                .has_value());
      CHECK(std::as_const(s)
                .transform( //
                    fn::overload([](int &) -> void { throw 0; }, [](int const &) -> void {},
                                 [](int &&) -> void { throw 0; }, [](int const &&) -> void { throw 0; })) //
                .has_value());
      CHECK(std::move(std::as_const(s))
                .transform( //
                    fn::overload([](int &) -> void { throw 0; }, [](int const &) -> void { throw 0; },
                                 [](int &&) -> void { throw 0; }, [](int const &&) -> void {})) //
                .has_value());
      CHECK(std::move(s)
                .transform( //
                    fn::overload([](int &) -> void { throw 0; }, [](int const &) -> void { throw 0; },
                                 [](int &&) -> void {}, [](int const &&) -> void { throw 0; })) //
                .has_value());
    }

    WHEN("error")
    {
      fn::expected<int, Error> s{std::unexpect, Unknown};
      CHECK(s.transform([](auto) -> bool { throw 0; }).error() == Unknown);
      CHECK(std::as_const(s).transform([](auto) -> bool { throw 0; }).error() == Unknown);
      CHECK(std::move(std::as_const(s)).transform([](auto) -> bool { throw 0; }).error() == Unknown);
      CHECK(std::move(s).transform([](auto) -> bool { throw 0; }).error() == Unknown);
    }
  }

  WHEN("void")
  {
    fn::expected<void, Error> s{};
    CHECK(s.transform([]() -> bool { return true; }).value());
    CHECK(std::as_const(s).transform([]() -> bool { return true; }).value());
    CHECK(std::move(std::as_const(s)).transform([]() -> bool { return true; }).value());
    CHECK(std::move(s).transform([]() -> bool { return true; }).value());

    WHEN("void result")
    {
      fn::expected<void, Error> s{};
      CHECK(s.transform([]() -> void {}).has_value());
      CHECK(std::as_const(s).transform([]() -> void {}).has_value());
      CHECK(std::move(std::as_const(s)).transform([]() -> void {}).has_value());
      CHECK(std::move(s).transform([]() -> void {}).has_value());
    }

    WHEN("error")
    {
      fn::expected<void, Error> s{std::unexpect, Unknown};
      CHECK(s.transform([]() -> bool { throw 0; }).error() == Unknown);
      CHECK(std::as_const(s).transform([]() -> bool { throw 0; }).error() == Unknown);
      CHECK(std::move(std::as_const(s)).transform([]() -> bool { throw 0; }).error() == Unknown);
      CHECK(std::move(s).transform([]() -> bool { throw 0; }).error() == Unknown);
    }
  }
}

TEST_CASE("expected polyfills transform_error", "[expected][polyfill][transform_error][pack]")
{
  WHEN("value")
  {
    fn::expected<int, Error> s{12};
    CHECK(s.transform_error([](Error) -> bool { throw 0; }).value() == 12);
    CHECK(std::as_const(s).transform_error([](Error) -> bool { throw 0; }).value() == 12);
    CHECK(fn::expected<int, Error>{12}.transform_error([](Error) -> bool { throw 0; }).value() == 12);
    CHECK(std::move(std::as_const(s)).transform_error([](Error) -> bool { throw 0; }).value() == 12);

    WHEN("error")
    {
      fn::expected<int, Error> s{std::unexpect, FileNotFound};
      CHECK(
          s.transform_error( //
               fn::overload([](Error &e) -> bool { return e == FileNotFound; }, [](Error const &) -> bool { throw 0; },
                            [](Error &&) -> bool { throw 0; }, [](Error const &&) -> bool { throw 0; })) //
              .error());
      CHECK(std::as_const(s)
                .transform_error( //
                    fn::overload([](Error &) -> bool { throw 0; },
                                 [](Error const &e) -> bool { return e == FileNotFound; },
                                 [](Error &&) -> bool { throw 0; }, [](Error const &&) -> bool { throw 0; })) //
                .error());
      CHECK(std::move(std::as_const(s))
                .transform_error( //
                    fn::overload([](Error &) -> bool { throw 0; }, [](Error const &) -> bool { throw 0; },
                                 [](Error &&) -> bool { throw 0; },
                                 [](Error const &&e) -> bool { return e == FileNotFound; })) //
                .error());
      CHECK(std::move(s)
                .transform_error( //
                    fn::overload([](Error &) -> bool { throw 0; }, [](Error const &) -> bool { throw 0; },
                                 [](Error &&e) -> bool { return e == FileNotFound; },
                                 [](Error const &&) -> bool { throw 0; })) //
                .error());
    }

    WHEN("pack error")
    {
      fn::expected<int, fn::pack<int, Error>> s{std::unexpect, fn::pack{12, FileNotFound}};
      CHECK(s.transform_error( //
                 fn::overload([](int, Error &e) -> bool { return e == FileNotFound; },
                              [](int, Error const &) -> bool { throw 0; }, [](int, Error &&) -> bool { throw 0; },
                              [](int, Error const &&) -> bool { throw 0; })) //
                .error());
      CHECK(
          std::as_const(s)
              .transform_error( //
                  fn::overload([](int, Error &) -> bool { throw 0; },
                               [](int, Error const &e) -> bool { return e == FileNotFound; },
                               [](int, Error &&) -> bool { throw 0; }, [](int, Error const &&) -> bool { throw 0; })) //
              .error());
      CHECK(std::move(std::as_const(s))
                .transform_error( //
                    fn::overload([](int, Error &) -> bool { throw 0; }, [](int, Error const &) -> bool { throw 0; },
                                 [](int, Error &&) -> bool { throw 0; },
                                 [](int, Error const &&e) -> bool { return e == FileNotFound; })) //
                .error());
      CHECK(std::move(s)
                .transform_error( //
                    fn::overload([](int, Error &) -> bool { throw 0; }, [](int, Error const &) -> bool { throw 0; },
                                 [](int, Error &&e) -> bool { return e == FileNotFound; },
                                 [](int, Error const &&) -> bool { throw 0; })) //
                .error());
    }
  }

  WHEN("void")
  {
    fn::expected<void, Error> s{};
    CHECK(s.transform_error([](auto) -> bool { throw 0; }).has_value());
    CHECK(std::as_const(s).transform_error([](auto) -> bool { throw 0; }).has_value());
    CHECK(fn::expected<void, Error>{}.transform_error([](auto) -> bool { throw 0; }).has_value());
    CHECK(std::move(std::as_const(s)).transform_error([](auto) -> bool { throw 0; }).has_value());

    WHEN("error")
    {
      fn::expected<void, Error> s{std::unexpect, FileNotFound};
      CHECK(s.transform_error( //
                 fn::overload([](Error &) -> bool { return true; }, [](Error const &) -> bool { throw 0; },
                              [](Error &&) -> bool { throw 0; }, [](Error const &&) -> bool { throw 0; }))
                .error());
      CHECK(std::as_const(s)
                .transform_error( //
                    fn::overload([](Error &) -> bool { throw 0; }, [](Error const &) -> bool { return true; },
                                 [](Error &&) -> bool { throw 0; }, [](Error const &&) -> bool { throw 0; }))
                .error());
      CHECK(std::move(std::as_const(s))
                .transform_error( //
                    fn::overload([](Error &) -> bool { throw 0; }, [](Error const &) -> bool { throw 0; },
                                 [](Error &&) -> bool { throw 0; }, [](Error const &&) -> bool { return true; }))
                .error());
      CHECK(std::move(s)
                .transform_error( //
                    fn::overload([](Error &) -> bool { throw 0; }, [](Error const &) -> bool { throw 0; },
                                 [](Error &&) -> bool { return true; }, [](Error const &&) -> bool { throw 0; }))
                .error());
    }

    WHEN("pack error")
    {
      fn::expected<void, fn::pack<int, Error>> s{std::unexpect, fn::pack{12, FileNotFound}};
      CHECK(s.transform_error( //
                 fn::overload([](int, Error &) -> bool { return true; }, [](int, Error const &) -> bool { throw 0; },
                              [](int, Error &&) -> bool { throw 0; }, [](int, Error const &&) -> bool { throw 0; }))
                .error());
      CHECK(std::as_const(s)
                .transform_error( //
                    fn::overload([](int, Error &) -> bool { throw 0; }, [](int, Error const &) -> bool { return true; },
                                 [](int, Error &&) -> bool { throw 0; }, [](int, Error const &&) -> bool { throw 0; }))
                .error());
      CHECK(std::move(std::as_const(s))
                .transform_error( //
                    fn::overload([](int, Error &) -> bool { throw 0; }, [](int, Error const &) -> bool { throw 0; },
                                 [](int, Error &&) -> bool { throw 0; },
                                 [](int, Error const &&) -> bool { return true; }))
                .error());
      CHECK(std::move(s)
                .transform_error( //
                    fn::overload([](int, Error &) -> bool { throw 0; }, [](int, Error const &) -> bool { throw 0; },
                                 [](int, Error &&) -> bool { return true; },
                                 [](int, Error const &&) -> bool { throw 0; }))
                .error());
    }
  }
}
