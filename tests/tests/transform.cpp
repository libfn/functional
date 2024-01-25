// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/transform.hpp"
#include "functional/functor.hpp"

#include <catch2/catch_all.hpp>

#include <expected>
#include <optional>
#include <string>
#include <utility>

namespace {
struct Error final {
  std::string what;
};

struct Xint final {
  int value;
};
} // namespace

TEST_CASE("transform", "[transform][expected][expected_value]")
{
  using namespace fn;

  using operand_t = std::expected<int, Error>;
  constexpr auto fnValue = [](int i) -> int { return i + 1; };

  static_assert(monadic_invocable<transform_t, operand_t, decltype(fnValue)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<transform_t, operand_t, decltype(fn)>;
  }([](auto...) -> int { return 0; })); // allow generic call
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<transform_t, operand_t, decltype(fn)>;
  }([](unsigned) -> int { return 0; })); // allow conversion
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<transform_t, operand_t, decltype(fn)>;
  }([](int &&) -> int { return 0; })); // alow move from rvalue
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<transform_t, operand_t const, decltype(fn)>;
  }([](int &&) -> int { return 0; })); // disallow removing const
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<transform_t, operand_t &, decltype(fn)>;
  }([](int &&) -> int { return 0; })); // disallow move from lvalue
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<transform_t, operand_t &, decltype(fn)>;
  }([](int &) -> int { return 0; })); // allow lvalue binding
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<transform_t, operand_t const &, decltype(fn)>;
  }([](int &) -> int { return 0; })); // disallow removing const
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<transform_t, operand_t, decltype(fn)>;
  }([](int &) -> int { return 0; })); // disallow lvalue binding to rvalue

  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<transform_t, operand_t, decltype(fn)>;
  }([](std::string) {})); // wrong type
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<transform_t, operand_t, decltype(fn)>;
  }([]() {})); // wrong arity
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<transform_t, operand_t, decltype(fn)>;
  }([](int, int) {})); // wrong arity

  constexpr auto wrong = [](int) -> int { throw 0; };
  constexpr auto fnXabs = [](int i) -> Xint { return {std::abs(8 - i)}; };

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place, 12};
      using T = decltype(a | transform(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | transform(fnValue)).value() == 13);

      WHEN("change type")
      {
        using T = decltype(a | transform(fnXabs));
        static_assert(std::is_same_v<T, std::expected<Xint, Error>>);
        REQUIRE((a | transform(fnXabs)).value().value == 4);
      }
    }
    WHEN("operand is error")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | transform(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a //
               | transform(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place, 12} | transform(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::in_place, 12} | transform(fnValue)).value()
              == 13);

      WHEN("change type")
      {
        using T = decltype(operand_t{std::in_place, 12} | transform(fnXabs));
        static_assert(std::is_same_v<T, std::expected<Xint, Error>>);
        REQUIRE((operand_t{std::in_place, 12} | transform(fnXabs)).value().value
                == 4);
      }
    }
    WHEN("operand is error")
    {
      using T
          = decltype(operand_t{std::unexpect, "Not good"} | transform(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::unexpect, "Not good"} //
               | transform(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }
}

TEST_CASE("transform", "[transform][expected][expected_void]")
{
  using namespace fn;

  using operand_t = std::expected<void, Error>;
  int count = 0;
  auto fnValue = [&count]() -> void { count += 1; };

  static_assert(monadic_invocable<transform_t, operand_t, decltype(fnValue)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<transform_t, operand_t, decltype(fn)>;
  }([](auto...) -> int { return 0; })); // allow generic call
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<transform_t, operand_t, decltype(fn)>;
  }([]() -> Xint { return {0}; })); // allow conversion

  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<transform_t, operand_t, decltype(fn)>;
  }([](auto) {})); // wrong arity
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<transform_t, operand_t, decltype(fn)>;
  }([](auto, auto) {})); // wrong arity

  constexpr auto wrong = []() -> void { throw 0; };
  constexpr auto fnXabs = []() -> Xint { return {42}; };

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place};
      using T = decltype(a | transform(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      (a | transform(fnValue)).value();
      REQUIRE(count == 1);

      WHEN("change type")
      {
        using T = decltype(a | transform(fnXabs));
        static_assert(std::is_same_v<T, std::expected<Xint, Error>>);
        REQUIRE((a | transform(fnXabs)).value().value == 42);
      }
    }
    WHEN("operand is error")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | transform(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a //
               | transform(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place} | transform(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      (operand_t{std::in_place} | transform(fnValue)).value();
      REQUIRE(count == 1);

      WHEN("change type")
      {
        using T = decltype(operand_t{std::in_place} | transform(fnXabs));
        static_assert(std::is_same_v<T, std::expected<Xint, Error>>);
        REQUIRE((operand_t{std::in_place} | transform(fnXabs)).value().value
                == 42);
      }
    }
    WHEN("operand is error")
    {
      using T
          = decltype(operand_t{std::unexpect, "Not good"} | transform(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::unexpect, "Not good"} //
               | transform(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }
}

TEST_CASE("transform", "[transform][optional]")
{
  using namespace fn;

  using operand_t = std::optional<int>;
  constexpr auto fnValue = [](int i) -> int { return i + 1; };

  static_assert(monadic_invocable<transform_t, operand_t, decltype(fnValue)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<transform_t, operand_t, decltype(fn)>;
  }([](auto...) -> int { return 0; })); // allow generic call
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<transform_t, operand_t, decltype(fn)>;
  }([](unsigned) -> int { return 0; })); // allow conversion
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<transform_t, operand_t, decltype(fn)>;
  }([](int &&) -> int { return 0; })); // alow move from rvalue
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<transform_t, operand_t const, decltype(fn)>;
  }([](int &&) -> int { return 0; })); // disallow removing const
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<transform_t, operand_t &, decltype(fn)>;
  }([](int &&) -> int { return 0; })); // disallow move from lvalue
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<transform_t, operand_t &, decltype(fn)>;
  }([](int &) -> int { return 0; })); // allow lvalue binding
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<transform_t, operand_t const &, decltype(fn)>;
  }([](int &) -> int { return 0; })); // disallow removing const
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<transform_t, operand_t, decltype(fn)>;
  }([](int &) -> int { return 0; })); // disallow lvalue binding to rvalue

  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<transform_t, operand_t, decltype(fn)>;
  }([](std::string) {})); // wrong type
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<transform_t, operand_t, decltype(fn)>;
  }([]() {})); // wrong arity
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<transform_t, operand_t, decltype(fn)>;
  }([](int, int) {})); // wrong arity

  constexpr auto wrong = [](int) -> int { throw 0; };
  constexpr auto fnXabs = [](int i) -> Xint { return {std::abs(8 - i)}; };

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{12};
      using T = decltype(a | transform(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | transform(fnValue)).value() == 13);

      WHEN("change type")
      {
        using T = decltype(a | transform(fnXabs));
        static_assert(std::is_same_v<T, std::optional<Xint>>);
        REQUIRE((a | transform(fnXabs)).value().value == 4);
      }
    }
    WHEN("operand is error")
    {
      operand_t a{std::nullopt};
      using T = decltype(a | transform(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(not(a | transform(wrong)).has_value());
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{12} | transform(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{12} | transform(fnValue)).value() == 13);

      WHEN("change type")
      {
        using T = decltype(operand_t{12} | transform(fnXabs));
        static_assert(std::is_same_v<T, std::optional<Xint>>);
        REQUIRE((operand_t{12} | transform(fnXabs)).value().value == 4);
      }
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::nullopt} | transform(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(not(operand_t{std::nullopt} //
                  | transform(wrong))
                     .has_value());
    }
  }
}

TEST_CASE("constexpr transform expected", "[transform][constexpr][expected]")
{
  enum class Error { ThresholdExceeded, SomethingElse };
  using T = std::expected<int, Error>;

  WHEN("same value type")
  {
    constexpr auto fn = [](int i) constexpr noexcept -> int {
      if (i < 2)
        return i + 1;
      return i;
    };
    constexpr auto r1 = T{0} | fn::transform(fn);
    static_assert(r1.value() == 1);
    constexpr auto r2
        = r1 | fn::transform(fn) | fn::transform(fn) | fn::transform(fn);
    static_assert(r2.value() == 2);
    constexpr auto r3
        = T{std::unexpect, Error::SomethingElse} | fn::transform(fn);
    static_assert(r3.error() == Error::SomethingElse);
  }

  WHEN("different value type")
  {
    constexpr auto fn = [](int i) constexpr noexcept -> bool {
      if (i == 1)
        return true;
      return false;
    };
    constexpr auto r1 = T{1} | fn::transform(fn);
    static_assert(
        std::is_same_v<decltype(r1), std::expected<bool, Error> const>);
    static_assert(r1.value() == true);
    constexpr auto r2 = T{0} | fn::transform(fn);
    static_assert(r2.value() == false);
    constexpr auto r3 = T{2} | fn::transform(fn);
    static_assert(r3.value() == false);
    constexpr auto r4
        = T{std::unexpect, Error::SomethingElse} | fn::transform(fn);
    static_assert(r4.error() == Error::SomethingElse);
  }

  SUCCEED();
}

TEST_CASE("constexpr transform optional", "[transform][constexpr][optional]")
{
  using T = std::optional<int>;

  WHEN("same value type")
  {
    constexpr auto fn = [](int i) constexpr noexcept -> int {
      if (i < 2)
        return i + 1;
      return i;
    };
    constexpr auto r1 = T{0} | fn::transform(fn);
    static_assert(r1.value() == 1);
    constexpr auto r2
        = r1 | fn::transform(fn) | fn::transform(fn) | fn::transform(fn);
    static_assert(r2.value() == 2);
    constexpr auto r4 = T{} | fn::transform(fn);
    static_assert(not r4.has_value());
  }

  WHEN("different value type")
  {
    constexpr auto fn1 = [](int i) constexpr noexcept -> bool {
      if (i == 1)
        return true;
      return false;
    };
    constexpr auto r1 = T{1} | fn::transform(fn1);
    static_assert(std::is_same_v<decltype(r1), std::optional<bool> const>);
    static_assert(r1.value() == true);
    constexpr auto r2 = T{0} | fn::transform(fn1);
    static_assert(r2.value() == false);
    constexpr auto r3 = T{2} | fn::transform(fn1);
    static_assert(r3.value() == false);
    constexpr auto r4 = T{} | fn::transform(fn1);
    static_assert(not r4.has_value());
  }

  SUCCEED();
}
