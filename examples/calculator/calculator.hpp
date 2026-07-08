// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef EXAMPLES_CALCULATOR_CALCULATOR
#define EXAMPLES_CALCULATOR_CALCULATOR

#include "fn/and_then.hpp"
#include "fn/expected.hpp"
#include "fn/pack.hpp"
#include "fn/transform.hpp"
#include "fn/utility.hpp"

#include <cerrno>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace calc {

// A number is a long or a double, selected by the input's spelling — a type-indexed sum
using Number = fn::sum<double, long>;
using Stack = std::vector<Number>;

enum class ParseError { UnknownToken };
enum class StackError { NotEnoughOperands };
enum class MathError { DivisionByZero, NotIntegral, Overflow };

// Each step contributes its own error type; the API exposes their sum
using Error = fn::sum<MathError, ParseError, StackError>;
using Result = fn::expected<Stack, Error>;

// An operation is a tiny struct: argument_count says how many operands it pops. A parsed number is
// itself an operation — the nullary Push of its value — so a whole line is a sequence of operations.
struct Add {
  static constexpr int argument_count = 2;
};
struct Div {
  static constexpr int argument_count = 2;
};
struct Drop {
  static constexpr int argument_count = 1;
};
struct Dup {
  static constexpr int argument_count = 1;
};
struct Mod {
  static constexpr int argument_count = 2;
};
struct Mul {
  static constexpr int argument_count = 2;
};
struct Push {
  static constexpr int argument_count = 0;
  Number value;
};
struct Sub {
  static constexpr int argument_count = 2;
};
struct Swap {
  static constexpr int argument_count = 2;
};
using Operation = fn::sum<Add, Div, Drop, Dup, Mod, Mul, Push, Sub, Swap>;

// "42", "052" and "0x2a" are long, "2.5", "1e-3" and ".5" are double; a long overflow promotes to double
inline auto parse(std::string const &token) -> fn::expected<Number, ParseError>
{
  char const *last = token.data() + token.size();
  char *end = {};
  errno = 0;
  if (long const i = std::strtol(token.data(), &end, 0); end == last && end != token.data() && errno == 0)
    return Number{i};
  if (double const d = std::strtod(token.data(), &end); end == last && end != token.data())
    return Number{d};
  return pfn::unexpected(ParseError::UnknownToken);
}

inline auto lookup(std::string const &token) -> fn::expected<Operation, ParseError>
{
  if (token == "+")
    return {Add{}};
  if (token == "-")
    return {Sub{}};
  if (token == "*")
    return {Mul{}};
  if (token == "/")
    return {Div{}};
  if (token == "%")
    return {Mod{}};
  if (token == "swap")
    return {Swap{}};
  if (token == "dup")
    return {Dup{}};
  if (token == "drop")
    return {Drop{}};

  return parse(token) | fn::transform([](auto n) { //
           return Push{Number{n}};
         });
}

inline auto pop(Stack &s) -> fn::expected<Number, StackError>
{
  if (s.empty())
    return pfn::unexpected(StackError::NotEnoughOperands);
  Number n = s.back();
  s.pop_back();
  return n;
}

// What an operation pushes back — zero, one or two numbers — or its typed failure
using Results = fn::expected<fn::sum_for<fn::pack<>, fn::pack<long>, fn::pack<double>, fn::pack<long, long>,
                                         fn::pack<double, long>, fn::pack<long, double>, fn::pack<double, double>>,
                             MathError>;

constexpr inline long minimum = std::numeric_limits<long>::min();
constexpr inline long maximum = std::numeric_limits<long>::max();

// Signed overflow is UB, so operands are checked before the arithmetic, never after; the Mul bounds
// are exact because integer division truncates toward zero
constexpr auto overflows(long x, long y, Add) -> bool { return y > 0 ? x > maximum - y : x < minimum - y; }
constexpr auto overflows(long x, long y, Sub) -> bool { return y < 0 ? x > maximum + y : x < minimum + y; }
constexpr auto overflows(long x, long y, Mul) -> bool
{
  if (x > 0)
    return y > 0 ? x > maximum / y : y < minimum / x;
  return y > 0 ? x < minimum / y : x != 0 && y < maximum / x;
}

// One table defines every operation, dispatched on the runtime types of *both* operands and the
// operation itself; overload resolution picks the arm, so `7 2 /` is exact and `7 2.0 /` promotes
constexpr inline auto execute = fn::overload{
    [](long x, long y, Add op) -> Results {
      if (overflows(x, y, op))
        return pfn::unexpected(MathError::Overflow);
      return {fn::pack{x + y}};
    },
    [](auto x, auto y, Add) -> Results { return {fn::pack{x + y}}; },
    [](long x, long y, Sub op) -> Results {
      if (overflows(x, y, op))
        return pfn::unexpected(MathError::Overflow);
      return {fn::pack{x - y}};
    },
    [](auto x, auto y, Sub) -> Results { return {fn::pack{x - y}}; },
    [](long x, long y, Mul op) -> Results {
      if (overflows(x, y, op))
        return pfn::unexpected(MathError::Overflow);
      return {fn::pack{x * y}};
    },
    [](auto x, auto y, Mul) -> Results { return {fn::pack{x * y}}; },
    [](long x, long y, Div) -> Results {
      if (y == 0)
        return pfn::unexpected(MathError::DivisionByZero);
      if (x == minimum && y == -1)
        return pfn::unexpected(MathError::Overflow);
      return {fn::pack{x / y}}; // exact: `7 2 /` is 3
    },
    [](auto x, auto y, Div) -> Results {
      if (y == 0)
        return pfn::unexpected(MathError::DivisionByZero);
      return {fn::pack{x / y}}; // a double anywhere promotes: `7 2.0 /` is 3.5
    },
    [](long x, long y, Mod) -> Results {
      if (y == 0)
        return pfn::unexpected(MathError::DivisionByZero);
      if (x == minimum && y == -1)
        return pfn::unexpected(MathError::Overflow);
      return {fn::pack{x % y}};
    },
    // dispatch is also how an operation rejects operands it is not defined for
    [](auto, auto, Mod) -> Results { return pfn::unexpected(MathError::NotIntegral); },
    // named arguments deduce reference elements in a pack; spell the value types instead
    [](auto x, auto y, Swap) -> Results { return {fn::pack<decltype(y), decltype(x)>{y, x}}; },
    [](auto x, Dup) -> Results { return {fn::pack<decltype(x), decltype(x)>{x, x}}; },
    [](auto, Drop) -> Results { return {fn::pack<>{}}; },
    // a Push carries a runtime Number; one more dispatch unwraps it into a raw pack
    [](Push p) -> Results { return p.value.invoke([](auto v) -> Results { return {fn::pack<decltype(v)>{v}}; }); }};

// One token = one step: look the operation up, pop as many operands as it declares (folding them and
// the operation itself into one cartesian sum of packs), execute the matching arm, store what it returns
inline auto step(Stack s, std::string const &token) -> Result
{
  auto const store = [&s](auto &&...args) -> fn::expected<Stack, MathError> {
    static_assert(sizeof...(args) < 3);
    (s.push_back(Number{args}), ...);
    return {std::move(s)};
  };

  return (fn::expected<void, fn::sum<>>{} & lookup(token)) //
         | fn::and_then([&s, &store](auto op) -> Result {
             // the wrapper's StackError is never raised; `&` needs a non-empty error type to fold with
             using loaded = fn::expected<decltype(op), StackError>;
             constexpr int take = decltype(op)::argument_count;
             if constexpr (take == 0)
               return execute(op) | fn::and_then(store);
             else if constexpr (take == 1)
               return (fn::expected<void, fn::sum<>>{} & pop(s) & loaded{op}) //
                      | fn::and_then(execute)                                 //
                      | fn::and_then(store);
             else {
               static_assert(decltype(op)::argument_count == 2);
               auto rhs = pop(s); // popped before lhs: in `2 1 -` the top of the stack, 1, is the right operand
               auto lhs = pop(s);
               return (fn::expected<void, fn::sum<>>{} & std::move(lhs) & std::move(rhs) & loaded{op}) //
                      | fn::and_then(execute)                                                          //
                      | fn::and_then(store);
             }
           });
}

// Evaluate one line of whitespace-separated tokens — a recursive monadic fold over the stack
// (Y-combinator: self is passed explicitly), where the first error short-circuits the rest of the line
inline auto evaluate(Stack s, std::string_view line) -> Result
{
  using iterator = std::vector<std::string>::const_iterator;
  static constexpr auto fold = [](auto self, Stack acc, iterator it, iterator end) -> Result {
    if (it == end)
      return {std::move(acc)};
    return step(std::move(acc), *it) //
           | fn::and_then([self, it, end](Stack next) { return self(self, std::move(next), std::next(it), end); });
  };

  std::istringstream stream{std::string{line}};
  std::vector<std::string> const tokens{std::istream_iterator<std::string>{stream}, {}};
  return fold(fold, std::move(s), tokens.begin(), tokens.end());
}

} // namespace calc

#endif // EXAMPLES_CALCULATOR_CALCULATOR
