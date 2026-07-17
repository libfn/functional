// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "calculator.hpp"

#include "fn/pack.hpp"

#include <catch2/catch_all.hpp>

#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace {
auto run(std::string_view line) -> calc::Result { return calc::evaluate({}, line); }
} // namespace

// `&` folds two parses into a cartesian product of packs and a copack of errors, all composed by the library:
static_assert(std::is_same_v<decltype(fn::expected<void, fn::copack<>>{} & calc::parse("") & calc::parse("")),
                             fn::expected<fn::copack_for<fn::pack<double, double>, fn::pack<double, long>,
                                                         fn::pack<long, double>, fn::pack<long, long>>,
                                          fn::copack<calc::ParseError>>>);

// ... `step` folds the operands AND the operation into one cartesian dispatch table for `execute`:
static_assert(std::is_same_v<
              decltype(fn::expected<void, fn::copack<>>{} & calc::pop(std::declval<calc::Stack &>())
                       & calc::pop(std::declval<calc::Stack &>()) & fn::expected<calc::Div, fn::copack<>>{calc::Div{}}),
              fn::expected<fn::copack_for<fn::pack<double, double, calc::Div>, fn::pack<double, long, calc::Div>,
                                          fn::pack<long, double, calc::Div>, fn::pack<long, long, calc::Div>>,
                           fn::copack<calc::StackError>>>);

// ... and the calculator's Error alias is exactly the normalized copack of its three error types:
static_assert(std::is_same_v<calc::Error, fn::copack_for<calc::MathError, calc::ParseError, calc::StackError>>);

TEST_CASE("parse", "[calculator]")
{
  SECTION("long formats")
  {
    CHECK(calc::parse("42").value() == calc::Number{42L});
    CHECK(calc::parse("-7").value() == calc::Number{-7L});
    CHECK(calc::parse("0x2a").value() == calc::Number{42L});
    CHECK(calc::parse("052").value() == calc::Number{42L});
  }

  SECTION("double formats")
  {
    CHECK(calc::parse("2.5").value() == calc::Number{2.5});
    CHECK(calc::parse("1e3").value() == calc::Number{1000.0});
    CHECK(calc::parse(".5").value() == calc::Number{0.5});
    CHECK(calc::parse("2.").value() == calc::Number{2.0});
    CHECK(calc::parse("0x1.8p1").value() == calc::Number{3.0});
    CHECK(calc::parse("1e-999").value() == calc::Number{0.0}); // an underflow rounds to zero
  }

  SECTION("a long overflow promotes to the double alternative")
  {
    CHECK(calc::parse("9223372036854775808").value() == calc::Number{9223372036854775808.0});
  }

  SECTION("rejected tokens")
  {
    CHECK(calc::parse("").error() == calc::ParseError::UnknownToken);
    CHECK(calc::parse("x").error() == calc::ParseError::UnknownToken);
    CHECK(calc::parse("1x").error() == calc::ParseError::UnknownToken);
    CHECK(calc::parse("1e999").error() == calc::ParseError::UnknownToken);
    CHECK(calc::parse("inf").error() == calc::ParseError::UnknownToken);
    CHECK(calc::parse("nan").error() == calc::ParseError::UnknownToken);
  }
}

TEST_CASE("arithmetic dispatches on the runtime types of both operands", "[calculator]")
{
  SECTION("long with long stays exact")
  {
    CHECK(run("1 2 +").value() == calc::Stack{calc::Number{3L}});
    CHECK(run("1 2 -").value() == calc::Stack{calc::Number{-1L}});
    CHECK(run("2 3 *").value() == calc::Stack{calc::Number{6L}});
    CHECK(run("7 2 /").value() == calc::Stack{calc::Number{3L}});
    CHECK(run("7 3 %").value() == calc::Stack{calc::Number{1L}});
  }

  SECTION("a double anywhere promotes")
  {
    CHECK(run("1 2.5 +").value() == calc::Stack{calc::Number{3.5}});
    CHECK(run("7 2.0 /").value() == calc::Stack{calc::Number{3.5}});
    CHECK(run("7.0 2 /").value() == calc::Stack{calc::Number{3.5}});
  }

  SECTION("modulo is defined for longs only")
  {
    CHECK(run("7 3.0 %").error() == fn::copack{calc::MathError::NotIntegral});
  }
}

TEST_CASE("stack operations", "[calculator]")
{
  CHECK(run("2 1 swap").value() == calc::Stack{calc::Number{1L}, calc::Number{2L}});
  CHECK(run("2 1 swap /").value() == calc::Stack{calc::Number{0L}});
  CHECK(run("3 dup *").value() == calc::Stack{calc::Number{9L}});
  CHECK(run("1 2 drop").value() == calc::Stack{calc::Number{1L}});
  CHECK(run("").value() == calc::Stack{});
}

TEST_CASE("errors", "[calculator]")
{
  SECTION("division by zero")
  {
    CHECK(run("1 0 /").error() == fn::copack{calc::MathError::DivisionByZero});
    CHECK(run("1 0.0 /").error() == fn::copack{calc::MathError::DivisionByZero});
    CHECK(run("1 0 %").error() == fn::copack{calc::MathError::DivisionByZero});
  }

  SECTION("long overflow")
  {
    auto const minimum = std::to_string(std::numeric_limits<long>::min());
    auto const maximum = std::to_string(std::numeric_limits<long>::max());
    CHECK(run(minimum + " -1 /").error() == fn::copack{calc::MathError::Overflow});
    CHECK(run(minimum + " -1 %").error() == fn::copack{calc::MathError::Overflow});
    CHECK(run(maximum + " 1 +").error() == fn::copack{calc::MathError::Overflow});
    CHECK(run(minimum + " 1 -").error() == fn::copack{calc::MathError::Overflow});
    CHECK(run(maximum + " 2 *").error() == fn::copack{calc::MathError::Overflow});
    CHECK(run(minimum + " -1 *").error() == fn::copack{calc::MathError::Overflow});
    // ... while the guards admit the exact bounds
    CHECK(run(maximum + " 0 +").value() == calc::Stack{calc::Number{std::numeric_limits<long>::max()}});
    CHECK(run(minimum + " 1 *").value() == calc::Stack{calc::Number{std::numeric_limits<long>::min()}});
  }

  SECTION("double overflow")
  {
    CHECK(run("1e308 1e308 +").error() == fn::copack{calc::MathError::Overflow});
    CHECK(run("-1e308 1e308 -").error() == fn::copack{calc::MathError::Overflow});
    CHECK(run("1e308 1e308 *").error() == fn::copack{calc::MathError::Overflow});
    CHECK(run("1e308 1e-308 /").error() == fn::copack{calc::MathError::Overflow});
  }

  SECTION("stack underflow")
  {
    CHECK(run("1 +").error() == fn::copack{calc::StackError::NotEnoughOperands});
    CHECK(run("swap").error() == fn::copack{calc::StackError::NotEnoughOperands});
    CHECK(run("dup").error() == fn::copack{calc::StackError::NotEnoughOperands});
  }

  SECTION("unknown token") { CHECK(run("1 2 bogus").error() == fn::copack{calc::ParseError::UnknownToken}); }

  SECTION("the first error short-circuits the rest of the line")
  {
    CHECK(run("1 0 / 5 5 +").error() == fn::copack{calc::MathError::DivisionByZero});
  }
}

TEST_CASE("the stack persists across lines", "[calculator]")
{
  auto first = calc::evaluate({}, "1 2");
  REQUIRE(first.has_value());
  auto second = calc::evaluate(std::move(first).value(), "+");
  CHECK(second.value() == calc::Stack{calc::Number{3L}});
}
