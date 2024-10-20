// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/and_then.hpp"
#include "functional/fail.hpp"
#include "functional/filter.hpp"
#include "functional/inspect.hpp"
#include "functional/inspect_error.hpp"
#include "functional/or_else.hpp"
#include "functional/recover.hpp"
#include "functional/transform.hpp"
#include "functional/transform_error.hpp"
#include "functional/utility.hpp"

#include <catch2/catch_all.hpp>

#include <charconv>
#include <cstddef>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

// example-error-struct
struct Error final {
  std::string what;
};
// example-error-struct

template <typename T> struct ImmovableValue final {
  T value;

  constexpr ImmovableValue(T &&v) noexcept : value(v) {}
  constexpr auto operator==(ImmovableValue const &) const noexcept -> bool = default;

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

TEST_CASE("Minimal expected", "[expected][and_then]")
{
  // note: formatting is disabled here because otherwise we need to add artificial empty comments to align the pipes
  // properly but they will show up as inline comments in the doc - not nice. in the future we could try and change this
  // behaviour in Znai if that's preferred.

  // clang-format off
  {
    // example-expected-and_then-value
    fn::expected<double, Error> ex{12.1};

    auto rounded = ex
      | fn::and_then([](auto&& v) -> fn::expected<unsigned, Error> {
          return static_cast<unsigned>(std::ceil(v + 0.5));
        });

    REQUIRE(rounded.value() == 13u);
    // example-expected-and_then-value
  }
  {
    // example-expected-and_then-error
    fn::expected<double, Error> ex = std::unexpected<Error>{"Not good"};

    auto oops = ex
      | fn::and_then([](auto&& v) -> fn::expected<unsigned, Error> {
          return static_cast<unsigned>(std::ceil(v + 0.5));
        }); // Not called because ex contains an Error

    REQUIRE(oops.error().what == "Not good");
    // example-expected-and_then-error
  }
  {
    // example-expected-filter-value
    fn::expected<int, Error> ex{42};

    auto value = ex
        | fn::filter([](auto &&i) { return i >= 42; },            // Filter out values less than 42
                     [](auto) { return Error{"Less than 42"}; }); // Return error if predicate fails

    REQUIRE(value.value() == 42);
    // example-expected-filter-value
  }
  {
    // example-expected-filter-error
    fn::expected<int, Error> ex{12};

    auto value = ex
        | fn::filter([](auto &&i) { return i >= 42; },
                     [](auto) { return Error{"Less than 42"}; });

    REQUIRE(value.error().what == "Less than 42");
    // example-expected-filter-error
  }
  // clang-format on
}

TEST_CASE("Demo expected", "[expected][pack][and_then][transform_error][transform][inspect][inspect_error]["
                           "recover][fail][filter][immovable]")
{
  constexpr auto fn1 = [](char const *str, double &peek) {
    using namespace fn;

    constexpr auto parse = [](char const *str) noexcept -> fn::expected<int, Error> {
      int tmp = {};
      char const *end = str + std::strlen(str);
      if (std::from_chars(str, end, tmp).ptr == end) {
        return {tmp};
      }
      return std::unexpected<Error>{"Failed to parse " + std::string(str)};
    };

    // Immovable operations must be captured as lvalues, and functor will store
    // reference to them rather than make a copy
    constexpr auto fn1 = [j = ImmovableValue{-1}](int i) noexcept -> fn::expected<double, Error> {
      if (i < j.value) {
        return std::unexpected<Error>{"Too small"};
      }
      return {i + 0.5};
    };
    constexpr auto const fn2 = ImmovableFn{[](double v) noexcept -> int { return std::floor(v - 0.5); }};

    return (parse(str) //
            | and_then(fn1)
            | transform_error([](Error v) noexcept -> std::runtime_error { return std::runtime_error{v.what}; })
            | transform(fn2) | inspect([&peek](double d) noexcept -> void { peek = d; })
            | inspect_error([&peek](std::runtime_error) noexcept -> void { peek = 0; })
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

  auto const e1 = fn::expected<int, Error>{0} //
                  | fn::fail([](int) noexcept -> Error { return {"Dummy"}; });
  CHECK(not e1.has_value());
  CHECK(e1.error().what == "Dummy");

  constexpr auto fn2 = [](int v) noexcept -> fn::expected<int, Error> {
    using namespace fn;
    return fn::expected<int, Error>{v}                             //
           | filter([](int v) noexcept -> bool { return v >= 0; }, //
                    [](int) noexcept -> Error { return {"Negative"}; });
  };

  CHECK(fn2(0).value() == 0);
  CHECK(fn2(42).value() == 42);

  auto const e2 = fn2(-12);
  CHECK(not e2.has_value());
  CHECK(e2.error().what == "Negative");

  constexpr auto fn3 = [](auto first, auto second) noexcept {
    using namespace fn;

    constexpr auto parse = [](char const *str) noexcept -> fn::expected<int, Error> {
      int tmp = {};
      char const *end = str + std::strlen(str);
      if (std::from_chars(str, end, tmp).ptr == end) {
        return {tmp};
      }
      return std::unexpected<Error>{"Failed to parse " + std::string(str)};
    };

    constexpr auto parse_twelve = [](std::string str) noexcept -> fn::expected<double, Error> {
      if (str != "12")
        return std::unexpected<Error>{"Not 12"};
      return {12.};
    };

    return (parse(first) & parse_twelve(second)) //
           | filter([](int a, double b) noexcept -> bool { return a > b; },
                    [](int, double) noexcept -> Error { return {"First can't be smaller than second"}; })
           | transform([](int a, double b) noexcept -> double { return a * b; });
  };

  auto const p1 = fn3("42", "wrong");
  CHECK(not p1.has_value());
  CHECK(p1.error().what == "Not 12");

  auto const p2 = fn3("10", "12");
  CHECK(not p2.has_value());
  CHECK(p2.error().what == "First can't be smaller than second");

  auto const p3 = fn3("wrong", "12");
  CHECK(not p3.has_value());
  CHECK(p3.error().what == "Failed to parse wrong");

  auto const p4 = fn3("42", "12");
  CHECK(p4.has_value());
  CHECK(p4.value() == 42 * 12);
}

TEST_CASE("Minimal optional", "[optional][and_then]")
{
  // clang-format off
  {
    // example-optional-and_then-value
    fn::optional<double> op{12.1};

    auto rounded = op
      | fn::and_then([](auto&& v) -> fn::optional<unsigned> {
          return static_cast<unsigned>(std::ceil(v + 0.5));
        });

    REQUIRE(rounded.value() == 13u);
    // example-optional-and_then-value
  }
  {
    // example-optional-and_then-empty
    fn::optional<double> op = std::nullopt;

    auto empty = op
      | fn::and_then([](auto&& v) -> fn::optional<unsigned> {
          return static_cast<unsigned>(std::ceil(v + 0.5));
        }); // Not called because op is empty

    REQUIRE(not empty.has_value());
    // example-optional-and_then-empty
  }
  {
    // example-optional-filter-value
    fn::optional<int> ex{42};

    auto value = ex
        | fn::filter([](auto &&i) { return i >= 42; }); // Filter out values less than 42

    REQUIRE(value.value() == 42);
    // example-optional-filter-value
  }
  {
    // example-optional-filter-empty
    fn::optional<int> ex{12};

    auto value = ex
        | fn::filter([](auto &&i) { return i >= 42; });

    REQUIRE(not value.has_value());
    // example-optional-filter-empty
  }
  // clang-format on
}

TEST_CASE("Demo optional", "[optional][pack][and_then][or_else][inspect][transform][fail][filter][recover]")
{
  constexpr auto fn1 = [](char const *str, int &peek) {
    using namespace fn;

    constexpr auto parse = [](char const *str) noexcept -> fn::optional<int> {
      int tmp = {};
      char const *end = str + std::strlen(str);
      if (std::from_chars(str, end, tmp).ptr == end) {
        return {tmp};
      }
      return {};
    };

    return (parse(str) //
            | and_then([](int i) noexcept -> fn::optional<int> {
                if (i > 0) {
                  return {i};
                }
                return {std::nullopt};
              })
            | inspect([&peek](int d) noexcept -> void { peek = d; })
            | inspect_error([&peek]() noexcept -> void { peek = 0; })
            | or_else([]() noexcept -> fn::optional<int> { return -13; })
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

  auto const o1 = fn::optional<int>{0} //
                  | fn::fail([](int) noexcept {}) | fn::recover([]() { return -1; });
  CHECK(o1.has_value());
  CHECK(o1.value() == -1);

  constexpr auto fn2 = [](int v) noexcept -> fn::optional<int> {
    using namespace fn;
    return fn::optional<int>{v} //
           | filter([](int v) noexcept -> bool { return v >= 0; });
  };

  CHECK(fn2(0).value() == 0);
  CHECK(fn2(42).value() == 42);
  CHECK(not fn2(-12).has_value());

  constexpr auto fn3 = [](auto first, auto second) noexcept {
    using namespace fn;

    constexpr auto parse = [](char const *str) noexcept -> fn::optional<int> {
      int tmp = {};
      char const *end = str + std::strlen(str);
      if (std::from_chars(str, end, tmp).ptr == end) {
        return {tmp};
      }
      return {};
    };

    constexpr auto parse_twelve = [](std::string str) noexcept -> fn::optional<double> {
      if (str != "12")
        return {};
      return {12.};
    };

    return (parse(first) & parse_twelve(second)) //
           | filter([](int a, double b) noexcept -> bool { return a > b; })
           | transform([](int a, double b) noexcept -> double { return a * b; });
  };

  auto const p1 = fn3("42", "wrong");
  CHECK(not p1.has_value());

  auto const p2 = fn3("10", "12"); // filter `a > b` fails
  CHECK(not p2.has_value());

  auto const p3 = fn3("wrong", "12");
  CHECK(not p3.has_value());

  auto const p4 = fn3("42", "12");
  CHECK(p4.has_value());
  CHECK(p4.value() == 42 * 12);
}

TEST_CASE("Demo choice and graded monad", "[choice][and_then][inspect][transform][graded]")
{
  // example-choice-parse
  static constexpr auto parse = [](std::string_view str) noexcept
      -> fn::choice_for<bool, double, long, std::string_view, std::nullptr_t, std::nullopt_t> {
    if (str.size() > 0) {
      if (str.size() > 1 && str[0] == '\'' && str[str.size() - 1] == '\'')
        return {str.substr(1, str.size() - 2)};
      else if (str.size() > 1 && str[0] == '\"' && str[str.size() - 1] == '\"')
        return {str.substr(1, str.size() - 2)};
      else if (str == "true")
        return {true};
      else if (str == "false")
        return {false};
      else if (str == "null")
        return {nullptr};
      else {
        if (str.find_first_not_of("01234567890") != std::string_view::npos) {
          double tmp = {};
          // TODO switch to std::from_chars when supported by libc++
          std::istringstream ss{std::string{str.data(), str.size()}};
          if ((ss >> tmp)) {
            return {tmp};
          }
        } else {
          long tmp = {};
          auto const end = str.data() + str.size();
          if (std::from_chars(str.data(), end, tmp).ptr == end) {
            return {tmp};
          }
        }
      }
      return {std::nullopt};
    }
    return {nullptr};
  };
  // example-choice-parse

  // example-choice-checks
  static_assert(std::is_same_v<decltype(parse("")),
                               fn::choice_for<bool, double, long, std::string_view, std::nullopt_t, std::nullptr_t>>);
  CHECK(parse("'abc'") == fn::choice{std::string_view{"abc"}});
  CHECK(parse(R"("def")") == fn::choice{std::string_view{"def"}});
  CHECK(parse("null") == fn::choice(nullptr));
  CHECK(parse("") == fn::choice(nullptr));
  CHECK(parse("true") == fn::choice(true));
  CHECK(parse("false") == fn::choice(false));
  CHECK(parse("1025") == fn::choice(1025l));
  CHECK(parse("10.25") == fn::choice(10.25));
  CHECK(parse("2e9") == fn::choice(2e9));
  CHECK(parse("5e9") == fn::choice(5e9));
  CHECK(parse("foo").has_value(std::in_place_type<std::nullopt_t>));
  // example-choice-checks

  std::ostringstream ss;
  auto fn = [&ss](auto const &v) {
    return parse(v)
           // Example use of transform to collapse several types ...
           | fn::transform(fn::overload([](long const &i) -> double { return static_cast<double>(i); },
                                        [](double const &i) -> double { return static_cast<double>(i); },
                                        [](std::nullopt_t const &) { return nullptr; }, //
                                        [](auto &i) { return FWD(i); }))
           // ... and add a new type
           | fn::transform(fn::overload(
               [](double v) -> fn::sum<double, int> {
                 if (std::ceil(v) == v && v >= -2e9 && v <= 2e9) {
                   return {static_cast<int>(v)};
                 }
                 return {FWD(v)};
               },
               [](auto &i) -> auto { return FWD(i); }))
           | fn::inspect(fn::overload{[&](std::nullptr_t const &) { ss << "nullptr" << ','; }, //
                                      [&](bool const &v) { ss << v << ','; },                  //
                                      [&](int const &v) { ss << v << ','; },                   //
                                      [&](double const &v) { ss << v << ','; },                //
                                      [&](std::string_view const &v) { ss << v << ','; }});
  };

  auto const a = fn("true");
  static_assert(std::is_same_v<decltype(a), fn::choice_for<bool, double, int, std::string_view, std::nullptr_t> const>);
  CHECK(a == fn::choice{true});
  CHECK(fn("123") == fn::choice(123));
  CHECK(fn("2e9") == fn::choice(2000000000));
  CHECK(fn("5e9") == fn::choice(5e9));
  CHECK(fn("") == fn::choice(nullptr));
  CHECK(fn("foo") == fn::choice(nullptr));
  CHECK(ss.str() == "1,123,2000000000,5e+09,nullptr,nullptr,");

  struct ConfigProd {
    std::string hostname;
    int port;
    std::string filename;
    double threshold;
  };

  struct ConfigTest {
    std::string hostname;
    int port;
    std::string filename;
    double threshold;
    std::string test_name;
  };

  enum InputError { InvalidType, InvalidConfiguration };
  enum ConfigError { InvalidHostname, InvalidPort, InvalidFilename, InvalidThreshold, InvalidTest };
  enum NetworkError { ConnectError, ProtocolError, Unknown };

  static constexpr auto convert = []<typename T>(std::in_place_type_t<T>, std::string_view v) {
    return parse(v).invoke([](auto &&v) -> fn::expected<T, InputError> {
      if constexpr (std::is_same_v<std::decay_t<decltype(v)>, T>) {
        return {FWD(v)};
      } else
        return std::unexpected<InputError>{InvalidType};
    });
  };

  static constexpr auto fn2 = [](std::string_view configuration, std::string_view hostname, std::string_view port,
                                 std::string_view filename, std::string_view threshold,
                                 std::string_view test_name = "") {
    return (fn::expected<void, fn::sum<>>() //
            & ([](std::string_view configuration, std::string_view test_name) {
                using type = fn::expected<
                    fn::sum<fn::pack<std::type_identity<ConfigTest>, std::string_view>, std::type_identity<ConfigProd>>,
                    InputError>;
                if (configuration == "prod")
                  return type{std::in_place, fn::sum{std::type_identity<ConfigProd>{}}};
                else if (configuration == "test")
                  return type{std::in_place, fn::sum{fn::pack{std::type_identity<ConfigTest>{}, std::move(test_name)}}};
                else
                  return type{std::unexpect, InvalidConfiguration};
              })(configuration, test_name)                            //
            & convert(std::in_place_type<std::string_view>, hostname) //
            & convert(std::in_place_type<long>, port)                 //
            & convert(std::in_place_type<std::string_view>, filename) //
            & convert(std::in_place_type<double>, threshold))         //
           | fn::transform(
               fn::overload{[](std::type_identity<ConfigProd>, std::string_view const &host, long port,
                               std::string_view const &fname, double tshold) {
                              return ConfigProd{.hostname = std::string{host},
                                                .port = static_cast<int>(port),
                                                .filename = std::string{fname},
                                                .threshold = tshold};
                            },
                            [](std::type_identity<ConfigTest>, std::string_view test_name, std::string_view const &host,
                               long port, std::string_view const &fname, double tshold) {
                              return ConfigTest{.hostname = std::string{host},
                                                .port = static_cast<int>(port),
                                                .filename = std::string{fname},
                                                .threshold = tshold,
                                                .test_name = std::string{test_name}};
                            }})
           | fn::and_then([](auto &&config) -> fn::expected<fn::sum<ConfigProd, ConfigTest>, ConfigError> {
               if (config.hostname.size() < 3 || config.hostname.size() > 127
                   || config.hostname.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789.")
                          != std::string_view::npos
                   || config.hostname.find("..") != std::string_view::npos)
                 return std::unexpected<ConfigError>(InvalidHostname);
               else if (config.port < 1 || config.port > 0xffff)
                 return std::unexpected<ConfigError>(InvalidPort);
               else if (config.filename.size() < 1 || config.filename.size() > 254)
                 return std::unexpected<ConfigError>(InvalidFilename);
               else if (config.threshold < 0 || config.threshold > 1)
                 return std::unexpected<ConfigError>(InvalidThreshold);

               if constexpr (std::is_same_v<std::remove_cvref_t<decltype(config)>, ConfigTest>) {
                 if (config.test_name != "foo")
                   return std::unexpected<ConfigError>(InvalidTest);
               }

               return FWD(config);
             })
           | fn::and_then([](auto const &config) -> fn::expected<int, NetworkError> {
               if (config.port < 1024)
                 return std::unexpected<NetworkError>(ConnectError);
               if constexpr (std::is_same_v<decltype(config), ConfigProd const &>)
                 return {0x50eda7a}; // dummy result
               else
                 return {0x31eda7a + config.test_name.size()}; // dummy result
             });
  };

  auto const b = fn2("prod", "123", "1024", "'file.txt'", "0.5"); // 123 is not valid hostname
  static_assert(std::is_same_v<decltype(b), fn::expected<int, fn::sum<ConfigError, InputError, NetworkError>> const>);
  CHECK(b.error() == fn::sum{InvalidType});
  CHECK(fn2("foobar", "'localhost'", "1024", "'file.txt'", "0.5").error() == fn::sum{InvalidConfiguration});
  CHECK(fn2("test", "123", "1024", "'file.txt'", "-1.0").error() == fn::sum{InvalidType});
  CHECK(fn2("test", "'localhost'", "0", "'file.txt'", "-1.0", "foo").error() == fn::sum{InvalidPort});
  CHECK(fn2("prod", "'localhost'", "'foo'", "'file.txt'", "0.5").error() == fn::sum{InvalidType});
  CHECK(fn2("prod", "'..'", "0", "''", "0").error() == fn::sum{InvalidType}); // 0 is not a double
  CHECK(fn2("prod", "'..'", "1024", "'file.txt'", "0.5").error() == fn::sum{InvalidHostname});
  CHECK(fn2("prod", "'..'", "0", "''", "0.5").error() == fn::sum{InvalidHostname}); // hostname is bound first
  CHECK(fn2("prod", "'localhost'", "0", "'file.txt'", "0.5").error() == fn::sum{InvalidPort});
  CHECK(fn2("prod", "'localhost'", "1024", "''", "0.5").error() == fn::sum{InvalidFilename});
  CHECK(fn2("prod", "'localhost'", "1024", "'file.txt'", "-1.0").error() == fn::sum{InvalidThreshold});
  CHECK(fn2("test", "'localhost'", "1024", "'file.txt'", "1.0").error() == fn::sum{InvalidTest});
  CHECK(fn2("test", "'localhost'", "1024", "'file.txt'", "1.0", "bar").error() == fn::sum{InvalidTest});
  CHECK(fn2("prod", "'localhost'", "1023", "'file.txt'", "0.5").error() == fn::sum{ConnectError});
  CHECK(fn2("prod", "'localhost'", "1024", "'file.txt'", "0.5").value() == 0x50eda7a);              // dummy result
  CHECK(fn2("test", "'localhost'", "1024", "'file.txt'", "0.5", "foo").value() == (0x31eda7a + 3)); // dummy result
}
