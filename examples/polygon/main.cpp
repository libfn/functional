#include "polygon.hpp"

#include "fn/and_then.hpp"
#include "fn/expected.hpp"
#include "fn/or_else.hpp"

#include <exception>
#include <iostream>
#include <type_traits>

// Exit code for any exception that escapes the typed error channel
struct unexpected_exception {
  static constexpr int code = 13;
};

auto main(int argc, char const **argv) -> int
try {
  static constexpr auto wrap = []<typename T>(T &&v) -> fn::expected<std::remove_cvref_t<T>, fn::sum<>> { //
    return FWD(v);
  };

  return ((wrap(argc) & wrap(argv))        //
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
