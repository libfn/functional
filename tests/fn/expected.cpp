// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/expected.hpp>
#include <fn/utility.hpp>

#include <catch2/catch_all.hpp>

#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <variant>

#include <fn/detail/macro_begin.hpp>

namespace {
enum Error { Unknown, FileNotFound };

struct Xint {
  int value;
  constexpr bool operator==(Xint const &) const noexcept = default;
};

// Nothrow move, throwing copy: joining lvalue operands copies the value or error, joining rvalues
// moves it
struct MoveNothrow {
  MoveNothrow() = default;
  MoveNothrow(MoveNothrow const &) noexcept(false) {}
  MoveNothrow(MoveNothrow &&) noexcept = default;
};

// Copacks whose alternatives include a non-builtin (Error/Xint/std::string_view/fn::pack — any
// class/struct/enum) have platform-specific order (see copack.cpp); pure-builtin copacks keep `copack<...>`.
} // namespace

TEST_CASE("graded monad", "[expected][copack][graded][and_then][or_else][copack_value][copack_error]")
{
  SECTION("unit")
  {
    constexpr fn::expected<void, fn::copack<>> unit{};
    static_assert(unit.has_value());

    SECTION("constexpr")
    {
      SECTION("and_then to value/copack<>")
      {
        constexpr auto fn = []() -> fn::expected<int, fn::copack<>> { return {7}; };
        constexpr auto a = unit.and_then(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<int, fn::copack<>> const>);
        static_assert(a.value() == 7);
      }

      SECTION("and_then to value")
      {
        constexpr auto fn = []() -> fn::expected<int, Error> { return {12}; };
        constexpr auto a = unit.and_then(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<int, fn::copack<Error>> const>);
        static_assert(a.value() == 12);
      }

      SECTION("and_then to error")
      {
        constexpr auto fn = []() -> fn::expected<int, Error> { return ::fn::unexpected<Error>(FileNotFound); };
        constexpr auto a = unit.and_then(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<int, fn::copack<Error>> const>);
        static_assert(a.error() == fn::copack{FileNotFound});
      }

      SECTION("transform to int")
      {
        constexpr auto fn = []() -> int { return 144'000; };
        constexpr auto a = unit.transform(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<int, fn::copack<>> const>);
        static_assert(a.value() == 144'000);
      }
    }

    SECTION("runtime")
    {
      SECTION("and_then to value/copack<>")
      {
        constexpr auto fn = []() -> fn::expected<int, fn::copack<>> { return {7}; };
        auto a = unit.and_then(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<int, fn::copack<>>>);
        CHECK(a.value() == 7);
      }

      SECTION("and_then to value")
      {
        constexpr auto fn = []() -> fn::expected<int, Error> { return {12}; };
        auto a = unit.and_then(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<int, fn::copack<Error>>>);
        CHECK(a.value() == 12);
      }

      SECTION("and_then to error")
      {
        constexpr auto fn = []() -> fn::expected<int, Error> { return ::fn::unexpected<Error>(FileNotFound); };
        auto a = unit.and_then(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<int, fn::copack<Error>>>);
        CHECK(a.error() == fn::copack{FileNotFound});
      }

      SECTION("transform to int")
      {
        constexpr auto fn = []() -> int { return 144'000; };
        auto a = unit.transform(fn);
        static_assert(std::is_same_v<decltype(a), fn::expected<int, fn::copack<>>>);
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
        static_assert(std::is_same_v<decltype(a), fn::expected<immovable_t, fn::copack<>>>);
        CHECK(a.value().v == 7);

        // the from-apply tag ctor backing this is not part of the public interface
        // (is_constructible_v cannot see private ctors)
        static_assert(
            not std::is_constructible_v<fn::expected<immovable_t, fn::copack<>>, pfn::detail::_expected_from_invoke_t,
                                        std::in_place_t, immovable_t (*)()>);
      }
    }
  }

  SECTION("copack_error from copack")
  {
    using T = fn::expected<int, fn::copack<Error>>;
    T s{12};
    static_assert(std::is_same_v<decltype(s.copack_error()), T &>);
    static_assert(std::is_same_v<decltype(std::as_const(s).copack_error()), T const &>);
    static_assert(std::is_same_v<decltype(std::move(std::as_const(s)).copack_error()), T const &&>);
    static_assert(std::is_same_v<decltype(std::move(s).copack_error()), T &&>);
    // these overloads only return *this
    static_assert(noexcept(s.copack_error()));
    static_assert(noexcept(std::as_const(s).copack_error()));
    static_assert(noexcept(std::move(std::as_const(s)).copack_error()));
    static_assert(noexcept(std::move(s).copack_error()));
    SECTION("value")
    {
      CHECK(s.copack_error().value() == 12);
      CHECK(std::as_const(s).copack_error().value() == 12);
      CHECK(std::move(std::as_const(s)).copack_error().value() == 12);
      CHECK(std::move(s).copack_error().value() == 12);
    }
    SECTION("error")
    {
      T s{::fn::unexpect, Unknown};
      CHECK(s.copack_error().error() == fn::copack{Unknown});
      CHECK(std::as_const(s).copack_error().error() == fn::copack{Unknown});
      CHECK(std::move(std::as_const(s)).copack_error().error() == fn::copack{Unknown});
      CHECK(std::move(s).copack_error().error() == fn::copack{Unknown});
    }

    static_assert(std::is_same_v<decltype(fn::copack_error(s)), T &>);
    static_assert(noexcept(fn::copack_error(s))); // the free function propagates what the member says
  }

  SECTION("copack_error from non-copack")
  {
    using T = fn::expected<int, Error>;
    T s{12};
    static_assert(std::is_same_v<decltype(s.copack_error()), fn::expected<int, fn::copack<Error>>>);
    static_assert(std::is_same_v<decltype(std::as_const(s).copack_error()), fn::expected<int, fn::copack<Error>>>);
    static_assert(
        std::is_same_v<decltype(std::move(std::as_const(s)).copack_error()), fn::expected<int, fn::copack<Error>>>);
    static_assert(std::is_same_v<decltype(std::move(s).copack_error()), fn::expected<int, fn::copack<Error>>>);
    // this overload wraps the error in a copack and relocates the value, so it weighs both - neither of
    // which can throw here
    static_assert(noexcept(s.copack_error()));
    static_assert(noexcept(std::as_const(s).copack_error()));
    static_assert(noexcept(std::move(std::as_const(s)).copack_error()));
    static_assert(noexcept(std::move(s).copack_error()));
    SECTION("value")
    {
      CHECK(s.copack_error().value() == 12);
      CHECK(std::as_const(s).copack_error().value() == 12);
      CHECK(std::move(std::as_const(s)).copack_error().value() == 12);
      CHECK(std::move(s).copack_error().value() == 12);
    }
    SECTION("error")
    {
      T s{::fn::unexpect, Unknown};
      CHECK(s.copack_error().error() == fn::copack{Unknown});
      CHECK(std::as_const(s).copack_error().error() == fn::copack{Unknown});
      CHECK(std::move(std::as_const(s)).copack_error().error() == fn::copack{Unknown});
      CHECK(std::move(s).copack_error().error() == fn::copack{Unknown});
    }

    static_assert(std::is_same_v<decltype(fn::copack_error(s)), fn::expected<int, fn::copack<Error>>>);
    static_assert(noexcept(fn::copack_error(s))); // the free function propagates what the member says

    SECTION("throwing value")
    {
      // the lift weighs the side it does not touch: relocating the value is what can throw here, so
      // the promise tracks the category that value is relocated by
      using W = fn::expected<std::string, Error>;
      static_assert(not noexcept(std::declval<W const &>().copack_error())); // copies
      static_assert(noexcept(std::declval<W &&>().copack_error()));          // moves
      SUCCEED();
    }
  }

  SECTION("copack_error from non-copack, void value")
  {
    using T = fn::expected<void, Error>;
    T s{};
    static_assert(std::is_same_v<decltype(s.copack_error()), fn::expected<void, fn::copack<Error>>>);
    static_assert(std::is_same_v<decltype(std::as_const(s).copack_error()), fn::expected<void, fn::copack<Error>>>);
    static_assert(
        std::is_same_v<decltype(std::move(std::as_const(s)).copack_error()), fn::expected<void, fn::copack<Error>>>);
    static_assert(std::is_same_v<decltype(std::move(s).copack_error()), fn::expected<void, fn::copack<Error>>>);
    // with a void value there is nothing to relocate, so the lift weighs only the error it wraps
    static_assert(noexcept(s.copack_error()));
    static_assert(noexcept(std::as_const(s).copack_error()));
    static_assert(noexcept(std::move(std::as_const(s)).copack_error()));
    static_assert(noexcept(std::move(s).copack_error()));
    SECTION("value")
    {
      CHECK(s.copack_error().has_value());
      CHECK(std::as_const(s).copack_error().has_value());
      CHECK(std::move(std::as_const(s)).copack_error().has_value());
      CHECK(std::move(s).copack_error().has_value());
    }
    SECTION("error")
    {
      T s{::fn::unexpect, Unknown};
      CHECK(s.copack_error().error() == fn::copack{Unknown});
      CHECK(std::as_const(s).copack_error().error() == fn::copack{Unknown});
      CHECK(std::move(std::as_const(s)).copack_error().error() == fn::copack{Unknown});
      CHECK(std::move(s).copack_error().error() == fn::copack{Unknown});
    }

    static_assert(std::is_same_v<decltype(fn::copack_error(s)), fn::expected<void, fn::copack<Error>>>);
    static_assert(noexcept(fn::copack_error(s))); // the free function propagates what the member says
  }

  SECTION("copack_error from copack, void value")
  {
    using T = fn::expected<void, fn::copack<Error>>;
    T s{};
    static_assert(std::is_same_v<decltype(s.copack_error()), T &>);
    static_assert(std::is_same_v<decltype(std::as_const(s).copack_error()), T const &>);
    static_assert(std::is_same_v<decltype(std::move(std::as_const(s)).copack_error()), T const &&>);
    static_assert(std::is_same_v<decltype(std::move(s).copack_error()), T &&>);
    // the void-value self-return overloads only return *this
    static_assert(noexcept(s.copack_error()));
    static_assert(noexcept(std::as_const(s).copack_error()));
    static_assert(noexcept(std::move(std::as_const(s)).copack_error()));
    static_assert(noexcept(std::move(s).copack_error()));
    SECTION("value")
    {
      CHECK(s.copack_error().has_value());
      CHECK(std::as_const(s).copack_error().has_value());
      CHECK(std::move(std::as_const(s)).copack_error().has_value());
      CHECK(std::move(s).copack_error().has_value());
    }
    SECTION("error")
    {
      T s{::fn::unexpect, Unknown};
      CHECK(s.copack_error().error() == fn::copack{Unknown});
      CHECK(std::as_const(s).copack_error().error() == fn::copack{Unknown});
      CHECK(std::move(std::as_const(s)).copack_error().error() == fn::copack{Unknown});
      CHECK(std::move(s).copack_error().error() == fn::copack{Unknown});
    }

    static_assert(std::is_same_v<decltype(fn::copack_error(s)), T &>);
    static_assert(noexcept(fn::copack_error(s))); // the free function propagates what the member says
  }

  SECTION("constexpr")
  {
    static_assert([] {
      fn::expected<int, Error> const a{::fn::unexpect, Unknown};
      return a.copack_error().error() == fn::copack{Unknown};
    }());
    static_assert([] { return fn::expected<int, Error>{12}.copack_error().value() == 12; }());
    static_assert([] { return fn::expected<int, Error>{12}.copack_value().value() == fn::copack{12}; }());
    static_assert([] {
      fn::expected<void, Error> const a{::fn::unexpect, Unknown};
      return a.copack_error().error() == fn::copack{Unknown};
    }());
    static_assert([] {
      fn::expected<int, Error> a{12};
      return fn::copack_value(a).value() == fn::copack{12};
    }());
    SUCCEED();
  }

  SECTION("copack_value from copack")
  {
    using T = fn::expected<fn::copack<int>, Error>;
    T s{12};
    static_assert(std::is_same_v<decltype(s.copack_value()), T &>);
    static_assert(std::is_same_v<decltype(std::as_const(s).copack_value()), T const &>);
    static_assert(std::is_same_v<decltype(std::move(std::as_const(s)).copack_value()), T const &&>);
    static_assert(std::is_same_v<decltype(std::move(s).copack_value()), T &&>);
    // these overloads only return *this
    static_assert(noexcept(s.copack_value()));
    static_assert(noexcept(std::as_const(s).copack_value()));
    static_assert(noexcept(std::move(std::as_const(s)).copack_value()));
    static_assert(noexcept(std::move(s).copack_value()));
    SECTION("value")
    {
      CHECK(s.copack_value().value() == fn::copack{12});
      CHECK(std::as_const(s).copack_value().value() == fn::copack{12});
      CHECK(std::move(std::as_const(s)).copack_value().value() == fn::copack{12});
      CHECK(std::move(s).copack_value().value() == fn::copack{12});
    }
    SECTION("error")
    {
      T s{::fn::unexpect, Unknown};
      CHECK(s.copack_value().error() == Unknown);
      CHECK(std::as_const(s).copack_value().error() == Unknown);
      CHECK(std::move(std::as_const(s)).copack_value().error() == Unknown);
      CHECK(std::move(s).copack_value().error() == Unknown);
    }

    static_assert(std::is_same_v<decltype(fn::copack_value(s)), T &>);
    static_assert(noexcept(fn::copack_value(s))); // the free function propagates what the member says
  }

  SECTION("copack_value from non-copack")
  {
    using T = fn::expected<int, Error>;
    T s{12};
    static_assert(std::is_same_v<decltype(s.copack_value()), fn::expected<fn::copack<int>, Error>>);
    static_assert(std::is_same_v<decltype(std::as_const(s).copack_value()), fn::expected<fn::copack<int>, Error>>);
    static_assert(
        std::is_same_v<decltype(std::move(std::as_const(s)).copack_value()), fn::expected<fn::copack<int>, Error>>);
    static_assert(std::is_same_v<decltype(std::move(s).copack_value()), fn::expected<fn::copack<int>, Error>>);
    // this overload wraps the value in a copack and relocates the error, so it weighs both - neither of
    // which can throw here
    static_assert(noexcept(s.copack_value()));
    static_assert(noexcept(std::as_const(s).copack_value()));
    static_assert(noexcept(std::move(std::as_const(s)).copack_value()));
    static_assert(noexcept(std::move(s).copack_value()));
    SECTION("value")
    {
      CHECK(s.copack_value().value() == fn::copack{12});
      CHECK(std::as_const(s).copack_value().value() == fn::copack{12});
      CHECK(std::move(std::as_const(s)).copack_value().value() == fn::copack{12});
      CHECK(std::move(s).copack_value().value() == fn::copack{12});
    }
    SECTION("error")
    {
      T s{::fn::unexpect, Unknown};
      CHECK(s.copack_value().error() == Unknown);
      CHECK(std::as_const(s).copack_value().error() == Unknown);
      CHECK(std::move(std::as_const(s)).copack_value().error() == Unknown);
      CHECK(std::move(s).copack_value().error() == Unknown);
    }

    static_assert(std::is_same_v<decltype(fn::copack_value(s)), fn::expected<fn::copack<int>, Error>>);
    static_assert(noexcept(fn::copack_value(s))); // the free function propagates what the member says
  }

  SECTION("copack_value absent for void value")
  {
    // by design: a void value cannot be copack-wrapped -- the void specialization has no
    // copack_value member and the free fn::copack_value is constrained some_expected_non_void
    // (include/fn/expected.hpp:1020); copack_error, by contrast, serves void (asserted above)
    constexpr auto can_member_copack_value = [](auto &&e) { return requires { e.copack_value(); }; };
    constexpr auto can_free_copack_value = [](auto &&e) { return requires { fn::copack_value(e); }; };
    static_assert(not can_member_copack_value(fn::expected<void, Error>{}));
    static_assert(not can_free_copack_value(fn::expected<void, Error>{}));
    static_assert(can_member_copack_value(fn::expected<int, Error>{1}));
    static_assert(can_free_copack_value(fn::expected<int, Error>{1}));
    SUCCEED();
  }

  SECTION("and_then")
  {
    SECTION("value to value")
    {
      fn::expected<int, fn::copack<Error>> s{12};

      constexpr auto fn1 = [](int) -> fn::expected<int, bool> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn1)), fn::expected<int, fn::copack_for<Error, bool>>>);
      constexpr auto fn2 = [](int) -> fn::expected<int, Error> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn2)), fn::expected<int, fn::copack<Error>>>);
      constexpr auto fn3 = [](int) -> fn::expected<int, fn::copack<Error>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn3)), fn::expected<int, fn::copack<Error>>>);
      constexpr auto fn4 = [](int) -> fn::expected<int, fn::copack<bool>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn4)), fn::expected<int, fn::copack_for<Error, bool>>>);
      constexpr auto fn5 = [](int) -> fn::expected<int, fn::copack_for<Error, bool>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn5)), fn::expected<int, fn::copack_for<Error, bool>>>);
      constexpr auto fn6 = [](int) -> fn::expected<int, fn::copack<bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn6)), fn::expected<int, fn::copack_for<Error, bool, int>>>);
      constexpr auto fn7 = [](int) -> fn::expected<int, fn::copack_for<Error, bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn7)), fn::expected<int, fn::copack_for<Error, bool, int>>>);
      constexpr auto fn8 = [](int) -> fn::expected<Xint, fn::copack_for<Error, bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn8)), fn::expected<Xint, fn::copack_for<Error, bool, int>>>);

      SECTION("value to value")
      {
        constexpr auto fn = [](int i) -> fn::expected<int, bool> { return {i + 12}; };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<int, fn::copack_for<Error, bool>>>);
        CHECK(s.and_then(fn).value() == 24);
        CHECK(std::as_const(s).and_then(fn).value() == 24);
        CHECK(std::move(std::as_const(s)).and_then(fn).value() == 24);
        CHECK(std::move(s).and_then(fn).value() == 24);
      }

      SECTION("value to error")
      {
        constexpr auto fn = [](int i) -> fn::expected<int, bool> { return ::fn::unexpected<bool>(i >= 1); };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<int, fn::copack_for<Error, bool>>>);
        CHECK(s.and_then(fn).error() == fn::copack{true});
        CHECK(std::as_const(s).and_then(fn).error() == fn::copack{true});
        CHECK(std::move(std::as_const(s)).and_then(fn).error() == fn::copack{true});
        CHECK(std::move(s).and_then(fn).error() == fn::copack{true});
      }

      SECTION("error")
      {
        fn::expected<int, fn::copack<Error>> s{::fn::unexpect, fn::copack{FileNotFound}};
        constexpr auto fn = [](int) -> fn::expected<int, bool> { throw 0; };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<int, fn::copack_for<Error, bool>>>);
        CHECK(s.and_then(fn).error() == fn::copack{FileNotFound});
        CHECK(s.and_then(fn).error() != fn::copack{false});
        CHECK(s.and_then(fn).error() != fn::copack{true});
        CHECK(std::as_const(s).and_then(fn).error() == fn::copack{FileNotFound});
        CHECK(std::move(std::as_const(s)).and_then(fn).error() == fn::copack{FileNotFound});
        CHECK(std::move(s).and_then(fn).error() == fn::copack{FileNotFound});
      }
    }

    SECTION("void to value")
    {
      fn::expected<void, fn::copack<Error>> s{};

      constexpr auto fn1 = []() -> fn::expected<int, bool> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn1)), fn::expected<int, fn::copack_for<Error, bool>>>);
      constexpr auto fn2 = []() -> fn::expected<int, Error> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn2)), fn::expected<int, fn::copack<Error>>>);
      constexpr auto fn3 = []() -> fn::expected<int, fn::copack<Error>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn3)), fn::expected<int, fn::copack<Error>>>);
      constexpr auto fn4 = []() -> fn::expected<int, fn::copack<bool>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn4)), fn::expected<int, fn::copack_for<Error, bool>>>);
      constexpr auto fn5 = []() -> fn::expected<int, fn::copack_for<Error, bool>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn5)), fn::expected<int, fn::copack_for<Error, bool>>>);
      constexpr auto fn6 = []() -> fn::expected<int, fn::copack<bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn6)), fn::expected<int, fn::copack_for<Error, bool, int>>>);
      constexpr auto fn7 = []() -> fn::expected<int, fn::copack_for<Error, bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn7)), fn::expected<int, fn::copack_for<Error, bool, int>>>);

      SECTION("value to value")
      {
        constexpr auto fn = []() -> fn::expected<int, bool> { return {12}; };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<int, fn::copack_for<Error, bool>>>);
        CHECK(s.and_then(fn).value() == 12);
        CHECK(std::as_const(s).and_then(fn).value() == 12);
        CHECK(std::move(std::as_const(s)).and_then(fn).value() == 12);
        CHECK(std::move(s).and_then(fn).value() == 12);
      }

      SECTION("value to error")
      {
        constexpr auto fn = []() -> fn::expected<int, bool> { return ::fn::unexpected<bool>(true); };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<int, fn::copack_for<Error, bool>>>);
        CHECK(s.and_then(fn).error() == fn::copack{true});
        CHECK(std::as_const(s).and_then(fn).error() == fn::copack{true});
        CHECK(std::move(std::as_const(s)).and_then(fn).error() == fn::copack{true});
        CHECK(std::move(s).and_then(fn).error() == fn::copack{true});
      }

      SECTION("error")
      {
        fn::expected<void, fn::copack<Error>> s{::fn::unexpect, fn::copack{FileNotFound}};
        constexpr auto fn = []() -> fn::expected<int, bool> { throw 0; };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<int, fn::copack_for<Error, bool>>>);
        CHECK(s.and_then(fn).error() == fn::copack{FileNotFound});
        CHECK(s.and_then(fn).error() != fn::copack{false});
        CHECK(s.and_then(fn).error() != fn::copack{true});
        CHECK(std::as_const(s).and_then(fn).error() == fn::copack{FileNotFound});
        CHECK(std::move(std::as_const(s)).and_then(fn).error() == fn::copack{FileNotFound});
        CHECK(std::move(s).and_then(fn).error() == fn::copack{FileNotFound});
      }
    }

    SECTION("value to void")
    {
      fn::expected<int, fn::copack<Error>> s{12};

      constexpr auto fn1 = [](int) -> fn::expected<void, bool> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn1)), fn::expected<void, fn::copack_for<Error, bool>>>);
      constexpr auto fn2 = [](int) -> fn::expected<void, Error> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn2)), fn::expected<void, fn::copack<Error>>>);
      constexpr auto fn3 = [](int) -> fn::expected<void, fn::copack<Error>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn3)), fn::expected<void, fn::copack<Error>>>);
      constexpr auto fn4 = [](int) -> fn::expected<void, fn::copack<bool>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn4)), fn::expected<void, fn::copack_for<Error, bool>>>);
      constexpr auto fn5 = [](int) -> fn::expected<void, fn::copack_for<Error, bool>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn5)), fn::expected<void, fn::copack_for<Error, bool>>>);
      constexpr auto fn6 = [](int) -> fn::expected<void, fn::copack<bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn6)), fn::expected<void, fn::copack_for<Error, bool, int>>>);
      constexpr auto fn7 = [](int) -> fn::expected<void, fn::copack_for<Error, bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn7)), fn::expected<void, fn::copack_for<Error, bool, int>>>);

      SECTION("value to value")
      {
        constexpr auto fn = [](int) -> fn::expected<void, bool> { return {}; };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<void, fn::copack_for<Error, bool>>>);
        CHECK(s.and_then(fn).has_value());
        CHECK(std::as_const(s).and_then(fn).has_value());
        CHECK(std::move(std::as_const(s)).and_then(fn).has_value());
        CHECK(std::move(s).and_then(fn).has_value());
      }

      SECTION("value to error")
      {
        constexpr auto fn = [](int i) -> fn::expected<void, bool> { return ::fn::unexpected<bool>(i >= 1); };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<void, fn::copack_for<Error, bool>>>);
        CHECK(s.and_then(fn).error() == fn::copack{true});
        CHECK(std::as_const(s).and_then(fn).error() == fn::copack{true});
        CHECK(std::move(std::as_const(s)).and_then(fn).error() == fn::copack{true});
        CHECK(std::move(s).and_then(fn).error() == fn::copack{true});
      }

      SECTION("error")
      {
        fn::expected<int, fn::copack<Error>> s{::fn::unexpect, fn::copack{FileNotFound}};
        constexpr auto fn = [](int) -> fn::expected<void, bool> { throw 0; };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<void, fn::copack_for<Error, bool>>>);
        CHECK(s.and_then(fn).error() == fn::copack{FileNotFound});
        CHECK(s.and_then(fn).error() != fn::copack{false});
        CHECK(s.and_then(fn).error() != fn::copack{true});
        CHECK(std::as_const(s).and_then(fn).error() == fn::copack{FileNotFound});
        CHECK(std::move(std::as_const(s)).and_then(fn).error() == fn::copack{FileNotFound});
        CHECK(std::move(s).and_then(fn).error() == fn::copack{FileNotFound});
      }
    }

    SECTION("void to void")
    {
      fn::expected<void, fn::copack<Error>> s{};

      constexpr auto fn1 = []() -> fn::expected<void, bool> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn1)), fn::expected<void, fn::copack_for<Error, bool>>>);
      constexpr auto fn2 = []() -> fn::expected<void, Error> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn2)), fn::expected<void, fn::copack<Error>>>);
      constexpr auto fn3 = []() -> fn::expected<void, fn::copack<Error>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn3)), fn::expected<void, fn::copack<Error>>>);
      constexpr auto fn4 = []() -> fn::expected<void, fn::copack<bool>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn4)), fn::expected<void, fn::copack_for<Error, bool>>>);
      constexpr auto fn5 = []() -> fn::expected<void, fn::copack_for<Error, bool>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn5)), fn::expected<void, fn::copack_for<Error, bool>>>);
      constexpr auto fn6 = []() -> fn::expected<void, fn::copack<bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn6)), fn::expected<void, fn::copack_for<Error, bool, int>>>);
      constexpr auto fn7 = []() -> fn::expected<void, fn::copack_for<Error, bool, int>> { throw 0; };
      static_assert(std::is_same_v<decltype(s.and_then(fn7)), fn::expected<void, fn::copack_for<Error, bool, int>>>);

      SECTION("value to value")
      {
        constexpr auto fn = []() -> fn::expected<void, bool> { return {}; };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<void, fn::copack_for<Error, bool>>>);
        CHECK(s.and_then(fn).has_value());
        CHECK(std::as_const(s).and_then(fn).has_value());
        CHECK(std::move(std::as_const(s)).and_then(fn).has_value());
        CHECK(std::move(s).and_then(fn).has_value());
      }

      SECTION("value to error")
      {
        constexpr auto fn = []() -> fn::expected<void, bool> { return ::fn::unexpected<bool>(true); };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<void, fn::copack_for<Error, bool>>>);
        CHECK(s.and_then(fn).error() == fn::copack{true});
        CHECK(std::as_const(s).and_then(fn).error() == fn::copack{true});
        CHECK(std::move(std::as_const(s)).and_then(fn).error() == fn::copack{true});
        CHECK(std::move(s).and_then(fn).error() == fn::copack{true});
      }

      SECTION("error")
      {
        fn::expected<void, fn::copack<Error>> s{::fn::unexpect, fn::copack{FileNotFound}};
        constexpr auto fn = []() -> fn::expected<void, bool> { throw 0; };
        static_assert(std::is_same_v<decltype(s.and_then(fn)), fn::expected<void, fn::copack_for<Error, bool>>>);
        CHECK(s.and_then(fn).error() == fn::copack{FileNotFound});
        CHECK(s.and_then(fn).error() != fn::copack{false});
        CHECK(s.and_then(fn).error() != fn::copack{true});
        CHECK(std::as_const(s).and_then(fn).error() == fn::copack{FileNotFound});
        CHECK(std::move(std::as_const(s)).and_then(fn).error() == fn::copack{FileNotFound});
        CHECK(std::move(s).and_then(fn).error() == fn::copack{FileNotFound});
      }
    }
  }

  SECTION("or_else")
  {
    fn::expected<fn::copack<int>, Error> s{::fn::unexpect, FileNotFound};

    constexpr auto fn1 = [](int) -> fn::expected<Xint, Error> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn1)), fn::expected<fn::copack_for<Xint, int>, Error>>);
    constexpr auto fn2 = [](int) -> fn::expected<int, Error> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn2)), fn::expected<fn::copack<int>, Error>>);
    constexpr auto fn3 = [](int) -> fn::expected<fn::copack<int>, Error> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn3)), fn::expected<fn::copack<int>, Error>>);
    constexpr auto fn4 = [](int) -> fn::expected<fn::copack<Xint>, Error> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn4)), fn::expected<fn::copack_for<Xint, int>, Error>>);
    constexpr auto fn5 = [](int) -> fn::expected<fn::copack_for<Xint, int>, Error> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn5)), fn::expected<fn::copack_for<Xint, int>, Error>>);
    constexpr auto fn6 = [](int) -> fn::expected<fn::copack_for<Xint, long>, Error> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn6)), fn::expected<fn::copack_for<Xint, int, long>, Error>>);
    constexpr auto fn7 = [](int) -> fn::expected<fn::copack_for<Xint, int, long>, Error> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn7)), fn::expected<fn::copack_for<Xint, int, long>, Error>>);
    constexpr auto fn8 = [](int) -> fn::expected<fn::copack_for<Xint, int, long>, std::string> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn8)), fn::expected<fn::copack_for<Xint, int, long>, std::string>>);

    SECTION("error to value")
    {
      constexpr auto fn = [](Error) -> fn::expected<Xint, std::string> { return {Xint{12}}; };
      static_assert(std::is_same_v<decltype(s.or_else(fn)), fn::expected<fn::copack_for<Xint, int>, std::string>>);
      CHECK(s.or_else(fn).value() == fn::copack{Xint{12}});
      CHECK(std::as_const(s).or_else(fn).value() == fn::copack{Xint{12}});
      CHECK(std::move(std::as_const(s)).or_else(fn).value() == fn::copack{Xint{12}});
      CHECK(std::move(s).or_else(fn).value() == fn::copack{Xint{12}});
    }

    SECTION("error to error")
    {
      constexpr auto fn = [](Error) -> fn::expected<Xint, std::string> { return ::fn::unexpected<std::string>("Boo"); };
      static_assert(std::is_same_v<decltype(s.or_else(fn)), fn::expected<fn::copack_for<Xint, int>, std::string>>);
      CHECK(s.or_else(fn).error() == "Boo");
      CHECK(std::as_const(s).or_else(fn).error() == "Boo");
      CHECK(std::move(std::as_const(s)).or_else(fn).error() == "Boo");
      CHECK(std::move(s).or_else(fn).error() == "Boo");
    }

    SECTION("value")
    {
      fn::expected<fn::copack<int>, Error> s{fn::copack{12}};
      constexpr auto fn = [](int) -> fn::expected<Xint, std::string> { throw 0; };
      static_assert(std::is_same_v<decltype(s.or_else(fn)), fn::expected<fn::copack_for<Xint, int>, std::string>>);
      CHECK(s.or_else(fn).value() == fn::copack{12});
      CHECK(std::as_const(s).or_else(fn).value() == fn::copack{12});
      CHECK(std::move(std::as_const(s)).or_else(fn).value() == fn::copack{12});
      CHECK(std::move(s).or_else(fn).value() == fn::copack{12});
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

  SECTION("assignment")
  {
    // a graded monad is assignable because its co-product error is: without copack::operator= the whole
    // expected<T, copack<...>> would not be assignable at all
    using T = fn::expected<int, fn::copack_for<Error, bool>>; // Error is an enum: the order is per-platform
    static_assert(std::is_copy_assignable_v<T>);
    static_assert(std::is_move_assignable_v<T>);

    T a{12};
    T const b{::fn::unexpect, fn::copack{FileNotFound}};
    a = b;
    CHECK(a.error() == fn::copack{FileNotFound});
    a = T{42};
    CHECK(a.value() == 42);

    static_assert([] {
      T a{12};
      a = T{::fn::unexpect, fn::copack{true}};
      return a.error() == fn::copack{true};
    }());
  }

  SECTION("noexcept")
  {
    // the widening arms relocate BOTH sides into the co-product result - self's, and the callback's -
    // and weigh both, rather than reporting potentially-throwing merely because the shape changed
    SECTION("and_then widens the error")
    {
      using T = fn::expected<int, fn::copack<Error>>;
      constexpr auto widen = [](int) noexcept -> fn::expected<bool, bool> { return {true}; };
      static_assert(std::is_same_v<decltype(std::declval<T &>().and_then(widen)),
                                   fn::expected<bool, fn::copack_for<Error, bool>>>);
      static_assert(noexcept(std::declval<T &>().and_then(widen)));
      static_assert(not noexcept(std::declval<T &>().and_then([](int) -> fn::expected<bool, bool> { return {true}; })));

      using W = fn::expected<int, fn::copack_for<MoveNothrow, Error>>;
      static_assert(not noexcept(std::declval<W &>().and_then(widen))); // copies self's error
      static_assert(noexcept(std::declval<W &&>().and_then(widen)));    // moves it
    }

    SECTION("or_else widens the value")
    {
      using T = fn::expected<fn::copack<int>, Error>;
      constexpr auto widen = [](Error) noexcept -> fn::expected<bool, bool> { return {true}; };
      static_assert(noexcept(std::declval<T &>().or_else(widen)));
      static_assert(
          not noexcept(std::declval<T &>().or_else([](Error) -> fn::expected<bool, bool> { return {true}; })));

      using W = fn::expected<fn::copack_for<MoveNothrow, int>, Error>;
      static_assert(not noexcept(std::declval<W &>().or_else(widen))); // copies self's value
      static_assert(noexcept(std::declval<W &&>().or_else(widen)));    // moves it
    }

    SECTION("the unit error grade")
    {
      // copack<> can hold no error, so the arm lifting self's error is unreachable, and cannot throw
      using T = fn::expected<int, fn::copack<>>;
      static_assert(
          noexcept(std::declval<T &>().and_then([](int) noexcept -> fn::expected<bool, Error> { return {true}; })));
    }
    SUCCEED();
  }
}

TEST_CASE("graded monad constexpr and runtime", "[constexpr][and_then][or_else][expected][graded][copack]")
{
  enum class Error : int { Unknown, InvalidValue };
  using T = fn::expected<int, fn::copack<Error>>;

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
      static_assert(std::is_same_v<decltype(r1), fn::expected<int, fn::copack_for<Error, int>> const>);
      static_assert(r1.value() == 1);
      constexpr auto r2 = r1.and_then(fn1);
      static_assert(r2.value() == 2);
      constexpr auto r3 = r2.and_then(fn1);
      static_assert(r3.error() == fn::copack{2});
      constexpr auto r4 = r3.and_then(fn1);
      static_assert(r4.error() == fn::copack{2});

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
      static_assert(std::is_same_v<decltype(r2), fn::expected<bool, fn::copack<Error>> const>);
      static_assert(r2.value());

      constexpr auto r3 = T{2}.and_then(fn2);
      static_assert(std::is_same_v<decltype(r3), fn::expected<bool, fn::copack<Error>> const>);
      static_assert(r3.error() == fn::copack{Error::InvalidValue});

      constexpr auto fn3 = [](int i) -> fn::expected<int, int> { return {i + 1}; };
      constexpr auto r4 = r3.and_then(fn3);
      static_assert(std::is_same_v<decltype(r4), fn::expected<int, fn::copack_for<Error, int>> const>);
      static_assert(r4.error() == fn::copack{Error::InvalidValue});

      constexpr auto r5 = T{2}.and_then(fn3);
      static_assert(std::is_same_v<decltype(r5), fn::expected<int, fn::copack_for<Error, int>> const>);
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
      static_assert(std::is_same_v<decltype(r1), fn::expected<int, fn::copack_for<Error, int>> const>);
      CHECK(r1.value() == 1);
      auto const r2 = r1.and_then(fn1);
      CHECK(r2.value() == 2);
      auto const r3 = r2.and_then(fn1);
      CHECK(r3.error() == fn::copack{2});
      auto const r4 = r3.and_then(fn1);
      CHECK(r4.error() == fn::copack{2});
    }

    SECTION("accummulate errors")
    {
      constexpr auto fn2 = [](int i) -> fn::expected<bool, Error> {
        if (i < 0 || i > 1)
          return ::fn::unexpected<Error>{Error::InvalidValue};
        return {i == 1};
      };

      auto const r2 = T{1}.and_then(fn2);
      static_assert(std::is_same_v<decltype(r2), fn::expected<bool, fn::copack<Error>> const>);
      CHECK(r2.value());
      auto const r3 = T{2}.and_then(fn2);
      CHECK(r3.error() == fn::copack{Error::InvalidValue});

      auto const fn3 = [](int i) -> fn::expected<int, int> { return {i + 1}; };
      auto const r4 = r3.and_then(fn3);
      static_assert(std::is_same_v<decltype(r4), fn::expected<int, fn::copack_for<Error, int>> const>);
      CHECK(r4.error() == fn::copack{Error::InvalidValue});
      auto const r5 = T{2}.and_then(fn3);
      CHECK(r5.value() == 3);
    }
  }

  SECTION("or_else constexpr")
  {
    using T = fn::expected<fn::copack<int>, Error>;

    constexpr auto fn1 = [](Error i) -> fn::expected<int, int> {
      if (i == Error::Unknown)
        return {0};
      return ::fn::unexpected<int>{(int)i};
    };

    constexpr auto r1 = T{14}.or_else(fn1);
    static_assert(std::is_same_v<decltype(r1), fn::expected<fn::copack<int>, int> const>);
    static_assert(r1.value() == fn::copack{14});
    constexpr auto r2 = T{::fn::unexpect, Error::InvalidValue}.or_else(fn1);
    static_assert(r2.error() == 1);
    constexpr auto r3 = T{::fn::unexpect, Error::Unknown}.or_else(fn1);
    static_assert(r3.value() == fn::copack{0});

    SUCCEED();
  }

  SECTION("or_else runtime")
  {
    using T = fn::expected<fn::copack<int>, Error>;

    constexpr auto fn1 = [](Error i) -> fn::expected<int, int> {
      if (i == Error::Unknown)
        return {0};
      return ::fn::unexpected<int>{(int)i};
    };

    auto const r1 = T{14}.or_else(fn1);
    static_assert(std::is_same_v<decltype(r1), fn::expected<fn::copack<int>, int> const>);
    CHECK(r1.value() == fn::copack{14});
    auto const r2 = T{::fn::unexpect, Error::InvalidValue}.or_else(fn1);
    CHECK(r2.error() == 1);
    auto const r3 = T{::fn::unexpect, Error::Unknown}.or_else(fn1);
    CHECK(r3.value() == fn::copack{0});
  }
}

TEST_CASE("expected pack support", "[expected][pack][and_then][transform][operator_and][graded][copack]")
{
  SECTION("and_then")
  {
    using S = fn::expected<fn::pack<int, std::string_view>, Error>;

    // noexcept (extension): the spec asks fn's own nothrow-applicable trait, which asks the
    // pack-apply dispatch that will actually run - one argument per element
    constexpr auto nothrow_two = [](int &, std::string_view &) noexcept -> fn::expected<bool, Error> { return {true}; };
    static_assert(noexcept(std::declval<S &>().and_then(nothrow_two)));
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

    SECTION("noexcept")
    {
      // a pack's callback is invoked through fn's own dispatch, taking one argument per element: it
      // is not directly applicable on the pack, so only fn's nothrow-applicable trait can answer
      using T = fn::expected<fn::pack<int, double>, Error>;
      static_assert(noexcept(
          std::declval<T &>().and_then([](int, double) noexcept -> fn::expected<bool, Error> { return {true}; })));
      static_assert(
          not noexcept(std::declval<T &>().and_then([](int, double) -> fn::expected<bool, Error> { return {true}; })));
      SUCCEED();
    }
  }

  SECTION("transform")
  {
    using S = fn::expected<fn::pack<int, std::string_view>, Error>;

    // noexcept and constraints mirror and_then above (expected.hpp:216-220): the non-copack
    // _transform is constrained on pack-apply invocability AND the untouched error's copy --
    // contrast the copack case (see "expected copack support transform")
    constexpr auto nothrow_two = [](int &, std::string_view &) noexcept -> bool { return true; };
    static_assert(noexcept(std::declval<S &>().transform(nothrow_two)));
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

    SECTION("noexcept")
    {
      using T = fn::expected<fn::pack<int, double>, Error>;
      static_assert(noexcept(std::declval<T &>().transform([](int, double) noexcept -> bool { return true; })));
      static_assert(not noexcept(std::declval<T &>().transform([](int, double) -> bool { return true; })));
      SUCCEED();
    }
  }

  SECTION("operator &")
  {
    // noexcept: the join relocates both operands' values and errors into the result, and promises
    // only what relocating them promises - so a throwing-copy value type makes the lvalue join
    // throwing.
    struct throwing_copy {
      // defined, not just declared: the instantiated join references it (-Wundefined-internal)
      throwing_copy(throwing_copy const &) noexcept(false) {}
    };
    static_assert(noexcept(std::declval<fn::expected<int, Error> &>() & std::declval<fn::expected<void, Error> &>()));
    static_assert(not noexcept(std::declval<fn::expected<throwing_copy, Error> &>()
                               & std::declval<fn::expected<int, Error> &>()));

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

      SECTION("copack on both sides")
      {
        using Lh = fn::expected<fn::copack<double, int>, Error>;
        using Rh = fn::expected<fn::copack<bool, int>, Error>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::copack< //
                                                    fn::pack<double, bool>, fn::pack<double, int>, fn::pack<int, bool>,
                                                    fn::pack<int, int>>,
                                                Error>>);

        CHECK((Lh{fn::copack{0.5}} & Rh{fn::copack{12}})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::copack{true});
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{fn::copack{12}}).error() == FileNotFound);
        CHECK((Lh{fn::copack{0.5}} & Rh{::fn::unexpect, Unknown}).error() == Unknown);
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, Unknown}).error() == FileNotFound);

        SECTION("copack of packs on left")
        {
          using Lh = fn::expected<fn::copack_for<fn::pack<double, bool>, fn::pack<double, int>>, Error>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::copack< //
                                                      fn::pack<double, bool, bool>, fn::pack<double, bool, int>,
                                                      fn::pack<double, int, bool>, fn::pack<double, int, int>>,
                                                  Error>>);

          CHECK((Lh{fn::copack{fn::pack{0.5, 3}}} & Rh{fn::copack{12}})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                    })
                    .value()
                == fn::copack{true});
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{fn::copack{12}}).error() == FileNotFound);
          CHECK((Lh{fn::copack{fn::pack{0.5, 3}}} & Rh{::fn::unexpect, Unknown}).error() == Unknown);
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, Unknown}).error() == FileNotFound);
        }

        SECTION("copack of packs on right")
        {
          using Rh = fn::expected<fn::copack_for<fn::pack<double, bool>, fn::pack<double, int>>, Error>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::copack< //
                                                      fn::pack<double, double, bool>, fn::pack<double, double, int>,
                                                      fn::pack<int, double, bool>, fn::pack<int, double, int>>,
                                                  Error>>);

          CHECK((Lh{fn::copack{12}} & Rh{fn::copack{fn::pack{0.5, 3}}})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 12 == static_cast<int>(i) && 0.5 == static_cast<double>(j) && 3 == static_cast<int>(k);
                    })
                    .value()
                == fn::copack{true});
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{fn::copack{fn::pack{0.5, 3}}}).error() == FileNotFound);
          CHECK((Lh{fn::copack{12}} & Rh{::fn::unexpect, Unknown}).error() == Unknown);
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, Unknown}).error() == FileNotFound);
        }
      }

      SECTION("copack on left side only")
      {
        using Lh = fn::expected<fn::copack<double, int>, Error>;
        using Rh = fn::expected<int, Error>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::copack< //
                                                    fn::pack<double, int>, fn::pack<int, int>>,
                                                Error>>);

        CHECK((Lh{fn::copack{0.5}} & Rh{12})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::copack{true});
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{12}).error() == FileNotFound);
        CHECK((Lh{fn::copack{0.5}} & Rh{::fn::unexpect, Unknown}).error() == Unknown);
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, Unknown}).error() == FileNotFound);

        SECTION("copack of packs on left")
        {
          using Lh = fn::expected<fn::copack_for<fn::pack<double, bool>, fn::pack<double, int>>, Error>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::copack< //
                                                      fn::pack<double, bool, int>, fn::pack<double, int, int>>,
                                                  Error>>);

          CHECK((Lh{fn::copack{fn::pack{0.5, 3}}} & Rh{12})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                    })
                    .value()
                == fn::copack{true});
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{12}).error() == FileNotFound);
          CHECK((Lh{fn::copack{fn::pack{0.5, 3}}} & Rh{::fn::unexpect, Unknown}).error() == Unknown);
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, Unknown}).error() == FileNotFound);
        }

        SECTION("pack on right")
        {
          using Rh = fn::expected<fn::pack<double, bool>, Error>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::copack< //
                                                      fn::pack<double, double, bool>, fn::pack<int, double, bool>>,
                                                  Error>>);

          CHECK((Lh{fn::copack{1.5}} & Rh{std::in_place, fn::pack{0.5, true}})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 1.5 == static_cast<double>(i) && 0.5 == static_cast<double>(j) && static_cast<bool>(k);
                    })
                    .value()
                == fn::copack{true});
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{std::in_place, fn::pack{0.5, true}}).error() == FileNotFound);
          CHECK((Lh{fn::copack{1.5}} & Rh{::fn::unexpect, Unknown}).error() == Unknown);
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, Unknown}).error() == FileNotFound);
        }
      }

      SECTION("copack on right side only")
      {
        using Lh = fn::expected<double, Error>;
        using Rh = fn::expected<fn::copack<bool, int>, Error>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::copack< //
                                                    fn::pack<double, bool>, fn::pack<double, int>>,
                                                Error>>);

        CHECK((Lh{0.5} & Rh{fn::copack{12}})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::copack{true});
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{fn::copack{12}}).error() == FileNotFound);
        CHECK((Lh{0.5} & Rh{::fn::unexpect, Unknown}).error() == Unknown);
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, Unknown}).error() == FileNotFound);

        SECTION("pack on left")
        {
          using Lh = fn::expected<fn::pack<double, int>, Error>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::copack< //
                                                      fn::pack<double, int, bool>, fn::pack<double, int, int>>,
                                                  Error>>);

          CHECK((Lh{fn::pack{0.5, 3}} & Rh{fn::copack{12}})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                    })
                    .value()
                == fn::copack{true});
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{fn::copack{12}}).error() == FileNotFound);
          CHECK((Lh{fn::pack{0.5, 3}} & Rh{::fn::unexpect, Unknown}).error() == Unknown);
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, Unknown}).error() == FileNotFound);
        }
      }
    }

    SECTION("graded monad as left operand")
    {
      static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::copack<Error>>>()
                                          & std::declval<fn::expected<void, Error>>()),
                                 fn::expected<int, fn::copack<Error>>>);

      static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::copack<Error>>>()
                                          & std::declval<fn::expected<void, fn::copack<Error>>>()),
                                 fn::expected<int, fn::copack<Error>>>);

      static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::copack<Error>>>()
                                          & std::declval<fn::expected<void, fn::copack<int>>>()),
                                 fn::expected<int, fn::copack_for<Error, int>>>);

      static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::copack<Error>>>()
                                          & std::declval<fn::expected<void, fn::copack<bool, int>>>()),
                                 fn::expected<int, fn::copack_for<Error, bool, int>>>);

      static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::copack<bool, int>>>()
                                          & std::declval<fn::expected<void, fn::copack<Error>>>()),
                                 fn::expected<int, fn::copack_for<Error, bool, int>>>);

      SECTION("value & void yield value")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::copack<Error>>>()
                                            & std::declval<fn::expected<void, int>>()),
                                   fn::expected<int, fn::copack_for<Error, int>>>);

        CHECK((fn::expected<int, fn::copack<Error>>{42} //
               & fn::expected<void, int>{})
                  .value()
              == 42);
        CHECK((fn::expected<int, fn::copack<Error>>{::fn::unexpect, fn::copack{FileNotFound}} //
               & fn::expected<void, int>{})
                  .error()
              == fn::copack{FileNotFound});
        CHECK((fn::expected<int, fn::copack<Error>>{42} //
               & fn::expected<void, int>{::fn::unexpect, 13})
                  .error()
              == fn::copack{13});
        CHECK((fn::expected<int, fn::copack<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<void, int>{::fn::unexpect, 13})
                  .error()
              == fn::copack{FileNotFound});
      }

      SECTION("void & value yield value")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<void, fn::copack<Error>>>()
                                            & std::declval<fn::expected<int, int>>()),
                                   fn::expected<int, fn::copack_for<Error, int>>>);

        CHECK((fn::expected<void, fn::copack<Error>>{} //
               & fn::expected<int, int>{12})
                  .value()
              == 12);
        CHECK((fn::expected<void, fn::copack<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<int, int>{12})
                  .error()
              == fn::copack{FileNotFound});
        CHECK((fn::expected<void, fn::copack<Error>>{} //
               & fn::expected<int, int>{::fn::unexpect, 13})
                  .error()
              == fn::copack{13});
        CHECK((fn::expected<void, fn::copack<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<int, int>{::fn::unexpect, 13})
                  .error()
              == fn::copack{FileNotFound});
      }

      SECTION("void & void yield void")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<void, fn::copack<Error>>>()
                                            & std::declval<fn::expected<void, int>>()),
                                   fn::expected<void, fn::copack_for<Error, int>>>);

        CHECK((fn::expected<void, fn::copack<Error>>{} //
               & fn::expected<void, int>{})
                  .has_value());
        CHECK((fn::expected<void, fn::copack<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<void, int>{})
                  .error()
              == fn::copack{FileNotFound});
        CHECK((fn::expected<void, fn::copack<Error>>{} //
               & fn::expected<void, int>{::fn::unexpect, 13})
                  .error()
              == fn::copack{13});
        CHECK((fn::expected<void, fn::copack<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<void, int>{::fn::unexpect, 13})
                  .error()
              == fn::copack{FileNotFound});
      }

      SECTION("value & value yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::copack<Error>>>()
                                            & std::declval<fn::expected<double, int>>()),
                                   fn::expected<fn::pack<int, double>, fn::copack_for<Error, int>>>);

        CHECK((fn::expected<double, fn::copack<Error>>{0.5} //
               & fn::expected<int, int>{12})
                  .transform([](double d, int i) constexpr -> bool { return d == 0.5 && i == 12; })
                  .value());
        CHECK((fn::expected<double, fn::copack<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<int, int>{12})
                  .error()
              == fn::copack{FileNotFound});
        CHECK((fn::expected<double, fn::copack<Error>>{} //
               & fn::expected<int, int>{::fn::unexpect, 13})
                  .error()
              == fn::copack{13});
        CHECK((fn::expected<double, fn::copack<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<int, int>{::fn::unexpect, 13})
                  .error()
              == fn::copack{FileNotFound});
      }

      SECTION("pack & value yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<fn::pack<double, bool>, fn::copack<Error>>>()
                                            & std::declval<fn::expected<int, int>>()),
                                   fn::expected<fn::pack<double, bool, int>, fn::copack_for<Error, int>>>);

        CHECK((fn::expected<fn::pack<double, bool>, fn::copack<Error>>{std::in_place, fn::pack<double, bool>{0.5, true}}
               //
               & fn::expected<int, int>{12})
                  .transform([](double d, bool b, int i) constexpr -> bool { return d == 0.5 && b && i == 12; })
                  .value());
        CHECK((fn::expected<fn::pack<double, bool>, fn::copack<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<int, int>{12})
                  .error()
              == fn::copack{FileNotFound});
        CHECK((fn::expected<fn::pack<double, bool>, fn::copack<Error>>{std::in_place, fn::pack<double, bool>{0.5, true}}
               //
               & fn::expected<int, int>{::fn::unexpect, 13})
                  .error()
              == fn::copack{13});
        CHECK((fn::expected<fn::pack<double, bool>, fn::copack<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<int, int>{::fn::unexpect, 13})
                  .error()
              == fn::copack{FileNotFound});
      }

      SECTION("pack & void yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<fn::pack<double, bool>, fn::copack<Error>>>()
                                            & std::declval<fn::expected<void, int>>()),
                                   fn::expected<fn::pack<double, bool>, fn::copack_for<Error, int>>>);

        CHECK((fn::expected<fn::pack<double, bool>, fn::copack<Error>>{std::in_place, fn::pack<double, bool>{0.5, true}}
               //
               & fn::expected<void, int>{})
                  .transform([](double d, bool b) constexpr -> bool { return d == 0.5 && b; })
                  .value());
        CHECK((fn::expected<fn::pack<double, bool>, fn::copack<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<void, int>{})
                  .error()
              == fn::copack{FileNotFound});
        CHECK((fn::expected<fn::pack<double, bool>, fn::copack<Error>>{std::in_place, fn::pack<double, bool>{0.5, true}}
               //
               & fn::expected<void, int>{::fn::unexpect, 13})
                  .error()
              == fn::copack{13});
        CHECK((fn::expected<fn::pack<double, bool>, fn::copack<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<void, int>{::fn::unexpect, 13})
                  .error()
              == fn::copack{FileNotFound});
      }

      SECTION("void & pack yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<void, fn::copack<Error>>>()
                                            & std::declval<fn::expected<fn::pack<double, bool>, int>>()),
                                   fn::expected<fn::pack<double, bool>, fn::copack_for<Error, int>>>);

        CHECK((fn::expected<void, fn::copack<Error>>{} //
               & fn::expected<fn::pack<double, bool>, int>{std::in_place, fn::pack<double, bool>{0.5, true}})
                  .transform([](double d, bool b) constexpr -> bool { return d == 0.5 && b; })
                  .value());
        CHECK((fn::expected<void, fn::copack<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<fn::pack<double, bool>, int>{std::in_place, fn::pack<double, bool>{0.5, true}})
                  .error()
              == fn::copack{FileNotFound});
        CHECK((fn::expected<void, fn::copack<Error>>{} //
               & fn::expected<fn::pack<double, bool>, int>{::fn::unexpect, 13})
                  .error()
              == fn::copack{13});
        CHECK((fn::expected<void, fn::copack<Error>>{::fn::unexpect, FileNotFound} //
               & fn::expected<fn::pack<double, bool>, int>{::fn::unexpect, 13})
                  .error()
              == fn::copack{FileNotFound});
      }

      SECTION("copack on both sides")
      {
        using Lh = fn::expected<fn::copack<double, int>, fn::copack<Error>>;
        using Rh = fn::expected<fn::copack<bool, int>, int>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::copack< //
                                                    fn::pack<double, bool>, fn::pack<double, int>, fn::pack<int, bool>,
                                                    fn::pack<int, int>>,
                                                fn::copack_for<Error, int>>>);

        CHECK((Lh{fn::copack{0.5}} & Rh{fn::copack{12}})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::copack{true});
        CHECK((Lh{::fn::unexpect, fn::copack{FileNotFound}} & Rh{fn::copack{12}}).error() == fn::copack{FileNotFound});
        CHECK((Lh{fn::copack{0.5}} & Rh{::fn::unexpect, 13}).error() == fn::copack{13});
        CHECK((Lh{::fn::unexpect, fn::copack{FileNotFound}} & Rh{::fn::unexpect, 13}).error()
              == fn::copack{FileNotFound});

        SECTION("copack of packs on left")
        {
          using Lh = fn::expected<fn::copack_for<fn::pack<double, bool>, fn::pack<double, int>>, fn::copack<Error>>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::copack< //
                                                      fn::pack<double, bool, bool>, fn::pack<double, bool, int>,
                                                      fn::pack<double, int, bool>, fn::pack<double, int, int>>,
                                                  fn::copack_for<Error, int>>>);

          CHECK((Lh{fn::copack{fn::pack{0.5, 3}}} & Rh{fn::copack{12}})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                    })
                    .value()
                == fn::copack{true});
          CHECK((Lh{::fn::unexpect, fn::copack{FileNotFound}} & Rh{fn::copack{12}}).error()
                == fn::copack{FileNotFound});
          CHECK((Lh{fn::copack{fn::pack{0.5, 3}}} & Rh{::fn::unexpect, 13}).error() == fn::copack{13});
          CHECK((Lh{::fn::unexpect, fn::copack{FileNotFound}} & Rh{::fn::unexpect, 13}).error()
                == fn::copack{FileNotFound});
        }
      }

      SECTION("copack on left side only")
      {
        using Lh = fn::expected<fn::copack<double, int>, fn::copack<Error>>;
        using Rh = fn::expected<int, int>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::copack< //
                                                    fn::pack<double, int>, fn::pack<int, int>>,
                                                fn::copack_for<Error, int>>>);

        CHECK((Lh{fn::copack{0.5}} & Rh{12})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::copack{true});
        CHECK((Lh{::fn::unexpect, fn::copack{FileNotFound}} & Rh{12}).error() == fn::copack{FileNotFound});
        CHECK((Lh{fn::copack{0.5}} & Rh{::fn::unexpect, 13}).error() == fn::copack{13});
        CHECK((Lh{::fn::unexpect, fn::copack{FileNotFound}} & Rh{::fn::unexpect, 13}).error()
              == fn::copack{FileNotFound});

        SECTION("copack of packs on left")
        {
          using Lh = fn::expected<fn::copack_for<fn::pack<double, bool>, fn::pack<double, int>>, fn::copack<Error>>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::copack< //
                                                      fn::pack<double, bool, int>, fn::pack<double, int, int>>,
                                                  fn::copack_for<Error, int>>>);

          CHECK((Lh{fn::copack{fn::pack{0.5, 3}}} & Rh{12})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                    })
                    .value()
                == fn::copack{true});
          CHECK((Lh{::fn::unexpect, fn::copack{FileNotFound}} & Rh{12}).error() == fn::copack{FileNotFound});
          CHECK((Lh{fn::copack{fn::pack{0.5, 3}}} & Rh{::fn::unexpect, 13}).error() == fn::copack{13});
          CHECK((Lh{::fn::unexpect, fn::copack{FileNotFound}} & Rh{::fn::unexpect, 13}).error()
                == fn::copack{FileNotFound});
        }
      }

      SECTION("copack on right side only")
      {
        using Lh = fn::expected<double, fn::copack<Error>>;
        using Rh = fn::expected<fn::copack<bool, int>, int>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::copack< //
                                                    fn::pack<double, bool>, fn::pack<double, int>>,
                                                fn::copack_for<Error, int>>>);

        CHECK((Lh{0.5} & Rh{fn::copack{12}})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::copack{true});
        CHECK((Lh{::fn::unexpect, fn::copack{FileNotFound}} & Rh{fn::copack{12}}).error() == fn::copack{FileNotFound});
        CHECK((Lh{0.5} & Rh{::fn::unexpect, 13}).error() == fn::copack{13});
        CHECK((Lh{::fn::unexpect, fn::copack{FileNotFound}} & Rh{::fn::unexpect, 13}).error()
              == fn::copack{FileNotFound});

        SECTION("pack on left")
        {
          using Lh = fn::expected<fn::pack<double, int>, fn::copack<Error>>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::copack< //
                                                      fn::pack<double, int, bool>, fn::pack<double, int, int>>,
                                                  fn::copack_for<Error, int>>>);

          CHECK((Lh{fn::pack{0.5, 3}} & Rh{fn::copack{12}})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                    })
                    .value()
                == fn::copack{true});
          CHECK((Lh{::fn::unexpect, fn::copack{FileNotFound}} & Rh{fn::copack{12}}).error()
                == fn::copack{FileNotFound});
          CHECK((Lh{fn::pack{0.5, 3}} & Rh{::fn::unexpect, 13}).error() == fn::copack{13});
          CHECK((Lh{::fn::unexpect, fn::copack{FileNotFound}} & Rh{::fn::unexpect, 13}).error()
                == fn::copack{FileNotFound});
        }
      }
    }

    SECTION("graded monad as right operand")
    {
      static_assert(std::same_as<decltype(std::declval<fn::expected<void, Error>>()
                                          & std::declval<fn::expected<int, fn::copack<Error>>>()),
                                 fn::expected<int, fn::copack<Error>>>);

      static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::copack<Error>>>()
                                          & std::declval<fn::expected<void, fn::copack<Error>>>()),
                                 fn::expected<int, fn::copack<Error>>>);

      static_assert(std::same_as<decltype(std::declval<fn::expected<void, fn::copack<int>>>()
                                          & std::declval<fn::expected<int, fn::copack<Error>>>()),
                                 fn::expected<int, fn::copack_for<Error, int>>>);

      static_assert(std::same_as<decltype(std::declval<fn::expected<void, fn::copack<bool, int>>>()
                                          & std::declval<fn::expected<int, fn::copack<Error>>>()),
                                 fn::expected<int, fn::copack_for<Error, bool, int>>>);

      static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::copack<bool, int>>>()
                                          & std::declval<fn::expected<void, fn::copack<Error>>>()),
                                 fn::expected<int, fn::copack_for<Error, bool, int>>>);

      SECTION("value & void & yield value")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<int, int>>()
                                            & std::declval<fn::expected<void, fn::copack<Error>>>()),
                                   fn::expected<int, fn::copack_for<Error, int>>>);

        CHECK((fn::expected<int, int>{12} //
               & fn::expected<void, fn::copack<Error>>{})
                  .value()
              == 12);
        CHECK((fn::expected<int, int>{12} //
               & fn::expected<void, fn::copack<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::copack{FileNotFound});
        CHECK((fn::expected<int, int>{::fn::unexpect, 13} //
               & fn::expected<void, fn::copack<Error>>{})
                  .error()
              == fn::copack{13});
        CHECK((fn::expected<int, int>{::fn::unexpect, 13} //
               & fn::expected<void, fn::copack<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::copack{13});
      }

      SECTION("void & value yield value")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<void, int>>()
                                            & std::declval<fn::expected<int, fn::copack<Error>>>()),
                                   fn::expected<int, fn::copack_for<Error, int>>>);

        CHECK((fn::expected<void, int>{} //
               & fn::expected<int, fn::copack<Error>>{42})
                  .value()
              == 42);
        CHECK((fn::expected<void, int>{} //
               & fn::expected<int, fn::copack<Error>>{::fn::unexpect, fn::copack{FileNotFound}})
                  .error()
              == fn::copack{FileNotFound});
        CHECK((fn::expected<void, int>{::fn::unexpect, 13} //
               & fn::expected<int, fn::copack<Error>>{42})
                  .error()
              == fn::copack{13});
        CHECK((fn::expected<void, int>{::fn::unexpect, 13} //
               & fn::expected<int, fn::copack<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::copack{13});
      }

      SECTION("void & void yield void")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<void, int>>()
                                            & std::declval<fn::expected<void, fn::copack<Error>>>()),
                                   fn::expected<void, fn::copack_for<Error, int>>>);

        CHECK((fn::expected<void, int>{} //
               & fn::expected<void, fn::copack<Error>>{})
                  .has_value());
        CHECK((fn::expected<void, int>{} //
               & fn::expected<void, fn::copack<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::copack{FileNotFound});
        CHECK((fn::expected<void, int>{::fn::unexpect, 13} //
               & fn::expected<void, fn::copack<Error>>{})
                  .error()
              == fn::copack{13});
        CHECK((fn::expected<void, int>{::fn::unexpect, 13} //
               & fn::expected<void, fn::copack<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::copack{13});
      }

      SECTION("value & value yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<double, int>>()
                                            & std::declval<fn::expected<int, fn::copack<Error>>>()),
                                   fn::expected<fn::pack<double, int>, fn::copack_for<Error, int>>>);

        CHECK((fn::expected<double, int>{0.5} //
               & fn::expected<int, fn::copack<Error>>{12})
                  .transform([](double d, int i) constexpr -> bool { return d == 0.5 && i == 12; })
                  .value());
        CHECK((fn::expected<double, int>{0.5} //
               & fn::expected<int, fn::copack<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::copack{FileNotFound});
        CHECK((fn::expected<double, int>{::fn::unexpect, 13} //
               & fn::expected<int, fn::copack<Error>>{12})
                  .error()
              == fn::copack{13});
        CHECK((fn::expected<double, int>{::fn::unexpect, 13} //
               & fn::expected<int, fn::copack<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::copack{13});
      }

      SECTION("pack & value yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<fn::pack<double, bool>, int>>()
                                            & std::declval<fn::expected<int, fn::copack<Error>>>()),
                                   fn::expected<fn::pack<double, bool, int>, fn::copack_for<Error, int>>>);

        CHECK((fn::expected<fn::pack<double, bool>, int>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<int, fn::copack<Error>>{12})
                  .transform([](double d, bool b, int i) constexpr -> bool { return d == 0.5 && b && i == 12; })
                  .value());
        CHECK((fn::expected<fn::pack<double, bool>, int>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<int, fn::copack<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::copack{FileNotFound});
        CHECK((fn::expected<fn::pack<double, bool>, int>{::fn::unexpect, 13} //
               & fn::expected<int, fn::copack<Error>>{12})
                  .error()
              == fn::copack{13});
        CHECK((fn::expected<fn::pack<double, bool>, int>{::fn::unexpect, 13} //
               & fn::expected<int, fn::copack<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::copack{13});
      }

      SECTION("pack & void yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<fn::pack<double, bool>, int>>()
                                            & std::declval<fn::expected<void, fn::copack<Error>>>()),
                                   fn::expected<fn::pack<double, bool>, fn::copack_for<Error, int>>>);

        CHECK((fn::expected<fn::pack<double, bool>, int>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<void, fn::copack<Error>>{})
                  .transform([](double d, bool b) constexpr -> bool { return d == 0.5 && b; })
                  .value());
        CHECK((fn::expected<fn::pack<double, bool>, int>{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & fn::expected<void, fn::copack<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::copack{FileNotFound});
        CHECK((fn::expected<fn::pack<double, bool>, int>{::fn::unexpect, 13} //
               & fn::expected<void, fn::copack<Error>>{})
                  .error()
              == fn::copack{13});
        CHECK((fn::expected<fn::pack<double, bool>, int>{::fn::unexpect, 13} //
               & fn::expected<void, fn::copack<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::copack{13});
      }

      SECTION("void & pack yield pack")
      {
        static_assert(std::same_as<decltype(std::declval<fn::expected<void, int>>()
                                            & std::declval<fn::expected<fn::pack<double, bool>, fn::copack<Error>>>()),
                                   fn::expected<fn::pack<double, bool>, fn::copack_for<Error, int>>>);

        CHECK((fn::expected<void, int>{} //
               & fn::expected<fn::pack<double, bool>, fn::copack<Error>>{std::in_place,
                                                                         fn::pack<double, bool>{0.5, true}})
                  .transform([](double d, bool b) constexpr -> bool { return d == 0.5 && b; })
                  .value());
        CHECK((fn::expected<void, int>{} //
               & fn::expected<fn::pack<double, bool>, fn::copack<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::copack{FileNotFound});
        CHECK((fn::expected<void, int>{::fn::unexpect, 13} //
               & fn::expected<fn::pack<double, bool>, fn::copack<Error>>{std::in_place,
                                                                         fn::pack<double, bool>{0.5, true}})
                  .error()
              == fn::copack{13});
        CHECK((fn::expected<void, int>{::fn::unexpect, 13} //
               & fn::expected<fn::pack<double, bool>, fn::copack<Error>>{::fn::unexpect, FileNotFound})
                  .error()
              == fn::copack{13});
      }

      SECTION("copack on both sides")
      {
        using Lh = fn::expected<fn::copack<double, int>, Error>;
        using Rh = fn::expected<fn::copack<bool, int>, fn::copack<int>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::copack< //
                                                    fn::pack<double, bool>, fn::pack<double, int>, fn::pack<int, bool>,
                                                    fn::pack<int, int>>,
                                                fn::copack_for<Error, int>>>);

        CHECK((Lh{fn::copack{0.5}} & Rh{fn::copack{12}})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::copack{true});
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{fn::copack{12}}).error() == fn::copack{FileNotFound});
        CHECK((Lh{fn::copack{0.5}} & Rh{::fn::unexpect, fn::copack{13}}).error() == fn::copack{13});
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, fn::copack{13}}).error()
              == fn::copack{FileNotFound});

        SECTION("copack of packs on left")
        {
          using Lh = fn::expected<fn::copack_for<fn::pack<double, bool>, fn::pack<double, int>>, Error>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::copack< //
                                                      fn::pack<double, bool, bool>, fn::pack<double, bool, int>,
                                                      fn::pack<double, int, bool>, fn::pack<double, int, int>>,
                                                  fn::copack_for<Error, int>>>);

          CHECK((Lh{fn::copack{fn::pack{0.5, 3}}} & Rh{fn::copack{12}})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                    })
                    .value()
                == fn::copack{true});
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{fn::copack{12}}).error() == fn::copack{FileNotFound});
          CHECK((Lh{fn::copack{fn::pack{0.5, 3}}} & Rh{::fn::unexpect, fn::copack{13}}).error() == fn::copack{13});
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, fn::copack{13}}).error()
                == fn::copack{FileNotFound});
        }
      }

      SECTION("copack on left side only")
      {
        using Lh = fn::expected<fn::copack<double, int>, Error>;
        using Rh = fn::expected<int, fn::copack<int>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::copack< //
                                                    fn::pack<double, int>, fn::pack<int, int>>,
                                                fn::copack_for<Error, int>>>);

        CHECK((Lh{fn::copack{0.5}} & Rh{12})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::copack{true});
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{12}).error() == fn::copack{FileNotFound});
        CHECK((Lh{fn::copack{0.5}} & Rh{::fn::unexpect, 13}).error() == fn::copack{13});
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, 13}).error() == fn::copack{FileNotFound});

        SECTION("copack of packs on left")
        {
          using Lh = fn::expected<fn::copack_for<fn::pack<double, bool>, fn::pack<double, int>>, Error>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::copack< //
                                                      fn::pack<double, bool, int>, fn::pack<double, int, int>>,
                                                  fn::copack_for<Error, int>>>);

          CHECK((Lh{fn::copack{fn::pack{0.5, 3}}} & Rh{12})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                    })
                    .value()
                == fn::copack{true});
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{12}).error() == fn::copack{FileNotFound});
          CHECK((Lh{fn::copack{fn::pack{0.5, 3}}} & Rh{::fn::unexpect, fn::copack{13}}).error() == fn::copack{13});
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, fn::copack{13}}).error()
                == fn::copack{FileNotFound});
        }
      }

      SECTION("copack on right side only")
      {
        using Lh = fn::expected<double, Error>;
        using Rh = fn::expected<fn::copack<bool, int>, fn::copack<int>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::copack< //
                                                    fn::pack<double, bool>, fn::pack<double, int>>,
                                                fn::copack_for<Error, int>>>);

        CHECK((Lh{0.5} & Rh{fn::copack{12}})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::copack{true});
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{fn::copack{12}}).error() == fn::copack{FileNotFound});
        CHECK((Lh{0.5} & Rh{::fn::unexpect, fn::copack{13}}).error() == fn::copack{13});
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, fn::copack{13}}).error()
              == fn::copack{FileNotFound});

        SECTION("pack on left")
        {
          using Lh = fn::expected<fn::pack<double, int>, Error>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::copack< //
                                                      fn::pack<double, int, bool>, fn::pack<double, int, int>>,
                                                  fn::copack_for<Error, int>>>);

          CHECK((Lh{fn::pack{0.5, 3}} & Rh{fn::copack{12}})
                    .transform([](auto i, auto j, auto k) constexpr -> bool {
                      return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                    })
                    .value()
                == fn::copack{true});
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{fn::copack{12}}).error() == fn::copack{FileNotFound});
          CHECK((Lh{fn::pack{0.5, 3}} & Rh{::fn::unexpect, fn::copack{13}}).error() == fn::copack{13});
          CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, fn::copack{13}}).error()
                == fn::copack{FileNotFound});
        }
      }
    }

    SECTION("graded monad on both sides")
    {
      static_assert(std::same_as<decltype(std::declval<fn::expected<int, fn::copack<bool, int>>>()
                                          & std::declval<fn::expected<void, fn::copack<Error>>>()),
                                 fn::expected<int, fn::copack_for<Error, bool, int>>>);

      SECTION("value & void & yield value")
      {
        using Lh = fn::expected<int, fn::copack<bool, int>>;
        using Rh = fn::expected<void, fn::copack<Error>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<int, fn::copack_for<Error, bool, int>>>);

        CHECK((Lh{12} & Rh{}).value() == 12);
        CHECK((Lh{12} & Rh{::fn::unexpect, fn::copack{FileNotFound}}).error() == fn::copack{FileNotFound});
        CHECK((Lh{::fn::unexpect, fn::copack{13}} & Rh{}).error() == fn::copack{13});
        CHECK((Lh{::fn::unexpect, fn::copack{13}} & Rh{::fn::unexpect, fn::copack{FileNotFound}}).error()
              == fn::copack{13});
      }

      SECTION("void & value yield value")
      {
        using Lh = fn::expected<void, fn::copack<bool, int>>;
        using Rh = fn::expected<int, fn::copack<Error>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<int, fn::copack_for<Error, bool, int>>>);

        CHECK((Lh{} & Rh{42}).value() == 42);
        CHECK((Lh{} & Rh{::fn::unexpect, fn::copack{FileNotFound}}).error() == fn::copack{FileNotFound});
        CHECK((Lh{::fn::unexpect, fn::copack{13}} & Rh{42}).error() == fn::copack{13});
        CHECK((Lh{::fn::unexpect, fn::copack{13}} & Rh{::fn::unexpect, fn::copack{FileNotFound}}).error()
              == fn::copack{13});
      }

      SECTION("void & void yield void")
      {
        using Lh = fn::expected<void, fn::copack<bool, int>>;
        using Rh = fn::expected<void, fn::copack<Error>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<void, fn::copack_for<Error, bool, int>>>);

        CHECK((Lh{} & Rh{}).has_value());
        CHECK((Lh{} & Rh{::fn::unexpect, fn::copack{FileNotFound}}).error() == fn::copack{FileNotFound});
        CHECK((Lh{::fn::unexpect, fn::copack{13}} & Rh{}).error() == fn::copack{13});
        CHECK((Lh{::fn::unexpect, fn::copack{13}} & Rh{::fn::unexpect, fn::copack{FileNotFound}}).error()
              == fn::copack{13});
      }

      SECTION("value & value yield pack")
      {
        using Lh = fn::expected<double, fn::copack<bool, int>>;
        using Rh = fn::expected<int, fn::copack<Error>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::pack<double, int>, fn::copack_for<Error, bool, int>>>);

        CHECK((Lh{0.5} & Rh{12})
                  .transform([](double d, int i) constexpr -> bool { return d == 0.5 && i == 12; })
                  .value());
        CHECK((Lh{0.5} & Rh{::fn::unexpect, fn::copack{FileNotFound}}).error() == fn::copack{FileNotFound});
        CHECK((Lh{::fn::unexpect, fn::copack{13}} & Rh{12}).error() == fn::copack{13});
        CHECK((Lh{::fn::unexpect, fn::copack{13}} & Rh{::fn::unexpect, fn::copack{FileNotFound}}).error()
              == fn::copack{13});
      }

      SECTION("pack & value yield pack")
      {
        using Lh = fn::expected<fn::pack<double, bool>, fn::copack<bool, int>>;
        using Rh = fn::expected<int, fn::copack<Error>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::pack<double, bool, int>, fn::copack_for<Error, bool, int>>>);

        CHECK((Lh{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & Rh{12})
                  .transform([](double d, bool b, int i) constexpr -> bool { return d == 0.5 && b && i == 12; })
                  .value());
        CHECK((Lh{std::in_place, fn::pack<double, bool>{0.5, true}} //
               & Rh{::fn::unexpect, fn::copack{FileNotFound}})
                  .error()
              == fn::copack{FileNotFound});
        CHECK((Lh{::fn::unexpect, fn::copack{13}} //
               & Rh{12})
                  .error()
              == fn::copack{13});
        CHECK((Lh{::fn::unexpect, fn::copack{13}} //
               & Rh{::fn::unexpect, fn::copack{FileNotFound}})
                  .error()
              == fn::copack{13});
      }

      SECTION("pack & void yield pack")
      {
        using Lh = fn::expected<fn::pack<double, bool>, fn::copack<bool, int>>;
        using Rh = fn::expected<void, fn::copack<Error>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::pack<double, bool>, fn::copack_for<Error, bool, int>>>);

        CHECK((Lh{std::in_place, fn::pack<double, bool>{0.5, true}} & Rh{})
                  .transform([](double d, bool b) constexpr -> bool { return d == 0.5 && b; })
                  .value());
        CHECK((Lh{std::in_place, fn::pack<double, bool>{0.5, true}} & Rh{::fn::unexpect, fn::copack{FileNotFound}})
                  .error()
              == fn::copack{FileNotFound});
        CHECK((Lh{::fn::unexpect, fn::copack{13}} & Rh{}).error() == fn::copack{13});
        CHECK((Lh{::fn::unexpect, fn::copack{13}} & Rh{::fn::unexpect, fn::copack{FileNotFound}}).error()
              == fn::copack{13});
      }

      SECTION("void & pack yield pack")
      {
        using Lh = fn::expected<void, fn::copack<bool, int>>;
        using Rh = fn::expected<fn::pack<double, bool>, fn::copack<Error>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::pack<double, bool>, fn::copack_for<Error, bool, int>>>);

        CHECK((Lh{} & Rh{std::in_place, fn::pack<double, bool>{0.5, true}})
                  .transform([](double d, bool b) constexpr -> bool { return d == 0.5 && b; })
                  .value());
        CHECK((Lh{} & Rh{::fn::unexpect, fn::copack{FileNotFound}}).error() == fn::copack{FileNotFound});
        CHECK((Lh{::fn::unexpect, fn::copack{13}} & Rh{std::in_place, fn::pack<double, bool>{0.5, true}}).error()
              == fn::copack{13});
        CHECK((Lh{::fn::unexpect, fn::copack{13}} & Rh{::fn::unexpect, fn::copack{FileNotFound}}).error()
              == fn::copack{13});
      }

      SECTION("copack on both sides")
      {
        using Lh = fn::expected<fn::copack<double, int>, fn::copack<Error>>;
        using Rh = fn::expected<fn::copack<bool, int>, fn::copack<bool, int>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::copack< //
                                                    fn::pack<double, bool>, fn::pack<double, int>, fn::pack<int, bool>,
                                                    fn::pack<int, int>>,
                                                fn::copack_for<Error, bool, int>>>);

        CHECK((Lh{fn::copack{0.5}} & Rh{fn::copack{12}})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::copack{true});
        CHECK((Lh{::fn::unexpect, fn::copack{FileNotFound}} & Rh{fn::copack{12}}).error() == fn::copack{FileNotFound});
        CHECK((Lh{fn::copack{0.5}} & Rh{::fn::unexpect, fn::copack{13}}).error() == fn::copack{13});
        CHECK((Lh{::fn::unexpect, fn::copack{FileNotFound}} & Rh{::fn::unexpect, fn::copack{13}}).error()
              == fn::copack{FileNotFound});

        SECTION("copack of packs on left")
        {
          using Lh = fn::expected<fn::copack_for<fn::pack<double, bool>, fn::pack<double, int>>, fn::copack<Error>>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::copack< //
                                                      fn::pack<double, bool, bool>, fn::pack<double, bool, int>,
                                                      fn::pack<double, int, bool>, fn::pack<double, int, int>>,
                                                  fn::copack_for<Error, bool, int>>>);

          CHECK((Lh{fn::copack{fn::pack{0.5, 3}}} & Rh{fn::copack{12}})
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
                == fn::copack{true});
          CHECK((Lh{::fn::unexpect, fn::copack{FileNotFound}} & Rh{fn::copack{12}}).error()
                == fn::copack{FileNotFound});
          CHECK((Lh{fn::copack{fn::pack{0.5, 3}}} & Rh{::fn::unexpect, fn::copack{13}}).error() == fn::copack{13});
          CHECK((Lh{::fn::unexpect, fn::copack{FileNotFound}} & Rh{::fn::unexpect, fn::copack{13}}).error()
                == fn::copack{FileNotFound});
        }
      }

      SECTION("copack on left side only")
      {
        using Lh = fn::expected<fn::copack<double, int>, fn::copack<Error>>;
        using Rh = fn::expected<int, fn::copack<bool, int>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::copack< //
                                                    fn::pack<double, int>, fn::pack<int, int>>,
                                                fn::copack_for<Error, bool, int>>>);

        CHECK((Lh{fn::copack{0.5}} & Rh{12})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::copack{true});
        CHECK((Lh{::fn::unexpect, fn::copack{FileNotFound}} & Rh{12}).error() == fn::copack{FileNotFound});
        CHECK((Lh{fn::copack{0.5}} & Rh{::fn::unexpect, 13}).error() == fn::copack{13});
        CHECK((Lh{::fn::unexpect, fn::copack{FileNotFound}} & Rh{::fn::unexpect, 13}).error()
              == fn::copack{FileNotFound});

        SECTION("copack of packs on left")
        {
          using Lh = fn::expected<fn::copack_for<fn::pack<double, bool>, fn::pack<double, int>>, fn::copack<Error>>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::copack< //
                                                      fn::pack<double, bool, int>, fn::pack<double, int, int>>,
                                                  fn::copack_for<Error, bool, int>>>);

          CHECK((Lh{fn::copack{fn::pack{0.5, 3}}} & Rh{12})
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
                == fn::copack{true});
          CHECK((Lh{::fn::unexpect, fn::copack{FileNotFound}} & Rh{12}).error() == fn::copack{FileNotFound});
          CHECK((Lh{fn::copack{fn::pack{0.5, 3}}} & Rh{::fn::unexpect, fn::copack{13}}).error() == fn::copack{13});
          CHECK((Lh{::fn::unexpect, fn::copack{FileNotFound}} & Rh{::fn::unexpect, fn::copack{13}}).error()
                == fn::copack{FileNotFound});
        }
      }

      SECTION("copack on right side only")
      {
        using Lh = fn::expected<double, fn::copack<Error>>;
        using Rh = fn::expected<fn::copack<bool, int>, fn::copack<bool, int>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::expected<fn::copack< //
                                                    fn::pack<double, bool>, fn::pack<double, int>>,
                                                fn::copack_for<Error, bool, int>>>);

        CHECK((Lh{0.5} & Rh{fn::copack{12}})
                  .transform([](auto i, auto j) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                  })
                  .value()
              == fn::copack{true});
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{fn::copack{12}}).error() == fn::copack{FileNotFound});
        CHECK((Lh{0.5} & Rh{::fn::unexpect, fn::copack{13}}).error() == fn::copack{13});
        CHECK((Lh{::fn::unexpect, FileNotFound} & Rh{::fn::unexpect, fn::copack{13}}).error()
              == fn::copack{FileNotFound});

        SECTION("pack on left")
        {
          using Lh = fn::expected<fn::pack<double, int>, fn::copack<Error>>;
          static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                     fn::expected<fn::copack< //
                                                      fn::pack<double, int, bool>, fn::pack<double, int, int>>,
                                                  fn::copack_for<Error, bool, int>>>);

          CHECK((Lh{fn::pack{0.5, 3}} & Rh{fn::copack{12}})
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
                == fn::copack{true});
          CHECK((Lh{::fn::unexpect, fn::copack{FileNotFound}} & Rh{fn::copack{12}}).error()
                == fn::copack{FileNotFound});
          CHECK((Lh{fn::pack{0.5, 3}} & Rh{::fn::unexpect, fn::copack{13}}).error() == fn::copack{13});
          CHECK((Lh{::fn::unexpect, fn::copack{FileNotFound}} & Rh{::fn::unexpect, fn::copack{13}}).error()
                == fn::copack{FileNotFound});
        }
      }
    }

    SECTION("unit error grade copack<>")
    {
      // A never-erroring expected<T, copack<>> composes with a fallible one: the copack<> operand adds no
      // alternative to the widened error, only its value to the pack. Previously ill-formed, because the
      // copack<> side made operator&'s error lambda deduce void and poisoned _join's return type.
      using Unit = fn::expected<int, fn::copack<>>;

      SECTION("different error, unit on left")
      {
        using Rh = fn::expected<int, Error>;
        static_assert(std::same_as<decltype(std::declval<Unit>() & std::declval<Rh>()),
                                   fn::expected<fn::pack<int, int>, fn::copack<Error>>>);

        static_assert((Unit{7} & Rh{5}) //
                          .transform([](int a, int b) constexpr -> bool { return a == 7 && b == 5; })
                          .value());
        static_assert((Unit{7} & Rh{::fn::unexpect, FileNotFound}).error() == fn::copack{FileNotFound});

        CHECK((Unit{7} & Rh{5}) //
                  .transform([](int a, int b) constexpr -> bool { return a == 7 && b == 5; })
                  .value());
        CHECK((Unit{7} & Rh{::fn::unexpect, FileNotFound}).error() == fn::copack{FileNotFound});
      }

      SECTION("different error, unit on right")
      {
        using Lh = fn::expected<int, Error>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Unit>()),
                                   fn::expected<fn::pack<int, int>, fn::copack<Error>>>);

        static_assert((Lh{5} & Unit{7}) //
                          .transform([](int a, int b) constexpr -> bool { return a == 5 && b == 7; })
                          .value());
        static_assert((Lh{::fn::unexpect, FileNotFound} & Unit{7}).error() == fn::copack{FileNotFound});

        CHECK((Lh{5} & Unit{7}) //
                  .transform([](int a, int b) constexpr -> bool { return a == 5 && b == 7; })
                  .value());
        CHECK((Lh{::fn::unexpect, FileNotFound} & Unit{7}).error() == fn::copack{FileNotFound});
      }

      SECTION("unit meets a copack grade, either order")
      {
        using Rh = fn::expected<int, fn::copack<Error>>;
        static_assert(std::same_as<decltype(std::declval<Unit>() & std::declval<Rh>()),
                                   fn::expected<fn::pack<int, int>, fn::copack<Error>>>);
        static_assert(std::same_as<decltype(std::declval<Rh>() & std::declval<Unit>()),
                                   fn::expected<fn::pack<int, int>, fn::copack<Error>>>);

        static_assert((Unit{7} & Rh{::fn::unexpect, fn::copack{FileNotFound}}).error() == fn::copack{FileNotFound});
        static_assert((Rh{::fn::unexpect, fn::copack{FileNotFound}} & Unit{7}).error() == fn::copack{FileNotFound});

        CHECK((Unit{7} & Rh{::fn::unexpect, fn::copack{FileNotFound}}).error() == fn::copack{FileNotFound});
        CHECK((Rh{::fn::unexpect, fn::copack{FileNotFound}} & Unit{7}).error() == fn::copack{FileNotFound});
      }

      SECTION("same error, both unit")
      {
        static_assert(std::same_as<decltype(std::declval<Unit>() & std::declval<Unit>()),
                                   fn::expected<fn::pack<int, int>, fn::copack<>>>);

        static_assert((Unit{7} & Unit{5}) //
                          .transform([](int a, int b) constexpr -> bool { return a == 7 && b == 5; })
                          .value());
        CHECK((Unit{7} & Unit{5}) //
                  .transform([](int a, int b) constexpr -> bool { return a == 7 && b == 5; })
                  .value());
      }

      SECTION("void operand carries the unit error")
      {
        using VoidUnit = fn::expected<void, fn::copack<>>;
        using Rh = fn::expected<int, Error>;
        static_assert(std::same_as<decltype(std::declval<VoidUnit>() & std::declval<Rh>()),
                                   fn::expected<int, fn::copack<Error>>>);
        static_assert(std::same_as<decltype(std::declval<Rh>() & std::declval<VoidUnit>()),
                                   fn::expected<int, fn::copack<Error>>>);

        static_assert((VoidUnit{} & Rh{5}).value() == 5);
        static_assert((VoidUnit{} & Rh{::fn::unexpect, FileNotFound}).error() == fn::copack{FileNotFound});
        static_assert((Rh{5} & VoidUnit{}).value() == 5);
        static_assert((Rh{::fn::unexpect, FileNotFound} & VoidUnit{}).error() == fn::copack{FileNotFound});

        CHECK((VoidUnit{} & Rh{5}).value() == 5);
        CHECK((VoidUnit{} & Rh{::fn::unexpect, FileNotFound}).error() == fn::copack{FileNotFound});
        CHECK((Rh{5} & VoidUnit{}).value() == 5);
        CHECK((Rh{::fn::unexpect, FileNotFound} & VoidUnit{}).error() == fn::copack{FileNotFound});
      }
    }

    SECTION("noexcept")
    {
      // the join relocates both operands' values and errors into the result, so it promises only
      // what relocating them promises
      using Lh = fn::expected<MoveNothrow, Error>;
      using Rh = fn::expected<int, Error>;
      static_assert(noexcept(std::declval<Rh &>() & std::declval<Rh &>()));
      static_assert(not noexcept(std::declval<Lh &>() & std::declval<Rh &>())); // copies the value
      static_assert(noexcept(std::declval<Lh &&>() & std::declval<Rh &&>()));   // moves it

      using Eh = fn::expected<int, MoveNothrow>;
      static_assert(not noexcept(std::declval<Eh &>() & std::declval<Eh &>())); // copies the error
      static_assert(noexcept(std::declval<Eh &&>() & std::declval<Eh &&>()));

      SECTION("void operand")
      {
        using Vh = fn::expected<void, Error>;
        static_assert(noexcept(std::declval<Vh &>() & std::declval<Rh &>()));
        static_assert(not noexcept(std::declval<Vh &>() & std::declval<Lh &>())); // carries the value
        static_assert(noexcept(std::declval<Vh &&>() & std::declval<Lh &&>()));
        static_assert(noexcept(std::declval<Vh &>() & std::declval<Vh &>()));
      }

      SECTION("widening the error")
      {
        using Wh = fn::expected<int, fn::copack<MoveNothrow>>;
        static_assert(not noexcept(std::declval<Wh &>() & std::declval<Rh &>())); // copies the error
        static_assert(noexcept(std::declval<Wh &&>() & std::declval<Rh &&>()));
      }

      SECTION("unit error operand")
      {
        // copack<> can hold no error, so the arm lifting it is unreachable and cannot throw - but the
        // value is still relocated, and still weighs
        using Uh = fn::expected<int, fn::copack<>>;
        using Ut = fn::expected<MoveNothrow, fn::copack<>>;
        static_assert(noexcept(std::declval<Uh &>() & std::declval<Rh &>()));
        static_assert(noexcept(std::declval<fn::expected<void, fn::copack<>> &>() & std::declval<Rh &>()));
        static_assert(not noexcept(std::declval<Ut &>() & std::declval<Rh &>()));
      }

      SUCCEED();
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

TEST_CASE("expected copack support and_then", "[expected][copack][and_then]")
{
  using S = fn::expected<fn::copack_for<int, std::string_view>, Error>;

  // noexcept (extension): same-error-type result AND nothrow apply AND nothrow error copy - and
  // the apply is weighed by fn's own trait, which asks the per-alternative dispatch that will run,
  // so a visitor set is weighed alternative by alternative.
  constexpr auto nothrow_lval
      = fn::overload{[](int &) noexcept -> fn::expected<bool, Error> { return {true}; },
                     [](std::string_view &) noexcept -> fn::expected<bool, Error> { return {false}; }};
  static_assert(noexcept(std::declval<S &>().and_then(nothrow_lval)));
  constexpr auto nothrow_generic = [](auto &&) noexcept -> fn::expected<bool, Error> { return {true}; };
  static_assert(noexcept(std::declval<S &>().and_then(nothrow_generic)));

  // constraints (extension, :82-83): exhaustive invocability over the copack's alternatives,
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
  using M = fn::expected<fn::copack<int>, move_only_error>;
  constexpr auto generic_fn = [](auto &&) -> fn::expected<bool, move_only_error> { throw 0; };
  constexpr auto can_and_then_M_lval = [](auto &&f) { return requires { std::declval<M &>().and_then(f); }; };
  constexpr auto can_and_then_M_rval = [](auto &&f) { return requires { std::declval<M &&>().and_then(f); }; };
  static_assert(not can_and_then_M_lval(generic_fn)); // error copy required, E move-only
  static_assert(can_and_then_M_rval(generic_fn));     // error moved

  SECTION("value")
  {
    fn::expected<fn::copack_for<int, std::string_view>, Error> s{fn::copack{12}};

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
    fn::expected<fn::copack_for<int, std::string_view>, Error> s{::fn::unexpect, FileNotFound};
    CHECK(s.and_then( //
               [](auto...) -> fn::expected<bool, Error> { return {true}; })
              .error()
          == FileNotFound);
    CHECK(std::as_const(s)
              .and_then( //
                  [](auto...) -> fn::expected<bool, Error> { return {true}; })
              .error()
          == FileNotFound);
    CHECK(fn::expected<fn::copack_for<int, std::string_view>, Error>{::fn::unexpect, FileNotFound}
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
    constexpr fn::expected<fn::copack_for<int, std::string_view>, Error> a{fn::copack{42}};
    static_assert(std::is_same_v<decltype(a.and_then(fn)), fn::expected<bool, Error>>);
    static_assert(a.and_then(fn).value());
  }

  SECTION("noexcept")
  {
    // the callback is invoked through fn's own dispatch, so only fn's nothrow-applicable trait can
    // answer for it - and it is nothrow only if EVERY alternative's call is
    using T = fn::expected<fn::copack<double, int>, Error>;
    static_assert(
        noexcept(std::declval<T &>().and_then([](auto) noexcept -> fn::expected<bool, Error> { return {true}; })));
    static_assert(not noexcept(std::declval<T &>().and_then([](auto) -> fn::expected<bool, Error> { return {true}; })));
    static_assert(not noexcept(std::declval<T &>().and_then(
        fn::overload{[](int) noexcept -> fn::expected<bool, Error> { return {true}; },
                     [](double) -> fn::expected<bool, Error> { return {true}; }}))); // one throwing arm is enough
    SUCCEED();
  }
}

TEST_CASE("expected copack support or_else", "[expected][copack][or_else]")
{
  using S = fn::expected<double, fn::copack_for<int, std::string_view>>;

  // noexcept (extension): same-value-type result AND nothrow apply AND nothrow value copy, with
  // the apply weighed per alternative by fn's own trait.
  constexpr auto nothrow_lval
      = fn::overload{[](int &) noexcept -> fn::expected<double, Error> { return {0.5}; },
                     [](std::string_view &) noexcept -> fn::expected<double, Error> { return {0.5}; }};
  static_assert(noexcept(std::declval<S &>().or_else(nothrow_lval)));
  constexpr auto nothrow_generic = [](auto &&) noexcept -> fn::expected<double, Error> { return {0.5}; };
  static_assert(noexcept(std::declval<S &>().or_else(nothrow_generic)));

  // constraints (extension, :167-168): invocability over the error copack's alternatives, tracking
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
  using M = fn::expected<move_only, fn::copack<int>>;
  constexpr auto generic_fn = [](auto &&) -> fn::expected<move_only, Error> { throw 0; };
  constexpr auto can_or_else_M_lval = [](auto &&f) { return requires { std::declval<M &>().or_else(f); }; };
  constexpr auto can_or_else_M_rval = [](auto &&f) { return requires { std::declval<M &&>().or_else(f); }; };
  static_assert(not can_or_else_M_lval(generic_fn)); // value copy required, T move-only
  static_assert(can_or_else_M_rval(generic_fn));     // value moved

  SECTION("value")
  {
    fn::expected<double, fn::copack_for<int, std::string_view>> s{::fn::unexpect, fn::copack{12}};

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
      fn::expected<double, fn::copack_for<int, std::string_view>> s{1.5};
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
      constexpr fn::expected<double, fn::copack_for<int, std::string_view>> a{::fn::unexpect, fn::copack{42}};
      static_assert(std::is_same_v<decltype(a.or_else(fn)), fn::expected<double, Error>>);
      static_assert(a.or_else(fn).value() == 42);
    }
  }

  SECTION("void")
  {
    fn::expected<void, fn::copack_for<int, std::string_view>> s{::fn::unexpect, fn::copack{12}};

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
      fn::expected<void, fn::copack_for<int, std::string_view>> s{};
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
      constexpr fn::expected<void, fn::copack_for<int, std::string_view>> a{::fn::unexpect, fn::copack{42}};
      static_assert(std::is_same_v<decltype(a.or_else(fn)), fn::expected<void, Error>>);
      static_assert(a.or_else(fn).error() == FileNotFound);
    }
  }
}

TEST_CASE("expected copack support transform", "[expected][copack][transform]")
{
  using S = fn::expected<fn::copack_for<int, std::string_view>, Error>;

  // noexcept: the copack-case _transform weighs the per-alternative dispatch, as and_then does
  constexpr auto nothrow_visitor = fn::overload{[](int const &) noexcept -> bool { return true; },
                                                [](std::string_view const &) noexcept -> bool { return false; }};
  static_assert(noexcept(std::declval<S &>().transform(nothrow_visitor)));
  constexpr auto nothrow_generic = [](auto &&) noexcept -> bool { return true; };
  static_assert(noexcept(std::declval<S &>().transform(nothrow_generic)));

  constexpr auto can_transform = [](auto &&f) { return requires { std::declval<S &>().transform(f); }; };
  static_assert(can_transform(nothrow_visitor));
  // the error-copy conjunct (:239) IS constrained: a move-only error cleanly drops the
  // overloads whose self would copy it, before the unconstrained body could hard-error
  struct move_only_error {
    move_only_error(move_only_error &&) = default;
  };
  using M = fn::expected<fn::copack<int>, move_only_error>;
  constexpr auto can_transform_M_lval = [](auto &&f) { return requires { std::declval<M &>().transform(f); }; };
  constexpr auto can_transform_M_rval = [](auto &&f) { return requires { std::declval<M &&>().transform(f); }; };
  static_assert(not can_transform_M_lval(nothrow_visitor));
  static_assert(can_transform_M_rval(nothrow_visitor));

  SECTION("value")
  {
    fn::expected<fn::copack_for<int, std::string_view>, Error> s{fn::copack{12}};

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
    fn::expected<fn::copack_for<int, std::string_view>, Error> s{::fn::unexpect, FileNotFound};
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
    constexpr fn::expected<fn::copack_for<int, std::string_view>, Error> a{fn::copack{42}};
    static_assert(std::is_same_v<decltype(a.transform(fn)), fn::expected<fn::copack<bool, int>, Error>>);
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

    S s{fn::copack{12}};
    CHECK(s.transform(lval_only).value() == fn::copack{true});
  }

  SECTION("noexcept")
  {
    // the dispatch is nothrow only if every alternative's call is, and only if relocating what each
    // returns into the result copack is - and, as ever, if lifting the untouched error is
    using T = fn::expected<fn::copack<double, int>, Error>;
    static_assert(noexcept(std::declval<T &>().transform([](auto) noexcept -> bool { return true; })));
    static_assert(not noexcept(std::declval<T &>().transform([](auto) -> bool { return true; })));
    static_assert(not noexcept(std::declval<fn::expected<fn::copack_for<MoveNothrow, int>, Error> &>().transform(
        [](auto v) noexcept { return v; })));
    static_assert(not noexcept(std::declval<fn::expected<fn::copack<double, int>, MoveNothrow> &>().transform(
        [](auto) noexcept -> bool { return true; }))); // copies the error
    SUCCEED();
  }
}

TEST_CASE("expected copack support transform_error", "[expected][copack][transform_error]")
{
  using S = fn::expected<double, fn::copack_for<int, std::string_view>>;

  // noexcept: the copack-case _transform_error weighs the per-alternative dispatch over the error,
  // and the untouched value it relocates
  constexpr auto nothrow_visitor = fn::overload{[](int const &) noexcept -> bool { return true; },
                                                [](std::string_view const &) noexcept -> bool { return false; }};
  static_assert(noexcept(std::declval<S &>().transform_error(nothrow_visitor)));
  constexpr auto nothrow_generic = [](auto &&) noexcept -> bool { return true; };
  static_assert(noexcept(std::declval<S &>().transform_error(nothrow_generic)));

  constexpr auto can_transform_error = [](auto &&f) { return requires { std::declval<S &>().transform_error(f); }; };
  static_assert(can_transform_error(nothrow_visitor));

  SECTION("value")
  {
    fn::expected<double, fn::copack_for<int, std::string_view>> s{::fn::unexpect, fn::copack{12}};

    CHECK(s.transform_error( //
               fn::overload{
                   [](int &i) -> bool { return i == 12; }, [](int const &) -> bool { throw 0; },
                   [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; },
                   [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                   [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }})
              .error()
          == fn::copack{true});

    CHECK(std::as_const(s)
              .transform_error( //
                  fn::overload{
                      [](int &) -> bool { throw 0; }, [](int const &i) -> bool { return i == 12; },
                      [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }})
              .error()
          == fn::copack{true});

    CHECK(std::move(std::as_const(s))
              .transform_error( //
                  fn::overload{
                      [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                      [](int &&) -> bool { throw 0; }, [](int const &&i) -> bool { return i == 12; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }})
              .error()
          == fn::copack{true});

    CHECK(std::move(s)
              .transform_error( //
                  fn::overload{
                      [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                      [](int &&i) -> bool { return i == 12; }, [](int const &&) -> bool { throw 0; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }})
              .error()
          == fn::copack{true});

    SECTION("value")
    {
      fn::expected<double, fn::copack_for<int, std::string_view>> s{1.5};
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
      constexpr fn::expected<double, fn::copack_for<int, std::string_view>> a{::fn::unexpect, fn::copack{42}};
      static_assert(std::is_same_v<decltype(a.transform_error(fn)), fn::expected<double, fn::copack<bool, int>>>);
      static_assert(a.transform_error(fn).error() == fn::copack{true});
    }
  }

  SECTION("void")
  {
    fn::expected<void, fn::copack_for<int, std::string_view>> s{::fn::unexpect, fn::copack{12}};

    CHECK(s.transform_error( //
               fn::overload{
                   [](int &i) -> int { return i; }, [](int const &) -> int { throw 0; }, [](int &&) -> int { throw 0; },
                   [](int const &&) -> int { throw 0; }, [](std::string_view &) -> int { throw 0; },
                   [](std::string_view const &) -> int { throw 0; }, [](std::string_view &&) -> int { throw 0; },
                   [](std::string_view const &&) -> int { throw 0; }})
              .error()
          == fn::copack{12});

    CHECK(std::as_const(s)
              .transform_error( //
                  fn::overload{
                      [](int &) -> int { throw 0; }, [](int const &i) -> int { return i; },
                      [](int &&) -> int { throw 0; }, [](int const &&) -> int { throw 0; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }})
              .error()
          == fn::copack{12});

    CHECK(std::move(std::as_const(s))
              .transform_error( //
                  fn::overload{
                      [](int &) -> int { throw 0; }, [](int const &) -> int { throw 0; },
                      [](int &&) -> int { throw 0; }, [](int const &&i) -> int { return i; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }})
              .error()
          == fn::copack{12});

    CHECK(std::move(s)
              .transform_error( //
                  fn::overload{
                      [](int &) -> int { throw 0; }, [](int const &) -> int { throw 0; },
                      [](int &&i) -> int { return i; }, [](int const &&) -> int { throw 0; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }})
              .error()
          == fn::copack{12});

    SECTION("value")
    {
      fn::expected<void, fn::copack_for<int, std::string_view>> s{};
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
      constexpr fn::expected<void, fn::copack_for<int, std::string_view>> a{::fn::unexpect, fn::copack{42}};
      static_assert(std::is_same_v<decltype(a.transform_error(fn)), fn::expected<void, fn::copack<int>>>);
      static_assert(a.transform_error(fn).error() == fn::copack{42});
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

    S s{::fn::unexpect, fn::copack{12}};
    CHECK(s.transform_error(lval_only).error() == fn::copack{true});
  }

  SECTION("move-only value")
  {
    // the conjunct or_else carries and pfn's transform_error requires: the untouched value goes
    // into the result, so only the overloads whose self can be moved from survive
    constexpr auto generic = [](auto &&) -> bool { throw 0; };

    using M = fn::expected<std::unique_ptr<int>, fn::copack<int>>; // copack-case overload
    constexpr auto can_M_lval = [](auto &&f) { return requires { std::declval<M &>().transform_error(f); }; };
    constexpr auto can_M_rval = [](auto &&f) { return requires { std::declval<M &&>().transform_error(f); }; };
    static_assert(not can_M_lval(generic)); // would copy the value
    static_assert(can_M_rval(generic));     // moves it

    using N = fn::expected<std::unique_ptr<int>, Error>; // non-copack overload
    constexpr auto can_N_lval = [](auto &&f) { return requires { std::declval<N &>().transform_error(f); }; };
    constexpr auto can_N_rval = [](auto &&f) { return requires { std::declval<N &&>().transform_error(f); }; };
    static_assert(not can_N_lval(generic));
    static_assert(can_N_rval(generic));

    N n{std::make_unique<int>(7)};
    auto r = std::move(n).transform_error([](Error const &) -> bool { throw 0; });
    static_assert(std::is_same_v<decltype(r), fn::expected<std::unique_ptr<int>, bool>>);
    CHECK(*r.value() == 7);
  }

  SECTION("noexcept")
  {
    // the error side dispatches through the copack exactly as the value side does
    using T = fn::expected<int, fn::copack<double, int>>;
    static_assert(noexcept(std::declval<T &>().transform_error([](auto) noexcept -> bool { return true; })));
    static_assert(not noexcept(std::declval<T &>().transform_error([](auto) -> bool { return true; })));
    static_assert(not noexcept(std::declval<fn::expected<MoveNothrow, fn::copack<double, int>> &>().transform_error(
        [](auto) noexcept -> bool { return true; }))); // copies the untouched value
    SUCCEED();
  }
}

TEST_CASE("expected pack support or_else", "[expected][or_else][pack]")
{
  using S = fn::expected<int, fn::pack<int, Error>>;

  // noexcept (extension): the apply is weighed by fn's own trait, which asks the pack-apply
  // dispatch that will run - one argument per element
  constexpr auto nothrow_two = [](int, Error &) noexcept -> fn::expected<int, Error> { return {1}; };
  static_assert(noexcept(std::declval<S &>().or_else(nothrow_two)));
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

  // noexcept (extension): nothrow apply AND nothrow value copy, with the apply weighed by fn's
  // own trait against the pack-apply dispatch that will run
  constexpr auto nothrow_two = [](int, Error &) noexcept -> bool { return true; };
  static_assert(noexcept(std::declval<S &>().transform_error(nothrow_two)));
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

namespace {
template <typename S, typename Fn, typename... Args>
concept can_apply = requires(S s, Fn fn, Args... args) { FWD(s).apply(FWD(fn), FWD(args)...); };

template <typename S, typename R, typename Fn>
concept can_apply_r = requires(S s, Fn fn) { FWD(s).template apply_r<R>(FWD(fn)); };

template <typename S, typename Fn, typename... Args>
concept can_apply_type = requires(S s, Fn fn, Args... args) { FWD(s).apply_type(FWD(fn), FWD(args)...); };

template <typename S, typename R, typename Fn, typename... Args>
concept can_apply_type_r
    = requires(S s, Fn fn, Args... args) { FWD(s).template apply_type_r<R>(FWD(fn), FWD(args)...); };
} // anonymous namespace

TEST_CASE("expected apply", "[expected][apply]")
{
  using fn::expected;

  // both arms are required outright: each arm receives its side's value as fn::apply hands it over
  constexpr auto arms = fn::overload{[](double v) noexcept -> double { return v; },
                                     [](std::string const &) noexcept -> double { return -1.0; }};
  expected<double, std::string> a{21.0};
  expected<double, std::string> e{fn::unexpect, "boom"};

  SECTION("noexcept")
  {
    static_assert(noexcept(a.apply(arms)));
    static_assert(noexcept(std::move(a).apply(arms)));
    static_assert(noexcept(a.apply_r<double>(arms)));
    constexpr auto throwing = fn::overload{[](double v) noexcept(false) -> double { return v; },
                                           [](std::string const &) noexcept -> double { return -1.0; }};
    static_assert(not noexcept(a.apply(throwing)));
    static_assert(not noexcept(a.apply_r<double>(throwing)));
    SUCCEED();
  }

  SECTION("both arms required")
  {
    static_assert(can_apply<expected<double, std::string> &, decltype(arms) const &>);
    static_assert(not can_apply<expected<double, std::string> &, decltype(fn::overload{[](double) {}}) const &>);
    static_assert(
        not can_apply<expected<double, std::string> &, decltype(fn::overload{[](std::string const &) {}}) const &>);

    // the untagged path is forgetful where T and E interconvert: one arm serves both rows
    static_assert(can_apply<expected<double, int> &, decltype(fn::overload{[](double) -> int { return 0; }}) const &>);

    CHECK(a.apply(arms) == 21.0);
    CHECK(e.apply(arms) == -1.0);

    SECTION("constexpr")
    {
      constexpr auto xarms
          = fn::overload{[](double v) noexcept -> int { return int(v); }, [](int e) noexcept -> int { return -e; }};
      static_assert(expected<double, int>{21.0}.apply(xarms) == 21);
      static_assert(expected<double, int>{fn::unexpect, 7}.apply(xarms) == -7);
      SUCCEED();
    }
  }

  SECTION("value categories")
  {
    CHECK(a.apply(fn::overload{[](std::string const &) -> bool { throw 1; }, //
                               [](double &) -> bool { return true; }, [](double const &) -> bool { throw 0; },
                               [](double &&) -> bool { throw 0; }, [](double const &&) -> bool { throw 0; }}));
    CHECK(std::as_const(a).apply(
        fn::overload{[](std::string const &) -> bool { throw 1; }, //
                     [](double &) -> bool { throw 0; }, [](double const &) -> bool { return true; },
                     [](double &&) -> bool { throw 0; }, [](double const &&) -> bool { throw 0; }}));
    CHECK(std::move(std::as_const(a))
              .apply(fn::overload{[](std::string const &) -> bool { throw 1; }, //
                                  [](double &) -> bool { throw 0; }, [](double const &) -> bool { throw 0; },
                                  [](double &&) -> bool { throw 0; }, [](double const &&) -> bool { return true; }}));
    CHECK(std::move(a).apply(fn::overload{[](std::string const &) -> bool { throw 1; }, //
                                          [](double &) -> bool { throw 0; }, [](double const &) -> bool { throw 0; },
                                          [](double &&) -> bool { return true; },
                                          [](double const &&) -> bool { throw 0; }}));
    // the error side forwards the same way
    CHECK(e.apply(fn::overload{[](double) -> bool { throw 1; }, //
                               [](std::string &) -> bool { return true; }, [](std::string const &) -> bool { throw 0; },
                               [](std::string &&) -> bool { throw 0; }}));
    CHECK(std::move(e).apply(fn::overload{[](double) -> bool { throw 1; }, //
                                          [](std::string &) -> bool { throw 0; },
                                          [](std::string const &) -> bool { throw 0; },
                                          [](std::string &&) -> bool { return true; }}));
  }

  SECTION("extra arguments")
  {
    constexpr auto xarms = fn::overload{[](double v, int x) noexcept -> double { return v + x; },
                                        [](std::string const &, int x) noexcept -> double { return -x; }};
    CHECK(a.apply(xarms, 2) == 23.0);
    CHECK(e.apply(xarms, 2) == -2.0);
  }

  SECTION("void value type")
  {
    constexpr auto varms = fn::overload{[]() noexcept -> int { return 1; }, [](int v) noexcept -> int { return -v; }};
    expected<void, int> v{};
    expected<void, int> ve{fn::unexpect, 7};
    CHECK(v.apply(varms) == 1);
    CHECK(ve.apply(varms) == -7);
    static_assert(not can_apply<expected<void, int> &, decltype(fn::overload{[](int) {}}) const &>);

    SECTION("constexpr")
    {
      static_assert(expected<void, int>{}.apply(varms) == 1);
      static_assert(expected<void, int>{fn::unexpect, 7}.apply(varms) == -7);
      SUCCEED();
    }
  }

  SECTION("pack payload")
  {
    using P = fn::pack<int, int>;
    expected<P, std::string> p{std::in_place, fn::pack{6, 7}};
    constexpr auto parms = fn::overload{[](int x, int y) noexcept -> int { return x * y; },
                                        [](std::string const &) noexcept -> int { return -1; }};
    CHECK(p.apply(parms) == 42);
    CHECK(expected<P, std::string>{fn::unexpect, "boom"}.apply(parms) == -1);
  }

  SECTION("tuple-like payload")
  {
    using T = std::tuple<int, char>;
    expected<T, std::string> t{std::in_place, 40, char(2)};
    constexpr auto tarms = fn::overload{[](int x, char y) noexcept -> int { return x + y; },
                                        [](std::string const &) noexcept -> int { return -1; }};
    CHECK(t.apply(tarms) == 42);
    // pass-whole still serves a whole-tuple arm on the untagged path (contrast apply_type)
    constexpr auto whole = fn::overload{[](T const &v) noexcept -> int { return std::get<0>(v); },
                                        [](std::string const &) noexcept -> int { return -1; }};
    CHECK(t.apply(whole) == 40);
  }

  SECTION("copack error payload")
  {
    using S = fn::copack_for<bool, std::string>;
    expected<int, S> s{fn::unexpect, S{std::string{"boom"}}};
    constexpr auto sarms
        = fn::overload{[](int v) noexcept -> int { return v; }, [](bool) noexcept -> int { return -1; },
                       [](std::string const &) noexcept -> int { return -2; }};
    CHECK(s.apply(sarms) == -2);
    CHECK(expected<int, S>{42}.apply(sarms) == 42);
  }

  SECTION("apply_r")
  {
    static_assert(std::is_same_v<long, decltype(a.apply_r<long>(arms))>);
    CHECK(a.apply_r<long>(arms) == 21L);
    CHECK(e.apply_r<long>(arms) == -1L);

    // the conversion to Ret is part of the question
    static_assert(not can_apply_r<expected<double, std::string> &, char *, decltype(arms) const &>);
    static_assert(can_apply_r<expected<double, std::string> &, long, decltype(arms) const &>);

    SECTION("constexpr")
    {
      constexpr auto xarms
          = fn::overload{[](double v) noexcept -> int { return int(v); }, [](int e) noexcept -> int { return -e; }};
      static_assert(expected<double, int>{21.0}.apply_r<long>(xarms) == 21L);
      SUCCEED();
    }
  }
}

TEST_CASE("expected apply_type", "[expected][apply_type]")
{
  using fn::expected;
  using fn::unexpect_t;
  using std::in_place_t;

  // the tags are the constructor tags naming each state: std::in_place for the value,
  // fn::unexpect for the error
  constexpr auto arms = fn::overload{[](in_place_t, double v) noexcept -> double { return v; },
                                     [](unexpect_t, int e) noexcept -> double { return -e; }};
  expected<double, int> a{21.0};
  expected<double, int> e{fn::unexpect, 7};

  SECTION("noexcept")
  {
    static_assert(noexcept(a.apply_type(arms)));
    static_assert(noexcept(std::move(a).apply_type(arms)));
    static_assert(noexcept(a.apply_type_r<double>(arms)));
    constexpr auto throwing = fn::overload{[](in_place_t, double v) noexcept(false) -> double { return v; },
                                           [](unexpect_t, int e) noexcept -> double { return -e; }};
    static_assert(not noexcept(a.apply_type(throwing)));
    static_assert(not noexcept(a.apply_type_r<double>(throwing)));
    SUCCEED();
  }

  SECTION("airtight over interconvertible value and error")
  {
    // the untagged path is forgetful: a lone double arm serves both rows of expected<double, int>;
    // the tagged path keys each row by a tag that never converts
    static_assert(can_apply<expected<double, int> &, decltype(fn::overload{[](double) -> int { return 0; }}) const &>);
    static_assert(
        not can_apply_type<expected<double, int> &, decltype(fn::overload{[](double) -> int { return 0; }}) const &>);

    // arm selection is by tag, not by value conversion: generic value parameters stay unambiguous
    constexpr auto rows = fn::overload{[](in_place_t, auto &&) noexcept -> int { return 1; },
                                       [](unexpect_t, auto &&) noexcept -> int { return 2; }};
    CHECK(a.apply_type(rows) == 1);
    CHECK(e.apply_type(rows) == 2);

    // dropping either arm makes the whole dispatch non-viable
    static_assert(can_apply_type<expected<double, int> &, decltype(arms) const &>);
    static_assert(
        not can_apply_type<expected<double, int> &, decltype(fn::overload{[](in_place_t, double) {}}) const &>);
    static_assert(not can_apply_type<expected<double, int> &, decltype(fn::overload{[](unexpect_t, int) {}}) const &>);

    // the tags reach the arms as prvalues, so rvalue-tag arms are served - probe and deed agree
    constexpr auto rv_tag = fn::overload{[](in_place_t &&, double v) noexcept -> double { return v; },
                                         [](unexpect_t &&, int e) noexcept -> double { return -e; }};
    static_assert(can_apply_type<expected<double, int> &, decltype(rv_tag) const &>);
    CHECK(a.apply_type(rv_tag) == 21.0);
    CHECK(e.apply_type(rv_tag) == -7.0);

    CHECK(a.apply_type(arms) == 21.0);
    CHECK(e.apply_type(arms) == -7.0);

    SECTION("constexpr")
    {
      static_assert(expected<double, int>{21.0}.apply_type(arms) == 21.0);
      static_assert(expected<double, int>{fn::unexpect, 7}.apply_type(arms) == -7.0);
      SUCCEED();
    }
  }

  SECTION("value categories")
  {
    CHECK(a.apply_type(fn::overload{
        [](unexpect_t, int) -> bool { throw 1; }, //
        [](in_place_t, double &) -> bool { return true; }, [](in_place_t, double const &) -> bool { throw 0; },
        [](in_place_t, double &&) -> bool { throw 0; }, [](in_place_t, double const &&) -> bool { throw 0; }}));
    CHECK(std::as_const(a).apply_type(fn::overload{
        [](unexpect_t, int) -> bool { throw 1; }, //
        [](in_place_t, double &) -> bool { throw 0; }, [](in_place_t, double const &) -> bool { return true; },
        [](in_place_t, double &&) -> bool { throw 0; }, [](in_place_t, double const &&) -> bool { throw 0; }}));
    CHECK(std::move(std::as_const(a))
              .apply_type(fn::overload{[](unexpect_t, int) -> bool { throw 1; }, //
                                       [](in_place_t, double &) -> bool { throw 0; },
                                       [](in_place_t, double const &) -> bool { throw 0; },
                                       [](in_place_t, double &&) -> bool { throw 0; },
                                       [](in_place_t, double const &&) -> bool { return true; }}));
    CHECK(std::move(a).apply_type(fn::overload{
        [](unexpect_t, int) -> bool { throw 1; }, //
        [](in_place_t, double &) -> bool { throw 0; }, [](in_place_t, double const &) -> bool { throw 0; },
        [](in_place_t, double &&) -> bool { return true; }, [](in_place_t, double const &&) -> bool { throw 0; }}));
    // the error side forwards the same way
    CHECK(e.apply_type(fn::overload{[](in_place_t, double) -> bool { throw 1; }, //
                                    [](unexpect_t, int &) -> bool { return true; },
                                    [](unexpect_t, int const &) -> bool { throw 0; },
                                    [](unexpect_t, int &&) -> bool { throw 0; }}));
    CHECK(std::move(e).apply_type(fn::overload{[](in_place_t, double) -> bool { throw 1; }, //
                                               [](unexpect_t, int &) -> bool { throw 0; },
                                               [](unexpect_t, int const &) -> bool { throw 0; },
                                               [](unexpect_t, int &&) -> bool { return true; }}));

    SECTION("constexpr")
    {
      // one result type across both arms is the rule, so selection is encoded in values
      constexpr expected<double, int> b{21.0};
      constexpr expected<double, int> be{fn::unexpect, 7};
      constexpr auto categories = fn::overload{
          [](in_place_t, double &) -> int { return 1; },     [](in_place_t, double const &) -> int { return 2; },
          [](in_place_t, double &&) -> int { return 3; },    [](in_place_t, double const &&) -> int { return 4; },
          [](unexpect_t, int const &) -> int { return 12; }, [](unexpect_t, int const &&) -> int { return 14; }};
      static_assert(b.apply_type(categories) == 2);
      static_assert(std::move(b).apply_type(categories) == 4);
      static_assert(be.apply_type(categories) == 12);
      static_assert(std::move(be).apply_type(categories) == 14);
      SUCCEED();
    }
  }

  SECTION("void value type")
  {
    // the value arm receives the tag alone
    constexpr auto varms = fn::overload{[](in_place_t) noexcept -> int { return 1; },
                                        [](unexpect_t, int e) noexcept -> int { return -e; }};
    expected<void, int> v{};
    expected<void, int> ve{fn::unexpect, 7};
    CHECK(v.apply_type(varms) == 1);
    CHECK(ve.apply_type(varms) == -7);
    CHECK(v.apply_type_r<long>(varms) == 1L);
    static_assert(not can_apply_type<expected<void, int> &, decltype(fn::overload{[](unexpect_t, int) {}}) const &>);
    static_assert(not can_apply_type<expected<void, int> &, decltype(fn::overload{[](in_place_t) {}}) const &>);

    SECTION("constexpr")
    {
      static_assert(expected<void, int>{}.apply_type(varms) == 1);
      static_assert(expected<void, int>{fn::unexpect, 7}.apply_type(varms) == -7);
      SUCCEED();
    }
  }

  SECTION("pack payload")
  {
    // the arm receives (tag, elements...)
    using P = fn::pack<int, int>;
    expected<P, std::string> p{std::in_place, fn::pack{6, 7}};
    constexpr auto parms = fn::overload{[](in_place_t, int x, int y) noexcept -> int { return x * y; },
                                        [](unexpect_t, std::string const &) noexcept -> int { return -1; }};
    CHECK(p.apply_type(parms) == 42);
    CHECK(expected<P, std::string>{fn::unexpect, "boom"}.apply_type(parms) == -1);
  }

  SECTION("tuple-like payload")
  {
    using T = std::tuple<int, char>;
    expected<T, std::string> t{std::in_place, 40, char(2)};
    constexpr auto tarms = fn::overload{[](in_place_t, int x, char y) noexcept -> int { return x + y; },
                                        [](unexpect_t, std::string const &) noexcept -> int { return -1; }};
    CHECK(t.apply_type(tarms) == 42);

    // the elements form is the row's one signature: an arm for the whole tuple is not served
    static_assert(
        not can_apply_type<expected<T, std::string> &,
                           decltype(fn::overload{[](in_place_t, T const &) -> int { return 0; },
                                                 [](unexpect_t, std::string const &) -> int { return 0; }}) const &>);

    // a tuple-like error unpacks the same way
    using TE = std::tuple<int, char>;
    expected<double, TE> te{fn::unexpect, TE{40, char(2)}};
    CHECK(te.apply_type(fn::overload{[](in_place_t, double) noexcept -> int { return 0; },
                                     [](unexpect_t, int x, char y) noexcept -> int { return x + y; }})
          == 42);
  }

  SECTION("copack error payload")
  {
    // a copack error dispatches under fn::unexpect, and its exhaustiveness composes
    using S = fn::copack_for<bool, std::string>;
    expected<int, S> s{fn::unexpect, S{std::string{"boom"}}};
    constexpr auto sarms = fn::overload{[](in_place_t, int v) noexcept -> int { return v; },
                                        [](unexpect_t, bool) noexcept -> int { return -1; },
                                        [](unexpect_t, std::string const &) noexcept -> int { return -2; }};
    CHECK(s.apply_type(sarms) == -2);
    CHECK(expected<int, S>{42}.apply_type(sarms) == 42);

    constexpr auto no_bool = fn::overload{[](in_place_t, int v) noexcept -> int { return v; },
                                          [](unexpect_t, std::string const &) noexcept -> int { return -2; }};
    static_assert(not can_apply_type<expected<int, S> &, decltype(no_bool) const &>);
  }

  SECTION("extra arguments")
  {
    // trailing arguments follow either arm's content - a void value arm receives (tag, extras...)
    constexpr auto xarms = fn::overload{[](in_place_t, double v, int x) noexcept -> double { return v + x; },
                                        [](unexpect_t, int e, int x) noexcept -> double { return -e - x; }};
    CHECK(a.apply_type(xarms, 2) == 23.0);
    CHECK(e.apply_type(xarms, 2) == -9.0);
    CHECK(a.apply_type_r<long>(xarms, 2) == 23L);
    static_assert(noexcept(a.apply_type(xarms, 2)));

    // an arm set that does not take the extra answers non-viable
    static_assert(not can_apply_type<expected<double, int> &, decltype(arms) const &, int>);
    static_assert(can_apply_type<expected<double, int> &, decltype(xarms) const &, int>);

    constexpr auto varms = fn::overload{[](in_place_t, int x) noexcept -> int { return x; },
                                        [](unexpect_t, int e, int x) noexcept -> int { return -e - x; }};
    expected<void, int> v{};
    CHECK(v.apply_type(varms, 42) == 42);
    CHECK(expected<void, int>{fn::unexpect, 7}.apply_type(varms, 2) == -9);

    SECTION("constexpr")
    {
      static_assert(expected<double, int>{21.0}.apply_type(xarms, 2) == 23.0);
      static_assert(expected<double, int>{fn::unexpect, 7}.apply_type(xarms, 2) == -9.0);
      static_assert(expected<void, int>{}.apply_type(varms, 42) == 42);
      SUCCEED();
    }
  }

  SECTION("apply_type_r")
  {
    static_assert(std::is_same_v<long, decltype(a.apply_type_r<long>(arms))>);
    CHECK(a.apply_type_r<long>(arms) == 21L);
    CHECK(e.apply_type_r<long>(arms) == -7L);

    // the conversion to Ret is part of the question
    static_assert(not can_apply_type_r<expected<double, int> &, char *, decltype(arms) const &>);
    static_assert(can_apply_type_r<expected<double, int> &, long, decltype(arms) const &>);

    SECTION("constexpr")
    {
      static_assert(expected<double, int>{21.0}.apply_type_r<long>(arms) == 21L);
      SUCCEED();
    }
  }
}

namespace {
// Instantiating this callable for any argument is a dependent hard error: a verb that compiles
// while receiving it provably never instantiates its callback.
struct Poison final {
  template <typename T> constexpr void operator()(T &&) const { static_assert(sizeof(T) == 0); }
};

template <typename...> [[maybe_unused]] constexpr bool always_false = false;

// Keyed to the uninhabited row's tag: viable only there, and instantiating its body is a hard
// error - a call that compiles proves the dead row is never even named.
struct PoisonInPlace {
  template <typename... Ts> constexpr int operator()(std::in_place_t, Ts &&...) const
  {
    static_assert(always_false<Ts...>);
    return 0;
  }
};

struct PoisonUnexpect {
  template <typename... Ts> constexpr int operator()(fn::unexpect_t, Ts &&...) const
  {
    static_assert(always_false<Ts...>);
    return 0;
  }
};

template <typename S, typename Fn>
concept can_and_then = requires(S s, Fn fn) { FWD(s).and_then(FWD(fn)); };

template <typename S, typename Fn>
concept can_transform = requires(S s, Fn fn) { FWD(s).transform(FWD(fn)); };

template <typename S, typename Fn>
concept can_or_else = requires(S s, Fn fn) { FWD(s).or_else(FWD(fn)); };

template <typename S, typename Fn>
concept can_transform_error = requires(S s, Fn fn) { FWD(s).transform_error(FWD(fn)); };
} // anonymous namespace

TEST_CASE("expected with empty copack side", "[expected][copack]")
{
  using S0 = fn::copack<>;
  constexpr Poison poison{};

  SECTION("empty copack error: always engaged")
  {
    using E = fn::expected<int, S0>;
    E e{42};
    static_assert(not std::is_constructible_v<E, fn::unexpect_t>);

    // transform_error and or_else short-circuit: the callback is never presented an error - not
    // invoked, not even instantiated - and the result is *this unchanged
    auto r1 = e.transform_error(poison);
    static_assert(std::is_same_v<decltype(r1), E>);
    CHECK(r1.value() == 42);
    auto r2 = std::as_const(e).or_else(poison);
    static_assert(std::is_same_v<decltype(r2), E>);
    CHECK(r2.value() == 42);
    auto r3 = std::move(e).or_else(poison);
    CHECK(r3.value() == 42);
    // asking answers - the defect this case pins down
    static_assert(can_transform_error<E &, Poison const &>);
    static_assert(can_or_else<E &, Poison const &>);

    // the value side is untouched
    CHECK(r3.transform([](int v) { return v + 1; }).value() == 43);

    SECTION("constexpr")
    {
      static_assert(E{42}.transform_error(Poison{}).value() == 42);
      static_assert(E{42}.or_else(Poison{}).value() == 42);
      SUCCEED();
    }
  }

  SECTION("empty copack error, void value")
  {
    using E = fn::expected<void, S0>;
    E e{};
    CHECK(e.transform_error(poison).has_value());
    CHECK(e.or_else(poison).has_value());

    SECTION("constexpr")
    {
      static_assert(E{}.transform_error(Poison{}).has_value());
      static_assert(E{}.or_else(Poison{}).has_value());
      SUCCEED();
    }
  }

  SECTION("apply family over the empty error")
  {
    // the members know in their constraints that the error state is never set: the value arm
    // alone is exhaustive, and the error row is never named
    using E = fn::expected<int, S0>;
    E e{42};
    CHECK(e.apply_type([](std::in_place_t, int i) { return i; }) == 42);
    CHECK(e.apply([](int i) { return i; }) == 42);
    CHECK(e.apply([](int i, int x) { return i + x; }, 2) == 44);
    CHECK(e.apply_r<long>([](int i) { return i; }) == 42L);
    CHECK(e.apply_type_r<long>([](std::in_place_t, int i) { return i; }) == 42L);
    CHECK(e.apply_type([](std::in_place_t, int i, int x) { return i + x; }, 2) == 44);

    // an arm set carrying an arm for the error row compiles without instantiating it
    CHECK(e.apply_type(fn::overload{[](std::in_place_t, int i) { return i; }, PoisonUnexpect{}}) == 42);
    // ... while the value row keeps its requirement
    static_assert(not can_apply_type<E &, decltype(fn::overload{PoisonUnexpect{}}) const &>);

    using V = fn::expected<void, S0>;
    V v{};
    CHECK(v.apply_type([](std::in_place_t) { return 1; }) == 1);
    CHECK(v.apply([]() { return 1; }) == 1);

    SECTION("constexpr")
    {
      static_assert(E{42}.apply_type([](std::in_place_t, int i) { return i; }) == 42);
      static_assert(E{42}.apply([](int i) { return i; }) == 42);
      static_assert(V{}.apply([]() { return 1; }) == 1);
      SUCCEED();
    }
  }

  SECTION("apply family over the empty value")
  {
    using E = fn::expected<S0, int>;
    E e{fn::unexpect, 7};
    CHECK(e.apply_type([](fn::unexpect_t, int v) { return -v; }) == -7);
    CHECK(e.apply([](int v) { return -v; }) == -7);
    CHECK(e.apply_r<long>([](int v) { return -v; }) == -7L);
    CHECK(e.apply_type_r<long>([](fn::unexpect_t, int v) { return -v; }) == -7L);
    CHECK(e.apply_type([](fn::unexpect_t, int v, int x) { return -v - x; }, 2) == -9);
    CHECK(e.apply_type(fn::overload{[](fn::unexpect_t, int v) { return -v; }, PoisonInPlace{}}) == -7);
    static_assert(not can_apply_type<E &, decltype(fn::overload{PoisonInPlace{}}) const &>);

    SECTION("constexpr")
    {
      static_assert(E{fn::unexpect, 7}.apply_type([](fn::unexpect_t, int v) { return -v; }) == -7);
      static_assert(E{fn::unexpect, 7}.apply([](int v) { return -v; }) == -7);
      SUCCEED();
    }
  }

  SECTION("empty copack value: always error")
  {
    using E = fn::expected<S0, int>;
    E e{fn::unexpect, 7};
    static_assert(not std::is_constructible_v<E, std::in_place_t>);

    // and_then and transform short-circuit the same way on the value side
    auto r1 = e.and_then(poison);
    static_assert(std::is_same_v<decltype(r1), E>);
    CHECK(r1.error() == 7);
    auto r2 = std::move(e).transform(poison);
    static_assert(std::is_same_v<decltype(r2), E>);
    CHECK(r2.error() == 7);
    static_assert(can_and_then<E &, Poison const &>);
    static_assert(can_transform<E &, Poison const &>);

    // the error side is untouched
    CHECK(r2.transform_error([](int v) { return v + 1; }).error() == 8);

    SECTION("constexpr")
    {
      static_assert(E{fn::unexpect, 7}.and_then(Poison{}).error() == 7);
      static_assert(E{fn::unexpect, 7}.transform(Poison{}).error() == 7);
      SUCCEED();
    }
  }

  SECTION("widening across an empty copack side")
  {
    // an empty-copack side contributes nothing to the widened copack, and the arm relocating it is never
    // named: or_else may widen away from an empty value, and either verb may take a callback whose
    // own expected carries the empty copack
    using E = fn::expected<S0, int>;
    E e{fn::unexpect, 7};
    constexpr auto widen = [](int v) noexcept -> fn::expected<int, bool> { return {-v}; };
    auto r1 = e.or_else(widen);
    static_assert(std::is_same_v<decltype(r1), fn::expected<fn::copack<int>, bool>>);
    CHECK(r1.value() == fn::copack{-7});
    static_assert(noexcept(e.or_else(widen)));
    static_assert(not noexcept(e.or_else([](int v) -> fn::expected<int, bool> { return {-v}; })));

    // or_else, callback's expected carries the empty copack value
    using X = fn::expected<fn::copack<int>, int>;
    constexpr auto empty_cb
        = [](int v) noexcept -> fn::expected<S0, bool> { return fn::expected<S0, bool>{fn::unexpect, v != 0}; };
    X x{fn::unexpect, 7};
    auto r2 = x.or_else(empty_cb);
    static_assert(std::is_same_v<decltype(r2), fn::expected<fn::copack<int>, bool>>);
    CHECK(r2.error() == true);
    X y{fn::copack<int>{42}};
    CHECK(y.or_else(empty_cb).value() == fn::copack{42});
    static_assert(noexcept(x.or_else(empty_cb)));

    // and_then, callback's expected carries the empty copack error
    using W = fn::expected<int, fn::copack<int>>;
    constexpr auto unit_cb = [](int v) noexcept -> fn::expected<bool, S0> { return {v != 0}; };
    W w{42};
    auto r3 = w.and_then(unit_cb);
    static_assert(std::is_same_v<decltype(r3), fn::expected<bool, fn::copack<int>>>);
    CHECK(r3.value() == true);
    W u{fn::unexpect, fn::copack<int>{13}};
    CHECK(std::move(u).and_then(unit_cb).error() == fn::copack{13});
    static_assert(noexcept(w.and_then(unit_cb)));

    // ... through the void and_then as well
    fn::expected<void, fn::copack<int>> wv{};
    constexpr auto unit_cb0 = []() noexcept -> fn::expected<bool, S0> { return {true}; };
    CHECK(wv.and_then(unit_cb0).value() == true);
    static_assert(noexcept(wv.and_then(unit_cb0)));

    SECTION("constexpr")
    {
      static_assert(E{fn::unexpect, 7}.or_else(widen).value() == fn::copack{-7});
      static_assert(W{42}.and_then(unit_cb).value() == true);
      static_assert(X{fn::unexpect, 7}.or_else(empty_cb).error() == true);
      SUCCEED();
    }
  }
}
