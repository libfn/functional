// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/optional.hpp"
#include "functional/utility.hpp"

#include <catch2/catch_all.hpp>

#include <utility>

TEST_CASE("optional pack support", "[optional][pack][and_then][transform][operator_and]")
{
  WHEN("and_then")
  {
    WHEN("value")
    {
      fn::optional<fn::pack<int, std::string_view>> s{
          fn::pack<int>{12}.append(std::in_place_type<std::string_view>, "bar")};

      CHECK(s.and_then( //
                 fn::overload([](int &i, auto &&...) -> fn::optional<bool> { return i == 12; },
                              [](int const &, auto &&...) -> fn::optional<bool> { throw 0; },
                              [](int &&, auto &&...) -> fn::optional<bool> { throw 0; },
                              [](int const &&, auto &&...) -> fn::optional<bool> { return 0; })) //
                .value());
      CHECK(std::as_const(s)
                .and_then( //
                    fn::overload([](int &, auto &&...) -> fn::optional<bool> { throw 0; },
                                 [](int const &i, auto &&...) -> fn::optional<bool> { return i == 12; },
                                 [](int &&, auto &&...) -> fn::optional<bool> { throw 0; },
                                 [](int const &&, auto &&...) -> fn::optional<bool> { throw 0; })) //
                .value());
      CHECK(fn::optional<fn::pack<int, std::string_view>>{
          fn::pack<int>{12}.append(std::in_place_type<std::string_view>, "bar")}
                .and_then( //
                    fn::overload([](int &, auto &&...) -> fn::optional<bool> { throw 0; },
                                 [](int const &, auto &&...) -> fn::optional<bool> { throw 0; },
                                 [](int &&i, auto &&...) -> fn::optional<bool> { return i == 12; },
                                 [](int const &&, auto &&...) -> fn::optional<bool> { throw 0; })) //
                .value());
      CHECK(std::move(std::as_const(s))
                .and_then( //
                    fn::overload([](int &, auto &&...) -> fn::optional<bool> { throw 0; },
                                 [](int const &, auto &&...) -> fn::optional<bool> { throw 0; },
                                 [](int &&, auto &&...) -> fn::optional<bool> { throw 0; },
                                 [](int const &&i, auto &&...) -> fn::optional<bool> { return i == 12; })) //
                .value());
    }

    WHEN("error")
    {
      fn::optional<fn::pack<int, std::string_view>> s{std::nullopt};
      CHECK(not s.and_then( //
                     [](auto...) -> fn::optional<bool> { throw 0; })
                    .has_value());
      CHECK(not std::as_const(s)
                    .and_then( //
                        [](auto...) -> fn::optional<bool> { throw 0; })
                    .has_value());
      CHECK(not fn::optional<fn::pack<int, std::string_view>>{std::nullopt}
                    .and_then( //
                        [](auto...) -> fn::optional<bool> { throw 0; })
                    .has_value());
      CHECK(not std::move(std::as_const(s))
                    .and_then( //
                        [](auto...) -> fn::optional<bool> { throw 0; })
                    .has_value());
    }
  }

  WHEN("transform")
  {
    WHEN("value")
    {
      fn::optional<fn::pack<int, std::string_view>> s{
          fn::pack<int>{12}.append(std::in_place_type<std::string_view>, "bar")};

      CHECK(s.transform( //
                 fn::overload([](int &i, auto &&...) -> bool { return i == 12; },
                              [](int const &, auto &&...) -> bool { throw 0; },
                              [](int &&, auto &&...) -> bool { throw 0; },
                              [](int const &&, auto &&...) -> bool { throw 0; })) //
                .value());
      CHECK(std::as_const(s)
                .transform( //
                    fn::overload([](int &, auto &&...) -> bool { throw 0; },
                                 [](int const &i, auto &&...) -> bool { return i == 12; },
                                 [](int &&, auto &&...) -> bool { throw 0; },
                                 [](int const &&, auto &&...) -> bool { throw 0; })) //
                .value());
      CHECK(fn::optional<fn::pack<int, std::string_view>>{
          fn::pack<int>{12}.append(std::in_place_type<std::string_view>, "bar")}
                .transform( //
                    fn::overload([](int &, auto &&...) -> bool { throw 0; },
                                 [](int const &, auto &&...) -> bool { throw 0; },
                                 [](int &&i, auto &&...) -> bool { return i == 12; },
                                 [](int const &&, auto &&...) -> bool { throw 0; })) //
                .value());
      CHECK(std::move(std::as_const(s))
                .transform( //
                    fn::overload([](int &, auto &&...) -> bool { throw 0; },
                                 [](int const &, auto &&...) -> bool { throw 0; },
                                 [](int &&, auto &&...) -> bool { throw 0; },
                                 [](int const &&i, auto &&...) -> bool { return i == 12; })) //
                .value());
    }

    WHEN("error")
    {
      fn::optional<fn::pack<int, std::string_view>> s{std::nullopt};
      CHECK(not s.transform([](auto...) -> bool { throw 0; }).has_value());
      CHECK(not std::as_const(s).transform([](auto...) -> bool { throw 0; }).has_value());
      CHECK(not fn::optional<fn::pack<int, std::string_view>>{std::nullopt}
                    .transform([](auto...) -> bool { throw 0; })
                    .has_value());
      CHECK(not std::move(std::as_const(s)).transform([](auto...) -> bool { throw 0; }).has_value());
    }
  }

  WHEN("operator &")
  {
    WHEN("value & value yield pack")
    {
      static_assert(std::same_as<decltype(std::declval<fn::optional<int>>() & std::declval<fn::optional<double>>()),
                                 fn::optional<fn::pack<int, double>>>);

      CHECK((fn::optional<double>{0.5} //
             & fn::optional<int>{12})
                .transform([](double d, int i) constexpr -> bool { return d == 0.5 && i == 12; })
                .value());
      CHECK(not(fn::optional<double>{std::nullopt} //
                & fn::optional<int>{12})
                   .has_value());
      CHECK(not(fn::optional<double>{0.5} //
                & fn::optional<int>{std::nullopt})
                   .has_value());
      CHECK(not(fn::optional<double>{std::nullopt} //
                & fn::optional<int>{std::nullopt})
                   .has_value());
    }

    WHEN("pack & value yield pack")
    {
      static_assert(std::same_as<decltype(std::declval<fn::optional<fn::pack<double, bool>>>()
                                          & std::declval<fn::optional<int>>()),
                                 fn::optional<fn::pack<double, bool, int>>>);

      CHECK((fn::optional<fn::pack<double, bool>>{std::in_place, fn::pack<double, bool>{0.5, true}} //
             & fn::optional<int>{12})
                .transform([](double d, bool b, int i) constexpr -> bool { return d == 0.5 && b && i == 12; })
                .value());
      CHECK(not(fn::optional<fn::pack<double, bool>>{std::nullopt} //
                & fn::optional<int>{12})
                   .has_value());
      CHECK(not(fn::optional<fn::pack<double, bool>>{std::in_place, fn::pack<double, bool>{0.5, true}} //
                & fn::optional<int>{std::nullopt})
                   .has_value());
      CHECK(not(fn::optional<fn::pack<double, bool>>{std::nullopt} //
                & fn::optional<int>{std::nullopt})
                   .has_value());
    }

    WHEN("value & pack is unsupported")
    {
      static_assert([](auto &&lh,                                                          //
                       auto &&rh) constexpr -> bool { return not requires { lh & rh; }; }( //
                                                fn::optional<int>{12}, fn::optional<fn::pack<double, bool>>{
                                                                           fn::pack<double, bool>{0.5, true}}));
    }
    WHEN("pack & pack is unsupported")
    {
      static_assert(
          [](auto &&lh,
             auto &&rh) constexpr -> bool { return not requires { lh & rh; }; }( //
                                      fn::optional<fn::pack<double, bool>>{fn::pack<double, bool>{0.5, true}},
                                      fn::optional<fn::pack<double, bool>>{fn::pack<double, bool>{0.5, true}}));
    }
  }
}

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
