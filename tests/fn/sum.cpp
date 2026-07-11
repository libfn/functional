// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/pack.hpp>
#include <fn/sum.hpp>
#include <fn/utility.hpp>

#include <catch2/catch_all.hpp>

#include <concepts>
#include <stdexcept>
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
} // anonymous namespace

TEST_CASE("sum basic functionality tests", "[sum]")
{
  // NOTE This test looks very similar to test in choice.cpp - for good reason.

  using fn::sum;

  WHEN("sum<> unit")
  {
    static_assert(sum<>::size == 0);
    static_assert(sum<>::has_type<bool> == false);
    static_assert(std::same_as<fn::sum_for<sum<>, sum<>>, sum<>>);
  }

  WHEN("as_sum")
  {
    constexpr auto a = fn::as_sum(12);
    static_assert(std::same_as<decltype(a), fn::sum<int> const>);
    static_assert(a == fn::sum{12});

    constexpr auto b = fn::as_sum(std::in_place_type<long>, 12);
    static_assert(std::same_as<decltype(b), fn::sum<long> const>);
    static_assert(b == fn::sum{12l});

    WHEN("constraints")
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

  WHEN("sum_for")
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

  WHEN("invocable")
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
  }

  WHEN("check destructor call")
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

  WHEN("single parameter constructor")
  {
    constexpr sum<int> a = 12;
    static_assert(a == sum{12});

    constexpr sum<bool> b{false};
    static_assert(b == sum{false});

    WHEN("CTAD")
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

    WHEN("move from rvalue")
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

    WHEN("copy from lvalue")
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

  WHEN("forwarding constructors (immovable)")
  {
    sum<NonCopyable> a{std::in_place_type<NonCopyable>, 42};
    CHECK(a.invoke([](auto &i) -> bool { return i.v == 42; }));

    WHEN("CTAD")
    {
      constexpr auto a = sum{std::in_place_type<NonCopyable>, 42};
      static_assert(std::is_same_v<decltype(a), sum<NonCopyable> const>);

      auto b = sum{std::in_place_type<NonCopyable>, 42};
      static_assert(std::is_same_v<decltype(b), sum<NonCopyable>>);
    }

    WHEN("constraints")
    {
      static_assert(can_in_place<sum<NonCopyable>, NonCopyable, int>);
      static_assert(not can_in_place<sum<NonCopyable>, int, int>); // int is not an alternative

      // an argument list the alternative cannot be constructed from is not viable - it used to be
      // reported viable, then fail to compile inside variadic_union, beyond SFINAE's reach
      static_assert(not can_in_place<sum<NonCopyable>, NonCopyable>);               // no default ctor
      static_assert(not can_in_place<sum<NonCopyable>, NonCopyable, char const *>); // not constructible from
      SUCCEED();
    }
  }

  WHEN("forwarding constructors (aggregate)")
  {
    WHEN("constraints")
    {
      using T = std::array<int, 3>;
      // the element is brace-initialized, which elides braces for an aggregate - a constraint
      // spelled with is_constructible_v (parenthesized init) would reject this very construction
      static_assert(can_in_place<sum<T>, T, int, int, int>);
      static_assert(not can_in_place<sum<T>, T, int, int, int, int>); // one too many
      static_assert(not can_in_place<sum<int>, int, double>);         // narrowing, rejected by braces
      SUCCEED();
    }

    WHEN("regular")
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

    WHEN("constexpr")
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

    WHEN("CTAD")
    {
      constexpr auto a = sum{std::in_place_type<std::array<int, 3>>, 1, 2, 3};
      static_assert(std::is_same_v<decltype(a), sum<std::array<int, 3>> const>);

      auto b = sum{std::in_place_type<std::array<int, 3>>, 1, 2, 3};
      static_assert(std::is_same_v<decltype(b), sum<std::array<int, 3>>>);
    }
  }

  WHEN("has_type type mismatch")
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
  }

  WHEN("equality comparison")
  {
    using type = sum<bool, int>;

    // != is synthesized by C++20 rewriting from ==, so it cannot claim to be viable where == is not.
    // Against the uninstantiable sum<> it used to report itself viable and then fail in its own body
    static_assert(can_eq<type, type>);
    static_assert(can_ne<type, type>);
    static_assert(not can_eq<sum<int>, sum<>>);
    static_assert(not can_ne<sum<int>, sum<>>);
    static_assert(not can_eq<sum<>, sum<int>>);
    static_assert(not can_ne<sum<>, sum<int>>);

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

    WHEN("constexpr")
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
  }

  WHEN("invoke")
  {
    sum<int> a{std::in_place_type<int>, 42};

    WHEN("value only")
    {
      static_assert(std::is_same_v<bool, decltype(a.invoke(fn::overload{[](auto) -> bool { throw 1; },
                                                                        [](int) -> bool { return true; }}))>);

      CHECK(a.invoke(fn::overload{[](auto) -> bool { throw 1; }, [](int &) -> bool { return true; },
                                  [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                  [](int const &&) -> bool { throw 0; }}));
      CHECK(std::as_const(a).invoke(fn::overload{
          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { return true; },
          [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; }}));
      CHECK(std::move(std::as_const(a))
                .invoke(fn::overload{[](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                     [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                     [](int const &&) -> bool { return true; }}));
      CHECK(std::move(a).invoke(fn::overload{[](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                             [](int const &) -> bool { throw 0; }, [](int &&) -> bool { return true; },
                                             [](int const &&) -> bool { throw 0; }}));

      WHEN("constexpr")
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

    WHEN("extra arguments")
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

      WHEN("constexpr")
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

  WHEN("invoke_r")
  {
    sum<int> a{std::in_place_type<int>, 42};

    WHEN("value only")
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

      WHEN("constexpr")
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

    WHEN("extra arguments")
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

      WHEN("constexpr")
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

  WHEN("sum of packs")
  {
    using fn::pack;
    constexpr sum a{pack{"abc", 42, 12.5}};
    static_assert(std::is_same_v<decltype(a), sum<pack<char const(&)[4], int, double>> const>);

    WHEN("constexpr")
    {
      constexpr auto b
          = a.invoke([]<std::size_t I>(char const(&)[I], int i, double d) { return I + i + static_cast<int>(d); });
      static_assert(b == 4 + 42 + 12);

      constexpr sum<pack<int, int, int, int>, pack<int, int, int>, pack<int, int>, pack<int>> c = pack{3, 14, 15};
      static_assert(c.invoke([](std::integral auto... args) -> int { return (... + args); }) == 3 + 14 + 15);

      SUCCEED();
    }

    WHEN("runtime")
    {
      auto const b = a.invoke([](char const *s, int i, double d) { return std::strlen(s) + i + static_cast<int>(d); });
      CHECK(b == 3 + 42 + 12);

      constexpr sum<pack<int, int, int, int>, pack<int, int, int>, pack<int, int>, pack<int>> c = pack{3, 14, 15, 92};
      CHECK(c.invoke([](std::integral auto... args) -> int { return (... + args); }) == 3 + 14 + 15 + 92);
    }
  }

  WHEN("structural type")
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
    bool operator==(Throwy const &) const noexcept(false) { return true; }
  };
  struct Quiet {
    Quiet() = default;
    Quiet(Quiet const &) noexcept {}
    Quiet(Quiet &&) noexcept {}
    bool operator==(Quiet const &) const noexcept { return true; }
  };

  WHEN("constructors")
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

  WHEN("dispatch")
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

  WHEN("comparison")
  {
    static_assert(noexcept(std::declval<sum<int> const &>() == std::declval<sum<int> const &>()));
    static_assert(noexcept(std::declval<sum<Quiet> const &>() == std::declval<sum<Quiet> const &>()));
    static_assert(not noexcept(std::declval<sum<Throwy> const &>() == std::declval<sum<Throwy> const &>()));

    // the rewritten != inherits it
    static_assert(noexcept(std::declval<sum<int> const &>() != std::declval<sum<int> const &>()));
    static_assert(not noexcept(std::declval<sum<Throwy> const &>() != std::declval<sum<Throwy> const &>()));
    SUCCEED();
  }

  WHEN("assignment")
  {
    // assignment weighs the construction it performs: the copy, or - where that copy can throw - the
    // move of the temporary it is made into
    static_assert(std::is_nothrow_copy_assignable_v<sum<int>>);
    static_assert(std::is_nothrow_copy_assignable_v<sum<Quiet>>);
    static_assert(std::is_nothrow_move_assignable_v<sum<int>>);
    static_assert(not std::is_nothrow_copy_assignable_v<sum<std::string>>);
    static_assert(std::is_nothrow_move_assignable_v<sum<std::string>>);

    // moving is nothrow wherever it is offered at all: a throwing move is constrained away, having
    // nowhere to fail safely (see TEST_CASE "sum assignment")
    static_assert(not std::is_move_assignable_v<sum<Throwy>>);
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

  WHEN("one element")
  {
    constexpr auto fn = PassThrough{};
    using type = sum<double>;
    static_assert(
        std::same_as<typename _sum_invoke_result<_collapsing_sum_tag, decltype(fn), type &>::type, sum<double>>);
  }

  WHEN("two elements")
  {
    constexpr auto fn = PassThrough{};
    using type = sum<double, int>;
    static_assert(
        std::same_as<typename _sum_invoke_result<_collapsing_sum_tag, decltype(fn), type &>::type, sum<double, int>>);
  }

  WHEN("one sum, one element only")
  {
    constexpr auto fn = [](sum_bool const &) -> sum<bool> && { throw 0; };
    using type = sum<sum_bool>;
    static_assert(
        std::same_as<typename _sum_invoke_result<_collapsing_sum_tag, decltype(fn), type &>::type, sum<bool>>);
  }

  WHEN("element and one sum with one element")
  {
    constexpr auto fn = overload{PassThrough{}, //
                                 [](sum_bool const &) -> sum<bool> && { throw 0; }};
    using type = sum<double, sum_bool>;
    static_assert(
        std::same_as<typename _sum_invoke_result<_collapsing_sum_tag, decltype(fn), type &>::type, sum<bool, double>>);
  }

  WHEN("one sum with two elements")
  {
    constexpr auto fn = [](sum_bool_int const &) -> sum<bool, int> && { throw 0; };
    using type = sum<sum_bool_int>;
    static_assert(
        std::same_as<typename _sum_invoke_result<_collapsing_sum_tag, decltype(fn), type &>::type, sum<bool, int>>);
  }

  WHEN("sum with two elements and sum with one element")
  {
    constexpr auto fn = overload{[](sum_bool_int const &) -> sum<bool, int> && { throw 0; },
                                 [](sum_bool const &) -> sum<bool> && { throw 0; }};
    using type = sum<sum_bool_int, sum_bool>;
    static_assert(
        std::same_as<typename _sum_invoke_result<_collapsing_sum_tag, decltype(fn), type &>::type, sum<bool, int>>);
  }

  WHEN("two sums with two elements and two elements")
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

TEST_CASE("sum functions", "[sum][invoke]")
{
  using namespace fn;
  constexpr auto fn1 = [](auto i) noexcept -> std::size_t { return sizeof(i); };

  sum<int> a{std::in_place_type<int>, 42};
  static_assert(decltype(a)::size == 1);
  CHECK(a.data.v0 == 42);

  CHECK(a.invoke(fn1) == 4);
  CHECK(a.invoke(  //
      fn::overload{//
                   [](auto) -> bool { throw 1; }, [](int &i) -> bool { return i == 42; },
                   [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                   [](int const &&) -> bool { throw 0; }}));
  CHECK(std::as_const(a).invoke( //
      fn::overload{              //
                   [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                   [](int const &i) -> bool { return i == 42; }, [](int &&) -> bool { throw 0; },
                   [](int const &&) -> bool { throw 0; }}));
  CHECK(std::move(std::as_const(a))
            .invoke(         //
                fn::overload{//
                             [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                             [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                             [](int const &&i) -> bool { return i == 42; }}));
  CHECK(std::move(a).invoke( //
      fn::overload{          //
                   [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                   [](int &&i) -> bool { return i == 42; }, [](int const &&) -> bool { throw 0; }}));
}

TEST_CASE("sum move and copy", "[sum][has_value][get_ptr]")
{
  using fn::sum;

  WHEN("sum_for")
  {
    static_assert(std::same_as<fn::sum_for<int>, fn::sum<int>>);
    static_assert(std::same_as<fn::sum_for<int, bool>, fn::sum<bool, int>>);
    static_assert(std::same_as<fn::sum_for<bool, int>, fn::sum<bool, int>>);
    // Canonical order is platform-specific by design (see the note in the WHEN("sum_for") block of
    // the TEST_CASE above); assert only the platform-independent guarantees here.
    static_assert(std::same_as<fn::sum_for<NonCopyable, int>, fn::sum_for<int, NonCopyable>>);
    static_assert(std::same_as<fn::sum_for<int, bool, NonCopyable>, fn::sum_for<NonCopyable, bool, int>>);
    static_assert(fn::sum_for<int, bool, NonCopyable>::size == 3);
  }

  WHEN("move and copy")
  {
    WHEN("one type only")
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

    WHEN("mixed with other types")
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

  WHEN("copy only")
  {
    WHEN("one type only")
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

    WHEN("mixed with other types")
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

  WHEN("move only")
  {
    WHEN("one type only")
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

    WHEN("mixed with other types")
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

  WHEN("immovable type")
  {
    WHEN("one type only")
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

    WHEN("mixed with other types")
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
}

TEST_CASE("sum", "[sum][has_value][get_ptr]")
{
  // NOTE We have 5 different specializations, need to test each
  using namespace fn;

  constexpr auto fn1 = [](auto) {};
  constexpr auto fn2 = [](auto...) {};
  constexpr auto fn3 = overload{[](int &) {}, [](double &) {}};

  constexpr sum<std::array<int, 3>> a{std::in_place_type<std::array<int, 3>>, 3, 14, 15};
  static_assert(a.index == 0);
  static_assert(decltype(a)::template has_type<std::array<int, 3>>);
  static_assert(not decltype(a)::template has_type<int>);
  static_assert(a.has_value(std::in_place_type<std::array<int, 3>>));
  static_assert(a.template has_value<std::array<int, 3>>());
  static_assert(a.invoke([](auto &i) -> bool {
    return std::same_as<std::array<int, 3> const &, decltype(i)> && i.size() == 3 && i[0] == 3 && i[1] == 14
           && i[2] == 15;
  }));

  using T = sum<double, int>;
  T b{std::in_place_type<int>, 42};
  static_assert(T::size == 2);
  static_assert(std::is_same_v<T::template select_nth<0>, double>);
  static_assert(std::is_same_v<T::template select_nth<1>, int>);
  static_assert(T::has_type<int>);
  static_assert(T::has_type<double>);
  static_assert(not T::has_type<bool>);
  CHECK(b.index == 1);
  CHECK(b.template has_value<int>());
  CHECK(b.has_value(std::in_place_type<int>));

  static_assert(fn::typelist_invocable<decltype(fn1), decltype(b)>);
  static_assert(fn::typelist_invocable<decltype(fn2), decltype(b)>);
  static_assert(fn::typelist_invocable<decltype(fn2), decltype(b)>);
  static_assert(fn::typelist_invocable<decltype(fn3), decltype(b) &>);
  static_assert(not fn::typelist_invocable<decltype(fn3), decltype(b) const &>);
  static_assert(not fn::typelist_invocable<decltype(fn3), decltype(b)>);
  static_assert(not fn::typelist_invocable<decltype(fn3), decltype(b) const>);
  static_assert(not fn::typelist_invocable<decltype(fn3), decltype(b) &&>);
  static_assert(not fn::typelist_invocable<decltype(fn3), decltype(b) const &&>);

  static_assert(std::is_same_v<decltype(b.get_ptr(std::in_place_type<int>)), int *>);
  CHECK(b.get_ptr(std::in_place_type<int>) == &b.data.v1);
  CHECK(b.get_ptr(std::in_place_type<double>) == nullptr);
  static_assert(std::is_same_v<decltype(std::as_const(b).get_ptr(std::in_place_type<int>)), int const *>);
  CHECK(std::as_const(b).get_ptr(std::in_place_type<int>) == &b.data.v1);
  CHECK(std::as_const(b).get_ptr(std::in_place_type<double>) == nullptr);

  T const c{std::in_place_type<double>, 4.25};
  CHECK(c.get_ptr(std::in_place_type<int>) == nullptr);
  CHECK(c.get_ptr(std::in_place_type<double>) == &c.data.v0);

  constexpr auto d = sum<double, int>{std::in_place_type<int>, 12};
  static_assert(d.get_ptr(std::in_place_type<double>) == nullptr);
  static_assert(*d.get_ptr(std::in_place_type<int>) == 12);
}

TEST_CASE("sum assignment", "[sum][assignment]")
{
  using fn::sum;

  WHEN("same alternative")
  {
    sum<bool, int> a{12};
    sum<bool, int> const b{42};
    a = b;
    CHECK(a == sum{42});
    CHECK(a.has_value(std::in_place_type<int>));

    a = sum<bool, int>{7};
    CHECK(a == sum{7});
  }

  WHEN("the alternative changes")
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

  WHEN("self-assignment")
  {
    sum<bool, int> a{12};
    sum<bool, int> const &self = a; // through an alias: `a = a` is a warning, and rightly so
    a = self;
    CHECK(a == sum{12});
    CHECK(a.has_value(std::in_place_type<int>));
  }

  WHEN("the replaced alternative is destroyed")
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
      a = T{std::in_place_type<Counted>, 3}; // same alternative: the old one still goes
      CHECK(Counted::live == 1);
    }
    CHECK(Counted::live == 0);
  }

  WHEN("constexpr")
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

  WHEN("strong exception guarantee")
  {
    // The alternative held is destroyed only once its replacement is certain: where the copy can
    // throw, it is made into a temporary first, and only the (nothrow) move touches the storage.
    struct ThrowingCopy final {
      int v;
      ThrowingCopy(int i) noexcept : v(i) {} // NOLINT: implicit on purpose
      ThrowingCopy(ThrowingCopy const &o) : v(o.v)
      {
        if (v == 0)
          throw std::runtime_error("copy");
      }
      ThrowingCopy(ThrowingCopy &&o) noexcept : v(o.v) {}
    };
    static_assert(std::is_copy_assignable_v<sum<ThrowingCopy>>);
    static_assert(not std::is_nothrow_copy_assignable_v<sum<ThrowingCopy>>);

    sum<ThrowingCopy> a{ThrowingCopy{7}};
    sum<ThrowingCopy> const bad{ThrowingCopy{0}};
    CHECK_THROWS_AS(a = bad, std::runtime_error);
    CHECK(a.invoke([](ThrowingCopy const &t) { return t.v; }) == 7); // untouched

    WHEN("the two arms in one sum")
    {
      // whichever arm the incoming alternative needs, the one being replaced survives a throw
      struct ThrowingMove final { // takes the direct arm: its move is never called
        int v;
        ThrowingMove(int i) noexcept : v(i) {} // NOLINT: implicit on purpose
        ThrowingMove(ThrowingMove const &o) noexcept : v(o.v) {}
        ThrowingMove(ThrowingMove &&) { throw std::runtime_error("move"); }
      };
      using M = fn::sum_for<ThrowingCopy, ThrowingMove>;
      constexpr auto value = fn::overload{[](ThrowingCopy const &t) { return t.v; }, //
                                          [](ThrowingMove const &t) { return t.v; }};

      M m{std::in_place_type<ThrowingMove>, 7}; // in place: its move would throw
      M const bad_copy{ThrowingCopy{0}};
      CHECK_THROWS_AS(m = bad_copy, std::runtime_error); // the copy into the temporary throws
      CHECK(m.invoke(value) == 7);                       // ... and the ThrowingMove is still there

      M const good{std::in_place_type<ThrowingMove>, 3};
      m = good; // the direct arm: copied straight over, its throwing move never used
      CHECK(m.invoke(value) == 3);
    }
  }

  WHEN("constraints")
  {
    // Assignment CONSTRUCTS the new alternative, so it asks nothing of the alternatives' own
    // assignment operators: a type with none at all is still assignable through the sum.
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
    static_assert(not std::is_copy_assignable_v<NoAssign>);
    static_assert(std::is_copy_assignable_v<sum<NoAssign>>); // ... yet the sum of it is
    static_assert(std::is_move_assignable_v<sum<NoAssign>>);

    // An alternative that can be neither copied nor moved without throwing has no safe arm: the
    // temporary would itself have to be moved into the storage, and snapshotting the old one is no
    // help when the old one is the same type. It is constrained away rather than half-served.
    struct ThrowingBoth final {
      ThrowingBoth() = default;
      ThrowingBoth(ThrowingBoth const &) noexcept(false) {}
      ThrowingBoth(ThrowingBoth &&) noexcept(false) {}
    };
    static_assert(not std::is_copy_assignable_v<sum<ThrowingBoth>>);
    static_assert(not std::is_move_assignable_v<sum<ThrowingBoth>>);

    // The arm is chosen per ALTERNATIVE, not for the sum as a whole: one that is nothrow-copyable is
    // built straight over the old, and one whose copy can throw is copied into a temporary first. A
    // sum whose alternatives need different arms is therefore still assignable.
    struct QuietCopy final { // needs the direct arm: its move throws
      QuietCopy() = default;
      QuietCopy(QuietCopy const &) noexcept = default;
      QuietCopy(QuietCopy &&) noexcept(false) {}
    };
    struct QuietMove final { // needs the temporary arm: its copy throws
      QuietMove() = default;
      QuietMove(QuietMove const &) noexcept(false) {}
      QuietMove(QuietMove &&) noexcept = default;
    };
    static_assert(std::is_nothrow_copy_assignable_v<sum<QuietCopy>>); // direct
    static_assert(std::is_move_assignable_v<sum<QuietCopy>>);         // the copy assignment takes the rvalue
    static_assert(std::is_copy_assignable_v<sum<QuietMove>>);         // through the temporary
    static_assert(not std::is_nothrow_copy_assignable_v<sum<QuietMove>>);
    static_assert(std::is_nothrow_move_assignable_v<sum<QuietMove>>);

    using Mixed = fn::sum_for<QuietCopy, QuietMove>; // one alternative per arm
    static_assert(std::is_copy_assignable_v<Mixed>);
    static_assert(not std::is_nothrow_copy_assignable_v<Mixed>); // QuietMove's copy can throw
    static_assert(std::is_move_assignable_v<Mixed>);             // the copy assignment takes the rvalue ...
    static_assert(not std::is_nothrow_move_assignable_v<Mixed>); // ... and says that it can throw
  }
}
