// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/choice.hpp>

#include <fn/utility.hpp>

#include <util/helper_types.hpp>

#include <catch2/catch_all.hpp>

#include <utility>

namespace {

struct TestType final {
  static int count;
  TestType() noexcept { ++count; }
  ~TestType() noexcept { --count; }
};
int TestType::count = 0;

struct NonCopyable final {
  int i;

  constexpr NonCopyable(int i) noexcept : i(i) {}
  NonCopyable(NonCopyable const &) = delete;
  NonCopyable &operator=(NonCopyable const &) = delete;
};

// Every operation choice performs on an alternative - copy, move, compare - can throw here, while
// the member wrapping it promises noexcept regardless. Witnesses the #280 tripwires below.
struct Throwing final {
  int v;

  constexpr operator int() const { return v; }
  constexpr Throwing(int i) noexcept : v(i) {}
  constexpr Throwing(Throwing const &o) noexcept(false) : v(o.v) {}
  constexpr Throwing(Throwing &&o) noexcept(false) : v(o.v) {}
  constexpr bool operator==(Throwing const &o) const noexcept(false) { return v == o.v; }
};

} // anonymous namespace

TEST_CASE("choice non-monadic functionality", "[choice]")
{
  // NOTE This test looks very similar to test in sum.cpp - for good reason.

  using fn::choice;

  SECTION("value")
  {
    // _for aliases: a choice's value_type is the sum of its alternatives, but the canonical order is
    // platform-specific (see SECTION("choice_for") below), so normalize both sides per platform.
    static_assert(std::same_as<fn::sum_for<NonCopyable, int>, typename fn::choice_for<NonCopyable, int>::value_type>);
    static_assert(std::same_as<fn::sum<int>, typename choice<int>::value_type>);

    using type = fn::choice<bool, helper>;
    using value_type = fn::sum<bool, helper>;
    static_assert(std::same_as<value_type &, decltype(std::declval<type &>().value())>);
    static_assert(std::same_as<value_type const &, decltype(std::declval<type const &>().value())>);
    static_assert(std::same_as<value_type &&, decltype(std::declval<type &&>().value())>);
    static_assert(std::same_as<value_type const &&, decltype(std::declval<type const &&>().value())>);

    type s{helper{1}};
    s.value().get_ptr<helper>()->v = 42;
    constexpr auto fn = fn::overload{
        [](auto &&) -> int { throw 0; }, //
        [](helper &o) -> int { return o.v + 1; },  [](helper const &o) -> int { return o.v + 2; },
        [](helper &&o) -> int { return o.v + 3; }, [](helper const &&o) -> int { return o.v + 4; },
    };
    static_assert(std::same_as<int, decltype(s.value().invoke(fn))>);
    CHECK((s.value().invoke(fn)) == 43);
    CHECK((std::as_const(s).value().invoke(fn)) == 44);
    CHECK((std::move(std::as_const(s)).value().invoke(fn)) == 46);
    CHECK((std::move(s).value().invoke(fn)) == 45);
  }

  SECTION("choice_for")
  {
    static_assert(std::same_as<fn::choice_for<int>, fn::choice<int>>);
    static_assert(std::same_as<fn::choice_for<int, int>, fn::choice<int>>);
    static_assert(std::same_as<fn::choice_for<int, bool>, fn::choice<bool, int>>);
    static_assert(std::same_as<fn::choice_for<bool, int>, fn::choice<bool, int>>);
    // Canonical alternative order is platform/ABI-specific (derived from the compiler's type spelling
    // — GCC/Clang __PRETTY_FUNCTION__ vs MSVC __FUNCSIG__): MSVC sorts class/struct types after the
    // fundamentals, GCC/Clang before. Inherent and deliberately NOT unified (not even by C++26
    // std::type_order — an ABI-tied total order); see tests/fn/sum.cpp for the full rationale. This
    // one assert documents the divergence; the rest assert only platform-independent invariants.
    // Spelling: choices/sums whose alternatives include a non-builtin have platform-specific order, so
    // they are written choice_for<...>; pure-builtin ones keep a fixed choice<...>.
#ifdef _MSC_VER
    static_assert(std::same_as<fn::choice_for<int, NonCopyable>, fn::choice<int, NonCopyable>>);
#else
    static_assert(std::same_as<fn::choice_for<int, NonCopyable>, fn::choice<NonCopyable, int>>);
#endif
    static_assert(std::same_as<fn::choice_for<NonCopyable, int>, fn::choice_for<int, NonCopyable>>); // commutative
    static_assert(
        std::same_as<fn::choice_for<int, bool, NonCopyable>, fn::choice_for<NonCopyable, bool, int>>); // commutative
    static_assert(
        std::same_as<fn::choice_for<NonCopyable, int, NonCopyable>, fn::choice_for<int, NonCopyable>>); // unique
    static_assert(fn::choice_for<int, bool, NonCopyable>::size == 3);

    static_assert(std::same_as<fn::choice_for<int, fn::sum<int>>, fn::choice<int>>);
    static_assert(std::same_as<fn::choice_for<int, fn::sum<bool>>, fn::choice<bool, int>>);
    static_assert(std::same_as<fn::choice_for<int, fn::sum<bool, int>>, fn::choice<bool, int>>);
    static_assert(std::same_as<fn::choice_for<int, fn::sum<bool, double>>, fn::choice<bool, double, int>>);

    static_assert(std::same_as<fn::choice_for<fn::sum<bool>, fn::sum<int>>, fn::choice<bool, int>>);
    static_assert(
        std::same_as<fn::choice_for<fn::sum<bool>, fn::sum<bool, double, int>>, fn::choice<bool, double, int>>);
    static_assert(std::same_as<fn::choice_for<fn::sum<bool>, fn::sum<double, int>>, fn::choice<bool, double, int>>);
    static_assert(std::same_as<fn::choice_for<fn::sum<bool, int>, double>, fn::choice<bool, double, int>>);

    static_assert(std::same_as<fn::choice_for<int, fn::sum<>>, fn::choice<int>>);
    static_assert(std::same_as<fn::choice_for<fn::sum<>, int>, fn::choice<int>>);
    static_assert(std::same_as<fn::choice_for<fn::sum<>, fn::sum<bool, int>>, fn::choice<bool, int>>);
    static_assert(std::same_as<fn::choice_for<double, fn::sum<>, fn::sum<bool, int>>, fn::choice<bool, double, int>>);
  }

  SECTION("invocable")
  {
    using type = fn::choice_for<TestType, int>; // choice<...> order is platform-specific; choice_for normalizes
    static_assert(fn::typelist_invocable<decltype([](auto) {}), type &>);
    static_assert(fn::typelist_invocable<decltype([](auto &) {}), type &>);
    static_assert(fn::typelist_invocable<decltype([](auto const &) {}), type &>);
    static_assert(fn::typelist_invocable<decltype(fn::overload{[](int &) {}, [](TestType &) {}}), type &>);
    static_assert(fn::typelist_invocable<decltype(fn::overload{[](int) {}, [](TestType) {}}), type const &>);

    static_assert(not fn::typelist_invocable<decltype([](TestType &) {}), type &>); // missing int
    static_assert(not fn::typelist_invocable<decltype([](int &) {}), type &>);      // missing TestType
    static_assert(not fn::typelist_invocable<decltype(fn::overload{[](int &&) {}, [](TestType &&) {}}),
                                             type &>); // cannot bind lvalue to rvalue-reference
    static_assert(not fn::typelist_invocable<decltype([](auto &) {}),
                                             type &&>); // cannot bind rvalue to lvalue-reference
    static_assert(not fn::typelist_invocable<decltype([](auto, auto &) {}), type &>); // bad arity
    static_assert(not fn::typelist_invocable<decltype(fn::overload{[](int &) {}, [](TestType &) {}}),
                                             type const &>); // cannot bind const to non-const reference

    static_assert(fn::typelist_invocable<decltype([](auto &) {}), choice<NonCopyable> &>);
    static_assert(not fn::typelist_invocable<decltype([](auto) {}), NonCopyable &>); // copy-constructor not available
  }

  SECTION("single parameter constructor")
  {
    constexpr choice<int> a = 12;
    static_assert(a == choice{12});

    constexpr choice<bool> b{false};
    static_assert(b == choice{false});

    SECTION("CTAD")
    {
      choice a{42};
      static_assert(std::is_same_v<decltype(a), choice<int>>);
      CHECK(a == choice<int>{42});

      constexpr choice b{false};
      static_assert(std::is_same_v<decltype(b), choice<bool> const>);
      static_assert(b == choice<bool>{false});

      constexpr auto c = choice{std::array<int, 3>{3, 14, 15}};
      static_assert(std::is_same_v<decltype(c), choice<std::array<int, 3>> const>);
      static_assert(c.invoke([](auto &&a) -> bool { return a.size() == 3 && a[0] == 3 && a[1] == 14 && a[2] == 15; }));
    }

    SECTION("constexpr move from rvalue")
    {
      using T = fn::choice<bool, helper>;
      constexpr auto fn = [](auto i) constexpr noexcept -> T { return {std::move(i)}; };
      constexpr auto a = fn(true);
      static_assert(std::is_same_v<decltype(a), T const>);
      static_assert(a.has_value<bool>());
      static_assert(a.value() == fn::sum{true});

      constexpr auto b = fn(helper{{0.5, 2.0}, 19});
      static_assert(std::is_same_v<decltype(b), T const>);
      static_assert(b.has_value<helper>());
      static_assert(b.value().get_ptr<helper>()->v == 19 * from_rval);
    }

    SECTION("move from rvalue")
    {
      using T = fn::choice<bool, helper>;
      constexpr auto fn = [](auto i) noexcept -> T { return {std::move(i)}; };
      auto const a = fn(helper{9});
      static_assert(std::is_same_v<decltype(a), T const>);
      CHECK(a.has_value<helper>());
      CHECK(a.value().get_ptr<helper>()->v == 9 * from_rval);

      auto b = fn(true);
      static_assert(std::is_same_v<decltype(b), T>);
      CHECK(b.has_value<bool>());
      CHECK(b.value() == fn::sum{true});
    }

    SECTION("constexpr move from const rvalue")
    {
      using T = fn::choice<bool, helper>;
      constexpr auto fn = [](auto const i) constexpr noexcept -> T { return {std::move(i)}; };
      constexpr auto a = fn(true);
      static_assert(std::is_same_v<decltype(a), T const>);
      static_assert(a.has_value<bool>());
      static_assert(a.value() == fn::sum{true});

      constexpr auto b = fn(helper{{0.5, 2.0}, 17});
      static_assert(std::is_same_v<decltype(b), T const>);
      static_assert(b.has_value<helper>());
      static_assert(b.value().get_ptr<helper>()->v == 17 * from_rval_const);
    }

    SECTION("move from const rvalue")
    {
      using T = fn::choice<bool, helper>;
      constexpr auto fn = [](auto const i) noexcept -> T { return {std::move(i)}; };
      auto const a = fn(helper{7});
      static_assert(std::is_same_v<decltype(a), T const>);
      CHECK(a.has_value<helper>());
      CHECK(a.value().get_ptr<helper>()->v == 7 * from_rval_const);

      auto b = fn(true);
      static_assert(std::is_same_v<decltype(b), T>);
      CHECK(b.has_value<bool>());
      CHECK(b.value() == fn::sum{true});
    }

    SECTION("constexpr copy from lvalue")
    {
      using T = fn::choice<bool, helper>;
      constexpr auto fn = [](auto i) constexpr noexcept -> T { return {i}; };
      constexpr auto a = fn(true);
      static_assert(std::is_same_v<decltype(a), T const>);
      static_assert(a.has_value<bool>());
      static_assert(a.value() == fn::sum{true});

      constexpr auto b = fn(helper{{0.5, 2.0}, 17});
      static_assert(std::is_same_v<decltype(b), T const>);
      static_assert(b.has_value<helper>());
      static_assert(b.value().get_ptr<helper>()->v == 17 * from_lval);
    }

    SECTION("copy from lvalue")
    {
      using T = fn::choice<bool, helper>;
      constexpr auto fn = [](auto i) noexcept -> T { return {i}; };
      auto const a = fn(true);
      static_assert(std::is_same_v<decltype(a), T const>);
      CHECK(a.has_value<bool>());
      CHECK(a.value() == fn::sum{true});

      auto b = fn(helper{13});
      static_assert(std::is_same_v<decltype(b), T>);
      CHECK(b.has_value<helper>());
      CHECK(b.value().get_ptr<helper>()->v == 13 * from_lval);
    }

    SECTION("constexpr copy from const lvalue")
    {
      using T = fn::choice<bool, helper>;
      constexpr auto fn = [](auto const i) constexpr noexcept -> T { return {i}; };
      constexpr auto a = fn(true);
      static_assert(std::is_same_v<decltype(a), T const>);
      static_assert(a.has_value<bool>());
      static_assert(a.value() == fn::sum{true});

      constexpr auto b = fn(helper{{0.5, 2.0}, 15});
      static_assert(std::is_same_v<decltype(b), T const>);
      static_assert(b.has_value<helper>());
      static_assert(b.value().get_ptr<helper>()->v == 15 * from_lval_const);
    }

    SECTION("copy from const lvalue")
    {
      using T = fn::choice<bool, helper>;
      constexpr auto fn = [](auto const i) noexcept -> T { return {i}; };
      auto const a = fn(true);
      static_assert(std::is_same_v<decltype(a), T const>);
      CHECK(a.has_value<bool>());
      CHECK(a.value() == fn::sum{true});

      auto b = fn(helper{5});
      static_assert(std::is_same_v<decltype(b), T>);
      CHECK(b.has_value<helper>());
      CHECK(b.value().get_ptr<helper>()->v == 5 * from_lval_const);
    }

    SECTION("copy ctor")
    {
      using T = fn::choice<bool, helper>;
      auto a = T{helper{1}};
      a.value().get_ptr<helper>()->v = 23;
      auto const b = std::as_const(a);

      CHECK(b.has_value<helper>());
      CHECK(b.value().get_ptr<helper>()->v == 23 * from_lval_const);
    }

    SECTION("move ctor")
    {
      using T = fn::choice<bool, helper>;
      auto a = T{helper{1}};
      a.value().get_ptr<helper>()->v = 29;
      auto const b = std::move(a);

      CHECK(b.has_value<helper>());
      CHECK(b.value().get_ptr<helper>()->v == 29 * from_rval);
    }
  }

  SECTION("constructor from sum")
  {
    using T = fn::choice<bool, helper>;
    SECTION("move from rvalue")
    {
      fn::sum h{helper{1}};
      h.get_ptr<helper>()->v = 17;
      T const a{std::move(h)};
      CHECK(a.value().get_ptr<helper>()->v == 17 * from_rval);
    }
    SECTION("copy from const lvalue")
    {
      fn::sum h{helper{1}};
      h.get_ptr<helper>()->v = 19;
      T const a{std::as_const(h)};
      CHECK(a.value().get_ptr<helper>()->v == 19 * from_lval_const);
    }
  }

  SECTION("forwarding constructors (immovable)")
  {
    choice<NonCopyable> a{std::in_place_type<NonCopyable>, 42};
    CHECK(a.invoke([](auto &i) -> bool { return i.i == 42; }));

    SECTION("CTAD")
    {
      constexpr auto a = choice{std::in_place_type<NonCopyable>, 42};
      static_assert(std::is_same_v<decltype(a), choice<NonCopyable> const>);

      auto b = choice{std::in_place_type<NonCopyable>, 42};
      static_assert(std::is_same_v<decltype(b), choice<NonCopyable>>);
    }
  }

  SECTION("forwarding constructors (aggregate)")
  {
    SECTION("regular")
    {
      choice<std::array<int, 3>> a{std::in_place_type<std::array<int, 3>>, 1, 2, 3};
      static_assert(decltype(a)::has_type<std::array<int, 3>>);
      static_assert(not decltype(a)::has_type<int>);
      CHECK(a.has_value(std::in_place_type<std::array<int, 3>>));
      CHECK(a.template has_value<std::array<int, 3>>());
      CHECK(a.invoke([](auto &i) -> bool {
        return std::same_as<std::array<int, 3> &, decltype(i)> && i.size() == 3 && i[0] == 1 && i[1] == 2 && i[2] == 3;
      }));
    }

    SECTION("constexpr")
    {
      constexpr choice<std::array<int, 3>> a{std::in_place_type<std::array<int, 3>>, 1, 2, 3};
      static_assert(decltype(a)::has_type<std::array<int, 3>>);
      static_assert(not decltype(a)::has_type<int>);
      static_assert(a.has_value(std::in_place_type<std::array<int, 3>>));
      static_assert(a.template has_value<std::array<int, 3>>());
      static_assert(a.invoke([](auto &i) -> bool {
        return std::same_as<std::array<int, 3> const &, decltype(i)> && i.size() == 3 && i[0] == 1 && i[1] == 2
               && i[2] == 3;
      }));
    }

    SECTION("CTAD")
    {
      constexpr auto a = choice{std::in_place_type<std::array<int, 3>>, 1, 2, 3};
      static_assert(std::is_same_v<decltype(a), choice<std::array<int, 3>> const>);

      auto b = choice{std::in_place_type<std::array<int, 3>>, 1, 2, 3};
      static_assert(std::is_same_v<decltype(b), choice<std::array<int, 3>>>);
    }
  }

  SECTION("equality comparison")
  {
    // The comparison operators are sum's - free functions taking sum<Ts...> const & - and sum.cpp
    // owns their grid. What is choice's own is that a choice reaches them at all, through its public
    // base; that, and the result type, is what is asserted here.
    using type = choice<bool, int>;
    constexpr type a{std::in_place_type<int>, 42};

    static_assert(std::is_same_v<bool, decltype(a == choice{42})>);
    static_assert(a == type{42});
    static_assert(a != type{41});
    static_assert(a != type{true});
    static_assert(a == choice<double, int>{42}); // and across differing alternative lists
    static_assert(a != choice<double, int>{41});

    CHECK(a == type{42});
    CHECK(a != type{41});
  }

  SECTION("invoke")
  {
    choice<int> a{std::in_place_type<int>, 42};
    SECTION("value only")
    {
      CHECK(a.invoke(  //
          fn::overload{//
                       [](auto) -> bool { throw 1; }, [](int &) -> bool { return true; },
                       [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                       [](int const &&) -> bool { throw 0; }}));
      CHECK(std::as_const(a).invoke( //
          fn::overload{              //
                       [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                       [](int const &) -> bool { return true; }, [](int &&) -> bool { throw 0; },
                       [](int const &&) -> bool { throw 0; }}));
      CHECK(std::move(std::as_const(a))
                .invoke(fn::overload{[](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                     [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                     [](int const &&) -> bool { return true; }}));
      CHECK(std::move(a).invoke( //
          fn::overload{          //
                       [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                       [](int const &) -> bool { throw 0; }, [](int &&) -> bool { return true; },
                       [](int const &&) -> bool { throw 0; }}));

      SECTION("constexpr")
      {
        constexpr choice<int> a{std::in_place_type<int>, 42};
        static_assert(a.invoke(fn::overload{
            [](auto) -> std::false_type { return {}; }, //
            [](int &) -> std::false_type { return {}; }, [](int const &) -> std::true_type { return {}; },
            [](int &&) -> std::false_type { return {}; }, [](int const &&) -> std::false_type { return {}; }}));
        static_assert(std::move(a).invoke(fn::overload{
            [](auto) -> std::false_type { return {}; }, //
            [](int &) -> std::false_type { return {}; }, [](int const &) -> std::false_type { return {}; },
            [](int &&) -> std::false_type { return {}; }, [](int const &&) -> std::true_type { return {}; }}));
      }
    }
  }

  SECTION("invoke_r")
  {
    choice<int> a{std::in_place_type<int>, 42};
    SECTION("value only")
    {
      CHECK(a.invoke_r<int>( //
          fn::overload{      //
                       [](auto) -> int { throw 1; }, [](int &) -> bool { return true; },
                       [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                       [](int const &&) -> bool { throw 0; }}));
      CHECK(std::as_const(a).invoke_r<int>( //
          fn::overload{                     //
                       [](auto) -> int { throw 1; }, [](int &) -> bool { throw 0; },
                       [](int const &) -> bool { return true; }, [](int &&) -> bool { throw 0; },
                       [](int const &&) -> bool { throw 0; }}));
      CHECK(std::move(std::as_const(a))
                .invoke_r<int>(fn::overload{[](auto) -> int { throw 1; }, [](int &) -> bool { throw 0; },
                                            [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                            [](int const &&) -> bool { return true; }}));
      CHECK(std::move(a).invoke_r<int>( //
          fn::overload{                 //
                       [](auto) -> int { throw 1; }, [](int &) -> bool { throw 0; },
                       [](int const &) -> bool { throw 0; }, [](int &&) -> bool { return true; },
                       [](int const &&) -> bool { throw 0; }}));

      SECTION("constexpr")
      {
        constexpr choice<int> a{std::in_place_type<int>, 42};
        static_assert(a.invoke_r<int>(fn::overload{
            [](auto) -> std::false_type { return {}; }, //
            [](int &) -> std::false_type { return {}; }, [](int const &) -> std::true_type { return {}; },
            [](int &&) -> std::false_type { return {}; }, [](int const &&) -> std::false_type { return {}; }}));
        static_assert(std::move(a).invoke_r<int>(fn::overload{
            [](auto) -> std::false_type { return {}; }, //
            [](int &) -> std::false_type { return {}; }, [](int const &) -> std::false_type { return {}; },
            [](int &&) -> std::false_type { return {}; }, [](int const &&) -> std::true_type { return {}; }}));
      }
    }
  }
}

TEST_CASE("choice noexcept", "[choice][noexcept]")
{
  using fn::choice;
  using T = choice<Throwing>;

  static_assert(not std::is_nothrow_copy_constructible_v<Throwing>);
  static_assert(not std::is_nothrow_move_constructible_v<Throwing>);

  // GAP #280: choice inherits sum's unconditional promise and adds its own. The copy and move
  // constructors are defaulted noexcept over an alternative whose own copy and move can throw.
  static_assert(std::is_nothrow_copy_constructible_v<T>);
  static_assert(std::is_nothrow_move_constructible_v<T>);

  // The monadic members are unconditionally noexcept whatever the callback promises. This is where
  // choice parts company with fn::optional and fn::expected, whose specs for the same operations are
  // precise - so the same monadic operation has different exception behaviour depending on which
  // monad it is written against: those two propagate, choice terminates.
  constexpr auto fnChoice = [](Throwing const &t) noexcept(false) -> choice<int> { return {t.v}; };
  static_assert(noexcept(std::declval<T &>().and_then(fnChoice)));
  static_assert(noexcept(std::declval<T const &>().and_then(fnChoice)));
  static_assert(noexcept(std::declval<T &&>().and_then(fnChoice)));
  static_assert(noexcept(std::declval<T const &&>().and_then(fnChoice)));

  constexpr auto fnInt = [](Throwing const &t) noexcept(false) -> int { return t.v; };
  static_assert(noexcept(std::declval<T &>().transform(fnInt)));
  static_assert(noexcept(std::declval<T const &>().transform(fnInt)));
  static_assert(noexcept(std::declval<T &>().invoke(fnInt)));
  static_assert(noexcept(std::declval<T &>().template invoke_r<long>(fnInt)));

  // The constructors that widen a sum into a choice carry it too - each copies or moves every
  // alternative of the source across.
  using W = fn::choice_for<Throwing, int>;
  static_assert(noexcept(W{std::declval<fn::sum<Throwing> const &>()}));
  static_assert(noexcept(W{std::declval<fn::sum<Throwing> &&>()}));

  // The value constructors, as in sum, carry no noexcept specifier at all - the converse
  // under-promise, reported potentially-throwing even where nothing can throw.
  static_assert(not noexcept(choice<int>{42}));

  // Inherited from sum, and accurate: reading the discriminator touches no alternative.
  static_assert(noexcept(std::declval<T const &>().has_value(std::in_place_type<Throwing>)));
  static_assert(noexcept(std::declval<T &>().get_ptr(std::in_place_type<Throwing>)));

  SUCCEED();
}

TEST_CASE("choice and_then", "[choice][and_then]")
{
  using type = fn::choice<bool, int>;
  constexpr auto init = std::in_place_type<int>;

  type s{init, 12};
  CHECK(s.and_then(                                                 //
            fn::overload{[](bool) -> fn::choice<bool> { throw 1; }, //
                         [](int &i) -> fn::choice<bool> {           //
                           return fn::choice<bool>{i == 12};
                         },
                         [](int const &) -> fn::choice<bool> { throw 0; },   //
                         [](int &&) -> fn::choice<bool> { throw 0; },        //
                         [](int const &&) -> fn::choice<bool> { throw 0; }}) //
        == fn::choice{true});
  CHECK(std::as_const(s).and_then(                                   //
            fn::overload{[](bool) -> fn::choice<bool> { throw 1; },  //
                         [](int &) -> fn::choice<bool> { throw 0; }, //
                         [](int const &i) -> fn::choice<bool> {      //
                           return {i == 12};
                         },
                         [](int &&) -> fn::choice<bool> { throw 0; },        //
                         [](int const &&) -> fn::choice<bool> { throw 0; }}) //
        == fn::choice{true});
  CHECK(type{init, 12}.and_then(                                           //
            fn::overload{[](bool) -> fn::choice<bool> { throw 1; },        //
                         [](int &) -> fn::choice<bool> { throw 0; },       //
                         [](int const &) -> fn::choice<bool> { throw 0; }, //
                         [](int &&i) -> fn::choice<bool> {                 //
                           return {i == 12};
                         },
                         [](int const &&) -> fn::choice<bool> { throw 0; }})
        == fn::choice{true});
  CHECK(std::move(std::as_const(s))
            .and_then(                                                         //
                fn::overload{[](bool) -> fn::choice<bool> { throw 1; },        //
                             [](int &) -> fn::choice<bool> { throw 0; },       //
                             [](int const &) -> fn::choice<bool> { throw 0; }, //
                             [](int &&) -> fn::choice<bool> { throw 0; },      //
                             [](int const &&i) -> fn::choice<bool> { return {i == 12}; }})
        == fn::choice{true});

  constexpr type a{std::in_place_type<int>, 42};
  constexpr auto fn = fn::overload{[](bool) -> fn::choice<bool> { throw 1; },  //
                                   [](int &) -> fn::choice<bool> { throw 0; }, //
                                   [](int const &i) -> fn::choice<bool> {      //
                                     return {i == 42};
                                   },
                                   [](int &&) -> fn::choice<bool> { throw 0; }, //
                                   [](int const &&) -> fn::choice<bool> { throw 0; }};
  static_assert(std::is_same_v<fn::choice<bool>, decltype(a.and_then(fn))>);
  static_assert(a.and_then(fn) == fn::choice{true});
  static_assert(std::move(a).and_then(                                             //
                    fn::overload{[](bool) -> fn::choice<bool> { throw 1; },        //
                                 [](int &) -> fn::choice<bool> { throw 0; },       //
                                 [](int const &) -> fn::choice<bool> { throw 0; }, //
                                 [](int &&) -> fn::choice<bool> { throw 0; },      //
                                 [](int const &&i) -> fn::choice<bool> { return {i == 42}; }})
                == fn::choice{true});
}

TEST_CASE("choice transform", "[choice][transform]")
{
  SECTION("size 2, only one set")
  {
    using type = fn::choice<bool, int>;
    constexpr auto init = std::in_place_type<int>;
    type s{init, 12};
    CHECK(s.transform(                                      //
              fn::overload{[](bool) -> double { throw 1; }, //
                           [](int &i) -> double {           //
                             return i / 8.0;
                           },
                           [](int const &) -> double { throw 0; },   //
                           [](int &&) -> double { throw 0; },        //
                           [](int const &&) -> double { throw 0; }}) //
          == fn::choice<double>{1.5});
    CHECK(std::as_const(s).transform(                        //
              fn::overload{[](bool) -> double { throw 1; },  //
                           [](int &) -> double { throw 0; }, //
                           [](int const &i) -> double {      //
                             return i / 8.0;
                           },
                           [](int &&) -> double { throw 0; },        //
                           [](int const &&) -> double { throw 0; }}) //
          == fn::choice<double>{1.5});
    CHECK(type{init, 12}.transform(                                //
              fn::overload{[](bool) -> double { throw 1; },        //
                           [](int &) -> double { throw 0; },       //
                           [](int const &) -> double { throw 0; }, //
                           [](int &&i) -> double {                 //
                             return i / 8.0;
                           },
                           [](int const &&) -> double { throw 0; }})
          == fn::choice<double>{1.5});
    CHECK(std::move(std::as_const(s))
              .transform(                                              //
                  fn::overload{[](bool) -> double { throw 1; },        //
                               [](int &) -> double { throw 0; },       //
                               [](int const &) -> double { throw 0; }, //
                               [](int &&) -> double { throw 0; },      //
                               [](int const &&i) -> double { return i / 8.0; }})
          == fn::choice<double>{1.5});

    constexpr type a{std::in_place_type<int>, 42};
    constexpr auto fn = fn::overload{[](bool) -> fn::sum<bool> { throw 1; },    //
                                     [](int &) -> fn::sum<double> { throw 0; }, //
                                     [](int const &i) -> fn::sum<double> {      //
                                       return i / 8.0;
                                     },
                                     [](int &&) -> fn::sum<double> { throw 0; }, //
                                     [](int const &&) -> fn::sum<double> { throw 0; }};
    static_assert(std::is_same_v<fn::choice<bool, double>, decltype(a.transform(fn))>);
    static_assert(a.transform(fn) == fn::choice<bool, double>{5.25});
    static_assert(std::move(a).transform(                                  //
                      fn::overload{[](bool) -> double { throw 1; },        //
                                   [](int &) -> double { throw 0; },       //
                                   [](int const &) -> double { throw 0; }, //
                                   [](int &&) -> double { throw 0; },      //
                                   [](int const &&i) -> double { return i / 8.0; }})
                  == fn::choice<double>{5.25});
  }

  SECTION("size 2")
  {
    using ::fn::choice;
    using ::fn::sum;
    constexpr auto fn1 = [](auto i) noexcept -> std::size_t { return sizeof(i); };

    using type = choice<double, int>;
    static_assert(type::size == 2);

    SECTION("element v0 set")
    {
      type a{std::in_place_type<double>, 0.5};
      CHECK(a.data.v0 == 0.5);
      SECTION("value only")
      {
        static_assert(type{0.5}.transform(fn1) == choice{std::size_t{8}});
        CHECK(a.transform(     //
                  fn::overload{//
                               [](auto) -> int { throw 1; }, [](double &i) -> bool { return i == 0.5; },
                               [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                               [](double const &&) -> bool { throw 0; }})
              == choice<bool, int>{true});
        CHECK(std::as_const(a).transform( //
                  fn::overload{           //
                               [](auto) -> int { throw 1; }, [](double &) -> bool { throw 0; },
                               [](double const &i) -> bool { return i == 0.5; }, [](double &&) -> bool { throw 0; },
                               [](double const &&) -> bool { throw 0; }})
              == choice<bool, int>{true});
        CHECK(type{std::in_place_type<double>, 0.5}.transform( //
                  fn::overload{                                //
                               [](auto) -> int { throw 1; }, [](double &) -> bool { throw 0; },
                               [](double const &) -> bool { throw 0; }, [](double &&i) -> bool { return i == 0.5; },
                               [](double const &&) -> bool { throw 0; }})
              == choice<bool, int>{true});
        CHECK(std::move(std::as_const(a))
                  .transform(      //
                      fn::overload{//
                                   [](auto) -> int { throw 1; }, [](double &) -> bool { throw 0; },
                                   [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                                   [](double const &&i) -> bool { return i == 0.5; }})
              == choice<bool, int>{true});
      }
    }

    SECTION("element v1 set")
    {
      type a{std::in_place_type<int>, 42};
      CHECK(a.data.v1 == 42);

      SECTION("value only")
      {
        static_assert(type{42}.transform(fn1) == choice{std::size_t{4}});
        CHECK(a.transform(     //
                  fn::overload{//
                               [](auto) -> bool { throw 1; }, [](int &i) -> bool { return i == 42; },
                               [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                               [](int const &&) -> bool { throw 0; }})
              == choice{true});
        CHECK(std::as_const(a).transform( //
                  fn::overload{           //
                               [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                               [](int const &i) -> bool { return i == 42; }, [](int &&) -> bool { throw 0; },
                               [](int const &&) -> bool { throw 0; }})
              == choice{true});
        CHECK(choice<int>{std::in_place_type<int>, 42}.transform( //
                  fn::overload{                                   //
                               [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                               [](int const &) -> bool { throw 0; }, [](int &&i) -> bool { return i == 42; },
                               [](int const &&) -> bool { throw 0; }})
              == choice{true});
        CHECK(std::move(std::as_const(a))
                  .transform(      //
                      fn::overload{//
                                   [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                   [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                   [](int const &&i) -> bool { return i == 42; }})
              == choice{true});
      }
    }
  }
}
