// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/filter.hpp"
#include "static_check.hpp"

#include <catch2/catch_all.hpp>

#include <expected>
#include <optional>
#include <string>
#include <utility>

using namespace util;

namespace {
struct Error {
  std::string what;
};
struct DerivedError : Error {};
struct IncompatibleError {};
} // namespace

TEST_CASE("filter", "[filter][expected][expected_value]")
{
  using namespace fn;

  using operand_t = std::expected<int, Error>;
  constexpr auto truePred = [](int) { return true; };
  constexpr auto falsePred = [](int) { return false; };
  constexpr auto onError
      = [](int v) { return Error{"Got " + std::to_string(v)}; };

  using check_p = static_check::bind_right<filter_t, decltype(onError)>;
  using check_e = static_check::bind_left<filter_t, decltype(truePred)>;

  // lvalue operand
  // --------------
  static_assert(monadic_invocable<filter_t, operand_t, decltype(truePred),
                                  decltype(onError)>);

  static_assert(monadic_invocable<filter_t, operand_t, decltype(falsePred),
                                  decltype(onError)>);

  static_assert(
      check_p::invocable<operand_t>([](auto...) -> bool { throw 0; }));
  static_assert(
      check_e::invocable<operand_t>([](auto...) -> Error { throw 0; }));
  static_assert(
      check_p::invocable<operand_t>([](unsigned) -> bool { throw 0; }));
  static_assert(
      check_e::invocable<operand_t>([](unsigned) -> Error { throw 0; }));
  static_assert(not check_p::invocable<operand_t>([](int &&) -> bool {
    throw 0;
  })); // cannot bind (implicitly) const into non-const rvalue-ref
  static_assert(
      check_e::invocable<operand_t>([](int &&) -> Error { throw 0; }));
  static_assert(not check_p::invocable<operand_t const>(
      [](int &&) -> bool { throw 0; })); // cannot move from const
  static_assert(not check_e::invocable<operand_t const>(
      [](int &&) -> Error { throw 0; })); // cannot move from const
  static_assert(not check_p::invocable<operand_t &>(
      [](int &&) -> bool { throw 0; })); // cannot move from lvalue
  static_assert(not check_e::invocable<operand_t &>(
      [](int &&) -> Error { throw 0; })); // cannot move from lvalue
  static_assert(
      check_p::invocable<operand_t>([](int const &) -> bool { throw 0; }));
  static_assert(check_p::invocable<operand_t const>(
      [](int const &) -> bool { throw 0; }));
  static_assert(
      check_p::invocable<operand_t &>([](int const &) -> bool { throw 0; }));
  static_assert(check_p::invocable<operand_t const &>(
      [](int const &) -> bool { throw 0; }));
  static_assert(
      check_e::invocable<operand_t &>([](int const &) -> Error { throw 0; }));
  static_assert(not check_p::invocable<operand_t &>([](int &) -> bool {
    throw 0;
  })); // cannot bind (implicitly) const into non-const lvalue
  static_assert(
      check_e::invocable<operand_t &>([](int &) -> Error { throw 0; }));
  static_assert(not check_p::invocable<operand_t const &>([](int &) -> bool {
    throw 0;
  })); // cannot bind const into non-const lvalue
  static_assert(not check_e::invocable<operand_t const &>([](int &) -> Error {
    throw 0;
  })); // cannot bind const into non-const lvalue
  static_assert(not check_p::invocable<operand_t>([](int &) -> bool {
    throw 0;
  })); // cannot bind rvalue into non-const lvalue
  static_assert(not check_e::invocable<operand_t>([](int &) -> Error {
    throw 0;
  })); // cannot bind rvalue into non-const lvalue
  static_assert(not check_p::invocable<operand_t>(
      [](std::string) -> bool { throw 0; })); // bad type
  static_assert(not check_e::invocable<operand_t>(
      [](std::string) -> Error { throw 0; })); // bad type
  static_assert(not check_p::invocable<operand_t>(
      []() -> bool { throw 0; })); // bad arity
  static_assert(not check_e::invocable<operand_t>(
      []() -> Error { throw 0; })); // bad arity
  static_assert(not check_p::invocable<operand_t>(
      [](int, int) -> bool { throw 0; })); // bad arity
  static_assert(not check_e::invocable<operand_t>(
      [](int, int) -> Error { throw 0; })); // bad arity
  static_assert(
      check_e::invocable<operand_t>([](int const &&) -> Error { throw 0; }));

  // rvalue operand
  // --------------
  static_assert(
      check_p::invocable<operand_t &&>([](int const &) -> bool { throw 0; }));
  static_assert(
      check_e::invocable<operand_t &&>([](int const &&) -> Error { throw 0; }));
  static_assert(check_p::invocable<operand_t const &&>(
      [](int const &) -> bool { throw 0; }));
  static_assert(check_e::invocable<operand_t const &&>(
      [](int const &&) -> Error { throw 0; }));

  constexpr auto wrong = [](int) -> Error { throw 0; };

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      WHEN("predicate returns true")
      {
        operand_t a{std::in_place, 42};
        using T = decltype(a | filter(truePred, onError));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((a | filter(truePred, onError)).value() == 42);
      }
      WHEN("predicate returns false")
      {
        operand_t a{std::in_place, 42};
        using T = decltype(a | filter(falsePred, onError));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((a | filter(falsePred, onError)).error().what == "Got 42");
      }
    }
    WHEN("operand is error")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | filter(truePred, wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a //
               | filter(truePred, wrong))
                  .error()
                  .what
              == "Not good");
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      WHEN("predicate returns true")
      {
        using T = decltype(operand_t{std::in_place, 42}
                           | filter(truePred, onError));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE(
            (operand_t{std::in_place, 42} | filter(truePred, onError)).value()
            == 42);
      }
      WHEN("predicate returns false")
      {
        using T = decltype(operand_t{std::in_place, 42}
                           | filter(falsePred, onError));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((operand_t{std::in_place, 42} | filter(falsePred, onError))
                    .error()
                    .what
                == "Got 42");
      }
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::unexpect, "Not good"}
                         | filter(truePred, wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::unexpect, "Not good"} //
               | filter(truePred, wrong))
                  .error()
                  .what
              == "Not good");
    }
  }
}

namespace {
struct Value final {
  int v;

  constexpr bool ok() const noexcept { return v < 2; }
  Error error() const { return {"Got " + std::to_string(v)}; }
  Error error_() { return {"Got " + std::to_string(v)}; }
};
} // namespace

TEST_CASE("filter member function",
          "[filter][expected][expected_value][member_functions]")
{
  using namespace fn;

  using operand_t = std::expected<Value, Error>;
  constexpr auto predicate = &Value::ok;
  constexpr auto onError = &Value::error;

  using check_p = static_check::bind_right<filter_t, decltype(onError)>;
  using check_e = static_check::bind_left<filter_t, decltype(predicate)>;

  // lvalue operand
  // --------------
  static_assert(monadic_invocable<filter_t, operand_t, decltype(predicate),
                                  decltype(onError)>);

  static_assert(
      check_p::invocable<operand_t>([](auto...) -> bool { throw 0; }));
  static_assert(
      check_e::invocable<operand_t>([](auto...) -> Error { throw 0; }));
  static_assert(check_p::invocable<operand_t>([](Value) -> bool { throw 0; }));
  static_assert(check_e::invocable<operand_t>([](Value) -> Error { throw 0; }));
  static_assert(not check_p::invocable<operand_t>([](Value &&) -> bool {
    throw 0;
  })); // cannot bind (implicitly) const into non-const rvalue-ref
  static_assert(
      check_e::invocable<operand_t>([](Value &&) -> Error { throw 0; }));
  static_assert(not check_p::invocable<operand_t const>(
      [](Value &&) -> bool { throw 0; })); // cannot move from const
  static_assert(not check_e::invocable<operand_t const>(
      [](Value &&) -> Error { throw 0; })); // cannot move from const
  static_assert(not check_p::invocable<operand_t &>(
      [](Value &&) -> bool { throw 0; })); // cannot move from lvalue
  static_assert(not check_e::invocable<operand_t &>(
      [](Value &&) -> Error { throw 0; })); // cannot move from lvalue
  static_assert(
      check_p::invocable<operand_t>([](Value const &) -> bool { throw 0; }));
  static_assert(check_p::invocable<operand_t const>(
      [](Value const &) -> bool { throw 0; }));
  static_assert(
      check_p::invocable<operand_t &>([](Value const &) -> bool { throw 0; }));
  static_assert(check_p::invocable<operand_t const &>(
      [](Value const &) -> bool { throw 0; }));
  static_assert(
      check_e::invocable<operand_t &>([](Value const &) -> Error { throw 0; }));
  static_assert(not check_p::invocable<operand_t &>([](Value &) -> bool {
    throw 0;
  })); // cannot bind (implicitly) const into non-const lvalue
  static_assert(
      check_e::invocable<operand_t &>([](Value &) -> Error { throw 0; }));
  static_assert(not check_p::invocable<operand_t const &>([](Value &) -> bool {
    throw 0;
  })); // cannot bind const into non-const lvalue
  static_assert(not check_e::invocable<operand_t const &>([](Value &) -> Error {
    throw 0;
  })); // cannot bind const into non-const lvalue
  static_assert(not check_p::invocable<operand_t>([](Value &) -> bool {
    throw 0;
  })); // cannot bind rvalue into non-const lvalue
  static_assert(not check_e::invocable<operand_t>([](Value &) -> Error {
    throw 0;
  })); // cannot bind rvalue into non-const lvalue
  static_assert(not check_p::invocable<operand_t>(
      [](std::string) -> bool { throw 0; })); // bad type
  static_assert(not check_e::invocable<operand_t>(
      [](std::string) -> Error { throw 0; })); // bad type
  static_assert(not check_p::invocable<operand_t>(
      []() -> bool { throw 0; })); // bad arity
  static_assert(not check_e::invocable<operand_t>(
      []() -> Error { throw 0; })); // bad arity
  static_assert(not check_p::invocable<operand_t>(
      [](Value, int) -> bool { throw 0; })); // bad arity
  static_assert(not check_e::invocable<operand_t>(
      [](Value, int) -> Error { throw 0; })); // bad arity
  static_assert(
      check_e::invocable<operand_t>([](Value const &&) -> Error { throw 0; }));

  // rvalue operand
  // --------------
  static_assert(
      check_p::invocable<operand_t &&>([](Value const &) -> bool { throw 0; }));
  static_assert(check_e::invocable<operand_t &&>(
      [](Value const &&) -> Error { throw 0; }));
  static_assert(check_p::invocable<operand_t const &&>(
      [](Value const &) -> bool { throw 0; }));
  static_assert(check_e::invocable<operand_t const &&>(
      [](Value const &&) -> Error { throw 0; }));

  constexpr auto wrong = [](Value) -> Error { throw 0; };

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      WHEN("predicate returns true")
      {
        operand_t a{std::in_place, 1};
        using T = decltype(a | filter(predicate, onError));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((a | filter(predicate, onError)).value().v == 1);
        REQUIRE((a | filter(predicate, &Value::error_)).value().v == 1);
      }
      WHEN("predicate returns false")
      {
        operand_t a{std::in_place, 42};
        using T = decltype(a | filter(predicate, onError));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((a | filter(predicate, onError)).error().what == "Got 42");
        REQUIRE((a | filter(predicate, &Value::error_)).error().what
                == "Got 42");
      }
    }
    WHEN("operand is error")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | filter(predicate, wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a //
               | filter(predicate, wrong))
                  .error()
                  .what
              == "Not good");
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      WHEN("predicate returns true")
      {
        using T = decltype(operand_t{std::in_place, 1}
                           | filter(predicate, onError));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE(
            (operand_t{std::in_place, 1} | filter(predicate, onError)).value().v
            == 1);
        REQUIRE(
            (operand_t{std::in_place, 1} | filter(predicate, &Value::error_))
                .value()
                .v
            == 1);
      }
      WHEN("predicate returns false")
      {
        using T = decltype(operand_t{std::in_place, 42}
                           | filter(predicate, onError));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((operand_t{std::in_place, 42} | filter(predicate, onError))
                    .error()
                    .what
                == "Got 42");
        REQUIRE(
            (operand_t{std::in_place, 42} | filter(predicate, &Value::error_))
                .error()
                .what
            == "Got 42");
      }
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::unexpect, "Not good"}
                         | filter(predicate, wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::unexpect, "Not good"} //
               | filter(predicate, wrong))
                  .error()
                  .what
              == "Not good");
    }
  }
}

TEST_CASE("filter", "[filter][expected][expected_void]")
{
  using namespace fn;

  using operand_t = std::expected<void, Error>;
  auto truePred = [] { return true; };
  auto falsePred = [] { return false; };
  auto onError = [] { return Error{"Got error"}; };

  using check_p = static_check::bind_right<filter_t, decltype(onError)>;
  using check_e = static_check::bind_left<filter_t, decltype(truePred)>;

  // lvalue operand
  // --------------
  static_assert(monadic_invocable<filter_t, operand_t, decltype(truePred),
                                  decltype(onError)>);

  static_assert(monadic_invocable<filter_t, operand_t, decltype(falsePred),
                                  decltype(onError)>);

  static_assert(check_p::invocable<operand_t>(
      [](auto...) -> bool { throw 0; })); // allow generic call
  static_assert(check_e::invocable<operand_t>(
      [](auto...) -> Error { throw 0; })); // allow generic call
  static_assert(not check_p::invocable<operand_t>(
      [](auto) -> bool { throw 0; })); // wrong arity
  static_assert(not check_e::invocable<operand_t>(
      [](auto) -> Error { throw 0; })); // wrong arity

  // rvalue operand
  // --------------
  static_assert(monadic_invocable<filter_t, operand_t &&, decltype(truePred),
                                  decltype(onError)>);

  static_assert(monadic_invocable<filter_t, operand_t &&, decltype(falsePred),
                                  decltype(onError)>);

  static_assert(check_p::invocable<operand_t &&>(
      [](auto...) -> bool { throw 0; })); // allow generic call
  static_assert(check_e::invocable<operand_t &&>(
      [](auto...) -> Error { throw 0; })); // allow generic call
  static_assert(not check_p::invocable<operand_t &&>(
      [](auto) -> bool { throw 0; })); // wrong arity
  static_assert(not check_e::invocable<operand_t &&>(
      [](auto) -> Error { throw 0; })); // wrong arity

  constexpr auto wrong = []() -> Error { throw 0; };

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      WHEN("predicate returns true")
      {
        operand_t a{std::in_place};
        using T = decltype(a | filter(truePred, onError));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((a | filter(truePred, onError)).has_value());
      }
      WHEN("predicate returns false")
      {
        operand_t a{std::in_place};
        using T = decltype(a | filter(falsePred, onError));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((a | filter(falsePred, onError)).error().what == "Got error");
      }
    }
    WHEN("operand is error")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | filter(truePred, wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a //
               | filter(truePred, wrong))
                  .error()
                  .what
              == "Not good");
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      WHEN("predicate returns true")
      {
        using T
            = decltype(operand_t{std::in_place} | filter(truePred, onError));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE(
            (operand_t{std::in_place} | filter(truePred, onError)).has_value());
      }
      WHEN("predicate returns false")
      {
        using T
            = decltype(operand_t{std::in_place} | filter(falsePred, onError));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE(
            (operand_t{std::in_place} | filter(falsePred, onError)).error().what
            == "Got error");
      }
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::unexpect, "Not good"}
                         | filter(truePred, wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::unexpect, "Not good"} //
               | filter(truePred, wrong))
                  .error()
                  .what
              == "Not good");
    }
  }
}

TEST_CASE("filter", "[filter][optional]")
{
  using namespace fn;

  using operand_t = std::optional<int>;
  auto truePred = [](int) { return true; };
  auto falsePred = [](int) { return false; };

  using check = static_check::bind_left<filter_t>;

  // lvalue operand
  // --------------
  static_assert(monadic_invocable<filter_t, operand_t, decltype(truePred)>);
  static_assert(check::invocable<operand_t>(
      [](auto...) -> bool { throw 0; })); // allow generic call
  static_assert(check::invocable<operand_t>(
      [](int const &) -> bool { throw 0; })); // allow binding to const lvalue
  static_assert(not check::invocable<operand_t>(
      [](int &) -> bool { throw 0; })); // disallow binding to lvalue

  // rvalue operand
  // --------------
  static_assert(monadic_invocable<filter_t, operand_t &&, decltype(truePred)>);
  static_assert(check::invocable<operand_t &&>(
      [](auto...) -> bool { throw 0; })); // allow generic call
  static_assert(check::invocable<operand_t &&>(
      [](auto...) -> bool { throw 0; })); // allow generic call
  static_assert(not check::invocable<operand_t &&>(
      [](int &) -> bool { throw 0; })); // disallow binding to lvalue
  static_assert(check::invocable<operand_t &&>(
      [](int const &) -> bool { throw 0; })); // allow binding to const lvalue

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      WHEN("predicate returns true")
      {
        operand_t a{42};
        using T = decltype(a | filter(truePred));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((a | filter(truePred)).has_value());
      }
      WHEN("predicate returns false")
      {
        operand_t a{42};
        using T = decltype(a | filter(falsePred));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE(not(a | filter(falsePred)).has_value());
      }
    }
    WHEN("operand is nullopt")
    {
      operand_t a{std::nullopt};
      using T = decltype(a | filter(truePred));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(not(a | filter(truePred)).has_value());
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      WHEN("predicate returns true")
      {
        using T = decltype(operand_t{42} | filter(truePred));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((operand_t{42} | filter(truePred)).has_value());
      }
      WHEN("predicate returns false")
      {
        using T = decltype(operand_t{42} | filter(falsePred));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE(not(operand_t{42} | filter(falsePred)).has_value());
      }
    }
    WHEN("operand is nullopt")
    {
      using T = decltype(operand_t{std::nullopt} | filter(truePred));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(not(operand_t{std::nullopt} //
                  | filter(truePred))
                     .has_value());
    }
  }
}

TEST_CASE("filter member function", "[filter][optional]")
{
  using namespace fn;

  using operand_t = std::optional<Value>;
  constexpr auto predicate = &Value::ok;

  using check = static_check::bind_left<filter_t>;

  // lvalue operand
  // --------------
  static_assert(monadic_invocable<filter_t, operand_t, decltype(predicate)>);

  static_assert(check::invocable<operand_t>(
      [](auto...) -> bool { throw 0; })); // allow generic call
  static_assert(check::invocable<operand_t>(
      [](Value const &) -> bool { throw 0; })); // allow binding to const lvalue
  static_assert(not check::invocable<operand_t>(
      [](Value &) -> bool { throw 0; })); // disallow binding to lvalue

  // rvalue operand
  // --------------
  static_assert(monadic_invocable<filter_t, operand_t &&, decltype(predicate)>);
  static_assert(check::invocable<operand_t &&>(
      [](auto...) -> bool { throw 0; })); // allow generic call
  static_assert(not check::invocable<operand_t &&>(
      [](Value &) -> bool { throw 0; })); // disallow binding to lvalue
  static_assert(check::invocable<operand_t &&>(
      [](Value const &) -> bool { throw 0; })); // allow binding to const lvalue

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      WHEN("predicate returns true")
      {
        operand_t a{1};
        using T = decltype(a | filter(predicate));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((a | filter(predicate)).value().v == 1);
      }
      WHEN("predicate returns false")
      {
        operand_t a{42};
        using T = decltype(a | filter(predicate));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE(not(a | filter(predicate)).has_value());
      }
    }
    WHEN("operand is nullopt")
    {
      operand_t a{std::nullopt};
      using T = decltype(a | filter(predicate));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(not(a | filter(predicate)).has_value());
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      WHEN("predicate returns true")
      {
        using T = decltype(operand_t{1} | filter(predicate));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((operand_t{1} | filter(predicate)).value().v == 1);
      }
      WHEN("predicate returns false")
      {
        using T = decltype(operand_t{42} | filter(predicate));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE(not(operand_t{42} | filter(predicate)).has_value());
      }
    }
    WHEN("operand is nullopt")
    {
      using T = decltype(operand_t{std::nullopt} | filter(predicate));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(not(operand_t{std::nullopt} //
                  | filter(predicate))
                     .has_value());
    }
  }
}

TEST_CASE("constexpr filter expected", "[filter][constexpr][expected]")
{
  enum class Error { ThresholdExceeded, SomethingElse };
  using T = std::expected<int, Error>;

  constexpr auto fn = [](int i) constexpr noexcept -> bool { return i < 3; };
  constexpr auto error = [](int) -> Error { return Error::ThresholdExceeded; };
  constexpr auto r1 = T{0} | fn::filter(fn, error);
  static_assert(r1.value() == 0);
  constexpr auto r2 = T{3} | fn::filter(fn, error);
  static_assert(r2.error() == Error::ThresholdExceeded);

  SUCCEED();
}

TEST_CASE("constexpr filter optional", "[filter][constexpr][optional]")
{
  using T = std::optional<int>;

  constexpr auto fn = [](int i) constexpr noexcept -> bool { return i < 3; };
  constexpr auto r1 = T{0} | fn::filter(fn);
  static_assert(r1.value() == 0);
  constexpr auto r2 = T{3} | fn::filter(fn);
  static_assert(not r2.has_value());

  SUCCEED();
}
