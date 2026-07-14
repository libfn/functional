// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/pack.hpp>
#include <fn/sum.hpp>
#include <fn/utility.hpp>

#include <catch2/catch_all.hpp>

#include <array>
#include <concepts>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace {
struct TestType final {
  static int count;
  TestType() noexcept { ++count; }
  ~TestType() noexcept { --count; }
};
int TestType::count = 0;

struct NonCopyable final {
  int v;

  constexpr operator int() const { return v; }
  constexpr NonCopyable(int i) noexcept : v(i) {}
  NonCopyable(NonCopyable const &) = delete;
  NonCopyable &operator=(NonCopyable const &) = delete;
};

template <fn::sum<bool, int> S> struct sum_nttp final {};
template <fn::some_sum auto S> struct some_sum_nttp final {};
template <fn::some_sum auto S> auto read_nttp()
{
  return S.invoke([](auto const &...args) { return (0.0 + ... + static_cast<double>(args)); });
}

// Every operation sum performs on an alternative - copy, move, compare - can throw here, so the
// member wrapping it must say so. Witnesses the conditional noexcept below.
struct Throwing final {
  int v;

  constexpr operator int() const { return v; }
  constexpr Throwing(int i) noexcept : v(i) {}
  constexpr Throwing(Throwing const &o) noexcept(false) : v(o.v) {}
  constexpr Throwing(Throwing &&o) noexcept(false) : v(o.v) {}
  constexpr bool operator==(Throwing const &o) const noexcept(false) { return v == o.v; }
};

// Copy is explicit, move is implicit - so an lvalue argument is constructible-but-not-convertible
// and selects sum's explicit value constructor, while an rvalue selects the implicit one.
// NOTE no operator int(): with the implicit ExplicitCopy(int) ctor it would open an
// ExplicitCopy& -> int -> ExplicitCopy path, making the argument convertible after all.
struct ExplicitCopy final {
  int v;

  constexpr ExplicitCopy(int i) noexcept : v(i) {}
  constexpr explicit ExplicitCopy(ExplicitCopy const &o) noexcept : v(o.v) {}
  constexpr ExplicitCopy(ExplicitCopy &&o) noexcept : v(o.v) {}
};

// Counts live instances through every path a sum can take one: unlike TestType, whose implicit copy
// constructor does not count (leaving its counter negative once anything copies it), this one is
// balanced, so an assignment can be asked whether it destroyed what it replaced.
struct Counted final {
  static int live;
  int v;

  Counted(int i) noexcept : v(i) { ++live; } // NOLINT: implicit on purpose
  Counted(Counted const &o) noexcept : v(o.v) { ++live; }
  Counted(Counted &&o) noexcept : v(o.v) { ++live; }
  ~Counted() noexcept { --live; }
  Counted &operator=(Counted const &) noexcept = default; // assignment does not change what is alive
  Counted &operator=(Counted &&) noexcept = default;
  bool operator==(Counted const &) const noexcept = default;
};
int Counted::live = 0;

// Comparison probes are type-keyed rather than the file's usual value-taking lambda: sum<> has no
// values to pass one (its default constructor is deleted, by design).
template <typename L, typename R>
concept can_eq = requires { std::declval<L const &>() == std::declval<R const &>(); };
template <typename L, typename R>
concept can_ne = requires { std::declval<L const &>() != std::declval<R const &>(); };

template <typename S, typename T, typename... Args>
concept can_in_place = requires(Args... args) { S{std::in_place_type<T>, args...}; };

template <typename T, typename... Args>
concept can_as_sum = requires(Args... args) { fn::as_sum(std::in_place_type<T>, args...); };

template <typename T>
concept can_as_sum_value = requires(T v) { fn::as_sum(FWD(v)); };

template <typename S, typename Fn>
concept can_transform = requires(S s, Fn fn) { FWD(s).transform(fn); };
} // anonymous namespace

// A sum brace-initializes the alternative it stores. That is a DESIGN DIRECTION, not an
// implementation detail, and this TEST_CASE exists to fail if it is ever switched to the
// parenthesized initialization `std::variant` uses. Many other tests would fail with it, but
// incidentally - this one is here to say what was chosen, and why.
//
// What braces buy:
//   * Aggregate forwarding, through brace elision: `sum{in_place_type<array<int,3>>, 1, 2, 3}`.
//     `std::variant` cannot express this, and neither can parenthesized initialization - P0960
//     admits aggregates but forbids brace elision.
//   * Coherence with `sum` being a structural type. A structural type's alternatives must themselves
//     be structural - public-member types, written with braces, exactly as one writes them as a
//     template argument. Storing them any other way would put the two spellings at odds.
//   * Narrowing is rejected outright rather than silently truncating.
//
// What braces cost:
//   * An initializer-list constructor wins where `std::variant` would call the (count, value) one:
//     `sum<vector<int>>{in_place_type<vector<int>>, 3, 0}` holds `{3, 0}`, where a variant would hold
//     `{0, 0, 0}`. This divergence is the cost of giving `sum` a capability which `std::variant` lacks.
//
// And the consequence that actually bites, which is why the last section is here: every trait that
// constrains or specifies the construction of an alternative must ask the BRACE question
// (`fn::detail::_initializable` / `_nothrow_initializable`), never `std::is_[nothrow_]constructible`,
// which asks about parentheses. Where the two disagree, the parenthesized answer is a lie.
TEST_CASE("design: braces, not parentheses", "[sum][design]")
{
  using fn::sum;

  SECTION("aggregate forwarding")
  {
    using A = std::array<int, 3>;
    static_assert(can_in_place<sum<A>, A, int, int, int>);        // braces elide; parentheses cannot
    static_assert(not std::is_constructible_v<A, int, int, int>); // ... which is why the std trait misleads
    static_assert(fn::detail::_initializable<A, int, int, int>);  // ... and this is the trait that does not

    sum<A> a{std::in_place_type<A>, 1, 2, 3};
    CHECK(a.invoke([](A const &v) { return v[0] * 100 + v[1] * 10 + v[2]; }) == 123);
    static_assert(sum<A>{std::in_place_type<A>, 1, 2, 3}.invoke([](A const &v) { return v[2]; }) == 3);
  }

  SECTION("narrowing")
  {
    static_assert(not can_in_place<sum<int>, int, double>); // braces reject it ...
    static_assert(std::is_constructible_v<int, double>);    // ... where parentheses would truncate in silence
    SUCCEED();
  }

  SECTION("initializer-list constructors win")
  {
    // the price of the above, and the same mechanism as the trap below: `std::variant` would hold
    // three zeroes here, and a sum holds the two elements it was written with
    using V = std::vector<int>;
    sum<V> v{std::in_place_type<V>, 3, 0};
    CHECK(v.invoke([](V const &x) { return x.size(); }) == 2);
    CHECK(v.invoke([](V const &x) { return x[0]; }) == 3);
  }

  SECTION("the traits must ask the brace question")
  {
    // Braced initialization considers initializer-list constructors FIRST, so the two questions can
    // answer differently - and the parenthesized answer is not the one the storage acts on.
    struct Listy final {
      Listy(int) noexcept {}                               // parentheses select this ...
      Listy(std::initializer_list<int>) noexcept(false) {} // ... braces select this
    };
    static_assert(std::is_nothrow_constructible_v<Listy, int>);            // the parenthesized question: nothrow
    static_assert(not fn::detail::_nothrow_initializable<Listy, int>);     // the braced question: it can throw
    static_assert(not noexcept(sum<Listy>{std::in_place_type<Listy>, 1})); // the sum promises what it does
  }

  SECTION("... including where a compiler answers it differently")
  {
    // The divergence also reaches the copy and move constructors, and the assignment built on them.
    // `Evil` converts to double only as an rvalue, so `Evil{lvalue}` copies while `Evil{rvalue}`
    // selects the throwing initializer-list constructor - which is what [over.match.list]/1 requires,
    // [dcl.init.list]/3.2's same-type carve-out being for AGGREGATES only, and Evil is not one.
    // gcc and clang>=20 have it so; clang<=19, AppleClang and MSVC pick the move constructor instead.
    //
    // So these pin the INVARIANT rather than the answer: whatever a compiler makes of the braces is
    // what the sum must promise. Reading the parenthesized answer instead - which is always "nothrow
    // move" here - declared a `noexcept` that std::terminate had to keep, and offered an assignment
    // that destroyed the alternative it held and then failed to replace it.
    struct Tag final {};
    struct Evil final {
      int v;
      Evil(Tag, int i) noexcept : v(i) {} // NOLINT: constructs without touching either trap
      operator double() && noexcept { return v; }
      Evil(std::initializer_list<double>) { throw std::runtime_error("init-list"); }
      Evil(Evil &&o) noexcept : v(o.v) {}
      Evil(Evil const &o) noexcept(false) : v(o.v) {}
    };
    static_assert(std::is_nothrow_move_constructible_v<Evil>); // the parenthesized question, everywhere

    static_assert(std::is_nothrow_move_constructible_v<sum<Evil>> == fn::detail::_nothrow_initializable<Evil, Evil>);
    static_assert(std::is_nothrow_copy_constructible_v<sum<Evil>>
                  == fn::detail::_nothrow_initializable<Evil, Evil const &>);
    static_assert(std::is_move_assignable_v<sum<Evil>>
                  == (std::is_move_assignable_v<Evil> && fn::detail::_nothrow_initializable<Evil, Evil>));
    SUCCEED();
  }
}

TEST_CASE("sum basic functionality tests", "[sum]")
{
  // NOTE This test looks very similar to test in choice.cpp - for good reason.

  using fn::sum;

  SECTION("sum<> unit")
  {
    static_assert(sum<>::size == 0);
    static_assert(sum<>::has_type<bool> == false);
    static_assert(std::same_as<fn::sum_for<sum<>, sum<>>, sum<>>);
    static_assert(not std::is_default_constructible_v<sum<>>); // the deleted default ctor is the point
    static_assert(std::is_nothrow_copy_constructible_v<sum<>>);
    static_assert(std::is_nothrow_move_constructible_v<sum<>>);
    static_assert(std::is_nothrow_destructible_v<sum<>>);
    SUCCEED();
  }

  SECTION("as_sum")
  {
    constexpr auto a = fn::as_sum(12);
    static_assert(std::same_as<decltype(a), fn::sum<int> const>);
    static_assert(a == fn::sum{12});

    constexpr auto b = fn::as_sum(std::in_place_type<long>, 12);
    static_assert(std::same_as<decltype(b), fn::sum<long> const>);
    static_assert(b == fn::sum{12l});

    // both lifts weigh the alternative they construct, asking the brace initialization they perform
    static_assert(noexcept(fn::as_sum(12)));
    static_assert(noexcept(fn::as_sum(std::in_place_type<long>, 12)));
    static_assert(not noexcept(fn::as_sum(std::declval<Throwing const &>())));

    SECTION("constraints")
    {
      static_assert(can_as_sum<long, int>);
      static_assert(can_as_sum<std::array<int, 3>, int, int, int>); // an aggregate, brace-initialized
      static_assert(not can_as_sum<NonCopyable>);                   // no default constructor
      static_assert(not can_as_sum<NonCopyable, char const *>);     // not constructible from it

      // the tag selects the alternative, it is never itself one: with nothing to construct there is
      // no viable lift at all, rather than a sum whose alternative is the tag
      static_assert(not can_as_sum_value<std::in_place_type_t<NonCopyable> const &>);
      static_assert(can_as_sum_value<long>);
      SUCCEED();
    }
  }

  SECTION("sum_for")
  {
    static_assert(std::same_as<fn::sum_for<int>, fn::sum<int>>);
    static_assert(std::same_as<fn::sum_for<int, int>, fn::sum<int>>);
    static_assert(std::same_as<fn::sum_for<int, bool>, fn::sum<bool, int>>);
    static_assert(std::same_as<fn::sum_for<bool, int>, fn::sum<bool, int>>);
    // A sum's canonical alternative order comes from each type's compiler spelling (GCC/Clang
    // __PRETTY_FUNCTION__ vs MSVC __FUNCSIG__), so it is platform/ABI-specific: MSVC sorts
    // class/struct types after the fundamentals, GCC/Clang before. This divergence is inherent and
    // deliberately NOT unified — even C++26 std::type_order is an implementation-defined, ABI-tied
    // total order, so there is no single cross-platform order; don't try to make them match. This
    // one assert documents the difference; every other ordering check below asserts only the
    // platform-independent guarantees (commutativity, uniqueness). Revisit once std::type_order
    // ships on all supported platforms.
    // Spelling: sums whose alternatives include a non-builtin have platform-specific order, so they
    // are written sum_for<...>; pure-builtin sums keep a fixed sum<...>.
#ifdef _MSC_VER
    static_assert(std::same_as<fn::sum_for<int, NonCopyable>, fn::sum<int, NonCopyable>>);
#else
    static_assert(std::same_as<fn::sum_for<int, NonCopyable>, fn::sum<NonCopyable, int>>);
#endif
    static_assert(std::same_as<fn::sum_for<NonCopyable, int>, fn::sum_for<int, NonCopyable>>); // commutative
    static_assert(
        std::same_as<fn::sum_for<int, bool, NonCopyable>, fn::sum_for<NonCopyable, bool, int>>); // commutative
    static_assert(std::same_as<fn::sum_for<NonCopyable, int, NonCopyable>, fn::sum_for<int, NonCopyable>>); // unique
    static_assert(fn::sum_for<int, bool, NonCopyable>::size == 3);

    static_assert(std::same_as<fn::sum_for<int, fn::sum<int>>, fn::sum<int>>);
    static_assert(std::same_as<fn::sum_for<int, fn::sum<bool>>, fn::sum<bool, int>>);
    static_assert(std::same_as<fn::sum_for<int, fn::sum<bool, int>>, fn::sum<bool, int>>);
    static_assert(std::same_as<fn::sum_for<int, fn::sum<bool, double>>, fn::sum<bool, double, int>>);

    static_assert(std::same_as<fn::sum_for<fn::sum<bool>, fn::sum<int>>, fn::sum<bool, int>>);
    static_assert(std::same_as<fn::sum_for<fn::sum<bool>, fn::sum<bool, double, int>>, fn::sum<bool, double, int>>);
    static_assert(std::same_as<fn::sum_for<fn::sum<bool>, fn::sum<double, int>>, fn::sum<bool, double, int>>);
    static_assert(std::same_as<fn::sum_for<fn::sum<bool, int>, double>, fn::sum<bool, double, int>>);

    static_assert(std::same_as<fn::sum_for<int, fn::sum<>>, fn::sum<int>>);
    static_assert(std::same_as<fn::sum_for<fn::sum<>, int>, fn::sum<int>>);
    static_assert(std::same_as<fn::sum_for<fn::sum<>, fn::sum<bool, int>>, fn::sum<bool, int>>);
    static_assert(std::same_as<fn::sum_for<double, fn::sum<>, fn::sum<bool, int>>, fn::sum<bool, double, int>>);
  }

  SECTION("invocable")
  {
    using type = fn::sum_for<TestType, int>; // sum<...> order is platform-specific; sum_for normalizes per platform
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

    static_assert(fn::typelist_invocable<decltype([](auto &) {}), sum<NonCopyable> &>);
    static_assert(not fn::typelist_invocable<decltype([](auto) {}), NonCopyable &>); // copy-constructor not available

    // variadic-generic callback, and a per-category sweep of an lvalue-only overload set
    using T2 = fn::sum<double, int>;
    static_assert(fn::typelist_invocable<decltype([](auto...) {}), T2 &>);
    constexpr auto fnLvalue = fn::overload{[](int &) {}, [](double &) {}};
    static_assert(fn::typelist_invocable<decltype(fnLvalue), T2 &>);
    static_assert(not fn::typelist_invocable<decltype(fnLvalue), T2 const &>);
    static_assert(not fn::typelist_invocable<decltype(fnLvalue), T2>);
    static_assert(not fn::typelist_invocable<decltype(fnLvalue), T2 const>);
    static_assert(not fn::typelist_invocable<decltype(fnLvalue), T2 &&>);
    static_assert(not fn::typelist_invocable<decltype(fnLvalue), T2 const &&>);
  }

  SECTION("check destructor call")
  {
    {
      sum<TestType> s{std::in_place_type<TestType>};
      static_assert(decltype(s)::has_type<TestType>);
      static_assert(not decltype(s)::has_type<int>);
      CHECK(s.has_value(std::in_place_type<TestType>));
      CHECK(s.template has_value<TestType>());
      CHECK(TestType::count == 1);
    }
    CHECK(TestType::count == 0);
  }

  SECTION("single parameter constructor")
  {
    static_assert(sum<int>::size == 1);

    constexpr sum<int> a = 12;
    static_assert(a == sum{12});

    constexpr sum<bool> b{false};
    static_assert(b == sum{false});

    SECTION("noexcept")
    {
      // the value constructors weigh the alternative they construct
      static_assert(std::is_nothrow_constructible_v<int, int>);
      static_assert(noexcept(sum<int>{42}));
      static_assert(noexcept(sum<int>{std::in_place_type<int>, 42}));

      // ... and report it when that construction can throw
      static_assert(not noexcept(sum<Throwing>{std::declval<Throwing const &>()}));
      static_assert(not noexcept(sum<Throwing>{std::in_place_type<Throwing>, std::declval<Throwing const &>()}));
      SUCCEED();
    }

    SECTION("explicit (non-convertible) argument")
    {
      // The two value constructors differ only in the argument's convertibility to the alternative:
      // ExplicitCopy's copy constructor is explicit, so an lvalue selects the explicit arm and can
      // only direct-initialize, while an rvalue (implicit move) selects the implicit one.
      static_assert(std::is_constructible_v<ExplicitCopy, ExplicitCopy &>);
      static_assert(not std::is_convertible_v<ExplicitCopy &, ExplicitCopy>);

      static_assert(std::is_constructible_v<sum<ExplicitCopy>, ExplicitCopy &>);
      static_assert(not std::is_convertible_v<ExplicitCopy &, sum<ExplicitCopy>>); // explicit arm
      static_assert(std::is_constructible_v<sum<ExplicitCopy>, ExplicitCopy &&>);
      static_assert(std::is_convertible_v<ExplicitCopy &&, sum<ExplicitCopy>>); // implicit arm

      ExplicitCopy e{42};
      sum<ExplicitCopy> a{e};
      CHECK(a.invoke([](auto &&i) -> int { return i.v; }) == 42);

      sum<ExplicitCopy> b{std::move(e)};
      CHECK(b.invoke([](auto &&i) -> int { return i.v; }) == 42);

      SECTION("constexpr")
      {
        constexpr auto c = []() constexpr {
          ExplicitCopy e{42};
          return sum<ExplicitCopy>{e};
        }();
        static_assert(c.invoke([](auto &&i) -> int { return i.v; }) == 42);
        SUCCEED();
      }
    }

    SECTION("CTAD")
    {
      sum a{42};
      static_assert(std::is_same_v<decltype(a), sum<int>>);
      CHECK(a == sum<int>{42});

      constexpr sum b{false};
      static_assert(std::is_same_v<decltype(b), sum<bool> const>);
      static_assert(b == sum<bool>{false});

      constexpr auto c = sum{std::array<int, 3>{3, 14, 15}};
      static_assert(std::is_same_v<decltype(c), sum<std::array<int, 3>> const>);
      static_assert(c.invoke([](auto &&a) -> bool { return a.size() == 3 && a[0] == 3 && a[1] == 14 && a[2] == 15; }));
    }

    SECTION("move from rvalue")
    {
      using T = fn::sum<bool, int>;
      constexpr auto fn = [](auto i) constexpr noexcept -> T { return {std::move(i)}; };
      constexpr auto a = fn(true);
      static_assert(std::is_same_v<decltype(a), T const>);
      static_assert(a.has_value<bool>());

      constexpr auto b = fn(12);
      static_assert(std::is_same_v<decltype(b), T const>);
      static_assert(b.has_value<int>());
    }

    SECTION("copy from lvalue")
    {
      using T = fn::sum<bool, int>;
      constexpr auto fn = [](auto i) constexpr noexcept -> T { return {i}; };
      constexpr auto a = fn(true);
      static_assert(std::is_same_v<decltype(a), T const>);
      static_assert(a.has_value<bool>());

      constexpr auto b = fn(12);
      static_assert(std::is_same_v<decltype(b), T const>);
      static_assert(b.has_value<int>());
    }
  }

  SECTION("forwarding constructors (immovable)")
  {
    sum<NonCopyable> a{std::in_place_type<NonCopyable>, 42};
    CHECK(a.invoke([](auto &i) -> bool { return i.v == 42; }));

    SECTION("CTAD")
    {
      constexpr auto a = sum{std::in_place_type<NonCopyable>, 42};
      static_assert(std::is_same_v<decltype(a), sum<NonCopyable> const>);

      auto b = sum{std::in_place_type<NonCopyable>, 42};
      static_assert(std::is_same_v<decltype(b), sum<NonCopyable>>);
    }

    SECTION("constraints")
    {
      static_assert(can_in_place<sum<NonCopyable>, NonCopyable, int>);
      static_assert(not can_in_place<sum<NonCopyable>, int, int>); // int is not an alternative

      // an argument list the alternative cannot be constructed from is not viable - viability must
      // answer here, not fail to compile inside variadic_union, beyond SFINAE's reach
      static_assert(not can_in_place<sum<NonCopyable>, NonCopyable>);               // no default ctor
      static_assert(not can_in_place<sum<NonCopyable>, NonCopyable, char const *>); // not constructible from
      SUCCEED();
    }
  }

  SECTION("forwarding constructors (aggregate)")
  {
    SECTION("regular")
    {
      sum<std::array<int, 3>> a{std::in_place_type<std::array<int, 3>>, 1, 2, 3};
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
      constexpr sum<std::array<int, 3>> a{std::in_place_type<std::array<int, 3>>, 1, 2, 3};
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
      constexpr auto a = sum{std::in_place_type<std::array<int, 3>>, 1, 2, 3};
      static_assert(std::is_same_v<decltype(a), sum<std::array<int, 3>> const>);

      auto b = sum{std::in_place_type<std::array<int, 3>>, 1, 2, 3};
      static_assert(std::is_same_v<decltype(b), sum<std::array<int, 3>>>);
    }

    SECTION("constraints")
    {
      using T = std::array<int, 3>;
      // the element is brace-initialized, which elides braces for an aggregate - a constraint
      // spelled with is_constructible_v (parenthesized init) would reject this very construction
      static_assert(can_in_place<sum<T>, T, int, int, int>);
      static_assert(not can_in_place<sum<T>, T, int, int, int, int>); // one too many
      static_assert(not can_in_place<sum<int>, int, double>);         // narrowing, rejected by braces
      SUCCEED();
    }
  }

  SECTION("widening constructor")
  {
    using T = sum<bool, double, int>;
    using S = sum<bool, int>;

    SECTION("from lvalue")
    {
      S const a{std::in_place_type<int>, 42};
      T b{a};
      CHECK(b.has_value(std::in_place_type<int>));
      CHECK(b.invoke([](auto &&i) -> int { return static_cast<int>(i); }) == 42);
      CHECK(a.has_value(std::in_place_type<int>)); // the source is copied, not consumed
    }

    SECTION("from rvalue")
    {
      S a{std::in_place_type<int>, 42};
      T b{std::move(a)};
      CHECK(b.has_value(std::in_place_type<int>));
      CHECK(b.invoke([](auto &&i) -> int { return static_cast<int>(i); }) == 42);
    }

    SECTION("in_place_type names the source sum")
    {
      S const a{std::in_place_type<bool>, true};
      T b{std::in_place_type<S>, a};
      CHECK(b.has_value(std::in_place_type<bool>));
      CHECK(b.invoke([](auto &&i) -> bool { return static_cast<bool>(i); }));
    }

    SECTION("constexpr")
    {
      constexpr S a{std::in_place_type<int>, 42};
      constexpr T b{a};
      static_assert(b.has_value(std::in_place_type<int>));
      static_assert(b == T{42});

      constexpr T c{S{true}}; // rvalue
      static_assert(c.has_value(std::in_place_type<bool>));

      constexpr T d{std::in_place_type<S>, a};
      static_assert(d == b);
      SUCCEED();
    }

    SECTION("constraints")
    {
      // Widening only ever widens: the target must be a superset of the source.
      static_assert(std::is_constructible_v<T, S const &>);
      static_assert(std::is_constructible_v<T, S &&>);
      static_assert(std::is_constructible_v<T, sum<int> const &>);
      static_assert(not std::is_constructible_v<sum<int>, S const &>); // narrowing
      static_assert(not std::is_constructible_v<sum<int>, S &&>);
      static_assert(not std::is_constructible_v<S, sum<double> const &>); // disjoint

      // The in_place_type form names the SOURCE sum, and is superset-constrained the same way.
      static_assert(std::is_constructible_v<T, std::in_place_type_t<S>, S const &>);
      static_assert(not std::is_constructible_v<sum<int>, std::in_place_type_t<S>, S const &>);

      // Same-type construction is the copy/move constructor - the widening pair excludes it.
      static_assert(std::is_constructible_v<T, T const &>);
      SUCCEED();
    }

    SECTION("noexcept")
    {
      // each widening constructor copies or moves every alternative of the source into the wider
      // union, and weighs what it relocates
      using X = fn::sum_for<Throwing, int>;
      static_assert(not noexcept(X{std::declval<sum<Throwing> const &>()}));
      static_assert(not noexcept(X{std::declval<sum<Throwing> &&>()}));
      static_assert(not noexcept(X{std::in_place_type<sum<Throwing>>, std::declval<sum<Throwing> const &>()}));

      // ... so an alternative that cannot throw widens without an exception edge
      using Y = fn::sum<bool, int>;
      static_assert(noexcept(Y{std::declval<sum<int> const &>()}));
      static_assert(noexcept(Y{std::declval<sum<int> &&>()}));
      SUCCEED();
    }
  }

  SECTION("has_type type mismatch")
  {
    using type = sum<bool, int>;
    static_assert(type::has_type<int>);
    static_assert(type::has_type<bool>);
    static_assert(not type::has_type<double>);
    type a{std::in_place_type<int>, 42};
    CHECK(a.has_value(std::in_place_type<int>));
    CHECK(not a.has_value(std::in_place_type<bool>));
    static_assert([](auto const &a) constexpr -> bool { //
      return not requires { a.has_value(std::in_place_type<double>); };
    }(a));                                              // double is not a type member
    static_assert([](auto const &a) constexpr -> bool { //
      return not requires { a.template has_value<double>(); };
    }(a)); // double is not a type member

    // Accurate, unlike the members below: reading the discriminator touches no alternative, so the
    // promise holds even when every operation on that alternative can throw.
    static_assert(noexcept(a.has_value(std::in_place_type<int>)));
    static_assert(noexcept(a.template has_value<int>()));
    static_assert(noexcept(std::declval<sum<Throwing> const &>().has_value(std::in_place_type<Throwing>)));
  }

  SECTION("index")
  {
    constexpr fn::sum<std::array<int, 3>> a{std::in_place_type<std::array<int, 3>>, 3, 14, 15};
    static_assert(a.index == 0);

    fn::sum<double, int> b{std::in_place_type<int>, 42};
    CHECK(b.index == 1);
    constexpr fn::sum<double, int> c{std::in_place_type<int>, 12};
    static_assert(c.index == 1);
  }

  SECTION("select_nth")
  {
    using T = fn::sum<double, int>;
    static_assert(T::size == 2);
    static_assert(std::is_same_v<T::template select_nth<0>, double>);
    static_assert(std::is_same_v<T::template select_nth<1>, int>);

    SUCCEED();
  }

  SECTION("get_ptr")
  {
    using T = fn::sum<double, int>;
    T b{std::in_place_type<int>, 42};
    CHECK(b.template has_value<int>());
    CHECK(b.has_value(std::in_place_type<int>));

    static_assert(std::is_same_v<decltype(b.get_ptr(std::in_place_type<int>)), int *>);
    CHECK(b.get_ptr(std::in_place_type<int>) == &b.data.v1);
    CHECK(b.get_ptr(std::in_place_type<double>) == nullptr);
    static_assert(std::is_same_v<decltype(std::as_const(b).get_ptr(std::in_place_type<int>)), int const *>);
    CHECK(std::as_const(b).get_ptr(std::in_place_type<int>) == &b.data.v1);
    CHECK(std::as_const(b).get_ptr(std::in_place_type<double>) == nullptr);
    static_assert(noexcept(b.get_ptr(std::in_place_type<int>))); // accurate: no alternative is touched
    static_assert(noexcept(std::as_const(b).get_ptr(std::in_place_type<int>)));
    static_assert([](auto &b) constexpr -> bool { //
      return not requires { b.get_ptr(std::in_place_type<bool>); };
    }(b)); // bool is not a type member

    T const c{std::in_place_type<double>, 4.25};
    CHECK(c.get_ptr(std::in_place_type<int>) == nullptr);
    CHECK(c.get_ptr(std::in_place_type<double>) == &c.data.v0);

    constexpr auto d = fn::sum<double, int>{std::in_place_type<int>, 12};
    static_assert(d.get_ptr(std::in_place_type<double>) == nullptr);
    static_assert(*d.get_ptr(std::in_place_type<int>) == 12);
  }

  SECTION("equality comparison")
  {
    using type = sum<bool, int>;

    // != is synthesized by C++20 rewriting from ==, so it cannot claim to be viable where == is not.
    // Against the uninstantiable sum<> both must drop out of overload resolution together
    static_assert(can_eq<type, type>);
    static_assert(can_ne<type, type>);
    static_assert(not can_eq<sum<int>, sum<>>);
    static_assert(not can_ne<sum<int>, sum<>>);
    static_assert(not can_eq<sum<>, sum<int>>);
    static_assert(not can_ne<sum<>, sum<int>>);
    static_assert(not can_eq<sum<>, sum<>>);
    static_assert(not can_ne<sum<>, sum<>>);

    type const a{std::in_place_type<int>, 42};
    static_assert(std::is_same_v<bool, decltype(sum{42} == a)>);
    CHECK(a == type{42});
    CHECK(type{42} == a);
    CHECK(a != type{41});
    CHECK(type{41} != a);
    CHECK(a != type{true});
    CHECK(type{false} != a);
    CHECK(a == sum{42});
    CHECK(sum{42} == a);
    CHECK(a != sum{41});
    CHECK(sum{41} != a);
    CHECK(a != sum{false});
    CHECK(sum{true} != a);
    CHECK(a == sum<double, int>{42});
    CHECK(sum<double, int>{42} == a);
    CHECK(a != sum<double, int>{41});
    CHECK(sum<double, int>{41} != a);
    CHECK(sum{0.5} != a);
    CHECK(a != sum{0.5});

    SECTION("constexpr")
    {
      constexpr type a{std::in_place_type<int>, 42};
      static_assert(std::is_same_v<bool, decltype(a == sum{42})>);
      static_assert(a == type{42});
      static_assert(type{42} == a);
      static_assert(a != type{41});
      static_assert(type{41} != a);
      static_assert(a != type{true});
      static_assert(type{false} != a);
      static_assert(a == sum{42});
      static_assert(sum{42} == a);
      static_assert(a != sum{41});
      static_assert(sum{41} != a);
      static_assert(a != sum{false});
      static_assert(sum{true} != a);
      static_assert(a == sum<double, int>{42});
      static_assert(sum<double, int>{42} == a);
      static_assert(a != sum<double, int>{41});
      static_assert(sum<double, int>{41} != a);
      static_assert(sum{0.5} != a);
      static_assert(a != sum{0.5});

      static_assert([](auto const &a) constexpr -> bool {
        return not requires { a == 42; }; // no implicit conversion
      }(a));
      static_assert([](auto const &a) constexpr -> bool {
        return not requires { a != 42; }; // no implicit conversion
      }(a));
      static_assert([](auto const &a) constexpr -> bool {
        return not requires { a == 0.5; }; // no implicit conversion
      }(a));
      static_assert([](auto const &a) constexpr -> bool {
        return not requires { a != 0.5; }; // no implicit conversion
      }(a));
    }

    SECTION("noexcept")
    {
      using T = sum<Throwing>;

      // operator== reaches into the alternative's own comparison, and weighs it - as does the !=
      // rewritten from it
      static_assert(not noexcept(std::declval<Throwing const &>() == std::declval<Throwing const &>()));
      static_assert(not noexcept(std::declval<T const &>() == std::declval<T const &>()));
      static_assert(not noexcept(std::declval<T const &>() != std::declval<T const &>()));

      // ... while an alternative whose comparison cannot throw is compared without an exception edge
      using Q = sum<int>;
      static_assert(noexcept(std::declval<Q const &>() == std::declval<Q const &>()));
      static_assert(noexcept(std::declval<Q const &>() != std::declval<Q const &>()));

      SUCCEED();
    }
  }

  SECTION("invoke")
  {
    sum<int> a{std::in_place_type<int>, 42};

    SECTION("noexcept")
    {
      // invoke weighs the callback it dispatches to, in every value category
      constexpr auto throwing = [](int i) noexcept(false) -> bool { return i == 42; };
      static_assert(not noexcept(throwing(42)));
      static_assert(not noexcept(a.invoke(throwing)));
      static_assert(not noexcept(std::as_const(a).invoke(throwing)));
      static_assert(not noexcept(std::move(a).invoke(throwing)));
      static_assert(not noexcept(std::move(std::as_const(a)).invoke(throwing)));

      constexpr auto throwing2 = [](int i, std::monostate) noexcept(false) -> bool { return i == 42; };
      static_assert(not noexcept(a.invoke(throwing2, std::monostate{}))); // extra arguments, same promise

      constexpr auto nothrow = [](int i) noexcept -> bool { return i == 42; };
      static_assert(noexcept(a.invoke(nothrow)));
      static_assert(noexcept(std::move(a).invoke(nothrow)));
      SUCCEED();
    }

    SECTION("value only")
    {
      static_assert(std::is_same_v<bool, decltype(a.invoke(fn::overload{[](auto) -> bool { throw 1; },
                                                                        [](int) -> bool { return true; }}))>);

      // a result type other than bool, to witness the deduced return
      constexpr auto fn1 = [](auto i) noexcept -> std::size_t { return sizeof(i); };
      static_assert(std::is_same_v<std::size_t, decltype(a.invoke(fn1))>);
      CHECK(a.invoke(fn1) == sizeof(int));
      CHECK(a.data.v0 == 42); // white-box: the value went into the first alternative

      CHECK(a.invoke(fn::overload{[](auto) -> bool { throw 1; }, [](int &i) -> bool { return i == 42; },
                                  [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                  [](int const &&) -> bool { throw 0; }}));
      CHECK(std::as_const(a).invoke(fn::overload{
          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &i) -> bool { return i == 42; },
          [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; }}));
      CHECK(std::move(std::as_const(a))
                .invoke(fn::overload{[](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                     [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                     [](int const &&i) -> bool { return i == 42; }}));
      CHECK(std::move(a).invoke(fn::overload{
          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
          [](int &&i) -> bool { return i == 42; }, [](int const &&) -> bool { throw 0; }}));

      SECTION("constexpr")
      {
        constexpr sum<int> a{std::in_place_type<int>, 42};
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

    SECTION("extra arguments")
    {
      static_assert(std::is_same_v<bool, decltype(a.invoke([](int, int) -> bool { return true; }, 12))>);

      CHECK(a.invoke(fn::overload{                                                      //
                                  [](auto const &...) -> bool { throw 1; },             //
                                  [](int &, std::monostate) -> bool { return true; },   //
                                  [](int const &, std::monostate) -> bool { throw 0; }, //
                                  [](int &&, std::monostate) -> bool { throw 0; },      //
                                  [](int const &&, std::monostate) -> bool { throw 0; }},
                     std::monostate{}));
      CHECK(std::as_const(a).invoke(fn::overload{                                                          //
                                                 [](auto const &...) -> bool { throw 1; },                 //
                                                 [](int &, std::monostate) -> bool { throw 0; },           //
                                                 [](int const &, std::monostate) -> bool { return true; }, //
                                                 [](int &&, std::monostate) -> bool { throw 0; },          //
                                                 [](int const &&, std::monostate) -> bool { throw 0; }},
                                    std::monostate{}));
      CHECK(std::move(std::as_const(a))
                .invoke(fn::overload{                                                      //
                                     [](auto const &...) -> bool { throw 1; },             //
                                     [](int &, std::monostate) -> bool { throw 0; },       //
                                     [](int const &, std::monostate) -> bool { throw 0; }, //
                                     [](int &&, std::monostate) -> bool { throw 0; },      //
                                     [](int const &&, std::monostate) -> bool { return true; }},
                        std::monostate{}));
      CHECK(std::move(a).invoke(fn::overload{                                                      //
                                             [](auto const &...) -> bool { throw 1; },             //
                                             [](int &, std::monostate) -> bool { throw 0; },       //
                                             [](int const &, std::monostate) -> bool { throw 0; }, //
                                             [](int &&, std::monostate) -> bool { return true; },  //
                                             [](int const &&, std::monostate) -> bool { throw 0; }},
                                std::monostate{}));

      SECTION("constexpr")
      {
        constexpr sum<int> a{std::in_place_type<int>, 42};
        static_assert(a.invoke(fn::overload{[](auto...) -> bool { return false; }, //
                                            [](int &, std::monostate) -> bool { return false; },
                                            [](int const &, std::monostate) -> bool { return true; },
                                            [](int &&, std::monostate) -> bool { return false; },
                                            [](int const &&, std::monostate) -> bool { return false; }},
                               std::monostate{}));
        static_assert(std::move(a).invoke(fn::overload{[](auto...) -> bool { return false; }, //
                                                       [](int &, std::monostate) -> bool { return false; },
                                                       [](int const &, std::monostate) -> bool { return false; },
                                                       [](int &&, std::monostate) -> bool { return false; },
                                                       [](int const &&, std::monostate) -> bool { return true; }},
                                          std::monostate{}));
        static_assert(fn::invoke([](int i, std::monostate) -> bool { return i == 42; }, a, std::monostate{}));

        constexpr auto fn = [](auto &&...a) { return (0 + ... + static_cast<int>(a)); };
        static_assert(sum<bool, int>{2}.invoke(fn, 3) == 5);
      }
    }
  }

  SECTION("invoke_r")
  {
    sum<int> a{std::in_place_type<int>, 42};

    SECTION("noexcept")
    {
      // the same weighing as invoke, the conversion to Ret included
      constexpr auto throwing = [](int i) noexcept(false) -> bool { return i == 42; };
      static_assert(not noexcept(a.template invoke_r<bool>(throwing)));
      static_assert(not noexcept(std::as_const(a).template invoke_r<bool>(throwing)));
      static_assert(not noexcept(std::move(a).template invoke_r<bool>(throwing)));
      static_assert(not noexcept(std::move(std::as_const(a)).template invoke_r<bool>(throwing)));
      static_assert(not noexcept(a.template invoke_r<long>(throwing))); // converting the result, too

      constexpr auto nothrow = [](int i) noexcept -> bool { return i == 42; };
      static_assert(noexcept(a.template invoke_r<bool>(nothrow)));
      static_assert(noexcept(a.template invoke_r<long>(nothrow)));
      SUCCEED();
    }

    SECTION("value only")
    {
      static_assert(std::is_same_v<bool, decltype(a.template invoke_r<bool>(fn::overload{
                                             [](auto) -> bool { throw 1; }, [](int) -> bool { return true; }}))>);
      static_assert(
          std::is_same_v<int, decltype(a.template invoke_r<int>(fn::overload{[](auto) -> bool { throw 1; }, //
                                                                             [](int) -> int { return true; }}))>);

      CHECK(a.template invoke_r<bool>(fn::overload{
          [](auto) -> bool { throw 1; }, [](int &) -> bool { return true; }, [](int const &) -> bool { throw 0; },
          [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; }}));
      CHECK(std::as_const(a).template invoke_r<bool>(fn::overload{
          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { return true; },
          [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; }}));
      CHECK(std::move(std::as_const(a))
                .template invoke_r<bool>(fn::overload{
                    [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                    [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { return true; }}));
      CHECK(std::move(a).template invoke_r<bool>(fn::overload{
          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
          [](int &&) -> bool { return true; }, [](int const &&) -> bool { throw 0; }}));

      SECTION("constexpr")
      {
        constexpr sum<int> a{std::in_place_type<int>, 42};
        static_assert(a.template invoke_r<bool>(fn::overload{
            [](auto) -> std::false_type { return {}; }, //
            [](int &) -> std::false_type { return {}; }, [](int const &) -> std::true_type { return {}; },
            [](int &&) -> std::false_type { return {}; }, [](int const &&) -> std::false_type { return {}; }}));
        static_assert(std::move(a).template invoke_r<bool>(fn::overload{
            [](auto) -> std::false_type { return {}; }, //
            [](int &) -> std::false_type { return {}; }, [](int const &) -> std::false_type { return {}; },
            [](int &&) -> std::false_type { return {}; }, [](int const &&) -> std::true_type { return {}; }}));
        static_assert(
            fn::invoke_r<bool>([](int, std::monostate) -> std::true_type { return {}; }, a, std::monostate{}));
      }
    }

    SECTION("extra arguments")
    {
      static_assert(std::is_same_v<bool, decltype(a.template invoke_r<bool>(fn::overload{
                                             [](auto) -> bool { throw 1; }, [](int) -> bool { return true; }}))>);
      static_assert(
          std::is_same_v<int, decltype(a.template invoke_r<int>(fn::overload{[](auto) -> bool { throw 1; }, //
                                                                             [](int) -> int { return true; }}))>);
      CHECK(a.template invoke_r<bool>(fn::overload{                                                      //
                                                   [](auto const &...) -> bool { throw 1; },             //
                                                   [](int &, std::monostate) -> bool { return true; },   //
                                                   [](int const &, std::monostate) -> bool { throw 0; }, //
                                                   [](int &&, std::monostate) -> bool { throw 0; },      //
                                                   [](int const &&, std::monostate) -> bool { throw 0; }},
                                      std::monostate{}));
      CHECK(std::as_const(a).template invoke_r<bool>(
          fn::overload{                                                          //
                       [](auto const &...) -> bool { throw 1; },                 //
                       [](int &, std::monostate) -> bool { throw 0; },           //
                       [](int const &, std::monostate) -> bool { return true; }, //
                       [](int &&, std::monostate) -> bool { throw 0; },          //
                       [](int const &&, std::monostate) -> bool { throw 0; }},
          std::monostate{}));
      CHECK(std::move(std::as_const(a))
                .template invoke_r<bool>(fn::overload{                                                      //
                                                      [](auto const &...) -> bool { throw 1; },             //
                                                      [](int &, std::monostate) -> bool { throw 0; },       //
                                                      [](int const &, std::monostate) -> bool { throw 0; }, //
                                                      [](int &&, std::monostate) -> bool { throw 0; },      //
                                                      [](int const &&, std::monostate) -> bool { return true; }},
                                         std::monostate{}));
      CHECK(std::move(a).template invoke_r<bool>(fn::overload{                                                      //
                                                              [](auto const &...) -> bool { throw 1; },             //
                                                              [](int &, std::monostate) -> bool { throw 0; },       //
                                                              [](int const &, std::monostate) -> bool { throw 0; }, //
                                                              [](int &&, std::monostate) -> bool { return true; },  //
                                                              [](int const &&, std::monostate) -> bool { throw 0; }},
                                                 std::monostate{}));

      SECTION("constexpr")
      {
        constexpr sum<int> a{std::in_place_type<int>, 42};
        static_assert(
            a.template invoke_r<bool>(fn::overload{[](auto...) -> std::false_type { return {}; }, //
                                                   [](int &, std::monostate) -> std::false_type { return {}; },
                                                   [](int const &, std::monostate) -> std::true_type { return {}; },
                                                   [](int &&, std::monostate) -> std::false_type { return {}; },
                                                   [](int const &&, std::monostate) -> std::false_type { return {}; }},
                                      std::monostate{}));
        static_assert(std::move(a).template invoke_r<bool>(
            fn::overload{[](auto...) -> std::false_type { return {}; }, //
                         [](int &, std::monostate) -> std::false_type { return {}; },
                         [](int const &, std::monostate) -> std::false_type { return {}; },
                         [](int &&, std::monostate) -> std::false_type { return {}; },
                         [](int const &&, std::monostate) -> std::true_type { return {}; }},
            std::monostate{}));

        constexpr auto fn = [](auto &&...a) { return (0 + ... + static_cast<int>(a)); };
        static_assert(sum<bool, int>{2}.template invoke_r<long>(fn, 3) == 5l);
      }
    }
  }

  SECTION("sum of packs")
  {
    using fn::pack;
    constexpr sum a{pack{"abc", 42, 12.5}};
    static_assert(std::is_same_v<decltype(a), sum<pack<char const(&)[4], int, double>> const>);

    SECTION("constexpr")
    {
      constexpr auto b
          = a.invoke([]<std::size_t I>(char const(&)[I], int i, double d) { return I + i + static_cast<int>(d); });
      static_assert(b == 4 + 42 + 12);

      constexpr sum<pack<int, int, int, int>, pack<int, int, int>, pack<int, int>, pack<int>> c = pack{3, 14, 15};
      static_assert(c.invoke([](std::integral auto... args) -> int { return (... + args); }) == 3 + 14 + 15);

      SUCCEED();
    }

    SECTION("runtime")
    {
      auto const b = a.invoke([](char const *s, int i, double d) { return std::strlen(s) + i + static_cast<int>(d); });
      CHECK(b == 3 + 42 + 12);

      constexpr sum<pack<int, int, int, int>, pack<int, int, int>, pack<int, int>, pack<int>> c = pack{3, 14, 15, 92};
      CHECK(c.invoke([](std::integral auto... args) -> int { return (... + args); }) == 3 + 14 + 15 + 92);
    }
  }

  SECTION("structural type")
  {
    // sum is a structural type: a constexpr sum can be a template parameter, with
    // template-argument equivalence comparing the active alternative and its value
    constexpr sum<bool, int> a{42};
    constexpr sum<bool, int> b{42};
    constexpr sum<bool, int> c{17};
    constexpr sum<bool, int> d{true};
    static_assert(std::is_same_v<sum_nttp<a>, sum_nttp<b>>);
    static_assert(not std::is_same_v<sum_nttp<a>, sum_nttp<c>>);
    static_assert(not std::is_same_v<sum_nttp<a>, sum_nttp<d>>);
    static_assert(std::is_same_v<some_sum_nttp<a>, some_sum_nttp<b>>);
    static_assert(not std::is_same_v<some_sum_nttp<a>, some_sum_nttp<d>>);

    // the property composes: alternatives may be packs of different sizes mixed with a scalar
    using fn::pack;
    using S = fn::sum_for<pack<int, bool>, pack<double>, long>;
    constexpr S e{pack<int, bool>{42, true}};
    constexpr S f{pack<int, bool>{42, true}};
    constexpr S g{pack<int, bool>{43, true}};
    constexpr S h{pack<double>{0.5}};
    constexpr S i{42L};
    static_assert(std::is_same_v<some_sum_nttp<e>, some_sum_nttp<f>>);
    static_assert(not std::is_same_v<some_sum_nttp<e>, some_sum_nttp<g>>);
    static_assert(not std::is_same_v<some_sum_nttp<e>, some_sum_nttp<h>>);
    static_assert(not std::is_same_v<some_sum_nttp<h>, some_sum_nttp<i>>);

    // the template-parameter object is usable at runtime
    CHECK(read_nttp<a>() == 42.0);
    CHECK(read_nttp<e>() == 43.0); // the pack alternative spreads: 42 + true
    CHECK(read_nttp<i>() == 42.0);
  }
}

namespace {
struct PassThrough {
  auto operator()(std::equality_comparable auto &&v) const -> std::remove_cvref_t<decltype(v)> { return FWD(v); }
};
} // namespace

TEST_CASE("sum noexcept", "[sum][noexcept]")
{
  using fn::sum;

  struct Throwy {
    Throwy() = default;
    Throwy(Throwy const &) noexcept(false) {}
    Throwy(Throwy &&) noexcept(false) {}
    Throwy &operator=(Throwy const &) noexcept(false) { return *this; }
    Throwy &operator=(Throwy &&) noexcept(false) { return *this; }
    bool operator==(Throwy const &) const noexcept(false) { return true; }
  };
  struct Quiet {
    Quiet() = default;
    Quiet(Quiet const &) noexcept {}
    Quiet(Quiet &&) noexcept {}
    Quiet &operator=(Quiet const &) noexcept { return *this; }
    Quiet &operator=(Quiet &&) noexcept { return *this; }
    bool operator==(Quiet const &) const noexcept { return true; }
  };

  SECTION("constructors")
  {
    static_assert(noexcept(sum<int>{42}));
    static_assert(noexcept(sum<int>{std::in_place_type<int>, 42}));
    static_assert(noexcept(fn::as_sum(42)));
    static_assert(not noexcept(sum<Throwy>{std::declval<Throwy const &>()}));

    static_assert(std::is_nothrow_copy_constructible_v<sum<int>>);
    static_assert(std::is_nothrow_copy_constructible_v<sum<Quiet>>);
    static_assert(not std::is_nothrow_copy_constructible_v<sum<Throwy>>);
    static_assert(not std::is_nothrow_move_constructible_v<sum<Throwy>>);
    SUCCEED();
  }

  SECTION("dispatch")
  {
    using S = sum<bool, int>;
    constexpr auto nothrow_fn = [](auto) noexcept { return 0; };
    constexpr auto throwing_fn = [](auto) { return 0; };

    static_assert(noexcept(std::declval<S &>().invoke(nothrow_fn)));
    static_assert(not noexcept(std::declval<S &>().invoke(throwing_fn)));
    static_assert(noexcept(std::declval<S &>().template invoke_r<int>(nothrow_fn)));
    static_assert(not noexcept(std::declval<S &>().template invoke_r<int>(throwing_fn)));
    static_assert(noexcept(std::declval<S &>().transform(nothrow_fn)));
    static_assert(not noexcept(std::declval<S &>().transform(throwing_fn)));

    // which alternative runs is a run-time choice, so one throwing handler makes the whole
    // dispatch throwing
    constexpr auto mixed = fn::overload{[](int) noexcept { return 0; }, [](bool) { return 0; }};
    static_assert(not noexcept(std::declval<S &>().invoke(mixed)));
    SUCCEED();
  }

  SECTION("comparison")
  {
    static_assert(noexcept(std::declval<sum<int> const &>() == std::declval<sum<int> const &>()));
    static_assert(noexcept(std::declval<sum<Quiet> const &>() == std::declval<sum<Quiet> const &>()));
    static_assert(not noexcept(std::declval<sum<Throwy> const &>() == std::declval<sum<Throwy> const &>()));

    // the rewritten != inherits it
    static_assert(noexcept(std::declval<sum<int> const &>() != std::declval<sum<int> const &>()));
    static_assert(not noexcept(std::declval<sum<Throwy> const &>() != std::declval<sum<Throwy> const &>()));
    SUCCEED();
  }

  SECTION("assignment")
  {
    // assignment weighs the alternative's own `operator=` - used when the alternative does not
    // change - and the construction that replaces it when it does
    static_assert(std::is_nothrow_copy_assignable_v<sum<int>>);
    static_assert(std::is_nothrow_copy_assignable_v<sum<Quiet>>);
    static_assert(std::is_nothrow_move_assignable_v<sum<int>>);
    static_assert(not std::is_nothrow_copy_assignable_v<sum<std::string>>);
    static_assert(std::is_nothrow_move_assignable_v<sum<std::string>>);

    // a throwing move constructor is constrained away - the replacement path has nowhere to fail
    // safely - and Throwy's throwing everything leaves no nothrow arm at all
    static_assert(not std::is_move_assignable_v<sum<Throwy>>);
    static_assert(not std::is_copy_assignable_v<sum<Throwy>>);

    // widening assignment weighs only the source's alternatives: Throwy, uninvolved, neither
    // forbids the assignment nor enters its specification
    static_assert(std::is_assignable_v<fn::sum_for<Quiet, Throwy> &, sum<Quiet> const &>);
    static_assert(noexcept(std::declval<fn::sum_for<Quiet, Throwy> &>() = std::declval<sum<Quiet> const &>()));
    struct Loud { // nothrow to deliver, throwing to assign: viability and the specification differ
      Loud() = default;
      Loud(Loud const &) noexcept {}
      Loud(Loud &&) noexcept {}
      Loud &operator=(Loud const &) noexcept(false) { return *this; }
      Loud &operator=(Loud &&) noexcept(false) { return *this; }
    };
    static_assert(std::is_assignable_v<fn::sum_for<Quiet, Loud> &, sum<Loud> const &>);
    static_assert(not noexcept(std::declval<fn::sum_for<Quiet, Loud> &>() = std::declval<sum<Loud> const &>()));
    SUCCEED();
  }
}

TEST_CASE("sum triviality", "[sum][triviality]")
{
  using fn::sum;

  SECTION("of fundamentals: every operation, as variant has")
  {
    using S = sum<double, int>;
    static_assert(std::is_trivially_copyable_v<S>);
    static_assert(std::is_trivially_destructible_v<S>);
    static_assert(std::is_trivially_copy_constructible_v<S>);
    static_assert(std::is_trivially_move_constructible_v<S>);
    static_assert(std::is_trivially_copy_assignable_v<S>);
    static_assert(std::is_trivially_move_assignable_v<S>);
    SUCCEED();
  }

  SECTION("of packs: the multidispatch case")
  {
    // a sum of packs is the normal form of the type algebra, and the join's cartesian product is
    // what a multidispatch pipeline copies at every stage
    using S = sum<fn::pack<int, double>>;
    static_assert(std::is_trivially_copyable_v<S>);
    static_assert(std::is_trivially_destructible_v<S>);
    static_assert(std::is_trivially_copy_constructible_v<S>);
    static_assert(std::is_trivially_move_constructible_v<S>);
    static_assert(std::is_trivially_copy_assignable_v<S>);
    static_assert(std::is_trivially_move_assignable_v<S>);
    SUCCEED();
  }

  SECTION("each operation follows its own gate")
  {
    // a pack holding a reference is trivially copyable and refuses assignment: the operations
    // decouple, exactly as they do on the pack itself
    using R = sum<fn::pack<int, int &>>;
    static_assert(std::is_trivially_destructible_v<R>);
    static_assert(std::is_trivially_copy_constructible_v<R>);
    static_assert(std::is_trivially_move_constructible_v<R>);
    static_assert(not std::is_copy_assignable_v<R>); // refused by the alternative, not merely non-trivial
    static_assert(not std::is_move_assignable_v<R>);

    // nothing about std::string is trivial, and everything still works
    using N = sum<std::string>;
    static_assert(not std::is_trivially_destructible_v<N>);
    static_assert(not std::is_trivially_copy_constructible_v<N>);
    static_assert(not std::is_trivially_copy_assignable_v<N>);
    static_assert(std::is_copy_assignable_v<N>);

    // one non-trivial alternative makes the operation non-trivial, never non-viable
    using M = fn::sum_for<std::string, int>;
    static_assert(not std::is_trivially_copy_assignable_v<M>);
    static_assert(std::is_copy_assignable_v<M>);
    SUCCEED();
  }
}

TEST_CASE("sum type collapsing", "[sum][transform][normalized]")
{
  using ::fn::overload;
  using ::fn::sum;
  using ::fn::detail::_collapsing_sum_tag;
  using ::fn::detail::_sum_invoke_result;
  using ::fn::detail::_typelist_collapsing_sum;

  struct sum_double_int {};
  struct sum_bool {};
  struct sum_bool_int {};

  SECTION("one element")
  {
    constexpr auto fn = PassThrough{};
    using type = sum<double>;
    static_assert(
        std::same_as<typename _sum_invoke_result<_collapsing_sum_tag, decltype(fn), type &>::type, sum<double>>);
  }

  SECTION("two elements")
  {
    constexpr auto fn = PassThrough{};
    using type = sum<double, int>;
    static_assert(
        std::same_as<typename _sum_invoke_result<_collapsing_sum_tag, decltype(fn), type &>::type, sum<double, int>>);
  }

  SECTION("one sum, one element only")
  {
    constexpr auto fn = [](sum_bool const &) -> sum<bool> && { throw 0; };
    using type = sum<sum_bool>;
    static_assert(
        std::same_as<typename _sum_invoke_result<_collapsing_sum_tag, decltype(fn), type &>::type, sum<bool>>);
  }

  SECTION("element and one sum with one element")
  {
    constexpr auto fn = overload{PassThrough{}, //
                                 [](sum_bool const &) -> sum<bool> && { throw 0; }};
    using type = sum<double, sum_bool>;
    static_assert(
        std::same_as<typename _sum_invoke_result<_collapsing_sum_tag, decltype(fn), type &>::type, sum<bool, double>>);
  }

  SECTION("one sum with two elements")
  {
    constexpr auto fn = [](sum_bool_int const &) -> sum<bool, int> && { throw 0; };
    using type = sum<sum_bool_int>;
    static_assert(
        std::same_as<typename _sum_invoke_result<_collapsing_sum_tag, decltype(fn), type &>::type, sum<bool, int>>);
  }

  SECTION("sum with two elements and sum with one element")
  {
    constexpr auto fn = overload{[](sum_bool_int const &) -> sum<bool, int> && { throw 0; },
                                 [](sum_bool const &) -> sum<bool> && { throw 0; }};
    using type = sum<sum_bool_int, sum_bool>;
    static_assert(
        std::same_as<typename _sum_invoke_result<_collapsing_sum_tag, decltype(fn), type &>::type, sum<bool, int>>);
  }

  SECTION("two sums with two elements and two elements")
  {
    constexpr auto fn = overload{PassThrough{}, [](sum_double_int const &) -> sum<double, int> { throw 0; },
                                 [](sum_bool_int const &) -> sum<bool, int> const { throw 0; }};
    using type = sum<sum_bool_int, sum_double_int, double, int>;
    static_assert(std::same_as<typename _sum_invoke_result<_collapsing_sum_tag, decltype(fn), type &>::type,
                               sum<bool, double, int>>);
  }

  SECTION("a result no sum can hold")
  {
    // a void-returning callback must drop the caller's candidate in the immediate context: the
    // collapsing machinery would hard-error where no requires-expression can absorb it
    constexpr auto fnVoid = [](auto &&...) {};
    static_assert(not can_transform<sum<double, int> &, decltype(fnVoid)>);
    static_assert(not can_transform<sum<double, int> const &, decltype(fnVoid)>);
    static_assert(not can_transform<sum<double, int> &&, decltype(fnVoid)>);
    static_assert(not can_transform<sum<double, int> const &&, decltype(fnVoid)>);
    static_assert(can_transform<sum<double, int> &, PassThrough>); // the same sum, a holdable result
    SUCCEED();
  }
}

TEST_CASE("sum transform", "[sum][transform]")
{
  using ::fn::sum;
  constexpr auto fn1 = [](auto i) noexcept -> std::size_t { return sizeof(i); };

  using type = sum<double>;
  static_assert(type::size == 1);

  type a{std::in_place_type<double>, 0.5};
  CHECK(a.data.v0 == 0.5);

  static_assert(type{0.5}.transform(fn1) == sum{std::size_t{8}});
  CHECK(a.transform(     //
            fn::overload{//
                         [](auto) -> int { throw 1; }, [](double &i) -> bool { return i == 0.5; },
                         [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                         [](double const &&) -> bool { throw 0; }})
        == sum<bool, int>{true});
  CHECK(std::as_const(a).transform( //
            fn::overload{           //
                         [](auto) -> int { throw 1; }, [](double &) -> bool { throw 0; },
                         [](double const &i) -> bool { return i == 0.5; }, [](double &&) -> bool { throw 0; },
                         [](double const &&) -> bool { throw 0; }})
        == sum<bool, int>{true});
  CHECK(std::move(std::as_const(a))
            .transform(      //
                fn::overload{//
                             [](auto) -> int { throw 1; }, [](double &) -> bool { throw 0; },
                             [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                             [](double const &&i) -> bool { return i == 0.5; }})
        == sum<bool, int>{true});
  CHECK(std::move(a).transform( //
            fn::overload{       //
                         [](auto) -> int { throw 1; }, [](double &) -> bool { throw 0; },
                         [](double const &) -> bool { throw 0; }, [](double &&i) -> bool { return i == 0.5; },
                         [](double const &&) -> bool { throw 0; }})
        == sum<bool, int>{true});

  SECTION("extra arguments")
  {
    constexpr auto add = [](double i, int j) noexcept -> double { return i + j; };
    static_assert(std::same_as<decltype(a.transform(add, 3)), sum<double>>);

    CHECK(a.transform(add, 3) == sum{3.5});
    CHECK(std::as_const(a).transform(add, 3) == sum{3.5});
    CHECK(std::move(std::as_const(a)).transform(add, 3) == sum{3.5});
    CHECK(std::move(a).transform(add, 3) == sum{3.5});

    SECTION("constexpr")
    {
      constexpr type b{std::in_place_type<double>, 0.5};
      static_assert(b.transform(add, 3) == sum{3.5});
      static_assert(std::move(b).transform(add, 3) == sum{3.5});
      SUCCEED();
    }
  }

  SECTION("noexcept")
  {
    // transform weighs the callback in every value category, as do the typed-dispatch internals
    // behind operator& and the widening constructors
    constexpr auto throwing = [](double i) noexcept(false) -> bool { return i == 0.5; };
    static_assert(not noexcept(a.transform(throwing)));
    static_assert(not noexcept(std::as_const(a).transform(throwing)));
    static_assert(not noexcept(std::move(a).transform(throwing)));
    static_assert(not noexcept(std::move(std::as_const(a)).transform(throwing)));

    constexpr auto typed = []<typename T>(std::in_place_type_t<T>, auto &&v) noexcept(false) -> bool {
      return static_cast<double>(v) == 0.5;
    };
    static_assert(not noexcept(std::as_const(a)._transform(typed)));
    static_assert(not noexcept(std::move(a)._transform(typed)));
    static_assert(not noexcept(std::as_const(a).template _invoke<bool>(typed)));
    static_assert(not noexcept(std::move(a).template _invoke<bool>(typed)));

    constexpr auto nothrow = [](double i) noexcept -> bool { return i == 0.5; };
    static_assert(noexcept(a.transform(nothrow)));
    static_assert(noexcept(std::move(a).transform(nothrow)));

    SUCCEED();
  }
}

namespace {
struct MoveOnly final {
  int v;

  constexpr operator int() const { return v; }
  constexpr MoveOnly(int i) noexcept : v(i) {}
  MoveOnly(MoveOnly const &) = delete;
  constexpr MoveOnly(MoveOnly &&s) : v(s.v) { s.v = -1; }

  MoveOnly &operator=(MoveOnly const &) = delete;
  constexpr MoveOnly &operator=(MoveOnly &&s) = delete;
};

struct CopyOnly final {
  int v;

  constexpr operator int() const { return v; }
  constexpr CopyOnly(int i) noexcept : v(i) {}
  constexpr CopyOnly(CopyOnly const &) = default;
  constexpr CopyOnly(CopyOnly &&s) = delete;

  constexpr CopyOnly &operator=(CopyOnly const &) = delete;
  constexpr CopyOnly &operator=(CopyOnly &&s) = delete;
};
} // anonymous namespace

TEST_CASE("sum move and copy", "[sum][has_value][get_ptr]")
{
  using fn::sum;

  SECTION("move and copy")
  {
    SECTION("one type only")
    {
      using T = sum<std::string>;
      T a{std::in_place_type<std::string>, "baz"};
      CHECK(a.invoke([](auto &&i) { return i; }) == "baz");

      static_assert([](auto &&a) { // OK copy
        return requires { static_cast<T>(FWD(a)); };
      }(a));
      static_assert([](auto &&a) { // OK copy
        return requires { static_cast<T>(FWD(a)); };
      }(std::as_const(a)));
      static_assert([](auto &&a) { // OK move
        return requires { static_cast<T>(FWD(a)); };
      }(std::move(a)));
      static_assert([](auto &&a) { // OK copy
        return requires { static_cast<T>(FWD(a)); };
      }(std::move(std::as_const(a))));

      T b{a};
      CHECK(a.invoke([](auto &&i) { return i; }) == "baz");
      CHECK(b.invoke([](auto &&i) { return i; }) == "baz");

      T c{std::move(a)};
      CHECK(c.invoke([](auto &&i) { return i; }) == "baz");
    }

    SECTION("mixed with other types")
    {
      using T = sum<std::string, std::string_view>;
      T a{std::in_place_type<std::string>, "baz"};
      CHECK(a.invoke([](auto &&i) { return std::string(i); }) == "baz");

      static_assert([](auto &&a) { // OK copy
        return requires { static_cast<T>(FWD(a)); };
      }(a));
      static_assert([](auto &&a) { // OK copy
        return requires { static_cast<T>(FWD(a)); };
      }(std::as_const(a)));
      static_assert([](auto &&a) { // OK move
        return requires { static_cast<T>(FWD(a)); };
      }(std::move(a)));
      static_assert([](auto &&a) { // OK copy
        return requires { static_cast<T>(FWD(a)); };
      }(std::move(std::as_const(a))));

      T b{a};
      CHECK(a.invoke([](auto &&i) { return std::string(i); }) == "baz");
      CHECK(b.invoke([](auto &&i) { return std::string(i); }) == "baz");

      T c{std::move(a)};
      CHECK(c.invoke([](auto &&i) { return std::string(i); }) == "baz");
    }
  }

  SECTION("copy only")
  {
    SECTION("one type only")
    {
      using T = sum<CopyOnly>;
      T a{std::in_place_type<CopyOnly>, 12};
      CHECK(a.invoke([](auto &&i) { return static_cast<int>(i); }) == 12);

      static_assert([](auto &&a) { // OK copy
        return requires { static_cast<T>(FWD(a)); };
      }(a));
      static_assert([](auto &&a) { // OK copy
        return requires { static_cast<T>(FWD(a)); };
      }(std::as_const(a)));
      static_assert([](auto &&a) { // OK copy (binding rvalue to const lvalue-reference)
        return requires { static_cast<T>(FWD(a)); };
      }(std::move(a)));
      static_assert([](auto &&a) { // OK copy
        return requires { static_cast<T>(FWD(a)); };
      }(std::move(std::as_const(a))));

      T b{a};
      CHECK(a.invoke([](auto &&i) { return static_cast<int>(i); }) == 12);
      CHECK(b.invoke([](auto &&i) { return static_cast<int>(i); }) == 12);
    }

    SECTION("mixed with other types")
    {
      using T = fn::sum_for<CopyOnly, double, int>; // sum_for: canonical order is platform-specific
      T a{std::in_place_type<CopyOnly>, 12};
      CHECK(a.invoke([](auto &&i) { return static_cast<int>(i); }) == 12);

      static_assert([](auto &&a) { // OK copy
        return requires { static_cast<T>(FWD(a)); };
      }(a));
      static_assert([](auto &&a) { // OK copy
        return requires { static_cast<T>(FWD(a)); };
      }(std::as_const(a)));
      static_assert([](auto &&a) { // OK copy (binding rvalue to const lvalue-reference)
        return requires { static_cast<T>(FWD(a)); };
      }(std::move(a)));
      static_assert([](auto &&a) { // OK copy
        return requires { static_cast<T>(FWD(a)); };
      }(std::move(std::as_const(a))));

      T b{a};
      CHECK(a.invoke([](auto &&i) { return static_cast<int>(i); }) == 12);
      CHECK(b.invoke([](auto &&i) { return static_cast<int>(i); }) == 12);
    }
  }

  SECTION("move only")
  {
    SECTION("one type only")
    {
      using T = sum<MoveOnly>;
      T a{std::in_place_type<MoveOnly>, 12};
      CHECK(a.invoke([](auto &&i) { return static_cast<int>(i); }) == 12);

      static_assert([](auto &&a) { // cannot copy from lvalue
        return not requires { static_cast<T>(FWD(a)); };
      }(a));
      static_assert([](auto &&a) { // cannot copy from const lvalue
        return not requires { static_cast<T>(FWD(a)); };
      }(std::as_const(a)));
      static_assert([](auto &&a) { // OK move
        return requires { static_cast<T>(FWD(a)); };
      }(std::move(a)));
      static_assert([](auto &&a) { // cannot copy from const rvalue
        return not requires { static_cast<T>(FWD(a)); };
      }(std::move(std::as_const(a))));

      T b{std::move(a)};
      CHECK(a.invoke([](auto &&i) { return static_cast<int>(i); }) == -1);
      CHECK(b.invoke([](auto &&i) { return static_cast<int>(i); }) == 12);
    }

    SECTION("mixed with other types")
    {
      using T = fn::sum_for<MoveOnly, double, int>; // sum_for: canonical order is platform-specific
      T a{std::in_place_type<MoveOnly>, 12};
      CHECK(a.invoke([](auto &&i) { return static_cast<int>(i); }) == 12);

      static_assert([](auto &&a) { // cannot copy from lvalue
        return not requires { static_cast<T>(FWD(a)); };
      }(a));
      static_assert([](auto &&a) { // cannot copy from const lvalue
        return not requires { static_cast<T>(FWD(a)); };
      }(std::as_const(a)));
      static_assert([](auto &&a) { // OK move
        return requires { static_cast<T>(FWD(a)); };
      }(std::move(a)));
      static_assert([](auto &&a) { // cannot copy from const rvalue
        return not requires { static_cast<T>(FWD(a)); };
      }(std::move(std::as_const(a))));

      T b{std::move(a)};
      CHECK(a.invoke([](auto &&i) { return static_cast<int>(i); }) == -1);
      CHECK(b.invoke([](auto &&i) { return static_cast<int>(i); }) == 12);
    }
  }

  SECTION("immovable type")
  {
    SECTION("one type only")
    {
      using T = sum<NonCopyable>;
      T a{std::in_place_type<NonCopyable>, 12};
      CHECK(a.invoke([](auto &&i) { return static_cast<int>(i); }) == 12);

      static_assert([](auto &&a) { // cannot copy from lvalue
        return not requires { static_cast<T>(FWD(a)); };
      }(a));
      static_assert([](auto &&a) { // cannot copy from const lvalue
        return not requires { static_cast<T>(FWD(a)); };
      }(std::as_const(a)));
      static_assert([](auto &&a) { // cannot move from rvalue
        return not requires { static_cast<T>(FWD(a)); };
      }(std::move(a)));
      static_assert([](auto &&a) { // cannot copy from const rvalue
        return not requires { static_cast<T>(FWD(a)); };
      }(std::move(std::as_const(a))));
    }

    SECTION("mixed with other types")
    {
      using T = fn::sum_for<NonCopyable, double, int>; // sum_for: canonical order is platform-specific
      T a{std::in_place_type<NonCopyable>, 12};
      CHECK(a.invoke([](auto &&i) { return static_cast<int>(i); }) == 12);

      static_assert([](auto &&a) { // cannot copy from lvalue
        return not requires { static_cast<T>(FWD(a)); };
      }(a));
      static_assert([](auto &&a) { // cannot copy from const lvalue
        return not requires { static_cast<T>(FWD(a)); };
      }(std::as_const(a)));
      static_assert([](auto &&a) { // cannot move from rvalue
        return not requires { static_cast<T>(FWD(a)); };
      }(std::move(a)));
      static_assert([](auto &&a) { // cannot copy from const rvalue
        return not requires { static_cast<T>(FWD(a)); };
      }(std::move(std::as_const(a))));
    }
  }

  SECTION("widening")
  {
    // The widening pair carries the copy/move constructors' requirements over to the SOURCE's
    // alternatives: copy-widening needs each of them copyable, move-widening needs each movable.
    static_assert(std::is_constructible_v<fn::sum_for<CopyOnly, int>, sum<CopyOnly> const &>);
    static_assert(std::is_constructible_v<fn::sum_for<CopyOnly, int>, sum<CopyOnly> &&>); // binds const &

    static_assert(not std::is_constructible_v<fn::sum_for<MoveOnly, int>, sum<MoveOnly> const &>);
    static_assert(std::is_constructible_v<fn::sum_for<MoveOnly, int>, sum<MoveOnly> &&>);

    static_assert(not std::is_constructible_v<fn::sum_for<NonCopyable, int>, sum<NonCopyable> const &>);
    static_assert(not std::is_constructible_v<fn::sum_for<NonCopyable, int>, sum<NonCopyable> &&>);

    sum<MoveOnly> a{std::in_place_type<MoveOnly>, 12};
    fn::sum_for<MoveOnly, int> b{std::move(a)};
    CHECK(b.invoke([](auto &&i) { return static_cast<int>(i); }) == 12);
    CHECK(a.invoke([](auto &&i) { return static_cast<int>(i); }) == -1); // moved from
  }

  SECTION("noexcept")
  {
    // the copy and move constructors weigh the alternative they relocate; the destructor cannot
    // throw whatever the alternatives are
    static_assert(not std::is_nothrow_copy_constructible_v<Throwing>);
    static_assert(not std::is_nothrow_move_constructible_v<Throwing>);
    static_assert(not std::is_nothrow_copy_constructible_v<sum<Throwing>>);
    static_assert(not std::is_nothrow_move_constructible_v<sum<Throwing>>);
    static_assert(std::is_nothrow_destructible_v<sum<Throwing>>);

    static_assert(std::is_nothrow_copy_constructible_v<sum<int>>);
    static_assert(std::is_nothrow_move_constructible_v<sum<int>>);
    SUCCEED();
  }
}

TEST_CASE("sum assignment", "[sum][assignment]")
{
  using fn::sum;

  SECTION("same alternative")
  {
    sum<bool, int> a{12};
    sum<bool, int> const b{42};
    a = b;
    CHECK(a == sum{42});
    CHECK(a.has_value(std::in_place_type<int>));

    a = sum<bool, int>{7};
    CHECK(a == sum{7});
  }

  SECTION("the alternative changes")
  {
    sum<bool, int> a{12};
    sum<bool, int> const b{true};
    a = b;
    CHECK(a == sum{true});
    CHECK(a.has_value(std::in_place_type<bool>));
    CHECK(not a.has_value(std::in_place_type<int>));

    a = sum<bool, int>{42};
    CHECK(a == sum{42});
    CHECK(a.has_value(std::in_place_type<int>));
  }

  SECTION("self-assignment")
  {
    sum<bool, int> a{12};
    sum<bool, int> const &self = a; // through an alias: `a = a` is a warning, and rightly so
    a = self;
    CHECK(a == sum{12});
    CHECK(a.has_value(std::in_place_type<int>));
  }

  SECTION("the alternative's own operator= is used")
  {
    // when the incoming alternative is the one already held, it is assigned - not destroyed and
    // rebuilt - so the alternative's own assignment operator is what runs
    struct Tracked final {
      int v;
      bool assigned = false;
      constexpr Tracked(int i) noexcept : v(i) {} // NOLINT: implicit on purpose
      constexpr Tracked(Tracked const &o) noexcept : v(o.v) {}
      constexpr Tracked(Tracked &&o) noexcept : v(o.v) {}
      constexpr Tracked &operator=(Tracked const &o) noexcept
      {
        v = o.v;
        assigned = true;
        return *this;
      }
      constexpr Tracked &operator=(Tracked &&o) noexcept
      {
        v = o.v;
        assigned = true;
        return *this;
      }
    };
    using T = fn::sum_for<Tracked, int>;
    constexpr auto probe
        = fn::overload{[](Tracked const &t) { return t.assigned ? t.v : -t.v; }, [](int const &) { return 0; }};

    T a{std::in_place_type<Tracked>, 7};
    a = T{std::in_place_type<Tracked>, 5};
    CHECK(a.invoke(probe) == 5); // assigned, in place

    a = T{12}; // a different alternative is replaced by construction ...
    a = T{std::in_place_type<Tracked>, 3};
    CHECK(a.invoke(probe) == -3); // ... so this Tracked was constructed, not assigned

    static_assert([] {
      T a{std::in_place_type<Tracked>, 7};
      a = T{std::in_place_type<Tracked>, 5};
      return a.invoke([](auto const &t) {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(t)>, Tracked>)
          return t.assigned && t.v == 5;
        else
          return false;
      });
    }());
  }

  SECTION("the replaced alternative is destroyed")
  {
    using T = fn::sum_for<Counted, int>; // sum<...> order is platform-specific; sum_for normalizes
    Counted::live = 0;
    {
      T a{std::in_place_type<Counted>, 7};
      CHECK(Counted::live == 1);
      a = T{12}; // the Counted goes
      CHECK(Counted::live == 0);
      a = T{std::in_place_type<Counted>, 9};
      CHECK(Counted::live == 1);
      a = T{std::in_place_type<Counted>, 3}; // same alternative: assigned in place, nothing is destroyed
      CHECK(Counted::live == 1);
    }
    CHECK(Counted::live == 0);
  }

  SECTION("widening")
  {
    // a narrower sum assigns on the widening constructors' terms, deciding per incoming
    // alternative: the one held is assigned in place, any other replaces by construction
    constexpr auto basic = [] {
      sum<bool, int> a{12};
      sum<int> const n{42};
      a = n; // the alternative in hand, by copy
      bool ok = a == sum{42};
      a = sum<bool>{true}; // a different alternative
      ok = ok && a == sum{true};
      a = sum<int>{7}; // and back, by move
      return ok && a == sum{7};
    };
    CHECK(basic());
    static_assert(basic());

    SECTION("one relocation, or none")
    {
      // `wide = narrow` used to route through the widening constructor: a whole temporary sum, one
      // copy plus one move; the incoming alternative is now delivered straight to its destination
      struct Reloc final {
        int v;
        int *copied;
        int *moved;
        int *assigned;
        constexpr Reloc(int i, int *c, int *m, int *a) noexcept : v(i), copied(c), moved(m), assigned(a) {}
        constexpr Reloc(Reloc const &o) noexcept : v(o.v), copied(o.copied), moved(o.moved), assigned(o.assigned)
        {
          ++*copied;
        }
        constexpr Reloc(Reloc &&o) noexcept : v(o.v), copied(o.copied), moved(o.moved), assigned(o.assigned)
        {
          ++*moved;
        }
        constexpr Reloc &operator=(Reloc const &o) noexcept
        {
          v = o.v;
          ++*assigned;
          return *this;
        }
        constexpr Reloc &operator=(Reloc &&o) noexcept
        {
          v = o.v;
          ++*assigned;
          return *this;
        }
      };
      constexpr auto counts = [] {
        int copied = 0;
        int moved = 0;
        int assigned = 0;
        using W = fn::sum_for<Reloc, int>;
        fn::sum<Reloc> n{Reloc{42, &copied, &moved, &assigned}};
        fn::sum<Reloc> m{Reloc{9, &copied, &moved, &assigned}};

        W a{12};
        copied = moved = assigned = 0;
        a = std::as_const(n); // a different alternative, by copy: one copy, straight into place
        bool ok = copied == 1 && moved == 0 && assigned == 0 && a.get_ptr<Reloc>()->v == 42;

        copied = moved = assigned = 0;
        a = std::as_const(n); // the alternative in hand: assigned, nothing relocates
        ok = ok && copied == 0 && moved == 0 && assigned == 1;

        a = W{12};
        copied = moved = assigned = 0;
        a = std::move(m); // a different alternative, by move: one move, nothing else
        return ok && copied == 0 && moved == 1 && assigned == 0 && a.get_ptr<Reloc>()->v == 9;
      };
      CHECK(counts());
      static_assert(counts());
    }

    SECTION("constraints")
    {
      // constrained on the source's alternatives: an uninvolved alternative of the destination
      // forbids nothing - though it still forbids assignment of the whole type - and decides only
      // when the source can actually deliver it
      struct Fixed final { // copyable, not assignable
        constexpr Fixed() noexcept = default;
        constexpr Fixed(Fixed const &) noexcept = default;
        constexpr Fixed(Fixed &&) noexcept = default;
        Fixed &operator=(Fixed const &) = delete;
        Fixed &operator=(Fixed &&) = delete;
      };
      using S = fn::sum_for<Fixed, int>;
      static_assert(not std::is_copy_assignable_v<S>);
      static_assert(std::is_assignable_v<S &, sum<int> const &>);
      static_assert(std::is_assignable_v<S &, sum<int> &&>);
      static_assert(not std::is_assignable_v<S &, sum<Fixed> const &>);

      struct Skittish final { // assignable, but no arm can deliver it without throwing
        constexpr Skittish() noexcept = default;
        Skittish(Skittish const &) noexcept(false) {}
        Skittish(Skittish &&) noexcept(false) {}
        Skittish &operator=(Skittish const &) noexcept { return *this; }
        Skittish &operator=(Skittish &&) noexcept { return *this; }
      };
      using S2 = fn::sum_for<Skittish, int>;
      static_assert(not std::is_copy_assignable_v<S2>);
      static_assert(std::is_assignable_v<S2 &, sum<int> const &>);
      static_assert(not std::is_assignable_v<S2 &, sum<Skittish> const &>);

      // the constructor route survives where the direct overload declines: a copy-deleted move
      // assignment refuses widening-copy, and `wide = narrow` still compiles as it always did -
      // the widening constructor's temporary, then whole-type assignment
      struct MoveAssign final {
        constexpr MoveAssign() noexcept = default;
        constexpr MoveAssign(MoveAssign const &) noexcept = default;
        constexpr MoveAssign(MoveAssign &&) noexcept = default;
        MoveAssign &operator=(MoveAssign const &) = delete;
        constexpr MoveAssign &operator=(MoveAssign &&) noexcept = default;
      };
      static_assert(not std::is_copy_assignable_v<MoveAssign>);
      using S3 = fn::sum_for<MoveAssign, int>;
      static_assert(std::is_assignable_v<S3 &, sum<MoveAssign> const &>); // through the constructor
      static_assert(std::is_assignable_v<S3 &, sum<MoveAssign> &&>);      // directly

      // a sum it is not a superset of is refused
      static_assert(not std::is_assignable_v<sum<int> &, sum<bool> const &>);
      SUCCEED();
    }
  }

  SECTION("constexpr")
  {
    static_assert([] {
      sum<bool, int> a{42};
      sum<bool, int> const b{true};
      a = b;
      return a == sum{true};
    }());
    static_assert([] {
      sum<bool, int> a{true};
      a = sum<bool, int>{42};
      return a == sum{42};
    }());
    static_assert([] {
      sum<bool, int> a{42};
      sum<bool, int> const &self = a;
      a = self;
      return a == sum{42};
    }());
    SUCCEED();
  }

  SECTION("strong exception guarantee")
  {
    SECTION("a throwing assignment is rolled back")
    {
      // where the alternative's own operator= can throw in every form, the value in hand is
      // snapshot by its (nothrow) move first, and restored if the assignment fails
      struct SnapAssign final {
        int v;
        constexpr SnapAssign(int i) noexcept : v(i) {} // NOLINT: implicit on purpose
        constexpr SnapAssign(SnapAssign const &) noexcept = default;
        constexpr SnapAssign(SnapAssign &&) noexcept = default;
        constexpr SnapAssign &operator=(SnapAssign const &o)
        {
          if (o.v == 0)
            throw std::runtime_error("assign");
          v = o.v;
          return *this;
        }
        constexpr SnapAssign &operator=(SnapAssign &&o)
        {
          if (o.v == 0)
            throw std::runtime_error("assign");
          v = o.v;
          return *this;
        }
      };
      static_assert(std::is_copy_assignable_v<sum<SnapAssign>>);
      static_assert(not std::is_nothrow_copy_assignable_v<sum<SnapAssign>>);
      constexpr auto value = [](SnapAssign const &t) { return t.v; };

      sum<SnapAssign> a{SnapAssign{7}};
      sum<SnapAssign> const bad{SnapAssign{0}};
      CHECK_THROWS_AS(a = bad, std::runtime_error);
      CHECK(a.invoke(value) == 7); // restored

      // the same arm, completing
      sum<SnapAssign> const good{SnapAssign{5}};
      a = good;
      CHECK(a.invoke(value) == 5);

      static_assert([] {
        sum<SnapAssign> a{SnapAssign{7}};
        sum<SnapAssign> const good{SnapAssign{5}};
        a = good; // the snapshot arm, in a constant expression
        return a.invoke([](SnapAssign const &t) { return t.v; }) == 5;
      }());
    }

    SECTION("a throwing copy is made into a temporary")
    {
      // where the copy construction can throw but the move assignment cannot, the copy is made
      // into a temporary first, and only its (nothrow) move assignment touches the value in hand
      struct TempAssign final {
        int v;
        constexpr TempAssign(int i) noexcept : v(i) {} // NOLINT: implicit on purpose
        constexpr TempAssign(TempAssign const &o) : v(o.v)
        {
          if (v == 0)
            throw std::runtime_error("copy");
        }
        constexpr TempAssign(TempAssign &&o) noexcept : v(o.v) {}
        constexpr TempAssign &operator=(TempAssign const &o) // NOLINT: not noexcept on purpose
        {
          v = o.v;
          return *this;
        }
        constexpr TempAssign &operator=(TempAssign &&o) noexcept
        {
          v = o.v;
          return *this;
        }
      };
      static_assert(std::is_copy_assignable_v<sum<TempAssign>>);
      static_assert(not std::is_nothrow_copy_assignable_v<sum<TempAssign>>);
      constexpr auto value = [](TempAssign const &t) { return t.v; };

      sum<TempAssign> a{TempAssign{7}};
      sum<TempAssign> const bad{TempAssign{0}};
      CHECK_THROWS_AS(a = bad, std::runtime_error); // the copy into the temporary throws
      CHECK(a.invoke(value) == 7);                  // untouched

      sum<TempAssign> const good{TempAssign{5}};
      a = good;
      CHECK(a.invoke(value) == 5);

      static_assert([] {
        sum<TempAssign> a{TempAssign{7}};
        sum<TempAssign> const good{TempAssign{5}};
        a = good; // the temporary arm, in a constant expression
        return a.invoke([](TempAssign const &t) { return t.v; }) == 5;
      }());
    }

    SECTION("a throwing replacement leaves the value in hand")
    {
      // when the alternative changes, the old one is destroyed only once its replacement is
      // certain: a copy that can throw is made into a temporary first, and only its (nothrow)
      // move touches the storage
      struct ThrowingCopy final {
        int v;
        ThrowingCopy(int i) noexcept : v(i) {} // NOLINT: implicit on purpose
        ThrowingCopy(ThrowingCopy const &o) : v(o.v)
        {
          if (v == 0)
            throw std::runtime_error("copy");
        }
        ThrowingCopy(ThrowingCopy &&o) noexcept : v(o.v) {}
        ThrowingCopy &operator=(ThrowingCopy const &) noexcept = default;
        ThrowingCopy &operator=(ThrowingCopy &&) noexcept = default;
      };
      struct ThrowingMove final {
        int v;
        ThrowingMove(int i) noexcept : v(i) {} // NOLINT: implicit on purpose
        ThrowingMove(ThrowingMove const &o) noexcept : v(o.v) {}
        ThrowingMove(ThrowingMove &&) { throw std::runtime_error("move"); }
        ThrowingMove &operator=(ThrowingMove const &) noexcept = default;
        ThrowingMove &operator=(ThrowingMove &&) noexcept = default;
      };
      using M = fn::sum_for<ThrowingCopy, ThrowingMove>;
      constexpr auto value = fn::overload{[](ThrowingCopy const &t) { return t.v; }, //
                                          [](ThrowingMove const &t) { return t.v; }};

      M m{std::in_place_type<ThrowingMove>, 7}; // in place: its move would throw
      M const bad_copy{ThrowingCopy{0}};
      CHECK_THROWS_AS(m = bad_copy, std::runtime_error); // the copy into the temporary throws
      CHECK(m.invoke(value) == 7);                       // ... and the ThrowingMove is still there

      M const good{std::in_place_type<ThrowingMove>, 3};
      m = good; // the same alternative: assigned in place, its throwing move never used
      CHECK(m.invoke(value) == 3);

      // ... and the same temporary arm run to completion: a throwing-path test alone never lets
      // the replacement finish, so this copy succeeds and the old alternative is destroyed
      M const good_copy{ThrowingCopy{5}};
      m = good_copy;
      CHECK(m.invoke(value) == 5);
    }
  }

  SECTION("constraints")
  {
    // Assignment requires of every alternative its own operator=: a sum does not offer an
    // operation its alternative refuses.
    static_assert(not std::is_copy_assignable_v<NonCopyable>);
    static_assert(not std::is_copy_constructible_v<NonCopyable>);
    static_assert(not std::is_copy_assignable_v<sum<NonCopyable>>); // it cannot be copied at all
    static_assert(not std::is_move_assignable_v<sum<NonCopyable>>);

    struct NoAssign final {
      int v;
      constexpr NoAssign(int i) noexcept : v(i) {} // NOLINT: implicit on purpose
      constexpr NoAssign(NoAssign const &) noexcept = default;
      constexpr NoAssign(NoAssign &&) noexcept = default;
      NoAssign &operator=(NoAssign const &) = delete;
      NoAssign &operator=(NoAssign &&) = delete;
    };
    static_assert(std::is_copy_constructible_v<NoAssign>);
    static_assert(not std::is_copy_assignable_v<NoAssign>);
    static_assert(not std::is_copy_assignable_v<sum<NoAssign>>); // the sum respects the refusal
    static_assert(not std::is_move_assignable_v<sum<NoAssign>>);

    // ... above all where the refusal is load-bearing: a pack holding a reference deletes its
    // assignment because C++ cannot rebind a reference, and reconstructing the pack in place would
    // synthesize exactly the operation the language does not have
    static_assert(std::is_copy_constructible_v<fn::pack<int, int &>>);
    static_assert(not std::is_copy_assignable_v<fn::pack<int, int &>>);
    static_assert(not std::is_copy_assignable_v<sum<fn::pack<int, int &>>>);
    static_assert(not std::is_move_assignable_v<sum<fn::pack<int, int &>>>);

    // An alternative that can be neither copied nor moved without throwing leaves the replacement
    // path no safe arm, whatever its own operator= promises. It is constrained away rather than
    // half-served.
    struct ThrowingBoth final {
      ThrowingBoth() = default;
      ThrowingBoth(ThrowingBoth const &) noexcept(false) {}
      ThrowingBoth(ThrowingBoth &&) noexcept(false) {}
      ThrowingBoth &operator=(ThrowingBoth const &) noexcept { return *this; }
      ThrowingBoth &operator=(ThrowingBoth &&) noexcept { return *this; }
    };
    static_assert(std::is_nothrow_copy_assignable_v<ThrowingBoth>);
    static_assert(not std::is_copy_assignable_v<sum<ThrowingBoth>>);
    static_assert(not std::is_move_assignable_v<sum<ThrowingBoth>>);

    // The arm is chosen per ALTERNATIVE, not for the sum as a whole - both for the assignment of
    // an unchanged alternative and for the construction that replaces one. A sum whose
    // alternatives need different arms is therefore still assignable.
    struct QuietCopy final { // replaced by copying straight over: its move throws
      QuietCopy() = default;
      QuietCopy(QuietCopy const &) noexcept = default;
      QuietCopy(QuietCopy &&) noexcept(false) {}
      QuietCopy &operator=(QuietCopy const &) noexcept = default;
      QuietCopy &operator=(QuietCopy &&) noexcept = default;
    };
    struct QuietMove final { // replaced through a temporary: its copy throws
      QuietMove() = default;
      QuietMove(QuietMove const &) noexcept(false) {}
      QuietMove(QuietMove &&) noexcept = default;
      QuietMove &operator=(QuietMove const &) noexcept = default;
      QuietMove &operator=(QuietMove &&) noexcept = default;
    };
    static_assert(std::is_nothrow_copy_assignable_v<sum<QuietCopy>>); // nothing on its path throws
    static_assert(std::is_move_assignable_v<sum<QuietCopy>>);         // the copy assignment takes the rvalue
    static_assert(std::is_copy_assignable_v<sum<QuietMove>>);         // replaced through the temporary
    static_assert(not std::is_nothrow_copy_assignable_v<sum<QuietMove>>);
    static_assert(std::is_nothrow_move_assignable_v<sum<QuietMove>>);

    using Mixed = fn::sum_for<QuietCopy, QuietMove>; // one alternative per arm
    static_assert(std::is_copy_assignable_v<Mixed>);
    static_assert(not std::is_nothrow_copy_assignable_v<Mixed>); // QuietMove's copy can throw
    static_assert(std::is_move_assignable_v<Mixed>);             // the copy assignment takes the rvalue ...
    static_assert(not std::is_nothrow_move_assignable_v<Mixed>); // ... and says that it can throw
  }
}
