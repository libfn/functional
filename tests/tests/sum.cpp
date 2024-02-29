// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/sum.hpp"
#include "functional/utility.hpp"

#include <catch2/catch_all.hpp>

#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
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
    static_assert(fn::detail::typelist_invocable<decltype([](auto) {}), type &>);
    static_assert(fn::detail::typelist_invocable<decltype([](auto &) {}), type &>);
    static_assert(fn::detail::typelist_invocable<decltype([](auto const &) {}), type &>);
    static_assert(fn::detail::typelist_invocable<decltype(fn::overload{[](int &) {}, [](TestType &) {}}), type &>);
    static_assert(fn::detail::typelist_invocable<decltype(fn::overload{[](int) {}, [](TestType) {}}), type const &>);

    static_assert(not fn::detail::typelist_invocable<decltype([](TestType &) {}), type &>); // missing int
    static_assert(not fn::detail::typelist_invocable<decltype([](int &) {}), type &>);      // missing TestType
    static_assert(not fn::detail::typelist_invocable<decltype(fn::overload{[](int &&) {}, [](TestType &&) {}}),
                                                     type &>); // cannot bind lvalue to rvalue-reference
    static_assert(
        not fn::detail::typelist_invocable<decltype([](auto &) {}), type &&>); // cannot bind rvalue to lvalue-reference
    static_assert(not fn::detail::typelist_invocable<decltype([](auto, auto &) {}), type &>); // bad arity
    static_assert(not fn::detail::typelist_invocable<decltype(fn::overload{[](int &) {}, [](TestType &) {}}),
                                                     type const &>); // cannot bind const to non-const reference

    static_assert(fn::detail::typelist_invocable<decltype([](auto &) {}), sum<NonCopyable> &>);
    static_assert(
        not fn::detail::typelist_invocable<decltype([](auto) {}), NonCopyable &>); // copy-constructor not available
  }

  WHEN("type_invocable")
  {
    using type = sum<TestType, int>;
    static_assert(fn::detail::typelist_type_invocable<decltype([](auto, auto) {}), type &>);
    static_assert(fn::detail::typelist_type_invocable<decltype([](some_in_place_type auto, auto) {}), type &>);
    static_assert(fn::detail::typelist_type_invocable<decltype([](some_in_place_type auto, auto &) {}), type &>);
    static_assert(
        fn::detail::typelist_type_invocable<decltype(fn::overload{[](some_in_place_type auto, int &) {},
                                                                  [](some_in_place_type auto, TestType &) {}}),
                                            type &>);
    static_assert(fn::detail::typelist_type_invocable<decltype(fn::overload{[](some_in_place_type auto, int) {},
                                                                            [](some_in_place_type auto, TestType) {}}),
                                                      type const &>);
    static_assert(not fn::detail::typelist_type_invocable<decltype([](some_in_place_type auto, TestType &) {}),
                                                          type &>); // missing int
    static_assert(not fn::detail::typelist_type_invocable<decltype([](some_in_place_type auto, int &) {}),
                                                          type &>); // missing TestType
    static_assert(
        not fn::detail::typelist_type_invocable<decltype(fn::overload{[](some_in_place_type auto, int &&) {},
                                                                      [](some_in_place_type auto, TestType &&) {}}),
                                                type &>); // cannot bind lvalue to rvalue-reference
    static_assert(not fn::detail::typelist_type_invocable<decltype([](some_in_place_type auto, auto &) {}),
                                                          type &&>); // cannot bind rvalue to lvalue-reference
    static_assert(not fn::detail::typelist_type_invocable<decltype([](auto) {}), type &>); // bad arity
    static_assert(
        not fn::detail::typelist_type_invocable<decltype(fn::overload{[](some_in_place_type auto, int &) {},
                                                                      [](some_in_place_type auto, TestType &) {}}),
                                                type const &>); // cannot bind const to non-const reference

    static_assert(
        fn::detail::typelist_type_invocable<decltype([](some_in_place_type auto, auto &) {}), sum<NonCopyable> &>);
    static_assert(not fn::detail::typelist_type_invocable<decltype([](some_in_place_type auto, auto) {}),
                                                          NonCopyable &>); // copy-constructor not available

    static_assert(
        fn::detail::typelist_type_invocable<decltype([](some_in_place_type auto, auto &) {}), sum<NonCopyable> &>);
    static_assert(not fn::detail::typelist_type_invocable<decltype([](some_in_place_type auto, auto) {}),
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
      static_assert(
          c.invoke_to([](auto &&a) -> bool { return a.size() == 3 && a[0] == 3 && a[1] == 14 && a[2] == 15; }));
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
    CHECK(a.invoke_to([](auto &i) -> bool { return i.v == 42; }));

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
      CHECK(a.invoke_to([](auto &i) -> bool {
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
      static_assert(a.invoke_to([](auto &i) -> bool {
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
    CHECK(a == type{42});
    CHECK(a != type{41});
    CHECK(a != type{false});

    CHECK(a == type{std::in_place_type<int>, 42});
    CHECK(a != type{std::in_place_type<int>, 41});
    CHECK(a != type{std::in_place_type<bool>, true});
    CHECK(not(a == type{std::in_place_type<int>, 41}));
    CHECK(not(a == type{std::in_place_type<bool>, true}));

    WHEN("constexpr")
    {
      constexpr type a{std::in_place_type<int>, 42};
      static_assert(a == 42);
      static_assert(a != 41);
      static_assert(a != false);
      static_assert(a != true);
      static_assert(not(a == 41));
      static_assert(not(a == false));
      static_assert(not(a == true));
      static_assert(a == type{std::in_place_type<int>, 42});
      static_assert(a != type{std::in_place_type<int>, 41});
      static_assert(a != type{std::in_place_type<bool>, true});
      static_assert(not(a == type{std::in_place_type<int>, 41}));
      static_assert(not(a == type{std::in_place_type<bool>, true}));

      static_assert([](auto const &a) constexpr -> bool { //
        return not requires { a == 0.5; };
      }(a));                                              // double is not a type member
      static_assert([](auto const &a) constexpr -> bool { //
        return not requires { a != 0.5; };
      }(a));                                              // double is not a type member
      static_assert([](auto const &a) constexpr -> bool { //
        return not requires { a == sum(std::in_place_type<int>, 1); };
      }(a));                                              // type mismatch sum<int>
      static_assert([](auto const &a) constexpr -> bool { //
        return not requires { a != sum(std::in_place_type<int>, 1); };
      }(a)); // type mismatch sum<int>
    }
  }

  WHEN("invoke_to")
  {
    sum<int> a{std::in_place_type<int>, 42};
    WHEN("value only")
    {
      CHECK(a.invoke_to(fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { return true; },
                                     [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                     [](int const &&) -> bool { throw 0; })));
      CHECK(std::as_const(a).invoke_to(fn::overload(
          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { return true; },
          [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; })));
      CHECK(sum<int>{std::in_place_type<int>, 42}.invoke_to(fn::overload(
          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
          [](int &&) -> bool { return true; }, [](int const &&) -> bool { throw 0; })));
      CHECK(std::move(std::as_const(a))
                .invoke_to(fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                        [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                        [](int const &&) -> bool { return true; })));

      WHEN("constexpr")
      {
        constexpr sum<int> a{std::in_place_type<int>, 42};
        static_assert(a.invoke_to(fn::overload(
            [](auto) -> std::false_type { return {}; }, //
            [](int &) -> std::false_type { return {}; }, [](int const &) -> std::true_type { return {}; },
            [](int &&) -> std::false_type { return {}; }, [](int const &&) -> std::false_type { return {}; })));
        static_assert(std::move(a).invoke_to(fn::overload(
            [](auto) -> std::false_type { return {}; }, //
            [](int &) -> std::false_type { return {}; }, [](int const &) -> std::false_type { return {}; },
            [](int &&) -> std::false_type { return {}; }, [](int const &&) -> std::true_type { return {}; })));
      }
    }

    WHEN("tag and value")
    {
      CHECK(a.invoke_to(fn::overload([](auto, auto) -> bool { throw 1; },
                                     [](std::in_place_type_t<int>, int &) -> bool { return true; },
                                     [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                                     [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                     [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
      CHECK(std::as_const(a).invoke_to(fn::overload([](auto, auto) -> bool { throw 1; },
                                                    [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                                                    [](std::in_place_type_t<int>, int const &) -> bool { return true; },
                                                    [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                                    [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
      CHECK(sum<int>{std::in_place_type<int>, 42}.invoke_to(
          fn::overload([](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                       [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                       [](std::in_place_type_t<int>, int &&) -> bool { return true; },
                       [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
      CHECK(std::move(std::as_const(a))
                .invoke_to(fn::overload([](auto, auto) -> bool { throw 1; },
                                        [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                                        [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                                        [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                        [](std::in_place_type_t<int>, int const &&) -> bool { return true; })));
      WHEN("constexpr")
      {
        constexpr sum<int> a{std::in_place_type<int>, 42};
        static_assert(
            a.invoke_to(fn::overload([](auto, auto) -> std::false_type { return {}; }, //
                                     [](std::in_place_type_t<int>, int &) -> std::false_type { return {}; },
                                     [](std::in_place_type_t<int>, int const &) -> std::true_type { return {}; },
                                     [](std::in_place_type_t<int>, int &&) -> std::false_type { return {}; },
                                     [](std::in_place_type_t<int>, int const &&) -> std::false_type { return {}; })));
        static_assert(std::move(a).invoke_to(
            fn::overload([](auto, auto) -> std::false_type { return {}; }, //
                         [](std::in_place_type_t<int>, int &) -> std::false_type { return {}; },
                         [](std::in_place_type_t<int>, int const &) -> std::false_type { return {}; },
                         [](std::in_place_type_t<int>, int &&) -> std::false_type { return {}; },
                         [](std::in_place_type_t<int>, int const &&) -> std::true_type { return {}; })));
      }
    }
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

  WHEN("sum_for")
  {
    static_assert(std::same_as<fn::sum_for<int>, fn::sum<int>>);
    static_assert(std::same_as<fn::sum_for<int, bool>, fn::sum<bool, int>>);
    static_assert(std::same_as<fn::sum_for<bool, int>, fn::sum<bool, int>>);
    static_assert(std::same_as<fn::sum_for<int, NonCopyable>, fn::sum<NonCopyable, int>>);
    static_assert(std::same_as<fn::sum_for<NonCopyable, int>, fn::sum<NonCopyable, int>>);
    static_assert(std::same_as<fn::sum_for<int, bool, NonCopyable>, fn::sum<NonCopyable, bool, int>>);
  }

  WHEN("move and copy")
  {
    WHEN("one type only")
    {
      using T = sum<std::string>;
      T a{std::in_place_type<std::string>, "baz"};
      CHECK(a.invoke_to([](auto &&i) { return i; }) == "baz");

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
      CHECK(a.invoke_to([](auto &&i) { return i; }) == "baz");
      CHECK(b.invoke_to([](auto &&i) { return i; }) == "baz");

      T c{std::move(a)};
      CHECK(c.invoke_to([](auto &&i) { return i; }) == "baz");
    }

    WHEN("mixed with other types")
    {
      using T = sum<std::string, std::string_view>;
      T a{std::in_place_type<std::string>, "baz"};
      CHECK(a.invoke_to([](auto &&i) { return std::string(i); }) == "baz");

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
      CHECK(a.invoke_to([](auto &&i) { return std::string(i); }) == "baz");
      CHECK(b.invoke_to([](auto &&i) { return std::string(i); }) == "baz");

      T c{std::move(a)};
      CHECK(c.invoke_to([](auto &&i) { return std::string(i); }) == "baz");
    }
  }

  WHEN("copy only")
  {
    WHEN("one type only")
    {
      using T = sum<CopyOnly>;
      T a{std::in_place_type<CopyOnly>, 12};
      CHECK(a.invoke_to([](auto &&i) { return static_cast<int>(i); }) == 12);

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
      CHECK(a.invoke_to([](auto &&i) { return static_cast<int>(i); }) == 12);
      CHECK(b.invoke_to([](auto &&i) { return static_cast<int>(i); }) == 12);
    }

    WHEN("mixed with other types")
    {
      using T = sum<CopyOnly, double, int>;
      T a{std::in_place_type<CopyOnly>, 12};
      CHECK(a.invoke_to([](auto &&i) { return static_cast<int>(i); }) == 12);

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
      CHECK(a.invoke_to([](auto &&i) { return static_cast<int>(i); }) == 12);
      CHECK(b.invoke_to([](auto &&i) { return static_cast<int>(i); }) == 12);
    }
  }

  WHEN("move only")
  {
    WHEN("one type only")
    {
      using T = sum<MoveOnly>;
      T a{std::in_place_type<MoveOnly>, 12};
      CHECK(a.invoke_to([](auto &&i) { return static_cast<int>(i); }) == 12);

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
      CHECK(a.invoke_to([](auto &&i) { return static_cast<int>(i); }) == -1);
      CHECK(b.invoke_to([](auto &&i) { return static_cast<int>(i); }) == 12);
    }

    WHEN("mixed with other types")
    {
      using T = sum<MoveOnly, double, int>;
      T a{std::in_place_type<MoveOnly>, 12};
      CHECK(a.invoke_to([](auto &&i) { return static_cast<int>(i); }) == 12);

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
      CHECK(a.invoke_to([](auto &&i) { return static_cast<int>(i); }) == -1);
      CHECK(b.invoke_to([](auto &&i) { return static_cast<int>(i); }) == 12);
    }
  }

  WHEN("immovable type")
  {
    WHEN("one type only")
    {
      using T = sum<NonCopyable>;
      T a{std::in_place_type<NonCopyable>, 12};
      CHECK(a.invoke_to([](auto &&i) { return static_cast<int>(i); }) == 12);

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
      using T = sum<NonCopyable, double, int>;
      T a{std::in_place_type<NonCopyable>, 12};
      CHECK(a.invoke_to([](auto &&i) { return static_cast<int>(i); }) == 12);

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
  constexpr auto fn3 = [](int &) {};
  constexpr auto fn4 = [](std::in_place_type_t<int>, int &) {};

  constexpr sum<std::array<int, 3>> a{std::in_place_type<std::array<int, 3>>, 3, 14, 15};
  static_assert(a.index == 0);
  static_assert(decltype(a)::template has_type<std::array<int, 3>>);
  static_assert(not decltype(a)::template has_type<int>);
  static_assert(a.has_value(std::in_place_type<std::array<int, 3>>));
  static_assert(a.template has_value<std::array<int, 3>>());
  static_assert(a.invoke_to([](auto &i) -> bool {
    return std::same_as<std::array<int, 3> const &, decltype(i)> && i.size() == 3 && i[0] == 3 && i[1] == 14
           && i[2] == 15;
  }));

  WHEN("size 1")
  {
    using T = sum<int>;
    T a{std::in_place_type<int>, 42};
    static_assert(T::size == 1);
    static_assert(std::is_same_v<T::template select_nth<0>, int>);
    static_assert(T::has_type<int>);
    static_assert(not T::has_type<bool>);
    CHECK(a.index == 0);
    CHECK(a.template has_value<int>());
    CHECK(a.has_value(std::in_place_type<int>));

    static_assert(fn::detail::typelist_invocable<decltype(fn1), decltype(a)>);
    static_assert(fn::detail::typelist_invocable<decltype(fn2), decltype(a)>);
    static_assert(not fn::detail::typelist_type_invocable<decltype(fn1), decltype(a)>);
    static_assert(fn::detail::typelist_invocable<decltype(fn2), decltype(a)>);
    static_assert(fn::detail::typelist_invocable<decltype(fn3), decltype(a) &>);
    static_assert(fn::detail::typelist_type_invocable<decltype(fn4), decltype(a) &>);
    static_assert(not fn::detail::typelist_invocable<decltype(fn3), decltype(a) const &>);
    static_assert(not fn::detail::typelist_type_invocable<decltype(fn4), decltype(a) const &>);
    static_assert(not fn::detail::typelist_invocable<decltype(fn3), decltype(a)>);
    static_assert(not fn::detail::typelist_type_invocable<decltype(fn4), decltype(a)>);
    static_assert(not fn::detail::typelist_invocable<decltype(fn3), decltype(a) const>);
    static_assert(not fn::detail::typelist_type_invocable<decltype(fn4), decltype(a) const>);
    static_assert(not fn::detail::typelist_invocable<decltype(fn3), decltype(a) &&>);
    static_assert(not fn::detail::typelist_type_invocable<decltype(fn4), decltype(a) &&>);
    static_assert(not fn::detail::typelist_invocable<decltype(fn3), decltype(a) const &&>);
    static_assert(not fn::detail::typelist_type_invocable<decltype(fn4), decltype(a) const &&>);

    static_assert(std::is_same_v<decltype(a.get_ptr(std::in_place_type<int>)), int *>);
    CHECK(a.get_ptr(std::in_place_type<int>) == &a.data.v0);
    static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(std::in_place_type<int>)), int const *>);
    CHECK(std::as_const(a).get_ptr(std::in_place_type<int>) == &a.data.v0);

    constexpr auto a1 = sum<int>{std::in_place_type<int>, 12};
    static_assert(*a1.get_ptr(std::in_place_type<int>) == 12);
  }

  WHEN("size 2")
  {
    using T = sum<double, int>;
    static_assert(T::size == 2);
    static_assert(std::is_same_v<T::template select_nth<0>, double>);
    static_assert(std::is_same_v<T::template select_nth<1>, int>);
    static_assert(T::has_type<int>);
    static_assert(T::has_type<double>);
    static_assert(not T::has_type<bool>);

    WHEN("element v0 set")
    {
      constexpr auto element = std::in_place_type<double>;
      T a{element, 0.5};
      CHECK(a.data.v0 == 0.5);
      CHECK(a.index == 0);
      CHECK(a.template has_value<double>());
      CHECK(a.has_value(std::in_place_type<double>));
      CHECK(not a.template has_value<int>());
      CHECK(not a.has_value(std::in_place_type<int>));

      constexpr auto fn5 = [](auto &) {};
      constexpr auto fn6 = [](some_in_place_type auto, auto...) {};
      static_assert(fn::detail::typelist_invocable<decltype(fn5), decltype(a) &>);
      static_assert(fn::detail::typelist_type_invocable<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), double *>);
      CHECK(a.get_ptr(element) == &a.data.v0);
      CHECK(a.get_ptr(std::in_place_type<int>) == nullptr);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), double const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v0);
      CHECK(std::as_const(a).get_ptr(std::in_place_type<int>) == nullptr);
    }

    WHEN("element v1 set")
    {
      constexpr auto element = std::in_place_type<int>;
      T a{element, 42};
      CHECK(a.data.v1 == 42);
      CHECK(a.index == 1);
      CHECK(not a.template has_value<double>());
      CHECK(not a.has_value(std::in_place_type<double>));
      CHECK(a.template has_value<int>());
      CHECK(a.has_value(std::in_place_type<int>));

      static_assert(fn::detail::typelist_invocable<decltype(fn1), decltype(a)>);
      static_assert(fn::detail::typelist_invocable<decltype(fn2), decltype(a)>);
      static_assert(not fn::detail::typelist_type_invocable<decltype(fn1), decltype(a)>);
      static_assert(fn::detail::typelist_invocable<decltype(fn2), decltype(a)>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), int *>);
      CHECK(a.get_ptr(element) == &a.data.v1);
      CHECK(a.get_ptr(std::in_place_type<double>) == nullptr);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), int const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v1);
      CHECK(std::as_const(a).get_ptr(std::in_place_type<double>) == nullptr);
    }
  }
  WHEN("size 3")
  {
    using T = sum<double, int, std::string_view>;
    static_assert(T::size == 3);
    static_assert(std::is_same_v<T::template select_nth<0>, double>);
    static_assert(std::is_same_v<T::template select_nth<1>, int>);
    static_assert(std::is_same_v<T::template select_nth<2>, std::string_view>);
    static_assert(T::has_type<int>);
    static_assert(T::has_type<double>);
    static_assert(T::has_type<std::string_view>);
    static_assert(not T::has_type<bool>);

    WHEN("element v0 set")
    {
      constexpr auto element = std::in_place_type<double>;
      T a{element, 0.5};
      CHECK(a.data.v0 == 0.5);
      CHECK(a.index == 0);
      CHECK(a.template has_value<double>());
      CHECK(a.has_value(std::in_place_type<double>));
      CHECK(not a.template has_value<int>());
      CHECK(not a.has_value(std::in_place_type<int>));
      CHECK(not a.template has_value<std::string_view>());
      CHECK(not a.has_value(std::in_place_type<std::string_view>));

      constexpr auto fn5 = [](auto &) {};
      constexpr auto fn6 = [](some_in_place_type auto, auto...) {};
      static_assert(fn::detail::typelist_invocable<decltype(fn5), decltype(a) &>);
      static_assert(fn::detail::typelist_type_invocable<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), double *>);
      CHECK(a.get_ptr(element) == &a.data.v0);
      CHECK(a.get_ptr(std::in_place_type<int>) == nullptr);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), double const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v0);
      CHECK(std::as_const(a).get_ptr(std::in_place_type<int>) == nullptr);
    }

    WHEN("element v1 set")
    {
      constexpr auto element = std::in_place_type<int>;
      T a{element, 42};
      CHECK(a.data.v1 == 42);
      CHECK(a.index == 1);
      CHECK(not a.template has_value<double>());
      CHECK(not a.has_value(std::in_place_type<double>));
      CHECK(a.template has_value<int>());
      CHECK(a.has_value(std::in_place_type<int>));
      CHECK(not a.template has_value<std::string_view>());
      CHECK(not a.has_value(std::in_place_type<std::string_view>));

      static_assert(fn::detail::typelist_invocable<decltype(fn1), decltype(a)>);
      static_assert(fn::detail::typelist_invocable<decltype(fn2), decltype(a)>);
      static_assert(not fn::detail::typelist_type_invocable<decltype(fn1), decltype(a)>);
      static_assert(fn::detail::typelist_invocable<decltype(fn2), decltype(a)>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), int *>);
      CHECK(a.get_ptr(element) == &a.data.v1);
      CHECK(a.get_ptr(std::in_place_type<double>) == nullptr);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), int const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v1);
      CHECK(std::as_const(a).get_ptr(std::in_place_type<double>) == nullptr);
    }

    WHEN("element v2 set")
    {
      constexpr auto element = std::in_place_type<std::string_view>;
      T a{element, "baz"};
      CHECK(a.data.v2 == "baz");
      CHECK(a.index == 2);
      CHECK(not a.template has_value<double>());
      CHECK(not a.has_value(std::in_place_type<double>));
      CHECK(not a.template has_value<int>());
      CHECK(not a.has_value(std::in_place_type<int>));
      CHECK(a.template has_value<std::string_view>());
      CHECK(a.has_value(std::in_place_type<std::string_view>));

      constexpr auto fn5 = [](auto &) {};
      constexpr auto fn6 = [](some_in_place_type auto, auto...) {};
      static_assert(fn::detail::typelist_invocable<decltype(fn5), decltype(a) &>);
      static_assert(fn::detail::typelist_type_invocable<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), std::string_view *>);
      CHECK(a.get_ptr(element) == &a.data.v2);
      CHECK(a.get_ptr(std::in_place_type<double>) == nullptr);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), std::string_view const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v2);
      CHECK(std::as_const(a).get_ptr(std::in_place_type<double>) == nullptr);
    }
  }

  WHEN("size 4")
  {
    using T = sum<double, int, std::string, std::string_view>;
    static_assert(T::size == 4);
    static_assert(std::is_same_v<T::template select_nth<0>, double>);
    static_assert(std::is_same_v<T::template select_nth<1>, int>);
    static_assert(std::is_same_v<T::template select_nth<2>, std::string>);
    static_assert(std::is_same_v<T::template select_nth<3>, std::string_view>);
    static_assert(T::has_type<int>);
    static_assert(T::has_type<double>);
    static_assert(T::has_type<std::string>);
    static_assert(T::has_type<std::string_view>);
    static_assert(not T::has_type<bool>);

    WHEN("element v0 set")
    {
      constexpr auto element = std::in_place_type<double>;
      T a{element, 0.5};
      CHECK(a.data.v0 == 0.5);
      CHECK(a.index == 0);
      CHECK(a.template has_value<double>());
      CHECK(a.has_value(std::in_place_type<double>));
      CHECK(not a.template has_value<int>());
      CHECK(not a.has_value(std::in_place_type<int>));
      CHECK(not a.template has_value<std::string>());
      CHECK(not a.has_value(std::in_place_type<std::string>));
      CHECK(not a.template has_value<std::string_view>());
      CHECK(not a.has_value(std::in_place_type<std::string_view>));

      constexpr auto fn5 = [](auto &) {};
      constexpr auto fn6 = [](some_in_place_type auto, auto...) {};
      static_assert(fn::detail::typelist_invocable<decltype(fn5), decltype(a) &>);
      static_assert(fn::detail::typelist_type_invocable<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), double *>);
      CHECK(a.get_ptr(element) == &a.data.v0);
      CHECK(a.get_ptr(std::in_place_type<int>) == nullptr);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), double const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v0);
      CHECK(std::as_const(a).get_ptr(std::in_place_type<int>) == nullptr);
    }

    WHEN("element v1 set")
    {
      constexpr auto element = std::in_place_type<int>;
      T a{element, 42};
      CHECK(a.data.v1 == 42);
      CHECK(a.index == 1);
      CHECK(not a.template has_value<double>());
      CHECK(not a.has_value(std::in_place_type<double>));
      CHECK(a.template has_value<int>());
      CHECK(a.has_value(std::in_place_type<int>));
      CHECK(not a.template has_value<std::string>());
      CHECK(not a.has_value(std::in_place_type<std::string>));
      CHECK(not a.template has_value<std::string_view>());
      CHECK(not a.has_value(std::in_place_type<std::string_view>));

      static_assert(fn::detail::typelist_invocable<decltype(fn1), decltype(a)>);
      static_assert(fn::detail::typelist_invocable<decltype(fn2), decltype(a)>);
      static_assert(not fn::detail::typelist_type_invocable<decltype(fn1), decltype(a)>);
      static_assert(fn::detail::typelist_invocable<decltype(fn2), decltype(a)>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), int *>);
      CHECK(a.get_ptr(element) == &a.data.v1);
      CHECK(a.get_ptr(std::in_place_type<double>) == nullptr);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), int const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v1);
      CHECK(std::as_const(a).get_ptr(std::in_place_type<double>) == nullptr);
    }

    WHEN("element v2 set")
    {
      constexpr auto element = std::in_place_type<std::string>;
      T a{element, "bar"};
      CHECK(a.data.v2 == "bar");
      CHECK(a.index == 2);
      CHECK(not a.template has_value<double>());
      CHECK(not a.has_value(std::in_place_type<double>));
      CHECK(not a.template has_value<int>());
      CHECK(not a.has_value(std::in_place_type<int>));
      CHECK(a.template has_value<std::string>());
      CHECK(a.has_value(std::in_place_type<std::string>));
      CHECK(not a.template has_value<std::string_view>());
      CHECK(not a.has_value(std::in_place_type<std::string_view>));

      constexpr auto fn5 = [](auto &) {};
      constexpr auto fn6 = [](some_in_place_type auto, auto...) {};
      static_assert(fn::detail::typelist_invocable<decltype(fn5), decltype(a) &>);
      static_assert(fn::detail::typelist_type_invocable<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), std::string *>);
      CHECK(a.get_ptr(element) == &a.data.v2);
      CHECK(a.get_ptr(std::in_place_type<double>) == nullptr);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), std::string const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v2);
      CHECK(std::as_const(a).get_ptr(std::in_place_type<double>) == nullptr);
    }

    WHEN("element v3 set")
    {
      constexpr auto element = std::in_place_type<std::string_view>;
      T a{element, "baz"};
      CHECK(a.data.v3 == "baz");
      CHECK(a.index == 3);
      CHECK(not a.template has_value<double>());
      CHECK(not a.has_value(std::in_place_type<double>));
      CHECK(not a.template has_value<int>());
      CHECK(not a.has_value(std::in_place_type<int>));
      CHECK(not a.template has_value<std::string>());
      CHECK(not a.has_value(std::in_place_type<std::string>));
      CHECK(a.template has_value<std::string_view>());
      CHECK(a.has_value(std::in_place_type<std::string_view>));

      constexpr auto fn5 = [](auto &) {};
      constexpr auto fn6 = [](some_in_place_type auto, auto...) {};
      static_assert(fn::detail::typelist_invocable<decltype(fn5), decltype(a) &>);
      static_assert(fn::detail::typelist_type_invocable<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), std::string_view *>);
      CHECK(a.get_ptr(element) == &a.data.v3);
      CHECK(a.get_ptr(std::in_place_type<double>) == nullptr);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), std::string_view const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v3);
      CHECK(std::as_const(a).get_ptr(std::in_place_type<double>) == nullptr);
    }
  }

  WHEN("size 5")
  {
    using T = sum<double, int, std::string, std::string_view, std::vector<int>>;
    static_assert(T::size == 5);
    static_assert(std::is_same_v<T::template select_nth<0>, double>);
    static_assert(std::is_same_v<T::template select_nth<1>, int>);
    static_assert(std::is_same_v<T::template select_nth<2>, std::string>);
    static_assert(std::is_same_v<T::template select_nth<3>, std::string_view>);
    static_assert(std::is_same_v<T::template select_nth<4>, std::vector<int>>);
    static_assert(T::has_type<int>);
    static_assert(T::has_type<double>);
    static_assert(T::has_type<std::string>);
    static_assert(T::has_type<std::string_view>);
    static_assert(T::has_type<std::vector<int>>);
    static_assert(not T::has_type<bool>);

    WHEN("element v0 set")
    {
      constexpr auto element = std::in_place_type<double>;
      T a{element, 0.5};
      CHECK(a.data.v0 == 0.5);
      CHECK(a.index == 0);
      CHECK(a.template has_value<double>());
      CHECK(a.has_value(std::in_place_type<double>));
      CHECK(not a.template has_value<int>());
      CHECK(not a.has_value(std::in_place_type<int>));
      CHECK(not a.template has_value<std::string>());
      CHECK(not a.has_value(std::in_place_type<std::string>));
      CHECK(not a.template has_value<std::string_view>());
      CHECK(not a.has_value(std::in_place_type<std::string_view>));
      CHECK(not a.template has_value<std::vector<int>>());
      CHECK(not a.has_value(std::in_place_type<std::vector<int>>));

      constexpr auto fn5 = [](auto &) {};
      constexpr auto fn6 = [](some_in_place_type auto, auto...) {};
      static_assert(fn::detail::typelist_invocable<decltype(fn5), decltype(a) &>);
      static_assert(fn::detail::typelist_type_invocable<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), double *>);
      CHECK(a.get_ptr(element) == &a.data.v0);
      CHECK(a.get_ptr(std::in_place_type<int>) == nullptr);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), double const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v0);
      CHECK(std::as_const(a).get_ptr(std::in_place_type<int>) == nullptr);
    }

    WHEN("element v1 set")
    {
      constexpr auto element = std::in_place_type<int>;
      T a{element, 42};
      CHECK(a.data.v1 == 42);
      CHECK(a.index == 1);
      CHECK(not a.template has_value<double>());
      CHECK(not a.has_value(std::in_place_type<double>));
      CHECK(a.template has_value<int>());
      CHECK(a.has_value(std::in_place_type<int>));
      CHECK(not a.template has_value<std::string>());
      CHECK(not a.has_value(std::in_place_type<std::string>));
      CHECK(not a.template has_value<std::string_view>());
      CHECK(not a.has_value(std::in_place_type<std::string_view>));
      CHECK(not a.template has_value<std::vector<int>>());
      CHECK(not a.has_value(std::in_place_type<std::vector<int>>));

      static_assert(fn::detail::typelist_invocable<decltype(fn1), decltype(a)>);
      static_assert(fn::detail::typelist_invocable<decltype(fn2), decltype(a)>);
      static_assert(not fn::detail::typelist_type_invocable<decltype(fn1), decltype(a)>);
      static_assert(fn::detail::typelist_invocable<decltype(fn2), decltype(a)>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), int *>);
      CHECK(a.get_ptr(element) == &a.data.v1);
      CHECK(a.get_ptr(std::in_place_type<double>) == nullptr);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), int const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v1);
      CHECK(std::as_const(a).get_ptr(std::in_place_type<double>) == nullptr);
    }

    WHEN("element v2 set")
    {
      constexpr auto element = std::in_place_type<std::string>;
      T a{element, "bar"};
      CHECK(a.data.v2 == "bar");
      CHECK(a.index == 2);
      CHECK(not a.template has_value<double>());
      CHECK(not a.has_value(std::in_place_type<double>));
      CHECK(not a.template has_value<int>());
      CHECK(not a.has_value(std::in_place_type<int>));
      CHECK(a.template has_value<std::string>());
      CHECK(a.has_value(std::in_place_type<std::string>));
      CHECK(not a.template has_value<std::string_view>());
      CHECK(not a.has_value(std::in_place_type<std::string_view>));
      CHECK(not a.template has_value<std::vector<int>>());
      CHECK(not a.has_value(std::in_place_type<std::vector<int>>));

      constexpr auto fn5 = [](auto &) {};
      constexpr auto fn6 = [](some_in_place_type auto, auto...) {};
      static_assert(fn::detail::typelist_invocable<decltype(fn5), decltype(a) &>);
      static_assert(fn::detail::typelist_type_invocable<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), std::string *>);
      CHECK(a.get_ptr(element) == &a.data.v2);
      CHECK(a.get_ptr(std::in_place_type<double>) == nullptr);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), std::string const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v2);
      CHECK(std::as_const(a).get_ptr(std::in_place_type<double>) == nullptr);
    }

    WHEN("element v3 set")
    {
      constexpr auto element = std::in_place_type<std::string_view>;
      T a{element, "baz"};
      CHECK(a.data.v3 == "baz");
      CHECK(a.index == 3);
      CHECK(not a.template has_value<double>());
      CHECK(not a.has_value(std::in_place_type<double>));
      CHECK(not a.template has_value<int>());
      CHECK(not a.has_value(std::in_place_type<int>));
      CHECK(not a.template has_value<std::string>());
      CHECK(not a.has_value(std::in_place_type<std::string>));
      CHECK(a.template has_value<std::string_view>());
      CHECK(a.has_value(std::in_place_type<std::string_view>));
      CHECK(not a.template has_value<std::vector<int>>());
      CHECK(not a.has_value(std::in_place_type<std::vector<int>>));

      constexpr auto fn5 = [](auto &) {};
      constexpr auto fn6 = [](some_in_place_type auto, auto...) {};
      static_assert(fn::detail::typelist_invocable<decltype(fn5), decltype(a) &>);
      static_assert(fn::detail::typelist_type_invocable<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), std::string_view *>);
      CHECK(a.get_ptr(element) == &a.data.v3);
      CHECK(a.get_ptr(std::in_place_type<double>) == nullptr);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), std::string_view const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v3);
      CHECK(std::as_const(a).get_ptr(std::in_place_type<double>) == nullptr);
    }

    WHEN("more elements set")
    {
      std::vector<int> const foo{3, 14, 15};
      constexpr auto element = std::in_place_type<std::vector<int>>;
      T a{element, foo};
      CHECK(a.data.more.v0 == foo);
      CHECK(a.index == 4);
      CHECK(not a.template has_value<double>());
      CHECK(not a.has_value(std::in_place_type<double>));
      CHECK(not a.template has_value<int>());
      CHECK(not a.has_value(std::in_place_type<int>));
      CHECK(not a.template has_value<std::string>());
      CHECK(not a.has_value(std::in_place_type<std::string>));
      CHECK(not a.template has_value<std::string_view>());
      CHECK(not a.has_value(std::in_place_type<std::string_view>));
      CHECK(a.template has_value<std::vector<int>>());
      CHECK(a.has_value(std::in_place_type<std::vector<int>>));

      constexpr auto fn5 = [](auto &) {};
      constexpr auto fn6 = [](some_in_place_type auto, auto...) {};
      static_assert(fn::detail::typelist_invocable<decltype(fn5), decltype(a) &>);
      static_assert(fn::detail::typelist_type_invocable<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), std::vector<int> *>);
      CHECK(a.get_ptr(element) == &a.data.more.v0);
      CHECK(a.get_ptr(std::in_place_type<double>) == nullptr);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), std::vector<int> const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.more.v0);
      CHECK(std::as_const(a).get_ptr(std::in_place_type<double>) == nullptr);
    }
  }
}

TEST_CASE("sum functions", "[sum][invoke_to]")
{
  // NOTE We have 5 different specializations, need to test each. This test is
  // ridiculously long to exercise the value-category preserving FWD(v) in apply_variadic_union

  using namespace fn;
  constexpr auto fn1 = [](auto i) noexcept -> std::size_t { return sizeof(i); };
  constexpr auto fn2 = [](auto, auto i) noexcept -> std::size_t { return sizeof(i); };

  WHEN("size 1")
  {
    sum<int> a{std::in_place_type<int>, 42};
    static_assert(decltype(a)::size == 1);
    CHECK(a.data.v0 == 42);

    WHEN("value only")
    {
      CHECK(a.invoke_to(fn1) == 4);
      CHECK(a.invoke_to( //
          fn::overload(  //
              [](auto) -> bool { throw 1; }, [](int &i) -> bool { return i == 42; },
              [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
              [](int const &&) -> bool { throw 0; })));
      CHECK(std::as_const(a).invoke_to( //
          fn::overload(                 //
              [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
              [](int const &i) -> bool { return i == 42; }, [](int &&) -> bool { throw 0; },
              [](int const &&) -> bool { throw 0; })));
      CHECK(sum<int>{std::in_place_type<int>, 42}.invoke_to( //
          fn::overload(                                      //
              [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
              [](int &&i) -> bool { return i == 42; }, [](int const &&) -> bool { throw 0; })));
      CHECK(std::move(std::as_const(a))
                .invoke_to(       //
                    fn::overload( //
                        [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                        [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                        [](int const &&i) -> bool { return i == 42; })));
    }

    WHEN("tag and value")
    {
      CHECK(a.invoke_to(fn2) == 4);
      CHECK(a.invoke_to( //
          fn::overload(  //
              [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &i) -> bool { return i == 42; },
              [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
              [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
              [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
      CHECK(std::as_const(a).invoke_to( //
          fn::overload(                 //
              [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
              [](std::in_place_type_t<int>, int const &i) -> bool { return i == 42; },
              [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
              [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
      CHECK(sum<int>{std::in_place_type<int>, 42}.invoke_to( //
          fn::overload(                                      //
              [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
              [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
              [](std::in_place_type_t<int>, int &&i) -> bool { return i == 42; },
              [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
      CHECK(std::move(std::as_const(a))
                .invoke_to(       //
                    fn::overload( //
                        [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &&i) -> bool { return i == 42; })));
    }
  }

  WHEN("size 2")
  {
    using type = sum<double, int>;
    static_assert(type::size == 2);

    WHEN("element v0 set")
    {
      type a{std::in_place_type<double>, 0.5};
      CHECK(a.data.v0 == 0.5);
      WHEN("value only")
      {
        CHECK(a.invoke_to(fn1) == 8);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto) -> bool { throw 1; }, [](double &i) -> bool { return i == 0.5; },
                [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(                 //
                [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                [](double const &i) -> bool { return i == 0.5; }, [](double &&) -> bool { throw 0; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<double>, 0.5}.invoke_to( //
            fn::overload(                                      //
                [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                [](double const &) -> bool { throw 0; }, [](double &&i) -> bool { return i == 0.5; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke_to(       //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                          [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                          [](double const &&i) -> bool { return i == 0.5; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke_to(fn2) == 8);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<double>, double &i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(                 //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<double>, 0.5}.invoke_to( //
            fn::overload(                                      //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double &&i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke_to(       //
                      fn::overload( //
                          [](auto, auto) -> bool { throw 1; },
                          [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double const &&i) -> bool { return i == 0.5; })));
      }
    }

    WHEN("element v1 set")
    {
      type a{std::in_place_type<int>, 42};
      CHECK(a.data.v1 == 42);

      WHEN("value only")
      {
        CHECK(a.invoke_to(fn1) == 4);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto) -> bool { throw 1; }, [](int &i) -> bool { return i == 42; },
                [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                [](int const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(                 //
                [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                [](int const &i) -> bool { return i == 42; }, [](int &&) -> bool { throw 0; },
                [](int const &&) -> bool { throw 0; })));
        CHECK(sum<int>{std::in_place_type<int>, 42}.invoke_to( //
            fn::overload(                                      //
                [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                [](int &&i) -> bool { return i == 42; }, [](int const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke_to(       //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                          [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                          [](int const &&i) -> bool { return i == 42; })));
      }

      WHEN("tag and value")
      {
        CHECK(a.invoke_to(fn2) == 4);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(                 //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(sum<int>{std::in_place_type<int>, 42}.invoke_to( //
            fn::overload(                                      //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int &&i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke_to(       //
                    fn::overload( //
                        [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &&i) -> bool { return i == 42; })));
      }
    }
  }

  WHEN("size 3")
  {
    using type = sum<double, int, std::string_view>;
    static_assert(type::size == 3);

    WHEN("element v0 set")
    {
      type a{std::in_place_type<double>, 0.5};
      CHECK(a.data.v0 == 0.5);
      WHEN("value only")
      {
        CHECK(a.invoke_to(fn1) == 8);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto) -> bool { throw 1; }, [](double &i) -> bool { return i == 0.5; },
                [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(                 //
                [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                [](double const &i) -> bool { return i == 0.5; }, [](double &&) -> bool { throw 0; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<double>, 0.5}.invoke_to( //
            fn::overload(                                      //
                [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                [](double const &) -> bool { throw 0; }, [](double &&i) -> bool { return i == 0.5; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke_to(       //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                          [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                          [](double const &&i) -> bool { return i == 0.5; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke_to(fn2) == 8);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<double>, double &i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(                 //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<double>, 0.5}.invoke_to( //
            fn::overload(                                      //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double &&i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke_to(       //
                      fn::overload( //
                          [](auto, auto) -> bool { throw 1; },
                          [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double const &&i) -> bool { return i == 0.5; })));
      }
    }

    WHEN("element v1 set")
    {
      type a{std::in_place_type<int>, 42};
      CHECK(a.data.v1 == 42);

      WHEN("value only")
      {
        CHECK(a.invoke_to(fn1) == 4);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto) -> bool { throw 1; }, [](int &i) -> bool { return i == 42; },
                [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                [](int const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(                 //
                [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                [](int const &i) -> bool { return i == 42; }, [](int &&) -> bool { throw 0; },
                [](int const &&) -> bool { throw 0; })));
        CHECK(sum<int>{std::in_place_type<int>, 42}.invoke_to( //
            fn::overload(                                      //
                [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                [](int &&i) -> bool { return i == 42; }, [](int const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke_to(       //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                          [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                          [](int const &&i) -> bool { return i == 42; })));
      }

      WHEN("tag and value")
      {
        CHECK(a.invoke_to(fn2) == 4);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(                 //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(sum<int>{std::in_place_type<int>, 42}.invoke_to( //
            fn::overload(                                      //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int &&i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke_to(       //
                    fn::overload( //
                        [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &&i) -> bool { return i == 42; })));
      }
    }

    WHEN("element v2 set")
    {
      type a{std::in_place_type<std::string_view>, "baz"};
      CHECK(a.data.v2 == "baz");
      WHEN("value only")
      {
        CHECK(a.invoke_to(fn1) == 16);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto) -> bool { throw 1; }, [](std::string_view &i) -> bool { return i == "baz"; },
                [](std::string_view const &) -> bool { throw 0; }, [](std::string_view &&) -> bool { throw 0; },
                [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(                 //
                [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                [](std::string_view const &i) -> bool { return i == "baz"; },
                [](std::string_view &&) -> bool { throw 0; }, [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string_view>, "baz"}.invoke_to( //
            fn::overload(                                                  //
                [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                [](std::string_view const &) -> bool { throw 0; },
                [](std::string_view &&i) -> bool { return i == "baz"; },
                [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke_to(       //
                    fn::overload( //
                        [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                        [](std::string_view const &) -> bool { throw 0; }, [](std::string_view &&) -> bool { throw 0; },
                        [](std::string_view const &&i) -> bool { return i == "baz"; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke_to(fn2) == 16);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string_view>, "baz"}.invoke_to( //
            fn::overload(
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke_to( //
                      fn::overload(
                          [](auto, auto) -> bool { throw 1; },
                          [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                          [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                          [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                          [](std::in_place_type_t<std::string_view>, std::string_view const &&i) -> bool {
                            return i == "baz";
                          })));
      }
    }
  }

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
        CHECK(a.invoke_to(fn1) == 8);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto) -> bool { throw 1; }, [](double &i) -> bool { return i == 0.5; },
                [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(                 //
                [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                [](double const &i) -> bool { return i == 0.5; }, [](double &&) -> bool { throw 0; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<double>, 0.5}.invoke_to( //
            fn::overload(                                      //
                [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                [](double const &) -> bool { throw 0; }, [](double &&i) -> bool { return i == 0.5; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke_to(       //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                          [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                          [](double const &&i) -> bool { return i == 0.5; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke_to(fn2) == 8);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<double>, double &i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(                 //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<double>, 0.5}.invoke_to( //
            fn::overload(                                      //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double &&i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke_to(       //
                      fn::overload( //
                          [](auto, auto) -> bool { throw 1; },
                          [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double const &&i) -> bool { return i == 0.5; })));
      }
    }

    WHEN("element v1 set")
    {
      type a{std::in_place_type<int>, 42};
      CHECK(a.data.v1 == 42);

      WHEN("value only")
      {
        CHECK(a.invoke_to(fn1) == 4);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto) -> bool { throw 1; }, [](int &i) -> bool { return i == 42; },
                [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                [](int const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(                 //
                [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                [](int const &i) -> bool { return i == 42; }, [](int &&) -> bool { throw 0; },
                [](int const &&) -> bool { throw 0; })));
        CHECK(sum<int>{std::in_place_type<int>, 42}.invoke_to( //
            fn::overload(                                      //
                [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                [](int &&i) -> bool { return i == 42; }, [](int const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke_to(       //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                          [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                          [](int const &&i) -> bool { return i == 42; })));
      }

      WHEN("tag and value")
      {
        CHECK(a.invoke_to(fn2) == 4);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(                 //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(sum<int>{std::in_place_type<int>, 42}.invoke_to( //
            fn::overload(                                      //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int &&i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke_to(       //
                    fn::overload( //
                        [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &&i) -> bool { return i == 42; })));
      }
    }

    WHEN("element v2 set")
    {
      type a{std::in_place_type<std::string>, "bar"};
      CHECK(a.data.v2 == "bar");
      WHEN("value only")
      {
        CHECK(a.invoke_to(fn1) == 32);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto) -> bool { throw 1; }, [](std::string &i) -> bool { return i == "bar"; },
                [](std::string const &) -> bool { throw 0; }, [](std::string &&) -> bool { throw 0; },
                [](std::string const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(                 //
                [](auto) -> bool { throw 1; }, [](std::string &) -> bool { throw 0; },
                [](std::string const &i) -> bool { return i == "bar"; }, [](std::string &&) -> bool { throw 0; },
                [](std::string const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string>, "bar"}.invoke_to( //
            fn::overload(                                             //
                [](auto) -> bool { throw 1; }, [](std::string &) -> bool { throw 0; },
                [](std::string const &) -> bool { throw 0; }, [](std::string &&i) -> bool { return i == "bar"; },
                [](std::string const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke_to(       //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](std::string &) -> bool { throw 0; },
                          [](std::string const &) -> bool { throw 0; }, [](std::string &&) -> bool { throw 0; },
                          [](std::string const &&i) -> bool { return i == "bar"; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke_to(fn2) == 32);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string>, std::string &i) -> bool { return i == "bar"; },
                [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(                 //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string const &i) -> bool { return i == "bar"; },
                [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string>, "bar"}.invoke_to( //
            fn::overload(                                             //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string &&i) -> bool { return i == "bar"; },
                [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke_to(       //
                    fn::overload( //
                        [](auto, auto) -> bool { throw 1; },
                        [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                        [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                        [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                        [](std::in_place_type_t<std::string>, std::string const &&i) -> bool { return i == "bar"; })));
      }
    }

    WHEN("element v3 set")
    {
      type a{std::in_place_type<std::string_view>, "baz"};
      CHECK(a.data.v3 == "baz");
      WHEN("value only")
      {
        CHECK(a.invoke_to(fn1) == 16);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto) -> bool { throw 1; }, [](std::string_view &i) -> bool { return i == "baz"; },
                [](std::string_view const &) -> bool { throw 0; }, [](std::string_view &&) -> bool { throw 0; },
                [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(                 //
                [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                [](std::string_view const &i) -> bool { return i == "baz"; },
                [](std::string_view &&) -> bool { throw 0; }, [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string_view>, "baz"}.invoke_to( //
            fn::overload(                                                  //
                [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                [](std::string_view const &) -> bool { throw 0; },
                [](std::string_view &&i) -> bool { return i == "baz"; },
                [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke_to(       //
                    fn::overload( //
                        [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                        [](std::string_view const &) -> bool { throw 0; }, [](std::string_view &&) -> bool { throw 0; },
                        [](std::string_view const &&i) -> bool { return i == "baz"; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke_to(fn2) == 16);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string_view>, "baz"}.invoke_to( //
            fn::overload(
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke_to( //
                      fn::overload(
                          [](auto, auto) -> bool { throw 1; },
                          [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                          [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                          [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                          [](std::in_place_type_t<std::string_view>, std::string_view const &&i) -> bool {
                            return i == "baz";
                          })));
      }
    }
  }

  WHEN("size 5")
  {
    using type = sum<double, int, std::string, std::string_view, std::vector<int>>;
    static_assert(type::size == 5);

    WHEN("element v0 set")
    {
      type a{std::in_place_type<double>, 0.5};
      CHECK(a.data.v0 == 0.5);
      WHEN("value only")
      {
        CHECK(a.invoke_to(fn1) == 8);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto) -> bool { throw 1; }, [](double &i) -> bool { return i == 0.5; },
                [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(                 //
                [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                [](double const &i) -> bool { return i == 0.5; }, [](double &&) -> bool { throw 0; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<double>, 0.5}.invoke_to( //
            fn::overload(                                      //
                [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                [](double const &) -> bool { throw 0; }, [](double &&i) -> bool { return i == 0.5; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke_to(       //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                          [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                          [](double const &&i) -> bool { return i == 0.5; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke_to(fn2) == 8);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<double>, double &i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(                 //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<double>, 0.5}.invoke_to( //
            fn::overload(                                      //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double &&i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke_to(       //
                      fn::overload( //
                          [](auto, auto) -> bool { throw 1; },
                          [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double const &&i) -> bool { return i == 0.5; })));
      }
    }

    WHEN("element v1 set")
    {
      type a{std::in_place_type<int>, 42};
      CHECK(a.data.v1 == 42);

      WHEN("value only")
      {
        CHECK(a.invoke_to(fn1) == 4);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto) -> bool { throw 1; }, [](int &i) -> bool { return i == 42; },
                [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                [](int const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(                 //
                [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                [](int const &i) -> bool { return i == 42; }, [](int &&) -> bool { throw 0; },
                [](int const &&) -> bool { throw 0; })));
        CHECK(sum<int>{std::in_place_type<int>, 42}.invoke_to( //
            fn::overload(                                      //
                [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                [](int &&i) -> bool { return i == 42; }, [](int const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke_to(       //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                          [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                          [](int const &&i) -> bool { return i == 42; })));
      }

      WHEN("tag and value")
      {
        CHECK(a.invoke_to(fn2) == 4);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(                 //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(sum<int>{std::in_place_type<int>, 42}.invoke_to( //
            fn::overload(                                      //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int &&i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke_to(       //
                    fn::overload( //
                        [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &&i) -> bool { return i == 42; })));
      }
    }

    WHEN("element v2 set")
    {
      type a{std::in_place_type<std::string>, "bar"};
      CHECK(a.data.v2 == "bar");
      WHEN("value only")
      {
        CHECK(a.invoke_to(fn1) == 32);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto) -> bool { throw 1; }, [](std::string &i) -> bool { return i == "bar"; },
                [](std::string const &) -> bool { throw 0; }, [](std::string &&) -> bool { throw 0; },
                [](std::string const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(                 //
                [](auto) -> bool { throw 1; }, [](std::string &) -> bool { throw 0; },
                [](std::string const &i) -> bool { return i == "bar"; }, [](std::string &&) -> bool { throw 0; },
                [](std::string const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string>, "bar"}.invoke_to( //
            fn::overload(                                             //
                [](auto) -> bool { throw 1; }, [](std::string &) -> bool { throw 0; },
                [](std::string const &) -> bool { throw 0; }, [](std::string &&i) -> bool { return i == "bar"; },
                [](std::string const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke_to(       //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](std::string &) -> bool { throw 0; },
                          [](std::string const &) -> bool { throw 0; }, [](std::string &&) -> bool { throw 0; },
                          [](std::string const &&i) -> bool { return i == "bar"; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke_to(fn2) == 32);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string>, std::string &i) -> bool { return i == "bar"; },
                [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(                 //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string const &i) -> bool { return i == "bar"; },
                [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string>, "bar"}.invoke_to( //
            fn::overload(                                             //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string &&i) -> bool { return i == "bar"; },
                [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke_to(       //
                    fn::overload( //
                        [](auto, auto) -> bool { throw 1; },
                        [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                        [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                        [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                        [](std::in_place_type_t<std::string>, std::string const &&i) -> bool { return i == "bar"; })));
      }
    }

    WHEN("element v3 set")
    {
      type a{std::in_place_type<std::string_view>, "baz"};
      CHECK(a.data.v3 == "baz");
      WHEN("value only")
      {
        CHECK(a.invoke_to(fn1) == 16);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto) -> bool { throw 1; }, [](std::string_view &i) -> bool { return i == "baz"; },
                [](std::string_view const &) -> bool { throw 0; }, [](std::string_view &&) -> bool { throw 0; },
                [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(                 //
                [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                [](std::string_view const &i) -> bool { return i == "baz"; },
                [](std::string_view &&) -> bool { throw 0; }, [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string_view>, "baz"}.invoke_to( //
            fn::overload(                                                  //
                [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                [](std::string_view const &) -> bool { throw 0; },
                [](std::string_view &&i) -> bool { return i == "baz"; },
                [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke_to(       //
                    fn::overload( //
                        [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                        [](std::string_view const &) -> bool { throw 0; }, [](std::string_view &&) -> bool { throw 0; },
                        [](std::string_view const &&i) -> bool { return i == "baz"; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke_to(fn2) == 16);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string_view>, "baz"}.invoke_to( //
            fn::overload(
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke_to( //
                      fn::overload(
                          [](auto, auto) -> bool { throw 1; },
                          [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                          [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                          [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                          [](std::in_place_type_t<std::string_view>, std::string_view const &&i) -> bool {
                            return i == "baz";
                          })));
      }
    }

    WHEN("more elements set")
    {
      std::vector<int> const foo{3, 14, 15, 92};
      type a{std::in_place_type<std::vector<int>>, foo};
      CHECK(a.data.more.v0 == foo);
      WHEN("value only")
      {
        CHECK(a.invoke_to(fn1) == 24);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto) -> bool { throw 1; }, [&](std::vector<int> &i) -> bool { return i == foo; },
                [](std::vector<int> const &) -> bool { throw 0; }, [](std::vector<int> &&) -> bool { throw 0; },
                [](std::vector<int> const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(                 //
                [](auto) -> bool { throw 1; }, [](std::vector<int> &) -> bool { throw 0; },
                [&](std::vector<int> const &i) -> bool { return i == foo; },
                [](std::vector<int> &&) -> bool { throw 0; }, [](std::vector<int> const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::vector<int>>, foo}.invoke_to( //
            fn::overload(                                                //
                [](auto) -> bool { throw 1; }, [](std::vector<int> &) -> bool { throw 0; },
                [](std::vector<int> const &) -> bool { throw 0; },
                [&](std::vector<int> &&i) -> bool { return i == foo; },
                [](std::vector<int> const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke_to(       //
                    fn::overload( //
                        [](auto) -> bool { throw 1; }, [](std::vector<int> &) -> bool { throw 0; },
                        [](std::vector<int> const &) -> bool { throw 0; }, [](std::vector<int> &&) -> bool { throw 0; },
                        [&](std::vector<int> const &&i) -> bool { return i == foo; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke_to(fn2) == 24);
        CHECK(a.invoke_to( //
            fn::overload(  //
                [](auto, auto) -> bool { throw 1; },
                [&](std::in_place_type_t<std::vector<int>>, std::vector<int> &i) -> bool { return i == foo; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke_to( //
            fn::overload(
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> &) -> bool { throw 0; },
                [&](std::in_place_type_t<std::vector<int>>, std::vector<int> const &i) -> bool { return i == foo; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::vector<int>>, foo}.invoke_to( //
            fn::overload(                                                //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> &) -> bool { throw 0; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> const &) -> bool { throw 0; },
                [&](std::in_place_type_t<std::vector<int>>, std::vector<int> &&i) -> bool { return i == foo; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke_to( //
                      fn::overload(
                          [](auto, auto) -> bool { throw 1; },
                          [](std::in_place_type_t<std::vector<int>>, std::vector<int> &) -> bool { throw 0; },
                          [](std::in_place_type_t<std::vector<int>>, std::vector<int> const &) -> bool { throw 0; },
                          [](std::in_place_type_t<std::vector<int>>, std::vector<int> &&) -> bool { throw 0; },
                          [&](std::in_place_type_t<std::vector<int>>, std::vector<int> const &&i) -> bool {
                            return i == foo;
                          })));
      }
    }
  }
}
