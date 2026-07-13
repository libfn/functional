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

// Sums whose alternatives include a non-builtin (Xint/std::string_view/fn::pack — any
// class/struct/enum) have platform-specific order (see sum.cpp); pure-builtin sums keep sum<...>.
} // namespace

TEST_CASE("optional graded monad", "[optional][sum][graded][or_else][sum_value]")
{
  SECTION("sum_value from sum")
  {
    using T = fn::optional<fn::sum<int>>;
    T s{12};
    static_assert(std::is_same_v<decltype(s.sum_value()), T &>);
    static_assert(std::is_same_v<decltype(std::as_const(s).sum_value()), T const &>);
    static_assert(std::is_same_v<decltype(std::move(std::as_const(s)).sum_value()), T const &&>);
    static_assert(std::is_same_v<decltype(std::move(s).sum_value()), T &&>);
    // these overloads only return *this
    static_assert(noexcept(s.sum_value()));
    static_assert(noexcept(std::as_const(s).sum_value()));
    static_assert(noexcept(std::move(std::as_const(s)).sum_value()));
    static_assert(noexcept(std::move(s).sum_value()));
    SECTION("value")
    {
      CHECK(s.sum_value().value() == fn::sum{12});
      CHECK(std::as_const(s).sum_value().value() == fn::sum{12});
      CHECK(std::move(std::as_const(s)).sum_value().value() == fn::sum{12});
      CHECK(std::move(s).sum_value().value() == fn::sum{12});
    }
    SECTION("error")
    {
      T s{std::nullopt};
      CHECK(not s.sum_value().has_value());
      CHECK(not std::as_const(s).sum_value().has_value());
      CHECK(not std::move(std::as_const(s)).sum_value().has_value());
      CHECK(not std::move(s).sum_value().has_value());
    }

    static_assert(std::is_same_v<decltype(fn::sum_value(s)), T &>);
    static_assert(noexcept(fn::sum_value(s))); // the free function propagates what the member says
  }

  SECTION("sum_value from non-sum")
  {
    using T = fn::optional<int>;
    T s{12};
    static_assert(std::is_same_v<decltype(s.sum_value()), fn::optional<fn::sum<int>>>);
    static_assert(std::is_same_v<decltype(std::as_const(s).sum_value()), fn::optional<fn::sum<int>>>);
    static_assert(std::is_same_v<decltype(std::move(std::as_const(s)).sum_value()), fn::optional<fn::sum<int>>>);
    static_assert(std::is_same_v<decltype(std::move(s).sum_value()), fn::optional<fn::sum<int>>>);
    // GAP #280: these overloads wrap the value in a sum, so they weigh that construction - and sum's
    // own value constructor carries no noexcept specifier, so they are conservatively false even for
    // a nothrow value type. They sharpen when #280 lands.
    static_assert(not noexcept(s.sum_value()));
    static_assert(not noexcept(std::as_const(s).sum_value()));
    static_assert(not noexcept(std::move(std::as_const(s)).sum_value()));
    static_assert(not noexcept(std::move(s).sum_value()));
    SECTION("value")
    {
      CHECK(s.sum_value().value() == fn::sum{12});
      CHECK(std::as_const(s).sum_value().value() == fn::sum{12});
      CHECK(std::move(std::as_const(s)).sum_value().value() == fn::sum{12});
      CHECK(std::move(s).sum_value().value() == fn::sum{12});
    }
    SECTION("error")
    {
      T s{std::nullopt};
      CHECK(not s.sum_value().has_value());
      CHECK(not std::as_const(s).sum_value().has_value());
      CHECK(not std::move(std::as_const(s)).sum_value().has_value());
      CHECK(not std::move(s).sum_value().has_value());
    }

    static_assert(std::is_same_v<decltype(fn::sum_value(s)), fn::optional<fn::sum<int>>>);
    static_assert(not noexcept(fn::sum_value(s))); // the free function propagates what the member says

    SECTION("constexpr")
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

  SECTION("or_else")
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

    // noexcept (extension): true only when the callback is nothrow-invocable, returns exactly
    // optional<sum<int>> (no widening), and *this is nothrow-constructible from itself.
    constexpr auto nothrow_same = []() noexcept -> fn::optional<fn::sum<int>> { return {std::nullopt}; };
    static_assert(noexcept(s.or_else(nothrow_same)));
    static_assert(noexcept(std::move(s).or_else(nothrow_same)));
    static_assert(not noexcept(s.or_else(fn3))); // fn3 throws
    constexpr auto nothrow_widen = []() noexcept -> fn::optional<Xint> { return {std::nullopt}; };
    static_assert(not noexcept(s.or_else(nothrow_widen))); // nothrow but widens to sum_for<Xint, int>

    // constraints (extension): a non-invocable argument drops or_else from the overload set via
    // SFINAE (the return-type-is-optional requirement is instead a Mandates static_assert inside).
    constexpr auto can_or_else
        = [](auto &&f) { return requires { std::declval<fn::optional<fn::sum<int>>>().or_else(f); }; };
    static_assert(can_or_else(nothrow_same));
    static_assert(not can_or_else(42));

    SECTION("error to value")
    {
      constexpr auto fn = []() -> fn::optional<Xint> { return {Xint{12}}; };
      static_assert(std::is_same_v<decltype(s.or_else(fn)), fn::optional<fn::sum_for<Xint, int>>>);
      CHECK(s.or_else(fn).value() == fn::sum{Xint{12}});
      CHECK(std::as_const(s).or_else(fn).value() == fn::sum{Xint{12}});
      CHECK(std::move(std::as_const(s)).or_else(fn).value() == fn::sum{Xint{12}});
      CHECK(std::move(s).or_else(fn).value() == fn::sum{Xint{12}});
    }

    SECTION("error to error")
    {
      constexpr auto fn = []() -> fn::optional<Xint> { return {std::nullopt}; };
      static_assert(std::is_same_v<decltype(s.or_else(fn)), fn::optional<fn::sum_for<Xint, int>>>);
      CHECK(not s.or_else(fn).has_value());
      CHECK(not std::as_const(s).or_else(fn).has_value());
      CHECK(not std::move(std::as_const(s)).or_else(fn).has_value());
      CHECK(not std::move(s).or_else(fn).has_value());
    }

    SECTION("value")
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
  SECTION("and_then")
  {
    using P = fn::optional<fn::pack<int, std::string_view>>;

    // noexcept (extension): the spec is std::is_nothrow_invocable_v on the whole pack
    // (optional.hpp:146), never satisfied by a multi-argument visitor -- conservatively
    // noexcept(false) even with nothrow handlers; fn-native noexcept for sum/pack dispatch is
    // GH #254. A generic callback IS invocable on the whole pack, so the borrowed trait then
    // reports true from a call shape the pack-apply dispatch never makes.
    constexpr auto nothrow_two = [](int &, std::string_view &) noexcept -> fn::optional<bool> { return {true}; };
    static_assert(not noexcept(std::declval<P &>().and_then(nothrow_two)));
    constexpr auto nothrow_generic = [](auto &&...) noexcept -> fn::optional<bool> { return {true}; };
    static_assert(noexcept(std::declval<P &>().and_then(nothrow_generic)));

    // constraints (extension): pack-apply invocability is a constraint (optional.hpp:147);
    // wrong arity or a non-callable SFINAE-drops, and the pack's value category is tracked
    constexpr auto can_and_then_lval = [](auto &&f) { return requires { std::declval<P &>().and_then(f); }; };
    constexpr auto can_and_then_rval = [](auto &&f) { return requires { std::declval<P &&>().and_then(f); }; };
    static_assert(can_and_then_lval(nothrow_two));
    static_assert(not can_and_then_rval(nothrow_two));                                        // lvalue-only visitor
    static_assert(not can_and_then_lval([](int &) -> fn::optional<bool> { return {true}; })); // wrong arity
    static_assert(not can_and_then_lval(42));

    SECTION("value")
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

    SECTION("error")
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

    SECTION("constexpr")
    {
      constexpr P a{fn::pack<int>{12}.append(std::in_place_type<std::string_view>, "bar")};
      static_assert(a.and_then([](int const &i, std::string_view const &s) -> fn::optional<bool> {
                       return {i == 12 && s == "bar"};
                     }).value());
      static_assert(not P{std::nullopt}.and_then([](auto &&...) -> fn::optional<bool> { return {true}; }).has_value());
      SUCCEED();
    }
  }

  SECTION("transform")
  {
    using P = fn::optional<fn::pack<int, std::string_view>>;

    // noexcept and constraints mirror and_then above (optional.hpp:219-220): the non-sum
    // _transform is constrained on pack-apply invocability -- contrast the sum case, whose
    // callback is checked only in its deduced-return body (see "optional transform sum")
    constexpr auto nothrow_two = [](int &, std::string_view &) noexcept -> bool { return true; };
    static_assert(not noexcept(std::declval<P &>().transform(nothrow_two)));
    constexpr auto nothrow_generic = [](auto &&...) noexcept -> bool { return true; };
    static_assert(noexcept(std::declval<P &>().transform(nothrow_generic)));

    constexpr auto can_transform = [](auto &&f) { return requires { std::declval<P &>().transform(f); }; };
    static_assert(can_transform(nothrow_two));
    static_assert(not can_transform([](int &) -> bool { return true; })); // wrong arity
    static_assert(not can_transform(42));

    SECTION("value")
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

    SECTION("error")
    {
      fn::optional<fn::pack<int, std::string_view>> s{std::nullopt};
      CHECK(not s.transform([](auto...) -> bool { throw 0; }).has_value());
      CHECK(not std::as_const(s).transform([](auto...) -> bool { throw 0; }).has_value());
      CHECK(not std::move(std::as_const(s)).transform([](auto...) -> bool { throw 0; }).has_value());
      CHECK(not std::move(s).transform([](auto...) -> bool { throw 0; }).has_value());
    }

    SECTION("constexpr")
    {
      constexpr P a{fn::pack<int>{12}.append(std::in_place_type<std::string_view>, "bar")};
      static_assert(
          a.transform([](int const &i, std::string_view const &s) -> bool { return i == 12 && s == "bar"; }).value());
      static_assert(not P{std::nullopt}.transform([](auto &&...) -> bool { return true; }).has_value());
      SUCCEED();
    }
  }

  SECTION("operator &")
  {
    // noexcept: operator& is declared unconditionally noexcept (optional.hpp:881) though
    // joining copies/moves the operands' values into the result pack -- for a throwing-copy
    // value type this is a promise the join cannot keep (make_optional, by contrast, spells
    // conditional noexcept). GAP: asserts current behaviour; flip to `not noexcept` when
    // fixed (issue #279).
    struct throwing_copy {
      // defined, not just declared: the instantiated join references it (-Wundefined-internal)
      throwing_copy(throwing_copy const &) noexcept(false) {}
    };
    static_assert(noexcept(std::declval<fn::optional<double> &>() & std::declval<fn::optional<int> &>()));
    static_assert(noexcept(std::declval<fn::optional<throwing_copy> &>() & std::declval<fn::optional<int> &>()));

    // constraints: both operands must be optionals
    constexpr auto can_amp = [](auto &&rh) { return requires { std::declval<fn::optional<int> &>() & rh; }; };
    static_assert(can_amp(fn::optional<double>{0.5}));
    static_assert(not can_amp(42));

    SECTION("value & value yield pack")
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

    SECTION("value & pack yield pack")
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

    SECTION("pack & value yield pack")
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

    SECTION("pack & pack yield pack")
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

    SECTION("sum on both sides")
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

      SECTION("sum of packs on left")
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

    SECTION("sum on left side only")
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

      SECTION("sum of packs on left")
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

    SECTION("sum on right side only")
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

      SECTION("pack on left")
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

    SECTION("constexpr")
    {
      static_assert((fn::optional<double>{0.5} & fn::optional<int>{12})
                        .transform([](double d, int i) constexpr -> bool { return d == 0.5 && i == 12; })
                        .value());
      static_assert(not(fn::optional<double>{std::nullopt} & fn::optional<int>{12}).has_value());
      using Lh = fn::optional<fn::sum<double, int>>;
      using Rh = fn::optional<fn::sum<bool, int>>;
      static_assert((Lh{fn::sum{0.5}} & Rh{fn::sum{12}})
                        .transform([](auto i, auto j) constexpr -> bool {
                          return 0.5 == static_cast<double>(i) && 12 == static_cast<int>(j);
                        })
                        .value()
                    == fn::sum{true});
      SUCCEED();
    }
  }
}

TEST_CASE("optional and_then sum", "[optional][sum][and_then]")
{
  using S = fn::optional<fn::sum_for<Xint, int>>;

  // noexcept (extension): the spec is std::is_nothrow_invocable_v on the whole sum
  // (optional.hpp:146); a visitor set is not invocable on the sum itself, so and_then over a
  // sum is conservatively noexcept(false) even with nothrow handlers -- fn-native noexcept for
  // sum dispatch is GH #254. A generic callback IS invocable on the whole sum, so the borrowed
  // trait then reports true from a call shape the per-alternative dispatch never makes.
  constexpr auto nothrow_lval = fn::overload{[](int &) noexcept -> fn::optional<bool> { return {true}; },
                                             [](Xint &) noexcept -> fn::optional<bool> { return {false}; }};
  static_assert(not noexcept(std::declval<S &>().and_then(nothrow_lval)));
  constexpr auto nothrow_generic = [](auto &&) noexcept -> fn::optional<bool> { return {true}; };
  static_assert(noexcept(std::declval<S &>().and_then(nothrow_generic)));

  // constraints (extension): exhaustive invocability over the sum's alternatives is a
  // constraint (optional.hpp:147) -- a partial or non-invocable visitor SFINAE-drops, and the
  // alternatives' value category is tracked
  constexpr auto can_and_then_lval = [](auto &&f) { return requires { std::declval<S &>().and_then(f); }; };
  constexpr auto can_and_then_rval = [](auto &&f) { return requires { std::declval<S &&>().and_then(f); }; };
  static_assert(can_and_then_lval(nothrow_lval));
  static_assert(not can_and_then_rval(nothrow_lval)); // no rvalue handlers
  static_assert(not can_and_then_lval(fn::overload{[](int &) -> fn::optional<bool> { return {true}; }})); // partial
  static_assert(not can_and_then_lval(42));

  SECTION("value")
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

  SECTION("error")
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

    SECTION("immovable result type")
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

  SECTION("constexpr")
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
  using S = fn::optional<fn::sum_for<Xint, int>>;

  // noexcept: the sum-case _transform (optional.hpp:233) carries no noexcept spec at all, so
  // transform over a sum is noexcept(false) even for a whole-sum-invocable nothrow callback
  // that and_then reports noexcept(true) for -- fn-native noexcept for sum dispatch is GH #254
  constexpr auto nothrow_visitor = fn::overload{[](int const &) noexcept -> bool { return true; },
                                                [](Xint const &) noexcept -> bool { return false; }};
  static_assert(not noexcept(std::declval<S &>().transform(nothrow_visitor)));
  constexpr auto nothrow_generic = [](auto &&) noexcept -> bool { return true; };
  static_assert(not noexcept(std::declval<S &>().transform(nothrow_generic)));

  constexpr auto can_transform = [](auto &&f) { return requires { std::declval<S &>().transform(f); }; };
  static_assert(can_transform(nothrow_visitor));

  SECTION("value")
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

  SECTION("error")
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

  SECTION("constexpr")
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

  SECTION("constraints")
  {
    constexpr auto can_transform_clval = [](auto &&f) { return requires { std::declval<S const &>().transform(f); }; };

    // a callback no alternative can take drops the candidate, rather than failing inside the body
    static_assert(not can_transform([](std::string_view) -> bool { throw 0; }));
    static_assert(not can_transform([](int &) -> bool { throw 0; })); // Xint is unhandled

    // a visitor need only serve the value category the call actually selects: the losing const&
    // candidate is dropped by its own constraint rather than forming its signature and poisoning
    // the call. The four-category visitors above are a spelling choice, not a requirement.
    constexpr auto lval_only = fn::overload{[](int &i) -> bool { return i == 12; }, [](Xint &) -> bool { throw 0; }};
    static_assert(can_transform(lval_only));
    static_assert(not can_transform_clval(lval_only));

    S s{12};
    CHECK(s.transform(lval_only).value() == fn::sum{true});
  }
}
