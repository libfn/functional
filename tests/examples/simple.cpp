// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/and_then.hpp"
#include "functional/fail.hpp"
#include "functional/fwd.hpp"
#include "functional/inspect.hpp"
#include "functional/inspect_error.hpp"
#include "functional/or_else.hpp"
#include "functional/recover.hpp"
#include "functional/transform.hpp"
#include "functional/transform_error.hpp"
#include "functional/utility.hpp"

#include <catch2/catch_all.hpp>

#include <iostream>

struct Error final {
  std::string what;
};

template <typename T> struct ImmovableValue final {
  T value;

  constexpr ImmovableValue(T &&v) noexcept : value(v) {}
  constexpr auto operator==(ImmovableValue const &) const noexcept -> bool
      = default;

  constexpr ImmovableValue(ImmovableValue const &) = delete;
  constexpr ImmovableValue(ImmovableValue &&) = delete;
  constexpr ImmovableValue &operator=(ImmovableValue const &) = delete;
  constexpr ImmovableValue &operator=(ImmovableValue &&) = delete;
};
template <typename T> ImmovableValue(T &&) -> ImmovableValue<T>;

template <typename Fn> struct ImmovableFn final {
  Fn fn;

  constexpr ImmovableFn(Fn &&fn) noexcept : fn(fn) {}
  constexpr auto operator()(auto &&...args) const noexcept -> decltype(auto)
    requires requires { fn(args...); }
  {
    return fn(FWD(args)...);
  }

  constexpr ImmovableFn(ImmovableFn const &) = delete;
  constexpr ImmovableFn(ImmovableFn &&) = delete;
  constexpr ImmovableFn &operator=(ImmovableFn const &) = delete;
  constexpr ImmovableFn &operator=(ImmovableFn &&) = delete;
};
template <typename Fn> ImmovableFn(Fn &&) -> ImmovableFn<Fn>;

TEST_CASE("Demo expected", "[expected][and_then][transform_error][transform]"
                           "[inspect][inspect_error][recover][fail][immovable]")
{
  constexpr auto fn1 = [](char const *str, double &peek) {
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

    // Immovable operations must be captured as lvalues, and functor will store
    // reference to them rather than make a copy
    constexpr auto fn1 = [j = ImmovableValue{-1}](
                             int i) noexcept -> std::expected<double, Error> {
      if (i < j.value) {
        return std::unexpected<Error>{"Too small"};
      }
      return {i + 0.5};
    };
    constexpr auto const fn2 = ImmovableFn{
        [](double v) noexcept -> int { return std::floor(v - 0.5); }};

    return (parse(str) //
            | and_then(fn1)
            | transform_error([](Error v) noexcept -> std::runtime_error {
                return std::runtime_error{v.what};
              })
            | transform(fn2)
            | inspect([&peek](double d) noexcept -> void { peek = d; })
            | inspect_error(
                [&peek](std::runtime_error) noexcept -> void { peek = 0; })
            | recover([](auto...) noexcept -> int { return -13; })
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

  auto const e1 = std::expected<int, Error>{0} //
                  | fn::fail([](int) noexcept -> Error { return {"Dummy"}; });
  CHECK(not e1.has_value());
  CHECK(e1.error().what == "Dummy");
}

TEST_CASE("Demo optional",
          "[optional][and_then][or_else][inspect][transform][fail][recover]")
{
  constexpr auto fn1 = [](char const *str, int &peek) {
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
            | and_then([](int i) noexcept -> std::optional<int> {
                if (i > 0) {
                  return {i};
                }
                return {std::nullopt};
              })
            | inspect([&peek](int d) noexcept -> void { peek = d; })
            | inspect_error([&peek]() noexcept -> void { peek = 0; })
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

  auto const o1 = std::optional<int>{0} //
                  | fn::fail([](int) noexcept {})
                  | fn::recover([]() { return -1; });
  CHECK(o1.has_value());
  CHECK(o1.value() == -1);
}
