// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/and_then.hpp>
#include <fn/utility.hpp>

#include <charconv>
#include <numeric>
#include <string_view>

// `parse` uses `std::from_chars` to parse a number from a string - only `constexpr` since C++23
#ifdef __cpp_lib_constexpr_charconv
#define FROM_CHARS std::from_chars
#else
#define FROM_CHARS fallback_parse_int

constexpr std::from_chars_result fallback_parse_int(char const *first, char const *last, int &value,
                                                    int base = 10) noexcept
{
  // std::from_chars constraints on base
  if (base < 2 || base > 36) return {first, std::errc::invalid_argument};

  char const *curr = first;
  if (curr == last) return {first, std::errc::invalid_argument};

  bool const negative = (*curr == '-');
  if (negative) {
    ++curr;
    if (curr == last) // Lone minus sign
      return {first, std::errc::invalid_argument};
  }

  // A valid character must follow the optional minus sign
  // Check digit mapping according to base rule
  auto get_digit = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'z') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'Z') return 10 + (c - 'A');
    return -1;
  };

  if (get_digit(*curr) == -1 || get_digit(*curr) >= base)
    return {first, std::errc::invalid_argument};

  long long val = 0;
  bool overflowed = false;

  while (curr != last) {
    int const digit = get_digit(*curr);
    if (digit == -1 || digit >= base) break; // Non-digit character ends parsing gracefully

    if (!overflowed) {
      val = val * base + digit;

      // Perform local overflow boundary checking for `int` limits
      if ((negative && -val < std::numeric_limits<int>::min())
          || (!negative && val > std::numeric_limits<int>::max())) {
        overflowed = true;
      }
    }
    ++curr;
  }

  if (overflowed) return {curr, std::errc::result_out_of_range};

  value = negative ? static_cast<int>(-val) : static_cast<int>(val);
  return {curr, std::errc{}};
}

#endif

enum class NotANumber;

constexpr auto parse(std::string_view s) noexcept
    -> fn::expected<fn::pack<int, int>, fn::copack<NotANumber>>
{
  int n = 0, d = 1;
  auto const bar = s.find('/');
  auto const head = s.substr(0, bar);
  auto const [p, e] = FROM_CHARS(head.data(), head.data() + head.size(), n);
  if (e != std::errc{} || p != head.data() + head.size())
    return fn::unexpected{fn::copack{NotANumber{}}};
  if (bar != std::string_view::npos) {
    auto const tail = s.substr(bar + 1);
    auto const [q, f] = FROM_CHARS(tail.data(), tail.data() + tail.size(), d);
    if (f != std::errc{} || q != tail.data() + tail.size())
      return fn::unexpected{fn::copack{NotANumber{}}};
  }
  return fn::pack<int, int>{n, d};
}

// readme-example
// Various error types.
enum class NotANumber {};
enum class DivByZero {};
enum class Overflow {};

// Operations on rational numbers.
enum class Add {};
enum class Sub {};
enum class Mul {};
enum class Div {};

// `parse` turns a '/' delimited string into a pair of numbers (a numerator and denominator)
constexpr auto parse(std::string_view s) noexcept
    -> fn::expected<fn::pack<int, int>, fn::copack<NotANumber>>;

class Rational {
  int n_, d_;
  constexpr Rational(int n, int d) noexcept : n_(n), d_(d) {}

public:
  constexpr bool operator==(Rational const &) const noexcept = default;
  constexpr int num() const noexcept { return n_; }
  constexpr int den() const noexcept { return d_; }

  // The invariants live in the type: `make` is the only way to build a `Rational`, and every one is
  // reduced, sign-normalized and representable. Callers receive a value they never need re-check.
  static constexpr struct make_t {
    constexpr auto operator()(long long n, long long d) const noexcept
        -> fn::expected<Rational, fn::copack_for<DivByZero, Overflow>>
    {
      if (d == 0) return fn::unexpected{fn::copack{DivByZero{}}};
      if (n == std::numeric_limits<long long>::min() || d == std::numeric_limits<long long>::min())
        return fn::unexpected{fn::copack{Overflow{}}};

      auto const g = (d < 0 ? -1 : 1) * std::gcd(n, d);
      n /= g;
      d /= g;
      if (n < std::numeric_limits<int>::min() || n > std::numeric_limits<int>::max()
          || d > std::numeric_limits<int>::max()) {
        return fn::unexpected{fn::copack{Overflow{}}};
      }

      return Rational(static_cast<int>(n), static_cast<int>(d));
    }

    constexpr auto operator()(std::string_view s) const noexcept
    {
      return parse(s) | fn::and_then(*this);
    }
  } make{};

  constexpr auto neg() const noexcept { return make(-1LL * n_, d_); }
  constexpr auto inv() const noexcept { return make(d_, n_); }
  constexpr auto add(Rational const &other) const noexcept
  {
    return make(1LL * n_ * other.d_ + 1LL * other.n_ * d_, //
                1LL * d_ * other.d_);
  }
  constexpr auto sub(Rational const &other) const noexcept
  {
    return other.neg() | fn::and_then([*this](Rational y) { return add(y); });
  }
  constexpr auto mul(Rational const &other) const noexcept
  {
    return make(1LL * n_ * other.n_, 1LL * d_ * other.d_);
  }
  constexpr auto div(Rational const &other) const noexcept
  {
    return other.inv() | fn::and_then([*this](Rational y) { return mul(y); });
  }
};

// `evaluate` parses each operand, applies the operator, and lets `make` re-check the result.
// Each stage fails its own way, and the library folds error types into one co-product.
constexpr auto evaluate(std::string_view a, fn::copack_for<Add, Sub, Mul, Div> op,
                        std::string_view b) noexcept
{
  using Op = fn::expected<decltype(op), fn::copack<>>;
  return (Rational::make(a) & Op{op} & Rational::make(b)) //
         | fn::and_then(fn::overload{[](Rational x, Add, Rational y) { return x.add(y); },
                                     [](Rational x, Sub, Rational y) { return x.sub(y); },
                                     [](Rational x, Mul, Rational y) { return x.mul(y); },
                                     [](Rational x, Div, Rational y) { return x.div(y); }});
}

// The error type of a sequence is the derived copack of all failure modes, never spelled by hand:
static_assert(
    std::is_same_v<decltype(Rational::make("1/1")),
                   fn::expected<Rational, fn::copack_for<DivByZero, NotANumber, Overflow>>>);
static_assert(
    std::is_same_v<decltype(evaluate("1/2", Add{}, "3/4")),
                   fn::expected<Rational, fn::copack_for<DivByZero, NotANumber, Overflow>>>);
// Constant evaluated calculations used to verify both values and errors during compilation:
static_assert(evaluate("1/2", Add{}, "1/3").value() == Rational::make(5, 6));
static_assert(evaluate("2/3", Div{}, "0/1").error().has_value<DivByZero>());
// readme-example

int main()
{
  return (evaluate("1/2", Add{}, "1/3").value() == Rational::make(5, 6)    //
          && evaluate("1/2", Sub{}, "1/3").value() == Rational::make(1, 6) //
          && evaluate("2/3", Mul{}, "3/4").value() == Rational::make(1, 2) //
          && evaluate("2/3", Div{}, "1/2").value() == Rational::make(4, 3) //
          && evaluate("2/3", Div{}, "0/1").error().has_value<DivByZero>()  //
          && Rational::make("1/0").error().has_value<DivByZero>()          //
          && Rational::make(1, 1)
                 .value()
                 .sub(Rational::make(std::numeric_limits<int>::min(), 1).value())
                 .error()
                 .has_value<Overflow>()
          && parse("abc").error().has_value<NotANumber>() //
          && evaluate("2000000000", Mul{}, "2000000000").error().has_value<Overflow>())
             ? 0
             : 1;
}
