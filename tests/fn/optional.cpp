// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/optional.hpp>
#include <fn/utility.hpp>

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

// Nothrow move, throwing copy: joining lvalue operands copies the value, joining rvalues moves it
struct MoveNothrow {
  MoveNothrow() = default;
  MoveNothrow(MoveNothrow const &) noexcept(false) {}
  MoveNothrow(MoveNothrow &&) noexcept = default;
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

    WHEN("constexpr")
    {
      static_assert([] {
        fn::optional<int> const a{12};
        return a.sum_value().value() == fn::sum{12};
      }());
      static_assert([] { return fn::optional<int>{12}.sum_value().value() == fn::sum{12}; }());
      static_assert([] { return not fn::optional<int>{std::nullopt}.sum_value().has_value(); }());
      static_assert([] {
        fn::optional<int> a{12};
        return fn::sum_value(a).value() == fn::sum{12};
      }());
      SUCCEED();
    }
  }

  WHEN("noexcept")
  {
    using T = fn::optional<int>;
    using S = fn::optional<fn::sum<int>>;

    // the sum-valued overloads only return *this
    static_assert(noexcept(std::declval<S &>().sum_value()));
    static_assert(noexcept(std::declval<S const &>().sum_value()));
    static_assert(noexcept(std::declval<S &&>().sum_value()));
    static_assert(noexcept(std::declval<S const &&>().sum_value()));
    static_assert(noexcept(fn::sum_value(std::declval<S &>())));

    // the lifting overloads wrap the value in a sum, so they weigh that construction
    static_assert(noexcept(std::declval<T &>().sum_value()));
    static_assert(noexcept(std::declval<T &&>().sum_value()));
    static_assert(noexcept(fn::sum_value(std::declval<T &>())));

    // ... and report it when the value's copy can throw
    struct throwing_copy {
      throwing_copy() = default;
      throwing_copy(throwing_copy const &) noexcept(false);
      throwing_copy(throwing_copy &&) noexcept;
    };
    using W = fn::optional<throwing_copy>;
    static_assert(not noexcept(std::declval<W const &>().sum_value())); // copies
    static_assert(noexcept(std::declval<W &&>().sum_value()));          // moves
    SUCCEED();
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

    WHEN("noexcept")
    {
      // the widening arm builds a sum from either side's value, and weighs both
      using T = fn::optional<fn::sum<int>>;
      constexpr auto widen = []() noexcept -> fn::optional<double> { return {0.5}; };
      static_assert(std::is_same_v<decltype(std::declval<T &>().or_else(widen)), fn::optional<fn::sum<double, int>>>);
      static_assert(noexcept(std::declval<T &>().or_else(widen)));
      static_assert(not noexcept(std::declval<T &>().or_else([]() -> fn::optional<double> { return {0.5}; })));

      // ... including relocating self's value into it
      using W = fn::optional<fn::sum_for<MoveNothrow, int>>;
      static_assert(not noexcept(std::declval<W &>().or_else(widen))); // copies
      static_assert(noexcept(std::declval<W &&>().or_else(widen)));    // moves

      // the same-shape arm, which pfn's own or_else would take
      static_assert(noexcept(std::declval<T &>().or_else([]() noexcept -> T { return {fn::sum{1}}; })));
      SUCCEED();
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

    WHEN("noexcept")
    {
      // a pack's callback is invoked through fn's own dispatch, taking one argument per element: it
      // is not directly invocable on the pack, so only fn's nothrow-invocable trait can answer
      using T = fn::optional<fn::pack<int, double>>;
      static_assert(
          noexcept(std::declval<T &>().and_then([](int, double) noexcept -> fn::optional<bool> { return {true}; })));
      static_assert(
          not noexcept(std::declval<T &>().and_then([](int, double) -> fn::optional<bool> { return {true}; })));
      SUCCEED();
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

    WHEN("noexcept")
    {
      using T = fn::optional<fn::pack<int, double>>;
      static_assert(noexcept(std::declval<T &>().transform([](int, double) noexcept -> bool { return true; })));
      static_assert(not noexcept(std::declval<T &>().transform([](int, double) -> bool { return true; })));
      SUCCEED();
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

    WHEN("noexcept")
    {
      // the join relocates both operands' values into the result, so it promises only what
      // relocating them promises - the nullopt arm is a tag, and cannot throw
      using Lh = fn::optional<MoveNothrow>;
      using Rh = fn::optional<int>;
      static_assert(noexcept(std::declval<Rh &>() & std::declval<Rh &>()));
      static_assert(not noexcept(std::declval<Lh &>() & std::declval<Rh &>())); // copies
      static_assert(not noexcept(std::declval<Rh &>() & std::declval<Lh &>()));
      static_assert(noexcept(std::declval<Lh &&>() & std::declval<Rh &&>())); // moves
      static_assert(noexcept(std::declval<Rh &&>() & std::declval<Lh &&>()));

      // the same, dispatched through a sum
      using Sh = fn::optional<fn::sum<MoveNothrow>>;
      static_assert(not noexcept(std::declval<Sh &>() & std::declval<Rh &>()));
      static_assert(noexcept(std::declval<Sh &&>() & std::declval<Rh &&>()));
      SUCCEED();
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

    WHEN("immovable result type")
    {
      // the disengaged path must compile even though the result cannot be moved (the
      // clang<=18 miscompile workaround must not force a move)
      struct immovable_t {
        int v;
        constexpr explicit immovable_t(int i) noexcept : v(i) {}
        immovable_t(immovable_t &&) = delete;
      };
      auto r = s.and_then([](auto) -> fn::optional<immovable_t> { throw 0; });
      static_assert(std::is_same_v<decltype(r), fn::optional<immovable_t>>);
      CHECK(not r.has_value());
    }
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

  WHEN("noexcept")
  {
    // the callback is invoked through fn's own dispatch, so only fn's nothrow-invocable trait can
    // answer for it - and it is nothrow only if EVERY alternative's call is
    using T = fn::optional<fn::sum<double, int>>;
    static_assert(noexcept(std::declval<T &>().and_then([](auto) noexcept -> fn::optional<bool> { return {true}; })));
    static_assert(not noexcept(std::declval<T &>().and_then([](auto) -> fn::optional<bool> { return {true}; })));
    static_assert(not noexcept(std::declval<T &>().and_then(
        fn::overload{[](int) noexcept -> fn::optional<bool> { return {true}; },
                     [](double) -> fn::optional<bool> { return {true}; }}))); // one throwing alternative is enough
    SUCCEED();
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

  WHEN("constraints")
  {
    using T = fn::optional<fn::sum_for<Xint, int>>;
    constexpr auto can_transform_lval = [](auto &&f) { return requires { std::declval<T &>().transform(f); }; };
    constexpr auto can_transform_clval = [](auto &&f) { return requires { std::declval<T const &>().transform(f); }; };

    // a callback no alternative can take drops the candidate, rather than failing inside the body
    static_assert(not can_transform_lval([](std::string_view) -> bool { throw 0; }));
    static_assert(not can_transform_lval([](int &) -> bool { throw 0; })); // Xint is unhandled

    // a visitor need only serve the value category the call actually selects: the losing const&
    // candidate is dropped by its own constraint rather than forming its signature and poisoning
    // the call. The four-category visitors above are a spelling choice, not a requirement.
    constexpr auto lval_only = fn::overload{[](int &i) -> bool { return i == 12; }, [](Xint &) -> bool { throw 0; }};
    static_assert(can_transform_lval(lval_only));
    static_assert(not can_transform_clval(lval_only));

    T s{12};
    CHECK(s.transform(lval_only).value() == fn::sum{true});
  }

  WHEN("noexcept")
  {
    // the dispatch is nothrow only if every alternative's call is, and only if relocating what each
    // returns into the result sum is
    using T = fn::optional<fn::sum<double, int>>;
    static_assert(noexcept(std::declval<T &>().transform([](auto) noexcept -> bool { return true; })));
    static_assert(not noexcept(std::declval<T &>().transform([](auto) -> bool { return true; })));
    static_assert(not noexcept(std::declval<T &>().transform(
        fn::overload{[](double) noexcept -> bool { return true; },
                     [](int) -> bool { return true; }}))); // one throwing alternative is enough
    static_assert(not noexcept(
        std::declval<fn::optional<fn::sum_for<MoveNothrow, int>> &>().transform([](auto v) noexcept { return v; })));
    SUCCEED();
  }
}
