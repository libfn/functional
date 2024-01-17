// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/or_else.hpp"
#include "functional/functor.hpp"

#include <catch2/catch_all.hpp>

#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace {
struct Error final {
  std::string what;

  operator std::string_view() const { return what; }
};
} // namespace

TEST_CASE("or_else", "[or_else][expected][expected_value]")
{
  using namespace fn;

  using operand_t = std::expected<int, Error>;
  constexpr auto fnError = [](Error e) -> operand_t { return {e.what.size()}; };

  static_assert(monadic_invocable<or_else_t, operand_t, decltype(fnError)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<or_else_t, operand_t, decltype(fn)>;
  }([](auto...) -> operand_t { throw 0; })); // allow generic call
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<or_else_t, operand_t, decltype(fn)>;
  }([](std::string_view) -> operand_t { throw 0; })); // allow conversion
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<or_else_t, operand_t, decltype(fn)>;
  }([](Error &&) -> operand_t { throw 0; })); // alow move from rvalue
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<or_else_t, operand_t const, decltype(fn)>;
  }([](Error &&) -> operand_t { throw 0; })); // disallow removing const
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<or_else_t, operand_t &, decltype(fn)>;
  }([](Error &&) -> operand_t { throw 0; })); // disallow move from lvalue
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<or_else_t, operand_t &, decltype(fn)>;
  }([](Error &) -> operand_t { throw 0; })); // allow lvalue binding
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<or_else_t, operand_t const &, decltype(fn)>;
  }([](Error &) -> operand_t { throw 0; })); // disallow removing const
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<or_else_t, operand_t, decltype(fn)>;
  }([](Error &) -> operand_t {
    throw 0;
  })); // disallow lvalue binding to rvalue

  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<or_else_t, operand_t, decltype(fn)>;
  }([](int) {})); // wrong type
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<or_else_t, operand_t, decltype(fn)>;
  }([]() {})); // wrong arity
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<or_else_t, operand_t, decltype(fn)>;
  }([](int, int) {})); // wrong arity

  constexpr auto wrong = [](Error) -> operand_t { throw 0; };
  constexpr auto fnFail = [](Error v) -> operand_t {
    return std::unexpected<Error>("Got: " + v.what);
  };

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place, 12};
      using T = decltype(a | or_else(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | or_else(wrong)).value() == 12);
    }
    WHEN("operand is error")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | or_else(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | or_else(fnError)).value() == 8);

      WHEN("fail")
      {
        using T = decltype(a | or_else(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((a | or_else(fnFail)).error().what == "Got: Not good");
      }
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place, 12} | or_else(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::in_place, 12} | or_else(wrong)).value() == 12);
    }
    WHEN("operand is error")
    {
      using T
          = decltype(operand_t{std::unexpect, "Not good"} | or_else(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::unexpect, "Not good"} | or_else(fnError)).value()
              == 8);

      WHEN("fail")
      {
        using T
            = decltype(operand_t{std::unexpect, "Not good"} | or_else(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((operand_t{std::unexpect, "Not good"} //
                 | or_else(fnFail))
                    .error()
                    .what
                == "Got: Not good");
      }
    }
  }
}

TEST_CASE("or_else", "[or_else][expected][expected_void]")
{
  using namespace fn;

  using operand_t = std::expected<void, Error>;
  int count = 0;
  auto fnError = [&count](Error) -> operand_t {
    count += 1;
    return {};
  };

  static_assert(monadic_invocable<or_else_t, operand_t, decltype(fnError)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<or_else_t, operand_t, decltype(fn)>;
  }([](auto...) -> operand_t { return {}; })); // allow generic call
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<or_else_t, operand_t, decltype(fn)>;
  }([](std::string_view) -> operand_t { return {}; })); // allow conversion
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<or_else_t, operand_t, decltype(fn)>;
  }([](Error &&) -> operand_t { return {}; })); // alow move from rvalue
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<or_else_t, operand_t const, decltype(fn)>;
  }([](Error &&) -> operand_t { return {}; })); // disallow removing const
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<or_else_t, operand_t &, decltype(fn)>;
  }([](Error &&) -> operand_t { return {}; })); // disallow move from lvalue
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<or_else_t, operand_t &, decltype(fn)>;
  }([](Error &) -> operand_t { return {}; })); // allow lvalue binding
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<or_else_t, operand_t const &, decltype(fn)>;
  }([](Error &) -> operand_t { return {}; })); // disallow removing const
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<or_else_t, operand_t, decltype(fn)>;
  }([](Error &) -> operand_t {
    return {};
  })); // disallow lvalue binding to rvalue

  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<or_else_t, operand_t, decltype(fn)>;
  }([](int) {})); // wrong type
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<or_else_t, operand_t, decltype(fn)>;
  }([]() {})); // wrong arity
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<or_else_t, operand_t, decltype(fn)>;
  }([](int, int) {})); // wrong arity

  constexpr auto wrong = [](Error) -> operand_t { throw 0; };
  constexpr auto fnFail = [](Error v) -> operand_t {
    return std::unexpected<Error>("Got: " + v.what);
  };

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place};
      using T = decltype(a | or_else(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      (a | or_else(wrong)).value();
      CHECK(count == 0);
    }
    WHEN("operand is error")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | or_else(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      (a | or_else(fnError)).value();
      CHECK(count == 1);

      WHEN("fail")
      {
        using T = decltype(a | or_else(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((a | or_else(fnFail)).error().what == "Got: Not good");
        CHECK(count == 1);
      }
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place} | or_else(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      (operand_t{std::in_place} | or_else(wrong)).value();
      CHECK(count == 0);
    }
    WHEN("operand is error")
    {
      using T
          = decltype(operand_t{std::unexpect, "Not good"} | or_else(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      (operand_t{std::unexpect, "Not good"} | or_else(fnError)).value();

      WHEN("fail")
      {
        using T
            = decltype(operand_t{std::unexpect, "Not good"} | or_else(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((operand_t{std::unexpect, "Not good"} //
                 | or_else(fnFail))
                    .error()
                    .what
                == "Got: Not good");
      }
    }
  }
}

TEST_CASE("or_else", "[or_else][optional]")
{
  using namespace fn;

  using operand_t = std::optional<int>;
  constexpr auto fnError = []() -> operand_t { return {42}; };

  static_assert(monadic_invocable<or_else_t, operand_t, decltype(fnError)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<or_else_t, operand_t, decltype(fn)>;
  }([](auto...) -> operand_t { throw 0; })); // allow generic call
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<or_else_t, operand_t, decltype(fn)>;
  }([](auto) {})); // wrong arity

  constexpr auto wrong = []() -> operand_t { throw 0; };
  constexpr auto fnFail = []() -> operand_t { return {}; };

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{12};
      using T = decltype(a | or_else(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | or_else(wrong)).value() == 12);
    }
    WHEN("operand is error")
    {
      operand_t a{std::nullopt};
      using T = decltype(a | or_else(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | or_else(fnError)).value() == 42);

      WHEN("fail")
      {
        using T = decltype(a | or_else(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE(not(a | or_else(fnFail)).has_value());
      }
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{12} | or_else(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{12} | or_else(wrong)).value() == 12);
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::nullopt} | or_else(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::nullopt} | or_else(fnError)).value() == 42);

      WHEN("fail")
      {
        using T = decltype(operand_t{std::nullopt} | or_else(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE(not(operand_t{std::nullopt} | or_else(fnFail)).has_value());
      }
    }
  }
}
