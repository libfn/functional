// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/and_then.hpp"
#include "functional/fail.hpp"
#include "functional/filter.hpp"
#include "functional/fwd.hpp"
#include "functional/inspect.hpp"
#include "functional/or_else.hpp"
#include "functional/recover.hpp"
#include "functional/transform.hpp"
#include "functional/transform_error.hpp"

#include <catch2/catch_all.hpp>

#include <iostream>

struct Error final {
  std::string what;

  bool operator==(Error const &) const = default;
  friend auto operator<<(std::ostream &os, Error const &self) -> std::ostream &
  {
    return (os << self.what);
  }
};

TEST_CASE(
    "Demo expected",
    "[expected][and_then][transform_error][transform][inspect][recover][fail]")
{
  constexpr auto fn1 = [](const char *str, double &peek) {
    using namespace fn;

    constexpr auto parse
        = [](char const *str) noexcept -> std::expected<int, Error> {
      int tmp = {};
      char const *end = str + std::strlen(str);
      if (std::from_chars(str, end, tmp).ptr == end) {
        return {tmp};
      }
      return std::unexpected<Error>{"Failed to parse " + std::string(str)};
    };

    return (parse(str) //
            | and_then([](int i) noexcept -> std::expected<double, Error> {
                if (i < -1) {
                  return std::unexpected<Error>("Too small");
                }
                return {i + 0.5};
              })
            | transform_error([](Error v) noexcept -> std::runtime_error {
                return std::runtime_error{v.what};
              })
            | transform(
                [](double v) noexcept -> int { return std::floor(v - 0.5); })
            | inspect(
                [&peek](double d) noexcept -> void { peek = d; },
                [&peek](std::runtime_error) noexcept -> void { peek = 0; })
            | recover([](auto...) noexcept -> double { return -13; })
            //
            )
        .value();
  };

  double d = {};
  CHECK(fn1("42", d) == 42);
  CHECK(d == 42);
  CHECK(fn1("-1", d) == -1);
  CHECK(d == -1);
  CHECK(fn1("-3", d) == -13);
  CHECK(d == 0);

  const auto e1 = std::expected<int, Error>{0} //
                  | fn::fail([](int) noexcept -> Error { return {"Dummy"}; });
  CHECK(not e1.has_value());
  CHECK(e1.error().what == "Dummy");
}

TEST_CASE("Demo optional",
          "[optional][and_then][or_else][inspect][transform][fail][recover]")
{
  constexpr auto fn1 = [](const char *str, int &peek) {
    using namespace fn;

    constexpr auto parse = [](char const *str) noexcept -> std::optional<int> {
      int tmp = {};
      char const *end = str + std::strlen(str);
      if (std::from_chars(str, end, tmp).ptr == end) {
        return {tmp};
      }
      return {};
    };

    return (parse(str) //
            | and_then([](int i) -> std::optional<int> {
                if (i > 0) {
                  return {i};
                }
                return {std::nullopt};
              })
            | inspect([&peek](int d) noexcept -> void { peek = d; },
                      [&peek]() noexcept -> void { peek = 0; })
            | or_else([]() noexcept -> std::optional<int> { return -13; })
            | transform([](int i) noexcept -> double { return i + 0.5; })
            //
            )
        .value();
  };

  int i = {};
  CHECK(fn1("42", i) == 42.5);
  CHECK(i == 42);
  CHECK(fn1("-1", i) == -12.5);
  CHECK(i == 0);
  CHECK(fn1("-2", i) == -12.5);
  CHECK(i == 0);

  const auto o1 = std::optional<int>{0} //
                  | fn::fail([](int) noexcept {})
                  | fn::recover([]() { return -1; });
  CHECK(o1.has_value());
  CHECK(o1.value() == -1);
}

TEST_CASE("Filter for expected", "[expected][filter]")
{
  using namespace fn;

  constexpr auto fn1 = [](int i) {
    return std::expected<int, Error>{i}
           | filter([](auto &&v) noexcept -> bool { return v == 42; },
                    [](auto &&v) {
                      return Error{"Wrong value " + std::to_string(v)};
                    });
  };

  CHECK(fn1(42).value() == 42);
  CHECK(fn1(13).error() == Error("Wrong value 13"));
}

TEST_CASE("Filter for optional", "[optional][filter]")
{
  using namespace fn;

  constexpr auto fn1 = [](int i) {
    return std::optional<int>{i}
           | filter([](auto &&v) noexcept -> bool { return v == 42; });
  };

  CHECK(fn1(42).value() == 42);
  CHECK(not fn1(13).has_value());
}