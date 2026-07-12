// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/pack.hpp>
#include <fn/sum.hpp>
#include <fn/utility.hpp>

#include <catch2/catch_all.hpp>

#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

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

// Every operation sum performs on an alternative - copy, move, compare - can throw here, while the
// sum member wrapping it promises noexcept regardless. Witnesses the #280 tripwires below.
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

// Comparison probes are type-keyed rather than the file's usual value-taking lambda: sum<> has no
// values to pass one (its default constructor is deleted, by design).
template <typename L, typename R>
concept can_eq = requires { std::declval<L const &>() == std::declval<R const &>(); };
template <typename L, typename R>
concept can_ne = requires { std::declval<L const &>() != std::declval<R const &>(); };

template <typename S, typename T, typename... Args>
concept can_in_place = requires(Args... args) { S{std::in_place_type<T>, args...}; };
} // anonymous namespace

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

    // GAP #280 (converse): neither lift carries a noexcept specifier, so both report noexcept(false)
    // even where nothing can throw. The same conditional-noexcept work fixes over- and under-promise.
    static_assert(not noexcept(fn::as_sum(12)));
    static_assert(not noexcept(fn::as_sum(std::in_place_type<long>, 12)));
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
      // GAP #280 (converse): the value constructors carry no noexcept specifier at all, so they
      // report noexcept(false) even for an alternative that cannot throw. This under-promise is the
      // safe direction - an unnecessary EH edge, not a terminate - but it is equally inaccurate.
      static_assert(std::is_nothrow_constructible_v<int, int>);
      static_assert(not noexcept(sum<int>{42}));
      static_assert(not noexcept(sum<int>{std::in_place_type<int>, 42}));
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

      // GAP #284: the constructor requires only has_type<T> - it never checks that T is
      // constructible from the arguments. So a bad argument list is reported viable here and then
      // fails to compile inside variadic_union, outside the immediate context and beyond SFINAE's
      // reach. Both of these should read `not`, and will once the conjunct is added.
      static_assert(can_in_place<sum<NonCopyable>, NonCopyable>);               // no default ctor
      static_assert(can_in_place<sum<NonCopyable>, NonCopyable, char const *>); // not constructible from
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
      // GAP #280: all three widening constructors are unconditionally noexcept, though each copies
      // or moves every alternative of the source into the wider union.
      using X = fn::sum_for<Throwing, int>;
      static_assert(noexcept(X{std::declval<sum<Throwing> const &>()}));
      static_assert(noexcept(X{std::declval<sum<Throwing> &&>()}));
      static_assert(noexcept(X{std::in_place_type<sum<Throwing>>, std::declval<sum<Throwing> const &>()}));
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

      // GAP #280: both operators are unconditionally noexcept, yet operator== reaches into the
      // alternative's own comparison - a throwing one terminates rather than propagating.
      static_assert(not noexcept(std::declval<Throwing const &>() == std::declval<Throwing const &>()));
      static_assert(noexcept(std::declval<T const &>() == std::declval<T const &>()));
      static_assert(noexcept(std::declval<T const &>() != std::declval<T const &>()));

      // GAP #281: operator!= drops the two sizeof...>0 conjuncts operator== carries, so against a
      // sum<> operand it reports itself viable and then fails to compile in its own body (which
      // calls operator==). Benign only because no object of sum<> can exist to call it with.
      static_assert(can_eq<sum<int>, sum<int>>);
      static_assert(can_ne<sum<int>, sum<int>>);
      static_assert(not can_eq<sum<int>, sum<>>);
      static_assert(can_ne<sum<int>, sum<>>);
      static_assert(not can_eq<sum<>, sum<>>);
      static_assert(can_ne<sum<>, sum<>>);

      SUCCEED();
    }
  }

  SECTION("invoke")
  {
    sum<int> a{std::in_place_type<int>, 42};

    SECTION("noexcept")
    {
      // GAP #280: invoke's noexcept is unconditional, so a callback that can throw - every visitor
      // in the sections below throws - still reports noexcept(true); the throw is std::terminate.
      constexpr auto throwing = [](int i) noexcept(false) -> bool { return i == 42; };
      static_assert(not noexcept(throwing(42)));
      static_assert(noexcept(a.invoke(throwing)));
      static_assert(noexcept(std::as_const(a).invoke(throwing)));
      static_assert(noexcept(std::move(a).invoke(throwing)));
      static_assert(noexcept(std::move(std::as_const(a)).invoke(throwing)));

      constexpr auto throwing2 = [](int i, std::monostate) noexcept(false) -> bool { return i == 42; };
      static_assert(noexcept(a.invoke(throwing2, std::monostate{}))); // extra arguments, same promise
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
      // GAP #280: the same unconditional promise as invoke, the conversion to Ret included.
      constexpr auto throwing = [](int i) noexcept(false) -> bool { return i == 42; };
      static_assert(noexcept(a.template invoke_r<bool>(throwing)));
      static_assert(noexcept(std::as_const(a).template invoke_r<bool>(throwing)));
      static_assert(noexcept(std::move(a).template invoke_r<bool>(throwing)));
      static_assert(noexcept(std::move(std::as_const(a)).template invoke_r<bool>(throwing)));
      static_assert(noexcept(a.template invoke_r<long>(throwing))); // converting the result, too
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
    // GAP #280: transform promises noexcept in every value category no matter what the callback
    // promises - the visitors above all throw. The typed-dispatch internals behind operator& and
    // the widening constructors carry the same unconditional promise.
    constexpr auto throwing = [](double i) noexcept(false) -> bool { return i == 0.5; };
    static_assert(noexcept(a.transform(throwing)));
    static_assert(noexcept(std::as_const(a).transform(throwing)));
    static_assert(noexcept(std::move(a).transform(throwing)));
    static_assert(noexcept(std::move(std::as_const(a)).transform(throwing)));

    constexpr auto typed = []<typename T>(std::in_place_type_t<T>, auto &&v) noexcept(false) -> bool {
      return static_cast<double>(v) == 0.5;
    };
    static_assert(noexcept(std::as_const(a)._transform(typed)));
    static_assert(noexcept(std::move(a)._transform(typed)));
    static_assert(noexcept(std::as_const(a).template _invoke<bool>(typed)));
    static_assert(noexcept(std::move(a).template _invoke<bool>(typed)));

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
    // GAP #280: the copy and move constructors are declared unconditionally noexcept, so an
    // alternative whose own copy or move throws terminates instead of propagating. The destructor's
    // promise, by contrast, is accurate - destroying the alternatives here cannot throw.
    static_assert(not std::is_nothrow_copy_constructible_v<Throwing>);
    static_assert(not std::is_nothrow_move_constructible_v<Throwing>);
    static_assert(std::is_nothrow_copy_constructible_v<sum<Throwing>>);
    static_assert(std::is_nothrow_move_constructible_v<sum<Throwing>>);
    static_assert(std::is_nothrow_destructible_v<sum<Throwing>>);
    SUCCEED();
  }
}
