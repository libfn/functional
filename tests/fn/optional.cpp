// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/optional.hpp>
#include <fn/utility.hpp>

#include <catch2/catch_all.hpp>

#include <type_traits>
#include <utility>
#include <vector>

namespace {
struct Xint {
  int v = {};

  constexpr bool operator==(Xint const &) const noexcept = default;
  constexpr explicit Xint(int i) : v(i) {}
  constexpr ~Xint() = default;
  constexpr Xint(Xint const &) = default;
  constexpr Xint &operator=(Xint const &) = default;
};

// Sums whose alternatives include a non-builtin (Xint/std::string_view/fn::pack — any
// class/struct/enum) have platform-specific order (see sum.cpp); pure-builtin sums keep sum<...>.
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

    static_assert(std::is_same_v<decltype(fn::sum_value(s)), T &>);
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

    static_assert(std::is_same_v<decltype(fn::sum_value(s)), fn::optional<fn::sum<int>>>);
  }

  WHEN("or_else")
  {
    fn::optional<fn::sum<int>> s{std::nullopt};

    constexpr auto fn1 = []() -> fn::optional<Xint> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn1)), fn::optional<fn::sum_for<Xint, int>>>);
    constexpr auto fn2 = []() -> fn::optional<int> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn2)), fn::optional<fn::sum<int>>>);
    constexpr auto fn3 = []() -> fn::optional<fn::sum<int>> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn3)), fn::optional<fn::sum<int>>>);
    constexpr auto fn4 = []() -> fn::optional<fn::sum<Xint>> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn4)), fn::optional<fn::sum_for<Xint, int>>>);
    constexpr auto fn5 = []() -> fn::optional<fn::sum_for<Xint, int>> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn5)), fn::optional<fn::sum_for<Xint, int>>>);
    constexpr auto fn6 = []() -> fn::optional<fn::sum_for<Xint, long>> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn6)), fn::optional<fn::sum_for<Xint, int, long>>>);
    constexpr auto fn7 = []() -> fn::optional<fn::sum_for<Xint, int, long>> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn7)), fn::optional<fn::sum_for<Xint, int, long>>>);
    constexpr auto fn8 = []() -> fn::optional<fn::sum_for<Xint, int, long>> { throw 0; };
    static_assert(std::is_same_v<decltype(s.or_else(fn8)), fn::optional<fn::sum_for<Xint, int, long>>>);

    WHEN("error to value")
    {
      constexpr auto fn = []() -> fn::optional<Xint> { return {Xint{12}}; };
      static_assert(std::is_same_v<decltype(s.or_else(fn)), fn::optional<fn::sum_for<Xint, int>>>);
      CHECK(s.or_else(fn).value() == fn::sum{Xint{12}});
      CHECK(std::as_const(s).or_else(fn).value() == fn::sum{Xint{12}});
      CHECK(std::move(std::as_const(s)).or_else(fn).value() == fn::sum{Xint{12}});
      CHECK(std::move(s).or_else(fn).value() == fn::sum{Xint{12}});
    }

    WHEN("error to error")
    {
      constexpr auto fn = []() -> fn::optional<Xint> { return {std::nullopt}; };
      static_assert(std::is_same_v<decltype(s.or_else(fn)), fn::optional<fn::sum_for<Xint, int>>>);
      CHECK(not s.or_else(fn).has_value());
      CHECK(not std::as_const(s).or_else(fn).has_value());
      CHECK(not std::move(std::as_const(s)).or_else(fn).has_value());
      CHECK(not std::move(s).or_else(fn).has_value());
    }

    WHEN("value")
    {
      fn::optional<fn::sum<int>> s{fn::sum{12}};
      constexpr auto fn = []() -> fn::optional<Xint> { throw 0; };
      static_assert(std::is_same_v<decltype(s.or_else(fn)), fn::optional<fn::sum_for<Xint, int>>>);
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
                 fn::overload{[](int &i, auto &&...) -> fn::optional<bool> { return i == 12; },
                              [](int const &, auto &&...) -> fn::optional<bool> { throw 0; },
                              [](int &&, auto &&...) -> fn::optional<bool> { throw 0; },
                              [](int const &&, auto &&...) -> fn::optional<bool> { return 0; }}) //
                .value());
      CHECK(std::as_const(s)
                .and_then( //
                    fn::overload{[](int &, auto &&...) -> fn::optional<bool> { throw 0; },
                                 [](int const &i, auto &&...) -> fn::optional<bool> { return i == 12; },
                                 [](int &&, auto &&...) -> fn::optional<bool> { throw 0; },
                                 [](int const &&, auto &&...) -> fn::optional<bool> { throw 0; }}) //
                .value());
      CHECK(std::move(std::as_const(s))
                .and_then( //
                    fn::overload{[](int &, auto &&...) -> fn::optional<bool> { throw 0; },
                                 [](int const &, auto &&...) -> fn::optional<bool> { throw 0; },
                                 [](int &&, auto &&...) -> fn::optional<bool> { throw 0; },
                                 [](int const &&i, auto &&...) -> fn::optional<bool> { return i == 12; }}) //
                .value());
      CHECK(std::move(s)
                .and_then( //
                    fn::overload{[](int &, auto &&...) -> fn::optional<bool> { throw 0; },
                                 [](int const &, auto &&...) -> fn::optional<bool> { throw 0; },
                                 [](int &&i, auto &&...) -> fn::optional<bool> { return i == 12; },
                                 [](int const &&, auto &&...) -> fn::optional<bool> { throw 0; }}) //
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
                        fn::overload{[](int &, auto &&...) -> fn::optional<bool> { throw 0; },
                                     [](int const &, auto &&...) -> fn::optional<bool> { throw 0; },
                                     [](int &&i, auto &&...) -> fn::optional<bool> { return i == 12; },
                                     [](int const &&, auto &&...) -> fn::optional<bool> { throw 0; }}) //
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
                 fn::overload{[](int &i, auto &&...) -> bool { return i == 12; },
                              [](int const &, auto &&...) -> bool { throw 0; },
                              [](int &&, auto &&...) -> bool { throw 0; },
                              [](int const &&, auto &&...) -> bool { throw 0; }}) //
                .value());
      CHECK(std::as_const(s)
                .transform( //
                    fn::overload{[](int &, auto &&...) -> bool { throw 0; },
                                 [](int const &i, auto &&...) -> bool { return i == 12; },
                                 [](int &&, auto &&...) -> bool { throw 0; },
                                 [](int const &&, auto &&...) -> bool { throw 0; }}) //
                .value());
      CHECK(std::move(std::as_const(s))
                .transform( //
                    fn::overload{[](int &, auto &&...) -> bool { throw 0; },
                                 [](int const &, auto &&...) -> bool { throw 0; },
                                 [](int &&, auto &&...) -> bool { throw 0; },
                                 [](int const &&i, auto &&...) -> bool { return i == 12; }}) //
                .value());
      CHECK(std::move(s)
                .transform( //
                    fn::overload{[](int &, auto &&...) -> bool { throw 0; },
                                 [](int const &, auto &&...) -> bool { throw 0; },
                                 [](int &&i, auto &&...) -> bool { return i == 12; },
                                 [](int const &&, auto &&...) -> bool { throw 0; }}) //
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

    WHEN("value & pack yield pack")
    {
      static_assert(std::same_as<decltype(std::declval<fn::optional<int>>()
                                          & std::declval<fn::optional<fn::pack<double, bool>>>()),
                                 fn::optional<fn::pack<int, double, bool>>>);

      CHECK((fn::optional<double>{0.5} //
             & fn::optional<fn::pack<bool, int>>{std::in_place, fn::pack{true, 12}})
                .transform([](double d, bool b, int i) constexpr -> bool { return d == 0.5 && b && i == 12; })
                .value());
      CHECK(not(fn::optional<double>{std::nullopt} //
                & fn::optional<fn::pack<bool, int>>{std::in_place, fn::pack{true, 12}})
                   .has_value());
      CHECK(not(fn::optional<double>{0.5} //
                & fn::optional<fn::pack<bool, int>>{std::nullopt})
                   .has_value());
      CHECK(not(fn::optional<double>{std::nullopt} //
                & fn::optional<fn::pack<bool, int>>{std::nullopt})
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

    WHEN("pack & pack yield pack")
    {
      static_assert(std::same_as<decltype(std::declval<fn::optional<fn::pack<double, bool>>>()
                                          & std::declval<fn::optional<fn::pack<bool, int>>>()),
                                 fn::optional<fn::pack<double, bool, bool, int>>>);

      CHECK((fn::optional<fn::pack<double, bool>>{std::in_place, fn::pack<double, bool>{0.5, true}} //
             & fn::optional<fn::pack<bool, int>>{std::in_place, fn::pack{true, 12}})
                .transform(
                    [](double d, bool b1, bool b2, int i) constexpr -> bool { return d == 0.5 && b1 && b2 && i == 12; })
                .value());
      CHECK(not(fn::optional<fn::pack<double, bool>>{std::nullopt} //
                & fn::optional<fn::pack<bool, int>>{std::in_place, fn::pack{true, 12}})
                   .has_value());
      CHECK(not(fn::optional<fn::pack<double, bool>>{std::in_place, fn::pack<double, bool>{0.5, true}} //
                & fn::optional<fn::pack<bool, int>>{std::nullopt})
                   .has_value());
      CHECK(not(fn::optional<fn::pack<double, bool>>{std::nullopt} //
                & fn::optional<fn::pack<bool, int>>{std::nullopt})
                   .has_value());
    }

    WHEN("sum on both sides")
    {
      using Lh = fn::optional<fn::sum<double, int>>;
      using Rh = fn::optional<fn::sum<bool, int>>;
      static_assert(
          std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                       fn::optional<fn::sum< //
                           fn::pack<double, bool>, fn::pack<double, int>, fn::pack<int, bool>, fn::pack<int, int>>>>);

      CHECK((Lh{fn::sum{0.5}} & Rh{fn::sum{12}})
                .transform([](auto i, auto j) constexpr -> bool {
                  return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                })
                .value()
            == fn::sum{true});
      CHECK(not(Lh{std::nullopt} & Rh{fn::sum{12}}).has_value());
      CHECK(not(Lh{fn::sum{0.5}} & Rh{std::nullopt}).has_value());
      CHECK(not(Lh{std::nullopt} & Rh{std::nullopt}).has_value());

      WHEN("sum of packs on left")
      {
        using Lh = fn::optional<fn::sum_for<fn::pack<double, bool>, fn::pack<double, int>>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::optional<fn::sum< //
                                       fn::pack<double, bool, bool>, fn::pack<double, bool, int>,
                                       fn::pack<double, int, bool>, fn::pack<double, int, int>>>>);

        CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{fn::sum{12}})
                  .transform([](auto i, auto j, auto k) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                  })
                  .value()
              == fn::sum{true});
        CHECK(not(Lh{std::nullopt} & Rh{fn::sum{12}}).has_value());
        CHECK(not(Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{std::nullopt}).has_value());
        CHECK(not(Lh{std::nullopt} & Rh{std::nullopt}).has_value());
      }
    }

    WHEN("sum on left side only")
    {
      using Lh = fn::optional<fn::sum<double, int>>;
      using Rh = fn::optional<int>;
      static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                 fn::optional<fn::sum< //
                                     fn::pack<double, int>, fn::pack<int, int>>>>);

      CHECK((Lh{fn::sum{0.5}} & Rh{12})
                .transform([](auto i, auto j) constexpr -> bool {
                  return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                })
                .value()
            == fn::sum{true});
      CHECK(not(Lh{std::nullopt} & Rh{12}).has_value());
      CHECK(not(Lh{fn::sum{0.5}} & Rh{std::nullopt}).has_value());
      CHECK(not(Lh{std::nullopt} & Rh{std::nullopt}).has_value());

      WHEN("sum of packs on left")
      {
        using Lh = fn::optional<fn::sum_for<fn::pack<double, bool>, fn::pack<double, int>>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::optional<fn::sum< //
                                       fn::pack<double, bool, int>, fn::pack<double, int, int>>>>);

        CHECK((Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{12})
                  .transform([](auto i, auto j, auto k) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                  })
                  .value()
              == fn::sum{true});
        CHECK(not(Lh{std::nullopt} & Rh{12}).has_value());
        CHECK(not(Lh{fn::sum{fn::pack{0.5, 3}}} & Rh{std::nullopt}).has_value());
        CHECK(not(Lh{std::nullopt} & Rh{std::nullopt}).has_value());
      }
    }

    WHEN("sum on right side only")
    {
      using Lh = fn::optional<double>;
      using Rh = fn::optional<fn::sum<bool, int>>;
      static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                 fn::optional<fn::sum< //
                                     fn::pack<double, bool>, fn::pack<double, int>>>>);

      CHECK((Lh{0.5} & Rh{fn::sum{12}})
                .transform([](auto i, auto j) constexpr -> bool {
                  return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                })
                .value()
            == fn::sum{true});
      CHECK(not(Lh{std::nullopt} & Rh{fn::sum{12}}).has_value());
      CHECK(not(Lh{0.5} & Rh{std::nullopt}).has_value());
      CHECK(not(Lh{std::nullopt} & Rh{std::nullopt}).has_value());

      WHEN("pack on left")
      {
        using Lh = fn::optional<fn::pack<double, int>>;
        static_assert(std::same_as<decltype(std::declval<Lh>() & std::declval<Rh>()),
                                   fn::optional<fn::sum< //
                                       fn::pack<double, int, bool>, fn::pack<double, int, int>>>>);

        CHECK((Lh{fn::pack{0.5, 3}} & Rh{fn::sum{12}})
                  .transform([](auto i, auto j, auto k) constexpr -> bool {
                    return 0.5 == static_cast<double>(i) && 3 == static_cast<int>(j) && 12 == static_cast<int>(k);
                  })
                  .value()
              == fn::sum{true});
        CHECK(not(Lh{std::nullopt} & Rh{fn::sum{12}}).has_value());
        CHECK(not(Lh{fn::pack{0.5, 3}} & Rh{std::nullopt}).has_value());
        CHECK(not(Lh{std::nullopt} & Rh{std::nullopt}).has_value());
      }
    }
  }
}

TEST_CASE("optional and_then sum", "[optional][sum][and_then]")
{
  WHEN("value")
  {
    fn::optional<fn::sum_for<Xint, int>> s{12};
    CHECK(s.and_then( //
               fn::overload{
                   [](int &i) -> fn::optional<bool> { return i == 12; },
                   [](int const &) -> fn::optional<bool> { throw 0; }, [](int &&) -> fn::optional<bool> { throw 0; },
                   [](int const &&) -> fn::optional<bool> { throw 0; }, [](Xint &) -> fn::optional<bool> { throw 0; },
                   [](Xint const &) -> fn::optional<bool> { throw 0; }, [](Xint &&) -> fn::optional<bool> { throw 0; },
                   [](Xint const &&) -> fn::optional<bool> { throw 0; }})
              .value());

    CHECK(std::as_const(s)
              .and_then( //
                  fn::overload{[](int &) -> fn::optional<bool> { throw 0; },
                               [](int const &i) -> fn::optional<bool> { return i == 12; },
                               [](int &&) -> fn::optional<bool> { throw 0; },
                               [](int const &&) -> fn::optional<bool> { throw 0; },
                               [](Xint &) -> fn::optional<bool> { throw 0; },
                               [](Xint const &) -> fn::optional<bool> { throw 0; },
                               [](Xint &&) -> fn::optional<bool> { throw 0; },
                               [](Xint const &&) -> fn::optional<bool> { throw 0; }}) //
              .value());

    CHECK(std::move(std::as_const(s))
              .and_then( //
                  fn::overload{[](int &) -> fn::optional<bool> { throw 0; },
                               [](int const &) -> fn::optional<bool> { throw 0; },
                               [](int &&) -> fn::optional<bool> { throw 0; },
                               [](int const &&i) -> fn::optional<bool> { return i == 12; },
                               [](Xint &) -> fn::optional<bool> { throw 0; },
                               [](Xint const &) -> fn::optional<bool> { throw 0; },
                               [](Xint &&) -> fn::optional<bool> { throw 0; },
                               [](Xint const &&) -> fn::optional<bool> { throw 0; }}) //
              .value());

    CHECK(
        std::move(s)
            .and_then( //
                fn::overload{
                    [](int &) -> fn::optional<bool> { throw 0; }, [](int const &) -> fn::optional<bool> { throw 0; },
                    [](int &&i) -> fn::optional<bool> { return i == 12; },
                    [](int const &&) -> fn::optional<bool> { throw 0; }, [](Xint &) -> fn::optional<bool> { throw 0; },
                    [](Xint const &) -> fn::optional<bool> { throw 0; }, [](Xint &&) -> fn::optional<bool> { throw 0; },
                    [](Xint const &&) -> fn::optional<bool> { throw 0; }}) //
            .value());
  }

  WHEN("error")
  {
    fn::optional<fn::sum_for<Xint, int>> s{};
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
    constexpr auto fn = fn::overload{[](int &) -> fn::optional<bool> { throw 0; },
                                     [](int const &i) -> fn::optional<bool> { return i == 42; },
                                     [](int &&) -> fn::optional<bool> { throw 0; },
                                     [](int const &&) -> fn::optional<bool> { throw 0; },
                                     [](std::string_view &) -> fn::optional<bool> { throw 0; },
                                     [](std::string_view const &) -> fn::optional<bool> { throw 0; },
                                     [](std::string_view &&) -> fn::optional<bool> { throw 0; },
                                     [](std::string_view const &&) -> fn::optional<bool> { throw 0; }};
    constexpr fn::optional<fn::sum_for<int, std::string_view>> a{fn::sum{42}};
    static_assert(std::is_same_v<decltype(a.and_then(fn)), fn::optional<bool>>);
    static_assert(a.and_then(fn).value());
  }
}

TEST_CASE("optional transform sum", "[optional][sum][transform]")
{
  WHEN("value")
  {
    fn::optional<fn::sum_for<Xint, int>> s{12};
    CHECK(s.transform( //
               fn::overload{[](int &i) -> bool { return i == 12; }, [](int const &) -> bool { throw 0; },
                            [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; },
                            [](Xint &) -> bool { throw 0; }, [](Xint const &) -> bool { throw 0; },
                            [](Xint &&) -> bool { throw 0; }, [](Xint const &&) -> bool { throw 0; }})
              .value()
          == fn::sum{true});

    CHECK(std::as_const(s)
              .transform( //
                  fn::overload{[](int &) -> bool { throw 0; }, [](int const &i) -> bool { return i == 12; },
                               [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; },
                               [](Xint &) -> bool { throw 0; }, [](Xint const &) -> bool { throw 0; },
                               [](Xint &&) -> bool { throw 0; }, [](Xint const &&) -> bool { throw 0; }}) //
              .value()
          == fn::sum{true});

    CHECK(std::move(std::as_const(s))
              .transform( //
                  fn::overload{[](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                               [](int &&) -> bool { throw 0; }, [](int const &&i) -> bool { return i == 12; },
                               [](Xint &) -> bool { throw 0; }, [](Xint const &) -> bool { throw 0; },
                               [](Xint &&) -> bool { throw 0; }, [](Xint const &&) -> bool { throw 0; }}) //
              .value()
          == fn::sum{true});

    CHECK(std::move(s)
              .transform( //
                  fn::overload{[](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                               [](int &&i) -> bool { return i == 12; }, [](int const &&) -> bool { throw 0; },
                               [](Xint &) -> bool { throw 0; }, [](Xint const &) -> bool { throw 0; },
                               [](Xint &&) -> bool { throw 0; }, [](Xint const &&) -> bool { throw 0; }}) //
              .value()
          == fn::sum{true});
  }

  WHEN("error")
  {
    fn::optional<fn::sum_for<Xint, int>> s{};
    CHECK(not s.transform( //
                   [](auto) -> bool { throw 0; })
                  .has_value());
    CHECK(not std::as_const(s)
                  .transform( //
                      [](auto) -> bool { throw 0; })
                  .has_value());
    CHECK(not fn::optional<fn::sum_for<Xint, int>>{}
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
    constexpr auto fn = fn::overload{[](int &) -> bool { throw 0; },
                                     [](int const &i) -> bool { return i == 42; },
                                     [](int &&) -> bool { throw 0; },
                                     [](int const &&) -> bool { throw 0; },
                                     [](std::string_view &) -> int { throw 0; },
                                     [](std::string_view const &) -> int { throw 0; },
                                     [](std::string_view &&) -> int { throw 0; },
                                     [](std::string_view const &&) -> int { throw 0; }};
    constexpr fn::optional<fn::sum_for<int, std::string_view>> a{fn::sum{42}};
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
               fn::overload{[](int &i) -> fn::optional<bool> { return i == 12; },
                            [](int const &) -> fn::optional<bool> { throw 0; },
                            [](int &&) -> fn::optional<bool> { throw 0; },
                            [](int const &&) -> fn::optional<bool> { throw 0; }}) //
              .value());
    CHECK(std::as_const(s)
              .and_then( //
                  fn::overload{[](int &) -> fn::optional<bool> { throw 0; },
                               [](int const &i) -> fn::optional<bool> { return i == 12; },
                               [](int &&) -> fn::optional<bool> { throw 0; },
                               [](int const &&) -> fn::optional<bool> { throw 0; }}) //
              .value());
    CHECK(std::move(std::as_const(s))
              .and_then( //
                  fn::overload{[](int &) -> fn::optional<bool> { throw 0; },
                               [](int const &) -> fn::optional<bool> { throw 0; },
                               [](int &&) -> fn::optional<bool> { throw 0; },
                               [](int const &&i) -> fn::optional<bool> { return i == 12; }}) //
              .value());
    CHECK(std::move(s)
              .and_then( //
                  fn::overload{[](int &) -> fn::optional<bool> { throw 0; },
                               [](int const &) -> fn::optional<bool> { throw 0; },
                               [](int &&i) -> fn::optional<bool> { return i == 12; },
                               [](int const &&) -> fn::optional<bool> { throw 0; }}) //
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
               fn::overload{[](int &i) -> bool { return i == 12; }, [](int const &) -> bool { throw 0; },
                            [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; }}) //
              .value());
    CHECK(std::as_const(s)
              .transform( //
                  fn::overload{[](int &) -> bool { throw 0; }, [](int const &i) -> bool { return i == 12; },
                               [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; }}) //
              .value());
    CHECK(std::move(std::as_const(s))
              .transform( //
                  fn::overload{[](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                               [](int &&) -> bool { throw 0; }, [](int const &&i) -> bool { return i == 12; }}) //
              .value());
    CHECK(std::move(s)
              .transform( //
                  fn::overload{[](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                               [](int &&i) -> bool { return i == 12; }, [](int const &&) -> bool { throw 0; }}) //
              .value());

    WHEN("transform direct-initializes its result")
    {
      // the value is direct-non-list-initialized from the callable's result: no extra
      // move, and an immovable type works
      struct immovable_t {
        int v;
        constexpr explicit immovable_t(int i) noexcept : v(i) {}
        immovable_t(immovable_t &&) = delete;
      };
      auto a = s.transform([](int i) -> immovable_t { return immovable_t(i + 1); });
      static_assert(std::is_same_v<decltype(a), fn::optional<immovable_t>>);
      CHECK(a.value().v == 13);

      // the from-invoke tag ctor backing this is not part of the public interface
      // (is_constructible_v cannot see private ctors)
      static_assert(not std::is_constructible_v<fn::optional<immovable_t>, pfn::detail::_optional_from_invoke_t,
                                                immovable_t (*)()>);
    }

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

TEST_CASE("optional constructors and assignment", "[optional][constructors][assignment]")
{
  WHEN("constructors")
  {
    fn::optional<short> const c{static_cast<short>(3)};
    fn::optional<int> const x{c};
    CHECK(x.value() == 3);
    fn::optional<int> const y{fn::optional<short>{static_cast<short>(5)}};
    CHECK(y.value() == 5);
    fn::optional<int> const e{fn::optional<short>{}};
    CHECK(not e.has_value());

    fn::optional<std::vector<int>> v{std::in_place, {1, 2, 3}};
    auto v2 = v;
    CHECK(v2.value() == std::vector<int>{1, 2, 3});
    auto v3 = std::move(v);
    CHECK(v3.value().size() == 3);
  }

  WHEN("assignment")
  {
    fn::optional<int> a{};
    a = 12;
    CHECK(a.value() == 12);
    a = std::nullopt;
    CHECK(not a.has_value());

    fn::optional<int> const b{42};
    a = b;
    CHECK(a.value() == 42);
    a = fn::optional<int>{7};
    CHECK(a.value() == 7);

    fn::optional<short> const c{static_cast<short>(3)};
    a = c;
    CHECK(a.value() == 3);
    a = fn::optional<short>{static_cast<short>(5)};
    CHECK(a.value() == 5);
  }

  WHEN("emplace and reset")
  {
    fn::optional<int> a{};
    CHECK(a.emplace(11) == 11);
    CHECK(a.value() == 11);
    a.reset();
    CHECK(not a.has_value());
  }

  WHEN("swap")
  {
    fn::optional<int> x{1};
    fn::optional<int> y{};
    x.swap(y);
    CHECK(not x.has_value());
    CHECK(y.value() == 1);
  }
}

TEST_CASE("optional comparison operators", "[optional][compare]")
{
  WHEN("optional to optional")
  {
    fn::optional<int> const a{12};
    fn::optional<double> const b{12.0};
    fn::optional<int> const e{};

    CHECK(a == b);
    CHECK(a != fn::optional<int>{13});
    CHECK(e == fn::optional<double>{});
    CHECK(a != e);
    CHECK(e < a);
    CHECK(a <= b);
    CHECK(a > e);
    CHECK(fn::optional<int>{13} >= b);
    static_assert(fn::optional<int>{1} == fn::optional<int>{1});

    // Xint is equality-comparable but not three-way-comparable: only the `==` family applies
    CHECK(fn::optional<Xint>{Xint{5}} == fn::optional<Xint>{Xint{5}});
  }

  WHEN("optional to nullopt")
  {
    fn::optional<int> const a{12};
    fn::optional<int> const e{};

    CHECK(e == std::nullopt);
    CHECK(std::nullopt == e);
    CHECK(a != std::nullopt);
    CHECK(std::nullopt < a);
    CHECK(e >= std::nullopt);
  }

  WHEN("optional to value")
  {
    fn::optional<int> const a{12};
    fn::optional<int> const e{};

    CHECK(a == 12);
    CHECK(12 == a);
    CHECK(a != 13);
    CHECK(e != 12);
    CHECK(a < 13.0);
    CHECK(13 > a);
    CHECK(e < 12);
    CHECK(fn::optional<Xint>{Xint{5}} == Xint{5});
  }
}
