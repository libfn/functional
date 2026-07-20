// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/copack.hpp>
#include <fn/pack.hpp>
#include <fn/utility.hpp>

#include <catch2/catch_all.hpp>

#include <array>
#include <concepts>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <fn/detail/macro_begin.hpp>

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

template <fn::copack<bool, int> S> struct copack_nttp final {};
template <fn::some_copack auto S> struct some_copack_nttp final {};
template <fn::some_copack auto S> auto read_nttp()
{
  return S.apply([](auto const &...args) { return (0.0 + ... + static_cast<double>(args)); });
}

// Every operation copack performs on an alternative - copy, move, compare - can throw here, so the
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
// and selects copack's explicit value constructor, while an rvalue selects the implicit one.
// NOTE no operator int(): with the implicit ExplicitCopy(int) ctor it would open an
// ExplicitCopy& -> int -> ExplicitCopy path, making the argument convertible after all.
struct ExplicitCopy final {
  int v;

  constexpr ExplicitCopy(int i) noexcept : v(i) {}
  constexpr explicit ExplicitCopy(ExplicitCopy const &o) noexcept : v(o.v) {}
  constexpr ExplicitCopy(ExplicitCopy &&o) noexcept : v(o.v) {}
};

// Counts live instances through every path a copack can take one: unlike TestType, whose implicit copy
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

// Comparison probes are type-keyed rather than the file's usual value-taking lambda: copack<> has no
// values to pass one (its default constructor is deleted, by design).
template <typename L, typename R>
concept can_eq = requires { std::declval<L const &>() == std::declval<R const &>(); };
template <typename L, typename R>
concept can_ne = requires { std::declval<L const &>() != std::declval<R const &>(); };

template <typename S, typename T, typename... Args>
concept can_in_place = requires(Args... args) { S{std::in_place_type<T>, args...}; };

template <typename S, typename T, typename... Args>
concept can_emplace = requires(S &s, Args &&...args) { s.template emplace<T>(static_cast<Args &&>(args)...); };

template <typename T, typename... Args>
concept can_as_copack = requires(Args... args) { fn::as_copack(std::in_place_type<T>, args...); };

template <typename T>
concept can_as_copack_value = requires(T v) { fn::as_copack(FWD(v)); };

template <typename S, typename Fn>
concept can_transform = requires(S s, Fn fn) { FWD(s).transform(fn); };

template <typename S, typename Fn, typename... Args>
concept can_apply_type = requires(S s, Fn fn, Args... args) { FWD(s).apply_type(FWD(fn), FWD(args)...); };

template <typename S, typename R, typename Fn, typename... Args>
concept can_apply_type_r
    = requires(S s, Fn fn, Args... args) { FWD(s).template apply_type_r<R>(FWD(fn), FWD(args)...); };
} // anonymous namespace

// A copack brace-initializes the alternative it stores. That is a DESIGN DIRECTION, not an
// implementation detail, and this TEST_CASE exists to fail if it is ever switched to the
// parenthesized initialization `std::variant` uses. Many other tests would fail with it, but
// incidentally - this one is here to say what was chosen, and why.
//
// What braces buy:
//   * Aggregate forwarding, through brace elision: `copack{in_place_type<array<int,3>>, 1, 2, 3}`.
//     `std::variant` cannot express this, and neither can parenthesized initialization - P0960
//     admits aggregates but forbids brace elision.
//   * Coherence with `copack` being a structural type. A structural type's alternatives must themselves
//     be structural - public-member types, written with braces, exactly as one writes them as a
//     template argument. Storing them any other way would put the two spellings at odds.
//   * Narrowing is rejected outright rather than silently truncating.
//
// What braces cost:
//   * An initializer-list constructor wins where `std::variant` would call the (count, value) one:
//     `copack<vector<int>>{in_place_type<vector<int>>, 3, 0}` holds `{3, 0}`, where a variant would hold
//     `{0, 0, 0}`. This divergence is the cost of giving `copack` a capability which `std::variant` lacks.
//
// And the consequence that actually bites, which is why the last section is here: every trait that
// constrains or specifies the construction of an alternative must ask the BRACE question
// (`fn::detail::_initializable` / `_nothrow_initializable`), never `std::is_[nothrow_]constructible`,
// which asks about parentheses. Where the two disagree, the parenthesized answer is a lie.
TEST_CASE("design: braces, not parentheses", "[copack][design]")
{
  using fn::copack;

  SECTION("aggregate forwarding")
  {
    using A = std::array<int, 3>;
    static_assert(can_in_place<copack<A>, A, int, int, int>);     // braces elide; parentheses cannot
    static_assert(not std::is_constructible_v<A, int, int, int>); // ... which is why the std trait misleads
    static_assert(fn::detail::_initializable<A, int, int, int>);  // ... and this is the trait that does not

    copack<A> a{std::in_place_type<A>, 1, 2, 3};
    CHECK(a.apply([](A const &v) { return v[0] * 100 + v[1] * 10 + v[2]; }) == 123);
    static_assert(copack<A>{std::in_place_type<A>, 1, 2, 3}.apply([](A const &v) { return v[2]; }) == 3);
  }

  SECTION("narrowing")
  {
    static_assert(not can_in_place<copack<int>, int, double>); // braces reject it ...
    static_assert(std::is_constructible_v<int, double>);       // ... where parentheses would truncate in silence
    SUCCEED();
  }

  SECTION("initializer-list constructors win")
  {
    // the price of the above, and the same mechanism as the trap below: `std::variant` would hold
    // three zeroes here, and a copack holds the two elements it was written with
    using V = std::vector<int>;
    copack<V> v{std::in_place_type<V>, 3, 0};
    CHECK(v.apply([](V const &x) { return x.size(); }) == 2);
    CHECK(v.apply([](V const &x) { return x[0]; }) == 3);
  }

  SECTION("the traits must ask the brace question")
  {
    // Braced initialization considers initializer-list constructors FIRST, so the two questions can
    // answer differently - and the parenthesized answer is not the one the storage acts on.
    struct Listy final {
      Listy(int) noexcept {}                               // parentheses select this ...
      Listy(std::initializer_list<int>) noexcept(false) {} // ... braces select this
    };
    static_assert(std::is_nothrow_constructible_v<Listy, int>);               // the parenthesized question: nothrow
    static_assert(not fn::detail::_nothrow_initializable<Listy, int>);        // the braced question: it can throw
    static_assert(not noexcept(copack<Listy>{std::in_place_type<Listy>, 1})); // the copack promises what it does
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
    // what the copack must promise. Reading the parenthesized answer instead - which is always "nothrow
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

    static_assert(std::is_nothrow_move_constructible_v<copack<Evil>> == fn::detail::_nothrow_initializable<Evil, Evil>);
    static_assert(std::is_nothrow_copy_constructible_v<copack<Evil>>
                  == fn::detail::_nothrow_initializable<Evil, Evil const &>);
    static_assert(std::is_move_assignable_v<copack<Evil>>
                  == (std::is_move_assignable_v<Evil> && fn::detail::_nothrow_initializable<Evil, Evil>));
    SUCCEED();
  }
}

TEST_CASE("copack basic functionality tests", "[copack]")
{
  // NOTE This test looks very similar to test in choice.cpp - for good reason.

  using fn::copack;

  SECTION("copack<> unit")
  {
    static_assert(copack<>::size == 0);
    static_assert(copack<>::has_type<bool> == false);
    static_assert(std::same_as<fn::copack_for<copack<>, copack<>>, copack<>>);
    static_assert(not std::is_default_constructible_v<copack<>>); // the deleted default ctor is the point
    static_assert(std::is_nothrow_copy_constructible_v<copack<>>);
    static_assert(std::is_nothrow_move_constructible_v<copack<>>);
    static_assert(std::is_nothrow_copy_assignable_v<copack<>>);
    static_assert(std::is_nothrow_move_assignable_v<copack<>>);
    static_assert(fn::empty_copack<copack<>>);
    static_assert(fn::empty_copack<copack<> const &>);
    static_assert(not fn::empty_copack<copack<int>>);
    static_assert(not fn::empty_copack<int>);
    static_assert(std::is_nothrow_destructible_v<copack<>>);
    SUCCEED();
  }

  SECTION("as_copack")
  {
    constexpr auto a = fn::as_copack(12);
    static_assert(std::same_as<decltype(a), fn::copack<int> const>);
    static_assert(a == fn::copack{12});

    constexpr auto b = fn::as_copack(std::in_place_type<long>, 12);
    static_assert(std::same_as<decltype(b), fn::copack<long> const>);
    static_assert(b == fn::copack{12l});

    // both lifts weigh the alternative they construct, asking the brace initialization they perform
    static_assert(noexcept(fn::as_copack(12)));
    static_assert(noexcept(fn::as_copack(std::in_place_type<long>, 12)));
    static_assert(not noexcept(fn::as_copack(std::declval<Throwing const &>())));

    SECTION("constraints")
    {
      static_assert(can_as_copack<long, int>);
      static_assert(can_as_copack<std::array<int, 3>, int, int, int>); // an aggregate, brace-initialized
      static_assert(not can_as_copack<NonCopyable>);                   // no default constructor
      static_assert(not can_as_copack<NonCopyable, char const *>);     // not constructible from it

      // the tag selects the alternative, it is never itself one: with nothing to construct there is
      // no viable lift at all, rather than a copack whose alternative is the tag
      static_assert(not can_as_copack_value<std::in_place_type_t<NonCopyable> const &>);
      static_assert(can_as_copack_value<long>);
      SUCCEED();
    }
  }

  SECTION("copack_for")
  {
    static_assert(std::same_as<fn::copack_for<int>, fn::copack<int>>);
    static_assert(std::same_as<fn::copack_for<int, int>, fn::copack<int>>);
    static_assert(std::same_as<fn::copack_for<int, bool>, fn::copack<bool, int>>);
    static_assert(std::same_as<fn::copack_for<bool, int>, fn::copack<bool, int>>);
    // A copack's canonical alternative order comes from each type's compiler spelling (GCC/Clang
    // __PRETTY_FUNCTION__ vs MSVC __FUNCSIG__), so it is platform/ABI-specific: MSVC sorts
    // class/struct types after the fundamentals, GCC/Clang before. This divergence is inherent and
    // deliberately NOT unified — even C++26 std::type_order is an implementation-defined, ABI-tied
    // total order, so there is no single cross-platform order; don't try to make them match. This
    // one assert documents the difference; every other ordering check below asserts only the
    // platform-independent guarantees (commutativity, uniqueness). Revisit once std::type_order
    // ships on all supported platforms.
    // Spelling: copacks whose alternatives include a non-builtin have platform-specific order, so they
    // are written copack_for<...>; pure-builtin copacks keep a fixed copack<...>.
#ifdef _MSC_VER
    static_assert(std::same_as<fn::copack_for<int, NonCopyable>, fn::copack<int, NonCopyable>>);
#else
    static_assert(std::same_as<fn::copack_for<int, NonCopyable>, fn::copack<NonCopyable, int>>);
#endif
    static_assert(std::same_as<fn::copack_for<NonCopyable, int>, fn::copack_for<int, NonCopyable>>); // commutative
    static_assert(
        std::same_as<fn::copack_for<int, bool, NonCopyable>, fn::copack_for<NonCopyable, bool, int>>); // commutative
    static_assert(
        std::same_as<fn::copack_for<NonCopyable, int, NonCopyable>, fn::copack_for<int, NonCopyable>>); // unique
    static_assert(fn::copack_for<int, bool, NonCopyable>::size == 3);

    static_assert(std::same_as<fn::copack_for<int, fn::copack<int>>, fn::copack<int>>);
    static_assert(std::same_as<fn::copack_for<int, fn::copack<bool>>, fn::copack<bool, int>>);
    static_assert(std::same_as<fn::copack_for<int, fn::copack<bool, int>>, fn::copack<bool, int>>);
    static_assert(std::same_as<fn::copack_for<int, fn::copack<bool, double>>, fn::copack<bool, double, int>>);

    static_assert(std::same_as<fn::copack_for<fn::copack<bool>, fn::copack<int>>, fn::copack<bool, int>>);
    static_assert(
        std::same_as<fn::copack_for<fn::copack<bool>, fn::copack<bool, double, int>>, fn::copack<bool, double, int>>);
    static_assert(
        std::same_as<fn::copack_for<fn::copack<bool>, fn::copack<double, int>>, fn::copack<bool, double, int>>);
    static_assert(std::same_as<fn::copack_for<fn::copack<bool, int>, double>, fn::copack<bool, double, int>>);

    static_assert(std::same_as<fn::copack_for<int, fn::copack<>>, fn::copack<int>>);
    static_assert(std::same_as<fn::copack_for<fn::copack<>, int>, fn::copack<int>>);
    static_assert(std::same_as<fn::copack_for<fn::copack<>, fn::copack<bool, int>>, fn::copack<bool, int>>);
    static_assert(
        std::same_as<fn::copack_for<double, fn::copack<>, fn::copack<bool, int>>, fn::copack<bool, double, int>>);
  }

  SECTION("applicable")
  {
    using type
        = fn::copack_for<TestType, int>; // copack<...> order is platform-specific; copack_for normalizes per platform
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

    static_assert(fn::typelist_applicable<decltype([](auto &) {}), copack<NonCopyable> &>);
    static_assert(not fn::typelist_applicable<decltype([](auto) {}), NonCopyable &>); // copy-constructor not available

    // variadic-generic callback, and a per-category sweep of an lvalue-only overload set
    using T2 = fn::copack<double, int>;
    static_assert(fn::typelist_applicable<decltype([](auto...) {}), T2 &>);
    constexpr auto fnLvalue = fn::overload{[](int &) {}, [](double &) {}};
    static_assert(fn::typelist_applicable<decltype(fnLvalue), T2 &>);
    static_assert(not fn::typelist_applicable<decltype(fnLvalue), T2 const &>);
    static_assert(not fn::typelist_applicable<decltype(fnLvalue), T2>);
    static_assert(not fn::typelist_applicable<decltype(fnLvalue), T2 const>);
    static_assert(not fn::typelist_applicable<decltype(fnLvalue), T2 &&>);
    static_assert(not fn::typelist_applicable<decltype(fnLvalue), T2 const &&>);

    // a tuple-like alternative asks applicability of its elements
    static_assert(fn::typelist_applicable<decltype([](int, int) {}), copack<std::tuple<int, int>> &>);
    static_assert(not fn::typelist_applicable<decltype([](int, int) {}), copack<std::tuple<int, int, int>> &>); // arity
    SUCCEED();
  }

  SECTION("check destructor call")
  {
    {
      copack<TestType> s{std::in_place_type<TestType>};
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
    static_assert(copack<int>::size == 1);

    constexpr copack<int> a = 12;
    static_assert(a == copack{12});

    constexpr copack<bool> b{false};
    static_assert(b == copack{false});

    SECTION("noexcept")
    {
      // the value constructors weigh the alternative they construct
      static_assert(std::is_nothrow_constructible_v<int, int>);
      static_assert(noexcept(copack<int>{42}));
      static_assert(noexcept(copack<int>{std::in_place_type<int>, 42}));

      // ... and report it when that construction can throw
      static_assert(not noexcept(copack<Throwing>{std::declval<Throwing const &>()}));
      static_assert(not noexcept(copack<Throwing>{std::in_place_type<Throwing>, std::declval<Throwing const &>()}));
      SUCCEED();
    }

    SECTION("explicit (non-convertible) argument")
    {
      // The two value constructors differ only in the argument's convertibility to the alternative:
      // ExplicitCopy's copy constructor is explicit, so an lvalue selects the explicit arm and can
      // only direct-initialize, while an rvalue (implicit move) selects the implicit one.
      static_assert(std::is_constructible_v<ExplicitCopy, ExplicitCopy &>);
      static_assert(not std::is_convertible_v<ExplicitCopy &, ExplicitCopy>);

      static_assert(std::is_constructible_v<copack<ExplicitCopy>, ExplicitCopy &>);
      static_assert(not std::is_convertible_v<ExplicitCopy &, copack<ExplicitCopy>>); // explicit arm
      static_assert(std::is_constructible_v<copack<ExplicitCopy>, ExplicitCopy &&>);
      static_assert(std::is_convertible_v<ExplicitCopy &&, copack<ExplicitCopy>>); // implicit arm

      ExplicitCopy e{42};
      copack<ExplicitCopy> a{e};
      CHECK(a.apply([](auto &&i) -> int { return i.v; }) == 42);

      copack<ExplicitCopy> b{std::move(e)};
      CHECK(b.apply([](auto &&i) -> int { return i.v; }) == 42);

      SECTION("constexpr")
      {
        constexpr auto c = []() constexpr {
          ExplicitCopy e{42};
          return copack<ExplicitCopy>{e};
        }();
        static_assert(c.apply([](auto &&i) -> int { return i.v; }) == 42);
        SUCCEED();
      }
    }

    SECTION("CTAD")
    {
      copack a{42};
      static_assert(std::is_same_v<decltype(a), copack<int>>);
      CHECK(a == copack<int>{42});

      constexpr copack b{false};
      static_assert(std::is_same_v<decltype(b), copack<bool> const>);
      static_assert(b == copack<bool>{false});

      constexpr auto c = copack{std::array<int, 3>{3, 14, 15}};
      static_assert(std::is_same_v<decltype(c), copack<std::array<int, 3>> const>);
      static_assert(c.apply([](auto &&a) -> bool { return a.size() == 3 && a[0] == 3 && a[1] == 14 && a[2] == 15; }));
    }

    SECTION("move from rvalue")
    {
      using T = fn::copack<bool, int>;
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
      using T = fn::copack<bool, int>;
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
    copack<NonCopyable> a{std::in_place_type<NonCopyable>, 42};
    CHECK(a.apply([](auto &i) -> bool { return i.v == 42; }));

    SECTION("CTAD")
    {
      constexpr auto a = copack{std::in_place_type<NonCopyable>, 42};
      static_assert(std::is_same_v<decltype(a), copack<NonCopyable> const>);

      auto b = copack{std::in_place_type<NonCopyable>, 42};
      static_assert(std::is_same_v<decltype(b), copack<NonCopyable>>);
    }

    SECTION("constraints")
    {
      static_assert(can_in_place<copack<NonCopyable>, NonCopyable, int>);
      static_assert(not can_in_place<copack<NonCopyable>, int, int>); // int is not an alternative

      // an argument list the alternative cannot be constructed from is not viable - viability must
      // answer here, not fail to compile inside variadic_union, beyond SFINAE's reach
      static_assert(not can_in_place<copack<NonCopyable>, NonCopyable>);               // no default ctor
      static_assert(not can_in_place<copack<NonCopyable>, NonCopyable, char const *>); // not constructible from
      SUCCEED();
    }
  }

  SECTION("forwarding constructors (aggregate)")
  {
    SECTION("regular")
    {
      copack<std::array<int, 3>> a{std::in_place_type<std::array<int, 3>>, 1, 2, 3};
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
      constexpr copack<std::array<int, 3>> a{std::in_place_type<std::array<int, 3>>, 1, 2, 3};
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
      constexpr auto a = copack{std::in_place_type<std::array<int, 3>>, 1, 2, 3};
      static_assert(std::is_same_v<decltype(a), copack<std::array<int, 3>> const>);

      auto b = copack{std::in_place_type<std::array<int, 3>>, 1, 2, 3};
      static_assert(std::is_same_v<decltype(b), copack<std::array<int, 3>>>);
    }

    SECTION("constraints")
    {
      using T = std::array<int, 3>;
      // the element is brace-initialized, which elides braces for an aggregate - a constraint
      // spelled with is_constructible_v (parenthesized init) would reject this very construction
      static_assert(can_in_place<copack<T>, T, int, int, int>);
      static_assert(not can_in_place<copack<T>, T, int, int, int, int>); // one too many
      static_assert(not can_in_place<copack<int>, int, double>);         // narrowing, rejected by braces
      SUCCEED();
    }
  }

  SECTION("widening constructor")
  {
    using T = copack<bool, double, int>;
    using S = copack<bool, int>;

    SECTION("from lvalue")
    {
      S const a{std::in_place_type<int>, 42};
      T b{a};
      CHECK(b.has_value(std::in_place_type<int>));
      CHECK(b.apply([](auto &&i) -> int { return static_cast<int>(i); }) == 42);
      CHECK(a.has_value(std::in_place_type<int>)); // the source is copied, not consumed
    }

    SECTION("from rvalue")
    {
      S a{std::in_place_type<int>, 42};
      T b{std::move(a)};
      CHECK(b.has_value(std::in_place_type<int>));
      CHECK(b.apply([](auto &&i) -> int { return static_cast<int>(i); }) == 42);
    }

    SECTION("in_place_type names the source copack")
    {
      S const a{std::in_place_type<bool>, true};
      T b{std::in_place_type<S>, a};
      CHECK(b.has_value(std::in_place_type<bool>));
      CHECK(b.apply([](auto &&i) -> bool { return static_cast<bool>(i); }));
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
      static_assert(std::is_constructible_v<T, copack<int> const &>);
      static_assert(not std::is_constructible_v<copack<int>, S const &>); // narrowing
      static_assert(not std::is_constructible_v<copack<int>, S &&>);
      static_assert(not std::is_constructible_v<S, copack<double> const &>); // disjoint

      // The in_place_type form names the SOURCE copack, and is superset-constrained the same way.
      static_assert(std::is_constructible_v<T, std::in_place_type_t<S>, S const &>);
      static_assert(not std::is_constructible_v<copack<int>, std::in_place_type_t<S>, S const &>);

      // Same-type construction is the copy/move constructor - the widening pair excludes it.
      static_assert(std::is_constructible_v<T, T const &>);
      SUCCEED();
    }

    SECTION("noexcept")
    {
      // each widening constructor copies or moves every alternative of the source into the wider
      // union, and weighs what it relocates
      using X = fn::copack_for<Throwing, int>;
      static_assert(not noexcept(X{std::declval<copack<Throwing> const &>()}));
      static_assert(not noexcept(X{std::declval<copack<Throwing> &&>()}));
      static_assert(not noexcept(X{std::in_place_type<copack<Throwing>>, std::declval<copack<Throwing> const &>()}));

      // ... so an alternative that cannot throw widens without an exception edge
      using Y = fn::copack<bool, int>;
      static_assert(noexcept(Y{std::declval<copack<int> const &>()}));
      static_assert(noexcept(Y{std::declval<copack<int> &&>()}));
      SUCCEED();
    }
  }

  SECTION("has_type type mismatch")
  {
    using type = copack<bool, int>;
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
    static_assert(noexcept(std::declval<copack<Throwing> const &>().has_value(std::in_place_type<Throwing>)));
  }

  SECTION("index")
  {
    constexpr fn::copack<std::array<int, 3>> a{std::in_place_type<std::array<int, 3>>, 3, 14, 15};
    static_assert(a.index == 0);

    fn::copack<double, int> b{std::in_place_type<int>, 42};
    CHECK(b.index == 1);
    constexpr fn::copack<double, int> c{std::in_place_type<int>, 12};
    static_assert(c.index == 1);
  }

  SECTION("select_nth")
  {
    using T = fn::copack<double, int>;
    static_assert(T::size == 2);
    static_assert(std::is_same_v<T::template select_nth<0>, double>);
    static_assert(std::is_same_v<T::template select_nth<1>, int>);

    SUCCEED();
  }

  SECTION("get_ptr")
  {
    using T = fn::copack<double, int>;
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

    constexpr auto d = fn::copack<double, int>{std::in_place_type<int>, 12};
    static_assert(d.get_ptr(std::in_place_type<double>) == nullptr);
    static_assert(*d.get_ptr(std::in_place_type<int>) == 12);
  }

  SECTION("equality comparison")
  {
    using type = copack<bool, int>;

    // != is synthesized by C++20 rewriting from ==, so it cannot claim to be viable where == is not.
    // Against the uninstantiable copack<> both must drop out of overload resolution together
    static_assert(can_eq<type, type>);
    static_assert(can_ne<type, type>);
    static_assert(not can_eq<copack<int>, copack<>>);
    static_assert(not can_ne<copack<int>, copack<>>);
    static_assert(not can_eq<copack<>, copack<int>>);
    static_assert(not can_ne<copack<>, copack<int>>);
    static_assert(not can_eq<copack<>, copack<>>);
    static_assert(not can_ne<copack<>, copack<>>);

    type const a{std::in_place_type<int>, 42};
    static_assert(std::is_same_v<bool, decltype(copack{42} == a)>);
    CHECK(a == type{42});
    CHECK(type{42} == a);
    CHECK(a != type{41});
    CHECK(type{41} != a);
    CHECK(a != type{true});
    CHECK(type{false} != a);
    CHECK(a == copack{42});
    CHECK(copack{42} == a);
    CHECK(a != copack{41});
    CHECK(copack{41} != a);
    CHECK(a != copack{false});
    CHECK(copack{true} != a);
    CHECK(a == copack<double, int>{42});
    CHECK(copack<double, int>{42} == a);
    CHECK(a != copack<double, int>{41});
    CHECK(copack<double, int>{41} != a);
    CHECK(copack{0.5} != a);
    CHECK(a != copack{0.5});

    SECTION("constexpr")
    {
      constexpr type a{std::in_place_type<int>, 42};
      static_assert(std::is_same_v<bool, decltype(a == copack{42})>);
      static_assert(a == type{42});
      static_assert(type{42} == a);
      static_assert(a != type{41});
      static_assert(type{41} != a);
      static_assert(a != type{true});
      static_assert(type{false} != a);
      static_assert(a == copack{42});
      static_assert(copack{42} == a);
      static_assert(a != copack{41});
      static_assert(copack{41} != a);
      static_assert(a != copack{false});
      static_assert(copack{true} != a);
      static_assert(a == copack<double, int>{42});
      static_assert(copack<double, int>{42} == a);
      static_assert(a != copack<double, int>{41});
      static_assert(copack<double, int>{41} != a);
      static_assert(copack{0.5} != a);
      static_assert(a != copack{0.5});

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
      using T = copack<Throwing>;

      // operator== reaches into the alternative's own comparison, and weighs it - as does the !=
      // rewritten from it
      static_assert(not noexcept(std::declval<Throwing const &>() == std::declval<Throwing const &>()));
      static_assert(not noexcept(std::declval<T const &>() == std::declval<T const &>()));
      static_assert(not noexcept(std::declval<T const &>() != std::declval<T const &>()));

      // ... while an alternative whose comparison cannot throw is compared without an exception edge
      using Q = copack<int>;
      static_assert(noexcept(std::declval<Q const &>() == std::declval<Q const &>()));
      static_assert(noexcept(std::declval<Q const &>() != std::declval<Q const &>()));

      SUCCEED();
    }
  }

  SECTION("apply")
  {
    copack<int> a{std::in_place_type<int>, 42};

    SECTION("noexcept")
    {
      // apply weighs the callback it dispatches to, in every value category
      constexpr auto throwing = [](int i) noexcept(false) -> bool { return i == 42; };
      static_assert(not noexcept(throwing(42)));
      static_assert(not noexcept(a.apply(throwing)));
      static_assert(not noexcept(std::as_const(a).apply(throwing)));
      static_assert(not noexcept(std::move(a).apply(throwing)));
      static_assert(not noexcept(std::move(std::as_const(a)).apply(throwing)));

      constexpr auto throwing2 = [](int i, std::monostate) noexcept(false) -> bool { return i == 42; };
      static_assert(not noexcept(a.apply(throwing2, std::monostate{}))); // extra arguments, same promise

      constexpr auto nothrow = [](int i) noexcept -> bool { return i == 42; };
      static_assert(noexcept(a.apply(nothrow)));
      static_assert(noexcept(std::move(a).apply(nothrow)));
      SUCCEED();
    }

    SECTION("value only")
    {
      static_assert(std::is_same_v<bool, decltype(a.apply(fn::overload{[](auto) -> bool { throw 1; },
                                                                       [](int) -> bool { return true; }}))>);

      // a result type other than bool, to witness the deduced return
      constexpr auto fn1 = [](auto i) noexcept -> std::size_t { return sizeof(i); };
      static_assert(std::is_same_v<std::size_t, decltype(a.apply(fn1))>);
      CHECK(a.apply(fn1) == sizeof(int));
      CHECK(a.data.v0 == 42); // white-box: the value went into the first alternative

      CHECK(a.apply(fn::overload{[](auto) -> bool { throw 1; }, [](int &i) -> bool { return i == 42; },
                                 [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                 [](int const &&) -> bool { throw 0; }}));
      CHECK(std::as_const(a).apply(fn::overload{
          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &i) -> bool { return i == 42; },
          [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; }}));
      CHECK(std::move(std::as_const(a))
                .apply(fn::overload{[](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                    [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                    [](int const &&i) -> bool { return i == 42; }}));
      CHECK(std::move(a).apply(fn::overload{
          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
          [](int &&i) -> bool { return i == 42; }, [](int const &&) -> bool { throw 0; }}));

      SECTION("constexpr")
      {
        constexpr copack<int> a{std::in_place_type<int>, 42};
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

    SECTION("extra arguments")
    {
      static_assert(std::is_same_v<bool, decltype(a.apply([](int, int) -> bool { return true; }, 12))>);

      CHECK(a.apply(fn::overload{                                                      //
                                 [](auto const &...) -> bool { throw 1; },             //
                                 [](int &, std::monostate) -> bool { return true; },   //
                                 [](int const &, std::monostate) -> bool { throw 0; }, //
                                 [](int &&, std::monostate) -> bool { throw 0; },      //
                                 [](int const &&, std::monostate) -> bool { throw 0; }},
                    std::monostate{}));
      CHECK(std::as_const(a).apply(fn::overload{                                                          //
                                                [](auto const &...) -> bool { throw 1; },                 //
                                                [](int &, std::monostate) -> bool { throw 0; },           //
                                                [](int const &, std::monostate) -> bool { return true; }, //
                                                [](int &&, std::monostate) -> bool { throw 0; },          //
                                                [](int const &&, std::monostate) -> bool { throw 0; }},
                                   std::monostate{}));
      CHECK(std::move(std::as_const(a))
                .apply(fn::overload{                                                      //
                                    [](auto const &...) -> bool { throw 1; },             //
                                    [](int &, std::monostate) -> bool { throw 0; },       //
                                    [](int const &, std::monostate) -> bool { throw 0; }, //
                                    [](int &&, std::monostate) -> bool { throw 0; },      //
                                    [](int const &&, std::monostate) -> bool { return true; }},
                       std::monostate{}));
      CHECK(std::move(a).apply(fn::overload{                                                      //
                                            [](auto const &...) -> bool { throw 1; },             //
                                            [](int &, std::monostate) -> bool { throw 0; },       //
                                            [](int const &, std::monostate) -> bool { throw 0; }, //
                                            [](int &&, std::monostate) -> bool { return true; },  //
                                            [](int const &&, std::monostate) -> bool { throw 0; }},
                               std::monostate{}));

      SECTION("constexpr")
      {
        constexpr copack<int> a{std::in_place_type<int>, 42};
        static_assert(a.apply(fn::overload{[](auto...) -> bool { return false; }, //
                                           [](int &, std::monostate) -> bool { return false; },
                                           [](int const &, std::monostate) -> bool { return true; },
                                           [](int &&, std::monostate) -> bool { return false; },
                                           [](int const &&, std::monostate) -> bool { return false; }},
                              std::monostate{}));
        static_assert(std::move(a).apply(fn::overload{[](auto...) -> bool { return false; }, //
                                                      [](int &, std::monostate) -> bool { return false; },
                                                      [](int const &, std::monostate) -> bool { return false; },
                                                      [](int &&, std::monostate) -> bool { return false; },
                                                      [](int const &&, std::monostate) -> bool { return true; }},
                                         std::monostate{}));
        static_assert(fn::apply([](int i, std::monostate) -> bool { return i == 42; }, a, std::monostate{}));

        constexpr auto fn = [](auto &&...a) { return (0 + ... + static_cast<int>(a)); };
        static_assert(copack<bool, int>{2}.apply(fn, 3) == 5);
      }
    }
  }

  SECTION("apply_r")
  {
    copack<int> a{std::in_place_type<int>, 42};

    SECTION("noexcept")
    {
      // the same weighing as apply, the conversion to Ret included
      constexpr auto throwing = [](int i) noexcept(false) -> bool { return i == 42; };
      static_assert(not noexcept(a.template apply_r<bool>(throwing)));
      static_assert(not noexcept(std::as_const(a).template apply_r<bool>(throwing)));
      static_assert(not noexcept(std::move(a).template apply_r<bool>(throwing)));
      static_assert(not noexcept(std::move(std::as_const(a)).template apply_r<bool>(throwing)));
      static_assert(not noexcept(a.template apply_r<long>(throwing))); // converting the result, too

      constexpr auto nothrow = [](int i) noexcept -> bool { return i == 42; };
      static_assert(noexcept(a.template apply_r<bool>(nothrow)));
      static_assert(noexcept(a.template apply_r<long>(nothrow)));
      SUCCEED();
    }

    SECTION("value only")
    {
      static_assert(std::is_same_v<bool, decltype(a.template apply_r<bool>(fn::overload{
                                             [](auto) -> bool { throw 1; }, [](int) -> bool { return true; }}))>);
      static_assert(
          std::is_same_v<int, decltype(a.template apply_r<int>(fn::overload{[](auto) -> bool { throw 1; }, //
                                                                            [](int) -> int { return true; }}))>);

      CHECK(a.template apply_r<bool>(fn::overload{[](auto) -> bool { throw 1; }, [](int &) -> bool { return true; },
                                                  [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                                  [](int const &&) -> bool { throw 0; }}));
      CHECK(std::as_const(a).template apply_r<bool>(fn::overload{
          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { return true; },
          [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; }}));
      CHECK(std::move(std::as_const(a))
                .template apply_r<bool>(fn::overload{
                    [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                    [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { return true; }}));
      CHECK(std::move(a).template apply_r<bool>(fn::overload{
          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
          [](int &&) -> bool { return true; }, [](int const &&) -> bool { throw 0; }}));

      SECTION("constexpr")
      {
        constexpr copack<int> a{std::in_place_type<int>, 42};
        static_assert(a.template apply_r<bool>(fn::overload{
            [](auto) -> std::false_type { return {}; }, //
            [](int &) -> std::false_type { return {}; }, [](int const &) -> std::true_type { return {}; },
            [](int &&) -> std::false_type { return {}; }, [](int const &&) -> std::false_type { return {}; }}));
        static_assert(std::move(a).template apply_r<bool>(fn::overload{
            [](auto) -> std::false_type { return {}; }, //
            [](int &) -> std::false_type { return {}; }, [](int const &) -> std::false_type { return {}; },
            [](int &&) -> std::false_type { return {}; }, [](int const &&) -> std::true_type { return {}; }}));
        static_assert(fn::apply_r<bool>([](int, std::monostate) -> std::true_type { return {}; }, a, std::monostate{}));
      }
    }

    SECTION("extra arguments")
    {
      static_assert(std::is_same_v<bool, decltype(a.template apply_r<bool>(fn::overload{
                                             [](auto) -> bool { throw 1; }, [](int) -> bool { return true; }}))>);
      static_assert(
          std::is_same_v<int, decltype(a.template apply_r<int>(fn::overload{[](auto) -> bool { throw 1; }, //
                                                                            [](int) -> int { return true; }}))>);
      CHECK(a.template apply_r<bool>(fn::overload{                                                      //
                                                  [](auto const &...) -> bool { throw 1; },             //
                                                  [](int &, std::monostate) -> bool { return true; },   //
                                                  [](int const &, std::monostate) -> bool { throw 0; }, //
                                                  [](int &&, std::monostate) -> bool { throw 0; },      //
                                                  [](int const &&, std::monostate) -> bool { throw 0; }},
                                     std::monostate{}));
      CHECK(std::as_const(a).template apply_r<bool>(
          fn::overload{                                                          //
                       [](auto const &...) -> bool { throw 1; },                 //
                       [](int &, std::monostate) -> bool { throw 0; },           //
                       [](int const &, std::monostate) -> bool { return true; }, //
                       [](int &&, std::monostate) -> bool { throw 0; },          //
                       [](int const &&, std::monostate) -> bool { throw 0; }},
          std::monostate{}));
      CHECK(std::move(std::as_const(a))
                .template apply_r<bool>(fn::overload{                                                      //
                                                     [](auto const &...) -> bool { throw 1; },             //
                                                     [](int &, std::monostate) -> bool { throw 0; },       //
                                                     [](int const &, std::monostate) -> bool { throw 0; }, //
                                                     [](int &&, std::monostate) -> bool { throw 0; },      //
                                                     [](int const &&, std::monostate) -> bool { return true; }},
                                        std::monostate{}));
      CHECK(std::move(a).template apply_r<bool>(fn::overload{                                                      //
                                                             [](auto const &...) -> bool { throw 1; },             //
                                                             [](int &, std::monostate) -> bool { throw 0; },       //
                                                             [](int const &, std::monostate) -> bool { throw 0; }, //
                                                             [](int &&, std::monostate) -> bool { return true; },  //
                                                             [](int const &&, std::monostate) -> bool { throw 0; }},
                                                std::monostate{}));

      SECTION("constexpr")
      {
        constexpr copack<int> a{std::in_place_type<int>, 42};
        static_assert(
            a.template apply_r<bool>(fn::overload{[](auto...) -> std::false_type { return {}; }, //
                                                  [](int &, std::monostate) -> std::false_type { return {}; },
                                                  [](int const &, std::monostate) -> std::true_type { return {}; },
                                                  [](int &&, std::monostate) -> std::false_type { return {}; },
                                                  [](int const &&, std::monostate) -> std::false_type { return {}; }},
                                     std::monostate{}));
        static_assert(std::move(a).template apply_r<bool>(
            fn::overload{[](auto...) -> std::false_type { return {}; }, //
                         [](int &, std::monostate) -> std::false_type { return {}; },
                         [](int const &, std::monostate) -> std::false_type { return {}; },
                         [](int &&, std::monostate) -> std::false_type { return {}; },
                         [](int const &&, std::monostate) -> std::true_type { return {}; }},
            std::monostate{}));

        constexpr auto fn = [](auto &&...a) { return (0 + ... + static_cast<int>(a)); };
        static_assert(copack<bool, int>{2}.template apply_r<long>(fn, 3) == 5l);
      }
    }
  }

  SECTION("copack of packs")
  {
    using fn::pack;
    constexpr copack a{pack{"abc", 42, 12.5}};
    static_assert(std::is_same_v<decltype(a), copack<pack<char const(&)[4], int, double>> const>);

    SECTION("constexpr")
    {
      constexpr auto b
          = a.apply([]<std::size_t I>(char const(&)[I], int i, double d) { return I + i + static_cast<int>(d); });
      static_assert(b == 4 + 42 + 12);

      constexpr copack<pack<int, int, int, int>, pack<int, int, int>, pack<int, int>, pack<int>> c = pack{3, 14, 15};
      static_assert(c.apply([](std::integral auto... args) -> int { return (... + args); }) == 3 + 14 + 15);

      SUCCEED();
    }

    SECTION("runtime")
    {
      auto const b = a.apply([](char const *s, int i, double d) { return std::strlen(s) + i + static_cast<int>(d); });
      CHECK(b == 3 + 42 + 12);

      constexpr copack<pack<int, int, int, int>, pack<int, int, int>, pack<int, int>, pack<int>> c
          = pack{3, 14, 15, 92};
      CHECK(c.apply([](std::integral auto... args) -> int { return (... + args); }) == 3 + 14 + 15 + 92);
    }
  }

  SECTION("copack of tuple-likes")
  {
    // the tuple-like arm of fn::apply reaches dispatch: a tuple-like alternative unpacks into the
    // callable's arguments, like a pack alternative always has
    constexpr auto add2 = [](int i, int j) noexcept -> int { return i + j; };
    copack<std::tuple<int, int>> a{std::tuple{2, 3}};

    SECTION("tuple, array, pair")
    {
      CHECK(copack<std::tuple<int, int>>{std::tuple{2, 3}}.apply(add2) == 5);
      CHECK(copack<std::array<int, 2>>{std::array{2, 3}}.apply(add2) == 5);
      CHECK(copack<std::pair<int, int>>{std::pair{2, 3}}.apply(add2) == 5);

      SECTION("constexpr")
      {
        static_assert(copack<std::tuple<int, int>>{std::tuple{2, 3}}.apply(add2) == 5);
        static_assert(copack<std::array<int, 2>>{std::array{2, 3}}.apply(add2) == 5);
        static_assert(copack<std::pair<int, int>>{std::pair{2, 3}}.apply(add2) == 5);
        SUCCEED();
      }
    }

    SECTION("value categories")
    {
      // the copack's category reaches the elements through the unpacking
      CHECK(a.apply(
          fn::overload{[](auto &&...) -> bool { throw 1; }, //
                       [](int &, int &) -> bool { return true; }, [](int const &, int const &) -> bool { throw 0; },
                       [](int &&, int &&) -> bool { throw 0; }, [](int const &&, int const &&) -> bool { throw 0; }}));
      CHECK(std::as_const(a).apply(
          fn::overload{[](auto &&...) -> bool { throw 1; }, //
                       [](int &, int &) -> bool { throw 0; }, [](int const &, int const &) -> bool { return true; },
                       [](int &&, int &&) -> bool { throw 0; }, [](int const &&, int const &&) -> bool { throw 0; }}));
      CHECK(std::move(std::as_const(a))
                .apply(fn::overload{
                    [](auto &&...) -> bool { throw 1; }, //
                    [](int &, int &) -> bool { throw 0; }, [](int const &, int const &) -> bool { throw 0; },
                    [](int &&, int &&) -> bool { throw 0; }, [](int const &&, int const &&) -> bool { return true; }}));
      CHECK(std::move(a).apply(fn::overload{
          [](auto &&...) -> bool { throw 1; }, //
          [](int &, int &) -> bool { throw 0; }, [](int const &, int const &) -> bool { throw 0; },
          [](int &&, int &&) -> bool { return true; }, [](int const &&, int const &&) -> bool { throw 0; }}));

      SECTION("constexpr")
      {
        constexpr copack<std::tuple<int, int>> a{std::tuple{2, 3}};
        static_assert(a.apply(fn::overload{[](auto &&...) -> std::false_type { return {}; }, //
                                           [](int &, int &) -> std::false_type { return {}; },
                                           [](int const &, int const &) -> std::true_type { return {}; },
                                           [](int &&, int &&) -> std::false_type { return {}; },
                                           [](int const &&, int const &&) -> std::false_type { return {}; }}));
        static_assert(
            std::move(a).apply(fn::overload{[](auto &&...) -> std::false_type { return {}; }, //
                                            [](int &, int &) -> std::false_type { return {}; },
                                            [](int const &, int const &) -> std::false_type { return {}; },
                                            [](int &&, int &&) -> std::false_type { return {}; },
                                            [](int const &&, int const &&) -> std::true_type { return {}; }}));
        SUCCEED();
      }
    }

    SECTION("mixed with a plain alternative")
    {
      // one overload set: the tuple-like alternative unpacks, the plain one arrives whole
      using type = fn::copack_for<std::tuple<int, int>, int>;
      constexpr auto handler = fn::overload{add2, [](int v) noexcept -> int { return -v; }};
      CHECK(type{std::tuple{2, 3}}.apply(handler) == 5);
      CHECK(type{7}.apply(handler) == -7);

      SECTION("constexpr")
      {
        static_assert(type{std::tuple{2, 3}}.apply(handler) == 5);
        static_assert(type{7}.apply(handler) == -7);
        SUCCEED();
      }
    }

    SECTION("whole-tuple callable")
    {
      // pass-whole serves a callable viable only for the whole tuple; a generic callable unpacks
      // (the tuple arm wins ordering); extra arguments leave the alternative whole
      constexpr auto whole = [](std::tuple<int, int> const &) noexcept -> int { return -1; };
      CHECK(a.apply(whole) == -1);

      constexpr auto arity = [](auto &&...args) noexcept -> int { return (0 + ... + (static_cast<void>(args), 1)); };
      copack<std::tuple<int, int, int>> b{std::tuple{1, 2, 3}};
      CHECK(b.apply(arity) == 3);
      CHECK(b.apply(arity, 0) == 2);

      SECTION("constexpr")
      {
        static_assert(copack<std::tuple<int, int>>{std::tuple{2, 3}}.apply(whole) == -1);
        static_assert(copack<std::tuple<int, int, int>>{std::tuple{1, 2, 3}}.apply(arity) == 3);
        static_assert(copack<std::tuple<int, int, int>>{std::tuple{1, 2, 3}}.apply(arity, 0) == 2);
        SUCCEED();
      }
    }

    SECTION("elements are terminal")
    {
      // a copack element inside the tuple alternative arrives whole, never dispatched
      constexpr auto takes_copack = [](copack<int> const &) noexcept -> int { return 9; };
      copack<std::tuple<copack<int>>> b{std::tuple<copack<int>>{copack<int>{1}}};
      CHECK(b.apply(takes_copack) == 9);
      static_assert(not fn::typelist_applicable<decltype([](int) {}), copack<std::tuple<copack<int>>> &>);

      SECTION("constexpr")
      {
        static_assert(copack<std::tuple<copack<int>>>{std::tuple<copack<int>>{copack<int>{1}}}.apply(takes_copack)
                      == 9);
        SUCCEED();
      }
    }

    SECTION("apply_r")
    {
      static_assert(std::is_same_v<long, decltype(a.template apply_r<long>(add2))>);
      CHECK(a.template apply_r<long>(add2) == 5L);

      SECTION("constexpr")
      {
        static_assert(copack<std::tuple<int, int>>{std::tuple{2, 3}}.template apply_r<long>(add2) == 5L);
        SUCCEED();
      }
    }

    SECTION("noexcept")
    {
      // apply weighs the unpacked invocation
      static_assert(noexcept(a.apply(add2)));
      constexpr auto throwing = [](int i, int j) noexcept(false) -> int { return i + j; };
      static_assert(not noexcept(a.apply(throwing)));
      SUCCEED();
    }
  }

  SECTION("structural type")
  {
    // copack is a structural type: a constexpr copack can be a template parameter, with
    // template-argument equivalence comparing the active alternative and its value
    constexpr copack<bool, int> a{42};
    constexpr copack<bool, int> b{42};
    constexpr copack<bool, int> c{17};
    constexpr copack<bool, int> d{true};
    static_assert(std::is_same_v<copack_nttp<a>, copack_nttp<b>>);
    static_assert(not std::is_same_v<copack_nttp<a>, copack_nttp<c>>);
    static_assert(not std::is_same_v<copack_nttp<a>, copack_nttp<d>>);
    static_assert(std::is_same_v<some_copack_nttp<a>, some_copack_nttp<b>>);
    static_assert(not std::is_same_v<some_copack_nttp<a>, some_copack_nttp<d>>);

    // the property composes: alternatives may be packs of different sizes mixed with a scalar
    using fn::pack;
    using S = fn::copack_for<pack<int, bool>, pack<double>, long>;
    constexpr S e{pack<int, bool>{42, true}};
    constexpr S f{pack<int, bool>{42, true}};
    constexpr S g{pack<int, bool>{43, true}};
    constexpr S h{pack<double>{0.5}};
    constexpr S i{42L};
    static_assert(std::is_same_v<some_copack_nttp<e>, some_copack_nttp<f>>);
    static_assert(not std::is_same_v<some_copack_nttp<e>, some_copack_nttp<g>>);
    static_assert(not std::is_same_v<some_copack_nttp<e>, some_copack_nttp<h>>);
    static_assert(not std::is_same_v<some_copack_nttp<h>, some_copack_nttp<i>>);

    // the template-parameter object is usable at runtime
    CHECK(read_nttp<a>() == 42.0);
    CHECK(read_nttp<e>() == 43.0); // the pack alternative spreads: 42 + true
    CHECK(read_nttp<i>() == 42.0);
  }
}

TEST_CASE("copack apply_type", "[copack][apply_type]")
{
  using fn::copack;
  using std::in_place_type_t;

  // the tag names the row of the dispatch table: arms match by exact alternative type, never by
  // conversion, and receive the alternative unpacked exactly as apply unpacks it
  constexpr auto arms
      = fn::overload{[](in_place_type_t<int>, int v) noexcept -> int { return v; },
                     [](in_place_type_t<double>, double d) noexcept -> int { return static_cast<int>(d) + 1000; }};
  copack<double, int> a{42};

  SECTION("noexcept")
  {
    static_assert(noexcept(a.apply_type(arms)));
    static_assert(noexcept(std::move(a).apply_type(arms)));
    static_assert(noexcept(a.apply_type_r<long>(arms)));
    constexpr auto throwing = fn::overload{[](in_place_type_t<int>, int v) noexcept(false) -> int { return v; },
                                           [](in_place_type_t<double>, double) noexcept -> int { return 0; }};
    static_assert(not noexcept(a.apply_type(throwing)));
    static_assert(not noexcept(a.apply_type_r<long>(throwing)));
    SUCCEED();
  }

  SECTION("airtight over interconvertible alternatives")
  {
    // the value path is subject to conversions: a lone double arm absorbs the int alternative
    static_assert(fn::typelist_applicable<decltype([](double) {}), copack<double, int> &>);
    // the tag path refuses: the int row has no arm, and asking answers
    constexpr auto only_double = fn::overload{[](in_place_type_t<double>, double) noexcept -> int { return 0; }};
    static_assert(not can_apply_type<copack<double, int> &, decltype(only_double) const &>);
    static_assert(can_apply_type<copack<double, int> &, decltype(arms) const &>);

    // the tag reaches the arm as a prvalue, so rvalue-tag arms are served - probe and deed agree
    constexpr auto rv_tag = fn::overload{[](in_place_type_t<int> &&, int v) noexcept -> int { return v; },
                                         [](in_place_type_t<double> &&, double) noexcept -> int { return 0; }};
    static_assert(can_apply_type<copack<double, int> &, decltype(rv_tag) const &>);
    CHECK(a.apply_type(rv_tag) == 42);

    CHECK(a.apply_type(arms) == 42);
    CHECK(copack<double, int>{0.5}.apply_type(arms) == 1000);

    SECTION("constexpr")
    {
      static_assert(copack<double, int>{42}.apply_type(arms) == 42);
      static_assert(copack<double, int>{0.5}.apply_type(arms) == 1000);
      SUCCEED();
    }
  }

  SECTION("value categories")
  {
    CHECK(a.apply_type(fn::overload{[](in_place_type_t<double>, auto &&) -> bool { throw 1; },
                                    [](in_place_type_t<int>, int &) -> bool { return true; },
                                    [](in_place_type_t<int>, int const &) -> bool { throw 0; },
                                    [](in_place_type_t<int>, int &&) -> bool { throw 0; },
                                    [](in_place_type_t<int>, int const &&) -> bool { throw 0; }}));
    CHECK(std::as_const(a).apply_type(fn::overload{[](in_place_type_t<double>, auto &&) -> bool { throw 1; },
                                                   [](in_place_type_t<int>, int &) -> bool { throw 0; },
                                                   [](in_place_type_t<int>, int const &) -> bool { return true; },
                                                   [](in_place_type_t<int>, int &&) -> bool { throw 0; },
                                                   [](in_place_type_t<int>, int const &&) -> bool { throw 0; }}));
    CHECK(std::move(std::as_const(a))
              .apply_type(fn::overload{[](in_place_type_t<double>, auto &&) -> bool { throw 1; },
                                       [](in_place_type_t<int>, int &) -> bool { throw 0; },
                                       [](in_place_type_t<int>, int const &) -> bool { throw 0; },
                                       [](in_place_type_t<int>, int &&) -> bool { throw 0; },
                                       [](in_place_type_t<int>, int const &&) -> bool { return true; }}));
    CHECK(std::move(a).apply_type(fn::overload{[](in_place_type_t<double>, auto &&) -> bool { throw 1; },
                                               [](in_place_type_t<int>, int &) -> bool { throw 0; },
                                               [](in_place_type_t<int>, int const &) -> bool { throw 0; },
                                               [](in_place_type_t<int>, int &&) -> bool { return true; },
                                               [](in_place_type_t<int>, int const &&) -> bool { throw 0; }}));

    SECTION("constexpr")
    {
      // one result type across ALL alternatives is the rule, so selection is encoded in values
      constexpr copack<double, int> b{42};
      constexpr auto categories = fn::overload{[](in_place_type_t<double>, auto &&) -> int { return 0; },
                                               [](in_place_type_t<int>, int &) -> int { return 1; },
                                               [](in_place_type_t<int>, int const &) -> int { return 2; },
                                               [](in_place_type_t<int>, int &&) -> int { return 3; },
                                               [](in_place_type_t<int>, int const &&) -> int { return 4; }};
      static_assert(b.apply_type(categories) == 2);
      static_assert(std::move(b).apply_type(categories) == 4);
      SUCCEED();
    }
  }

  SECTION("pack alternative")
  {
    // the arm receives (tag, elements...): the tag carries exactly what the unpacking loses
    using P = fn::pack<int, int>;
    copack<P> s{P{2, 3}};
    constexpr auto parms = fn::overload{[](in_place_type_t<P>, int x, int y) noexcept -> int { return x + y; }};
    CHECK(s.apply_type(parms) == 5);

    // the copack's category reaches the elements
    CHECK(s.apply_type(fn::overload{[](in_place_type_t<P>, int &, int &) -> bool { return true; },
                                    [](in_place_type_t<P>, int const &, int const &) -> bool { throw 0; }}));
    CHECK(std::as_const(s).apply_type(
        fn::overload{[](in_place_type_t<P>, int &, int &) -> bool { throw 0; },
                     [](in_place_type_t<P>, int const &, int const &) -> bool { return true; }}));

    SECTION("constexpr")
    {
      static_assert(copack<P>{P{2, 3}}.apply_type(parms) == 5);
      SUCCEED();
    }
  }

  SECTION("tuple-like alternative")
  {
    using T = std::tuple<int, int>;
    copack<T> s{T{20, 22}};
    constexpr auto tarms = fn::overload{[](in_place_type_t<T>, int x, int y) noexcept -> int { return x + y; }};
    CHECK(s.apply_type(tarms) == 42);
    CHECK(copack<std::array<int, 2>>{std::array{2, 3}}.apply_type(
              fn::overload{[](in_place_type_t<std::array<int, 2>>, int x, int y) noexcept -> int { return x + y; }})
          == 5);

    // the elements form is the row's one signature: an arm for the whole tuple is not served
    static_assert(not can_apply_type<copack<T> &, decltype(fn::overload{[](in_place_type_t<T>, T const &) -> int {
                                       return 0;
                                     }}) const &>);

    SECTION("constexpr")
    {
      static_assert(copack<T>{T{20, 22}}.apply_type(tarms) == 42);
      SUCCEED();
    }
  }

  SECTION("mixed alternatives, one arm set")
  {
    using P = fn::pack<int, int>;
    using T = std::tuple<int, bool>;
    using S = fn::copack_for<P, T, int>;
    constexpr auto marms = fn::overload{[](in_place_type_t<int>, int v) noexcept -> int { return -v; },
                                        [](in_place_type_t<P>, int x, int y) noexcept -> int { return x + y; },
                                        [](in_place_type_t<T>, int x, bool) noexcept -> int { return x; }};
    CHECK(S{7}.apply_type(marms) == -7);
    CHECK(S{P{2, 3}}.apply_type(marms) == 5);
    CHECK(S{T{9, true}}.apply_type(marms) == 9);

    // dropping any one arm makes the whole dispatch non-viable
    constexpr auto no_int = fn::overload{[](in_place_type_t<P>, int x, int y) noexcept -> int { return x + y; },
                                         [](in_place_type_t<T>, int x, bool) noexcept -> int { return x; }};
    static_assert(not can_apply_type<S &, decltype(no_int) const &>);
    static_assert(can_apply_type<S &, decltype(marms) const &>);

    SECTION("constexpr")
    {
      static_assert(S{7}.apply_type(marms) == -7);
      static_assert(S{P{2, 3}}.apply_type(marms) == 5);
      static_assert(S{T{9, true}}.apply_type(marms) == 9);
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

    using P = fn::pack<int, int>;
    copack<P> p{P{2, 3}};
    constexpr auto parms
        = fn::overload{[](in_place_type_t<P>, int x, int y, int z) noexcept -> int { return x + y + z; }};
    CHECK(p.apply_type(parms, 37) == 42);

    // an arm set that does not take the extra answers non-viable
    static_assert(not can_apply_type<copack<double, int> &, decltype(arms) const &, int>);
    static_assert(can_apply_type<copack<double, int> &, decltype(xarms) const &, int>);

    SECTION("constexpr")
    {
      static_assert(copack<double, int>{42}.apply_type(xarms, 2) == 44);
      static_assert(copack<P>{P{2, 3}}.apply_type(parms, 37) == 42);
      SUCCEED();
    }
  }

  SECTION("apply_type_r")
  {
    static_assert(std::is_same_v<long, decltype(a.apply_type_r<long>(arms))>);
    CHECK(a.apply_type_r<long>(arms) == 42L);
    CHECK(copack<double, int>{0.5}.apply_type_r<long>(arms) == 1000L);

    // the conversion to Ret is part of the question
    static_assert(not can_apply_type_r<copack<double, int> &, char *, decltype(arms) const &>);
    static_assert(can_apply_type_r<copack<double, int> &, long, decltype(arms) const &>);

    SECTION("constexpr")
    {
      static_assert(copack<double, int>{42}.apply_type_r<long>(arms) == 42L);
      SUCCEED();
    }
  }
}

namespace {
struct PassThrough {
  auto operator()(std::equality_comparable auto &&v) const -> std::remove_cvref_t<decltype(v)> { return FWD(v); }
};
} // namespace

TEST_CASE("copack noexcept", "[copack][noexcept]")
{
  using fn::copack;

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
    static_assert(noexcept(copack<int>{42}));
    static_assert(noexcept(copack<int>{std::in_place_type<int>, 42}));
    static_assert(noexcept(fn::as_copack(42)));
    static_assert(not noexcept(copack<Throwy>{std::declval<Throwy const &>()}));

    static_assert(std::is_nothrow_copy_constructible_v<copack<int>>);
    static_assert(std::is_nothrow_copy_constructible_v<copack<Quiet>>);
    static_assert(not std::is_nothrow_copy_constructible_v<copack<Throwy>>);
    static_assert(not std::is_nothrow_move_constructible_v<copack<Throwy>>);
    SUCCEED();
  }

  SECTION("dispatch")
  {
    using S = copack<bool, int>;
    constexpr auto nothrow_fn = [](auto) noexcept { return 0; };
    constexpr auto throwing_fn = [](auto) { return 0; };

    static_assert(noexcept(std::declval<S &>().apply(nothrow_fn)));
    static_assert(not noexcept(std::declval<S &>().apply(throwing_fn)));
    static_assert(noexcept(std::declval<S &>().template apply_r<int>(nothrow_fn)));
    static_assert(not noexcept(std::declval<S &>().template apply_r<int>(throwing_fn)));
    static_assert(noexcept(std::declval<S &>().transform(nothrow_fn)));
    static_assert(not noexcept(std::declval<S &>().transform(throwing_fn)));

    // which alternative runs is a run-time choice, so one throwing handler makes the whole
    // dispatch throwing
    constexpr auto mixed = fn::overload{[](int) noexcept { return 0; }, [](bool) { return 0; }};
    static_assert(not noexcept(std::declval<S &>().apply(mixed)));
    SUCCEED();
  }

  SECTION("comparison")
  {
    static_assert(noexcept(std::declval<copack<int> const &>() == std::declval<copack<int> const &>()));
    static_assert(noexcept(std::declval<copack<Quiet> const &>() == std::declval<copack<Quiet> const &>()));
    static_assert(not noexcept(std::declval<copack<Throwy> const &>() == std::declval<copack<Throwy> const &>()));

    // the rewritten != inherits it
    static_assert(noexcept(std::declval<copack<int> const &>() != std::declval<copack<int> const &>()));
    static_assert(not noexcept(std::declval<copack<Throwy> const &>() != std::declval<copack<Throwy> const &>()));
    SUCCEED();
  }

  SECTION("assignment")
  {
    // assignment weighs the alternative's own `operator=` - used when the alternative does not
    // change - and the construction that replaces it when it does
    static_assert(std::is_nothrow_copy_assignable_v<copack<int>>);
    static_assert(std::is_nothrow_copy_assignable_v<copack<Quiet>>);
    static_assert(std::is_nothrow_move_assignable_v<copack<int>>);
    static_assert(not std::is_nothrow_copy_assignable_v<copack<std::string>>);
    static_assert(std::is_nothrow_move_assignable_v<copack<std::string>>);

    // a throwing move constructor is constrained away - the replacement path has nowhere to fail
    // safely - and Throwy's throwing everything leaves no nothrow arm at all
    static_assert(not std::is_move_assignable_v<copack<Throwy>>);
    static_assert(not std::is_copy_assignable_v<copack<Throwy>>);

    // widening assignment weighs only the source's alternatives: Throwy, uninvolved, neither
    // forbids the assignment nor enters its specification
    static_assert(std::is_assignable_v<fn::copack_for<Quiet, Throwy> &, copack<Quiet> const &>);
    static_assert(noexcept(std::declval<fn::copack_for<Quiet, Throwy> &>() = std::declval<copack<Quiet> const &>()));
    struct Loud { // nothrow to deliver, throwing to assign: viability and the specification differ
      Loud() = default;
      Loud(Loud const &) noexcept {}
      Loud(Loud &&) noexcept {}
      Loud &operator=(Loud const &) noexcept(false) { return *this; }
      Loud &operator=(Loud &&) noexcept(false) { return *this; }
    };
    static_assert(std::is_assignable_v<fn::copack_for<Quiet, Loud> &, copack<Loud> const &>);
    static_assert(not noexcept(std::declval<fn::copack_for<Quiet, Loud> &>() = std::declval<copack<Loud> const &>()));

    // value assignment weighs only the alternative it takes: nothrow when both its delivery and
    // its own operator= are, throwing when either is not
    static_assert(std::is_assignable_v<fn::copack_for<Quiet, Throwy> &, Quiet const &>);
    static_assert(noexcept(std::declval<fn::copack_for<Quiet, Throwy> &>() = std::declval<Quiet const &>()));
    static_assert(std::is_assignable_v<fn::copack_for<Quiet, Loud> &, Loud const &>);
    static_assert(not noexcept(std::declval<fn::copack_for<Quiet, Loud> &>() = std::declval<Loud const &>()));
    SUCCEED();
  }
}

TEST_CASE("copack triviality", "[copack][triviality]")
{
  using fn::copack;

  SECTION("of fundamentals: every operation, as variant has")
  {
    using S = copack<double, int>;
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
    // a copack of packs is the normal form of the type algebra, and the join's cartesian product is
    // what a multidispatch pipeline copies at every stage
    using S = copack<fn::pack<int, double>>;
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
    using R = copack<fn::pack<int, int &>>;
    static_assert(std::is_trivially_destructible_v<R>);
    static_assert(std::is_trivially_copy_constructible_v<R>);
    static_assert(std::is_trivially_move_constructible_v<R>);
    static_assert(not std::is_copy_assignable_v<R>); // refused by the alternative, not merely non-trivial
    static_assert(not std::is_move_assignable_v<R>);

    // nothing about std::string is trivial, and everything still works
    using N = copack<std::string>;
    static_assert(not std::is_trivially_destructible_v<N>);
    static_assert(not std::is_trivially_copy_constructible_v<N>);
    static_assert(not std::is_trivially_copy_assignable_v<N>);
    static_assert(std::is_copy_assignable_v<N>);

    // one non-trivial alternative makes the operation non-trivial, never non-viable
    using M = fn::copack_for<std::string, int>;
    static_assert(not std::is_trivially_copy_assignable_v<M>);
    static_assert(std::is_copy_assignable_v<M>);
    SUCCEED();
  }
}

TEST_CASE("copack type collapsing", "[copack][transform][normalized]")
{
  using ::fn::copack;
  using ::fn::overload;
  using ::fn::detail::_collapsing_copack_tag;
  using ::fn::detail::_copack_apply_result;
  using ::fn::detail::_typelist_collapsing_copack;

  struct copack_double_int {};
  struct copack_bool {};
  struct copack_bool_int {};

  SECTION("one element")
  {
    constexpr auto fn = PassThrough{};
    using type = copack<double>;
    static_assert(std::same_as<typename _copack_apply_result<_collapsing_copack_tag, decltype(fn), type &>::type,
                               copack<double>>);
  }

  SECTION("two elements")
  {
    constexpr auto fn = PassThrough{};
    using type = copack<double, int>;
    static_assert(std::same_as<typename _copack_apply_result<_collapsing_copack_tag, decltype(fn), type &>::type,
                               copack<double, int>>);
  }

  SECTION("one copack, one element only")
  {
    constexpr auto fn = [](copack_bool const &) -> copack<bool> && { throw 0; };
    using type = copack<copack_bool>;
    static_assert(
        std::same_as<typename _copack_apply_result<_collapsing_copack_tag, decltype(fn), type &>::type, copack<bool>>);
  }

  SECTION("element and one copack with one element")
  {
    constexpr auto fn = overload{PassThrough{}, //
                                 [](copack_bool const &) -> copack<bool> && { throw 0; }};
    using type = copack<double, copack_bool>;
    static_assert(std::same_as<typename _copack_apply_result<_collapsing_copack_tag, decltype(fn), type &>::type,
                               copack<bool, double>>);
  }

  SECTION("one copack with two elements")
  {
    constexpr auto fn = [](copack_bool_int const &) -> copack<bool, int> && { throw 0; };
    using type = copack<copack_bool_int>;
    static_assert(std::same_as<typename _copack_apply_result<_collapsing_copack_tag, decltype(fn), type &>::type,
                               copack<bool, int>>);
  }

  SECTION("copack with two elements and copack with one element")
  {
    constexpr auto fn = overload{[](copack_bool_int const &) -> copack<bool, int> && { throw 0; },
                                 [](copack_bool const &) -> copack<bool> && { throw 0; }};
    using type = copack<copack_bool_int, copack_bool>;
    static_assert(std::same_as<typename _copack_apply_result<_collapsing_copack_tag, decltype(fn), type &>::type,
                               copack<bool, int>>);
  }

  SECTION("two copacks with two elements and two elements")
  {
    constexpr auto fn = overload{PassThrough{}, [](copack_double_int const &) -> copack<double, int> { throw 0; },
                                 [](copack_bool_int const &) -> copack<bool, int> const { throw 0; }};
    using type = copack<copack_bool_int, copack_double_int, double, int>;
    static_assert(std::same_as<typename _copack_apply_result<_collapsing_copack_tag, decltype(fn), type &>::type,
                               copack<bool, double, int>>);
  }

  SECTION("a result no copack can hold")
  {
    // a void-returning callback must drop the caller's candidate in the immediate context: the
    // collapsing machinery would hard-error where no requires-expression can absorb it
    constexpr auto fnVoid = [](auto &&...) {};
    static_assert(not can_transform<copack<double, int> &, decltype(fnVoid)>);
    static_assert(not can_transform<copack<double, int> const &, decltype(fnVoid)>);
    static_assert(not can_transform<copack<double, int> &&, decltype(fnVoid)>);
    static_assert(not can_transform<copack<double, int> const &&, decltype(fnVoid)>);
    static_assert(can_transform<copack<double, int> &, PassThrough>); // the same copack, a holdable result
    SUCCEED();
  }
}

TEST_CASE("copack transform", "[copack][transform]")
{
  using ::fn::copack;
  constexpr auto fn1 = [](auto i) noexcept -> std::size_t { return sizeof(i); };

  using type = copack<double>;
  static_assert(type::size == 1);

  type a{std::in_place_type<double>, 0.5};
  CHECK(a.data.v0 == 0.5);

  static_assert(type{0.5}.transform(fn1) == copack{std::size_t{8}});
  CHECK(a.transform(     //
            fn::overload{//
                         [](auto) -> int { throw 1; }, [](double &i) -> bool { return i == 0.5; },
                         [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                         [](double const &&) -> bool { throw 0; }})
        == copack<bool, int>{true});
  CHECK(std::as_const(a).transform( //
            fn::overload{           //
                         [](auto) -> int { throw 1; }, [](double &) -> bool { throw 0; },
                         [](double const &i) -> bool { return i == 0.5; }, [](double &&) -> bool { throw 0; },
                         [](double const &&) -> bool { throw 0; }})
        == copack<bool, int>{true});
  CHECK(std::move(std::as_const(a))
            .transform(      //
                fn::overload{//
                             [](auto) -> int { throw 1; }, [](double &) -> bool { throw 0; },
                             [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                             [](double const &&i) -> bool { return i == 0.5; }})
        == copack<bool, int>{true});
  CHECK(std::move(a).transform( //
            fn::overload{       //
                         [](auto) -> int { throw 1; }, [](double &) -> bool { throw 0; },
                         [](double const &) -> bool { throw 0; }, [](double &&i) -> bool { return i == 0.5; },
                         [](double const &&) -> bool { throw 0; }})
        == copack<bool, int>{true});

  SECTION("extra arguments")
  {
    constexpr auto add = [](double i, int j) noexcept -> double { return i + j; };
    static_assert(std::same_as<decltype(a.transform(add, 3)), copack<double>>);

    CHECK(a.transform(add, 3) == copack{3.5});
    CHECK(std::as_const(a).transform(add, 3) == copack{3.5});
    CHECK(std::move(std::as_const(a)).transform(add, 3) == copack{3.5});
    CHECK(std::move(a).transform(add, 3) == copack{3.5});

    SECTION("constexpr")
    {
      constexpr type b{std::in_place_type<double>, 0.5};
      static_assert(b.transform(add, 3) == copack{3.5});
      static_assert(std::move(b).transform(add, 3) == copack{3.5});
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

TEST_CASE("copack move and copy", "[copack][has_value][get_ptr]")
{
  using fn::copack;

  SECTION("move and copy")
  {
    SECTION("one type only")
    {
      using T = copack<std::string>;
      T a{std::in_place_type<std::string>, "baz"};
      CHECK(a.apply([](auto &&i) { return i; }) == "baz");

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
      CHECK(a.apply([](auto &&i) { return i; }) == "baz");
      CHECK(b.apply([](auto &&i) { return i; }) == "baz");

      T c{std::move(a)};
      CHECK(c.apply([](auto &&i) { return i; }) == "baz");
    }

    SECTION("mixed with other types")
    {
      using T = copack<std::string, std::string_view>;
      T a{std::in_place_type<std::string>, "baz"};
      CHECK(a.apply([](auto &&i) { return std::string(i); }) == "baz");

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
      CHECK(a.apply([](auto &&i) { return std::string(i); }) == "baz");
      CHECK(b.apply([](auto &&i) { return std::string(i); }) == "baz");

      T c{std::move(a)};
      CHECK(c.apply([](auto &&i) { return std::string(i); }) == "baz");
    }
  }

  SECTION("copy only")
  {
    SECTION("one type only")
    {
      using T = copack<CopyOnly>;
      T a{std::in_place_type<CopyOnly>, 12};
      CHECK(a.apply([](auto &&i) { return static_cast<int>(i); }) == 12);

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
      CHECK(a.apply([](auto &&i) { return static_cast<int>(i); }) == 12);
      CHECK(b.apply([](auto &&i) { return static_cast<int>(i); }) == 12);
    }

    SECTION("mixed with other types")
    {
      using T = fn::copack_for<CopyOnly, double, int>; // copack_for: canonical order is platform-specific
      T a{std::in_place_type<CopyOnly>, 12};
      CHECK(a.apply([](auto &&i) { return static_cast<int>(i); }) == 12);

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
      CHECK(a.apply([](auto &&i) { return static_cast<int>(i); }) == 12);
      CHECK(b.apply([](auto &&i) { return static_cast<int>(i); }) == 12);
    }
  }

  SECTION("move only")
  {
    SECTION("one type only")
    {
      using T = copack<MoveOnly>;
      T a{std::in_place_type<MoveOnly>, 12};
      CHECK(a.apply([](auto &&i) { return static_cast<int>(i); }) == 12);

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
      CHECK(a.apply([](auto &&i) { return static_cast<int>(i); }) == -1);
      CHECK(b.apply([](auto &&i) { return static_cast<int>(i); }) == 12);
    }

    SECTION("mixed with other types")
    {
      using T = fn::copack_for<MoveOnly, double, int>; // copack_for: canonical order is platform-specific
      T a{std::in_place_type<MoveOnly>, 12};
      CHECK(a.apply([](auto &&i) { return static_cast<int>(i); }) == 12);

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
      CHECK(a.apply([](auto &&i) { return static_cast<int>(i); }) == -1);
      CHECK(b.apply([](auto &&i) { return static_cast<int>(i); }) == 12);
    }
  }

  SECTION("immovable type")
  {
    SECTION("one type only")
    {
      using T = copack<NonCopyable>;
      T a{std::in_place_type<NonCopyable>, 12};
      CHECK(a.apply([](auto &&i) { return static_cast<int>(i); }) == 12);

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
      using T = fn::copack_for<NonCopyable, double, int>; // copack_for: canonical order is platform-specific
      T a{std::in_place_type<NonCopyable>, 12};
      CHECK(a.apply([](auto &&i) { return static_cast<int>(i); }) == 12);

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
    static_assert(std::is_constructible_v<fn::copack_for<CopyOnly, int>, copack<CopyOnly> const &>);
    static_assert(std::is_constructible_v<fn::copack_for<CopyOnly, int>, copack<CopyOnly> &&>); // binds const &

    static_assert(not std::is_constructible_v<fn::copack_for<MoveOnly, int>, copack<MoveOnly> const &>);
    static_assert(std::is_constructible_v<fn::copack_for<MoveOnly, int>, copack<MoveOnly> &&>);

    static_assert(not std::is_constructible_v<fn::copack_for<NonCopyable, int>, copack<NonCopyable> const &>);
    static_assert(not std::is_constructible_v<fn::copack_for<NonCopyable, int>, copack<NonCopyable> &&>);

    copack<MoveOnly> a{std::in_place_type<MoveOnly>, 12};
    fn::copack_for<MoveOnly, int> b{std::move(a)};
    CHECK(b.apply([](auto &&i) { return static_cast<int>(i); }) == 12);
    CHECK(a.apply([](auto &&i) { return static_cast<int>(i); }) == -1); // moved from
  }

  SECTION("noexcept")
  {
    // the copy and move constructors weigh the alternative they relocate; the destructor cannot
    // throw whatever the alternatives are
    static_assert(not std::is_nothrow_copy_constructible_v<Throwing>);
    static_assert(not std::is_nothrow_move_constructible_v<Throwing>);
    static_assert(not std::is_nothrow_copy_constructible_v<copack<Throwing>>);
    static_assert(not std::is_nothrow_move_constructible_v<copack<Throwing>>);
    static_assert(std::is_nothrow_destructible_v<copack<Throwing>>);

    static_assert(std::is_nothrow_copy_constructible_v<copack<int>>);
    static_assert(std::is_nothrow_move_constructible_v<copack<int>>);
    SUCCEED();
  }
}

TEST_CASE("copack assignment", "[copack][assignment]")
{
  using fn::copack;

  SECTION("same alternative")
  {
    copack<bool, int> a{12};
    copack<bool, int> const b{42};
    a = b;
    CHECK(a == copack{42});
    CHECK(a.has_value(std::in_place_type<int>));

    a = copack<bool, int>{7};
    CHECK(a == copack{7});
  }

  SECTION("the alternative changes")
  {
    copack<bool, int> a{12};
    copack<bool, int> const b{true};
    a = b;
    CHECK(a == copack{true});
    CHECK(a.has_value(std::in_place_type<bool>));
    CHECK(not a.has_value(std::in_place_type<int>));

    a = copack<bool, int>{42};
    CHECK(a == copack{42});
    CHECK(a.has_value(std::in_place_type<int>));
  }

  SECTION("self-assignment")
  {
    copack<bool, int> a{12};
    copack<bool, int> const &self = a; // through an alias: `a = a` is a warning, and rightly so
    a = self;
    CHECK(a == copack{12});
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
    using T = fn::copack_for<Tracked, int>;
    constexpr auto probe
        = fn::overload{[](Tracked const &t) { return t.assigned ? t.v : -t.v; }, [](int const &) { return 0; }};

    T a{std::in_place_type<Tracked>, 7};
    a = T{std::in_place_type<Tracked>, 5};
    CHECK(a.apply(probe) == 5); // assigned, in place

    a = T{12}; // a different alternative is replaced by construction ...
    a = T{std::in_place_type<Tracked>, 3};
    CHECK(a.apply(probe) == -3); // ... so this Tracked was constructed, not assigned

    static_assert([] {
      T a{std::in_place_type<Tracked>, 7};
      a = T{std::in_place_type<Tracked>, 5};
      return a.apply([](auto const &t) {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(t)>, Tracked>)
          return t.assigned && t.v == 5;
        else
          return false;
      });
    }());
  }

  SECTION("the replaced alternative is destroyed")
  {
    using T = fn::copack_for<Counted, int>; // copack<...> order is platform-specific; copack_for normalizes
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
    // a narrower copack assigns on the widening constructors' terms, deciding per incoming
    // alternative: the one held is assigned in place, any other replaces by construction
    constexpr auto basic = [] {
      copack<bool, int> a{12};
      copack<int> const n{42};
      a = n; // the alternative in hand, by copy
      bool ok = a == copack{42};
      a = copack<bool>{true}; // a different alternative
      ok = ok && a == copack{true};
      a = copack<int>{7}; // and back, by move
      return ok && a == copack{7};
    };
    CHECK(basic());
    static_assert(basic());

    SECTION("one relocation, or none")
    {
      // `wide = narrow` used to route through the widening constructor: a whole temporary copack, one
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
        using W = fn::copack_for<Reloc, int>;
        fn::copack<Reloc> n{Reloc{42, &copied, &moved, &assigned}};
        fn::copack<Reloc> m{Reloc{9, &copied, &moved, &assigned}};

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
      using S = fn::copack_for<Fixed, int>;
      static_assert(not std::is_copy_assignable_v<S>);
      static_assert(std::is_assignable_v<S &, copack<int> const &>);
      static_assert(std::is_assignable_v<S &, copack<int> &&>);
      static_assert(not std::is_assignable_v<S &, copack<Fixed> const &>);

      struct Skittish final { // assignable, but no arm can deliver it without throwing
        constexpr Skittish() noexcept = default;
        Skittish(Skittish const &) noexcept(false) {}
        Skittish(Skittish &&) noexcept(false) {}
        Skittish &operator=(Skittish const &) noexcept { return *this; }
        Skittish &operator=(Skittish &&) noexcept { return *this; }
      };
      using S2 = fn::copack_for<Skittish, int>;
      static_assert(not std::is_copy_assignable_v<S2>);
      static_assert(std::is_assignable_v<S2 &, copack<int> const &>);
      static_assert(not std::is_assignable_v<S2 &, copack<Skittish> const &>);

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
      using S3 = fn::copack_for<MoveAssign, int>;
      static_assert(std::is_assignable_v<S3 &, copack<MoveAssign> const &>); // through the constructor
      static_assert(std::is_assignable_v<S3 &, copack<MoveAssign> &&>);      // directly

      // a copack it is not a superset of is refused
      static_assert(not std::is_assignable_v<copack<int> &, copack<bool> const &>);
      SUCCEED();
    }
  }

  SECTION("from a value")
  {
    // a value of exactly one alternative assigns directly, as the converting constructors take
    // one: assigned in place when it is the alternative held, replacing it by construction
    // otherwise - `*this = copack{v}` with the temporary elided
    constexpr auto basic = [] {
      copack<bool, int> a{12};
      a = 42; // the alternative in hand
      bool ok = a == copack{42};
      a = true; // a different alternative
      ok = ok && a == copack{true};
      int const i = 7;
      a = i; // and back, from a const lvalue
      return ok && a == copack{7};
    };
    CHECK(basic());
    static_assert(basic());

    SECTION("no hidden temporary")
    {
      // the route this overload replaces - converting constructor, then whole-copack assignment -
      // relocated every value through a temporary copack; the direct delivery is one relocation
      // into place, or none at all
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
        using W = fn::copack_for<Reloc, int>;
        Reloc n{42, &copied, &moved, &assigned};
        Reloc m{9, &copied, &moved, &assigned};

        W a{12};
        copied = moved = assigned = 0;
        a = std::as_const(n); // a different alternative, by copy: one copy, straight into place
        bool ok = copied == 1 && moved == 0 && assigned == 0 && a.get_ptr<Reloc>()->v == 42;

        copied = moved = assigned = 0;
        a = std::as_const(n); // the alternative in hand: assigned with its own operator=, no relocation
        ok = ok && copied == 0 && moved == 0 && assigned == 1;

        a = W{12};
        copied = moved = assigned = 0;
        a = std::move(m); // a different alternative, by move: one move, nothing else
        return ok && copied == 0 && moved == 1 && assigned == 0 && a.get_ptr<Reloc>()->v == 9;
      };
      CHECK(counts());
      static_assert(counts());
    }

    SECTION("not hostage to a sibling alternative")
    {
      // the defect this overload fixes: through the temporary route, the whole-copack assignment's
      // constraints let an uninvolved alternative forbid the operation
      struct Fixed final { // copyable, not assignable
        constexpr Fixed() noexcept = default;
        constexpr Fixed(Fixed const &) noexcept = default;
        constexpr Fixed(Fixed &&) noexcept = default;
        Fixed &operator=(Fixed const &) = delete;
        Fixed &operator=(Fixed &&) = delete;
      };
      using S = fn::copack_for<Fixed, int>;
      static_assert(not std::is_copy_assignable_v<S>);             // the whole type still refuses
      static_assert(std::is_assignable_v<S &, int>);               // its sibling is not hostage
      static_assert(not std::is_assignable_v<S &, Fixed const &>); // the alternative's own refusal holds
      constexpr auto witness = [] {
        S s{std::in_place_type<Fixed>};
        s = 42; // the non-assignable alternative is replaced by construction
        return s.has_value(std::in_place_type<int>);
      };
      CHECK(witness());
      static_assert(witness());
    }

    SECTION("exact alternative only")
    {
      // never std::variant's converting-assignment resolution: a convertible non-alternative is
      // rejected - conversions keep their explicit spellings - and interconvertible alternatives
      // never meet in overload resolution
      static_assert(std::is_assignable_v<fn::copack_for<double, int> &, int>);
      static_assert(std::is_assignable_v<fn::copack_for<double, int> &, double>);
      static_assert(not std::is_assignable_v<fn::copack_for<double, int> &, short>);
      static_assert(not std::is_assignable_v<fn::copack_for<double, int> &, float>);
      constexpr auto exact = [] {
        fn::copack_for<double, int> s{1.5};
        s = 42; // int lands on int, not on double
        return s.has_value(std::in_place_type<int>);
      };
      CHECK(exact());
      static_assert(exact());
    }

    SECTION("constraints")
    {
      // each conjunct refuses on its own, and the gate follows the incoming value category,
      // collapsing to the widening assignment's copy gate for an lvalue and its move gate for
      // an rvalue
      struct MoveAssign final { // move-assignable only: the copy form declines, the move form serves
        constexpr MoveAssign() noexcept = default;
        constexpr MoveAssign(MoveAssign const &) noexcept = default;
        constexpr MoveAssign(MoveAssign &&) noexcept = default;
        MoveAssign &operator=(MoveAssign const &) = delete;
        constexpr MoveAssign &operator=(MoveAssign &&) noexcept = default;
      };
      using S = fn::copack_for<MoveAssign, int>;
      static_assert(std::is_assignable_v<S &, MoveAssign>);
      // ... while the constructor route survives where the direct overload declines: a temporary
      // copack, then whole-type move assignment, with the old costs and the old gating
      static_assert(std::is_assignable_v<S &, MoveAssign const &>);

      struct NoCopy final { // assignable, not copy-deliverable: the copy form has no route at all
        constexpr NoCopy() noexcept = default;
        NoCopy(NoCopy const &) = delete;
        constexpr NoCopy(NoCopy &&) noexcept = default;
        constexpr NoCopy &operator=(NoCopy const &) noexcept = default;
        constexpr NoCopy &operator=(NoCopy &&) noexcept = default;
      };
      using S2 = fn::copack_for<NoCopy, int>;
      static_assert(not std::is_assignable_v<S2 &, NoCopy const &>);
      static_assert(std::is_assignable_v<S2 &, NoCopy>);

      struct Skittish final { // assignable, but no arm can deliver it without throwing
        constexpr Skittish() noexcept = default;
        Skittish(Skittish const &) noexcept(false) {}
        Skittish(Skittish &&) noexcept(false) {}
        Skittish &operator=(Skittish const &) noexcept { return *this; }
        Skittish &operator=(Skittish &&) noexcept { return *this; }
      };
      using S3 = fn::copack_for<Skittish, int>;
      static_assert(not std::is_assignable_v<S3 &, Skittish const &>);
      static_assert(not std::is_assignable_v<S3 &, Skittish>);
      SUCCEED();
    }
  }

  SECTION("constexpr")
  {
    static_assert([] {
      copack<bool, int> a{42};
      copack<bool, int> const b{true};
      a = b;
      return a == copack{true};
    }());
    static_assert([] {
      copack<bool, int> a{true};
      a = copack<bool, int>{42};
      return a == copack{42};
    }());
    static_assert([] {
      copack<bool, int> a{42};
      copack<bool, int> const &self = a;
      a = self;
      return a == copack{42};
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
      static_assert(std::is_copy_assignable_v<copack<SnapAssign>>);
      static_assert(not std::is_nothrow_copy_assignable_v<copack<SnapAssign>>);
      constexpr auto value = [](SnapAssign const &t) { return t.v; };

      copack<SnapAssign> a{SnapAssign{7}};
      copack<SnapAssign> const bad{SnapAssign{0}};
      CHECK_THROWS_AS(a = bad, std::runtime_error);
      CHECK(a.apply(value) == 7); // restored

      // the same arm, completing
      copack<SnapAssign> const good{SnapAssign{5}};
      a = good;
      CHECK(a.apply(value) == 5);

      static_assert([] {
        copack<SnapAssign> a{SnapAssign{7}};
        copack<SnapAssign> const good{SnapAssign{5}};
        a = good; // the snapshot arm, in a constant expression
        return a.apply([](SnapAssign const &t) { return t.v; }) == 5;
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
      static_assert(std::is_copy_assignable_v<copack<TempAssign>>);
      static_assert(not std::is_nothrow_copy_assignable_v<copack<TempAssign>>);
      constexpr auto value = [](TempAssign const &t) { return t.v; };

      copack<TempAssign> a{TempAssign{7}};
      copack<TempAssign> const bad{TempAssign{0}};
      CHECK_THROWS_AS(a = bad, std::runtime_error); // the copy into the temporary throws
      CHECK(a.apply(value) == 7);                   // untouched

      copack<TempAssign> const good{TempAssign{5}};
      a = good;
      CHECK(a.apply(value) == 5);

      static_assert([] {
        copack<TempAssign> a{TempAssign{7}};
        copack<TempAssign> const good{TempAssign{5}};
        a = good; // the temporary arm, in a constant expression
        return a.apply([](TempAssign const &t) { return t.v; }) == 5;
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
      using M = fn::copack_for<ThrowingCopy, ThrowingMove>;
      constexpr auto value = fn::overload{[](ThrowingCopy const &t) { return t.v; }, //
                                          [](ThrowingMove const &t) { return t.v; }};

      M m{std::in_place_type<ThrowingMove>, 7}; // in place: its move would throw
      M const bad_copy{ThrowingCopy{0}};
      CHECK_THROWS_AS(m = bad_copy, std::runtime_error); // the copy into the temporary throws
      CHECK(m.apply(value) == 7);                        // ... and the ThrowingMove is still there

      M const good{std::in_place_type<ThrowingMove>, 3};
      m = good; // the same alternative: assigned in place, its throwing move never used
      CHECK(m.apply(value) == 3);

      // ... and the same temporary arm run to completion: a throwing-path test alone never lets
      // the replacement finish, so this copy succeeds and the old alternative is destroyed
      M const good_copy{ThrowingCopy{5}};
      m = good_copy;
      CHECK(m.apply(value) == 5);
    }
  }

  SECTION("constraints")
  {
    // Assignment requires of every alternative its own operator=: a copack does not offer an
    // operation its alternative refuses.
    static_assert(not std::is_copy_assignable_v<NonCopyable>);
    static_assert(not std::is_copy_constructible_v<NonCopyable>);
    static_assert(not std::is_copy_assignable_v<copack<NonCopyable>>); // it cannot be copied at all
    static_assert(not std::is_move_assignable_v<copack<NonCopyable>>);

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
    static_assert(not std::is_copy_assignable_v<copack<NoAssign>>); // the copack respects the refusal
    static_assert(not std::is_move_assignable_v<copack<NoAssign>>);

    // ... above all where the refusal is load-bearing: a pack holding a reference deletes its
    // assignment because C++ cannot rebind a reference, and reconstructing the pack in place would
    // synthesize exactly the operation the language does not have
    static_assert(std::is_copy_constructible_v<fn::pack<int, int &>>);
    static_assert(not std::is_copy_assignable_v<fn::pack<int, int &>>);
    static_assert(not std::is_copy_assignable_v<copack<fn::pack<int, int &>>>);
    static_assert(not std::is_move_assignable_v<copack<fn::pack<int, int &>>>);

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
    static_assert(not std::is_copy_assignable_v<copack<ThrowingBoth>>);
    static_assert(not std::is_move_assignable_v<copack<ThrowingBoth>>);

    // The arm is chosen per ALTERNATIVE, not for the copack as a whole - both for the assignment of
    // an unchanged alternative and for the construction that replaces one. A copack whose
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
    static_assert(std::is_nothrow_copy_assignable_v<copack<QuietCopy>>); // nothing on its path throws
    static_assert(std::is_move_assignable_v<copack<QuietCopy>>);         // the copy assignment takes the rvalue
    static_assert(std::is_copy_assignable_v<copack<QuietMove>>);         // replaced through the temporary
    static_assert(not std::is_nothrow_copy_assignable_v<copack<QuietMove>>);
    static_assert(std::is_nothrow_move_assignable_v<copack<QuietMove>>);

    using Mixed = fn::copack_for<QuietCopy, QuietMove>; // one alternative per arm
    static_assert(std::is_copy_assignable_v<Mixed>);
    static_assert(not std::is_nothrow_copy_assignable_v<Mixed>); // QuietMove's copy can throw
    static_assert(std::is_move_assignable_v<Mixed>);             // the copy assignment takes the rvalue ...
    static_assert(not std::is_nothrow_move_assignable_v<Mixed>); // ... and says that it can throw
  }
}

TEST_CASE("copack emplace", "[copack][emplace]")
{
  using fn::copack;

  // the mutation path for alternatives that do not support assignment: destroy-and-reconstruct,
  // requested at the call site by name, so the constraint asks about the incoming alternative
  // alone - the outgoing one is only destroyed, whatever its own traits
  struct Sender final { // the motivating archetype: not assignable, nothrow-move-constructible
    int target;
    constexpr explicit Sender(int t) noexcept : target(t) {}
    constexpr Sender(Sender &&) noexcept = default;
    Sender(Sender const &) = delete;
    Sender &operator=(Sender const &) = delete;
    Sender &operator=(Sender &&) = delete;
  };
  using S = fn::copack_for<Sender, int>;
  static_assert(not std::is_copy_assignable_v<S>);
  static_assert(not std::is_move_assignable_v<S>);
  static_assert(not std::is_assignable_v<S &, Sender>); // even the value operator= declines

  SECTION("re-targets a copack of senders")
  {
    constexpr auto battery = [] {
      S s{std::in_place_type<Sender>, 1};
      Sender &r = s.emplace<Sender>(42);
      bool ok = r.target == 42 && s.has_value(std::in_place_type<Sender>);
      int &i = s.emplace<int>(7); // a different alternative
      ok = ok && i == 7 && s.has_value(std::in_place_type<int>);
      s.emplace<Sender>(3); // and back
      return ok && s.has_value(std::in_place_type<Sender>);
    };
    CHECK(battery());
    static_assert(battery());
    static_assert(std::same_as<decltype(std::declval<S &>().emplace<int>(1)), int &>);
  }

  SECTION("always destroys and reconstructs")
  {
    // also when T is the alternative already held: assign-when-same is operator='s meaning and
    // stays there, and unconditional reconstruction is what serves non-assignable types
    struct Counter final {
      int v;
      int *constructed;
      int *destroyed;
      int *assigned;
      constexpr Counter(int i, int *c, int *d, int *a) noexcept : v(i), constructed(c), destroyed(d), assigned(a)
      {
        ++*constructed;
      }
      constexpr Counter(Counter &&o) noexcept
          : v(o.v), constructed(o.constructed), destroyed(o.destroyed), assigned(o.assigned)
      {
      }
      constexpr Counter &operator=(Counter &&o) noexcept
      {
        v = o.v;
        ++*assigned;
        return *this;
      }
      constexpr ~Counter() { ++*destroyed; }
    };
    constexpr auto counts = [] {
      int constructed = 0;
      int destroyed = 0;
      int assigned = 0;
      using T = fn::copack_for<Counter, int>;
      T s{std::in_place_type<Counter>, 7, &constructed, &destroyed, &assigned};

      constructed = destroyed = assigned = 0;
      Counter &r = s.emplace<Counter>(9, &constructed, &destroyed, &assigned);
      return r.v == 9 && constructed == 1 && destroyed == 1 && assigned == 0;
    };
    CHECK(counts());
    static_assert(counts());
  }

  SECTION("arm selection")
  {
    // nothrow construction goes straight into storage; a throwing construction is built into a
    // temporary first - the storage untouched until it cannot throw - and delivered by exactly
    // one nothrow move
    struct Fragile final {
      int v;
      int *moved;
      constexpr Fragile(int i, int *m) noexcept(false) : v(i), moved(m) {}
      constexpr Fragile(Fragile &&o) noexcept : v(o.v), moved(o.moved) { ++*moved; }
    };
    constexpr auto arms = [] {
      int moved = 0;
      using T = fn::copack_for<Fragile, int>;
      T s{12};
      Fragile &r = s.emplace<Fragile>(5, &moved); // arm 2: one move
      bool ok = r.v == 5 && moved == 1;
      moved = 0;
      int &i = s.emplace<int>(3); // arm 1: nothing relocates
      return ok && i == 3 && moved == 0;
    };
    CHECK(arms());
    static_assert(arms());

    // noexcept is the first arm's, exactly
    using T = fn::copack_for<Fragile, int>;
    static_assert(noexcept(std::declval<T &>().emplace<int>(1)));
    static_assert(not noexcept(std::declval<T &>().emplace<Fragile>(1, std::declval<int *>())));
    SUCCEED();
  }

  SECTION("re-points a reference")
  {
    // copack<pack<int, int&>> has no assignment at all - the pack deletes it because C++ cannot
    // rebind a reference - and emplace is the explicit spelling of that mutation
    using P = fn::pack<int, int &>;
    using R = copack<P>;
    static_assert(not std::is_copy_assignable_v<R>);
    static_assert(not std::is_move_assignable_v<R>);
    static_assert(can_emplace<R, P, int, int &>);
    constexpr auto repoint = [] {
      int x = 1;
      int y = 2;
      R s{std::in_place_type<P>, 5, x};
      s.emplace<P>(6, y);
      using std::get;
      P *p = s.get_ptr<P>();
      return p != nullptr && &get<1>(*p) == &y && get<0>(*p) == 6;
    };
    CHECK(repoint());
    static_assert(repoint());
  }

  SECTION("constraints")
  {
    static_assert(can_emplace<S, Sender, int>);
    static_assert(can_emplace<S, int, int>);
    static_assert(not can_emplace<S, double, double>);       // not an alternative
    static_assert(not can_emplace<S, Sender, char const *>); // not makeable from the arguments

    // a type that can be neither built nor moved without throwing leaves no safe arm; its
    // sibling is not held hostage
    struct ThrowingBoth final {
      ThrowingBoth() = default;
      ThrowingBoth(ThrowingBoth const &) noexcept(false) {}
      ThrowingBoth(ThrowingBoth &&) noexcept(false) {}
    };
    using T = fn::copack_for<ThrowingBoth, int>;
    static_assert(not can_emplace<T, ThrowingBoth, ThrowingBoth const &>);
    static_assert(can_emplace<T, int, int>);
    SUCCEED();
  }
}

TEST_CASE("get on a singular copack", "[copack][get]")
{
  struct A final {
    int v;
    constexpr bool operator==(A const &) const = default;
  };
  constexpr auto can_get = [](auto &&c) { return requires { fn::get(FWD(c)); }; };

  // the alternative comes back carrying the copack's cv-qualification and value category, exactly
  // as apply would pass it
  fn::copack<A> c{A{7}};
  static_assert(std::is_same_v<decltype(fn::get(c)), A &>);
  static_assert(std::is_same_v<decltype(fn::get(std::as_const(c))), A const &>);
  static_assert(std::is_same_v<decltype(fn::get(std::move(c))), A &&>);
  static_assert(std::is_same_v<decltype(fn::get(std::move(std::as_const(c)))), A const &&>);
  static_assert(noexcept(fn::get(c)));
  CHECK(fn::get(c) == A{7});
  fn::get(c).v = 9;
  CHECK(fn::get(std::as_const(c)) == A{9});
  CHECK(fn::get(std::move(c)) == A{9});

  SECTION("constexpr")
  {
    constexpr fn::copack<A> cc{A{5}};
    static_assert(fn::get(cc) == A{5});
    static_assert([] {
      fn::copack<A> m{A{1}};
      fn::get(m).v = 2;
      return fn::get(std::move(m)).v;
    }() == 2);

    SUCCEED();
  }

  SECTION("constraints")
  {
    // only the singular copack qualifies; a multi-alternative one dispatches, it does not get
    static_assert(can_get(fn::copack<A>{A{1}}));
    static_assert(not can_get(fn::copack_for<A, int>{1}));
    static_assert(not can_get(A{1}));

    SUCCEED();
  }
}
