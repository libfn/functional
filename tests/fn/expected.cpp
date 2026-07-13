// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/expected.hpp>
#include <fn/utility.hpp>

#include <catch2/catch_all.hpp>

#include <memory>
#include <string>
#include <utility>
#include <variant>

namespace {
enum Error { Unknown, FileNotFound };

struct Xint {
  int value;
  constexpr bool operator==(Xint const &) const noexcept = default;
};

// Sums whose alternatives include a non-builtin (Error/Xint/std::string_view/fn::pack — any
// class/struct/enum) have platform-specific order (see sum.cpp); pure-builtin sums keep `sum<...>`.
} // namespace

TEST_CASE("graded monad", "[expected][sum][graded][and_then][or_else][sum_value][sum_error]")
{
  SECTION("unit")
  {
    constexpr fn::expected<void, fn::sum<>> unit{};
    static_assert(unit.has_value());

    SECTION("constexpr")
    {
      SECTION("and_then to value/sum<>")
      {
        constexpr auto fn = []() -> fn::expected<int, fn::sum<>> { return {7}; };
        constexpr auto a = unit.and_then(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<int, fn::sum<>> const>);
        static_assert(a.value() == 7);
      }

      SECTION("and_then to value")
      {
        constexpr auto fn = []() -> fn::expected<int, Error> { return {12}; };
        constexpr auto a = unit.and_then(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<int, fn::sum<Error>> const>);
        static_assert(a.value() == 12);
      }

      SECTION("and_then to error")
      {
        constexpr auto fn = []() -> fn::expected<int, Error> { return ::fn::unexpected<Error>(FileNotFound); };
        constexpr auto a = unit.and_then(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<int, fn::sum<Error>> const>);
        static_assert(a.error() == fn::sum{FileNotFound});
      }

      SECTION("transform to int")
      {
        constexpr auto fn = []() -> int { return 144'000; };
        constexpr auto a = unit.transform(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<int, fn::sum<>> const>);
        static_assert(a.value() == 144'000);
      }
    }

    SECTION("runtime")
    {
      SECTION("and_then to value/sum<>")
      {
        constexpr auto fn = []() -> fn::expected<int, fn::sum<>> { return {7}; };
        auto a = unit.and_then(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<int, fn::sum<>>>);
        CHECK(a.value() == 7);
      }

      SECTION("and_then to value")
      {
        constexpr auto fn = []() -> fn::expected<int, Error> { return {12}; };
        auto a = unit.and_then(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<int, fn::sum<Error>>>);
        CHECK(a.value() == 12);
      }

      SECTION("and_then to error")
      {
        constexpr auto fn = []() -> fn::expected<int, Error> { return ::fn::unexpected<Error>(FileNotFound); };
        auto a = unit.and_then(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<int, fn::sum<Error>>>);
        CHECK(a.error() == fn::sum{FileNotFound});
      }

      SECTION("transform to int")
      {
        constexpr auto fn = []() -> int { return 144'000; };
        auto a = unit.transform(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<int, fn::sum<>>>);
        CHECK(a.value() == 144'000);
      }

      SECTION("transform direct-initializes its result")
      {
        // the value is direct-non-list-initialized from the callable's result: no extra
        // move, and an immovable type works
        struct immovable_t {
          int v;
          constexpr explicit immovable_t(int i) noexcept : v(i) {}
          immovable_t(immovable_t &&) = delete;
        };
        constexpr auto fn = []() -> immovable_t { return immovable_t(7); };
        auto a = unit.transform(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<immovable_t, fn::sum<>>>);
        CHECK(a.value().v == 7);

        // the from-invoke tag ctor backing this is not part of the public interface
        // (is_constructible_v cannot see private ctors)
        static_assert(
            not std::is_constructible_v<fn::expected<immovable_t, fn::sum<>>, pfn::detail::_expected_from_invoke_t,
                                        std::in_place_t, immovable_t (*)()>);
      }
    }
  }

  SECTION("sum_error from sum")
  {
    using T = fn::expected<int, fn::sum<Error>>;
    T s{12};
    static_assert(std::is_same_v<decltype(s.sum_error()), T &>);
    static_assert(std::is_same_v<decltype(std::as_const(s).sum_error()), T const &>);
    static_assert(std::is_same_v<decltype(std::move(std::as_const(s)).sum_error()), T const &&>);
    static_assert(std::is_same_v<decltype(std::move(s).sum_error()), T &&>);
    // these overloads only return *this
    static_assert(noexcept(s.sum_error()));
    static_assert(noexcept(std::as_const(s).sum_error()));
    static_assert(noexcept(std::move(std::as_const(s)).sum_error()));
    static_assert(noexcept(std::move(s).sum_error()));
    SECTION("value")
    {
      CHECK(s.sum_error().value() == 12);
      CHECK(std::as_const(s).sum_error().value() == 12);
      CHECK(std::move(std::as_const(s)).sum_error().value() == 12);
      CHECK(std::move(s).sum_error().value() == 12);
    }
    SECTION("error")
    {
      T s{::fn::unexpect, Unknown};
      CHECK(s.sum_error().error() == fn::sum{Unknown});
      CHECK(std::as_const(s).sum_error().error() == fn::sum{Unknown});
      CHECK(std::move(std::as_const(s)).sum_error().error() == fn::sum{Unknown});
      CHECK(std::move(s).sum_error().error() == fn::sum{Unknown});
    }

    static_assert(std::is_same_v<decltype(fn::sum_error(s)), T &>);
    static_assert(noexcept(fn::sum_error(s))); // the free function propagates what the member says
  }

  SECTION("sum_error from non-sum")
  {
    using T = fn::expected<int, Error>;
    T s{12};
    static_assert(std::is_same_v<decltype(s.sum_error()), fn::expected<int, fn::sum<Error>>>);
    static_assert(std::is_same_v<decltype(std::as_const(s).sum_error()), fn::expected<int, fn::sum<Error>>>);
    static_assert(std::is_same_v<decltype(std::move(std::as_const(s)).sum_error()), fn::expected<int, fn::sum<Error>>>);
    static_assert(std::is_same_v<decltype(std::move(s).sum_error()), fn::expected<int, fn::sum<Error>>>);
    // this overload wraps the error in a sum and relocates the value, so it weighs both - neither of
    // which can throw here
    static_assert(noexcept(s.sum_error()));
    static_assert(noexcept(std::as_const(s).sum_error()));
    static_assert(noexcept(std::move(std::as_const(s)).sum_error()));
    static_assert(noexcept(std::move(s).sum_error()));
    SECTION("value")
    {
      CHECK(s.sum_error().value() == 12);
      CHECK(std::as_const(s).sum_error().value() == 12);
      CHECK(std::move(std::as_const(s)).sum_error().value() == 12);
      CHECK(std::move(s).sum_error().value() == 12);
    }
    SECTION("error")
    {
      T s{::fn::unexpect, Unknown};
      CHECK(s.sum_error().error() == fn::sum{Unknown});
      CHECK(std::as_const(s).sum_error().error() == fn::sum{Unknown});
      CHECK(std::move(std::as_const(s)).sum_error().error() == fn::sum{Unknown});
      CHECK(std::move(s).sum_error().error() == fn::sum{Unknown});
    }

    static_assert(std::is_same_v<decltype(fn::sum_error(s)), fn::expected<int, fn::sum<Error>>>);
    static_assert(noexcept(fn::sum_error(s))); // the free function propagates what the member says

    SECTION("throwing value")
    {
      // the lift weighs the side it does not touch: relocating the value is what can throw here, so
      // the promise tracks the category that value is relocated by
      using W = fn::expected<std::string, Error>;
      static_assert(not noexcept(std::declval<W const &>().sum_error())); // copies
      static_assert(noexcept(std::declval<W &&>().sum_error()));          // moves
      SUCCEED();
    }
  }

  SECTION("sum_error from non-sum, void value")
  {
    using T = fn::expected<void, Error>;
    T s{};
    static_assert(std::is_same_v<decltype(s.sum_error()), fn::expected<void, fn::sum<Error>>>);
    static_assert(std::is_same_v<decltype(std::as_const(s).sum_error()), fn::expected<void, fn::sum<Error>>>);
    static_assert(
        std::is_same_v<decltype(std::move(std::as_const(s)).sum_error()), fn::expected<void, fn::sum<Error>>>);
    static_assert(std::is_same_v<decltype(std::move(s).sum_error()), fn::expected<void, fn::sum<Error>>>);
    // with a void value there is nothing to relocate, so the lift weighs only the error it wraps
    static_assert(noexcept(s.sum_error()));
    static_assert(noexcept(std::as_const(s).sum_error()));
    static_assert(noexcept(std::move(std::as_const(s)).sum_error()));
    static_assert(noexcept(std::move(s).sum_error()));
    SECTION("value")
    {
      CHECK(s.sum_error().has_value());
      CHECK(std::as_const(s).sum_error().has_value());
      CHECK(std::move(std::as_const(s)).sum_error().has_value());
      CHECK(std::move(s).sum_error().has_value());
    }
    SECTION("error")
    {
      T s{::fn::unexpect, Unknown};
      CHECK(s.sum_error().error() == fn::sum{Unknown});
      CHECK(std::as_const(s).sum_error().error() == fn::sum{Unknown});
      CHECK(std::move(std::as_const(s)).sum_error().error() == fn::sum{Unknown});
      CHECK(std::move(s).sum_error().error() == fn::sum{Unknown});
    }

    static_assert(std::is_same_v<decltype(fn::sum_error(s)), fn::expected<void, fn::sum<Error>>>);
    static_assert(noexcept(fn::sum_error(s))); // the free function propagates what the member says
  }

  SECTION("sum_error from sum, void value")
  {
    using T = fn::expected<void, fn::sum<Error>>;
    T s{};
    static_assert(std::is_same_v<decltype(s.sum_error()), T &>);
    static_assert(std::is_same_v<decltype(std::as_const(s).sum_error()), T const &>);
    static_assert(std::is_same_v<decltype(std::move(std::as_const(s)).sum_error()), T const &&>);
    static_assert(std::is_same_v<decltype(std::move(s).sum_error()), T &&>);
    // the void-value self-return overloads only return *this
    static_assert(noexcept(s.sum_error()));
    static_assert(noexcept(std::as_const(s).sum_error()));
    static_assert(noexcept(std::move(std::as_const(s)).sum_error()));
    static_assert(noexcept(std::move(s).sum_error()));
    SECTION("value")
    {
      CHECK(s.sum_error().has_value());
      CHECK(std::as_const(s).sum_error().has_value());
      CHECK(std::move(std::as_const(s)).sum_error().has_value());
      CHECK(std::move(s).sum_error().has_value());
    }
    SECTION("error")
    {
      T s{::fn::unexpect, Unknown};
      CHECK(s.sum_error().error() == fn::sum{Unknown});
      CHECK(std::as_const(s).sum_error().error() == fn::sum{Unknown});
      CHECK(std::move(std::as_const(s)).sum_error().error() == fn::sum{Unknown});
      CHECK(std::move(s).sum_error().error() == fn::sum{Unknown});
    }

    static_assert(std::is_same_v<decltype(fn::sum_error(s)), T &>);
    static_assert(noexcept(fn::sum_error(s))); // the free function propagates what the member says
  }

  SECTION("constexpr")
  {
    static_assert([] {
      fn::expected<int, Error> const a{::fn::unexpect, Unknown};
      return a.sum_error().error() == fn::sum{Unknown};
    }());
    static_assert([] { return fn::expected<int, Error>{12}.sum_error().value() == 12; }());
    static_assert([] { return fn::expected<int, Error>{12}.sum_value().value() == fn::sum{12}; }());
    static_assert([] {
      fn::expected<void, Error> const a{::fn::unexpect, Unknown};
      return a.sum_error().error() == fn::sum{Unknown};
    }());
    static_assert([] {
      fn::expected<int, Error> a{12};
      return fn::sum_value(a).value() == fn::sum{12};
    }());
    SUCCEED();
  }

  SECTION("sum_value from sum")
  {
    using T = fn::expected<fn::sum<int>, Error>;
    T s{12};
    static_assert(std::is_same_v<decltype(s.sum_value()), T &>);
    static_assert(std::is_same_v<decltype(std::as_const(s).sum_value()), T const &>);
    static_assert(std::is_same_v<decltype(std::move(std::as_const(s)).sum_value()), T const &&>);
    static_assert(std::is_same_v<decltype(std::move(s).sum_value()), T &&>);
    // these overloads only return *this
    static_assert(noexcept(s.sum_value()));
    static_assert(noexcept(std::as_const(s).sum_value()));
    static_assert(noexcept(std::move(std::as_const(s)).sum_value()));
    static_assert(noexcept(std::move(s).sum_value()));
    SECTION("value")
    {
      CHECK(s.sum_value().value() == fn::sum{12});
      CHECK(std::as_const(s).sum_value().value() == fn::sum{12});
      CHECK(std::move(std::as_const(s)).sum_value().value() == fn::sum{12});
      CHECK(std::move(s).sum_value().value() == fn::sum{12});
    }
    SECTION("error")
    {
      T s{::fn::unexpect, Unknown};
      CHECK(s.sum_value().error() == Unknown);
      CHECK(std::as_const(s).sum_value().error() == Unknown);
      CHECK(std::move(std::as_const(s)).sum_value().error() == Unknown);
      CHECK(std::move(s).sum_value().error() == Unknown);
    }

    static_assert(std::is_same_v<decltype(fn::sum_value(s)), T &>);
    static_assert(noexcept(fn::sum_value(s))); // the free function propagates what the member says
  }

  SECTION("sum_value from non-sum")
  {
    using T = fn::expected<int, Error>;
    T s{12};
    static_assert(std::is_same_v<decltype(s.sum_value()), fn::expected<fn::sum<int>, Error>>);
    static_assert(std::is_same_v<decltype(std::as_const(s).sum_value()), fn::expected<fn::sum<int>, Error>>);
    static_assert(std::is_same_v<decltype(std::move(std::as_const(s)).sum_value()), fn::expected<fn::sum<int>, Error>>);
    static_assert(std::is_same_v<decltype(std::move(s).sum_value()), fn::expected<fn::sum<int>, Error>>);
    // this overload wraps the value in a sum and relocates the error, so it weighs both - neither of
    // which can throw here
    static_assert(noexcept(s.sum_value()));
    static_assert(noexcept(std::as_const(s).sum_value()));
    static_assert(noexcept(std::move(std::as_const(s)).sum_value()));
    static_assert(noexcept(std::move(s).sum_value()));
    SECTION("value")
    {
      CHECK(s.sum_value().value() == fn::sum{12});
      CHECK(std::as_const(s).sum_value().value() == fn::sum{12});
      CHECK(std::move(std::as_const(s)).sum_value().value() == fn::sum{12});
      CHECK(std::move(s).sum_value().value() == fn::sum{12});
    }
    SECTION("error")
    {
      T s{::fn::unexpect, Unknown};
      CHECK(s.sum_value().error() == Unknown);
      CHECK(std::as_const(s).sum_value().error() == Unknown);
      CHECK(std::move(std::as_const(s)).sum_value().error() == Unknown);
      CHECK(std::move(s).sum_value().error() == Unknown);
    }

    static_assert(std::is_same_v<decltype(fn::sum_value(s)), fn::expected<fn::sum<int>, Error>>);
    static_assert(noexcept(fn::sum_value(s))); // the free function propagates what the member says
  }

  SECTION("sum_value absent for void value")
  {
    // by design: a void value cannot be sum-wrapped -- the void specialization has no
    // sum_value member and the free fn::sum_value is constrained some_expected_non_void
    // (include/fn/expected.hpp:1020); sum_error, by contrast, serves void (asserted above)
    constexpr auto can_member_sum_value = [](auto &&e) { return requires { e.sum_value(); }; };
    constexpr auto can_free_sum_value = [](auto &&e) { return requires { fn::sum_value(e); }; };
    static_assert(not can_member_sum_value(fn::expected<void, Error>{}));
    static_assert(not can_free_sum_value(fn::expected<void, Error>{}));
    static_assert(can_member_sum_value(fn::expected<int, Error>{1}));
    static_assert(can_free_sum_value(fn::expected<int, Error>{1}));
    SUCCEED();
  }

  SECTION("and_then")
  {
    SECTION("value to value")
    {
      fn::expected<int, fn::sum<Error>> s{12};

      constexpr auto fn1 = [](int) -> fn::expected<int, bool> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn1)), fn::expected<int, fn::sum_for<Error, bool>>>);
      constexpr auto fn2 = [](int) -> fn::expected<int, Error> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn2)), fn::expected<int, fn::sum<Error>>>);
      constexpr auto fn3 = [](int) -> fn::expected<int, fn::sum<Error>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn3)), fn::expected<int, fn::sum<Error>>>);
      constexpr auto fn4 = [](int) -> fn::expected<int, fn::sum<bool>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn4)), fn::expected<int, fn::sum_for<Error, bool>>>);
      constexpr auto fn5 = [](int) -> fn::expected<int, fn::sum_for<Error, bool>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn5)), fn::expected<int, fn::sum_for<Error, bool>>>);
      constexpr auto fn6 = [](int) -> fn::expected<int, fn::sum<bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn6)), fn::expected<int, fn::sum_for<Error, bool, int>>>);
      constexpr auto fn7 = [](int) -> fn::expected<int, fn::sum_for<Error, bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn7)), fn::expected<int, fn::sum_for<Error, bool, int>>>);
      constexpr auto fn8 = [](int) -> fn::expected<Xint, fn::sum_for<Error, bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn8)), fn::expected<Xint, fn::sum_for<Error, bool, int>>>);

      SECTION("value to value")
      {
        constexpr auto fn = [](int i) -> fn::expected<int, bool> { return {i + 12}; };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<int, fn::sum_for<Error, bool>>>);
        CHECK(s.and_then(fn).value() == 24);
        CHECK(std::as_const(s).and_then(fn).value() == 24);
        CHECK(std::move(std::as_const(s)).and_then(fn).value() == 24);
        CHECK(std::move(s).and_then(fn).value() == 24);
      }

      SECTION("value to error")
      {
        constexpr auto fn = [](int i) -> fn::expected<int, bool> { return ::fn::unexpected<bool>(i >= 1); };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<int, fn::sum_for<Error, bool>>>);
        CHECK(s.and_then(fn).error() == fn::sum{true});
        CHECK(std::as_const(s).and_then(fn).error() == fn::sum{true});
        CHECK(std::move(std::as_const(s)).and_then(fn).error() == fn::sum{true});
        CHECK(std::move(s).and_then(fn).error() == fn::sum{true});
      }

      SECTION("error")
      {
        fn::expected<int, fn::sum<Error>> s{::fn::unexpect, fn::sum{FileNotFound}};
        constexpr auto fn = [](int) -> fn::expected<int, bool> { throw 0; };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<int, fn::sum_for<Error, bool>>>);
        CHECK(s.and_then(fn).error() == fn::sum{FileNotFound});
        CHECK(s.and_then(fn).error() != fn::sum{false});
        CHECK(s.and_then(fn).error() != fn::sum{true});
        CHECK(std::as_const(s).and_then(fn).error() == fn::sum{FileNotFound});
        CHECK(std::move(std::as_const(s)).and_then(fn).error() == fn::sum{FileNotFound});
        CHECK(std::move(s).and_then(fn).error() == fn::sum{FileNotFound});
      }
    }

    SECTION("void to value")
    {
      fn::expected<void, fn::sum<Error>> s{};

      constexpr auto fn1 = []() -> fn::expected<int, bool> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn1)), fn::expected<int, fn::sum_for<Error, bool>>>);
      constexpr auto fn2 = []() -> fn::expected<int, Error> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn2)), fn::expected<int, fn::sum<Error>>>);
      constexpr auto fn3 = []() -> fn::expected<int, fn::sum<Error>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn3)), fn::expected<int, fn::sum<Error>>>);
      constexpr auto fn4 = []() -> fn::expected<int, fn::sum<bool>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn4)), fn::expected<int, fn::sum_for<Error, bool>>>);
      constexpr auto fn5 = []() -> fn::expected<int, fn::sum_for<Error, bool>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn5)), fn::expected<int, fn::sum_for<Error, bool>>>);
      constexpr auto fn6 = []() -> fn::expected<int, fn::sum<bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn6)), fn::expected<int, fn::sum_for<Error, bool, int>>>);
      constexpr auto fn7 = []() -> fn::expected<int, fn::sum_for<Error, bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn7)), fn::expected<int, fn::sum_for<Error, bool, int>>>);

      SECTION("value to value")
      {
        constexpr auto fn = []() -> fn::expected<int, bool> { return {12}; };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<int, fn::sum_for<Error, bool>>>);
        CHECK(s.and_then(fn).value() == 12);
        CHECK(std::as_const(s).and_then(fn).value() == 12);
        CHECK(std::move(std::as_const(s)).and_then(fn).value() == 12);
        CHECK(std::move(s).and_then(fn).value() == 12);
      }

      SECTION("value to error")
      {
        constexpr auto fn = []() -> fn::expected<int, bool> { return ::fn::unexpected<bool>(true); };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<int, fn::sum_for<Error, bool>>>);
        CHECK(s.and_then(fn).error() == fn::sum{true});
        CHECK(std::as_const(s).and_then(fn).error() == fn::sum{true});
        CHECK(std::move(std::as_const(s)).and_then(fn).error() == fn::sum{true});
        CHECK(std::move(s).and_then(fn).error() == fn::sum{true});
      }

      SECTION("error")
      {
        fn::expected<void, fn::sum<Error>> s{::fn::unexpect, fn::sum{FileNotFound}};
        constexpr auto fn = []() -> fn::expected<int, bool> { throw 0; };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<int, fn::sum_for<Error, bool>>>);
        CHECK(s.and_then(fn).error() == fn::sum{FileNotFound});
        CHECK(s.and_then(fn).error() != fn::sum{false});
        CHECK(s.and_then(fn).error() != fn::sum{true});
        CHECK(std::as_const(s).and_then(fn).error() == fn::sum{FileNotFound});
        CHECK(std::move(std::as_const(s)).and_then(fn).error() == fn::sum{FileNotFound});
        CHECK(std::move(s).and_then(fn).error() == fn::sum{FileNotFound});
      }
    }

    SECTION("value to void")
    {
      fn::expected<int, fn::sum<Error>> s{12};

      constexpr auto fn1 = [](int) -> fn::expected<void, bool> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn1)), fn::expected<void, fn::sum_for<Error, bool>>>);
      constexpr auto fn2 = [](int) -> fn::expected<void, Error> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn2)), fn::expected<void, fn::sum<Error>>>);
      constexpr auto fn3 = [](int) -> fn::expected<void, fn::sum<Error>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn3)), fn::expected<void, fn::sum<Error>>>);
      constexpr auto fn4 = [](int) -> fn::expected<void, fn::sum<bool>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn4)), fn::expected<void, fn::sum_for<Error, bool>>>);
      constexpr auto fn5 = [](int) -> fn::expected<void, fn::sum_for<Error, bool>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn5)), fn::expected<void, fn::sum_for<Error, bool>>>);
      constexpr auto fn6 = [](int) -> fn::expected<void, fn::sum<bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn6)), fn::expected<void, fn::sum_for<Error, bool, int>>>);
      constexpr auto fn7 = [](int) -> fn::expected<void, fn::sum_for<Error, bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn7)), fn::expected<void, fn::sum_for<Error, bool, int>>>);

      SECTION("value to value")
      {
        constexpr auto fn = [](int) -> fn::expected<void, bool> { return {}; };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<void, fn::sum_for<Error, bool>>>);
        CHECK(s.and_then(fn).has_value());
        CHECK(std::as_const(s).and_then(fn).has_value());
        CHECK(std::move(std::as_const(s)).and_then(fn).has_value());
        CHECK(std::move(s).and_then(fn).has_value());
      }

      SECTION("value to error")
      {
        constexpr auto fn = [](int i) -> fn::expected<void, bool> { return ::fn::unexpected<bool>(i >= 1); };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<void, fn::sum_for<Error, bool>>>);
        CHECK(s.and_then(fn).error() == fn::sum{true});
        CHECK(std::as_const(s).and_then(fn).error() == fn::sum{true});
        CHECK(std::move(std::as_const(s)).and_then(fn).error() == fn::sum{true});
        CHECK(std::move(s).and_then(fn).error() == fn::sum{true});
      }

      SECTION("error")
      {
        fn::expected<int, fn::sum<Error>> s{::fn::unexpect, fn::sum{FileNotFound}};
        constexpr auto fn = [](int) -> fn::expected<void, bool> { throw 0; };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<void, fn::sum_for<Error, bool>>>);
        CHECK(s.and_then(fn).error() == fn::sum{FileNotFound});
        CHECK(s.and_then(fn).error() != fn::sum{false});
        CHECK(s.and_then(fn).error() != fn::sum{true});
        CHECK(std::as_const(s).and_then(fn).error() == fn::sum{FileNotFound});
        CHECK(std::move(std::as_const(s)).and_then(fn).error() == fn::sum{FileNotFound});
        CHECK(std::move(s).and_then(fn).error() == fn::sum{FileNotFound});
      }
    }

    SECTION("void to void")
    {
      fn::expected<void, fn::sum<Error>> s{};

      constexpr auto fn1 = []() -> fn::expected<void, bool> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn1)), fn::expected<void, fn::sum_for<Error, bool>>>);
      constexpr auto fn2 = []() -> fn::expected<void, Error> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn2)), fn::expected<void, fn::sum<Error>>>);
      constexpr auto fn3 = []() -> fn::expected<void, fn::sum<Error>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn3)), fn::expected<void, fn::sum<Error>>>);
      constexpr auto fn4 = []() -> fn::expected<void, fn::sum<bool>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn4)), fn::expected<void, fn::sum_for<Error, bool>>>);
      constexpr auto fn5 = []() -> fn::expected<void, fn::sum_for<Error, bool>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn5)), fn::expected<void, fn::sum_for<Error, bool>>>);
      constexpr auto fn6 = []() -> fn::expected<void, fn::sum<bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn6)), fn::expected<void, fn::sum_for<Error, bool, int>>>);
      constexpr auto fn7 = []() -> fn::expected<void, fn::sum_for<Error, bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn7)), fn::expected<void, fn::sum_for<Error, bool, int>>>);

      SECTION("value to value")
      {
        constexpr auto fn = []() -> fn::expected<void, bool> { return {}; };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<void, fn::sum_for<Error, bool>>>);
        CHECK(s.and_then(fn).has_value());
        CHECK(std::as_const(s).and_then(fn).has_value());
        CHECK(std::move(std::as_const(s)).and_then(fn).has_value());
        CHECK(std::move(s).and_then(fn).has_value());
      }

      SECTION("value to error")
      {
        constexpr auto fn = []() -> fn::expected<void, bool> { return ::fn::unexpected<bool>(true); };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<void, fn::sum_for<Error, bool>>>);
        CHECK(s.and_then(fn).error() == fn::sum{true});
        CHECK(std::as_const(s).and_then(fn).error() == fn::sum{true});
        CHECK(std::move(std::as_const(s)).and_then(fn).error() == fn::sum{true});
        CHECK(std::move(s).and_then(fn).error() == fn::sum{true});
      }

      SECTION("error")
      {
        fn::expected<void, fn::sum<Error>> s{::fn::unexpect, fn::sum{FileNotFound}};
        constexpr auto fn = []() -> fn::expected<void, bool> { throw 0; };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<void, fn::sum_for<Error, bool>>>);
        CHECK(s.and_then(fn).error() == fn::sum{FileNotFound});
        CHECK(s.and_then(fn).error() != fn::sum{false});
        CHECK(s.and_then(fn).error() != fn::sum{true});
        CHECK(std::as_const(s).and_then(fn).error() == fn::sum{FileNotFound});
        CHECK(std::move(std::as_const(s)).and_then(fn).error() == fn::sum{FileNotFound});
        CHECK(std::move(s).and_then(fn).error() == fn::sum{FileNotFound});
      }
    }
  }

  SECTION("or_else")
  {
    fn::expected<fn::sum<int>, Error> s{::fn::unexpect, FileNotFound};

    constexpr auto fn1 = [](int) -> fn::expected<Xint, Error> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn1)), fn::expected<fn::sum_for<Xint, int>, Error>>);
    constexpr auto fn2 = [](int) -> fn::expected<int, Error> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn2)), fn::expected<fn::sum<int>, Error>>);
    constexpr auto fn3 = [](int) -> fn::expected<fn::sum<int>, Error> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn3)), fn::expected<fn::sum<int>, Error>>);
    constexpr auto fn4 = [](int) -> fn::expected<fn::sum<Xint>, Error> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn4)), fn::expected<fn::sum_for<Xint, int>, Error>>);
    constexpr auto fn5 = [](int) -> fn::expected<fn::sum_for<Xint, int>, Error> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn5)), fn::expected<fn::sum_for<Xint, int>, Error>>);
    constexpr auto fn6 = [](int) -> fn::expected<fn::sum_for<Xint, long>, Error> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn6)), fn::expected<fn::sum_for<Xint, int, long>, Error>>);
    constexpr auto fn7 = [](int) -> fn::expected<fn::sum_for<Xint, int, long>, Error> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn7)), fn::expected<fn::sum_for<Xint, int, long>, Error>>);
    constexpr auto fn8 = [](int) -> fn::expected<fn::sum_for<Xint, int, long>, std::string> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn8)), fn::expected<fn::sum_for<Xint, int, long>, std::string>>);

    SECTION("error to value")
    {
      constexpr auto fn = [](Error) -> fn::expected<Xint, std::string> { return {Xint{12}}; };
      static_assert(std::is_same_v<decltype(s.or_else(fn)), fn::expected<fn::sum_for<Xint, int>, std::string>>);
      CHECK(s.or_else(fn).value() == fn::sum{Xint{12}});
      CHECK(std::as_const(s).or_else(fn).value() == fn::sum{Xint{12}});
      CHECK(std::move(std::as_const(s)).or_else(fn).value() == fn::sum{Xint{12}});
      CHECK(std::move(s).or_else(fn).value() == fn::sum{Xint{12}});
    }

    SECTION("error to error")
    {
      constexpr auto fn = [](Error) -> fn::expected<Xint, std::string> { return ::fn::unexpected<std::string>("Boo"); };
      static_assert(std::is_same_v<decltype(s.or_else(fn)), fn::expected<fn::sum_for<Xint, int>, std::string>>);
      CHECK(s.or_else(fn).error() == "Boo");
      CHECK(std::as_const(s).or_else(fn).error() == "Boo");
      CHECK(std::move(std::as_const(s)).or_else(fn).error() == "Boo");
      CHECK(std::move(s).or_else(fn).error() == "Boo");
    }

    SECTION("value")
    {
      fn::expected<fn::sum<int>, Error> s{fn::sum{12}};
      constexpr auto fn = [](int) -> fn::expected<Xint, std::string> { throw 0; };
      static_assert(std::is_same_v<decltype(s.or_else(fn)), fn::expected<fn::sum_for<Xint, int>, std::string>>);
      CHECK(s.or_else(fn).value() == fn::sum{12});
      CHECK(std::as_const(s).or_else(fn).value() == fn::sum{12});
      CHECK(std::move(std::as_const(s)).or_else(fn).value() == fn::sum{12});
      CHECK(std::move(s).or_else(fn).value() == fn::sum{12});
    }

    SECTION("engaged void source, immovable error type in the result")
    {
      // the value-state path must compile even though the result cannot be moved (the
      // clang<=18 miscompile workaround must not force a move)
      struct immovable_t {
        int v;
        constexpr explicit immovable_t(int i) noexcept : v(i) {}
        immovable_t(immovable_t &&) = delete;
      };
      fn::expected<void, Error> u{};
      auto r = u.or_else([](Error) -> fn::expected<void, immovable_t> { return {}; });
      static_assert(std::is_same_v<decltype(r), fn::expected<void, immovable_t>>);
      CHECK(r.has_value());
    }
  }
}

TEST_CASE("graded monad constexpr and runtime", "[constexpr][and_then][or_else][expected][graded][sum]")
{
  enum class Error : int { Unknown, InvalidValue };
  using T = fn::expected<int, fn::sum<Error>>;

  SECTION("and_then constexpr")
  {
    SECTION("same error type")
    {
      constexpr auto fn1 = [](int i) -> fn::expected<int, int> {
        if (i < 2)
          return {i + 1};
        return ::fn::unexpected<int>{i};
      };

      constexpr auto r1 = T{0}.and_then(fn1);
      static_assert(std::is_same_v<decltype(r1), fn::expected<int, fn::sum_for<Error, int>> const>);
      static_assert(r1.value() == 1);
      constexpr auto r2 = r1.and_then(fn1);
      static_assert(r2.value() == 2);
      constexpr auto r3 = r2.and_then(fn1);
      static_assert(r3.error() == fn::sum{2});
      constexpr auto r4 = r3.and_then(fn1);
      static_assert(r4.error() == fn::sum{2});

      SUCCEED();
    }

    SECTION("accummulate errors")
    {
      constexpr auto fn2 = [](int i) -> fn::expected<bool, Error> {
        if (i < 0 || i > 1)
          return ::fn::unexpected<Error>{Error::InvalidValue};
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
      static_assert(std::is_same_v<decltype(r4), fn::expected<int, fn::sum_for<Error, int>> const>);
      static_assert(r4.error() == fn::sum{Error::InvalidValue});

      constexpr auto r5 = T{2}.and_then(fn3);
      static_assert(std::is_same_v<decltype(r5), fn::expected<int, fn::sum_for<Error, int>> const>);
      static_assert(r5.value() == 3);

      SUCCEED();
    }
  }

  SECTION("and_then runtime")
  {
    SECTION("same error type")
    {
      constexpr auto fn1 = [](int i) -> fn::expected<int, int> {
        if (i < 2)
          return {i + 1};
        return ::fn::unexpected<int>{i};
      };

      auto const r1 = T{0}.and_then(fn1);
      static_assert(std::is_same_v<decltype(r1), fn::expected<int, fn::sum_for<Error, int>> const>);
      CHECK(r1.value() == 1);
      auto const r2 = r1.and_then(fn1);
      CHECK(r2.value() == 2);
      auto const r3 = r2.and_then(fn1);
      CHECK(r3.error() == fn::sum{2});
      auto const r4 = r3.and_then(fn1);
      CHECK(r4.error() == fn::sum{2});
    }

    SECTION("accummulate errors")
    {
      constexpr auto fn2 = [](int i) -> fn::expected<bool, Error> {
        if (i < 0 || i > 1)
          return ::fn::unexpected<Error>{Error::InvalidValue};
        return {i == 1};
      };

      auto const r2 = T{1}.and_then(fn2);
      static_assert(std::is_same_v<decltype(r2), fn::expected<bool, fn::sum<Error>> const>);
      CHECK(r2.value());
      auto const r3 = T{2}.and_then(fn2);
      CHECK(r3.error() == fn::sum{Error::InvalidValue});

      auto const fn3 = [](int i) -> fn::expected<int, int> { return {i + 1}; };
      auto const r4 = r3.and_then(fn3);
      static_assert(std::is_same_v<decltype(r4), fn::expected<int, fn::sum_for<Error, int>> const>);
      CHECK(r4.error() == fn::sum{Error::InvalidValue});
      auto const r5 = T{2}.and_then(fn3);
      CHECK(r5.value() == 3);
    }
  }

  SECTION("or_else constexpr")
  {
    using T = fn::expected<fn::sum<int>, Error>;

    constexpr auto fn1 = [](Error i) -> fn::expected<int, int> {
      if (i == Error::Unknown)
        return {0};
      return ::fn::unexpected<int>{(int)i};
    };

    constexpr auto r1 = T{14}.or_else(fn1);
    static_assert(std::is_same_v<decltype(r1), fn::expected<fn::sum<int>, int> const>);
    static_assert(r1.value() == fn::sum{14});
    constexpr auto r2 = T{::fn::unexpect, Error::InvalidValue}.or_else(fn1);
    static_assert(r2.error() == 1);
    constexpr auto r3 = T{::fn::unexpect, Error::Unknown}.or_else(fn1);
    static_assert(r3.value() == fn::sum{0});

    SUCCEED();
  }

  SECTION("or_else runtime")
  {
    using T = fn::expected<fn::sum<int>, Error>;

    constexpr auto fn1 = [](Error i) -> fn::expected<int, int> {
      if (i == Error::Unknown)
        return {0};
      return ::fn::unexpected<int>{(int)i};
    };

    auto const r1 = T{14}.or_else(fn1);
    static_assert(std::is_same_v<decltype(r1), fn::expected<fn::sum<int>, int> const>);
    CHECK(r1.value() == fn::sum{14});
    auto const r2 = T{::fn::unexpect, Error::InvalidValue}.or_else(fn1);
    CHECK(r2.error() == 1);
    auto const r3 = T{::fn::unexpect, Error::Unknown}.or_else(fn1);
    CHECK(r3.value() == fn::sum{0});
  }
}

TEST_CASE("expected pack support", "[expected][pack][and_then][transform][operator_and][graded][sum]")
{
  SECTION("and_then")
  {
    using S = fn::expected<fn::pack<int, std::string_view>, Error>;

    // noexcept (extension, expected.hpp:77-81): a multi-argument visitor is not invocable on
    // the whole pack, so the borrowed std trait is conservatively false (GH #254); a generic
    // same-error-type callback IS, and reports true
    constexpr auto nothrow_two = [](int &, std::string_view &) noexcept -> fn::expected<bool, Error> { return {true}; };
    static_assert(not noexcept(std::declval<S &>().and_then(nothrow_two)));
    constexpr auto nothrow_generic = [](auto &&...) noexcept -> fn::expected<bool, Error> { return {true}; };
    static_assert(noexcept(std::declval<S &>().and_then(nothrow_generic)));

    // constraints (extension, :82-83): pack-apply invocability tracking the pack's value
    // category; wrong arity or a non-callable SFINAE-drops
    constexpr auto can_and_then_lval = [](auto &&f) { return requires { std::declval<S &>().and_then(f); }; };
    constexpr auto can_and_then_rval = [](auto &&f) { return requires { std::declval<S &&>().and_then(f); }; };
    static_assert(can_and_then_lval(nothrow_two));
    static_assert(not can_and_then_rval(nothrow_two));                                         // lvalue-only visitor
    static_assert(not can_and_then_lval([](int &) -> fn::expected<bool, Error> { throw 0; })); // wrong arity
    static_assert(not can_and_then_lval(42));

    SECTION("value")
    {
      fn::expected<fn::pack<int, std::string_view>, Error> s{
          fn::pack<int>{12}.append(std::in_place_type<std::string_view>, "bar")};

      CHECK(s.and_then( //
                 fn::overload{[](int &i, auto &&...) -> fn::expected<bool, Error> { return i == 12; },
                              [](int const &, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                              [](int &&, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                              [](int const &&, auto &&...) -> fn::expected<bool, Error> { throw 0; }}) //
                .value());
      CHECK(std::as_const(s)
                .and_then( //
                    fn::overload{[](int &, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                                 [](int const &i, auto &&...) -> fn::expected<bool, Error> { return i == 12; },
                                 [](int &&, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                                 [](int const &&, auto &&...) -> fn::expected<bool, Error> { throw 0; }}) //
                .value());
      CHECK(std::move(std::as_const(s))
                .and_then( //
                    fn::overload{[](int &, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                                 [](int const &, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                                 [](int &&, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                                 [](int const &&i, auto &&...) -> fn::expected<bool, Error> { return i == 12; }}) //
                .value());
      CHECK(std::move(s)
                .and_then( //
                    fn::overload{[](int &, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                                 [](int const &, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                                 [](int &&i, auto &&...) -> fn::expected<bool, Error> { return i == 12; },
                                 [](int const &&, auto &&...) -> fn::expected<bool, Error> { throw 0; }}) //
                .value());
    }

    SECTION("error")
    {
      fn::expected<fn::pack<int, std::string_view>, Error> s{::fn::unexpect, FileNotFound};
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

  SECTION("transform")
  {
    using S = fn::expected<fn::pack<int, std::string_view>, Error>;

    // noexcept and constraints mirror and_then above (expected.hpp:216-220): the non-sum
    // _transform is constrained on pack-apply invocability AND the untouched error's copy --
    // contrast the sum case (see "expected sum support transform")
    constexpr auto nothrow_two = [](int &, std::string_view &) noexcept -> bool { return true; };
    static_assert(not noexcept(std::declval<S &>().transform(nothrow_two)));
    constexpr auto nothrow_generic = [](auto &&...) noexcept -> bool { return true; };
    static_assert(noexcept(std::declval<S &>().transform(nothrow_generic)));

    constexpr auto can_transform = [](auto &&f) { return requires { std::declval<S &>().transform(f); }; };
    static_assert(can_transform(nothrow_two));
    static_assert(not can_transform([](int &) -> bool { throw 0; })); // wrong arity
    static_assert(not can_transform(42));

    SECTION("value")
    {
      fn::expected<fn::pack<int, std::string_view>, Error> s{
          fn::pack<int>{12}.append(std::in_place_type<std::string_view>, "bar")};

      CHECK(s.transform( //
                 fn::overload{[](int &i, auto &&...) -> bool { return i == 12; },
                              [](int const &, auto &&...) -> bool { throw 0; },
                              [](int &&, auto &&...) -> bool { throw 0; },
                              [](int const &&, auto &&...) -> bool { throw 0; }}) //
                .value());
      CHECK(std::as_const(s)
                .transform( //
                    fn::overload{[](int &, auto &&...) -> bool { throw 0; },
                                 [](int const &i, auto &&...) -> bool { return i == 12; },
                                 [](int &&, auto &&...) -> bool { throw 0; },
                                 [](int const &&, auto &&...) -> bool { throw 0; }}) //
                .value());
      CHECK(std::move(std::as_const(s))
                .transform( //
                    fn::overload{[](int &, auto &&...) -> bool { throw 0; },
                                 [](int const &, auto &&...) -> bool { throw 0; },
                                 [](int &&, auto &&...) -> bool { throw 0; },
                                 [](int const &&i, auto &&...) -> bool { return i == 12; }}) //
                .value());
      CHECK(std::move(s)
                .transform( //
                    fn::overload{[](int &, auto &&...) -> bool { throw 0; },
                                 [](int const &, auto &&...) -> bool { throw 0; },
                                 [](int &&i, auto &&...) -> bool { return i == 12; },
                                 [](int const &&, auto &&...) -> bool { throw 0; }}) //
                .value());
    }

    SECTION("void result")
    {
      fn::expected<fn::pack<int, std::string_view>, Error> s{
          fn::pack<int>{12}.append(std::in_place_type<std::string_view>, "bar")};

      CHECK(s.transform( //
                 fn::overload{[](int &, auto &&...) -> void {}, [](int const &, auto &&...) -> void { throw 0; },
                              [](int &&, auto &&...) -> void { throw 0; },
                              [](int const &&, auto &&...) -> void { throw 0; }}) //
                .has_value());
      CHECK(std::as_const(s)
                .transform( //
                    fn::overload{[](int &, auto &&...) -> void { throw 0; }, [](int const &, auto &&...) -> void {},
                                 [](int &&, auto &&...) -> void { throw 0; },
                                 [](int const &&, auto &&...) -> void { throw 0; }}) //
                .has_value());
      CHECK(std::move(std::as_const(s))
                .transform( //
                    fn::overload{
                        [](int &, auto &&...) -> void { throw 0; }, [](int const &, auto &&...) -> void { throw 0; },
                        [](int &&, auto &&...) -> void { throw 0; }, [](int const &&, auto &&...) -> void {}}) //
                .has_value());
      CHECK(std::move(s)
                .transform( //
                    fn::overload{[](int &, auto &&...) -> void { throw 0; },
                                 [](int const &, auto &&...) -> void { throw 0; }, [](int &&, auto &&...) -> void {},
                                 [](int const &&, auto &&...) -> void { throw 0; }}) //
                .has_value());
    }

    SECTION("error")
    {
      fn::expected<fn::pack<int, std::string_view>, Error> s{::fn::unexpect, FileNotFound};
      CHECK(s.transform([](auto...) -> bool { throw 0; }).error() == FileNotFound);
      CHECK(std::as_const(s).transform([](auto...) -> bool { throw 0; }).error() == FileNotFound);
      CHECK(fn::expected<fn::pack<int, std::string_view>, Error>{::fn::unexpect, FileNotFound}
                .transform([](auto...) -> bool { throw 0; })
                .error()
            == FileNotFound);
      CHECK(std::move(std::as_const(s)).transform([](auto...) -> bool { throw 0; }).error() == FileNotFound);
    }
  }

  SECTION("operator &")
  {
    // noexcept: all eight operator& overloads are declared unconditionally noexcept
    // (expected.hpp:1032-1183) though joining copies/moves the operands' values into the
    // result -- for a throwing-copy value type this is a promise the join cannot keep. GAP:
    // asserts current behaviour; flip to `not noexcept` when fixed (issue #279).
    struct throwing_copy {
      // defined, not just declared: the instantiated join references it (-Wundefined-internal)
      throwing_copy(throwing_copy const &) noexcept(false) {}
    };
    static_assert(noexcept(std::declval<fn::expected<int, Error> &>() & std::declval<fn::expected<void, Error> &>()));
    static_assert(
        noexcept(std::declval<fn::expected<throwing_copy, Error> &>() & std::declval<fn::expected<int, Error> &>()));

    // constraints: both operands are expected, and their error types must match or be graded
    constexpr auto can_amp = [](auto &&rh) { return requires { std::declval<fn::expected<int, Error> &>() & rh; }; };
    static_assert(can_amp(fn::expected<void, Error>{}));
    static_assert(not can_amp(42));
    enum class other_error {};
    static_assert(not can_amp(fn::expected<int, other_error>{1})); // mismatched non-graded error

    SECTION("same error type")
    {
      SECTION("value & void yield value")
      {
        static_assert(
            std::same_as<decltype(std::declval<fn::expected<int, Error>>() & std::declval<fn::expected<void, Error>>()),
                         fn::expected<int, Error>>);

        CHECK((fn::expected<int, Error>{42} //
               & fn::expected<void, Error>{})
                  .value()
              == 42);
        CHECK((fn::expected<int, Error>{::fn::unexpect, FileNotFound} //
               & fn::expected<void, Error>{})
                  .error()
              == FileNotFound);
        CHECK((fn::expected<int, Error>{42} //
               & fn::expected<void, Error>{::fn::unexpect, Unknown})
                  .error()
              == Unknown);
        CHECK((fn::expected<int, Error>{::fn::unexpect, FileNotFound} //
               & fn::expected<void, Error>{::fn::unexpect, Unknown})
                  .error()
              == FileNotFound);
      }

      SECTION("void & value yield value")
      {
        static_assert(
            std::same_as<decltype(std::declval<fn::expected<void, Error>>() & std::declval<fn::expected<int, Error>>()),
                         fn::expected<int, Error>>);

        CHECK((fn::expected<void, Error>{} //
               & fn::expected<int, Error>{12})
                  .value()
              == 12);
        CHECK((fn::expected<void, Error>{::fn::unexpect, FileNotFound} //
               & fn::expected<int, Error>{12})
                  .error()
              == FileNotFound);
        CHECK((fn::expected<void, Error>{} //
               & fn::expected<int, Error>{::fn::unexpect, Unknown})
                  .error()
              == Unknown);
        CHECK((fn::expected<void, Error>{::fn::unexpect, FileNotFound} //
               & fn::expected<int, Error>{::fn::unexpect, Unknown})
                  .error()
              == FileNotFound);
      }

      SECTION("void & void yield void")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<void, Error>>()
                                            & std::declval<fn::expected<void, Error>>()),
                                   fn::expected<void, Error>>);

        CHECK((fn::expected<void, Error>{} //
               & fn::expected<void, Error>{})
                  .has_value());
        CHECK((fn::expected<void, Error>{::fn::unexpect, FileNotFound} //
               & fn::expected<void, Error>{})
                  .error()
              == FileNotFound);
        CHECK((fn::expected<void, Error>{} //
               & fn::expected<void, Error>{::fn::unexpect, Unknown})
                  .error()
              == Unknown);
        CHECK((fn::expected<void, Error>{::fn::unexpect, FileNotFound} //
               & fn::expected<void, Error>{::fn::unexpect, Unknown})
                  .error()
              == FileNotFound);
      }

      SECTION("value & value yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<int, Error>>()
                                            & std::declval<fn::expected<double, Error>>()),
                                   fn::expected<fn::pack<int, double>, Error>>);

        CHECK((fn::expected<double, Error>{0.5} //
               & fn::expected<int, Error>{12})
                  .transform([](double d, int i) constexpr -> bool { return d == 0.5 && i == 12; })
                  .value());
        CHECK((fn::expected<double, Error>{::fn::unexpect, FileNotFound} //
               & fn::expected<int, Error>{12})
                  .error()
              == FileNotFound);
        CHECK((fn::expected<double, Error>{} //
               & fn::expected<int, Error>{::fn::unexpect, Unknown})
                  .error()
              == Unknown);
        CHECK((fn::expected<double, Error>{::fn::unexpect, FileNotFound} //
               & fn::expected<int, Error>{::fn::unexpect, Unknown})
                  .error()
              == FileNotFound);
      }

      SECTION("value & pack yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<int, Error>>()
                                            & std::declval<fn::expected<fn::pack<bool, int>, Error>>()),
                                   fn::expected<fn::pack<int, bool, int>, Error>>);

        CHECK((fn::expected<double, Error>{0.5} //
               & fn::expected<fn::pack<bool, int>, Error>{std::in_place, fn::pack{true, 12}})
                  .transform([](double d, bool b, int i) constexpr -> bool { return d == 0.5 && b && i == 12; })
                  .value());
        CHECK((fn::expected<double, Error>{::fn::unexpect, FileNotFound} //
               & fn::expected<fn::pack<bool, int>, Error>{std::in_place, fn::pack{true, 12}})
                  .error()
              == FileNotFound);
        CHECK((fn::expected<double, Error>{} //
               & fn::expected<fn::pack<bool, int>, Error>{::fn::unexpect, Unknown})
                  .error()
              == Unknown);
        CHECK((fn::expected<double, Error>{::fn::unexpect, FileNotFound} //
               & fn::expected<fn::pack<bool, int>, Error>{::fn::unexpect, Unknown})
                  .error()
              == FileNotFound);
      }

      SECTION("pack & value yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<fn::pack<double, bool>, Error>>()
                                            & std::declval<fn::expected<int, Error>>()),
                                   fn::expected<fn::pack<double, bool, int>, Error>>);

        CHECK((fn::expected<fn::pack<double, bool>, Error>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<int, Error>{12})
                  .transform([](double d, bool b, int i) constexpr -> bool { return d == 0.5 && b && i == 12; })
                  .value());
        CHECK((fn::expected<fn::pack<double, bool>, Error>{::fn::unexpect, FileNotFound} //
               & fn::expected<int, Error>{12})
                  .error()
              == FileNotFound);
        CHECK((fn::expected<fn::pack<double, bool>, Error>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<int, Error>{::fn::unexpect, Unknown})
                  .error()
              == Unknown);
        CHECK((fn::expected<fn::pack<double, bool>, Error>{::fn::unexpect, FileNotFound} //
               & fn::expected<int, Error>{::fn::unexpect, Unknown})
                  .error()
              == FileNotFound);
      }

      SECTION("pack & pack yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<fn::pack<double, bool>, Error>>()
                                            & std::declval<fn::expected<fn::pack<bool, int>, Error>>()),
                                   fn::expected<fn::pack<double, bool, bool, int>, Error>>);

        CHECK((fn::expected<fn::pack<double, bool>, Error>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<fn::pack<bool, int>, Error>{std::in_place, fn::pack{true, 12}})
                  .transform([](double d, bool b1, bool b2, int i) constexpr -> bool {
                    return d == 0.5 && b1 && b2 && i == 12;
                  })
                  .value());
        CHECK((fn::expected<fn::pack<double, bool>, Error>{::fn::unexpect, FileNotFound} //
               & fn::expected<fn::pack<bool, int>, Error>{std::in_place, fn::pack{true, 12}})
                  .error()
              == FileNotFound);
        CHECK((fn::expected<fn::pack<double, bool>, Error>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<fn::pack<bool, int>, Error>{::fn::unexpect, Unknown})
                  .error()
              == Unknown);
        CHECK((fn::expected<fn::pack<double, bool>, Error>{::fn::unexpect, FileNotFound} //
               & fn::expected<fn::pack<bool, int>, Error>{::fn::unexpect, Unknown})
                  .error()
              == FileNotFound);
      }

      SECTION("pack & void yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<fn::pack<double, bool>, Error>>()
                                            & std::declval<fn::expected<void, Error>>()),
                                   fn::expected<fn::pack<double, bool>, Error>>);

        CHECK((fn::expected<fn::pack<double, bool>, Error>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<void, Error>{})
                  .transform([](double d, bool b) constexpr -> bool { return d == 0.5 && b; })
                  .value());
        CHECK((fn::expected<fn::pack<double, bool>, Error>{::fn::unexpect, FileNotFound} //
               & fn::expected<void, Error>{})
                  .error()
              == FileNotFound);
        CHECK((fn::expected<fn::pack<double, bool>, Error>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<void, Error>{::fn::unexpect, Unknown})
                  .error()
              == Unknown);
        CHECK((fn::expected<fn::pack<double, bool>, Error>{::fn::unexpect, FileNotFound} //
               & fn::expected<void, Error>{::fn::unexpect, Unknown})
                  .error()
              == FileNotFound);
      }

      SECTION("void & pack yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<void, Error>>()
                                            & std::declval<fn::expected<fn::pack<double, bool>, Error>>()),
                                   fn::expected<fn::pack<double, bool>, Error>>);

        CHECK((fn::expected<void, Error>{} //
               & fn::expected<fn::pack<double, bool>, Error>{std::in_place, fn::pack<double, bool>{0.5, true}})
                  .transform([](double d, bool b) constexpr -> bool { return d == 0.5 && b; })
                  .value());
        CHECK((fn::expected<void, Error>{::fn::unexpect, FileNotFound} //
               & fn::expected<fn::pack<double, bool>, Error>{std::in_place, fn::pack<double, bool>{0.5, true}})
                  .error()
              == FileNotFound);
        CHECK((fn::expected<void, Error>{} //
               & fn::expected<fn::pack<double, bool>, Error>{::fn::unexpect, Unknown})
                  .error()
              == Unknown);
        CHECK((fn::expected<void, Error>{::fn::unexpect, FileNotFound} //
               & fn::expected<fn::pack<double, bool>, Error>{::fn::unexpect, Unknown})
                  .error()
              == FileNotFound);
      }

      SECTION("sum on both sides")
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
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{fn::sum{12}}).error() == FileNotFound);
        CHECK((Lh{fn::sum{0.5}} & Rh{::fn::unexpect, Unknown}).error() == Unknown);
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, Unknown}).error() == FileNotFound);

        SECTION("sum of packs on left")
        {
          using Lh = fn::expected<fn::sum_for<fn::pack<double, bool>, fn::pack<double, int>>, Error>;
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
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{fn::sum{12}}).error() == FileNotFound);
          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{::fn::unexpect, Unknown}).error() == Unknown);
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, Unknown}).error() == FileNotFound);
        }

        SECTION("sum of packs on right")
        {
          using Rh = fn::expected<fn::sum_for<fn::pack<double, bool>, fn::pack<double, int>>, Error>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::sum< //
                                                      fn::pack<double, double, bool>, fn::pack<double, double, int>,
                                                      fn::pack<int, double, bool>, fn::pack<int, double, int>>,
                                                  Error>>);

          CHECK((Lh{fn::sum{12}} & Rh{fn::sum{fn::pack{0.5, 3}}})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 12 == static_cast<int>(i) && 0.5 == static_cast<double>(j) && 3 == static_cast<int>(k);
                    })
                    .value()
                == fn::sum{true});
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{fn::sum{fn::pack{0.5, 3}}}).error() == FileNotFound);
          CHECK((Lh{fn::sum{12}} & Rh{::fn::unexpect, Unknown}).error() == Unknown);
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, Unknown}).error() == FileNotFound);
        }
      }

      SECTION("sum on left side only")
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
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{12}).error() == FileNotFound);
        CHECK((Lh{fn::sum{0.5}} & Rh{::fn::unexpect, Unknown}).error() == Unknown);
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, Unknown}).error() == FileNotFound);

        SECTION("sum of packs on left")
        {
          using Lh = fn::expected<fn::sum_for<fn::pack<double, bool>, fn::pack<double, int>>, Error>;
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
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{12}).error() == FileNotFound);
          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{::fn::unexpect, Unknown}).error() == Unknown);
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, Unknown}).error() == FileNotFound);
        }

        SECTION("pack on right")
        {
          using Rh = fn::expected<fn::pack<double, bool>, Error>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::sum< //
                                                      fn::pack<double, double, bool>, fn::pack<int, double, bool>>,
                                                  Error>>);

          CHECK((Lh{fn::sum{1.5}} & Rh{std::in_place, fn::pack{0.5, true}})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 1.5 == static_cast<double>(i) && 0.5 == static_cast<double>(j) && static_cast<bool>(k);
                    })
                    .value()
                == fn::sum{true});
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{std::in_place, fn::pack{0.5, true}}).error() == FileNotFound);
          CHECK((Lh{fn::sum{1.5}} & Rh{::fn::unexpect, Unknown}).error() == Unknown);
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, Unknown}).error() == FileNotFound);
        }
      }

      SECTION("sum on right side only")
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
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{fn::sum{12}}).error() == FileNotFound);
        CHECK((Lh{0.5} & Rh{::fn::unexpect, Unknown}).error() == Unknown);
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, Unknown}).error() == FileNotFound);

        SECTION("pack on left")
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
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{fn::sum{12}}).error() == FileNotFound);
          CHECK((Lh{fn::pack{0.5, 3}} & Rh{::fn::unexpect, Unknown}).error() == Unknown);
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, Unknown}).error() == FileNotFound);
        }
      }
    }

    SECTION("graded monad as left operand")
    {
      static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::sum<Error>>>()
                                          & std::declval<fn::expected<void, Error>>()),
                                 fn::expected<int, fn::sum<Error>>>);

      static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::sum<Error>>>()
                                          & std::declval<fn::expected<void, fn::sum<Error>>>()),
                                 fn::expected<int, fn::sum<Error>>>);

      static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::sum<Error>>>()
                                          & std::declval<fn::expected<void, fn::sum<int>>>()),
                                 fn::expected<int, fn::sum_for<Error, int>>>);

      static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::sum<Error>>>()
                                          & std::declval<fn::expected<void, fn::sum<bool, int>>>()),
                                 fn::expected<int, fn::sum_for<Error, bool, int>>>);

      static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::sum<bool, int>>>()
                                          & std::declval<fn::expected<void, fn::sum<Error>>>()),
                                 fn::expected<int, fn::sum_for<Error, bool, int>>>);

      SECTION("value & void yield value")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::sum<Error>>>()
                                            & std::declval<fn::expected<void, int>>()),
                                   fn::expected<int, fn::sum_for<Error, int>>>);

        CHECK((fn::expected<int, fn::sum<Error>>{42} //
               & fn::expected<void, int>{})
                  .value()
              == 42);
        CHECK((fn::expected<int, fn::sum<Error>>{::fn::unexpect, fn::sum{FileNotFound}} //
               & fn::expected<void, int>{})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<int, fn::sum<Error>>{42} //
               & fn::expected<void, int>{::fn::unexpect, 13})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<int, fn::sum<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<void, int>{::fn::unexpect, 13})
                  .error()
              == fn::sum{FileNotFound});
      }

      SECTION("void & value yield value")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<void, fn::sum<Error>>>()
                                            & std::declval<fn::expected<int, int>>()),
                                   fn::expected<int, fn::sum_for<Error, int>>>);

        CHECK((fn::expected<void, fn::sum<Error>>{} //
               & fn::expected<int, int>{12})
                  .value()
              == 12);
        CHECK((fn::expected<void, fn::sum<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<int, int>{12})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<void, fn::sum<Error>>{} //
               & fn::expected<int, int>{::fn::unexpect, 13})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<void, fn::sum<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<int, int>{::fn::unexpect, 13})
                  .error()
              == fn::sum{FileNotFound});
      }

      SECTION("void & void yield void")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<void, fn::sum<Error>>>()
                                            & std::declval<fn::expected<void, int>>()),
                                   fn::expected<void, fn::sum_for<Error, int>>>);

        CHECK((fn::expected<void, fn::sum<Error>>{} //
               & fn::expected<void, int>{})
                  .has_value());
        CHECK((fn::expected<void, fn::sum<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<void, int>{})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<void, fn::sum<Error>>{} //
               & fn::expected<void, int>{::fn::unexpect, 13})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<void, fn::sum<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<void, int>{::fn::unexpect, 13})
                  .error()
              == fn::sum{FileNotFound});
      }

      SECTION("value & value yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::sum<Error>>>()
                                            & std::declval<fn::expected<double, int>>()),
                                   fn::expected<fn::pack<int, double>, fn::sum_for<Error, int>>>);

        CHECK((fn::expected<double, fn::sum<Error>>{0.5} //
               & fn::expected<int, int>{12})
                  .transform([](double d, int i) constexpr -> bool { return d == 0.5 && i == 12; })
                  .value());
        CHECK((fn::expected<double, fn::sum<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<int, int>{12})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<double, fn::sum<Error>>{} //
               & fn::expected<int, int>{::fn::unexpect, 13})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<double, fn::sum<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<int, int>{::fn::unexpect, 13})
                  .error()
              == fn::sum{FileNotFound});
      }

      SECTION("pack & value yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<fn::pack<double, bool>, fn::sum<Error>>>()
                                            & std::declval<fn::expected<int, int>>()),
                                   fn::expected<fn::pack<double, bool, int>, fn::sum_for<Error, int>>>);

        CHECK((fn::expected<fn::pack<double, bool>, fn::sum<Error>>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<int, int>{12})
                  .transform([](double d, bool b, int i) constexpr -> bool { return d == 0.5 && b && i == 12; })
                  .value());
        CHECK((fn::expected<fn::pack<double, bool>, fn::sum<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<int, int>{12})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<fn::pack<double, bool>, fn::sum<Error>>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<int, int>{::fn::unexpect, 13})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<fn::pack<double, bool>, fn::sum<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<int, int>{::fn::unexpect, 13})
                  .error()
              == fn::sum{FileNotFound});
      }

      SECTION("pack & void yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<fn::pack<double, bool>, fn::sum<Error>>>()
                                            & std::declval<fn::expected<void, int>>()),
                                   fn::expected<fn::pack<double, bool>, fn::sum_for<Error, int>>>);

        CHECK((fn::expected<fn::pack<double, bool>, fn::sum<Error>>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<void, int>{})
                  .transform([](double d, bool b) constexpr -> bool { return d == 0.5 && b; })
                  .value());
        CHECK((fn::expected<fn::pack<double, bool>, fn::sum<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<void, int>{})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<fn::pack<double, bool>, fn::sum<Error>>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<void, int>{::fn::unexpect, 13})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<fn::pack<double, bool>, fn::sum<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<void, int>{::fn::unexpect, 13})
                  .error()
              == fn::sum{FileNotFound});
      }

      SECTION("void & pack yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<void, fn::sum<Error>>>()
                                            & std::declval<fn::expected<fn::pack<double, bool>, int>>()),
                                   fn::expected<fn::pack<double, bool>, fn::sum_for<Error, int>>>);

        CHECK((fn::expected<void, fn::sum<Error>>{} //
               & fn::expected<fn::pack<double, bool>, int>{std::in_place, fn::pack<double, bool>{0.5, true}})
                  .transform([](double d, bool b) constexpr -> bool { return d == 0.5 && b; })
                  .value());
        CHECK((fn::expected<void, fn::sum<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<fn::pack<double, bool>, int>{std::in_place, fn::pack<double, bool>{0.5, true}})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<void, fn::sum<Error>>{} //
               & fn::expected<fn::pack<double, bool>, int>{::fn::unexpect, 13})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<void, fn::sum<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<fn::pack<double, bool>, int>{::fn::unexpect, 13})
                  .error()
              == fn::sum{FileNotFound});
      }

      SECTION("sum on both sides")
      {
        using Lh = fn::expected<fn::sum<double, int>, fn::sum<Error>>;
        using Rh = fn::expected<fn::sum<bool, int>, int>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::sum< //
                                                    fn::pack<double, bool>, fn::pack<double, int>, fn::pack<int, bool>,
                                                    fn::pack<int, int>>,
                                                fn::sum_for<Error, int>>>);

        CHECK((Lh{fn::sum{0.5}} & Rh{fn::sum{12}})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::sum{true});
        CHECK((Lh{::fn::unexpect, fn::sum{FileNotFound}} & Rh{fn::sum{12}}).error() == fn::sum{FileNotFound});
        CHECK((Lh{fn::sum{0.5}} & Rh{::fn::unexpect, 13}).error() == fn::sum{13});
        CHECK((Lh{::fn::unexpect, fn::sum{FileNotFound}} & Rh{::fn::unexpect, 13}).error() == fn::sum{FileNotFound});

        SECTION("sum of packs on left")
        {
          using Lh = fn::expected<fn::sum_for<fn::pack<double, bool>, fn::pack<double, int>>, fn::sum<Error>>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::sum< //
                                                      fn::pack<double, bool, bool>, fn::pack<double, bool, int>,
                                                      fn::pack<double, int, bool>, fn::pack<double, int, int>>,
                                                  fn::sum_for<Error, int>>>);

          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{fn::sum{12}})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                    })
                    .value()
                == fn::sum{true});
          CHECK((Lh{::fn::unexpect, fn::sum{FileNotFound}} & Rh{fn::sum{12}}).error() == fn::sum{FileNotFound});
          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{::fn::unexpect, 13}).error() == fn::sum{13});
          CHECK((Lh{::fn::unexpect, fn::sum{FileNotFound}} & Rh{::fn::unexpect, 13}).error() == fn::sum{FileNotFound});
        }
      }

      SECTION("sum on left side only")
      {
        using Lh = fn::expected<fn::sum<double, int>, fn::sum<Error>>;
        using Rh = fn::expected<int, int>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::sum< //
                                                    fn::pack<double, int>, fn::pack<int, int>>,
                                                fn::sum_for<Error, int>>>);

        CHECK((Lh{fn::sum{0.5}} & Rh{12})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::sum{true});
        CHECK((Lh{::fn::unexpect, fn::sum{FileNotFound}} & Rh{12}).error() == fn::sum{FileNotFound});
        CHECK((Lh{fn::sum{0.5}} & Rh{::fn::unexpect, 13}).error() == fn::sum{13});
        CHECK((Lh{::fn::unexpect, fn::sum{FileNotFound}} & Rh{::fn::unexpect, 13}).error() == fn::sum{FileNotFound});

        SECTION("sum of packs on left")
        {
          using Lh = fn::expected<fn::sum_for<fn::pack<double, bool>, fn::pack<double, int>>, fn::sum<Error>>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::sum< //
                                                      fn::pack<double, bool, int>, fn::pack<double, int, int>>,
                                                  fn::sum_for<Error, int>>>);

          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{12})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                    })
                    .value()
                == fn::sum{true});
          CHECK((Lh{::fn::unexpect, fn::sum{FileNotFound}} & Rh{12}).error() == fn::sum{FileNotFound});
          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{::fn::unexpect, 13}).error() == fn::sum{13});
          CHECK((Lh{::fn::unexpect, fn::sum{FileNotFound}} & Rh{::fn::unexpect, 13}).error() == fn::sum{FileNotFound});
        }
      }

      SECTION("sum on right side only")
      {
        using Lh = fn::expected<double, fn::sum<Error>>;
        using Rh = fn::expected<fn::sum<bool, int>, int>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::sum< //
                                                    fn::pack<double, bool>, fn::pack<double, int>>,
                                                fn::sum_for<Error, int>>>);

        CHECK((Lh{0.5} & Rh{fn::sum{12}})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::sum{true});
        CHECK((Lh{::fn::unexpect, fn::sum{FileNotFound}} & Rh{fn::sum{12}}).error() == fn::sum{FileNotFound});
        CHECK((Lh{0.5} & Rh{::fn::unexpect, 13}).error() == fn::sum{13});
        CHECK((Lh{::fn::unexpect, fn::sum{FileNotFound}} & Rh{::fn::unexpect, 13}).error() == fn::sum{FileNotFound});

        SECTION("pack on left")
        {
          using Lh = fn::expected<fn::pack<double, int>, fn::sum<Error>>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::sum< //
                                                      fn::pack<double, int, bool>, fn::pack<double, int, int>>,
                                                  fn::sum_for<Error, int>>>);

          CHECK((Lh{fn::pack{0.5, 3}} & Rh{fn::sum{12}})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                    })
                    .value()
                == fn::sum{true});
          CHECK((Lh{::fn::unexpect, fn::sum{FileNotFound}} & Rh{fn::sum{12}}).error() == fn::sum{FileNotFound});
          CHECK((Lh{fn::pack{0.5, 3}} & Rh{::fn::unexpect, 13}).error() == fn::sum{13});
          CHECK((Lh{::fn::unexpect, fn::sum{FileNotFound}} & Rh{::fn::unexpect, 13}).error() == fn::sum{FileNotFound});
        }
      }
    }

    SECTION("graded monad as right operand")
    {
      static_assert(std::same_as<decltype(std::declval<fn::expected<void, Error>>()
                                          & std::declval<fn::expected<int, fn::sum<Error>>>()),
                                 fn::expected<int, fn::sum<Error>>>);

      static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::sum<Error>>>()
                                          & std::declval<fn::expected<void, fn::sum<Error>>>()),
                                 fn::expected<int, fn::sum<Error>>>);

      static_assert(std::same_as<decltype(std::declval<fn::expected<void, fn::sum<int>>>()
                                          & std::declval<fn::expected<int, fn::sum<Error>>>()),
                                 fn::expected<int, fn::sum_for<Error, int>>>);

      static_assert(std::same_as<decltype(std::declval<fn::expected<void, fn::sum<bool, int>>>()
                                          & std::declval<fn::expected<int, fn::sum<Error>>>()),
                                 fn::expected<int, fn::sum_for<Error, bool, int>>>);

      static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::sum<bool, int>>>()
                                          & std::declval<fn::expected<void, fn::sum<Error>>>()),
                                 fn::expected<int, fn::sum_for<Error, bool, int>>>);

      SECTION("value & void & yield value")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<int, int>>()
                                            & std::declval<fn::expected<void, fn::sum<Error>>>()),
                                   fn::expected<int, fn::sum_for<Error, int>>>);

        CHECK((fn::expected<int, int>{12} //
               & fn::expected<void, fn::sum<Error>>{})
                  .value()
              == 12);
        CHECK((fn::expected<int, int>{12} //
               & fn::expected<void, fn::sum<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<int, int>{::fn::unexpect, 13} //
               & fn::expected<void, fn::sum<Error>>{})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<int, int>{::fn::unexpect, 13} //
               & fn::expected<void, fn::sum<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::sum{13});
      }

      SECTION("void & value yield value")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<void, int>>()
                                            & std::declval<fn::expected<int, fn::sum<Error>>>()),
                                   fn::expected<int, fn::sum_for<Error, int>>>);

        CHECK((fn::expected<void, int>{} //
               & fn::expected<int, fn::sum<Error>>{42})
                  .value()
              == 42);
        CHECK((fn::expected<void, int>{} //
               & fn::expected<int, fn::sum<Error>>{::fn::unexpect, fn::sum{FileNotFound}})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<void, int>{::fn::unexpect, 13} //
               & fn::expected<int, fn::sum<Error>>{42})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<void, int>{::fn::unexpect, 13} //
               & fn::expected<int, fn::sum<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::sum{13});
      }

      SECTION("void & void yield void")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<void, int>>()
                                            & std::declval<fn::expected<void, fn::sum<Error>>>()),
                                   fn::expected<void, fn::sum_for<Error, int>>>);

        CHECK((fn::expected<void, int>{} //
               & fn::expected<void, fn::sum<Error>>{})
                  .has_value());
        CHECK((fn::expected<void, int>{} //
               & fn::expected<void, fn::sum<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<void, int>{::fn::unexpect, 13} //
               & fn::expected<void, fn::sum<Error>>{})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<void, int>{::fn::unexpect, 13} //
               & fn::expected<void, fn::sum<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::sum{13});
      }

      SECTION("value & value yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<double, int>>()
                                            & std::declval<fn::expected<int, fn::sum<Error>>>()),
                                   fn::expected<fn::pack<double, int>, fn::sum_for<Error, int>>>);

        CHECK((fn::expected<double, int>{0.5} //
               & fn::expected<int, fn::sum<Error>>{12})
                  .transform([](double d, int i) constexpr -> bool { return d == 0.5 && i == 12; })
                  .value());
        CHECK((fn::expected<double, int>{0.5} //
               & fn::expected<int, fn::sum<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<double, int>{::fn::unexpect, 13} //
               & fn::expected<int, fn::sum<Error>>{12})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<double, int>{::fn::unexpect, 13} //
               & fn::expected<int, fn::sum<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::sum{13});
      }

      SECTION("pack & value yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<fn::pack<double, bool>, int>>()
                                            & std::declval<fn::expected<int, fn::sum<Error>>>()),
                                   fn::expected<fn::pack<double, bool, int>, fn::sum_for<Error, int>>>);

        CHECK((fn::expected<fn::pack<double, bool>, int>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<int, fn::sum<Error>>{12})
                  .transform([](double d, bool b, int i) constexpr -> bool { return d == 0.5 && b && i == 12; })
                  .value());
        CHECK((fn::expected<fn::pack<double, bool>, int>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<int, fn::sum<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<fn::pack<double, bool>, int>{::fn::unexpect, 13} //
               & fn::expected<int, fn::sum<Error>>{12})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<fn::pack<double, bool>, int>{::fn::unexpect, 13} //
               & fn::expected<int, fn::sum<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::sum{13});
      }

      SECTION("pack & void yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<fn::pack<double, bool>, int>>()
                                            & std::declval<fn::expected<void, fn::sum<Error>>>()),
                                   fn::expected<fn::pack<double, bool>, fn::sum_for<Error, int>>>);

        CHECK((fn::expected<fn::pack<double, bool>, int>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<void, fn::sum<Error>>{})
                  .transform([](double d, bool b) constexpr -> bool { return d == 0.5 && b; })
                  .value());
        CHECK((fn::expected<fn::pack<double, bool>, int>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<void, fn::sum<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<fn::pack<double, bool>, int>{::fn::unexpect, 13} //
               & fn::expected<void, fn::sum<Error>>{})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<fn::pack<double, bool>, int>{::fn::unexpect, 13} //
               & fn::expected<void, fn::sum<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::sum{13});
      }

      SECTION("void & pack yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<void, int>>()
                                            & std::declval<fn::expected<fn::pack<double, bool>, fn::sum<Error>>>()),
                                   fn::expected<fn::pack<double, bool>, fn::sum_for<Error, int>>>);

        CHECK((fn::expected<void, int>{} //
               & fn::expected<fn::pack<double, bool>, fn::sum<Error>>{std::in_place, fn::pack<double, bool>{0.5, true}})
                  .transform([](double d, bool b) constexpr -> bool { return d == 0.5 && b; })
                  .value());
        CHECK((fn::expected<void, int>{} //
               & fn::expected<fn::pack<double, bool>, fn::sum<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((fn::expected<void, int>{::fn::unexpect, 13} //
               & fn::expected<fn::pack<double, bool>, fn::sum<Error>>{std::in_place, fn::pack<double, bool>{0.5, true}})
                  .error()
              == fn::sum{13});
        CHECK((fn::expected<void, int>{::fn::unexpect, 13} //
               & fn::expected<fn::pack<double, bool>, fn::sum<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::sum{13});
      }

      SECTION("sum on both sides")
      {
        using Lh = fn::expected<fn::sum<double, int>, Error>;
        using Rh = fn::expected<fn::sum<bool, int>, fn::sum<int>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::sum< //
                                                    fn::pack<double, bool>, fn::pack<double, int>, fn::pack<int, bool>,
                                                    fn::pack<int, int>>,
                                                fn::sum_for<Error, int>>>);

        CHECK((Lh{fn::sum{0.5}} & Rh{fn::sum{12}})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::sum{true});
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{fn::sum{12}}).error() == fn::sum{FileNotFound});
        CHECK((Lh{fn::sum{0.5}} & Rh{::fn::unexpect, fn::sum{13}}).error() == fn::sum{13});
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, fn::sum{13}}).error() == fn::sum{FileNotFound});

        SECTION("sum of packs on left")
        {
          using Lh = fn::expected<fn::sum_for<fn::pack<double, bool>, fn::pack<double, int>>, Error>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::sum< //
                                                      fn::pack<double, bool, bool>, fn::pack<double, bool, int>,
                                                      fn::pack<double, int, bool>, fn::pack<double, int, int>>,
                                                  fn::sum_for<Error, int>>>);

          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{fn::sum{12}})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                    })
                    .value()
                == fn::sum{true});
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{fn::sum{12}}).error() == fn::sum{FileNotFound});
          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{::fn::unexpect, fn::sum{13}}).error() == fn::sum{13});
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, fn::sum{13}}).error() == fn::sum{FileNotFound});
        }
      }

      SECTION("sum on left side only")
      {
        using Lh = fn::expected<fn::sum<double, int>, Error>;
        using Rh = fn::expected<int, fn::sum<int>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::sum< //
                                                    fn::pack<double, int>, fn::pack<int, int>>,
                                                fn::sum_for<Error, int>>>);

        CHECK((Lh{fn::sum{0.5}} & Rh{12})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::sum{true});
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{12}).error() == fn::sum{FileNotFound});
        CHECK((Lh{fn::sum{0.5}} & Rh{::fn::unexpect, 13}).error() == fn::sum{13});
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, 13}).error() == fn::sum{FileNotFound});

        SECTION("sum of packs on left")
        {
          using Lh = fn::expected<fn::sum_for<fn::pack<double, bool>, fn::pack<double, int>>, Error>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::sum< //
                                                      fn::pack<double, bool, int>, fn::pack<double, int, int>>,
                                                  fn::sum_for<Error, int>>>);

          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{12})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                    })
                    .value()
                == fn::sum{true});
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{12}).error() == fn::sum{FileNotFound});
          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{::fn::unexpect, fn::sum{13}}).error() == fn::sum{13});
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, fn::sum{13}}).error() == fn::sum{FileNotFound});
        }
      }

      SECTION("sum on right side only")
      {
        using Lh = fn::expected<double, Error>;
        using Rh = fn::expected<fn::sum<bool, int>, fn::sum<int>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::sum< //
                                                    fn::pack<double, bool>, fn::pack<double, int>>,
                                                fn::sum_for<Error, int>>>);

        CHECK((Lh{0.5} & Rh{fn::sum{12}})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::sum{true});
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{fn::sum{12}}).error() == fn::sum{FileNotFound});
        CHECK((Lh{0.5} & Rh{::fn::unexpect, fn::sum{13}}).error() == fn::sum{13});
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, fn::sum{13}}).error() == fn::sum{FileNotFound});

        SECTION("pack on left")
        {
          using Lh = fn::expected<fn::pack<double, int>, Error>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::sum< //
                                                      fn::pack<double, int, bool>, fn::pack<double, int, int>>,
                                                  fn::sum_for<Error, int>>>);

          CHECK((Lh{fn::pack{0.5, 3}} & Rh{fn::sum{12}})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                    })
                    .value()
                == fn::sum{true});
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{fn::sum{12}}).error() == fn::sum{FileNotFound});
          CHECK((Lh{fn::pack{0.5, 3}} & Rh{::fn::unexpect, fn::sum{13}}).error() == fn::sum{13});
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, fn::sum{13}}).error() == fn::sum{FileNotFound});
        }
      }
    }

    SECTION("graded monad on both sides")
    {
      static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::sum<bool, int>>>()
                                          & std::declval<fn::expected<void, fn::sum<Error>>>()),
                                 fn::expected<int, fn::sum_for<Error, bool, int>>>);

      SECTION("value & void & yield value")
      {
        using Lh = fn::expected<int, fn::sum<bool, int>>;
        using Rh = fn::expected<void, fn::sum<Error>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<int, fn::sum_for<Error, bool, int>>>);

        CHECK((Lh{12} & Rh{}).value() == 12);
        CHECK((Lh{12} & Rh{::fn::unexpect, fn::sum{FileNotFound}}).error() == fn::sum{FileNotFound});
        CHECK((Lh{::fn::unexpect, fn::sum{13}} & Rh{}).error() == fn::sum{13});
        CHECK((Lh{::fn::unexpect, fn::sum{13}} & Rh{::fn::unexpect, fn::sum{FileNotFound}}).error() == fn::sum{13});
      }

      SECTION("void & value yield value")
      {
        using Lh = fn::expected<void, fn::sum<bool, int>>;
        using Rh = fn::expected<int, fn::sum<Error>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<int, fn::sum_for<Error, bool, int>>>);

        CHECK((Lh{} & Rh{42}).value() == 42);
        CHECK((Lh{} & Rh{::fn::unexpect, fn::sum{FileNotFound}}).error() == fn::sum{FileNotFound});
        CHECK((Lh{::fn::unexpect, fn::sum{13}} & Rh{42}).error() == fn::sum{13});
        CHECK((Lh{::fn::unexpect, fn::sum{13}} & Rh{::fn::unexpect, fn::sum{FileNotFound}}).error() == fn::sum{13});
      }

      SECTION("void & void yield void")
      {
        using Lh = fn::expected<void, fn::sum<bool, int>>;
        using Rh = fn::expected<void, fn::sum<Error>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<void, fn::sum_for<Error, bool, int>>>);

        CHECK((Lh{} & Rh{}).has_value());
        CHECK((Lh{} & Rh{::fn::unexpect, fn::sum{FileNotFound}}).error() == fn::sum{FileNotFound});
        CHECK((Lh{::fn::unexpect, fn::sum{13}} & Rh{}).error() == fn::sum{13});
        CHECK((Lh{::fn::unexpect, fn::sum{13}} & Rh{::fn::unexpect, fn::sum{FileNotFound}}).error() == fn::sum{13});
      }

      SECTION("value & value yield pack")
      {
        using Lh = fn::expected<double, fn::sum<bool, int>>;
        using Rh = fn::expected<int, fn::sum<Error>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::pack<double, int>, fn::sum_for<Error, bool, int>>>);

        CHECK((Lh{0.5} & Rh{12})
                  .transform([](double d, int i) constexpr -> bool { return d == 0.5 && i == 12; })
                  .value());
        CHECK((Lh{0.5} & Rh{::fn::unexpect, fn::sum{FileNotFound}}).error() == fn::sum{FileNotFound});
        CHECK((Lh{::fn::unexpect, fn::sum{13}} & Rh{12}).error() == fn::sum{13});
        CHECK((Lh{::fn::unexpect, fn::sum{13}} & Rh{::fn::unexpect, fn::sum{FileNotFound}}).error() == fn::sum{13});
      }

      SECTION("pack & value yield pack")
      {
        using Lh = fn::expected<fn::pack<double, bool>, fn::sum<bool, int>>;
        using Rh = fn::expected<int, fn::sum<Error>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::pack<double, bool, int>, fn::sum_for<Error, bool, int>>>);

        CHECK((Lh{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & Rh{12})
                  .transform([](double d, bool b, int i) constexpr -> bool { return d == 0.5 && b && i == 12; })
                  .value());
        CHECK((Lh{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & Rh{::fn::unexpect, fn::sum{FileNotFound}})
                  .error()
              == fn::sum{FileNotFound});
        CHECK((Lh{::fn::unexpect, fn::sum{13}} //
               & Rh{12})
                  .error()
              == fn::sum{13});
        CHECK((Lh{::fn::unexpect, fn::sum{13}} //
               & Rh{::fn::unexpect, fn::sum{FileNotFound}})
                  .error()
              == fn::sum{13});
      }

      SECTION("pack & void yield pack")
      {
        using Lh = fn::expected<fn::pack<double, bool>, fn::sum<bool, int>>;
        using Rh = fn::expected<void, fn::sum<Error>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::pack<double, bool>, fn::sum_for<Error, bool, int>>>);

        CHECK((Lh{std::in_place, fn::pack<double, bool>{0.5, true}} & Rh{})
                  .transform([](double d, bool b) constexpr -> bool { return d == 0.5 && b; })
                  .value());
        CHECK((Lh{std::in_place, fn::pack<double, bool>{0.5, true}} & Rh{::fn::unexpect, fn::sum{FileNotFound}}).error()
              == fn::sum{FileNotFound});
        CHECK((Lh{::fn::unexpect, fn::sum{13}} & Rh{}).error() == fn::sum{13});
        CHECK((Lh{::fn::unexpect, fn::sum{13}} & Rh{::fn::unexpect, fn::sum{FileNotFound}}).error() == fn::sum{13});
      }

      SECTION("void & pack yield pack")
      {
        using Lh = fn::expected<void, fn::sum<bool, int>>;
        using Rh = fn::expected<fn::pack<double, bool>, fn::sum<Error>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::pack<double, bool>, fn::sum_for<Error, bool, int>>>);

        CHECK((Lh{} & Rh{std::in_place, fn::pack<double, bool>{0.5, true}})
                  .transform([](double d, bool b) constexpr -> bool { return d == 0.5 && b; })
                  .value());
        CHECK((Lh{} & Rh{::fn::unexpect, fn::sum{FileNotFound}}).error() == fn::sum{FileNotFound});
        CHECK((Lh{::fn::unexpect, fn::sum{13}} & Rh{std::in_place, fn::pack<double, bool>{0.5, true}}).error()
              == fn::sum{13});
        CHECK((Lh{::fn::unexpect, fn::sum{13}} & Rh{::fn::unexpect, fn::sum{FileNotFound}}).error() == fn::sum{13});
      }

      SECTION("sum on both sides")
      {
        using Lh = fn::expected<fn::sum<double, int>, fn::sum<Error>>;
        using Rh = fn::expected<fn::sum<bool, int>, fn::sum<bool, int>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::sum< //
                                                    fn::pack<double, bool>, fn::pack<double, int>, fn::pack<int, bool>,
                                                    fn::pack<int, int>>,
                                                fn::sum_for<Error, bool, int>>>);

        CHECK((Lh{fn::sum{0.5}} & Rh{fn::sum{12}})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::sum{true});
        CHECK((Lh{::fn::unexpect, fn::sum{FileNotFound}} & Rh{fn::sum{12}}).error() == fn::sum{FileNotFound});
        CHECK((Lh{fn::sum{0.5}} & Rh{::fn::unexpect, fn::sum{13}}).error() == fn::sum{13});
        CHECK((Lh{::fn::unexpect, fn::sum{FileNotFound}} & Rh{::fn::unexpect, fn::sum{13}}).error()
              == fn::sum{FileNotFound});

        SECTION("sum of packs on left")
        {
          using Lh = fn::expected<fn::sum_for<fn::pack<double, bool>, fn::pack<double, int>>, fn::sum<Error>>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::sum< //
                                                      fn::pack<double, bool, bool>, fn::pack<double, bool, int>,
                                                      fn::pack<double, int, bool>, fn::pack<double, int, int>>,
                                                  fn::sum_for<Error, bool, int>>>);

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
          CHECK((Lh{::fn::unexpect, fn::sum{FileNotFound}} & Rh{fn::sum{12}}).error() == fn::sum{FileNotFound});
          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{::fn::unexpect, fn::sum{13}}).error() == fn::sum{13});
          CHECK((Lh{::fn::unexpect, fn::sum{FileNotFound}} & Rh{::fn::unexpect, fn::sum{13}}).error()
                == fn::sum{FileNotFound});
        }
      }

      SECTION("sum on left side only")
      {
        using Lh = fn::expected<fn::sum<double, int>, fn::sum<Error>>;
        using Rh = fn::expected<int, fn::sum<bool, int>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::sum< //
                                                    fn::pack<double, int>, fn::pack<int, int>>,
                                                fn::sum_for<Error, bool, int>>>);

        CHECK((Lh{fn::sum{0.5}} & Rh{12})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::sum{true});
        CHECK((Lh{::fn::unexpect, fn::sum{FileNotFound}} & Rh{12}).error() == fn::sum{FileNotFound});
        CHECK((Lh{fn::sum{0.5}} & Rh{::fn::unexpect, 13}).error() == fn::sum{13});
        CHECK((Lh{::fn::unexpect, fn::sum{FileNotFound}} & Rh{::fn::unexpect, 13}).error() == fn::sum{FileNotFound});

        SECTION("sum of packs on left")
        {
          using Lh = fn::expected<fn::sum_for<fn::pack<double, bool>, fn::pack<double, int>>, fn::sum<Error>>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::sum< //
                                                      fn::pack<double, bool, int>, fn::pack<double, int, int>>,
                                                  fn::sum_for<Error, bool, int>>>);

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
          CHECK((Lh{::fn::unexpect, fn::sum{FileNotFound}} & Rh{12}).error() == fn::sum{FileNotFound});
          CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{::fn::unexpect, fn::sum{13}}).error() == fn::sum{13});
          CHECK((Lh{::fn::unexpect, fn::sum{FileNotFound}} & Rh{::fn::unexpect, fn::sum{13}}).error()
                == fn::sum{FileNotFound});
        }
      }

      SECTION("sum on right side only")
      {
        using Lh = fn::expected<double, fn::sum<Error>>;
        using Rh = fn::expected<fn::sum<bool, int>, fn::sum<bool, int>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::sum< //
                                                    fn::pack<double, bool>, fn::pack<double, int>>,
                                                fn::sum_for<Error, bool, int>>>);

        CHECK((Lh{0.5} & Rh{fn::sum{12}})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::sum{true});
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{fn::sum{12}}).error() == fn::sum{FileNotFound});
        CHECK((Lh{0.5} & Rh{::fn::unexpect, fn::sum{13}}).error() == fn::sum{13});
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, fn::sum{13}}).error() == fn::sum{FileNotFound});

        SECTION("pack on left")
        {
          using Lh = fn::expected<fn::pack<double, int>, fn::sum<Error>>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::sum< //
                                                      fn::pack<double, int, bool>, fn::pack<double, int, int>>,
                                                  fn::sum_for<Error, bool, int>>>);

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
          CHECK((Lh{::fn::unexpect, fn::sum{FileNotFound}} & Rh{fn::sum{12}}).error() == fn::sum{FileNotFound});
          CHECK((Lh{fn::pack{0.5, 3}} & Rh{::fn::unexpect, fn::sum{13}}).error() == fn::sum{13});
          CHECK((Lh{::fn::unexpect, fn::sum{FileNotFound}} & Rh{::fn::unexpect, fn::sum{13}}).error()
                == fn::sum{FileNotFound});
        }
      }
    }

    SECTION("unit error grade sum<>")
    {
      // A never-erroring expected<T, sum<>> composes with a fallible one: the sum<> operand adds no
      // alternative to the widened error, only its value to the pack. Previously ill-formed, because the
      // sum<> side made operator&'s error lambda deduce void and poisoned _join's return type.
      using Unit = fn::expected<int, fn::sum<>>;

      SECTION("different error, unit on left")
      {
        using Rh = fn::expected<int, Error>;
        static_assert(std::same_as<decltype(std::declval<Unit>() & std::declval<Rh>()),
                                   fn::expected<fn::pack<int, int>, fn::sum<Error>>>);

        static_assert((Unit{7} & Rh{5}) //
                          .transform([](int a, int b) constexpr -> bool { return a == 7 && b == 5; })
                          .value());
        static_assert((Unit{7} & Rh{::fn::unexpect, FileNotFound}).error() == fn::sum{FileNotFound});

        CHECK((Unit{7} & Rh{5}) //
                  .transform([](int a, int b) constexpr -> bool { return a == 7 && b == 5; })
                  .value());
        CHECK((Unit{7} & Rh{::fn::unexpect, FileNotFound}).error() == fn::sum{FileNotFound});
      }

      SECTION("different error, unit on right")
      {
        using Lh = fn::expected<int, Error>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Unit>()),
                                   fn::expected<fn::pack<int, int>, fn::sum<Error>>>);

        static_assert((Lh{5} & Unit{7}) //
                          .transform([](int a, int b) constexpr -> bool { return a == 5 && b == 7; })
                          .value());
        static_assert((Lh{::fn::unexpect, FileNotFound} & Unit{7}).error() == fn::sum{FileNotFound});

        CHECK((Lh{5} & Unit{7}) //
                  .transform([](int a, int b) constexpr -> bool { return a == 5 && b == 7; })
                  .value());
        CHECK((Lh{::fn::unexpect, FileNotFound} & Unit{7}).error() == fn::sum{FileNotFound});
      }

      SECTION("unit meets a sum grade, either order")
      {
        using Rh = fn::expected<int, fn::sum<Error>>;
        static_assert(std::same_as<decltype(std::declval<Unit>() & std::declval<Rh>()),
                                   fn::expected<fn::pack<int, int>, fn::sum<Error>>>);
        static_assert(std::same_as<decltype(std::declval<Rh>() & std::declval<Unit>()),
                                   fn::expected<fn::pack<int, int>, fn::sum<Error>>>);

        static_assert((Unit{7} & Rh{::fn::unexpect, fn::sum{FileNotFound}}).error() == fn::sum{FileNotFound});
        static_assert((Rh{::fn::unexpect, fn::sum{FileNotFound}} & Unit{7}).error() == fn::sum{FileNotFound});

        CHECK((Unit{7} & Rh{::fn::unexpect, fn::sum{FileNotFound}}).error() == fn::sum{FileNotFound});
        CHECK((Rh{::fn::unexpect, fn::sum{FileNotFound}} & Unit{7}).error() == fn::sum{FileNotFound});
      }

      SECTION("same error, both unit")
      {
        static_assert(std::same_as<decltype(std::declval<Unit>() & std::declval<Unit>()),
                                   fn::expected<fn::pack<int, int>, fn::sum<>>>);

        static_assert((Unit{7} & Unit{5}) //
                          .transform([](int a, int b) constexpr -> bool { return a == 7 && b == 5; })
                          .value());
        CHECK((Unit{7} & Unit{5}) //
                  .transform([](int a, int b) constexpr -> bool { return a == 7 && b == 5; })
                  .value());
      }

      SECTION("void operand carries the unit error")
      {
        using VoidUnit = fn::expected<void, fn::sum<>>;
        using Rh = fn::expected<int, Error>;
        static_assert(
            std::same_as<decltype(std::declval<VoidUnit>() & std::declval<Rh>()), fn::expected<int, fn::sum<Error>>>);
        static_assert(
            std::same_as<decltype(std::declval<Rh>() & std::declval<VoidUnit>()), fn::expected<int, fn::sum<Error>>>);

        static_assert((VoidUnit{} & Rh{5}).value() == 5);
        static_assert((VoidUnit{} & Rh{::fn::unexpect, FileNotFound}).error() == fn::sum{FileNotFound});
        static_assert((Rh{5} & VoidUnit{}).value() == 5);
        static_assert((Rh{::fn::unexpect, FileNotFound} & VoidUnit{}).error() == fn::sum{FileNotFound});

        CHECK((VoidUnit{} & Rh{5}).value() == 5);
        CHECK((VoidUnit{} & Rh{::fn::unexpect, FileNotFound}).error() == fn::sum{FileNotFound});
        CHECK((Rh{5} & VoidUnit{}).value() == 5);
        CHECK((Rh{::fn::unexpect, FileNotFound} & VoidUnit{}).error() == fn::sum{FileNotFound});
      }
    }

    SECTION("constexpr")
    {
      // the graded/unit shapes have constant-evaluation asserts above; these cover the plain
      // same-error shapes, which had none
      static_assert((fn::expected<double, Error>{0.5} & fn::expected<int, Error>{12})
                        .transform([](double d, int i) constexpr -> bool { return d == 0.5 && i == 12; })
                        .value());
      static_assert((fn::expected<double, Error>{::fn::unexpect, FileNotFound} & fn::expected<int, Error>{12}).error()
                    == FileNotFound);
      SUCCEED();
    }
  }
}

TEST_CASE("expected sum support and_then", "[expected][sum][and_then]")
{
  using S = fn::expected<fn::sum_for<int, std::string_view>, Error>;

  // noexcept (extension, expected.hpp:77-81): same-error-type result AND nothrow invoke AND
  // nothrow error copy. A visitor set is not invocable on the whole sum, so the borrowed std
  // trait is conservatively false even with nothrow handlers (GH #254); a generic callback IS
  // invocable on the whole sum, and the spec then reports true from a call shape the
  // per-alternative dispatch never makes.
  constexpr auto nothrow_lval
      = fn::overload{[](int &) noexcept -> fn::expected<bool, Error> { return {true}; },
                     [](std::string_view &) noexcept -> fn::expected<bool, Error> { return {false}; }};
  static_assert(not noexcept(std::declval<S &>().and_then(nothrow_lval)));
  constexpr auto nothrow_generic = [](auto &&) noexcept -> fn::expected<bool, Error> { return {true}; };
  static_assert(noexcept(std::declval<S &>().and_then(nothrow_generic)));

  // constraints (extension, :82-83): exhaustive invocability over the sum's alternatives,
  // tracking their value category, AND copyability of the untouched error
  constexpr auto can_and_then_lval = [](auto &&f) { return requires { std::declval<S &>().and_then(f); }; };
  constexpr auto can_and_then_rval = [](auto &&f) { return requires { std::declval<S &&>().and_then(f); }; };
  static_assert(can_and_then_lval(nothrow_lval));
  static_assert(not can_and_then_rval(nothrow_lval)); // no rvalue handlers
  static_assert(not can_and_then_lval(fn::overload{[](int &) -> fn::expected<bool, Error> { throw 0; }})); // partial
  static_assert(not can_and_then_lval(42));
  struct move_only_error {
    move_only_error(move_only_error &&) = default;
  };
  using M = fn::expected<fn::sum<int>, move_only_error>;
  constexpr auto generic_fn = [](auto &&) -> fn::expected<bool, move_only_error> { throw 0; };
  constexpr auto can_and_then_M_lval = [](auto &&f) { return requires { std::declval<M &>().and_then(f); }; };
  constexpr auto can_and_then_M_rval = [](auto &&f) { return requires { std::declval<M &&>().and_then(f); }; };
  static_assert(not can_and_then_M_lval(generic_fn)); // error copy required, E move-only
  static_assert(can_and_then_M_rval(generic_fn));     // error moved

  SECTION("value")
  {
    fn::expected<fn::sum_for<int, std::string_view>, Error> s{fn::sum{12}};

    CHECK(s.and_then( //
               fn::overload{[](int &i) -> fn::expected<bool, Error> { return i == 12; },
                            [](int const &) -> fn::expected<bool, Error> { throw 0; },
                            [](int &&) -> fn::expected<bool, Error> { throw 0; },
                            [](int const &&) -> fn::expected<bool, Error> { throw 0; },
                            [](std::string_view &) -> fn::expected<bool, Error> { throw 0; },
                            [](std::string_view const &) -> fn::expected<bool, Error> { throw 0; },
                            [](std::string_view &&) -> fn::expected<bool, Error> { throw 0; },
                            [](std::string_view const &&) -> fn::expected<bool, Error> { throw 0; }})
              .value());

    CHECK(std::as_const(s)
              .and_then( //
                  fn::overload{[](int &) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &i) -> fn::expected<bool, Error> { return i == 12; },
                               [](int &&) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &&) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view &) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view const &) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view &&) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view const &&) -> fn::expected<bool, Error> { throw 0; }})
              .value());

    CHECK(std::move(std::as_const(s))
              .and_then( //
                  fn::overload{[](int &) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &) -> fn::expected<bool, Error> { throw 0; },
                               [](int &&) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &&i) -> fn::expected<bool, Error> { return i == 12; },
                               [](std::string_view &) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view const &) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view &&) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view const &&) -> fn::expected<bool, Error> { throw 0; }})
              .value());

    CHECK(std::move(s)
              .and_then( //
                  fn::overload{[](int &) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &) -> fn::expected<bool, Error> { throw 0; },
                               [](int &&i) -> fn::expected<bool, Error> { return i == 12; },
                               [](int const &&) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view &) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view const &) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view &&) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view const &&) -> fn::expected<bool, Error> { throw 0; }})
              .value());
  }

  SECTION("error")
  {
    fn::expected<fn::sum_for<int, std::string_view>, Error> s{::fn::unexpect, FileNotFound};
    CHECK(s.and_then( //
               [](auto...) -> fn::expected<bool, Error> { return {true}; })
              .error()
          == FileNotFound);
    CHECK(std::as_const(s)
              .and_then( //
                  [](auto...) -> fn::expected<bool, Error> { return {true}; })
              .error()
          == FileNotFound);
    CHECK(fn::expected<fn::sum_for<int, std::string_view>, Error>{::fn::unexpect, FileNotFound}
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

  SECTION("constexpr")
  {
    constexpr auto fn = fn::overload{[](int &) -> fn::expected<bool, Error> { throw 0; },
                                     [](int const &i) -> fn::expected<bool, Error> { return i == 42; },
                                     [](int &&) -> fn::expected<bool, Error> { throw 0; },
                                     [](int const &&) -> fn::expected<bool, Error> { throw 0; },
                                     [](std::string_view &) -> fn::expected<bool, Error> { throw 0; },
                                     [](std::string_view const &) -> fn::expected<bool, Error> { throw 0; },
                                     [](std::string_view &&) -> fn::expected<bool, Error> { throw 0; },
                                     [](std::string_view const &&) -> fn::expected<bool, Error> { throw 0; }};
    constexpr fn::expected<fn::sum_for<int, std::string_view>, Error> a{fn::sum{42}};
    static_assert(std::is_same_v<decltype(a.and_then(fn)), fn::expected<bool, Error>>);
    static_assert(a.and_then(fn).value());
  }
}

TEST_CASE("expected sum support or_else", "[expected][sum][or_else]")
{
  using S = fn::expected<double, fn::sum_for<int, std::string_view>>;

  // noexcept (extension, expected.hpp:159-166): same-value-type result AND nothrow invoke AND
  // nothrow value copy. A visitor set is not invocable on the whole sum, so the borrowed std
  // trait is conservatively false (GH #254); a generic same-value-type callback reports true.
  constexpr auto nothrow_lval
      = fn::overload{[](int &) noexcept -> fn::expected<double, Error> { return {0.5}; },
                     [](std::string_view &) noexcept -> fn::expected<double, Error> { return {0.5}; }};
  static_assert(not noexcept(std::declval<S &>().or_else(nothrow_lval)));
  constexpr auto nothrow_generic = [](auto &&) noexcept -> fn::expected<double, Error> { return {0.5}; };
  static_assert(noexcept(std::declval<S &>().or_else(nothrow_generic)));

  // constraints (extension, :167-168): invocability over the error sum's alternatives, tracking
  // their value category, AND copyability of the untouched value -- a move-only value type
  // cleanly drops every overload whose self would copy it
  constexpr auto can_or_else_lval = [](auto &&f) { return requires { std::declval<S &>().or_else(f); }; };
  constexpr auto can_or_else_rval = [](auto &&f) { return requires { std::declval<S &&>().or_else(f); }; };
  static_assert(can_or_else_lval(nothrow_lval));
  static_assert(not can_or_else_rval(nothrow_lval)); // no rvalue handlers
  static_assert(not can_or_else_lval(fn::overload{[](int &) -> fn::expected<double, Error> { throw 0; }})); // partial
  static_assert(not can_or_else_lval(42));
  struct move_only {
    move_only(move_only &&) = default;
  };
  using M = fn::expected<move_only, fn::sum<int>>;
  constexpr auto generic_fn = [](auto &&) -> fn::expected<move_only, Error> { throw 0; };
  constexpr auto can_or_else_M_lval = [](auto &&f) { return requires { std::declval<M &>().or_else(f); }; };
  constexpr auto can_or_else_M_rval = [](auto &&f) { return requires { std::declval<M &&>().or_else(f); }; };
  static_assert(not can_or_else_M_lval(generic_fn)); // value copy required, T move-only
  static_assert(can_or_else_M_rval(generic_fn));     // value moved

  SECTION("value")
  {
    fn::expected<double, fn::sum_for<int, std::string_view>> s{::fn::unexpect, fn::sum{12}};

    CHECK(s.or_else( //
               fn::overload{[](int &i) -> fn::expected<double, Error> { return {i}; },
                            [](int const &) -> fn::expected<double, Error> { throw 0; },
                            [](int &&) -> fn::expected<double, Error> { throw 0; },
                            [](int const &&) -> fn::expected<double, Error> { throw 0; },
                            [](std::string_view &) -> fn::expected<double, Error> { throw 0; },
                            [](std::string_view const &) -> fn::expected<double, Error> { throw 0; },
                            [](std::string_view &&) -> fn::expected<double, Error> { throw 0; },
                            [](std::string_view const &&) -> fn::expected<double, Error> { throw 0; }})
              .value()
          == 12);

    CHECK(std::as_const(s)
              .or_else( //
                  fn::overload{[](int &) -> fn::expected<double, Error> { throw 0; },
                               [](int const &i) -> fn::expected<double, Error> { return {i}; },
                               [](int &&) -> fn::expected<double, Error> { throw 0; },
                               [](int const &&) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view &) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view const &) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view &&) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view const &&) -> fn::expected<double, Error> { throw 0; }})
              .value()
          == 12);

    CHECK(std::move(std::as_const(s))
              .or_else( //
                  fn::overload{[](int &) -> fn::expected<double, Error> { throw 0; },
                               [](int const &) -> fn::expected<double, Error> { throw 0; },
                               [](int &&) -> fn::expected<double, Error> { throw 0; },
                               [](int const &&i) -> fn::expected<double, Error> { return {i}; },
                               [](std::string_view &) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view const &) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view &&) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view const &&) -> fn::expected<double, Error> { throw 0; }})
              .value()
          == 12);

    CHECK(std::move(s)
              .or_else( //
                  fn::overload{[](int &) -> fn::expected<double, Error> { throw 0; },
                               [](int const &) -> fn::expected<double, Error> { throw 0; },
                               [](int &&i) -> fn::expected<double, Error> { return {i}; },
                               [](int const &&) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view &) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view const &) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view &&) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view const &&) -> fn::expected<double, Error> { throw 0; }})
              .value()
          == 12);

    SECTION("value")
    {
      fn::expected<double, fn::sum_for<int, std::string_view>> s{1.5};
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

    SECTION("constexpr")
    {
      constexpr auto fn = fn::overload{[](int &) -> fn::expected<double, Error> { throw 0; },
                                       [](int const &i) -> fn::expected<double, Error> { return {i}; },
                                       [](int &&) -> fn::expected<double, Error> { throw 0; },
                                       [](int const &&) -> fn::expected<double, Error> { throw 0; },
                                       [](std::string_view &) -> fn::expected<double, Error> { throw 0; },
                                       [](std::string_view const &) -> fn::expected<double, Error> { throw 0; },
                                       [](std::string_view &&) -> fn::expected<double, Error> { throw 0; },
                                       [](std::string_view const &&) -> fn::expected<double, Error> { throw 0; }};
      constexpr fn::expected<double, fn::sum_for<int, std::string_view>> a{::fn::unexpect, fn::sum{42}};
      static_assert(std::is_same_v<decltype(a.or_else(fn)), fn::expected<double, Error>>);
      static_assert(a.or_else(fn).value() == 42);
    }
  }

  SECTION("void")
  {
    fn::expected<void, fn::sum_for<int, std::string_view>> s{::fn::unexpect, fn::sum{12}};

    CHECK(s.or_else( //
               fn::overload{[](int &) -> fn::expected<void, Error> { return ::fn::unexpected<Error>{FileNotFound}; },
                            [](int const &) -> fn::expected<void, Error> { throw 0; },
                            [](int &&) -> fn::expected<void, Error> { throw 0; },
                            [](int const &&) -> fn::expected<void, Error> { throw 0; },
                            [](std::string_view &) -> fn::expected<void, Error> { throw 0; },
                            [](std::string_view const &) -> fn::expected<void, Error> { throw 0; },
                            [](std::string_view &&) -> fn::expected<void, Error> { throw 0; },
                            [](std::string_view const &&) -> fn::expected<void, Error> { throw 0; }})
              .error()
          == FileNotFound);

    CHECK(std::as_const(s)
              .or_else( //
                  fn::overload{
                      [](int &) -> fn::expected<void, Error> { throw 0; },
                      [](int const &) -> fn::expected<void, Error> { return ::fn::unexpected<Error>{FileNotFound}; },
                      [](int &&) -> fn::expected<void, Error> { throw 0; },
                      [](int const &&) -> fn::expected<void, Error> { throw 0; },
                      [](std::string_view &) -> fn::expected<void, Error> { throw 0; },
                      [](std::string_view const &) -> fn::expected<void, Error> { throw 0; },
                      [](std::string_view &&) -> fn::expected<void, Error> { throw 0; },
                      [](std::string_view const &&) -> fn::expected<void, Error> { throw 0; }})
              .error()
          == FileNotFound);

    CHECK(std::move(std::as_const(s))
              .or_else( //
                  fn::overload{
                      [](int &) -> fn::expected<void, Error> { throw 0; },
                      [](int const &) -> fn::expected<void, Error> { throw 0; },
                      [](int &&) -> fn::expected<void, Error> { throw 0; },
                      [](int const &&) -> fn::expected<void, Error> { return ::fn::unexpected<Error>{FileNotFound}; },
                      [](std::string_view &) -> fn::expected<void, Error> { throw 0; },
                      [](std::string_view const &) -> fn::expected<void, Error> { throw 0; },
                      [](std::string_view &&) -> fn::expected<void, Error> { throw 0; },
                      [](std::string_view const &&) -> fn::expected<void, Error> { throw 0; }})
              .error()
          == FileNotFound);

    CHECK(
        std::move(s)
            .or_else( //
                fn::overload{[](int &) -> fn::expected<void, Error> { throw 0; },
                             [](int const &) -> fn::expected<void, Error> { throw 0; },
                             [](int &&) -> fn::expected<void, Error> { return ::fn::unexpected<Error>{FileNotFound}; },
                             [](int const &&) -> fn::expected<void, Error> { throw 0; },
                             [](std::string_view &) -> fn::expected<void, Error> { throw 0; },
                             [](std::string_view const &) -> fn::expected<void, Error> { throw 0; },
                             [](std::string_view &&) -> fn::expected<void, Error> { throw 0; },
                             [](std::string_view const &&) -> fn::expected<void, Error> { throw 0; }})
            .error()
        == FileNotFound);

    SECTION("value")
    {
      fn::expected<void, fn::sum_for<int, std::string_view>> s{};
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

    SECTION("constexpr")
    {
      constexpr auto fn
          = fn::overload{[](int &) -> fn::expected<void, Error> { throw 0; },
                         [](int const &) -> fn::expected<void, Error> { return ::fn::unexpected<Error>{FileNotFound}; },
                         [](int &&) -> fn::expected<void, Error> { throw 0; },
                         [](int const &&) -> fn::expected<void, Error> { throw 0; },
                         [](std::string_view &) -> fn::expected<void, Error> { throw 0; },
                         [](std::string_view const &) -> fn::expected<void, Error> { throw 0; },
                         [](std::string_view &&) -> fn::expected<void, Error> { throw 0; },
                         [](std::string_view const &&) -> fn::expected<void, Error> { throw 0; }};
      constexpr fn::expected<void, fn::sum_for<int, std::string_view>> a{::fn::unexpect, fn::sum{42}};
      static_assert(std::is_same_v<decltype(a.or_else(fn)), fn::expected<void, Error>>);
      static_assert(a.or_else(fn).error() == FileNotFound);
    }
  }
}

TEST_CASE("expected sum support transform", "[expected][sum][transform]")
{
  using S = fn::expected<fn::sum_for<int, std::string_view>, Error>;

  // noexcept: the sum-case _transform (expected.hpp:238) carries no noexcept spec at all --
  // noexcept(false) even for callbacks whose and_then counterpart reports true (GH #254)
  constexpr auto nothrow_visitor = fn::overload{[](int const &) noexcept -> bool { return true; },
                                                [](std::string_view const &) noexcept -> bool { return false; }};
  static_assert(not noexcept(std::declval<S &>().transform(nothrow_visitor)));
  constexpr auto nothrow_generic = [](auto &&) noexcept -> bool { return true; };
  static_assert(not noexcept(std::declval<S &>().transform(nothrow_generic)));

  constexpr auto can_transform = [](auto &&f) { return requires { std::declval<S &>().transform(f); }; };
  static_assert(can_transform(nothrow_visitor));
  // the error-copy conjunct (:239) IS constrained: a move-only error cleanly drops the
  // overloads whose self would copy it, before the unconstrained body could hard-error
  struct move_only_error {
    move_only_error(move_only_error &&) = default;
  };
  using M = fn::expected<fn::sum<int>, move_only_error>;
  constexpr auto can_transform_M_lval = [](auto &&f) { return requires { std::declval<M &>().transform(f); }; };
  constexpr auto can_transform_M_rval = [](auto &&f) { return requires { std::declval<M &&>().transform(f); }; };
  static_assert(not can_transform_M_lval(nothrow_visitor));
  static_assert(can_transform_M_rval(nothrow_visitor));

  SECTION("value")
  {
    fn::expected<fn::sum_for<int, std::string_view>, Error> s{fn::sum{12}};

    CHECK(s.transform( //
               fn::overload{
                   [](int &) -> std::monostate { return {}; }, [](int const &) -> std::monostate { throw 0; },
                   [](int &&) -> std::monostate { throw 0; }, [](int const &&) -> std::monostate { throw 0; },
                   [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                   [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }})
              .value()
              .has_value<std::monostate>());

    CHECK(std::as_const(s)
              .transform( //
                  fn::overload{
                      [](int &) -> std::monostate { throw 0; }, [](int const &) -> std::monostate { return {}; },
                      [](int &&) -> std::monostate { throw 0; }, [](int const &&) -> std::monostate { throw 0; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }})
              .value()
              .has_value<std::monostate>());

    CHECK(std::move(std::as_const(s))
              .transform( //
                  fn::overload{
                      [](int &) -> std::monostate { throw 0; }, [](int const &) -> std::monostate { throw 0; },
                      [](int &&) -> std::monostate { throw 0; }, [](int const &&) -> std::monostate { return {}; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }})
              .value()
              .has_value<std::monostate>());

    CHECK(std::move(s)
              .transform( //
                  fn::overload{
                      [](int &) -> std::monostate { throw 0; }, [](int const &) -> std::monostate { throw 0; },
                      [](int &&) -> std::monostate { return {}; }, [](int const &&) -> std::monostate { throw 0; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }})
              .value()
              .has_value<std::monostate>());
  }

  SECTION("error")
  {
    fn::expected<fn::sum_for<int, std::string_view>, Error> s{::fn::unexpect, FileNotFound};
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

  SECTION("constexpr")
  {
    constexpr auto fn = fn::overload{[](int &) -> bool { throw 0; },
                                     [](int const &) -> bool { return true; },
                                     [](int &&) -> bool { throw 0; },
                                     [](int const &&) -> bool { throw 0; },
                                     [](std::string_view &) -> int { throw 0; },
                                     [](std::string_view const &) -> int { throw 0; },
                                     [](std::string_view &&) -> int { throw 0; },
                                     [](std::string_view const &&) -> int { throw 0; }};
    constexpr fn::expected<fn::sum_for<int, std::string_view>, Error> a{fn::sum{42}};
    static_assert(std::is_same_v<decltype(a.transform(fn)), fn::expected<fn::sum<bool, int>, Error>>);
    // TODO Switch bool to std::monostate or similar user-defined type
    static_assert(a.transform(fn).value().has_value<bool>());
  }

  SECTION("constraints")
  {
    constexpr auto can_transform_clval = [](auto &&f) { return requires { std::declval<S const &>().transform(f); }; };

    // a callback no alternative can take drops the candidate, rather than failing inside the body
    static_assert(not can_transform([](double) -> bool { throw 0; }));
    static_assert(not can_transform([](int &) -> bool { throw 0; })); // string_view is unhandled

    // a visitor need only serve the value category the call actually selects
    constexpr auto lval_only
        = fn::overload{[](int &i) -> bool { return i == 12; }, [](std::string_view &) -> bool { throw 0; }};
    static_assert(can_transform(lval_only));
    static_assert(not can_transform_clval(lval_only));

    S s{fn::sum{12}};
    CHECK(s.transform(lval_only).value() == fn::sum{true});
  }
}

TEST_CASE("expected sum support transform_error", "[expected][sum][transform_error]")
{
  using S = fn::expected<double, fn::sum_for<int, std::string_view>>;

  // noexcept: the sum-case _transform_error (expected.hpp:301) carries no noexcept spec at all
  // (GH #254)
  constexpr auto nothrow_visitor = fn::overload{[](int const &) noexcept -> bool { return true; },
                                                [](std::string_view const &) noexcept -> bool { return false; }};
  static_assert(not noexcept(std::declval<S &>().transform_error(nothrow_visitor)));
  constexpr auto nothrow_generic = [](auto &&) noexcept -> bool { return true; };
  static_assert(not noexcept(std::declval<S &>().transform_error(nothrow_generic)));

  constexpr auto can_transform_error = [](auto &&f) { return requires { std::declval<S &>().transform_error(f); }; };
  static_assert(can_transform_error(nothrow_visitor));

  SECTION("value")
  {
    fn::expected<double, fn::sum_for<int, std::string_view>> s{::fn::unexpect, fn::sum{12}};

    CHECK(s.transform_error( //
               fn::overload{
                   [](int &i) -> bool { return i == 12; }, [](int const &) -> bool { throw 0; },
                   [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; },
                   [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                   [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }})
              .error()
          == fn::sum{true});

    CHECK(std::as_const(s)
              .transform_error( //
                  fn::overload{
                      [](int &) -> bool { throw 0; }, [](int const &i) -> bool { return i == 12; },
                      [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }})
              .error()
          == fn::sum{true});

    CHECK(std::move(std::as_const(s))
              .transform_error( //
                  fn::overload{
                      [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                      [](int &&) -> bool { throw 0; }, [](int const &&i) -> bool { return i == 12; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }})
              .error()
          == fn::sum{true});

    CHECK(std::move(s)
              .transform_error( //
                  fn::overload{
                      [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                      [](int &&i) -> bool { return i == 12; }, [](int const &&) -> bool { throw 0; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }})
              .error()
          == fn::sum{true});

    SECTION("value")
    {
      fn::expected<double, fn::sum_for<int, std::string_view>> s{1.5};
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

    SECTION("constexpr")
    {
      constexpr auto fn = fn::overload{[](int &) -> bool { throw 0; },
                                       [](int const &i) -> bool { return i == 42; },
                                       [](int &&) -> bool { throw 0; },
                                       [](int const &&) -> bool { throw 0; },
                                       [](std::string_view &) -> int { throw 0; },
                                       [](std::string_view const &) -> int { throw 0; },
                                       [](std::string_view &&) -> int { throw 0; },
                                       [](std::string_view const &&) -> int { throw 0; }};
      constexpr fn::expected<double, fn::sum_for<int, std::string_view>> a{::fn::unexpect, fn::sum{42}};
      static_assert(std::is_same_v<decltype(a.transform_error(fn)), fn::expected<double, fn::sum<bool, int>>>);
      static_assert(a.transform_error(fn).error() == fn::sum{true});
    }
  }

  SECTION("void")
  {
    fn::expected<void, fn::sum_for<int, std::string_view>> s{::fn::unexpect, fn::sum{12}};

    CHECK(s.transform_error( //
               fn::overload{
                   [](int &i) -> int { return i; }, [](int const &) -> int { throw 0; }, [](int &&) -> int { throw 0; },
                   [](int const &&) -> int { throw 0; }, [](std::string_view &) -> int { throw 0; },
                   [](std::string_view const &) -> int { throw 0; }, [](std::string_view &&) -> int { throw 0; },
                   [](std::string_view const &&) -> int { throw 0; }})
              .error()
          == fn::sum{12});

    CHECK(std::as_const(s)
              .transform_error( //
                  fn::overload{
                      [](int &) -> int { throw 0; }, [](int const &i) -> int { return i; },
                      [](int &&) -> int { throw 0; }, [](int const &&) -> int { throw 0; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }})
              .error()
          == fn::sum{12});

    CHECK(std::move(std::as_const(s))
              .transform_error( //
                  fn::overload{
                      [](int &) -> int { throw 0; }, [](int const &) -> int { throw 0; },
                      [](int &&) -> int { throw 0; }, [](int const &&i) -> int { return i; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }})
              .error()
          == fn::sum{12});

    CHECK(std::move(s)
              .transform_error( //
                  fn::overload{
                      [](int &) -> int { throw 0; }, [](int const &) -> int { throw 0; },
                      [](int &&i) -> int { return i; }, [](int const &&) -> int { throw 0; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }})
              .error()
          == fn::sum{12});

    SECTION("value")
    {
      fn::expected<void, fn::sum_for<int, std::string_view>> s{};
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

    SECTION("constexpr")
    {
      constexpr auto fn = fn::overload{[](int &) -> int { throw 0; },
                                       [](int const &i) -> int { return i; },
                                       [](int &&) -> int { throw 0; },
                                       [](int const &&) -> int { throw 0; },
                                       [](std::string_view &) -> int { throw 0; },
                                       [](std::string_view const &) -> int { throw 0; },
                                       [](std::string_view &&) -> int { throw 0; },
                                       [](std::string_view const &&) -> int { throw 0; }};
      constexpr fn::expected<void, fn::sum_for<int, std::string_view>> a{::fn::unexpect, fn::sum{42}};
      static_assert(std::is_same_v<decltype(a.transform_error(fn)), fn::expected<void, fn::sum<int>>>);
      static_assert(a.transform_error(fn).error() == fn::sum{42});
    }
  }

  SECTION("constraints")
  {
    constexpr auto can_transform_error_clval
        = [](auto &&f) { return requires { std::declval<S const &>().transform_error(f); }; };

    // a callback no alternative can take drops the candidate, rather than failing inside the body
    static_assert(not can_transform_error([](double) -> bool { throw 0; }));
    static_assert(not can_transform_error([](int &) -> bool { throw 0; })); // string_view is unhandled

    // a visitor need only serve the value category the call actually selects
    constexpr auto lval_only
        = fn::overload{[](int &i) -> bool { return i == 12; }, [](std::string_view &) -> bool { throw 0; }};
    static_assert(can_transform_error(lval_only));
    static_assert(not can_transform_error_clval(lval_only));

    S s{::fn::unexpect, fn::sum{12}};
    CHECK(s.transform_error(lval_only).error() == fn::sum{true});
  }

  SECTION("move-only value")
  {
    // the conjunct or_else carries and pfn's transform_error requires: the untouched value goes
    // into the result, so only the overloads whose self can be moved from survive
    constexpr auto generic = [](auto &&) -> bool { throw 0; };

    using M = fn::expected<std::unique_ptr<int>, fn::sum<int>>; // sum-case overload
    constexpr auto can_M_lval = [](auto &&f) { return requires { std::declval<M &>().transform_error(f); }; };
    constexpr auto can_M_rval = [](auto &&f) { return requires { std::declval<M &&>().transform_error(f); }; };
    static_assert(not can_M_lval(generic)); // would copy the value
    static_assert(can_M_rval(generic));     // moves it

    using N = fn::expected<std::unique_ptr<int>, Error>; // non-sum overload
    constexpr auto can_N_lval = [](auto &&f) { return requires { std::declval<N &>().transform_error(f); }; };
    constexpr auto can_N_rval = [](auto &&f) { return requires { std::declval<N &&>().transform_error(f); }; };
    static_assert(not can_N_lval(generic));
    static_assert(can_N_rval(generic));

    N n{std::make_unique<int>(7)};
    auto r = std::move(n).transform_error([](Error const &) -> bool { throw 0; });
    static_assert(std::is_same_v<decltype(r), fn::expected<std::unique_ptr<int>, bool>>);
    CHECK(*r.value() == 7);
  }
}

TEST_CASE("expected pack support or_else", "[expected][or_else][pack]")
{
  using S = fn::expected<int, fn::pack<int, Error>>;

  // noexcept (extension, expected.hpp:159-166): a multi-argument visitor is not invocable on
  // the whole pack, so the borrowed std trait is conservatively false (GH #254); a generic
  // same-value-type callback IS, and reports true
  constexpr auto nothrow_two = [](int, Error &) noexcept -> fn::expected<int, Error> { return {1}; };
  static_assert(not noexcept(std::declval<S &>().or_else(nothrow_two)));
  constexpr auto nothrow_generic = [](auto &&...) noexcept -> fn::expected<int, Error> { return {1}; };
  static_assert(noexcept(std::declval<S &>().or_else(nothrow_generic)));

  // constraints (extension, :167-168): pack-apply invocability tracking the pack's value
  // category; wrong arity or a non-callable SFINAE-drops
  constexpr auto can_or_else_lval = [](auto &&f) { return requires { std::declval<S &>().or_else(f); }; };
  constexpr auto can_or_else_rval = [](auto &&f) { return requires { std::declval<S &&>().or_else(f); }; };
  static_assert(can_or_else_lval(nothrow_two));
  static_assert(not can_or_else_rval(nothrow_two));                                          // lvalue-only visitor
  static_assert(not can_or_else_lval([](Error &) -> fn::expected<int, Error> { throw 0; })); // wrong arity
  static_assert(not can_or_else_lval(42));

  SECTION("value")
  {
    fn::expected<int, fn::pack<int, Error>> s{::fn::unexpect, fn::pack{12, FileNotFound}};
    CHECK(s.or_else( //
               fn::overload{[](int, Error &e) -> fn::expected<int, Error> { return e == FileNotFound; },
                            [](int, Error const &) -> fn::expected<int, Error> { throw 0; },
                            [](int, Error &&) -> fn::expected<int, Error> { throw 0; },
                            [](int, Error const &&) -> fn::expected<int, Error> { throw 0; }}) //
              .value());
    CHECK(std::as_const(s)
              .or_else( //
                  fn::overload{[](int, Error &) -> fn::expected<int, Error> { throw 0; },
                               [](int, Error const &e) -> fn::expected<int, Error> { return e == FileNotFound; },
                               [](int, Error &&) -> fn::expected<int, Error> { throw 0; },
                               [](int, Error const &&) -> fn::expected<int, Error> { throw 0; }}) //
              .value());
    CHECK(std::move(std::as_const(s))
              .or_else( //
                  fn::overload{[](int, Error &) -> fn::expected<int, Error> { throw 0; },
                               [](int, Error const &) -> fn::expected<int, Error> { throw 0; },
                               [](int, Error &&) -> fn::expected<int, Error> { throw 0; },
                               [](int, Error const &&e) -> fn::expected<int, Error> { return e == FileNotFound; }}) //
              .value());
    CHECK(std::move(s)
              .or_else( //
                  fn::overload{[](int, Error &) -> fn::expected<int, Error> { throw 0; },
                               [](int, Error const &) -> fn::expected<int, Error> { throw 0; },
                               [](int, Error &&e) -> fn::expected<int, Error> { return e == FileNotFound; },
                               [](int, Error const &&) -> fn::expected<int, Error> { throw 0; }}) //
              .value());
  }

  SECTION("void")
  {
    fn::expected<void, fn::pack<int, Error>> s{::fn::unexpect, fn::pack{12, FileNotFound}};
    CHECK(s.or_else( //
               fn::overload{[](int, Error &) -> fn::expected<void, Error> { return {}; },
                            [](int, Error const &) -> fn::expected<void, Error> { throw 0; },
                            [](int, Error &&) -> fn::expected<void, Error> { throw 0; },
                            [](int, Error const &&) -> fn::expected<void, Error> { throw 0; }})
              .has_value());
    CHECK(std::as_const(s)
              .or_else( //
                  fn::overload{[](int, Error &) -> fn::expected<void, Error> { throw 0; },
                               [](int, Error const &) -> fn::expected<void, Error> { return {}; },
                               [](int, Error &&) -> fn::expected<void, Error> { throw 0; },
                               [](int, Error const &&) -> fn::expected<void, Error> { throw 0; }})
              .has_value());
    CHECK(std::move(std::as_const(s))
              .or_else( //
                  fn::overload{[](int, Error &) -> fn::expected<void, Error> { throw 0; },
                               [](int, Error const &) -> fn::expected<void, Error> { throw 0; },
                               [](int, Error &&) -> fn::expected<void, Error> { throw 0; },
                               [](int, Error const &&) -> fn::expected<void, Error> { return {}; }})
              .has_value());
    CHECK(std::move(s)
              .or_else( //
                  fn::overload{[](int, Error &) -> fn::expected<void, Error> { throw 0; },
                               [](int, Error const &) -> fn::expected<void, Error> { throw 0; },
                               [](int, Error &&) -> fn::expected<void, Error> { return {}; },
                               [](int, Error const &&) -> fn::expected<void, Error> { throw 0; }})
              .has_value());
  }

  SECTION("constexpr")
  {
    constexpr S a{::fn::unexpect, fn::pack{12, FileNotFound}};
    static_assert(a.or_else([](int i, Error const &e) -> fn::expected<int, Error> {
                     return {i == 12 && e == FileNotFound};
                   }).value());
    constexpr S b{1};
    static_assert(b.or_else([](auto &&...) -> fn::expected<int, Error> { throw 0; }).value() == 1);
    SUCCEED();
  }
}

TEST_CASE("expected pack support transform_error", "[expected][transform_error][pack]")
{
  using S = fn::expected<int, fn::pack<int, Error>>;

  // noexcept (extension, expected.hpp:279-282): nothrow invoke AND nothrow value copy; a
  // multi-argument visitor is not invocable on the whole pack, so conservatively false (GH
  // #254); a generic callback reports true (int value, nothrow copy)
  constexpr auto nothrow_two = [](int, Error &) noexcept -> bool { return true; };
  static_assert(not noexcept(std::declval<S &>().transform_error(nothrow_two)));
  constexpr auto nothrow_generic = [](auto &&...) noexcept -> bool { return true; };
  static_assert(noexcept(std::declval<S &>().transform_error(nothrow_generic)));

  // constraints (:283-284): pack-apply invocability, and the untouched value's copy
  constexpr auto can_te_lval = [](auto &&f) { return requires { std::declval<S &>().transform_error(f); }; };
  constexpr auto can_te_rval = [](auto &&f) { return requires { std::declval<S &&>().transform_error(f); }; };
  static_assert(can_te_lval(nothrow_two));
  static_assert(not can_te_rval(nothrow_two));                      // lvalue-only visitor
  static_assert(not can_te_lval([](Error &) -> bool { throw 0; })); // wrong arity
  static_assert(not can_te_lval(42));

  SECTION("value")
  {
    fn::expected<int, fn::pack<int, Error>> s{::fn::unexpect, fn::pack{12, FileNotFound}};
    CHECK(s.transform_error( //
               fn::overload{[](int, Error &e) -> bool { return e == FileNotFound; },
                            [](int, Error const &) -> bool { throw 0; }, [](int, Error &&) -> bool { throw 0; },
                            [](int, Error const &&) -> bool { throw 0; }}) //
              .error());
    CHECK(std::as_const(s)
              .transform_error( //
                  fn::overload{[](int, Error &) -> bool { throw 0; },
                               [](int, Error const &e) -> bool { return e == FileNotFound; },
                               [](int, Error &&) -> bool { throw 0; }, [](int, Error const &&) -> bool { throw 0; }}) //
              .error());
    CHECK(std::move(std::as_const(s))
              .transform_error( //
                  fn::overload{[](int, Error &) -> bool { throw 0; }, [](int, Error const &) -> bool { throw 0; },
                               [](int, Error &&) -> bool { throw 0; },
                               [](int, Error const &&e) -> bool { return e == FileNotFound; }}) //
              .error());
    CHECK(std::move(s)
              .transform_error( //
                  fn::overload{[](int, Error &) -> bool { throw 0; }, [](int, Error const &) -> bool { throw 0; },
                               [](int, Error &&e) -> bool { return e == FileNotFound; },
                               [](int, Error const &&) -> bool { throw 0; }}) //
              .error());
  }

  SECTION("void")
  {
    fn::expected<void, fn::pack<int, Error>> s{::fn::unexpect, fn::pack{12, FileNotFound}};
    CHECK(s.transform_error( //
               fn::overload{[](int, Error &) -> bool { return true; }, [](int, Error const &) -> bool { throw 0; },
                            [](int, Error &&) -> bool { throw 0; }, [](int, Error const &&) -> bool { throw 0; }})
              .error());
    CHECK(std::as_const(s)
              .transform_error( //
                  fn::overload{[](int, Error &) -> bool { throw 0; }, [](int, Error const &) -> bool { return true; },
                               [](int, Error &&) -> bool { throw 0; }, [](int, Error const &&) -> bool { throw 0; }})
              .error());
    CHECK(
        std::move(std::as_const(s))
            .transform_error( //
                fn::overload{[](int, Error &) -> bool { throw 0; }, [](int, Error const &) -> bool { throw 0; },
                             [](int, Error &&) -> bool { throw 0; }, [](int, Error const &&) -> bool { return true; }})
            .error());
    CHECK(
        std::move(s)
            .transform_error( //
                fn::overload{[](int, Error &) -> bool { throw 0; }, [](int, Error const &) -> bool { throw 0; },
                             [](int, Error &&) -> bool { return true; }, [](int, Error const &&) -> bool { throw 0; }})
            .error());
  }

  SECTION("constexpr")
  {
    constexpr S a{::fn::unexpect, fn::pack{12, FileNotFound}};
    static_assert(
        a.transform_error([](int i, Error const &e) -> bool { return i == 12 && e == FileNotFound; }).error());
    constexpr S b{1};
    static_assert(b.transform_error([](auto &&...) -> bool { throw 0; }).value() == 1);
    SUCCEED();
  }
}
