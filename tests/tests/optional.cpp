// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/optional.hpp"
#include "functional/utility.hpp"

#include <catch2/catch_all.hpp>

#include <utility>

namespace {
struct Xint {
  int v = {};

  constexpr bool operator==(Xint const &) const noexcept = default;
  constexpr explicit Xint(int i) : v(i) {}
  constexpr ~Xint() = default;
  constexpr Xint(Xint const &) = default;
  constexpr Xint &operator=(Xint const &) = default;
};
} // namespace

TEST_CASE("optional graded monad", "[optional][sum][graded][or_else][sum_value]")
{
  WHEN("sum_value from sum")
  {
    using T = fn::optional<fn::sum<int>>;
    T s{12};
    static_assert(std::is_same_v<decltype(s.sum_value()), T &>);
    static_assert(std::is_same_v<decltype(std::as_const(s).sum_value()), T const &>);
    static_assert(std::is_same_v<decltype(std::move(std::as_const(s)).sum_value()), T const &&>);
    static_assert(std::is_same_v<decltype(std::move(s).sum_value()), T &&>);
    WHEN("value")
    {
      CHECK(s.sum_value().value() == fn::sum{12});
      CHECK(std::as_const(s).sum_value().value() == fn::sum{12});
      CHECK(std::move(std::as_const(s)).sum_value().value() == fn::sum{12});
      CHECK(std::move(s).sum_value().value() == fn::sum{12});
    }
    WHEN("error")
    {
      T s{std::nullopt};
      CHECK(not s.sum_value().has_value());
      CHECK(not std::as_const(s).sum_value().has_value());
      CHECK(not std::move(std::as_const(s)).sum_value().has_value());
      CHECK(not std::move(s).sum_value().has_value());
    }
  }

  WHEN("sum_value from non-sum")
  {
    using T = fn::optional<int>;
    T s{12};
    static_assert(std::is_same_v<decltype(s.sum_value()), fn::optional<fn::sum<int>>>);
    static_assert(std::is_same_v<decltype(std::as_const(s).sum_value()), fn::optional<fn::sum<int>>>);
    static_assert(std::is_same_v<decltype(std::move(std::as_const(s)).sum_value()), fn::optional<fn::sum<int>>>);
    static_assert(std::is_same_v<decltype(std::move(s).sum_value()), fn::optional<fn::sum<int>>>);
    WHEN("value")
    {
      CHECK(s.sum_value().value() == fn::sum{12});
      CHECK(std::as_const(s).sum_value().value() == fn::sum{12});
      CHECK(std::move(std::as_const(s)).sum_value().value() == fn::sum{12});
      CHECK(std::move(s).sum_value().value() == fn::sum{12});
    }
    WHEN("error")
    {
      T s{std::nullopt};
      CHECK(not s.sum_value().has_value());
      CHECK(not std::as_const(s).sum_value().has_value());
      CHECK(not std::move(std::as_const(s)).sum_value().has_value());
      CHECK(not std::move(s).sum_value().has_value());
    }
  }

  WHEN("or_else")
  {
    fn::optional<fn::sum<int>> s{std::nullopt};

    constexpr auto fn1 = []() -> fn::optional<Xint> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn1)), fn::optional<fn::sum<Xint, int>>>);
    constexpr auto fn2 = []() -> fn::optional<int> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn2)), fn::optional<fn::sum<int>>>);
    constexpr auto fn3 = []() -> fn::optional<fn::sum<int>> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn3)), fn::optional<fn::sum<int>>>);
    constexpr auto fn4 = []() -> fn::optional<fn::sum<Xint>> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn4)), fn::optional<fn::sum<Xint, int>>>);
    constexpr auto fn5 = []() -> fn::optional<fn::sum<Xint, int>> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn5)), fn::optional<fn::sum<Xint, int>>>);
    constexpr auto fn6 = []() -> fn::optional<fn::sum<Xint, long>> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn6)), fn::optional<fn::sum<Xint, int, long>>>);
    constexpr auto fn7 = []() -> fn::optional<fn::sum<Xint, int, long>> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn7)), fn::optional<fn::sum<Xint, int, long>>>);
    constexpr auto fn8 = []() -> fn::optional<fn::sum<Xint, int, long>> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn8)), fn::optional<fn::sum<Xint, int, long>>>);

    WHEN("error to value")
    {
      constexpr auto fn = []() -> fn::optional<Xint> { return {Xint{12}}; };
      static_assert(std::is_same_v<decltype(s.or_else(fn)), fn::optional<fn::sum<Xint, int>>>);
      CHECK(s.or_else(fn).value() == fn::sum{Xint{12}});
      CHECK(std::as_const(s).or_else(fn).value() == fn::sum{Xint{12}});
      CHECK(std::move(std::as_const(s)).or_else(fn).value() == fn::sum{Xint{12}});
      CHECK(std::move(s).or_else(fn).value() == fn::sum{Xint{12}});
    }

    WHEN("error to error")
    {
      constexpr auto fn = []() -> fn::optional<Xint> { return {std::nullopt}; };
      static_assert(std::is_same_v<decltype(s.or_else(fn)), fn::optional<fn::sum<Xint, int>>>);
      CHECK(not s.or_else(fn).has_value());
      CHECK(not std::as_const(s).or_else(fn).has_value());
      CHECK(not std::move(std::as_const(s)).or_else(fn).has_value());
      CHECK(not std::move(s).or_else(fn).has_value());
    }

    WHEN("value")
    {
      fn::optional<fn::sum<int>> s{fn::sum{12}};
      constexpr auto fn = []() -> fn::optional<Xint> { throw 0; };
      static_assert(std::is_same_v<decltype(s.or_else(fn)), fn::optional<fn::sum<Xint, int>>>);
      CHECK(s.or_else(fn).value() == fn::sum{12});
      CHECK(std::as_const(s).or_else(fn).value() == fn::sum{12});
      CHECK(std::move(std::as_const(s)).or_else(fn).value() == fn::sum{12});
      CHECK(std::move(s).or_else(fn).value() == fn::sum{12});
    }
  }
}

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
      CHECK(std::move(std::as_const(s))
                .and_then( //
                    fn::overload([](int &, auto &&...) -> fn::optional<bool> { throw 0; },
                                 [](int const &, auto &&...) -> fn::optional<bool> { throw 0; },
                                 [](int &&, auto &&...) -> fn::optional<bool> { throw 0; },
                                 [](int const &&i, auto &&...) -> fn::optional<bool> { return i == 12; })) //
                .value());
      CHECK(std::move(s)
                .and_then( //
                    fn::overload([](int &, auto &&...) -> fn::optional<bool> { throw 0; },
                                 [](int const &, auto &&...) -> fn::optional<bool> { throw 0; },
                                 [](int &&i, auto &&...) -> fn::optional<bool> { return i == 12; },
                                 [](int const &&, auto &&...) -> fn::optional<bool> { throw 0; })) //
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
      CHECK(not std::move(std::as_const(s))
                    .and_then( //
                        [](auto...) -> fn::optional<bool> { throw 0; })
                    .has_value());
      CHECK(not std::move(s)
                    .and_then( //
                        fn::overload([](int &, auto &&...) -> fn::optional<bool> { throw 0; },
                                     [](int const &, auto &&...) -> fn::optional<bool> { throw 0; },
                                     [](int &&i, auto &&...) -> fn::optional<bool> { return i == 12; },
                                     [](int const &&, auto &&...) -> fn::optional<bool> { throw 0; })) //
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
      CHECK(std::move(std::as_const(s))
                .transform( //
                    fn::overload([](int &, auto &&...) -> bool { throw 0; },
                                 [](int const &, auto &&...) -> bool { throw 0; },
                                 [](int &&, auto &&...) -> bool { throw 0; },
                                 [](int const &&i, auto &&...) -> bool { return i == 12; })) //
                .value());
      CHECK(std::move(s)
                .transform( //
                    fn::overload([](int &, auto &&...) -> bool { throw 0; },
                                 [](int const &, auto &&...) -> bool { throw 0; },
                                 [](int &&i, auto &&...) -> bool { return i == 12; },
                                 [](int const &&, auto &&...) -> bool { throw 0; })) //
                .value());
    }

    WHEN("error")
    {
      fn::optional<fn::pack<int, std::string_view>> s{std::nullopt};
      CHECK(not s.transform([](auto...) -> bool { throw 0; }).has_value());
      CHECK(not std::as_const(s).transform([](auto...) -> bool { throw 0; }).has_value());
      CHECK(not std::move(std::as_const(s)).transform([](auto...) -> bool { throw 0; }).has_value());
      CHECK(not std::move(s).transform([](auto...) -> bool { throw 0; }).has_value());
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

TEST_CASE("optional and_then sum", "[optional][sum][and_then]")
{
  WHEN("value")
  {
    fn::optional<fn::sum<Xint, int>> s{12};
    CHECK(s.and_then( //
               fn::overload(
                   [](int &i) -> fn::optional<bool> { return i == 12; },
                   [](int const &) -> fn::optional<bool> { throw 0; }, [](int &&) -> fn::optional<bool> { throw 0; },
                   [](int const &&) -> fn::optional<bool> { throw 0; }, [](Xint &) -> fn::optional<bool> { throw 0; },
                   [](Xint const &) -> fn::optional<bool> { throw 0; }, [](Xint &&) -> fn::optional<bool> { throw 0; },
                   [](Xint const &&) -> fn::optional<bool> { throw 0; }))
              .value());

    CHECK(std::as_const(s)
              .and_then( //
                  fn::overload([](int &) -> fn::optional<bool> { throw 0; },
                               [](int const &i) -> fn::optional<bool> { return i == 12; },
                               [](int &&) -> fn::optional<bool> { throw 0; },
                               [](int const &&) -> fn::optional<bool> { throw 0; },
                               [](Xint &) -> fn::optional<bool> { throw 0; },
                               [](Xint const &) -> fn::optional<bool> { throw 0; },
                               [](Xint &&) -> fn::optional<bool> { throw 0; },
                               [](Xint const &&) -> fn::optional<bool> { throw 0; })) //
              .value());

    CHECK(std::move(std::as_const(s))
              .and_then( //
                  fn::overload([](int &) -> fn::optional<bool> { throw 0; },
                               [](int const &) -> fn::optional<bool> { throw 0; },
                               [](int &&) -> fn::optional<bool> { throw 0; },
                               [](int const &&i) -> fn::optional<bool> { return i == 12; },
                               [](Xint &) -> fn::optional<bool> { throw 0; },
                               [](Xint const &) -> fn::optional<bool> { throw 0; },
                               [](Xint &&) -> fn::optional<bool> { throw 0; },
                               [](Xint const &&) -> fn::optional<bool> { throw 0; })) //
              .value());

    CHECK(
        std::move(s)
            .and_then( //
                fn::overload(
                    [](int &) -> fn::optional<bool> { throw 0; }, [](int const &) -> fn::optional<bool> { throw 0; },
                    [](int &&i) -> fn::optional<bool> { return i == 12; },
                    [](int const &&) -> fn::optional<bool> { throw 0; }, [](Xint &) -> fn::optional<bool> { throw 0; },
                    [](Xint const &) -> fn::optional<bool> { throw 0; }, [](Xint &&) -> fn::optional<bool> { throw 0; },
                    [](Xint const &&) -> fn::optional<bool> { throw 0; })) //
            .value());
  }

  WHEN("error")
  {
    fn::optional<fn::sum<Xint, int>> s{};
    CHECK(not s.and_then( //
                   [](auto) -> fn::optional<bool> { throw 0; })
                  .has_value());
    CHECK(not std::as_const(s)
                  .and_then( //
                      [](auto) -> fn::optional<bool> { throw 0; })
                  .has_value());
    CHECK(not std::move(std::as_const(s))
                  .and_then( //
                      [](auto) -> fn::optional<bool> { throw 0; })
                  .has_value());
    CHECK(not std::move(s)
                  .and_then( //
                      [](auto) -> fn::optional<bool> { throw 0; })
                  .has_value());
  }

  WHEN("constexpr")
  {
    constexpr auto fn = fn::overload(
        [](int &) -> fn::optional<bool> { throw 0; }, [](int const &i) -> fn::optional<bool> { return i == 42; },
        [](int &&) -> fn::optional<bool> { throw 0; }, [](int const &&) -> fn::optional<bool> { throw 0; },
        [](std::string_view &) -> fn::optional<bool> { throw 0; },
        [](std::string_view const &) -> fn::optional<bool> { throw 0; },
        [](std::string_view &&) -> fn::optional<bool> { throw 0; },
        [](std::string_view const &&) -> fn::optional<bool> { throw 0; });
    constexpr fn::optional<fn::sum<int, std::string_view>> a{fn::sum{42}};
    static_assert(std::is_same_v<decltype(a.and_then(fn)), fn::optional<bool>>);
    static_assert(a.and_then(fn).value());
  }
}

TEST_CASE("optional transform sum", "[optional][sum][transform]")
{
  WHEN("value")
  {
    fn::optional<fn::sum<Xint, int>> s{12};
    CHECK(s.transform( //
               fn::overload([](int &i) -> bool { return i == 12; }, [](int const &) -> bool { throw 0; },
                            [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; },
                            [](Xint &) -> bool { throw 0; }, [](Xint const &) -> bool { throw 0; },
                            [](Xint &&) -> bool { throw 0; }, [](Xint const &&) -> bool { throw 0; }))
              .value()
          == fn::sum{true});

    CHECK(std::as_const(s)
              .transform( //
                  fn::overload([](int &) -> bool { throw 0; }, [](int const &i) -> bool { return i == 12; },
                               [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; },
                               [](Xint &) -> bool { throw 0; }, [](Xint const &) -> bool { throw 0; },
                               [](Xint &&) -> bool { throw 0; }, [](Xint const &&) -> bool { throw 0; })) //
              .value()
          == fn::sum{true});

    CHECK(std::move(std::as_const(s))
              .transform( //
                  fn::overload([](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                               [](int &&) -> bool { throw 0; }, [](int const &&i) -> bool { return i == 12; },
                               [](Xint &) -> bool { throw 0; }, [](Xint const &) -> bool { throw 0; },
                               [](Xint &&) -> bool { throw 0; }, [](Xint const &&) -> bool { throw 0; })) //
              .value()
          == fn::sum{true});

    CHECK(std::move(s)
              .transform( //
                  fn::overload([](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                               [](int &&i) -> bool { return i == 12; }, [](int const &&) -> bool { throw 0; },
                               [](Xint &) -> bool { throw 0; }, [](Xint const &) -> bool { throw 0; },
                               [](Xint &&) -> bool { throw 0; }, [](Xint const &&) -> bool { throw 0; })) //
              .value()
          == fn::sum{true});
  }

  WHEN("error")
  {
    fn::optional<fn::sum<Xint, int>> s{};
    CHECK(not s.transform( //
                   [](auto) -> bool { throw 0; })
                  .has_value());
    CHECK(not std::as_const(s)
                  .transform( //
                      [](auto) -> bool { throw 0; })
                  .has_value());
    CHECK(not fn::optional<fn::sum<Xint, int>>{}
                  .transform( //
                      [](auto) -> bool { throw 0; })
                  .has_value());
    CHECK(not std::move(std::as_const(s))
                  .transform( //
                      [](auto) -> bool { throw 0; })
                  .has_value());
  }

  WHEN("constexpr")
  {
    constexpr auto fn
        = fn::overload([](int &) -> bool { throw 0; }, [](int const &i) -> bool { return i == 42; },
                       [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; },
                       [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                       [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; });
    constexpr fn::optional<fn::sum<int, std::string_view>> a{fn::sum{42}};
    static_assert(std::is_same_v<decltype(a.transform(fn)), fn::optional<fn::sum<bool, int>>>);
    static_assert(a.transform(fn).value() == fn::sum{true});
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
    CHECK(std::move(std::as_const(s))
              .and_then( //
                  fn::overload([](int &) -> fn::optional<bool> { throw 0; },
                               [](int const &) -> fn::optional<bool> { throw 0; },
                               [](int &&) -> fn::optional<bool> { throw 0; },
                               [](int const &&i) -> fn::optional<bool> { return i == 12; })) //
              .value());
    CHECK(std::move(s)
              .and_then( //
                  fn::overload([](int &) -> fn::optional<bool> { throw 0; },
                               [](int const &) -> fn::optional<bool> { throw 0; },
                               [](int &&i) -> fn::optional<bool> { return i == 12; },
                               [](int const &&) -> fn::optional<bool> { throw 0; })) //
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
      CHECK(not std::move(std::as_const(s))
                    .and_then( //
                        [](auto) -> fn::optional<bool> { throw 0; })
                    .has_value());
      CHECK(not std::move(s)
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
    CHECK(std::move(std::as_const(s)).or_else([]() -> fn::optional<int> { throw 0; }).value());
    CHECK(std::move(s).or_else([]() -> fn::optional<int> { throw 0; }).value());

    WHEN("error")
    {
      fn::optional<int> s{};
      CHECK(s.or_else([]() -> fn::optional<int> { return 12; }).value() == 12);
      CHECK(std::as_const(s).or_else([]() -> fn::optional<int> { return 12; }).value() == 12);
      CHECK(std::move(std::as_const(s)).or_else([]() -> fn::optional<int> { return 12; }).value() == 12);
      CHECK(std::move(s).or_else([]() -> fn::optional<int> { return 12; }).value() == 12);
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
    CHECK(std::move(std::as_const(s))
              .transform( //
                  fn::overload([](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                               [](int &&) -> bool { throw 0; }, [](int const &&i) -> bool { return i == 12; })) //
              .value());
    CHECK(std::move(s)
              .transform( //
                  fn::overload([](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                               [](int &&i) -> bool { return i == 12; }, [](int const &&) -> bool { throw 0; })) //
              .value());

    WHEN("error")
    {
      fn::optional<int> s{};
      CHECK(not s.transform([](auto) -> bool { throw 0; }).has_value());
      CHECK(not std::as_const(s).transform([](auto) -> bool { throw 0; }).has_value());
      CHECK(not std::move(std::as_const(s)).transform([](auto) -> bool { throw 0; }).has_value());
      CHECK(not std::move(s).transform([](auto) -> bool { throw 0; }).has_value());
    }
  }
}
