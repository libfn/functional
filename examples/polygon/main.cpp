#include "polygon.hpp"

#include "fn/and_then.hpp"
#include "fn/expected.hpp"
#include "fn/or_else.hpp"

#include <exception>
#include <iostream>
#include <numeric>

// Exit code for any exception that escapes the typed error channel
struct unexpected_exception {
  static constexpr int code = 13;
};

auto main(int argc, char const **argv) -> int
try {
  auto get_args = [=]() -> fn::expected<parameters::arguments_t, fn::sum<>> {
    parameters::arguments_t args;
    args.reserve(static_cast<std::size_t>(argc));
    // Note: std::accumulate used as a move-threaded fold to build a vector.
    return std::accumulate(argv, argv + argc, std::move(args), [](parameters::arguments_t &&acc, char const *arg) {
      acc.emplace_back(arg ? arg : std::string_view{});
      return std::move(acc);
    });
  };

  return (get_args()                       //
          | fn::and_then(parameters::make) //
          | fn::and_then(inputs::make)     //
          | fn::and_then(algorithm)        //
          | fn::or_else(                   //
              [](auto &&p) -> fn::expected<int, fn::sum<>> {
                print_error(FWD(p));
                return p.code;
              }))
      .value();
} catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
  return unexpected_exception::code;
}
