// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/pack.hpp"
#include "functional/sum.hpp"
#include "functional/utility.hpp"

#include <catch2/catch_all.hpp>

#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

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

} // anonymous namespace

TEST_CASE("sum basic functionality tests", "[sum]")
{
  // NOTE This test looks very similar to test in choice.cpp - for good reason.

  using fn::some_in_place_type;
  using fn::sum;

  WHEN("sum_for")
  {
    static_assert(std::same_as<fn::sum_for<int>, fn::sum<int>>);
    static_assert(std::same_as<fn::sum_for<int, bool>, fn::sum<bool, int>>);
    static_assert(std::same_as<fn::sum_for<bool, int>, fn::sum<bool, int>>);
    static_assert(std::same_as<fn::sum_for<int, NonCopyable>, fn::sum<NonCopyable, int>>);
    static_assert(std::same_as<fn::sum_for<NonCopyable, int>, fn::sum<NonCopyable, int>>);
    static_assert(std::same_as<fn::sum_for<int, bool, NonCopyable>, fn::sum<NonCopyable, bool, int>>);
  }

  WHEN("invocable")
  {
    using type = sum<TestType, int>;
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

  WHEN("type_invocable")
  {
    using type = sum<TestType, int>;
    static_assert(fn::typelist_type_invocable<decltype([](auto, auto) {}), type &>);
    static_assert(fn::typelist_type_invocable<decltype([](some_in_place_type auto, auto) {}), type &>);
    static_assert(fn::typelist_type_invocable<decltype([](some_in_place_type auto, auto &) {}), type &>);
    static_assert(fn::typelist_type_invocable<decltype(fn::overload{[](some_in_place_type auto, int &) {},
                                                                    [](some_in_place_type auto, TestType &) {}}),
                                              type &>);
    static_assert(fn::typelist_type_invocable<decltype(fn::overload{[](some_in_place_type auto, int) {},
                                                                    [](some_in_place_type auto, TestType) {}}),
                                              type const &>);
    static_assert(not fn::typelist_type_invocable<decltype([](some_in_place_type auto, TestType &) {}),
                                                  type &>); // missing int
    static_assert(not fn::typelist_type_invocable<decltype([](some_in_place_type auto, int &) {}),
                                                  type &>); // missing TestType
    static_assert(not fn::typelist_type_invocable<decltype(fn::overload{[](some_in_place_type auto, int &&) {},
                                                                        [](some_in_place_type auto, TestType &&) {}}),
                                                  type &>); // cannot bind lvalue to rvalue-reference
    static_assert(not fn::typelist_type_invocable<decltype([](some_in_place_type auto, auto &) {}),
                                                  type &&>); // cannot bind rvalue to lvalue-reference
    static_assert(not fn::typelist_type_invocable<decltype([](auto) {}), type &>); // bad arity
    static_assert(not fn::typelist_type_invocable<decltype(fn::overload{[](some_in_place_type auto, int &) {},
                                                                        [](some_in_place_type auto, TestType &) {}}),
                                                  type const &>); // cannot bind const to non-const reference

    static_assert(fn::typelist_type_invocable<decltype([](some_in_place_type auto, auto &) {}), sum<NonCopyable> &>);
    static_assert(not fn::typelist_type_invocable<decltype([](some_in_place_type auto, auto) {}),
                                                  NonCopyable &>); // copy-constructor not available

    static_assert(fn::typelist_type_invocable<decltype([](some_in_place_type auto, auto &) {}), sum<NonCopyable> &>);
    static_assert(not fn::typelist_type_invocable<decltype([](some_in_place_type auto, auto) {}),
                                                  NonCopyable &>); // copy-constructor not available
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

    WHEN("CTAD with const")
    {
      constexpr auto a = sum{std::in_place_type<NonCopyable const>, 42};
      static_assert(std::is_same_v<decltype(a), sum<NonCopyable const> const>);

      auto b = sum{std::in_place_type<NonCopyable const>, 42};
      static_assert(std::is_same_v<decltype(b), sum<NonCopyable const>>);
    }
  }

  WHEN("forwarding constructors (aggregate)")
  {
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

    WHEN("CTAD with const")
    {
      constexpr auto a = sum{std::in_place_type<std::array<int, 3> const>, 1, 2, 3};
      static_assert(std::is_same_v<decltype(a), sum<std::array<int, 3> const> const>);

      auto b = sum{std::in_place_type<std::array<int, 3> const>, 1, 2, 3};
      static_assert(std::is_same_v<decltype(b), sum<std::array<int, 3> const>>);
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
      static_assert(std::is_same_v<bool, decltype(a.invoke(fn::overload([](auto) -> bool { throw 1; },
                                                                        [](int) -> bool { return true; })))>);

      CHECK(a.invoke(fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { return true; },
                                  [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                  [](int const &&) -> bool { throw 0; })));
      CHECK(std::as_const(a).invoke(fn::overload(
          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { return true; },
          [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; })));
      CHECK(std::move(std::as_const(a))
                .invoke(fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                     [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                     [](int const &&) -> bool { return true; })));
      CHECK(std::move(a).invoke(fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                             [](int const &) -> bool { throw 0; }, [](int &&) -> bool { return true; },
                                             [](int const &&) -> bool { throw 0; })));

      WHEN("constexpr")
      {
        constexpr sum<int> a{std::in_place_type<int>, 42};
        static_assert(a.invoke(fn::overload(
            [](auto) -> std::false_type { return {}; }, //
            [](int &) -> std::false_type { return {}; }, [](int const &) -> std::true_type { return {}; },
            [](int &&) -> std::false_type { return {}; }, [](int const &&) -> std::false_type { return {}; })));
        static_assert(std::move(a).invoke(fn::overload(
            [](auto) -> std::false_type { return {}; }, //
            [](int &) -> std::false_type { return {}; }, [](int const &) -> std::false_type { return {}; },
            [](int &&) -> std::false_type { return {}; }, [](int const &&) -> std::true_type { return {}; })));
      }
    }

    WHEN("tag and value")
    {
      static_assert(
          std::is_same_v<bool,
                         decltype(a.invoke(fn::overload([](some_in_place_type auto, auto...) -> bool { throw 1; },
                                                        [](some_in_place_type auto, int) -> bool { return true; })))>);

      CHECK(a.invoke(fn::overload([](auto, auto) -> bool { throw 1; },
                                  [](std::in_place_type_t<int>, int &) -> bool { return true; },
                                  [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                                  [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                  [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
      CHECK(std::as_const(a).invoke(fn::overload([](auto, auto) -> bool { throw 1; },
                                                 [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                                                 [](std::in_place_type_t<int>, int const &) -> bool { return true; },
                                                 [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                                 [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
      CHECK(std::move(std::as_const(a))
                .invoke(fn::overload([](auto, auto) -> bool { throw 1; },
                                     [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                                     [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                                     [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                     [](std::in_place_type_t<int>, int const &&) -> bool { return true; })));
      CHECK(std::move(a).invoke(fn::overload([](auto, auto) -> bool { throw 1; },
                                             [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                                             [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                                             [](std::in_place_type_t<int>, int &&) -> bool { return true; },
                                             [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));

      WHEN("constexpr")
      {
        constexpr sum<int> a{std::in_place_type<int>, 42};
        static_assert(
            a.invoke(fn::overload([](auto, auto) -> std::false_type { return {}; }, //
                                  [](std::in_place_type_t<int>, int &) -> std::false_type { return {}; },
                                  [](std::in_place_type_t<int>, int const &) -> std::true_type { return {}; },
                                  [](std::in_place_type_t<int>, int &&) -> std::false_type { return {}; },
                                  [](std::in_place_type_t<int>, int const &&) -> std::false_type { return {}; })));
        static_assert(std::move(a).invoke(
            fn::overload([](auto, auto) -> std::false_type { return {}; }, //
                         [](std::in_place_type_t<int>, int &) -> std::false_type { return {}; },
                         [](std::in_place_type_t<int>, int const &) -> std::false_type { return {}; },
                         [](std::in_place_type_t<int>, int &&) -> std::false_type { return {}; },
                         [](std::in_place_type_t<int>, int const &&) -> std::true_type { return {}; })));
      }
    }
  }

  WHEN("invoke_r")
  {
    sum<int> a{std::in_place_type<int>, 42};
    WHEN("value only")
    {
      static_assert(std::is_same_v<bool, decltype(a.template invoke_r<bool>(fn::overload(
                                             [](auto) -> bool { throw 1; }, [](int) -> bool { return true; })))>);
      static_assert(
          std::is_same_v<int, decltype(a.template invoke_r<int>(fn::overload([](auto) -> bool { throw 1; }, //
                                                                             [](int) -> int { return true; })))>);

      CHECK(a.template invoke_r<bool>(fn::overload(
          [](auto) -> bool { throw 1; }, [](int &) -> bool { return true; }, [](int const &) -> bool { throw 0; },
          [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; })));
      CHECK(std::as_const(a).template invoke_r<bool>(fn::overload(
          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { return true; },
          [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; })));
      CHECK(std::move(std::as_const(a))
                .template invoke_r<bool>(fn::overload(
                    [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                    [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { return true; })));
      CHECK(std::move(a).template invoke_r<bool>(fn::overload(
          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
          [](int &&) -> bool { return true; }, [](int const &&) -> bool { throw 0; })));

      WHEN("constexpr")
      {
        constexpr sum<int> a{std::in_place_type<int>, 42};
        static_assert(a.template invoke_r<bool>(fn::overload(
            [](auto) -> std::false_type { return {}; }, //
            [](int &) -> std::false_type { return {}; }, [](int const &) -> std::true_type { return {}; },
            [](int &&) -> std::false_type { return {}; }, [](int const &&) -> std::false_type { return {}; })));
        static_assert(std::move(a).template invoke_r<bool>(fn::overload(
            [](auto) -> std::false_type { return {}; }, //
            [](int &) -> std::false_type { return {}; }, [](int const &) -> std::false_type { return {}; },
            [](int &&) -> std::false_type { return {}; }, [](int const &&) -> std::true_type { return {}; })));
      }
    }

    WHEN("tag and value")
    {
      static_assert(std::is_same_v<bool, decltype(a.template invoke_r<bool>(fn::overload(
                                             [](some_in_place_type auto, auto...) -> bool { throw 1; },
                                             [](some_in_place_type auto, int) -> bool { return true; })))>);
      static_assert(std::is_same_v<int, decltype(a.template invoke_r<int>(
                                            fn::overload([](some_in_place_type auto, auto...) -> bool { throw 1; },
                                                         [](some_in_place_type auto, int) -> int { return true; })))>);

      CHECK(a.template invoke_r<bool>(fn::overload([](auto, auto) -> bool { throw 1; },
                                                   [](std::in_place_type_t<int>, int &) -> bool { return true; },
                                                   [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                                                   [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                                   [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
      CHECK(std::as_const(a).template invoke_r<bool>(
          fn::overload([](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                       [](std::in_place_type_t<int>, int const &) -> bool { return true; },
                       [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                       [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
      CHECK(std::move(std::as_const(a))
                .template invoke_r<bool>(fn::overload(
                    [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                    [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                    [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                    [](std::in_place_type_t<int>, int const &&) -> bool { return true; })));
      CHECK(std::move(a).template invoke_r<bool>(
          fn::overload([](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                       [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                       [](std::in_place_type_t<int>, int &&) -> bool { return true; },
                       [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));

      WHEN("constexpr")
      {
        constexpr sum<int> a{std::in_place_type<int>, 42};
        static_assert(a.template invoke_r<bool>(
            fn::overload([](auto, auto) -> std::false_type { return {}; }, //
                         [](std::in_place_type_t<int>, int &) -> std::false_type { return {}; },
                         [](std::in_place_type_t<int>, int const &) -> std::true_type { return {}; },
                         [](std::in_place_type_t<int>, int &&) -> std::false_type { return {}; },
                         [](std::in_place_type_t<int>, int const &&) -> std::false_type { return {}; })));
        static_assert(std::move(a).template invoke_r<bool>(
            fn::overload([](auto, auto) -> std::false_type { return {}; }, //
                         [](std::in_place_type_t<int>, int &) -> std::false_type { return {}; },
                         [](std::in_place_type_t<int>, int const &) -> std::false_type { return {}; },
                         [](std::in_place_type_t<int>, int &&) -> std::false_type { return {}; },
                         [](std::in_place_type_t<int>, int const &&) -> std::true_type { return {}; })));
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
}

namespace {
struct PassThrough {
  auto operator()(std::equality_comparable auto &&v) const -> std::remove_cvref_t<decltype(v)>;
  auto operator()(fn::some_in_place_type auto, std::equality_comparable auto &&v) const
      -> std::remove_cvref_t<decltype(v)>;
};
} // namespace

TEST_CASE("sum type collapsing", "[sum][transform][normalized]")
{
  using ::fn::overload;
  using ::fn::some_in_place_type;
  using ::fn::sum;
  using ::fn::detail::_collapsing_sum_tag;
  using ::fn::detail::_sum_invoke_result;
  using ::fn::detail::_sum_invoke_type_result;
  using ::fn::detail::_typelist_collapsing_sum;
  using ::fn::detail::_typelist_type_collapsing_sum;

  struct sum_double_int {};
  struct sum_bool {};
  struct sum_bool_int {};

  WHEN("one element")
  {
    constexpr auto fn = PassThrough{};
    using type = sum<double>;
    static_assert(
        std::same_as<typename _sum_invoke_result<_collapsing_sum_tag, decltype(fn), type &>::type, sum<double>>);
    static_assert(
        std::same_as<typename _sum_invoke_type_result<_collapsing_sum_tag, decltype(fn), type &>::type, sum<double>>);
  }

  WHEN("two elements")
  {
    constexpr auto fn = PassThrough{};
    using type = sum<double, int>;
    static_assert(
        std::same_as<typename _sum_invoke_result<_collapsing_sum_tag, decltype(fn), type &>::type, sum<double, int>>);
    static_assert(std::same_as<typename _sum_invoke_type_result<_collapsing_sum_tag, decltype(fn), type &>::type,
                               sum<double, int>>);
  }

  WHEN("one sum, one element only")
  {
    constexpr auto fn = overload{[](sum_bool const &) -> sum<bool> && { throw 0; },
                                 [](some_in_place_type auto, sum_bool const &) -> sum<bool> && { throw 0; }};
    using type = sum<sum_bool>;
    static_assert(
        std::same_as<typename _sum_invoke_result<_collapsing_sum_tag, decltype(fn), type &>::type, sum<bool>>);
    static_assert(
        std::same_as<typename _sum_invoke_type_result<_collapsing_sum_tag, decltype(fn), type &>::type, sum<bool>>);
  }

  WHEN("element and one sum with one element")
  {
    constexpr auto fn = overload{PassThrough{}, //
                                 [](sum_bool const &) -> sum<bool> && { throw 0; },
                                 [](some_in_place_type auto, sum_bool const &) -> sum<bool> && { throw 0; }};
    using type = sum<double, sum_bool>;
    static_assert(
        std::same_as<typename _sum_invoke_result<_collapsing_sum_tag, decltype(fn), type &>::type, sum<bool, double>>);
    static_assert(std::same_as<typename _sum_invoke_type_result<_collapsing_sum_tag, decltype(fn), type &>::type,
                               sum<bool, double>>);
  }

  WHEN("one sum with two elements")
  {
    constexpr auto fn = overload{[](sum_bool_int const &) -> sum<bool, int> && { throw 0; },
                                 [](some_in_place_type auto, sum_bool_int const &) -> sum<bool, int> && { throw 0; }};
    using type = sum<sum_bool_int>;
    static_assert(
        std::same_as<typename _sum_invoke_result<_collapsing_sum_tag, decltype(fn), type &>::type, sum<bool, int>>);
    static_assert(std::same_as<typename _sum_invoke_type_result<_collapsing_sum_tag, decltype(fn), type &>::type,
                               sum<bool, int>>);
  }

  WHEN("sum with two elements and sum with one element")
  {
    constexpr auto fn = overload{[](sum_bool_int const &) -> sum<bool, int> && { throw 0; },
                                 [](some_in_place_type auto, sum_bool_int const &) -> sum<bool, int> && { throw 0; },
                                 [](sum_bool const &) -> sum<bool> && { throw 0; },
                                 [](some_in_place_type auto, sum_bool const &) -> sum<bool> && { throw 0; }};
    using type = sum<sum_bool_int, sum_bool>;
    static_assert(
        std::same_as<typename _sum_invoke_result<_collapsing_sum_tag, decltype(fn), type &>::type, sum<bool, int>>);
    static_assert(std::same_as<typename _sum_invoke_type_result<_collapsing_sum_tag, decltype(fn), type &>::type,
                               sum<bool, int>>);
  }

  WHEN("two sums with two elements and two elements")
  {
    constexpr auto fn
        = overload{PassThrough{}, [](sum_double_int const &) -> sum<double, int> { throw 0; },
                   [](some_in_place_type auto, sum_double_int const &) -> sum<double, int> { throw 0; },
                   [](sum_bool_int const &) -> sum<bool, int> const { throw 0; },
                   [](some_in_place_type auto, sum_bool_int const &) -> sum<bool, int> const { throw 0; }};
    using type = sum<sum_bool_int, sum_double_int, double, int>;
    static_assert(std::same_as<typename _sum_invoke_result<_collapsing_sum_tag, decltype(fn), type &>::type,
                               sum<bool, double, int>>);
    static_assert(std::same_as<typename _sum_invoke_type_result<_collapsing_sum_tag, decltype(fn), type &>::type,
                               sum<bool, double, int>>);
  }
}

TEST_CASE("sum transform", "[sum][transform]")
{
  static constexpr auto sizeof_string = sizeof(std::string);
  using ::fn::sum;
  constexpr auto fn1 = [](auto i) noexcept -> std::size_t { return sizeof(i); };
  constexpr auto fn2 = [](auto, auto i) noexcept -> std::size_t { return sizeof(i); };

  WHEN("size 4")
  {
    using type = sum<double, int, std::string, std::string_view>;
    static_assert(type::size == 4);

    WHEN("element v0 set")
    {
      type a{std::in_place_type<double>, 0.5};
      CHECK(a.data.v0 == 0.5);
      WHEN("value only")
      {
        static_assert(type{0.5}.transform(fn1) == sum{8ul});
        CHECK(a.transform(      //
                  fn::overload( //
                      [](auto) -> int { throw 1; }, [](double &i) -> bool { return i == 0.5; },
                      [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                      [](double const &&) -> bool { throw 0; }))
              == sum<bool, int>{true});
        CHECK(std::as_const(a).transform( //
                  fn::overload(           //
                      [](auto) -> int { throw 1; }, [](double &) -> bool { throw 0; },
                      [](double const &i) -> bool { return i == 0.5; }, [](double &&) -> bool { throw 0; },
                      [](double const &&) -> bool { throw 0; }))
              == sum<bool, int>{true});
        CHECK(std::move(std::as_const(a))
                  .transform(       //
                      fn::overload( //
                          [](auto) -> int { throw 1; }, [](double &) -> bool { throw 0; },
                          [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                          [](double const &&i) -> bool { return i == 0.5; }))
              == sum<bool, int>{true});
        CHECK(std::move(a).transform( //
                  fn::overload(       //
                      [](auto) -> int { throw 1; }, [](double &) -> bool { throw 0; },
                      [](double const &) -> bool { throw 0; }, [](double &&i) -> bool { return i == 0.5; },
                      [](double const &&) -> bool { throw 0; }))
              == sum<bool, int>{true});
      }
      WHEN("tag and value")
      {
        // TODO Change single CHECK below to static_assert when supported by GCC
        CHECK(type{0.5}.transform(fn2) == sum{8ul});
        CHECK(a.transform(      //
                  fn::overload( //
                      [](auto, auto) -> int { throw 1; },
                      [](std::in_place_type_t<double>, double &i) -> bool { return i == 0.5; },
                      [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                      [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                      [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; }))
              == sum<bool, int>{true});
        CHECK(
            std::as_const(a).transform( //
                fn::overload(           //
                    [](auto, auto) -> int { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                    [](std::in_place_type_t<double>, double const &i) -> bool { return i == 0.5; },
                    [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                    [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; }))
            == sum<bool, int>{true});
        CHECK(std::move(std::as_const(a))
                  .transform(       //
                      fn::overload( //
                          [](auto, auto) -> int { throw 1; },
                          [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double const &&i) -> bool { return i == 0.5; }))
              == sum<bool, int>{true});
        CHECK(std::move(a).transform(fn::overload( //
                  [](auto, auto) -> int { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                  [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                  [](std::in_place_type_t<double>, double &&i) -> bool { return i == 0.5; },
                  [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; }))
              == sum<bool, int>{true});
      }
    }

    WHEN("element v1 set")
    {
      type a{std::in_place_type<int>, 42};
      CHECK(a.data.v1 == 42);

      WHEN("value only")
      {
        static_assert(type{42}.transform(fn1) == sum{4ul});
        CHECK(a.transform(      //
                  fn::overload( //
                      [](auto) -> bool { throw 1; }, [](int &i) -> bool { return i == 42; },
                      [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                      [](int const &&) -> bool { throw 0; }))
              == sum{true});
        CHECK(std::as_const(a).transform( //
                  fn::overload(           //
                      [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                      [](int const &i) -> bool { return i == 42; }, [](int &&) -> bool { throw 0; },
                      [](int const &&) -> bool { throw 0; }))
              == sum{true});
        CHECK(
            sum<int>{std::in_place_type<int>, 42}.transform( //
                fn::overload(                                //
                    [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                    [](int &&i) -> bool { return i == 42; }, [](int const &&) -> bool { throw 0; }))
            == sum{true});
        CHECK(std::move(std::as_const(a))
                  .transform(       //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                          [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                          [](int const &&i) -> bool { return i == 42; }))
              == sum{true});
      }

      WHEN("tag and value")
      {
        static_assert(type{42}.transform(fn2) == sum{4ul});
        CHECK(a.transform(      //
                  fn::overload( //
                      [](auto, auto) -> bool { throw 1; },
                      [](std::in_place_type_t<int>, int &i) -> bool { return i == 42; },
                      [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                      [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                      [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; }))
              == sum{true});
        CHECK(std::as_const(a).transform( //
                  fn::overload(           //
                      [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                      [](std::in_place_type_t<int>, int const &i) -> bool { return i == 42; },
                      [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                      [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; }))
              == sum{true});
        CHECK(
            std::move(std::as_const(a))
                .transform(       //
                    fn::overload( //
                        [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &&i) -> bool { return i == 42; }))
            == sum{true});
        CHECK(std::move(a).transform( //
                  fn::overload(       //
                      [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                      [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                      [](std::in_place_type_t<int>, int &&i) -> bool { return i == 42; },
                      [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; }))
              == sum{true});
      }
    }

    WHEN("element v2 set")
    {
      type a{std::in_place_type<std::string>, "bar"};
      CHECK(a.data.v2 == "bar");
      WHEN("value only")
      {
        // TODO Change single CHECK below to static_assert when supported by GCC
        CHECK(type{std::in_place_type<std::string>, "bar"}.transform(fn1) == sum{sizeof_string});
        CHECK(a.transform(      //
                  fn::overload( //
                      [](auto) -> sum<bool, std::string> { throw 1; },
                      [](std::string &i) -> bool { return i == "bar"; }, [](std::string const &) -> bool { throw 0; },
                      [](std::string &&) -> bool { throw 0; }, [](std::string const &&) -> bool { throw 0; }))
              == sum<bool, std::string>{true});
        CHECK(std::as_const(a).transform( //
                  fn::overload(           //
                      [](auto) -> sum<bool, std::string> { throw 1; }, [](std::string &) -> bool { throw 0; },
                      [](std::string const &i) -> bool { return i == "bar"; }, [](std::string &&) -> bool { throw 0; },
                      [](std::string const &&) -> bool { throw 0; }))
              == sum<bool, std::string>{true});
        CHECK(std::move(std::as_const(a))
                  .transform(       //
                      fn::overload( //
                          [](auto) -> sum<bool, std::string> { throw 1; }, [](std::string &) -> bool { throw 0; },
                          [](std::string const &) -> bool { throw 0; }, [](std::string &&) -> bool { throw 0; },
                          [](std::string const &&i) -> bool { return i == "bar"; }))
              == sum<bool, std::string>{true});
        CHECK(std::move(a).transform( //
                  fn::overload(       //
                      [](auto) -> sum<bool, std::string> { throw 1; }, [](std::string &) -> bool { throw 0; },
                      [](std::string const &) -> bool { throw 0; }, [](std::string &&i) -> bool { return i == "bar"; },
                      [](std::string const &&) -> bool { throw 0; }))
              == sum<bool, std::string>{true});
      }
      WHEN("tag and value")
      {
        // TODO Change single CHECK below to static_assert when supported by GCC
        CHECK(type{std::in_place_type<std::string>, "bar"}.transform(fn2) == sum{sizeof_string});
        CHECK(a.transform(      //
                  fn::overload( //
                      [](auto, auto) -> sum<bool, std::string> { throw 1; },
                      [](std::in_place_type_t<std::string>, std::string &i) -> bool { return i == "bar"; },
                      [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                      [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                      [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; }))
              == sum<bool, std::string>{true});
        CHECK(std::as_const(a).transform( //
                  fn::overload(           //
                      [](auto, auto) -> sum<bool, std::string> { throw 1; },
                      [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                      [](std::in_place_type_t<std::string>, std::string const &i) -> bool { return i == "bar"; },
                      [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                      [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; }))
              == sum<bool, std::string>{true});
        CHECK(std::move(std::as_const(a))
                  .transform(       //
                      fn::overload( //
                          [](auto, auto) -> sum<bool, std::string> { throw 1; },
                          [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                          [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                          [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                          [](std::in_place_type_t<std::string>, std::string const &&i) -> bool { return i == "bar"; }))
              == sum<bool, std::string>{true});
        CHECK(std::move(a).transform( //
                  fn::overload(       //
                      [](auto, auto) -> sum<bool, std::string> { throw 1; },
                      [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                      [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                      [](std::in_place_type_t<std::string>, std::string &&i) -> bool { return i == "bar"; },
                      [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; }))
              == sum<bool, std::string>{true});
      }
    }

    WHEN("element v3 set")
    {
      type a{std::in_place_type<std::string_view>, "baz"};
      CHECK(a.data.v3 == "baz");
      WHEN("value only")
      {
        static_assert(type{std::in_place_type<std::string_view>, "baz"}.transform(fn1) == sum{16ul});
        CHECK(a.transform(      //
                  fn::overload( //
                      [](auto) -> sum<int, std::string_view> { throw 1; },
                      [](std::string_view &i) -> sum<bool, int> { return {i == "baz"}; },
                      [](std::string_view const &) -> sum<bool, int> { throw 0; },
                      [](std::string_view &&) -> sum<bool, int> { throw 0; },
                      [](std::string_view const &&) -> sum<bool, int> { throw 0; }))
              == sum<bool, int, std::string_view>{true});
        CHECK(std::as_const(a).transform( //
                  fn::overload(           //
                      [](auto) -> sum<int, std::string_view> { throw 1; },
                      [](std::string_view &) -> sum<bool, int> { throw 0; },
                      [](std::string_view const &i) -> sum<bool, int> { return {i == "baz"}; },
                      [](std::string_view &&) -> sum<bool, int> { throw 0; },
                      [](std::string_view const &&) -> sum<bool, int> { throw 0; }))
              == sum<bool, int, std::string_view>{true});
        CHECK(std::move(std::as_const(a))
                  .transform(       //
                      fn::overload( //
                          [](auto) -> sum<int, std::string_view> { throw 1; },
                          [](std::string_view &) -> sum<bool, int> { throw 0; },
                          [](std::string_view const &) -> sum<bool, int> { throw 0; },
                          [](std::string_view &&) -> sum<bool, int> { throw 0; },
                          [](std::string_view const &&i) -> sum<bool, int> { return {i == "baz"}; }))
              == sum<bool, int, std::string_view>{true});
        CHECK(std::move(a).transform( //
                  fn::overload(       //
                      [](auto) -> sum<int, std::string_view> { throw 1; },
                      [](std::string_view &) -> sum<bool, int> { throw 0; },
                      [](std::string_view const &) -> sum<bool, int> { throw 0; },
                      [](std::string_view &&i) -> sum<bool, int> { return {i == "baz"}; },
                      [](std::string_view const &&) -> sum<bool, int> { throw 0; }))
              == sum<bool, int, std::string_view>{true});
      }
      WHEN("tag and value")
      {
        static_assert(type{std::in_place_type<std::string_view>, "baz"}.transform(fn2) == sum{16ul});
        CHECK(
            a.transform(      //
                fn::overload( //
                    [](auto, auto) -> sum<int, std::string_view> { throw 1; },
                    [](std::in_place_type_t<std::string_view>, std::string_view &i) -> sum<bool, int> {
                      return {i == "baz"};
                    },
                    [](std::in_place_type_t<std::string_view>, std::string_view const &) -> sum<bool, int> { throw 0; },
                    [](std::in_place_type_t<std::string_view>, std::string_view &&) -> sum<bool, int> { throw 0; },
                    [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> sum<bool, int> {
                      throw 0;
                    }))
            == sum<bool, int, std::string_view>{true});
        CHECK(std::as_const(a).transform( //
                  fn::overload(
                      [](auto, auto) -> sum<int, std::string_view> { throw 1; },
                      [](std::in_place_type_t<std::string_view>, std::string_view &) -> sum<bool, int> { throw 0; },
                      [](std::in_place_type_t<std::string_view>, std::string_view const &i) -> sum<bool, int> {
                        return {i == "baz"};
                      },
                      [](std::in_place_type_t<std::string_view>, std::string_view &&) -> sum<bool, int> { throw 0; },
                      [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> sum<bool, int> {
                        throw 0;
                      }))
              == sum<bool, int, std::string_view>{true});
        CHECK(
            std::move(std::as_const(a))
                .transform( //
                    fn::overload(
                        [](auto, auto) -> sum<int, std::string_view> { throw 1; },
                        [](std::in_place_type_t<std::string_view>, std::string_view &) -> sum<bool, int> { throw 0; },
                        [](std::in_place_type_t<std::string_view>, std::string_view const &) -> sum<bool, int> {
                          throw 0;
                        },
                        [](std::in_place_type_t<std::string_view>, std::string_view &&) -> sum<bool, int> { throw 0; },
                        [](std::in_place_type_t<std::string_view>, std::string_view const &&i) -> sum<bool, int> {
                          return {i == "baz"};
                        }))
            == sum<bool, int, std::string_view>{true});
        CHECK(
            std::move(a).transform( //
                fn::overload(
                    [](auto, auto) -> sum<int, std::string_view> { throw 1; },
                    [](std::in_place_type_t<std::string_view>, std::string_view &) -> sum<bool, int> { throw 0; },
                    [](std::in_place_type_t<std::string_view>, std::string_view const &) -> sum<bool, int> { throw 0; },
                    [](std::in_place_type_t<std::string_view>, std::string_view &&i) -> sum<bool, int> {
                      return {i == "baz"};
                    },
                    [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> sum<bool, int> {
                      throw 0;
                    }))
            == sum<bool, int, std::string_view>{true});
      }
    }
  }
}
