// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

// readme-example
#include <fn/and_then.hpp>
#include <fn/utility.hpp>

#include <charconv>
#include <climits>
#include <numeric>
#include <string_view>
#include <type_traits>

// Various error types — each type is unrelated to other, `enum` for brevity.
enum NotANumber { notANumber };
enum DivByZero { divByZero };
enum Overflow { overflow };

// Operations on rational numbers — `enum` for brevity.
enum Add { add };
enum Mul { mul };

class Rational {
  int n_, d_;
  constexpr Rational(int n, int d) noexcept : n_(n), d_(d) {}

public:
  constexpr bool operator==(Rational const &) const noexcept = default;
  constexpr int num() const noexcept { return n_; }
  constexpr int den() const noexcept { return d_; }

  // The invariant lives in the type: `make` is the only way to build one, so every
  // Rational is reduced, sign-normalized and representable — callers receive a value they
  // never have to re-check.
  static constexpr auto make(long long n, long long d)
      -> fn::expected<Rational, fn::sum_for<DivByZero, Overflow>>
  {
    if (d == 0) return pfn::unexpected{fn::sum{divByZero}};
    if (n == LLONG_MIN || d == LLONG_MIN) return pfn::unexpected{fn::sum{overflow}};
    auto const g = (d < 0 ? -1 : 1) * std::gcd(n, d);
    n /= g;
    d /= g;
    if (n < INT_MIN || n > INT_MAX || d > INT_MAX) return pfn::unexpected{fn::sum{overflow}};
    return Rational(int(n), int(d));
  }
};

// `parse` turns a string into a pair of numbers (a numerator and denominator)
auto parse(std::string_view s) -> fn::expected<fn::pack<int, int>, fn::sum<NotANumber>>
{
  int n = 0, d = 1;
  auto const bar = s.find('/');
  auto const head = s.substr(0, bar);
  auto const [p, e] = std::from_chars(head.data(), head.data() + head.size(), n);
  if (e != std::errc{} || p != head.data() + head.size())
    return pfn::unexpected{fn::sum{notANumber}};
  if (bar != std::string_view::npos) {
    auto const tail = s.substr(bar + 1);
    auto const [q, f] = std::from_chars(tail.data(), tail.data() + tail.size(), d);
    if (f != std::errc{} || q != tail.data() + tail.size())
      return pfn::unexpected{fn::sum{notANumber}};
  }
  return fn::pack<int, int>{n, d};
}

// Helper, does not need to name any error types — let the library compose them.
constexpr auto rational
    = [](std::string_view s) { return parse(s) | fn::and_then(Rational::make); };

// `evaluate` parses both operands, applies the operator, and lets `make` re-check the
// result. Each stage fails its own way, and the library folds those failures into one
// error sum, never spelled by hand:
auto evaluate(std::string_view a, fn::sum_for<Add, Mul> op, std::string_view b)
{
  using Op = fn::expected<decltype(op), fn::sum<>>;
  return (rational(a) & Op{op} & rational(b))
         | fn::and_then(fn::overload{
             [](Rational x, Add, Rational y) {
               return Rational::make( //
                   1LL * x.num() * y.den() + 1LL * y.num() * x.den(), 1LL * x.den() * y.den());
             },
             [](Rational x, Mul, Rational y) {
               return Rational::make(1LL * x.num() * y.num(), 1LL * x.den() * y.den());
             }});
}

// Result is a Rational, over the sum of every way a stage can fail:
static_assert(std::is_same_v<decltype(evaluate("1/2", add, "3/4")),
                             fn::expected<Rational, fn::sum<DivByZero, NotANumber, Overflow>>>);

// readme-example

int main()
{
  return (evaluate("1/2", add, "1/3").value() == Rational::make(5, 6)    //
          && evaluate("2/3", mul, "3/4").value() == Rational::make(1, 2) //
          && parse("abc").error().has_value<NotANumber>()                //
          && rational("1/0").error().has_value<DivByZero>()              //
          && evaluate("2000000000", mul, "2000000000").error().has_value<Overflow>())
             ? 0
             : 1;
}
