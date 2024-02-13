// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "static_check.hpp"

#include "functional/filter.hpp"

#include <catch2/catch_all.hpp>

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

  constexpr auto truePred = [](int) { return true; };
  constexpr auto falsePred = [](int) { return false; };
  constexpr auto onError = [](int v) { return Error{"Got " + std::to_string(v)}; };
  constexpr auto wrong = [](int) -> Error { throw 0; };

  using operand_t = fn::expected<int, Error>;
  using p_is = monadic_static_check<filter_t, operand_t>::bind_right<decltype(onError)>;
  using e_is = monadic_static_check<filter_t, operand_t>::bind_left<decltype(truePred)>;

  static_assert(p_is::invocable_with_any(truePred));
  static_assert(p_is::invocable_with_any([](auto...) -> bool { throw 0; }));         // allow generic call
  static_assert(p_is::invocable_with_any([](int) -> bool { throw 0; }));             // allow copy
  static_assert(p_is::invocable_with_any([](unsigned) -> bool { throw 0; }));        // allow conversion
  static_assert(p_is::invocable_with_any([](int const &) -> bool { throw 0; }));     // binds to const ref
  static_assert(p_is::not_invocable_with_any([](int &) -> bool { throw 0; }));       // cannot bind lvalue
  static_assert(p_is::not_invocable_with_any([](int &&) -> bool { throw 0; }));      // cannot move
  static_assert(p_is::not_invocable_with_any([](std::string) -> bool { throw 0; })); // bad type
  static_assert(p_is::not_invocable_with_any([]() -> bool { throw 0; }));            // bad arity
  static_assert(p_is::not_invocable_with_any([](int, int) -> bool { throw 0; }));    // bad arity

  static_assert(e_is::invocable_with_any(onError));
  static_assert(e_is::invocable_with_any([](auto...) -> Error { throw 0; }));              // allow generic call
  static_assert(e_is::invocable_with_any([](int) -> Error { throw 0; }));                  // allow copy
  static_assert(e_is::invocable_with_any([](unsigned) -> Error { throw 0; }));             // allow conversion
  static_assert(e_is::invocable_with_any([](int const &) -> Error { throw 0; }));          // binds to const ref
  static_assert(e_is::invocable<lvalue>([](int &) -> Error { throw 0; }));                 // binds to lvalue
  static_assert(e_is::invocable<rvalue, prvalue>([](int &&) -> Error { throw 0; }));       // can move
  static_assert(e_is::invocable<rvalue, crvalue>([](int const &&) -> Error { throw 0; })); // binds to const rvalue
  static_assert(e_is::not_invocable<clvalue, crvalue, cvalue>([](int &) -> Error { throw 0; })); // cannot remove const
  static_assert(e_is::not_invocable<rvalue>([](int &) -> Error { throw 0; }));                   // disallow bind
  static_assert(e_is::not_invocable<lvalue, clvalue, crvalue, cvalue>([](int &&) -> Error { throw 0; })); // cannot move
  static_assert(e_is::not_invocable_with_any([](std::string) -> Error { throw 0; }));                     // bad type
  static_assert(e_is::not_invocable_with_any([]() -> Error { throw 0; }));                                // bad arity
  static_assert(e_is::not_invocable_with_any([](int, int) -> Error { throw 0; }));                        // bad arity

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
        using T = decltype(operand_t{std::in_place, 42} | filter(truePred, onError));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((operand_t{std::in_place, 42} | filter(truePred, onError)).value() == 42);
      }
      WHEN("predicate returns false")
      {
        using T = decltype(operand_t{std::in_place, 42} | filter(falsePred, onError));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((operand_t{std::in_place, 42} | filter(falsePred, onError)).error().what == "Got 42");
      }
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::unexpect, "Not good"} | filter(truePred, wrong));
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

TEST_CASE("filter member function", "[filter][expected][expected_value][member_functions]")
{
  using namespace fn;

  constexpr auto predicate = &Value::ok;
  constexpr auto onError = &Value::error;
  constexpr auto wrong = [](Value) -> Error { throw 0; };

  using operand_t = fn::expected<Value, Error>;
  using p_is = monadic_static_check<filter_t, operand_t>::bind_right<decltype(onError)>;
  using e_is = monadic_static_check<filter_t, operand_t>::bind_left<decltype(predicate)>;

  static_assert(p_is::invocable_with_any(predicate));
  static_assert(p_is::invocable_with_any([](auto...) -> bool { throw 0; }));         // allow generic call
  static_assert(p_is::invocable_with_any([](Value) -> bool { throw 0; }));           // allow copy
  static_assert(p_is::invocable_with_any([](Value const &) -> bool { throw 0; }));   // binds to const ref
  static_assert(p_is::not_invocable_with_any([](Value &) -> bool { throw 0; }));     // cannot bind lvalue
  static_assert(p_is::not_invocable_with_any([](Value &&) -> bool { throw 0; }));    // cannot move
  static_assert(p_is::not_invocable_with_any([](std::string) -> bool { throw 0; })); // bad type
  static_assert(p_is::not_invocable_with_any([]() -> bool { throw 0; }));            // bad arity
  static_assert(p_is::not_invocable_with_any([](Value, int) -> bool { throw 0; }));  // bad arity

  static_assert(e_is::invocable_with_any(onError));
  static_assert(e_is::invocable_with_any([](auto...) -> Error { throw 0; }));                // allow generic call
  static_assert(e_is::invocable_with_any([](Value) -> Error { throw 0; }));                  // allow copy
  static_assert(e_is::invocable_with_any([](Value const &) -> Error { throw 0; }));          // binds to const ref
  static_assert(e_is::invocable<lvalue>([](Value &) -> Error { throw 0; }));                 // binds to lvalue
  static_assert(e_is::invocable<rvalue, prvalue>([](Value &&) -> Error { throw 0; }));       // can move
  static_assert(e_is::invocable<rvalue, crvalue>([](Value const &&) -> Error { throw 0; })); // binds to const rvalue
  static_assert(e_is::not_invocable<clvalue, crvalue, cvalue>([](Value &) -> Error { throw 0; })); // no remove const
  static_assert(e_is::not_invocable<rvalue>([](Value &) -> operand_t { throw 0; }));               // disallow bind
  static_assert(e_is::not_invocable<lvalue, clvalue, crvalue, cvalue>([](Value &&) -> Error { throw 0; })); // no move
  static_assert(e_is::not_invocable_with_any([](std::string) -> Error { throw 0; }));                       // bad type
  static_assert(e_is::not_invocable_with_any([]() -> Error { throw 0; }));                                  // bad arity
  static_assert(e_is::not_invocable_with_any([](Value, int) -> Error { throw 0; }));                        // bad arity

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
        REQUIRE((a | filter(predicate, &Value::error_)).error().what == "Got 42");
      }
    }

    WHEN("operand is pack")
    {
      using operand_t = fn::expected<fn::pack<int, double>, Error>;
      constexpr operand_t a{std::in_place, fn::pack{84, 0.5}};
      constexpr auto predPack = [](int i, double) constexpr noexcept -> bool { return i > 0; };
      constexpr auto errPack = [](int, double) constexpr noexcept -> Error { return {"Error"}; };
      using T = decltype(a | filter(predPack, errPack));
      static_assert(std::is_same_v<T, operand_t>);

      WHEN("operand is value")
      {
        REQUIRE((a | filter(predPack, errPack)).has_value());
        WHEN("fail")
        {
          constexpr auto fnFail = [](int, double) constexpr -> bool { return false; };
          using T = decltype(a | filter(fnFail, errPack));
          static_assert(std::is_same_v<T, operand_t>);
          REQUIRE((a | filter(fnFail, errPack)).error().what == "Error");
        }
      }

      WHEN("operand is error")
      {
        REQUIRE((operand_t{std::unexpect, Error{"Not good"}} | filter(predPack, errPack)).error().what == "Not good");
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
        using T = decltype(operand_t{std::in_place, 1} | filter(predicate, onError));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((operand_t{std::in_place, 1} | filter(predicate, onError)).value().v == 1);
        REQUIRE((operand_t{std::in_place, 1} | filter(predicate, &Value::error_)).value().v == 1);
      }
      WHEN("predicate returns false")
      {
        using T = decltype(operand_t{std::in_place, 42} | filter(predicate, onError));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((operand_t{std::in_place, 42} | filter(predicate, onError)).error().what == "Got 42");
        REQUIRE((operand_t{std::in_place, 42} | filter(predicate, &Value::error_)).error().what == "Got 42");
      }
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::unexpect, "Not good"} | filter(predicate, wrong));
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

  constexpr auto truePred = [] { return true; };
  constexpr auto falsePred = [] { return false; };
  constexpr auto onError = [] { return Error{"Got error"}; };
  constexpr auto wrong = []() -> Error { throw 0; };

  using operand_t = fn::expected<void, Error>;
  using p_is = monadic_static_check<filter_t, operand_t>::bind_right<decltype(onError)>;
  using e_is = monadic_static_check<filter_t, operand_t>::bind_left<decltype(truePred)>;

  static_assert(p_is::invocable_with_any(truePred));
  static_assert(p_is::invocable_with_any([](auto...) -> bool { throw 0; }));      // allow generic call
  static_assert(p_is::not_invocable_with_any([](int) -> bool { throw 0; }));      // bad arity
  static_assert(p_is::not_invocable_with_any([](int, int) -> bool { throw 0; })); // bad arity

  static_assert(e_is::invocable_with_any(onError));
  static_assert(e_is::invocable_with_any([](auto...) -> Error { throw 0; }));      // allow generic call
  static_assert(e_is::not_invocable_with_any([](int) -> Error { throw 0; }));      // bad arity
  static_assert(e_is::not_invocable_with_any([](int, int) -> Error { throw 0; })); // bad arity

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
        using T = decltype(operand_t{std::in_place} | filter(truePred, onError));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((operand_t{std::in_place} | filter(truePred, onError)).has_value());
      }
      WHEN("predicate returns false")
      {
        using T = decltype(operand_t{std::in_place} | filter(falsePred, onError));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((operand_t{std::in_place} | filter(falsePred, onError)).error().what == "Got error");
      }
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::unexpect, "Not good"} | filter(truePred, wrong));
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

  constexpr auto truePred = [](int) { return true; };
  constexpr auto falsePred = [](int) { return false; };

  using operand_t = fn::optional<int>;
  using is = monadic_static_check<filter_t, operand_t>;

  static_assert(is::invocable_with_any(truePred));
  static_assert(is::invocable_with_any([](auto...) -> bool { throw 0; }));         // allow generic call
  static_assert(is::invocable_with_any([](int) -> bool { throw 0; }));             // allow copy
  static_assert(is::invocable_with_any([](unsigned) -> bool { throw 0; }));        // allow conversion
  static_assert(is::invocable_with_any([](int const &) -> bool { throw 0; }));     // binds to const lvalue
  static_assert(is::not_invocable_with_any([](int &) -> bool { throw 0; }));       // cannot bind lvalue
  static_assert(is::not_invocable_with_any([](int &&) -> bool { throw 0; }));      // cannot move
  static_assert(is::not_invocable_with_any([](std::string) -> bool { throw 0; })); // bad type
  static_assert(is::not_invocable_with_any([]() -> bool { throw 0; }));            // bad arity
  static_assert(is::not_invocable_with_any([](int, int) -> bool { throw 0; }));    // bad arity

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

  WHEN("operand is pack")
  {
    using operand_t = fn::optional<fn::pack<int, double>>;
    constexpr operand_t a{std::in_place, fn::pack{84, 0.5}};
    constexpr auto predPack = [](int i, double) constexpr noexcept -> bool { return i > 0; };
    using T = decltype(a | filter(predPack));
    static_assert(std::is_same_v<T, operand_t>);

    WHEN("operand is value")
    {
      REQUIRE((a | filter(predPack)).has_value());
      WHEN("fail")
      {
        constexpr auto fnFail = [](int, double) constexpr -> bool { return false; };
        using T = decltype(a | filter(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE(not(a | filter(fnFail)).has_value());
      }
    }

    WHEN("operand is nullopt") { REQUIRE(not(operand_t{std::nullopt} | filter(predPack)).has_value()); }
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

  constexpr auto predicate = &Value::ok;

  using operand_t = fn::optional<Value>;
  using is = monadic_static_check<filter_t, operand_t>;

  static_assert(is::invocable_with_any(predicate));
  static_assert(is::invocable_with_any([](auto...) -> bool { throw 0; }));         // allow generic call
  static_assert(is::invocable_with_any([](Value) -> bool { throw 0; }));           // allow copy
  static_assert(is::invocable_with_any([](Value const &) -> bool { throw 0; }));   // binds to const lvalue
  static_assert(is::not_invocable_with_any([](Value &) -> bool { throw 0; }));     // cannot bind lvalue
  static_assert(is::not_invocable_with_any([](Value &&) -> bool { throw 0; }));    // cannot move
  static_assert(is::not_invocable_with_any([](std::string) -> bool { throw 0; })); // bad type
  static_assert(is::not_invocable_with_any([]() -> bool { throw 0; }));            // bad arity
  static_assert(is::not_invocable_with_any([](Value, int) -> bool { throw 0; }));  // bad arity

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
  using T = fn::expected<int, Error>;

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
  using T = fn::optional<int>;

  constexpr auto fn = [](int i) constexpr noexcept -> bool { return i < 3; };
  constexpr auto r1 = T{0} | fn::filter(fn);
  static_assert(r1.value() == 0);
  constexpr auto r2 = T{3} | fn::filter(fn);
  static_assert(not r2.has_value());

  SUCCEED();
}
