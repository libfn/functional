// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/and_then.hpp"
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
} // namespace

TEST_CASE("and_then", "[and_then][expected]")
{
  using namespace fn;

  using operand_t = std::expected<int, Error>;
  constexpr auto fnValue = [](int i) -> operand_t { return {i + 1}; };

  static_assert(monadic_invocable<and_then_t, operand_t, decltype(fnValue)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<and_then_t, operand_t, decltype(fn)>;
  }([](auto...) -> operand_t { throw 0; })); // allow generic call
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<and_then_t, operand_t, decltype(fn)>;
  }([](unsigned) -> operand_t { throw 0; })); // allow conversion
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<and_then_t, operand_t, decltype(fn)>;
  }([](std::string) {})); // wrong type
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<and_then_t, operand_t, decltype(fn)>;
  }([]() {})); // wrong arity
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<and_then_t, operand_t, decltype(fn)>;
  }([](int, int) {})); // wrong arity

  constexpr auto wrong = [](int) -> operand_t { throw 0; };
  constexpr auto fnFail = [](int i) -> operand_t {
    return std::unexpected<Error>("Got " + std::to_string(i));
  };
  constexpr auto fnXabs
      = [](int i) -> std::expected<unsigned, Error> { return std::abs(8 - i); };

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place, 12};
      using T = decltype(a | and_then(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | and_then(fnValue)).value() == 13);

      WHEN("fail")
      {
        using T = decltype(a | and_then(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((a | and_then(fnFail)).error().what == "Got 12");
      }

      WHEN("change type")
      {
        using T = decltype(a | and_then(fnXabs));
        static_assert(std::is_same_v<T, std::expected<unsigned, Error>>);
        REQUIRE((a | and_then(fnXabs)).value() == 4);
      }
    }
    WHEN("operand is error")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | and_then(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a //
               | and_then(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place, 12} | and_then(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::in_place, 12} | and_then(fnValue)).value() == 13);

      WHEN("fail")
      {
        using T = decltype(operand_t{std::in_place, 12} | and_then(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((operand_t{std::in_place, 12} | and_then(fnFail)).error().what
                == "Got 12");
      }

      WHEN("change type")
      {
        using T = decltype(operand_t{std::in_place, 12} | and_then(fnXabs));
        static_assert(std::is_same_v<T, std::expected<unsigned, Error>>);
        REQUIRE((operand_t{std::in_place, 12} | and_then(fnXabs)).value() == 4);
      }
    }
    WHEN("operand is error")
    {
      using T
          = decltype(operand_t{std::unexpect, "Not good"} | and_then(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::unexpect, "Not good"} //
               | and_then(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }
}

TEST_CASE("and_then", "[and_then][optional]")
{
  using namespace fn;

  using operand_t = std::optional<int>;
  constexpr auto fnValue = [](int i) -> operand_t { return {i + 1}; };

  static_assert(monadic_invocable<and_then_t, operand_t, decltype(fnValue)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<and_then_t, operand_t, decltype(fn)>;
  }([](auto...) -> operand_t { throw 0; })); // allow generic call
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<and_then_t, operand_t, decltype(fn)>;
  }([](unsigned) -> operand_t { throw 0; })); // allow conversion
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<and_then_t, operand_t, decltype(fn)>;
  }([](std::string) {})); // wrong type
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<and_then_t, operand_t, decltype(fn)>;
  }([]() {})); // wrong arity
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<and_then_t, operand_t, decltype(fn)>;
  }([](int, int) {})); // wrong arity

  constexpr auto wrong = [](int) -> operand_t { throw 0; };
  constexpr auto fnFail = [](int) -> operand_t { return {}; };
  constexpr auto fnXabs
      = [](int i) -> std::optional<unsigned> { return std::abs(8 - i); };

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{12};
      using T = decltype(a | and_then(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | and_then(fnValue)).value() == 13);

      WHEN("fail")
      {
        using T = decltype(a | and_then(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE(not(a | and_then(fnFail)).has_value());
      }

      WHEN("change type")
      {
        using T = decltype(a | and_then(fnXabs));
        static_assert(std::is_same_v<T, std::optional<unsigned>>);
        REQUIRE((a | and_then(fnXabs)).value() == 4);
      }
    }
    WHEN("operand is error")
    {
      operand_t a{std::nullopt};
      using T = decltype(a | and_then(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(not(a | and_then(wrong)).has_value());
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{12} | and_then(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{12} | and_then(fnValue)).value() == 13);

      WHEN("fail")
      {
        using T = decltype(operand_t{12} | and_then(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE(not(operand_t{12} | and_then(fnFail)).has_value());
      }

      WHEN("change type")
      {
        using T = decltype(operand_t{12} | and_then(fnXabs));
        static_assert(std::is_same_v<T, std::optional<unsigned>>);
        REQUIRE((operand_t{12} | and_then(fnXabs)).value() == 4);
      }
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::nullopt} | and_then(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(not(operand_t{std::nullopt} //
                  | and_then(wrong))
                     .has_value());
    }
  }
}
