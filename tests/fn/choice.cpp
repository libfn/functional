// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/choice.hpp>

#include <fn/utility.hpp>

#include <util/helper_types.hpp>

#include <catch2/catch_all.hpp>

#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include <fn/detail/macro_begin.hpp>

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

// Every operation choice performs on an alternative - copy, move, compare - can throw here, so the
// member wrapping it must say so. Witnesses the conditional noexcept below.
struct Throwing final {
  int v;

  constexpr operator int() const { return v; }
  constexpr Throwing(int i) noexcept : v(i) {}
  constexpr Throwing(Throwing const &o) noexcept(false) : v(o.v) {}
  constexpr Throwing(Throwing &&o) noexcept(false) : v(o.v) {}
  constexpr bool operator==(Throwing const &o) const noexcept(false) { return v == o.v; }
};

template <typename S, typename T, typename... Args>
concept can_in_place = requires(Args... args) { S{std::in_place_type<T>, args...}; };

template <typename S, typename T, typename... Args>
concept can_emplace = requires(S &s, Args &&...args) { s.template emplace<T>(static_cast<Args &&>(args)...); };

template <typename S, typename Fn>
concept can_transform = requires(S s, Fn fn) { FWD(s).transform(fn); };

template <typename S, typename Fn>
concept can_and_then = requires(S s, Fn fn) { FWD(s).and_then(fn); };

template <typename S, typename Fn, typename... Args>
concept can_apply_type = requires(S s, Fn fn, Args... args) { FWD(s).apply_type(FWD(fn), FWD(args)...); };

template <typename S, typename Fn, typename... Args>
concept can_apply = requires(S s, Fn fn, Args... args) { FWD(s).apply(FWD(fn), FWD(args)...); };

// Every special member of choice is defaulted, and choice adds no state to copack - so each must behave
// exactly as copack's, down to its noexcept and its constraints.
template <typename... Ts> consteval bool special_members_follow_copack()
{
  using C = fn::choice<Ts...>;
  using S = fn::copack<Ts...>;
  return std::is_copy_constructible_v<C> == std::is_copy_constructible_v<S>                        //
         && std::is_nothrow_copy_constructible_v<C> == std::is_nothrow_copy_constructible_v<S>     //
         && std::is_move_constructible_v<C> == std::is_move_constructible_v<S>                     //
         && std::is_nothrow_move_constructible_v<C> == std::is_nothrow_move_constructible_v<S>     //
         && std::is_copy_assignable_v<C> == std::is_copy_assignable_v<S>                           //
         && std::is_nothrow_copy_assignable_v<C> == std::is_nothrow_copy_assignable_v<S>           //
         && std::is_move_assignable_v<C> == std::is_move_assignable_v<S>                           //
         && std::is_nothrow_move_assignable_v<C> == std::is_nothrow_move_assignable_v<S>           //
         && std::is_destructible_v<C> == std::is_destructible_v<S>                                 //
         && std::is_nothrow_destructible_v<C> == std::is_nothrow_destructible_v<S>                 //
         && std::is_trivially_destructible_v<C> == std::is_trivially_destructible_v<S>             //
         && std::is_trivially_copy_constructible_v<C> == std::is_trivially_copy_constructible_v<S> //
         && std::is_trivially_move_constructible_v<C> == std::is_trivially_move_constructible_v<S> //
         && std::is_trivially_copy_assignable_v<C> == std::is_trivially_copy_assignable_v<S>       //
         && std::is_trivially_move_assignable_v<C> == std::is_trivially_move_assignable_v<S>;
}

} // anonymous namespace

TEST_CASE("choice non-monadic functionality", "[choice]")
{
  // NOTE This test looks very similar to test in copack.cpp - for good reason.

  using fn::choice;

  SECTION("value")
  {
    // _for aliases: a choice's value_type is the copack of its alternatives, but the canonical order is
    // platform-specific (see SECTION("choice_for") below), so normalize both sides per platform.
    static_assert(
        std::same_as<fn::copack_for<NonCopyable, int>, typename fn::choice_for<NonCopyable, int>::value_type>);
    static_assert(std::same_as<fn::copack<int>, typename choice<int>::value_type>);

    using type = fn::choice<bool, helper>;
    using value_type = fn::copack<bool, helper>;
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
    static_assert(std::same_as<int, decltype(s.value().apply(fn))>);
    CHECK((s.value().apply(fn)) == 43);
    CHECK((std::as_const(s).value().apply(fn)) == 44);
    CHECK((std::move(std::as_const(s)).value().apply(fn)) == 46);
    CHECK((std::move(s).value().apply(fn)) == 45);
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
    // std::type_order — an ABI-tied total order); see tests/fn/copack.cpp for the full rationale. This
    // one assert documents the divergence; the rest assert only platform-independent invariants.
    // Spelling: choices/copacks whose alternatives include a non-builtin have platform-specific order, so
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

    static_assert(std::same_as<fn::choice_for<int, fn::copack<int>>, fn::choice<int>>);
    static_assert(std::same_as<fn::choice_for<int, fn::copack<bool>>, fn::choice<bool, int>>);
    static_assert(std::same_as<fn::choice_for<int, fn::copack<bool, int>>, fn::choice<bool, int>>);
    static_assert(std::same_as<fn::choice_for<int, fn::copack<bool, double>>, fn::choice<bool, double, int>>);

    static_assert(std::same_as<fn::choice_for<fn::copack<bool>, fn::copack<int>>, fn::choice<bool, int>>);
    static_assert(
        std::same_as<fn::choice_for<fn::copack<bool>, fn::copack<bool, double, int>>, fn::choice<bool, double, int>>);
    static_assert(
        std::same_as<fn::choice_for<fn::copack<bool>, fn::copack<double, int>>, fn::choice<bool, double, int>>);
    static_assert(std::same_as<fn::choice_for<fn::copack<bool, int>, double>, fn::choice<bool, double, int>>);

    static_assert(std::same_as<fn::choice_for<int, fn::copack<>>, fn::choice<int>>);
    static_assert(std::same_as<fn::choice_for<fn::copack<>, int>, fn::choice<int>>);
    static_assert(std::same_as<fn::choice_for<fn::copack<>, fn::copack<bool, int>>, fn::choice<bool, int>>);
    static_assert(
        std::same_as<fn::choice_for<double, fn::copack<>, fn::copack<bool, int>>, fn::choice<bool, double, int>>);
  }

  SECTION("applicable")
  {
    using type = fn::choice_for<TestType, int>; // choice<...> order is platform-specific; choice_for normalizes
    static_assert(fn::typelist_applicable<decltype([](auto) {}), type &>);
    static_assert(fn::typelist_applicable<decltype([](auto &) {}), type &>);
    static_assert(fn::typelist_applicable<decltype([](auto const &) {}), type &>);
    static_assert(fn::typelist_applicable<decltype(fn::overload{[](int &) {}, [](TestType &) {}}), type &>);
    static_assert(fn::typelist_applicable<decltype(fn::overload{[](int) {}, [](TestType) {}}), type const &>);

    static_assert(not fn::typelist_applicable<decltype([](TestType &) {}), type &>); // missing int
    static_assert(not fn::typelist_applicable<decltype([](int &) {}), type &>);      // missing TestType
    static_assert(not fn::typelist_applicable<decltype(fn::overload{[](int &&) {}, [](TestType &&) {}}),
                                              type &>); // cannot bind lvalue to rvalue-reference
    static_assert(not fn::typelist_applicable<decltype([](auto &) {}),
                                              type &&>); // cannot bind rvalue to lvalue-reference
    static_assert(not fn::typelist_applicable<decltype([](auto, auto &) {}), type &>); // bad arity
    static_assert(not fn::typelist_applicable<decltype(fn::overload{[](int &) {}, [](TestType &) {}}),
                                              type const &>); // cannot bind const to non-const reference

    static_assert(fn::typelist_applicable<decltype([](auto &) {}), choice<NonCopyable> &>);
    static_assert(not fn::typelist_applicable<decltype([](auto) {}), NonCopyable &>); // copy-constructor not available
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
      static_assert(c.apply([](auto &&a) -> bool { return a.size() == 3 && a[0] == 3 && a[1] == 14 && a[2] == 15; }));
    }

    SECTION("constexpr move from rvalue")
    {
      using T = fn::choice<bool, helper>;
      constexpr auto fn = [](auto i) constexpr noexcept -> T { return {std::move(i)}; };
      constexpr auto a = fn(true);
      static_assert(std::is_same_v<decltype(a), T const>);
      static_assert(a.has_value<bool>());
      static_assert(a.value() == fn::copack{true});

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
      CHECK(b.value() == fn::copack{true});
    }

    SECTION("constexpr move from const rvalue")
    {
      using T = fn::choice<bool, helper>;
      constexpr auto fn = [](auto const i) constexpr noexcept -> T { return {std::move(i)}; };
      constexpr auto a = fn(true);
      static_assert(std::is_same_v<decltype(a), T const>);
      static_assert(a.has_value<bool>());
      static_assert(a.value() == fn::copack{true});

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
      CHECK(b.value() == fn::copack{true});
    }

    SECTION("constexpr copy from lvalue")
    {
      using T = fn::choice<bool, helper>;
      constexpr auto fn = [](auto i) constexpr noexcept -> T { return {i}; };
      constexpr auto a = fn(true);
      static_assert(std::is_same_v<decltype(a), T const>);
      static_assert(a.has_value<bool>());
      static_assert(a.value() == fn::copack{true});

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
      CHECK(a.value() == fn::copack{true});

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
      static_assert(a.value() == fn::copack{true});

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
      CHECK(a.value() == fn::copack{true});

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

  SECTION("constructor from copack")
  {
    using T = fn::choice<bool, helper>;
    SECTION("move from rvalue")
    {
      fn::copack h{helper{1}};
      h.get_ptr<helper>()->v = 17;
      T const a{std::move(h)};
      CHECK(a.value().get_ptr<helper>()->v == 17 * from_rval);
    }
    SECTION("copy from const lvalue")
    {
      fn::copack h{helper{1}};
      h.get_ptr<helper>()->v = 19;
      T const a{std::as_const(h)};
      CHECK(a.value().get_ptr<helper>()->v == 19 * from_lval_const);
    }
  }

  SECTION("assignment from copack")
  {
    // choice declares its operator=, hiding copack's widening overloads, so it carries its own pair
    // and delegates - which also admits a copack over the same alternatives, with nothing to widen
    constexpr auto battery = [] {
      choice<bool, int> a{12};
      a = fn::copack<int>{42}; // the alternative in hand
      bool ok = a == choice{42};
      a = fn::copack<bool>{true}; // a different alternative
      ok = ok && a == choice{true};
      fn::copack<int> const n{7};
      a = n; // by copy
      ok = ok && a == choice{7};
      a = choice<int>{3}; // a narrower choice, deduced through its copack base
      ok = ok && a == choice{3};
      a = fn::copack<bool, int>{5}; // the same alternatives, delegated to same-type assignment
      return ok && a == choice{5};
    };
    CHECK(battery());
    static_assert(battery());

    SECTION("constraints")
    {
      static_assert(std::is_assignable_v<choice<bool, int> &, fn::copack<int> const &>);
      static_assert(std::is_assignable_v<choice<bool, int> &, fn::copack<bool, int> &&>); // same alternatives
      static_assert(std::is_assignable_v<choice<bool, int> &, choice<int> const &>);      // through the base
      static_assert(not std::is_assignable_v<choice<int> &, fn::copack<bool> const &>);   // not a superset
      static_assert(noexcept(std::declval<choice<bool, int> &>() = std::declval<fn::copack<int> const &>()));
      SUCCEED();
    }
  }

  SECTION("assignment from a value")
  {
    // the delegating value assignment, restated for the same name-hiding reason as the widening
    // pair above; the answer - routing, constraints, noexcept - is copack's
    constexpr auto battery = [] {
      choice<bool, int> a{12};
      a = 42; // the alternative in hand
      bool ok = a == choice{42};
      a = true; // a different alternative
      ok = ok && a == choice{true};
      int const i = 7;
      a = i; // by copy
      return ok && a == choice{7};
    };
    CHECK(battery());
    static_assert(battery());

    SECTION("constraints")
    {
      static_assert(std::is_assignable_v<choice<bool, int> &, int>);
      static_assert(not std::is_assignable_v<choice<bool, int> &, short>); // exact alternative only
      static_assert(std::is_nothrow_assignable_v<choice<bool, int> &, int>);

      // copack- and choice-typed sources are left to the assignments above, including non-const
      // lvalues, which a forwarding reference would otherwise capture
      static_assert(std::is_assignable_v<choice<bool, int> &, choice<bool, int> &>);
      static_assert(std::is_assignable_v<choice<bool, int> &, choice<int> &>);
      static_assert(std::is_assignable_v<choice<bool, int> &, fn::copack<int> &>);

      // a sibling's refusal does not hold the value assignment hostage
      struct Fixed final { // copyable, not assignable
        constexpr Fixed() noexcept = default;
        constexpr Fixed(Fixed const &) noexcept = default;
        constexpr Fixed(Fixed &&) noexcept = default;
        Fixed &operator=(Fixed const &) = delete;
        Fixed &operator=(Fixed &&) = delete;
      };
      using C = fn::choice_for<Fixed, int>;
      static_assert(not std::is_copy_assignable_v<C>);
      static_assert(std::is_assignable_v<C &, int>);
      SUCCEED();
    }
  }

  SECTION("forwarding constructors (immovable)")
  {
    choice<NonCopyable> a{std::in_place_type<NonCopyable>, 42};
    CHECK(a.apply([](auto &i) -> bool { return i.i == 42; }));

    SECTION("CTAD")
    {
      constexpr auto a = choice{std::in_place_type<NonCopyable>, 42};
      static_assert(std::is_same_v<decltype(a), choice<NonCopyable> const>);

      auto b = choice{std::in_place_type<NonCopyable>, 42};
      static_assert(std::is_same_v<decltype(b), choice<NonCopyable>>);
    }

    SECTION("constraints")
    {
      static_assert(can_in_place<choice<NonCopyable>, NonCopyable, int>);
      static_assert(not can_in_place<choice<NonCopyable>, int, int>); // int is not an alternative

      // the constructor forwards to copack's, and rejects what copack rejects
      static_assert(not can_in_place<choice<NonCopyable>, NonCopyable>);               // no default ctor
      static_assert(not can_in_place<choice<NonCopyable>, NonCopyable, char const *>); // not constructible from
      SUCCEED();
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
      CHECK(a.apply([](auto &i) -> bool {
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
      static_assert(a.apply([](auto &i) -> bool {
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
    // The comparison operators are copack's - free functions taking copack<Ts...> const & - and copack.cpp
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

    // the alternative held is absent from the other choice's list, so there is nothing to compare
    // and the answer is false whatever it holds
    constexpr type b{std::in_place_type<bool>, true};
    static_assert(not(b == choice<double, int>{42}));
    static_assert(b != choice<double, int>{42});

    CHECK(a == type{42});
    CHECK(a != type{41});
    CHECK(a == choice<double, int>{42});
    CHECK(a != choice<double, int>{41});
    CHECK(not(b == choice<double, int>{42}));
    CHECK(b != choice<double, int>{42});
  }

  SECTION("apply")
  {
    choice<int> a{std::in_place_type<int>, 42};
    SECTION("value only")
    {
      CHECK(a.apply(   //
          fn::overload{//
                       [](auto) -> bool { throw 1; }, [](int &) -> bool { return true; },
                       [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                       [](int const &&) -> bool { throw 0; }}));
      CHECK(std::as_const(a).apply( //
          fn::overload{             //
                       [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                       [](int const &) -> bool { return true; }, [](int &&) -> bool { throw 0; },
                       [](int const &&) -> bool { throw 0; }}));
      CHECK(std::move(std::as_const(a))
                .apply(fn::overload{[](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                    [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                    [](int const &&) -> bool { return true; }}));
      CHECK(std::move(a).apply( //
          fn::overload{         //
                       [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                       [](int const &) -> bool { throw 0; }, [](int &&) -> bool { return true; },
                       [](int const &&) -> bool { throw 0; }}));

      SECTION("constexpr")
      {
        constexpr choice<int> a{std::in_place_type<int>, 42};
        static_assert(a.apply(fn::overload{
            [](auto) -> std::false_type { return {}; }, //
            [](int &) -> std::false_type { return {}; }, [](int const &) -> std::true_type { return {}; },
            [](int &&) -> std::false_type { return {}; }, [](int const &&) -> std::false_type { return {}; }}));
        static_assert(std::move(a).apply(fn::overload{
            [](auto) -> std::false_type { return {}; }, //
            [](int &) -> std::false_type { return {}; }, [](int const &) -> std::false_type { return {}; },
            [](int &&) -> std::false_type { return {}; }, [](int const &&) -> std::true_type { return {}; }}));
      }
    }

    SECTION("tuple-like alternative")
    {
      // the tuple-like arm of fn::apply reaches choice's dispatch identically to copack's
      constexpr auto add2 = [](int i, int j) noexcept -> int { return i + j; };
      CHECK(choice<std::tuple<int, int>>{std::tuple{2, 3}}.apply(add2) == 5);

      SECTION("constexpr")
      {
        static_assert(choice<std::tuple<int, int>>{std::tuple{2, 3}}.apply(add2) == 5);
        SUCCEED();
      }
    }
  }

  SECTION("apply_r")
  {
    choice<int> a{std::in_place_type<int>, 42};
    SECTION("value only")
    {
      CHECK(a.apply_r<int>( //
          fn::overload{     //
                       [](auto) -> int { throw 1; }, [](int &) -> bool { return true; },
                       [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                       [](int const &&) -> bool { throw 0; }}));
      CHECK(std::as_const(a).apply_r<int>( //
          fn::overload{                    //
                       [](auto) -> int { throw 1; }, [](int &) -> bool { throw 0; },
                       [](int const &) -> bool { return true; }, [](int &&) -> bool { throw 0; },
                       [](int const &&) -> bool { throw 0; }}));
      CHECK(std::move(std::as_const(a))
                .apply_r<int>(fn::overload{[](auto) -> int { throw 1; }, [](int &) -> bool { throw 0; },
                                           [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                           [](int const &&) -> bool { return true; }}));
      CHECK(std::move(a).apply_r<int>( //
          fn::overload{                //
                       [](auto) -> int { throw 1; }, [](int &) -> bool { throw 0; },
                       [](int const &) -> bool { throw 0; }, [](int &&) -> bool { return true; },
                       [](int const &&) -> bool { throw 0; }}));

      SECTION("constexpr")
      {
        constexpr choice<int> a{std::in_place_type<int>, 42};
        static_assert(a.apply_r<int>(fn::overload{
            [](auto) -> std::false_type { return {}; }, //
            [](int &) -> std::false_type { return {}; }, [](int const &) -> std::true_type { return {}; },
            [](int &&) -> std::false_type { return {}; }, [](int const &&) -> std::false_type { return {}; }}));
        static_assert(std::move(a).apply_r<int>(fn::overload{
            [](auto) -> std::false_type { return {}; }, //
            [](int &) -> std::false_type { return {}; }, [](int const &) -> std::false_type { return {}; },
            [](int &&) -> std::false_type { return {}; }, [](int const &&) -> std::true_type { return {}; }}));
      }
    }
  }
}

TEST_CASE("choice apply_type", "[choice][apply_type]")
{
  using fn::choice;
  using std::in_place_type_t;

  constexpr auto arms
      = fn::overload{[](in_place_type_t<int>, int v) noexcept -> int { return v; },
                     [](in_place_type_t<double>, double d) noexcept -> int { return static_cast<int>(d) + 1000; }};
  choice<double, int> a{42};

  SECTION("airtight, all value categories")
  {
    CHECK(a.apply_type(arms) == 42);
    CHECK(std::as_const(a).apply_type(arms) == 42);
    CHECK(std::move(std::as_const(a)).apply_type(arms) == 42);
    CHECK(std::move(a).apply_type(arms) == 42);
    CHECK(choice<double, int>{0.5}.apply_type(arms) == 1000);

    constexpr auto only_double = fn::overload{[](in_place_type_t<double>, double) noexcept -> int { return 0; }};
    static_assert(not can_apply_type<choice<double, int> &, decltype(only_double) const &>);
    static_assert(can_apply_type<choice<double, int> &, decltype(arms) const &>);

    SECTION("constexpr")
    {
      static_assert(choice<double, int>{42}.apply_type(arms) == 42);
      static_assert(choice<double, int>{0.5}.apply_type(arms) == 1000);
      SUCCEED();
    }
  }

  SECTION("tuple-like alternative")
  {
    using T = std::tuple<int, int>;
    constexpr auto tarms = fn::overload{[](in_place_type_t<T>, int x, int y) noexcept -> int { return x + y; }};
    CHECK(choice<T>{T{20, 22}}.apply_type(tarms) == 42);

    SECTION("constexpr")
    {
      static_assert(choice<T>{T{20, 22}}.apply_type(tarms) == 42);
      SUCCEED();
    }
  }

  SECTION("apply_type_r")
  {
    static_assert(std::is_same_v<long, decltype(a.apply_type_r<long>(arms))>);
    CHECK(a.apply_type_r<long>(arms) == 42L);

    SECTION("constexpr")
    {
      static_assert(choice<double, int>{42}.apply_type_r<long>(arms) == 42L);
      SUCCEED();
    }
  }

  SECTION("extra arguments")
  {
    // trailing arguments follow the alternative's unpacked content, as on apply
    constexpr auto xarms = fn::overload{[](in_place_type_t<int>, int v, int x) noexcept -> int { return v + x; },
                                        [](in_place_type_t<double>, double, int x) noexcept -> int { return -x; }};
    CHECK(a.apply_type(xarms, 2) == 44);
    CHECK(a.apply_type_r<long>(xarms, 2) == 44L);
    static_assert(noexcept(a.apply_type(xarms, 2)));

    // an arm set that does not take the extra answers non-viable
    static_assert(not can_apply_type<choice<double, int> &, decltype(arms) const &, int>);
    static_assert(can_apply_type<choice<double, int> &, decltype(xarms) const &, int>);

    SECTION("constexpr")
    {
      static_assert(choice<double, int>{42}.apply_type(xarms, 2) == 44);
      SUCCEED();
    }
  }

  SECTION("noexcept")
  {
    static_assert(noexcept(a.apply_type(arms)));
    constexpr auto throwing = fn::overload{[](in_place_type_t<int>, int v) noexcept(false) -> int { return v; },
                                           [](in_place_type_t<double>, double) noexcept -> int { return 0; }};
    static_assert(not noexcept(a.apply_type(throwing)));
    SUCCEED();
  }
}

TEST_CASE("choice apply", "[choice][apply]")
{
  using fn::choice;

  // the value-path members share copack::apply's shape: trailing arguments follow the alternative
  constexpr auto arms = fn::overload{[](int v, int x) noexcept -> int { return v + x; },
                                     [](double, int x) noexcept -> int { return -x; }};
  choice<double, int> a{42};

  SECTION("extra arguments")
  {
    CHECK(a.apply(arms, 2) == 44);
    CHECK(a.apply_r<long>(arms, 2) == 44L);
    static_assert(noexcept(a.apply(arms, 2)));
    constexpr auto noextra
        = fn::overload{[](int v) noexcept -> int { return v; }, [](double) noexcept -> int { return 0; }};
    static_assert(not can_apply<choice<double, int> &, decltype(noextra) const &, int>);
    static_assert(can_apply<choice<double, int> &, decltype(noextra) const &>);
    CHECK(a.apply(noextra) == 42);

    SECTION("constexpr")
    {
      static_assert(choice<double, int>{42}.apply(arms, 2) == 44);
      SUCCEED();
    }
  }
}

TEST_CASE("choice noexcept", "[choice][noexcept]")
{
  using fn::choice;
  using T = choice<Throwing>;
  using C = choice<int>;

  static_assert(not std::is_nothrow_copy_constructible_v<Throwing>);
  static_assert(not std::is_nothrow_move_constructible_v<Throwing>);

  // the copy and move constructors weigh the alternative they relocate
  static_assert(not std::is_nothrow_copy_constructible_v<T>);
  static_assert(not std::is_nothrow_move_constructible_v<T>);
  static_assert(std::is_nothrow_copy_constructible_v<C>);
  static_assert(std::is_nothrow_move_constructible_v<C>);

  // the same monadic operation must carry the same exception promise whichever monad it is written
  // against - choice's answers are pinned here to match optional's and expected's
  constexpr auto nothrow_fn = [](auto i) noexcept -> choice<int> { return {i}; };
  constexpr auto throwing_fn = [](auto i) -> choice<int> { return {i}; };

  static_assert(noexcept(std::declval<C &>().and_then(nothrow_fn)));
  static_assert(not noexcept(std::declval<C &>().and_then(throwing_fn)));
  static_assert(not noexcept(std::declval<C &&>().and_then(throwing_fn)));

  constexpr auto nothrow_t = [](auto) noexcept { return 0; };
  constexpr auto throwing_t = [](auto) { return 0; };
  static_assert(noexcept(std::declval<C &>().transform(nothrow_t)));
  static_assert(not noexcept(std::declval<C &>().transform(throwing_t)));
  static_assert(noexcept(std::declval<C &>().apply(nothrow_t)));
  static_assert(not noexcept(std::declval<C &>().apply(throwing_t)));

  // a throwing callback is weighed in every value category
  constexpr auto fnChoice = [](Throwing const &t) noexcept(false) -> choice<int> { return {t.v}; };
  static_assert(not noexcept(std::declval<T &>().and_then(fnChoice)));
  static_assert(not noexcept(std::declval<T const &>().and_then(fnChoice)));
  static_assert(not noexcept(std::declval<T &&>().and_then(fnChoice)));
  static_assert(not noexcept(std::declval<T const &&>().and_then(fnChoice)));

  constexpr auto fnInt = [](Throwing const &t) noexcept(false) -> int { return t.v; };
  static_assert(not noexcept(std::declval<T &>().transform(fnInt)));
  static_assert(not noexcept(std::declval<T const &>().transform(fnInt)));
  static_assert(not noexcept(std::declval<T &>().apply(fnInt)));
  static_assert(not noexcept(std::declval<T &>().template apply_r<long>(fnInt)));

  // the constructors that widen a copack into a choice copy or move every alternative across
  using W = fn::choice_for<Throwing, int>;
  static_assert(not noexcept(W{std::declval<fn::copack<Throwing> const &>()}));
  static_assert(not noexcept(W{std::declval<fn::copack<Throwing> &&>()}));

  // the value constructors weigh what they construct, rather than under-promising as they once did
  static_assert(noexcept(choice<int>{42}));

  // reading the discriminator touches no alternative
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

  SECTION("superset join")
  {
    struct A final {
      bool operator==(A const &) const = default;
    };
    struct B final {
      bool operator==(B const &) const = default;
    };
    struct U final {
      bool operator==(U const &) const = default;
    };
    struct V final {
      bool operator==(V const &) const = default;
    };
    struct W final {
      bool operator==(W const &) const = default;
    };
    using J = fn::choice_for<A, B>;

    // divergent branch choices join into the normalized superset choice
    constexpr auto fnJoin = fn::overload{[](A) { return fn::choice<U>{U{}}; }, //
                                         [](B) { return fn::choice_for<V, W>{V{}}; }};
    static_assert(std::is_same_v<decltype(J{A{}}.and_then(fnJoin)), fn::choice_for<U, V, W>>);
    CHECK(J{A{}}.and_then(fnJoin) == fn::choice_for<U, V, W>{U{}});
    CHECK(J{B{}}.and_then(fnJoin) == fn::choice_for<U, V, W>{V{}});

    // ... with overlapping alternatives deduplicated
    constexpr auto fnOverlap = fn::overload{[](A) { return fn::choice_for<U, V>{U{}}; }, //
                                            [](B) { return fn::choice_for<V, W>{W{}}; }};
    static_assert(std::is_same_v<decltype(J{A{}}.and_then(fnOverlap)), fn::choice_for<U, V, W>>);
    CHECK(J{A{}}.and_then(fnOverlap) == fn::choice_for<U, V, W>{U{}});
    CHECK(J{B{}}.and_then(fnOverlap) == fn::choice_for<U, V, W>{W{}});

    // constexpr twins dispatch from named sources: VS 2022's MSVC (observed through 19.44)
    // misreads the union's empty-class member as uninitialized when the source is a prvalue
    // materialized mid-expression - independent of the join, apply's select path trips the same way
    constexpr J ca{A{}};
    constexpr J cb{B{}};
    static_assert(ca.and_then(fnJoin) == fn::choice_for<U, V, W>{U{}});
    static_assert(std::move(cb).and_then(fnOverlap) == fn::choice_for<U, V, W>{W{}});

    // noexcept: the callback and the widening arms both weigh in
    constexpr auto fnNothrow = fn::overload{[](A) noexcept { return fn::choice<U>{U{}}; }, //
                                            [](B) noexcept { return fn::choice_for<V, W>{V{}}; }};
    J j{A{}};
    static_assert(noexcept(j.and_then(fnNothrow)));
    static_assert(noexcept(std::as_const(j).and_then(fnNothrow)));
    static_assert(noexcept(std::move(j).and_then(fnNothrow)));
    static_assert(not noexcept(j.and_then(fnJoin))); // fnJoin's branches may throw
    struct ThrowingMove final {
      ThrowingMove() = default;
      ThrowingMove(ThrowingMove &&) noexcept(false) {}
      bool operator==(ThrowingMove const &) const = default;
    };
    // nothrow branches, but relocating one branch's alternative into the superset may throw
    constexpr auto fnThrowingArm
        = fn::overload{[](A) noexcept { return fn::choice<ThrowingMove>{std::in_place_type<ThrowingMove>}; },
                       [](B) noexcept { return fn::choice<U>{U{}}; }};
    static_assert(not noexcept(j.and_then(fnThrowingArm)));

    // asking answers: an inapplicable callback drops and_then from the overload set; a
    // value-returning one stays viable - its rejection is the deliberately loud static_assert
    // on instantiation, not a constraint
    constexpr auto fnPartial = [](A) { return fn::choice<U>{U{}}; };
    static_assert(not can_and_then<J &, decltype(fnPartial)>);
    static_assert(can_and_then<fn::choice<A> &, decltype(fnPartial)>);
    constexpr auto fnValue = [](auto &&) { return 42; };
    static_assert(can_and_then<J &, decltype(fnValue)>);
  }
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
    constexpr auto fn = fn::overload{[](bool) -> fn::copack<bool> { throw 1; },    //
                                     [](int &) -> fn::copack<double> { throw 0; }, //
                                     [](int const &i) -> fn::copack<double> {      //
                                       return i / 8.0;
                                     },
                                     [](int &&) -> fn::copack<double> { throw 0; }, //
                                     [](int const &&) -> fn::copack<double> { throw 0; }};
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
    using ::fn::copack;
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

  SECTION("a result no choice can hold")
  {
    // a void-returning callback must drop the caller's candidate in the immediate context: the
    // collapsing machinery would hard-error where no requires-expression can absorb it
    constexpr auto fnVoid = [](auto &&...) {};
    static_assert(not can_transform<fn::choice<bool, int> &, decltype(fnVoid)>);
    static_assert(not can_transform<fn::choice<bool, int> const &, decltype(fnVoid)>);
    static_assert(not can_transform<fn::choice<bool, int> &&, decltype(fnVoid)>);
    static_assert(not can_transform<fn::choice<bool, int> const &&, decltype(fnVoid)>);
    SUCCEED();
  }
}

TEST_CASE("choice assignment", "[choice][assignment]")
{
  using fn::choice;

  // choice adds no state of its own, so its assignment is the base copack's: same strong guarantee,
  // same constraints, same noexcept - it only has to be declared, since choice's move constructor
  // would otherwise delete the implicit copy assignment and suppress the implicit move assignment
  static_assert(std::is_copy_assignable_v<choice<bool, int>>);
  static_assert(std::is_move_assignable_v<choice<bool, int>>);
  static_assert(std::is_nothrow_copy_assignable_v<choice<bool, int>>);
  static_assert(not std::is_nothrow_copy_assignable_v<choice<std::string>>);
  static_assert(std::is_nothrow_move_assignable_v<choice<std::string>>);

  SECTION("the alternative changes")
  {
    choice<bool, int> a{42};
    choice<bool, int> const b{true};
    a = b;
    CHECK(a == choice{true});
    CHECK(a.has_value(std::in_place_type<bool>));

    a = choice<bool, int>{12};
    CHECK(a == choice{12});
  }

  SECTION("constexpr")
  {
    static_assert([] {
      choice<bool, int> a{42};
      a = choice<bool, int>{true};
      return a == choice{true};
    }());
    SUCCEED();
  }
}

TEST_CASE("choice emplace", "[choice][emplace]")
{
  using fn::choice;

  // copack::emplace is inherited - a named member, so the name hiding that forces choice to restate
  // every operator= does not apply
  struct Sender final { // not assignable, nothrow-move-constructible
    int target;
    constexpr explicit Sender(int t) noexcept : target(t) {}
    constexpr Sender(Sender &&) noexcept = default;
    Sender(Sender const &) = delete;
    Sender &operator=(Sender const &) = delete;
    Sender &operator=(Sender &&) = delete;
  };
  using C = fn::choice_for<Sender, int>;
  static_assert(not std::is_copy_assignable_v<C>);
  static_assert(not std::is_move_assignable_v<C>);
  static_assert(can_emplace<C, Sender, int>);
  static_assert(not can_emplace<C, double, double>); // not an alternative
  static_assert(std::same_as<decltype(std::declval<C &>().emplace<int>(1)), int &>);
  static_assert(noexcept(std::declval<C &>().emplace<int>(1)));

  constexpr auto battery = [] {
    C c{42};
    Sender &r = c.emplace<Sender>(9);
    bool ok = r.target == 9 && c.has_value(std::in_place_type<Sender>);
    int &i = c.emplace<int>(5);
    return ok && i == 5 && c.has_value(std::in_place_type<int>);
  };
  CHECK(battery());
  static_assert(battery());
}

TEST_CASE("choice special members", "[choice]")
{
  using fn::choice;

  // choice declares all five, and defaults all five: the copy constructor because a user-declared
  // move constructor would otherwise delete it, and the two assignments because they would otherwise
  // be deleted and suppressed in turn. Removing a `= default`, adding a `noexcept` the base does not
  // promise, or narrowing a requires-clause would all break the equalities below.
  static_assert(special_members_follow_copack<int>());
  static_assert(special_members_follow_copack<bool, int>());
  static_assert(special_members_follow_copack<std::string>());
  static_assert(special_members_follow_copack<helper_move_only>());
  static_assert(special_members_follow_copack<helper_immovable>());

  // ... and what they follow it TO, so that the equalities cannot be satisfied by both being wrong
  static_assert(std::is_nothrow_copy_constructible_v<choice<int>>);
  static_assert(std::is_nothrow_move_constructible_v<choice<int>>);
  static_assert(std::is_nothrow_copy_assignable_v<choice<int>>);
  static_assert(std::is_nothrow_move_assignable_v<choice<int>>);
  static_assert(std::is_nothrow_destructible_v<choice<int>>);
  static_assert(std::is_trivially_destructible_v<choice<int>>); // as trivial as its alternatives permit

  // a throwing copy is reported as one, rather than promised away
  static_assert(std::is_copy_constructible_v<choice<std::string>>);
  static_assert(not std::is_nothrow_copy_constructible_v<choice<std::string>>);
  static_assert(std::is_nothrow_move_constructible_v<choice<std::string>>);
  static_assert(not std::is_nothrow_copy_assignable_v<choice<std::string>>);
  static_assert(std::is_nothrow_move_assignable_v<choice<std::string>>);

  // an alternative that cannot be copied takes copy construction and copy assignment with it
  static_assert(not std::is_copy_constructible_v<choice<helper_move_only>>);
  static_assert(not std::is_copy_assignable_v<choice<helper_move_only>>);
  static_assert(std::is_nothrow_move_constructible_v<choice<helper_move_only>>);
  static_assert(std::is_nothrow_move_assignable_v<choice<helper_move_only>>);

  // ... and one that can be neither copied nor moved leaves none of the four
  static_assert(not std::is_copy_constructible_v<choice<helper_immovable>>);
  static_assert(not std::is_move_constructible_v<choice<helper_immovable>>);
  static_assert(not std::is_copy_assignable_v<choice<helper_immovable>>);
  static_assert(not std::is_move_assignable_v<choice<helper_immovable>>);
  SUCCEED();
}
