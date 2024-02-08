// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/optional.hpp"
#include "functional/utility.hpp"

#include <catch2/catch_all.hpp>

#include <utility>

TEST_CASE("optional polyfills and_then", "[optional][polyfill][and_then]")
{
  WHEN("value")
  {
    fn::optional<int> s{12};
    CHECK(s.and_then( //
               fn::overload([](int &i) -> fn::optional<bool> { return i == 12; },
                            [](int const &) -> fn::optional<bool> { throw 0; },
                            [](int &&) -> fn::optional<bool> { throw 0; },
                            [](int const &&) -> fn::optional<bool> { throw 0; })) //
              .value());
    CHECK(std::as_const(s)
              .and_then( //
                  fn::overload([](int &) -> fn::optional<bool> { throw 0; },
                               [](int const &i) -> fn::optional<bool> { return i == 12; },
                               [](int &&) -> fn::optional<bool> { throw 0; },
                               [](int const &&) -> fn::optional<bool> { throw 0; })) //
              .value());
    CHECK(fn::optional<int>{12}
              .and_then( //
                  fn::overload([](int &) -> fn::optional<bool> { throw 0; },
                               [](int const &) -> fn::optional<bool> { throw 0; },
                               [](int &&i) -> fn::optional<bool> { return i == 12; },
                               [](int const &&) -> fn::optional<bool> { throw 0; })) //
              .value());
    CHECK(std::move(std::as_const(s))
              .and_then( //
                  fn::overload([](int &) -> fn::optional<bool> { throw 0; },
                               [](int const &) -> fn::optional<bool> { throw 0; },
                               [](int &&) -> fn::optional<bool> { throw 0; },
                               [](int const &&i) -> fn::optional<bool> { return i == 12; })) //
              .value());

    WHEN("error")
    {
      fn::optional<int> s{};
      CHECK(not s.and_then( //
                     [](auto) -> fn::optional<bool> { throw 0; })
                    .has_value());
      CHECK(not std::as_const(s)
                    .and_then( //
                        [](auto) -> fn::optional<bool> { throw 0; })
                    .has_value());
      CHECK(not fn::optional<int>{}
                    .and_then( //
                        [](auto) -> fn::optional<bool> { throw 0; })
                    .has_value());
      CHECK(not std::move(std::as_const(s))
                    .and_then( //
                        [](auto) -> fn::optional<bool> { throw 0; })
                    .has_value());
    }
  }
}

TEST_CASE("optional polyfills or_else", "[optional][polyfill][or_else]")
{
  WHEN("value")
  {
    fn::optional<int> s{1};
    CHECK(s.or_else([]() -> fn::optional<int> { throw 0; }).value());
    CHECK(std::as_const(s).or_else([]() -> fn::optional<int> { throw 0; }).value());
    CHECK(fn::optional<int>{1}.or_else([]() -> fn::optional<int> { throw 0; }).value());
    CHECK(std::move(std::as_const(s)).or_else([]() -> fn::optional<int> { throw 0; }).value());

    WHEN("error")
    {
      fn::optional<int> s{};
      CHECK(s.or_else([]() -> fn::optional<int> { return 12; }).value() == 12);
      CHECK(std::as_const(s).or_else([]() -> fn::optional<int> { return 12; }).value() == 12);
      CHECK(fn::optional<int>{}.or_else([]() -> fn::optional<int> { return 12; }).value() == 12);
      CHECK(std::move(std::as_const(s)).or_else([]() -> fn::optional<int> { return 12; }).value() == 12);
    }
  }
}

TEST_CASE("optional polyfills transform", "[optional][polyfill][transform]")
{
  WHEN("value")
  {
    fn::optional<int> s{12};
    CHECK(s.transform( //
               fn::overload([](int &i) -> bool { return i == 12; }, [](int const &) -> bool { throw 0; },
                            [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; })) //
              .value());
    CHECK(std::as_const(s)
              .transform( //
                  fn::overload([](int &) -> bool { throw 0; }, [](int const &i) -> bool { return i == 12; },
                               [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; })) //
              .value());
    CHECK(fn::optional<int>{12}
              .transform( //
                  fn::overload([](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                               [](int &&i) -> bool { return i == 12; }, [](int const &&) -> bool { throw 0; })) //
              .value());
    CHECK(std::move(std::as_const(s))
              .transform( //
                  fn::overload([](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                               [](int &&) -> bool { throw 0; }, [](int const &&i) -> bool { return i == 12; })) //
              .value());

    WHEN("error")
    {
      fn::optional<int> s{};
      CHECK(not s.transform([](auto) -> bool { throw 0; }).has_value());
      CHECK(not std::as_const(s).transform([](auto) -> bool { throw 0; }).has_value());
      CHECK(not fn::optional<int>{}.transform([](auto) -> bool { throw 0; }).has_value());
      CHECK(not std::move(std::as_const(s)).transform([](auto) -> bool { throw 0; }).has_value());
    }
  }
}
