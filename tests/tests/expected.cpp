// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/expected.hpp"
#include "functional/utility.hpp"

#include <catch2/catch_all.hpp>

#include <utility>

namespace {
enum Error { Unknown, FileNotFound };
}

TEST_CASE("expected polyfills and_then", "[expected][polyfill][and_then]")
{
  WHEN("value")
  {
    fn::expected<int, Error> s{12};
    CHECK(s.and_then( //
               fn::overload([](int &i) -> fn::expected<bool, Error> { return i == 12; },
                            [](int const &) -> fn::expected<bool, Error> { throw 0; },
                            [](int &&) -> fn::expected<bool, Error> { throw 0; },
                            [](int const &&) -> fn::expected<bool, Error> { throw 0; })) //
              .value());
    CHECK(std::as_const(s)
              .and_then( //
                  fn::overload([](int &) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &i) -> fn::expected<bool, Error> { return i == 12; },
                               [](int &&) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &&) -> fn::expected<bool, Error> { throw 0; })) //
              .value());
    CHECK(fn::expected<int, Error>{12}
              .and_then( //
                  fn::overload([](int &) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &) -> fn::expected<bool, Error> { throw 0; },
                               [](int &&i) -> fn::expected<bool, Error> { return i == 12; },
                               [](int const &&) -> fn::expected<bool, Error> { throw 0; })) //
              .value());
    CHECK(std::move(std::as_const(s))
              .and_then( //
                  fn::overload([](int &) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &) -> fn::expected<bool, Error> { throw 0; },
                               [](int &&) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &&i) -> fn::expected<bool, Error> { return i == 12; })) //
              .value());

    WHEN("error")
    {
      fn::expected<int, Error> s{std::unexpect, Unknown};
      CHECK(s.and_then( //
                 [](auto) -> fn::expected<bool, Error> { throw 0; })
                .error()
            == Unknown);
      CHECK(std::as_const(s)
                .and_then( //
                    [](auto) -> fn::expected<bool, Error> { throw 0; })
                .error()
            == Unknown);
      CHECK(fn::expected<int, Error>{std::unexpect, Unknown}
                .and_then( //
                    [](auto) -> fn::expected<bool, Error> { throw 0; })
                .error()
            == Unknown);
      CHECK(std::move(std::as_const(s))
                .and_then( //
                    [](auto) -> fn::expected<bool, Error> { throw 0; })
                .error()
            == Unknown);
    }
  }

  WHEN("void")
  {
    fn::expected<void, Error> s{};
    CHECK(s.and_then([]() -> fn::expected<bool, Error> { return true; }).value());
    CHECK(std::as_const(s).and_then([]() -> fn::expected<bool, Error> { return true; }).value());
    CHECK(fn::expected<void, Error>{}.and_then([]() -> fn::expected<bool, Error> { return true; }).value());
    CHECK(std::move(std::as_const(s)).and_then([]() -> fn::expected<bool, Error> { return true; }).value());

    WHEN("error")
    {
      fn::expected<void, Error> s{std::unexpect, Unknown};
      CHECK(s.and_then([]() -> fn::expected<bool, Error> { throw 0; }).error() == Unknown);
      CHECK(std::as_const(s).and_then([]() -> fn::expected<bool, Error> { throw 0; }).error() == Unknown);
      CHECK(fn::expected<void, Error>{std::unexpect, Unknown}
                .and_then([]() -> fn::expected<bool, Error> { throw 0; })
                .error()
            == Unknown);
      CHECK(std::move(std::as_const(s)).and_then([]() -> fn::expected<bool, Error> { throw 0; }).error() == Unknown);
    }
  }
}

TEST_CASE("expected polyfills or_else", "[expected][polyfill][or_else]")
{
  WHEN("value")
  {
    fn::expected<int, Error> s{1};
    CHECK(s.or_else([](auto) -> fn::expected<int, Error> { throw 0; }).value());
    CHECK(std::as_const(s).or_else([](auto) -> fn::expected<int, Error> { throw 0; }).value());
    CHECK(fn::expected<int, Error>{1}.or_else([](auto) -> fn::expected<int, Error> { throw 0; }).value());
    CHECK(std::move(std::as_const(s)).or_else([](auto) -> fn::expected<int, Error> { throw 0; }).value());

    WHEN("error")
    {
      fn::expected<int, Error> s{std::unexpect, FileNotFound};
      CHECK(s.or_else( //
                 fn::overload([](Error &e) -> fn::expected<int, Error> { return e == FileNotFound; },
                              [](Error const &) -> fn::expected<int, Error> { throw 0; },
                              [](Error &&) -> fn::expected<int, Error> { throw 0; },
                              [](Error const &&) -> fn::expected<int, Error> { throw 0; })) //
                .value());
      CHECK(std::as_const(s)
                .or_else( //
                    fn::overload([](Error &) -> fn::expected<int, Error> { throw 0; },
                                 [](Error const &e) -> fn::expected<int, Error> { return e == FileNotFound; },
                                 [](Error &&) -> fn::expected<int, Error> { throw 0; },
                                 [](Error const &&) -> fn::expected<int, Error> { throw 0; })) //
                .value());
      CHECK(fn::expected<int, Error>{std::unexpect, FileNotFound}
                .or_else( //
                    fn::overload([](Error &) -> fn::expected<int, Error> { throw 0; },
                                 [](Error const &) -> fn::expected<int, Error> { throw 0; },
                                 [](Error &&e) -> fn::expected<int, Error> { return e == FileNotFound; },
                                 [](Error const &&) -> fn::expected<int, Error> { throw 0; })) //
                .value());
      CHECK(std::move(std::as_const(s))
                .or_else( //
                    fn::overload([](Error &) -> fn::expected<int, Error> { throw 0; },
                                 [](Error const &) -> fn::expected<int, Error> { throw 0; },
                                 [](Error &&) -> fn::expected<int, Error> { throw 0; },
                                 [](Error const &&e) -> fn::expected<int, Error> { return e == FileNotFound; })) //
                .value());
    }
  }

  WHEN("void")
  {
    fn::expected<void, Error> s{};
    CHECK(s.or_else([](auto) -> fn::expected<void, Error> { throw 0; }).has_value());
    CHECK(std::as_const(s).or_else([](auto) -> fn::expected<void, Error> { throw 0; }).has_value());
    CHECK(fn::expected<void, Error>{}.or_else([](auto) -> fn::expected<void, Error> { throw 0; }).has_value());
    CHECK(std::move(std::as_const(s)).or_else([](auto) -> fn::expected<void, Error> { throw 0; }).has_value());

    WHEN("error")
    {
      fn::expected<void, Error> s{std::unexpect, FileNotFound};
      CHECK(s.or_else( //
                 fn::overload([](Error &) -> fn::expected<void, Error> { return {}; },
                              [](Error const &) -> fn::expected<void, Error> { throw 0; },
                              [](Error &&) -> fn::expected<void, Error> { throw 0; },
                              [](Error const &&) -> fn::expected<void, Error> { throw 0; }))
                .has_value());
      CHECK(std::as_const(s)
                .or_else( //
                    fn::overload([](Error &) -> fn::expected<void, Error> { throw 0; },
                                 [](Error const &) -> fn::expected<void, Error> { return {}; },
                                 [](Error &&) -> fn::expected<void, Error> { throw 0; },
                                 [](Error const &&) -> fn::expected<void, Error> { throw 0; }))
                .has_value());
      CHECK(fn::expected<void, Error>{std::unexpect, FileNotFound}
                .or_else( //
                    fn::overload([](Error &) -> fn::expected<void, Error> { throw 0; },
                                 [](Error const &) -> fn::expected<void, Error> { throw 0; },
                                 [](Error &&) -> fn::expected<void, Error> { return {}; },
                                 [](Error const &&) -> fn::expected<void, Error> { throw 0; }))
                .has_value());
      CHECK(std::move(std::as_const(s))
                .or_else( //
                    fn::overload([](Error &) -> fn::expected<void, Error> { throw 0; },
                                 [](Error const &) -> fn::expected<void, Error> { throw 0; },
                                 [](Error &&) -> fn::expected<void, Error> { throw 0; },
                                 [](Error const &&) -> fn::expected<void, Error> { return {}; }))
                .has_value());
    }
  }
}

TEST_CASE("expected polyfills transform", "[expected][polyfill][transform]")
{
  WHEN("value")
  {
    fn::expected<int, Error> s{12};
    CHECK(s.transform( //
               fn::overload([](int &i) -> bool { return i == 12; }, [](int const &) -> bool { throw 0; },
                            [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; })) //
              .value());
    CHECK(std::as_const(s)
              .transform( //
                  fn::overload([](int &) -> bool { throw 0; }, [](int const &i) -> bool { return i == 12; },
                               [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; })) //
              .value());
    CHECK(fn::expected<int, Error>{12}
              .transform( //
                  fn::overload([](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                               [](int &&i) -> bool { return i == 12; }, [](int const &&) -> bool { throw 0; })) //
              .value());
    CHECK(std::move(std::as_const(s))
              .transform( //
                  fn::overload([](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                               [](int &&) -> bool { throw 0; }, [](int const &&i) -> bool { return i == 12; })) //
              .value());

    WHEN("void result")
    {
      fn::expected<int, Error> s{12};
      CHECK(s.transform( //
                 fn::overload([](int &) -> void {}, [](int const &) -> void { throw 0; },
                              [](int &&) -> void { throw 0; }, [](int const &&) -> void { throw 0; })) //
                .has_value());
      CHECK(std::as_const(s)
                .transform( //
                    fn::overload([](int &) -> void { throw 0; }, [](int const &) -> void {},
                                 [](int &&) -> void { throw 0; }, [](int const &&) -> void { throw 0; })) //
                .has_value());
      CHECK(fn::expected<int, Error>{12}
                .transform( //
                    fn::overload([](int &) -> void { throw 0; }, [](int const &) -> void { throw 0; },
                                 [](int &&) -> void {}, [](int const &&) -> void { throw 0; })) //
                .has_value());
      CHECK(std::move(std::as_const(s))
                .transform( //
                    fn::overload([](int &) -> void { throw 0; }, [](int const &) -> void { throw 0; },
                                 [](int &&) -> void { throw 0; }, [](int const &&) -> void {})) //
                .has_value());
    }

    WHEN("error")
    {
      fn::expected<int, Error> s{std::unexpect, Unknown};
      CHECK(s.transform([](auto) -> bool { throw 0; }).error() == Unknown);
      CHECK(std::as_const(s).transform([](auto) -> bool { throw 0; }).error() == Unknown);
      CHECK(fn::expected<int, Error>{std::unexpect, Unknown}.transform([](auto) -> bool { throw 0; }).error()
            == Unknown);
      CHECK(std::move(std::as_const(s)).transform([](auto) -> bool { throw 0; }).error() == Unknown);
    }
  }

  WHEN("void")
  {
    fn::expected<void, Error> s{};
    CHECK(s.transform([]() -> bool { return true; }).value());
    CHECK(std::as_const(s).transform([]() -> bool { return true; }).value());
    CHECK(fn::expected<void, Error>{}.transform([]() -> bool { return true; }).value());
    CHECK(std::move(std::as_const(s)).transform([]() -> bool { return true; }).value());

    WHEN("void result")
    {
      fn::expected<void, Error> s{};
      CHECK(s.transform([]() -> void {}).has_value());
      CHECK(std::as_const(s).transform([]() -> void {}).has_value());
      CHECK(fn::expected<void, Error>{}.transform([]() -> void {}).has_value());
      CHECK(std::move(std::as_const(s)).transform([]() -> void {}).has_value());
    }

    WHEN("error")
    {
      fn::expected<void, Error> s{std::unexpect, Unknown};
      CHECK(s.transform([]() -> bool { throw 0; }).error() == Unknown);
      CHECK(std::as_const(s).transform([]() -> bool { throw 0; }).error() == Unknown);
      CHECK(fn::expected<void, Error>{std::unexpect, Unknown}.transform([]() -> bool { throw 0; }).error() == Unknown);
      CHECK(std::move(std::as_const(s)).transform([]() -> bool { throw 0; }).error() == Unknown);
    }
  }
}

TEST_CASE("expected polyfills transform_error", "[expected][polyfill][transform_error]")
{
  WHEN("value")
  {
    fn::expected<int, Error> s{12};
    CHECK(s.transform_error([](Error) -> bool { throw 0; }).value() == 12);
    CHECK(std::as_const(s).transform_error([](Error) -> bool { throw 0; }).value() == 12);
    CHECK(fn::expected<int, Error>{12}.transform_error([](Error) -> bool { throw 0; }).value() == 12);
    CHECK(std::move(std::as_const(s)).transform_error([](Error) -> bool { throw 0; }).value() == 12);

    WHEN("error")
    {
      fn::expected<int, Error> s{std::unexpect, FileNotFound};
      CHECK(
          s.transform_error( //
               fn::overload([](Error &e) -> bool { return e == FileNotFound; }, [](Error const &) -> bool { throw 0; },
                            [](Error &&) -> bool { throw 0; }, [](Error const &&) -> bool { throw 0; })) //
              .error());
      CHECK(std::as_const(s)
                .transform_error( //
                    fn::overload([](Error &) -> bool { throw 0; },
                                 [](Error const &e) -> bool { return e == FileNotFound; },
                                 [](Error &&) -> bool { throw 0; }, [](Error const &&) -> bool { throw 0; })) //
                .error());
      CHECK(fn::expected<int, Error>{std::unexpect, FileNotFound}
                .transform_error( //
                    fn::overload([](Error &) -> bool { throw 0; }, [](Error const &) -> bool { throw 0; },
                                 [](Error &&e) -> bool { return e == FileNotFound; },
                                 [](Error const &&) -> bool { throw 0; })) //
                .error());
      CHECK(std::move(std::as_const(s))
                .transform_error( //
                    fn::overload([](Error &) -> bool { throw 0; }, [](Error const &) -> bool { throw 0; },
                                 [](Error &&) -> bool { throw 0; },
                                 [](Error const &&e) -> bool { return e == FileNotFound; })) //
                .error());
    }
  }

  WHEN("void")
  {
    fn::expected<void, Error> s{};
    CHECK(s.transform_error([](auto) -> bool { throw 0; }).has_value());
    CHECK(std::as_const(s).transform_error([](auto) -> bool { throw 0; }).has_value());
    CHECK(fn::expected<void, Error>{}.transform_error([](auto) -> bool { throw 0; }).has_value());
    CHECK(std::move(std::as_const(s)).transform_error([](auto) -> bool { throw 0; }).has_value());

    WHEN("error")
    {
      fn::expected<void, Error> s{std::unexpect, FileNotFound};
      CHECK(s.transform_error( //
                 fn::overload([](Error &) -> bool { return true; }, [](Error const &) -> bool { throw 0; },
                              [](Error &&) -> bool { throw 0; }, [](Error const &&) -> bool { throw 0; }))
                .error());
      CHECK(std::as_const(s)
                .transform_error( //
                    fn::overload([](Error &) -> bool { throw 0; }, [](Error const &) -> bool { return true; },
                                 [](Error &&) -> bool { throw 0; }, [](Error const &&) -> bool { throw 0; }))
                .error());
      CHECK(fn::expected<void, Error>{std::unexpect, FileNotFound}
                .transform_error( //
                    fn::overload([](Error &) -> bool { throw 0; }, [](Error const &) -> bool { throw 0; },
                                 [](Error &&) -> bool { return true; }, [](Error const &&) -> bool { throw 0; }))
                .error());
      CHECK(std::move(std::as_const(s))
                .transform_error( //
                    fn::overload([](Error &) -> bool { throw 0; }, [](Error const &) -> bool { throw 0; },
                                 [](Error &&) -> bool { throw 0; }, [](Error const &&) -> bool { return true; }))
                .error());
    }
  }
}
