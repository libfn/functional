// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/optional.hpp>
#include <fn/utility.hpp>

#include <catch2/catch_all.hpp>

#include <tuple>
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

// The join invokes its error-continuation as an lvalue - a named parameter - whatever the value
// category it was passed in, and its specification must ask about that call. These answer the
// lvalue and the rvalue question differently, in both directions - and the third offers only the
// call the body performs. Namespace scope: a local class cannot have member templates.
struct EfnLvalueNothrow {
  constexpr auto operator()(auto const &) & noexcept -> std::nullopt_t { return std::nullopt; }
  auto operator()(auto const &) && noexcept(false) -> std::nullopt_t { throw 0; }
};
struct EfnRvalueNothrow {
  auto operator()(auto const &) & noexcept(false) -> std::nullopt_t { throw 0; }
  constexpr auto operator()(auto const &) && noexcept -> std::nullopt_t { return std::nullopt; }
};
struct EfnLvalueOnly {
  constexpr auto operator()(auto const &) & noexcept -> std::nullopt_t { return std::nullopt; }
  auto operator()(auto const &) && -> std::nullopt_t = delete;
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
    // these overloads wrap the value in a sum, so they weigh that construction - which for int
    // cannot throw
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

    static_assert(std::is_same_v<decltype(fn::sum_value(s)), fn::optional<fn::sum<int>>>);
    static_assert(noexcept(fn::sum_value(s))); // the free function propagates what the member says

    SECTION("throwing value")
    {
      // the lift weighs the value it relocates, so the spec tracks the category it relocates by
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

    // noexcept (extension): true only when the callback is nothrow-applicable, returns exactly
    // optional<sum<int>> (no widening), and *this is nothrow-constructible from itself.
    constexpr auto nothrow_same = []() noexcept -> fn::optional<fn::sum<int>> { return {std::nullopt}; };
    static_assert(noexcept(s.or_else(nothrow_same)));
    static_assert(noexcept(std::move(s).or_else(nothrow_same)));
    static_assert(not noexcept(s.or_else(fn3))); // fn3 throws
    constexpr auto nothrow_widen = []() noexcept -> fn::optional<Xint> { return {std::nullopt}; };
    static_assert(noexcept(s.or_else(nothrow_widen))); // nothrow but widens to sum_for<Xint, int>);

    // constraints (extension): a non-applicable argument drops or_else from the overload set via
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

    SECTION("noexcept")
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
  SECTION("and_then")
  {
    using P = fn::optional<fn::pack<int, std::string_view>>;

    // noexcept (extension): the spec asks fn's own nothrow-applicable trait, which asks the
    // dispatch that will actually run - one argument per element - so a multi-argument visitor is
    // weighed as it is called, not as std would call it on the pack itself.
    constexpr auto nothrow_two = [](int &, std::string_view &) noexcept -> fn::optional<bool> { return {true}; };
    static_assert(noexcept(std::declval<P &>().and_then(nothrow_two)));
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

    SECTION("noexcept")
    {
      // a pack's callback is invoked through fn's own dispatch, taking one argument per element: it
      // is not directly applicable on the pack, so only fn's nothrow-applicable trait can answer
      using T = fn::optional<fn::pack<int, double>>;
      static_assert(
          noexcept(std::declval<T &>().and_then([](int, double) noexcept -> fn::optional<bool> { return {true}; })));
      static_assert(
          not noexcept(std::declval<T &>().and_then([](int, double) -> fn::optional<bool> { return {true}; })));
      SUCCEED();
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
    static_assert(noexcept(std::declval<P &>().transform(nothrow_two)));
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

    SECTION("noexcept")
    {
      using T = fn::optional<fn::pack<int, double>>;
      static_assert(noexcept(std::declval<T &>().transform([](int, double) noexcept -> bool { return true; })));
      static_assert(not noexcept(std::declval<T &>().transform([](int, double) -> bool { return true; })));
      SUCCEED();
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
    // noexcept: the join relocates both operands' values into the result pack, and promises only
    // what relocating them promises - so a throwing-copy value type makes the lvalue join throwing.
    struct throwing_copy {
      // defined, not just declared: the instantiated join references it (-Wundefined-internal)
      throwing_copy(throwing_copy const &) noexcept(false) {}
    };
    static_assert(noexcept(std::declval<fn::optional<double> &>() & std::declval<fn::optional<int> &>()));
    static_assert(not noexcept(std::declval<fn::optional<throwing_copy> &>() & std::declval<fn::optional<int> &>()));

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

    SECTION("noexcept")
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

      SECTION("_join")
      {
        // the join invokes its error-continuation as an lvalue, and its specification asks about
        // that call - whether the callable arrives as a temporary or an lvalue
        using fn::detail::_join;
        static_assert(noexcept(_join<fn::optional>(std::declval<Rh &>(), std::declval<Rh &>(), EfnLvalueNothrow{})));
        static_assert(
            not noexcept(_join<fn::optional>(std::declval<Rh &>(), std::declval<Rh &>(), EfnRvalueNothrow{})));
        static_assert(noexcept(_join<fn::optional>(std::declval<Rh &>(), std::declval<Rh &>(), EfnLvalueOnly{})));
        static_assert(noexcept(
            _join<fn::optional>(std::declval<Rh &>(), std::declval<Rh &>(), std::declval<EfnLvalueNothrow &>())));
        static_assert(not noexcept(
            _join<fn::optional>(std::declval<Rh &>(), std::declval<Rh &>(), std::declval<EfnRvalueNothrow &>())));

        // the body performs the lvalue call: the rvalue overload throws, or does not exist
        CHECK(not _join<fn::optional>(Rh{std::nullopt}, Rh{12}, EfnLvalueNothrow{}).has_value());
        CHECK(not _join<fn::optional>(Rh{12}, Rh{std::nullopt}, EfnLvalueOnly{}).has_value());
        CHECK(_join<fn::optional>(Rh{3}, Rh{4}, EfnLvalueOnly{}).has_value());
        static_assert(not _join<fn::optional>(Rh{std::nullopt}, Rh{12}, EfnLvalueNothrow{}).has_value());
        static_assert(not _join<fn::optional>(Rh{12}, Rh{std::nullopt}, EfnLvalueOnly{}).has_value());
        static_assert(_join<fn::optional>(Rh{3}, Rh{4}, EfnLvalueOnly{}).has_value());
      }
      SUCCEED();
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

  // noexcept (extension): the spec asks fn's own nothrow-applicable trait, which asks the
  // per-alternative dispatch that will actually run - so a visitor set is weighed alternative by
  // alternative, and one throwing handler makes the whole dispatch throwing.
  constexpr auto nothrow_lval = fn::overload{[](int &) noexcept -> fn::optional<bool> { return {true}; },
                                             [](Xint &) noexcept -> fn::optional<bool> { return {false}; }};
  static_assert(noexcept(std::declval<S &>().and_then(nothrow_lval)));
  constexpr auto nothrow_generic = [](auto &&) noexcept -> fn::optional<bool> { return {true}; };
  static_assert(noexcept(std::declval<S &>().and_then(nothrow_generic)));

  // constraints (extension): exhaustive invocability over the sum's alternatives is a
  // constraint (optional.hpp:147) -- a partial or non-applicable visitor SFINAE-drops, and the
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

  SECTION("noexcept")
  {
    // the callback is invoked through fn's own dispatch, so only fn's nothrow-applicable trait can
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
  using S = fn::optional<fn::sum_for<Xint, int>>;

  // noexcept: the sum-case _transform weighs the per-alternative dispatch, as and_then does
  constexpr auto nothrow_visitor = fn::overload{[](int const &) noexcept -> bool { return true; },
                                                [](Xint const &) noexcept -> bool { return false; }};
  static_assert(noexcept(std::declval<S &>().transform(nothrow_visitor)));
  constexpr auto nothrow_generic = [](auto &&) noexcept -> bool { return true; };
  static_assert(noexcept(std::declval<S &>().transform(nothrow_generic)));

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

  SECTION("noexcept")
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

namespace {
template <typename S, typename Fn, typename... Args>
concept can_apply = requires(S s, Fn fn, Args... args) { FWD(s).apply(FWD(fn), FWD(args)...); };

template <typename S, typename R, typename Fn>
concept can_apply_r = requires(S s, Fn fn) { FWD(s).template apply_r<R>(FWD(fn)); };

template <typename S, typename Fn>
concept can_apply_type = requires(S s, Fn fn) { FWD(s).apply_type(FWD(fn)); };

template <typename S, typename R, typename Fn>
concept can_apply_type_r = requires(S s, Fn fn) { FWD(s).template apply_type_r<R>(FWD(fn)); };
} // anonymous namespace

TEST_CASE("optional apply", "[optional][apply]")
{
  using fn::optional;

  // both arms are required outright: the engaged arm receives the value as fn::apply hands it
  // over, the empty arm is invoked without it
  constexpr auto arms = fn::overload{[](int v) noexcept -> int { return v; }, []() noexcept -> int { return -1; }};
  optional<int> a{42};
  optional<int> e{std::nullopt};

  SECTION("noexcept")
  {
    static_assert(noexcept(a.apply(arms)));
    static_assert(noexcept(std::move(a).apply(arms)));
    static_assert(noexcept(a.apply_r<long>(arms)));
    constexpr auto throwing
        = fn::overload{[](int v) noexcept(false) -> int { return v; }, []() noexcept -> int { return -1; }};
    static_assert(not noexcept(a.apply(throwing)));
    static_assert(not noexcept(a.apply_r<long>(throwing)));
    SUCCEED();
  }

  SECTION("both arms required")
  {
    static_assert(can_apply<optional<int> &, decltype(arms) const &>);
    static_assert(not can_apply<optional<int> &, decltype(fn::overload{[](int) {}}) const &>);
    static_assert(not can_apply<optional<int> &, decltype(fn::overload{[]() {}}) const &>);
    // one generic arm serves both states
    static_assert(can_apply<optional<int> &, decltype([](auto &&...) {}) const &>);

    CHECK(a.apply(arms) == 42);
    CHECK(e.apply(arms) == -1);

    SECTION("constexpr")
    {
      static_assert(optional<int>{42}.apply(arms) == 42);
      static_assert(optional<int>{std::nullopt}.apply(arms) == -1);
      SUCCEED();
    }
  }

  SECTION("value categories")
  {
    CHECK(a.apply(fn::overload{[]() -> bool { throw 1; }, //
                               [](int &) -> bool { return true; }, [](int const &) -> bool { throw 0; },
                               [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; }}));
    CHECK(std::as_const(a).apply(fn::overload{[]() -> bool { throw 1; }, //
                                              [](int &) -> bool { throw 0; }, [](int const &) -> bool { return true; },
                                              [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; }}));
    CHECK(std::move(std::as_const(a))
              .apply(fn::overload{[]() -> bool { throw 1; }, //
                                  [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                                  [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { return true; }}));
    CHECK(std::move(a).apply(fn::overload{[]() -> bool { throw 1; }, //
                                          [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                                          [](int &&) -> bool { return true; }, [](int const &&) -> bool { throw 0; }}));

    SECTION("constexpr")
    {
      // one result type across both arms is the rule, so selection is encoded in values
      constexpr optional<int> b{42};
      constexpr auto categories = fn::overload{[]() -> int { return 0; }, //
                                               [](int &) -> int { return 1; }, [](int const &) -> int { return 2; },
                                               [](int &&) -> int { return 3; }, [](int const &&) -> int { return 4; }};
      static_assert(b.apply(categories) == 2);
      static_assert(std::move(b).apply(categories) == 4);
      SUCCEED();
    }
  }

  SECTION("extra arguments")
  {
    constexpr auto xarms
        = fn::overload{[](int v, int x) noexcept -> int { return v + x; }, [](int x) noexcept -> int { return -x; }};
    CHECK(a.apply(xarms, 2) == 44);
    CHECK(e.apply(xarms, 2) == -2);

    SECTION("constexpr")
    {
      static_assert(optional<int>{42}.apply(xarms, 2) == 44);
      static_assert(optional<int>{std::nullopt}.apply(xarms, 2) == -2);
      SUCCEED();
    }
  }

  SECTION("pack payload")
  {
    using P = fn::pack<int, int>;
    optional<P> p{std::in_place, fn::pack{6, 7}};
    constexpr auto parms
        = fn::overload{[](int x, int y) noexcept -> int { return x * y; }, []() noexcept -> int { return -1; }};
    CHECK(p.apply(parms) == 42);
    CHECK(optional<P>{std::nullopt}.apply(parms) == -1);

    SECTION("constexpr")
    {
      static_assert(optional<P>{std::in_place, fn::pack{6, 7}}.apply(parms) == 42);
      SUCCEED();
    }
  }

  SECTION("tuple-like payload")
  {
    using T = std::tuple<int, char>;
    optional<T> t{std::in_place, 40, char(2)};
    constexpr auto tarms
        = fn::overload{[](int x, char y) noexcept -> int { return x + y; }, []() noexcept -> int { return -1; }};
    CHECK(t.apply(tarms) == 42);
    // pass-whole still serves a whole-tuple arm on the untagged path (contrast apply_type)
    constexpr auto whole
        = fn::overload{[](T const &v) noexcept -> int { return std::get<0>(v); }, []() noexcept -> int { return -1; }};
    CHECK(t.apply(whole) == 40);

    SECTION("constexpr")
    {
      static_assert(optional<T>{std::in_place, 40, char(2)}.apply(tarms) == 42);
      SUCCEED();
    }
  }

  SECTION("sum payload")
  {
    using S = fn::sum<bool, int>;
    optional<S> s{std::in_place, S{42}};
    constexpr auto sarms = fn::overload{[](bool) noexcept -> int { return 1; }, [](int) noexcept -> int { return 2; },
                                        []() noexcept -> int { return -1; }};
    CHECK(s.apply(sarms) == 2);
    CHECK(optional<S>{std::nullopt}.apply(sarms) == -1);

    SECTION("constexpr")
    {
      static_assert(optional<S>{std::in_place, S{42}}.apply(sarms) == 2);
      SUCCEED();
    }
  }

  SECTION("reference optional")
  {
    int x = 41;
    optional<int &> r{x};
    constexpr auto rarms
        = fn::overload{[](int &v) noexcept -> int { return v + 1; }, []() noexcept -> int { return -1; }};
    CHECK(r.apply(rarms) == 42);
    CHECK(optional<int &>{std::nullopt}.apply(rarms) == -1);
    CHECK(r.apply_r<long>(rarms) == 42L);
  }

  SECTION("apply_r")
  {
    static_assert(std::is_same_v<long, decltype(a.apply_r<long>(arms))>);
    CHECK(a.apply_r<long>(arms) == 42L);
    CHECK(e.apply_r<long>(arms) == -1L);

    // the conversion to Ret is part of the question
    static_assert(not can_apply_r<optional<int> &, char *, decltype(arms) const &>);
    static_assert(can_apply_r<optional<int> &, long, decltype(arms) const &>);

    SECTION("constexpr")
    {
      static_assert(optional<int>{42}.apply_r<long>(arms) == 42L);
      SUCCEED();
    }
  }
}

TEST_CASE("optional apply_type", "[optional][apply_type]")
{
  using fn::optional;
  using std::in_place_t;
  using std::nullopt_t;

  // the tags name the two states: the engaged arm receives std::in_place followed by the value,
  // the empty arm std::nullopt alone
  constexpr auto arms
      = fn::overload{[](in_place_t, int v) noexcept -> int { return v; }, [](nullopt_t) noexcept -> int { return -1; }};
  optional<int> a{42};
  optional<int> e{std::nullopt};

  SECTION("noexcept")
  {
    static_assert(noexcept(a.apply_type(arms)));
    static_assert(noexcept(std::move(a).apply_type(arms)));
    static_assert(noexcept(a.apply_type_r<long>(arms)));
    constexpr auto throwing = fn::overload{[](in_place_t, int v) noexcept(false) -> int { return v; },
                                           [](nullopt_t) noexcept -> int { return -1; }};
    static_assert(not noexcept(a.apply_type(throwing)));
    static_assert(not noexcept(a.apply_type_r<long>(throwing)));
    SUCCEED();
  }

  SECTION("exhaustive over both states")
  {
    static_assert(can_apply_type<optional<int> &, decltype(arms) const &>);
    static_assert(not can_apply_type<optional<int> &, decltype(fn::overload{[](in_place_t, int) {}}) const &>);
    static_assert(not can_apply_type<optional<int> &, decltype(fn::overload{[](nullopt_t) {}}) const &>);
    // the tags never convert: arms keyed by a different tag kind are not served
    static_assert(not can_apply_type<optional<int> &, decltype(fn::overload{[](std::in_place_type_t<int>, int) {},
                                                                            [](nullopt_t) {}}) const &>);

    // the tags reach the arms as prvalues, so rvalue-tag arms are served - probe and deed agree
    constexpr auto rv_tag = fn::overload{[](in_place_t &&, int v) noexcept -> int { return v; },
                                         [](nullopt_t &&) noexcept -> int { return -1; }};
    static_assert(can_apply_type<optional<int> &, decltype(rv_tag) const &>);
    CHECK(a.apply_type(rv_tag) == 42);
    CHECK(e.apply_type(rv_tag) == -1);

    CHECK(a.apply_type(arms) == 42);
    CHECK(e.apply_type(arms) == -1);

    SECTION("constexpr")
    {
      static_assert(optional<int>{42}.apply_type(arms) == 42);
      static_assert(optional<int>{std::nullopt}.apply_type(arms) == -1);
      SUCCEED();
    }
  }

  SECTION("value categories")
  {
    CHECK(a.apply_type(
        fn::overload{[](nullopt_t) -> bool { throw 1; }, //
                     [](in_place_t, int &) -> bool { return true; }, [](in_place_t, int const &) -> bool { throw 0; },
                     [](in_place_t, int &&) -> bool { throw 0; }, [](in_place_t, int const &&) -> bool { throw 0; }}));
    CHECK(std::as_const(a).apply_type(
        fn::overload{[](nullopt_t) -> bool { throw 1; }, //
                     [](in_place_t, int &) -> bool { throw 0; }, [](in_place_t, int const &) -> bool { return true; },
                     [](in_place_t, int &&) -> bool { throw 0; }, [](in_place_t, int const &&) -> bool { throw 0; }}));
    CHECK(std::move(std::as_const(a))
              .apply_type(fn::overload{
                  [](nullopt_t) -> bool { throw 1; }, //
                  [](in_place_t, int &) -> bool { throw 0; }, [](in_place_t, int const &) -> bool { throw 0; },
                  [](in_place_t, int &&) -> bool { throw 0; }, [](in_place_t, int const &&) -> bool { return true; }}));
    CHECK(std::move(a).apply_type(fn::overload{
        [](nullopt_t) -> bool { throw 1; }, //
        [](in_place_t, int &) -> bool { throw 0; }, [](in_place_t, int const &) -> bool { throw 0; },
        [](in_place_t, int &&) -> bool { return true; }, [](in_place_t, int const &&) -> bool { throw 0; }}));

    SECTION("constexpr")
    {
      // one result type across both arms is the rule, so selection is encoded in values
      constexpr optional<int> b{42};
      constexpr auto categories = fn::overload{
          [](nullopt_t) -> int { return 0; }, //
          [](in_place_t, int &) -> int { return 1; }, [](in_place_t, int const &) -> int { return 2; },
          [](in_place_t, int &&) -> int { return 3; }, [](in_place_t, int const &&) -> int { return 4; }};
      static_assert(b.apply_type(categories) == 2);
      static_assert(std::move(b).apply_type(categories) == 4);
      SUCCEED();
    }
  }

  SECTION("pack payload")
  {
    // the arm receives (tag, elements...)
    using P = fn::pack<int, int>;
    optional<P> p{std::in_place, fn::pack{6, 7}};
    constexpr auto parms = fn::overload{[](in_place_t, int x, int y) noexcept -> int { return x * y; },
                                        [](nullopt_t) noexcept -> int { return -1; }};
    CHECK(p.apply_type(parms) == 42);
    CHECK(optional<P>{std::nullopt}.apply_type(parms) == -1);

    // the optional's category reaches the elements
    CHECK(p.apply_type(fn::overload{[](nullopt_t) -> bool { throw 1; },
                                    [](in_place_t, int &, int &) -> bool { return true; },
                                    [](in_place_t, int const &, int const &) -> bool { throw 0; }}));
    CHECK(std::as_const(p).apply_type(fn::overload{[](nullopt_t) -> bool { throw 1; },
                                                   [](in_place_t, int &, int &) -> bool { throw 0; },
                                                   [](in_place_t, int const &, int const &) -> bool { return true; }}));

    SECTION("constexpr")
    {
      static_assert(optional<P>{std::in_place, fn::pack{6, 7}}.apply_type(parms) == 42);
      SUCCEED();
    }
  }

  SECTION("tuple-like payload")
  {
    using T = std::tuple<int, char>;
    optional<T> t{std::in_place, 40, char(2)};
    constexpr auto tarms = fn::overload{[](in_place_t, int x, char y) noexcept -> int { return x + y; },
                                        [](nullopt_t) noexcept -> int { return -1; }};
    CHECK(t.apply_type(tarms) == 42);

    // the elements form is the row's one signature: an arm for the whole tuple is not served
    static_assert(
        not can_apply_type<optional<T> &, decltype(fn::overload{[](in_place_t, T const &) -> int { return 0; },
                                                                [](nullopt_t) -> int { return 0; }}) const &>);

    SECTION("constexpr")
    {
      static_assert(optional<T>{std::in_place, 40, char(2)}.apply_type(tarms) == 42);
      SUCCEED();
    }
  }

  SECTION("sum payload")
  {
    // a sum payload dispatches under the tag, and its exhaustiveness composes
    using S = fn::sum_for<int, Xint>;
    optional<S> s{std::in_place, S{42}};
    constexpr auto sarms
        = fn::overload{[](in_place_t, Xint const &) noexcept -> int { return 1; },
                       [](in_place_t, int) noexcept -> int { return 2; }, [](nullopt_t) noexcept -> int { return -1; }};
    CHECK(s.apply_type(sarms) == 2);
    CHECK(optional<S>{std::nullopt}.apply_type(sarms) == -1);

    constexpr auto no_xint
        = fn::overload{[](in_place_t, int) noexcept -> int { return 2; }, [](nullopt_t) noexcept -> int { return -1; }};
    static_assert(not can_apply_type<optional<S> &, decltype(no_xint) const &>);

    // within the payload the dispatch is the value path: over sum<bool, int> a lone int arm
    // absorbs the bool alternative - the tag guards the optional's row, not the sum's
    static_assert(can_apply_type<optional<fn::sum<bool, int>> &,
                                 decltype(fn::overload{[](in_place_t, int) -> int { return 0; },
                                                       [](nullopt_t) -> int { return 0; }}) const &>);

    SECTION("constexpr")
    {
      static_assert(optional<S>{std::in_place, S{42}}.apply_type(sarms) == 2);
      SUCCEED();
    }
  }

  SECTION("reference optional")
  {
    int x = 41;
    optional<int &> r{x};
    constexpr auto rarms = fn::overload{[](in_place_t, int &v) noexcept -> int { return v + 1; },
                                        [](nullopt_t) noexcept -> int { return -1; }};
    CHECK(r.apply_type(rarms) == 42);
    CHECK(optional<int &>{std::nullopt}.apply_type(rarms) == -1);
    CHECK(r.apply_type_r<long>(rarms) == 42L);
  }

  SECTION("apply_type_r")
  {
    static_assert(std::is_same_v<long, decltype(a.apply_type_r<long>(arms))>);
    CHECK(a.apply_type_r<long>(arms) == 42L);
    CHECK(e.apply_type_r<long>(arms) == -1L);

    // the conversion to Ret is part of the question
    static_assert(not can_apply_type_r<optional<int> &, char *, decltype(arms) const &>);
    static_assert(can_apply_type_r<optional<int> &, long, decltype(arms) const &>);

    SECTION("constexpr")
    {
      static_assert(optional<int>{42}.apply_type_r<long>(arms) == 42L);
      SUCCEED();
    }
  }
}
