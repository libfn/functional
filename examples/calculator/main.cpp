// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "calculator.hpp"

#include "fn/utility.hpp"

#include <exception>
#include <iostream>
#include <string>

namespace {

void print(calc::Stack const &stack)
{
  std::cout << "[";
  for (calc::Number const &n : stack)
    n.apply([](auto v) { std::cout << ' ' << v; });
  std::cout << " ]\n";
}

void report(calc::Error const &error)
{
  std::cout << error.apply( //
      fn::overload{[](calc::ParseError) { return "not a number or a known operation"; },
                   [](calc::StackError) { return "not enough operands on the stack"; },
                   [](calc::MathError e) {
                     if (e == calc::MathError::DivisionByZero)
                       return "division by zero";
                     if (e == calc::MathError::NotIntegral)
                       return "operation is defined for integers only";
                     return "overflow";
                   }})
            << '\n';
}

} // namespace

auto main() -> int
try {
  calc::Stack stack;
  // the prompt goes to stderr (like a shell's) so piped stdout stays clean without any TTY detection
  for (std::string line; std::cerr << "> ", std::getline(std::cin, line);) {
    // A failed line reports its error and leaves the stack untouched
    if (auto r = calc::evaluate(stack, line); r.has_value()) {
      stack = std::move(r).value();
      print(stack);
    } else {
      report(r.error());
    }
  }
  return 0;
} catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
