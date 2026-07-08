// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

// readme-example
#include <fn/and_then.hpp>
#include <fn/utility.hpp>

#include <climits>
#include <numeric>
#include <type_traits>

struct DivByZero {};
struct Overflow {};
struct Add {};
struct Mul {};
enum Side { num, den };
class Rational {
  int num_, den_;
  constexpr Rational(int n, int d) noexcept : num_(n), den_(d) {}

public:
  constexpr bool operator==(Rational const &) const noexcept = default;
  template <Side S> constexpr friend auto get(Rational const &r) noexcept -> int { return S == num ? r.num_ : r.den_; }
  // The invariant lives in the type: `make` is the only way to build one, so every Rational is reduced,
  // sign-normalized and representable — callers receive a value they never have to re-check.
  static auto make(long long n, long long d) -> fn::expected<Rational, fn::sum_for<DivByZero, Overflow>>
  {
    if (d == 0) return pfn::unexpected{fn::sum{DivByZero{}}};
    if (n == LLONG_MIN || d == LLONG_MIN) return pfn::unexpected{fn::sum{Overflow{}}};
    auto const g = (d < 0 ? -1 : 1) * std::gcd(n, d);
    n /= g;
    d /= g;
    if (n < INT_MIN || n > INT_MAX || d > INT_MAX) return pfn::unexpected{fn::sum{Overflow{}}};
    return Rational(int(n), int(d));
  }
};
// `evaluate` trusts its operands — the type guarantees them, so it never revalidates. The int64 intermediates
// cannot overflow for int32 fields, and `make` rejects any result that will not fit back, folding Overflow in:
auto evaluate(fn::expected<Rational, fn::sum_for<DivByZero, Overflow>> a, fn::sum_for<Add, Mul> op,
              fn::expected<Rational, fn::sum_for<DivByZero, Overflow>> b)
{
  return (a & b) | fn::and_then([op](Rational x, Rational y) {
           return op.invoke(fn::overload{
               [&](Add) {
                 return Rational::make(1LL * get<num>(x) * get<den>(y) + 1LL * get<num>(y) * get<den>(x),
                                       1LL * get<den>(x) * get<den>(y));
               },
               [&](Mul) { return Rational::make(1LL * get<num>(x) * get<num>(y), 1LL * get<den>(x) * get<den>(y)); }});
         });
}
// The library composes the pipeline type — a Rational, over the sum of every way construction can fail:
static_assert(std::is_same_v<decltype(evaluate(Rational::make(0, 1), Add{}, Rational::make(0, 1))),
                             fn::expected<Rational, fn::sum<DivByZero, Overflow>>>);
// readme-example

int main()
{
  auto const big = Rational::make(2000000000, 1); // fits int32; its square does not
  return (evaluate(Rational::make(1, 2), Add{}, Rational::make(1, 3)).value() == Rational::make(5, 6)    //
          && evaluate(Rational::make(2, 3), Mul{}, Rational::make(3, 4)).value() == Rational::make(1, 2) //
          && Rational::make(1, 0).error().has_value<DivByZero>()                                         //
          && evaluate(big, Mul{}, big).error().has_value<Overflow>())
             ? 0
             : 1;
}
