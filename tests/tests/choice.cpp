// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/choice.hpp"
#include "functional/utility.hpp"

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

} // anonymous namespace

TEST_CASE("choice non-monadic functionality", "[choice]")
{
  // NOTE This test looks very similar to test in sum.cpp - for good reason.

  using fn::choice;
  using fn::some_in_place_type;

  WHEN("choice_for")
  {
    static_assert(std::same_as<fn::choice_for<int>, fn::choice<int>>);
    static_assert(std::same_as<fn::choice_for<int, bool>, fn::choice<bool, int>>);
    static_assert(std::same_as<fn::choice_for<bool, int>, fn::choice<bool, int>>);
    static_assert(std::same_as<fn::choice_for<int, NonCopyable>, fn::choice<NonCopyable, int>>);
    static_assert(std::same_as<fn::choice_for<NonCopyable, int>, fn::choice<NonCopyable, int>>);
    static_assert(std::same_as<fn::choice_for<int, bool, NonCopyable>, fn::choice<NonCopyable, bool, int>>);
  }

  WHEN("invocable")
  {
    using type = choice<TestType, int>;
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

  WHEN("type_invocable")
  {
    using type = choice<TestType, int>;
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

    static_assert(fn::typelist_type_invocable<decltype([](some_in_place_type auto, auto &) {}), choice<NonCopyable> &>);
    static_assert(not fn::typelist_type_invocable<decltype([](some_in_place_type auto, auto) {}),
                                                  NonCopyable &>); // copy-constructor not available

    static_assert(fn::typelist_type_invocable<decltype([](some_in_place_type auto, auto &) {}), choice<NonCopyable> &>);
    static_assert(not fn::typelist_type_invocable<decltype([](some_in_place_type auto, auto) {}),
                                                  NonCopyable &>); // copy-constructor not available
  }

  WHEN("check destructor call")
  {
    {
      choice<TestType> s{std::in_place_type<TestType>};
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
    constexpr choice<int> a = 12;
    static_assert(a == choice{12});

    constexpr choice<bool> b{false};
    static_assert(b == choice{false});

    WHEN("CTAD")
    {
      choice a{42};
      static_assert(std::is_same_v<decltype(a), choice<int>>);
      CHECK(a == choice<int>{42});

      constexpr choice b{false};
      static_assert(std::is_same_v<decltype(b), choice<bool> const>);
      static_assert(b == choice<bool>{false});

      constexpr auto c = choice{std::array<int, 3>{3, 14, 15}};
      static_assert(std::is_same_v<decltype(c), choice<std::array<int, 3>> const>);
      static_assert(
          c.transform_to([](auto &&a) -> bool { return a.size() == 3 && a[0] == 3 && a[1] == 14 && a[2] == 15; }));
    }

    WHEN("move from rvalue")
    {
      using T = fn::choice<bool, int>;
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
      using T = fn::choice<bool, int>;
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
    choice<NonCopyable> a{std::in_place_type<NonCopyable>, 42};
    CHECK(a.transform_to([](auto &i) -> bool { return i.i == 42; }));

    WHEN("CTAD")
    {
      constexpr auto a = choice{std::in_place_type<NonCopyable>, 42};
      static_assert(std::is_same_v<decltype(a), choice<NonCopyable> const>);

      auto b = choice{std::in_place_type<NonCopyable>, 42};
      static_assert(std::is_same_v<decltype(b), choice<NonCopyable>>);
    }

    WHEN("CTAD with const")
    {
      constexpr auto a = choice{std::in_place_type<NonCopyable const>, 42};
      static_assert(std::is_same_v<decltype(a), choice<NonCopyable const> const>);

      auto b = choice{std::in_place_type<NonCopyable const>, 42};
      static_assert(std::is_same_v<decltype(b), choice<NonCopyable const>>);
    }
  }

  WHEN("forwarding constructors (aggregate)")
  {
    WHEN("regular")
    {
      choice<std::array<int, 3>> a{std::in_place_type<std::array<int, 3>>, 1, 2, 3};
      static_assert(decltype(a)::has_type<std::array<int, 3>>);
      static_assert(not decltype(a)::has_type<int>);
      CHECK(a.has_value(std::in_place_type<std::array<int, 3>>));
      CHECK(a.template has_value<std::array<int, 3>>());
      CHECK(a.transform_to([](auto &i) -> bool {
        return std::same_as<std::array<int, 3> &, decltype(i)> && i.size() == 3 && i[0] == 1 && i[1] == 2 && i[2] == 3;
      }));
    }

    WHEN("constexpr")
    {
      constexpr choice<std::array<int, 3>> a{std::in_place_type<std::array<int, 3>>, 1, 2, 3};
      static_assert(decltype(a)::has_type<std::array<int, 3>>);
      static_assert(not decltype(a)::has_type<int>);
      static_assert(a.has_value(std::in_place_type<std::array<int, 3>>));
      static_assert(a.template has_value<std::array<int, 3>>());
      static_assert(a.transform_to([](auto &i) -> bool {
        return std::same_as<std::array<int, 3> const &, decltype(i)> && i.size() == 3 && i[0] == 1 && i[1] == 2
               && i[2] == 3;
      }));
    }

    WHEN("CTAD")
    {
      constexpr auto a = choice{std::in_place_type<std::array<int, 3>>, 1, 2, 3};
      static_assert(std::is_same_v<decltype(a), choice<std::array<int, 3>> const>);

      auto b = choice{std::in_place_type<std::array<int, 3>>, 1, 2, 3};
      static_assert(std::is_same_v<decltype(b), choice<std::array<int, 3>>>);
    }

    WHEN("CTAD with const")
    {
      constexpr auto a = choice{std::in_place_type<std::array<int, 3> const>, 1, 2, 3};
      static_assert(std::is_same_v<decltype(a), choice<std::array<int, 3> const> const>);

      auto b = choice{std::in_place_type<std::array<int, 3> const>, 1, 2, 3};
      static_assert(std::is_same_v<decltype(b), choice<std::array<int, 3> const>>);
    }
  }

  WHEN("has_type type mismatch")
  {
    using type = choice<bool, int>;
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
    using type = choice<bool, int>;

    type const a{std::in_place_type<int>, 42};
    static_assert(std::is_same_v<bool, decltype(a == choice{42})>);
    CHECK(a == type{42});
    CHECK(type{42} == a);
    CHECK(a != type{41});
    CHECK(type{41} != a);
    CHECK(a != type{true});
    CHECK(type{false} != a);
    CHECK(a == choice{42});
    CHECK(choice{42} == a);
    CHECK(a != choice{41});
    CHECK(choice{41} != a);
    CHECK(a != choice{false});
    CHECK(choice{true} != a);
    CHECK(a == choice<double, int>{42});
    CHECK(choice<double, int>{42} == a);
    CHECK(a != choice<double, int>{41});
    CHECK(choice<double, int>{41} != a);
    CHECK(choice{0.5} != a);
    CHECK(a != choice{0.5});

    WHEN("constexpr")
    {
      constexpr type a{std::in_place_type<int>, 42};
      static_assert(std::is_same_v<bool, decltype(a == choice{42})>);
      static_assert(a == type{42});
      static_assert(type{42} == a);
      static_assert(a != type{41});
      static_assert(type{41} != a);
      static_assert(a != type{true});
      static_assert(type{false} != a);
      static_assert(a == choice{42});
      static_assert(choice{42} == a);
      static_assert(a != choice{41});
      static_assert(choice{41} != a);
      static_assert(a != choice{false});
      static_assert(choice{true} != a);
      static_assert(a == choice<double, int>{42});
      static_assert(choice<double, int>{42} == a);
      static_assert(a != choice<double, int>{41});
      static_assert(choice<double, int>{41} != a);
      static_assert(choice{0.5} != a);
      static_assert(a != choice{0.5});

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

  WHEN("transform_to")
  {
    choice<int> a{std::in_place_type<int>, 42};
    WHEN("value only")
    {
      CHECK(a.transform_to(fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { return true; },
                                        [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                        [](int const &&) -> bool { throw 0; })));
      CHECK(std::as_const(a).transform_to(fn::overload(
          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { return true; },
          [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; })));
      CHECK(choice<int>{std::in_place_type<int>, 42}.transform_to(fn::overload(
          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
          [](int &&) -> bool { return true; }, [](int const &&) -> bool { throw 0; })));
      CHECK(std::move(std::as_const(a))
                .transform_to(fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                           [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                           [](int const &&) -> bool { return true; })));

      WHEN("constexpr")
      {
        constexpr choice<int> a{std::in_place_type<int>, 42};
        static_assert(a.transform_to(fn::overload(
            [](auto) -> std::false_type { return {}; }, //
            [](int &) -> std::false_type { return {}; }, [](int const &) -> std::true_type { return {}; },
            [](int &&) -> std::false_type { return {}; }, [](int const &&) -> std::false_type { return {}; })));
        static_assert(std::move(a).transform_to(fn::overload(
            [](auto) -> std::false_type { return {}; }, //
            [](int &) -> std::false_type { return {}; }, [](int const &) -> std::false_type { return {}; },
            [](int &&) -> std::false_type { return {}; }, [](int const &&) -> std::true_type { return {}; })));
      }
    }

    WHEN("tag and value")
    {
      CHECK(a.transform_to(fn::overload([](auto, auto) -> bool { throw 1; },
                                        [](std::in_place_type_t<int>, int &) -> bool { return true; },
                                        [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                                        [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                        [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
      CHECK(std::as_const(a).transform_to(
          fn::overload([](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                       [](std::in_place_type_t<int>, int const &) -> bool { return true; },
                       [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                       [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
      CHECK(choice<int>{std::in_place_type<int>, 42}.transform_to(
          fn::overload([](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                       [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                       [](std::in_place_type_t<int>, int &&) -> bool { return true; },
                       [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
      CHECK(std::move(std::as_const(a))
                .transform_to(fn::overload([](auto, auto) -> bool { throw 1; },
                                           [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                                           [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                                           [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                           [](std::in_place_type_t<int>, int const &&) -> bool { return true; })));
      WHEN("constexpr")
      {
        constexpr choice<int> a{std::in_place_type<int>, 42};
        static_assert(a.transform_to(
            fn::overload([](auto, auto) -> std::false_type { return {}; }, //
                         [](std::in_place_type_t<int>, int &) -> std::false_type { return {}; },
                         [](std::in_place_type_t<int>, int const &) -> std::true_type { return {}; },
                         [](std::in_place_type_t<int>, int &&) -> std::false_type { return {}; },
                         [](std::in_place_type_t<int>, int const &&) -> std::false_type { return {}; })));
        static_assert(std::move(a).transform_to(
            fn::overload([](auto, auto) -> std::false_type { return {}; }, //
                         [](std::in_place_type_t<int>, int &) -> std::false_type { return {}; },
                         [](std::in_place_type_t<int>, int const &) -> std::false_type { return {}; },
                         [](std::in_place_type_t<int>, int &&) -> std::false_type { return {}; },
                         [](std::in_place_type_t<int>, int const &&) -> std::true_type { return {}; })));
      }
    }
  }
}

TEST_CASE("choice and_then", "[choice][and_then]")
{
  using type = fn::choice<bool, int>;
  constexpr auto init = std::in_place_type<int>;

  type s{init, 12};
  CHECK(s.and_then(                                                 //
            fn::overload([](bool) -> fn::choice<bool> { throw 1; }, //
                         [](int &i) -> fn::choice<bool> {           //
                           return fn::choice<bool>{i == 12};
                         },
                         [](int const &) -> fn::choice<bool> { throw 0; },   //
                         [](int &&) -> fn::choice<bool> { throw 0; },        //
                         [](int const &&) -> fn::choice<bool> { throw 0; })) //
        == fn::choice{true});
  CHECK(std::as_const(s).and_then(                                   //
            fn::overload([](bool) -> fn::choice<bool> { throw 1; },  //
                         [](int &) -> fn::choice<bool> { throw 0; }, //
                         [](int const &i) -> fn::choice<bool> {      //
                           return {i == 12};
                         },
                         [](int &&) -> fn::choice<bool> { throw 0; },        //
                         [](int const &&) -> fn::choice<bool> { throw 0; })) //
        == fn::choice{true});
  CHECK(type{init, 12}.and_then(                                           //
            fn::overload([](bool) -> fn::choice<bool> { throw 1; },        //
                         [](int &) -> fn::choice<bool> { throw 0; },       //
                         [](int const &) -> fn::choice<bool> { throw 0; }, //
                         [](int &&i) -> fn::choice<bool> {                 //
                           return {i == 12};
                         },
                         [](int const &&) -> fn::choice<bool> { throw 0; }))
        == fn::choice{true});
  CHECK(std::move(std::as_const(s))
            .and_then(                                                         //
                fn::overload([](bool) -> fn::choice<bool> { throw 1; },        //
                             [](int &) -> fn::choice<bool> { throw 0; },       //
                             [](int const &) -> fn::choice<bool> { throw 0; }, //
                             [](int &&) -> fn::choice<bool> { throw 0; },      //
                             [](int const &&i) -> fn::choice<bool> { return {i == 12}; }))
        == fn::choice{true});

  constexpr type a{std::in_place_type<int>, 42};
  constexpr auto fn = fn::overload([](bool) -> fn::choice<bool> { throw 1; },  //
                                   [](int &) -> fn::choice<bool> { throw 0; }, //
                                   [](int const &i) -> fn::choice<bool> {      //
                                     return {i == 42};
                                   },
                                   [](int &&) -> fn::choice<bool> { throw 0; }, //
                                   [](int const &&) -> fn::choice<bool> { throw 0; });
  static_assert(std::is_same_v<fn::choice<bool>, decltype(a.and_then(fn))>);
  static_assert(a.and_then(fn) == fn::choice{true});
  static_assert(std::move(a).and_then(                                             //
                    fn::overload([](bool) -> fn::choice<bool> { throw 1; },        //
                                 [](int &) -> fn::choice<bool> { throw 0; },       //
                                 [](int const &) -> fn::choice<bool> { throw 0; }, //
                                 [](int &&) -> fn::choice<bool> { throw 0; },      //
                                 [](int const &&i) -> fn::choice<bool> { return {i == 42}; }))
                == fn::choice{true});
}

TEST_CASE("choice transform", "[choice][transform]")
{
  WHEN("size 1")
  {
    using type = fn::choice<bool, int>;
    constexpr auto init = std::in_place_type<int>;
    type s{init, 12};
    CHECK(s.transform(                                      //
              fn::overload([](bool) -> double { throw 1; }, //
                           [](int &i) -> double {           //
                             return i / 8.0;
                           },
                           [](int const &) -> double { throw 0; },   //
                           [](int &&) -> double { throw 0; },        //
                           [](int const &&) -> double { throw 0; })) //
          == fn::choice<double>{1.5});
    CHECK(std::as_const(s).transform(                        //
              fn::overload([](bool) -> double { throw 1; },  //
                           [](int &) -> double { throw 0; }, //
                           [](int const &i) -> double {      //
                             return i / 8.0;
                           },
                           [](int &&) -> double { throw 0; },        //
                           [](int const &&) -> double { throw 0; })) //
          == fn::choice<double>{1.5});
    CHECK(type{init, 12}.transform(                                //
              fn::overload([](bool) -> double { throw 1; },        //
                           [](int &) -> double { throw 0; },       //
                           [](int const &) -> double { throw 0; }, //
                           [](int &&i) -> double {                 //
                             return i / 8.0;
                           },
                           [](int const &&) -> double { throw 0; }))
          == fn::choice<double>{1.5});
    CHECK(std::move(std::as_const(s))
              .transform(                                              //
                  fn::overload([](bool) -> double { throw 1; },        //
                               [](int &) -> double { throw 0; },       //
                               [](int const &) -> double { throw 0; }, //
                               [](int &&) -> double { throw 0; },      //
                               [](int const &&i) -> double { return i / 8.0; }))
          == fn::choice<double>{1.5});

    constexpr type a{std::in_place_type<int>, 42};
    constexpr auto fn = fn::overload([](bool) -> fn::sum<bool> { throw 1; },    //
                                     [](int &) -> fn::sum<double> { throw 0; }, //
                                     [](int const &i) -> fn::sum<double> {      //
                                       return i / 8.0;
                                     },
                                     [](int &&) -> fn::sum<double> { throw 0; }, //
                                     [](int const &&) -> fn::sum<double> { throw 0; });
    static_assert(std::is_same_v<fn::choice<bool, double>, decltype(a.transform(fn))>);
    static_assert(a.transform(fn) == fn::choice<bool, double>{5.25});
    static_assert(std::move(a).transform(                                  //
                      fn::overload([](bool) -> double { throw 1; },        //
                                   [](int &) -> double { throw 0; },       //
                                   [](int const &) -> double { throw 0; }, //
                                   [](int &&) -> double { throw 0; },      //
                                   [](int const &&i) -> double { return i / 8.0; }))
                  == fn::choice<double>{5.25});
  }

  WHEN("size 4")
  {
    using ::fn::choice;
    using ::fn::sum;
    constexpr auto fn1 = [](auto i) noexcept -> std::size_t { return sizeof(i); };
    constexpr auto fn2 = [](auto, auto i) noexcept -> std::size_t { return sizeof(i); };

    using type = choice<double, int, std::string, std::string_view>;
    static_assert(type::size == 4);

    WHEN("element v0 set")
    {
      type a{std::in_place_type<double>, 0.5};
      CHECK(a.data.v0 == 0.5);
      WHEN("value only")
      {
        static_assert(type{0.5}.transform(fn1) == choice{8ul});
        CHECK(a.transform(      //
                  fn::overload( //
                      [](auto) -> int { throw 1; }, [](double &i) -> bool { return i == 0.5; },
                      [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                      [](double const &&) -> bool { throw 0; }))
              == choice<bool, int>{true});
        CHECK(std::as_const(a).transform( //
                  fn::overload(           //
                      [](auto) -> int { throw 1; }, [](double &) -> bool { throw 0; },
                      [](double const &i) -> bool { return i == 0.5; }, [](double &&) -> bool { throw 0; },
                      [](double const &&) -> bool { throw 0; }))
              == choice<bool, int>{true});
        CHECK(type{std::in_place_type<double>, 0.5}.transform( //
                  fn::overload(                                //
                      [](auto) -> int { throw 1; }, [](double &) -> bool { throw 0; },
                      [](double const &) -> bool { throw 0; }, [](double &&i) -> bool { return i == 0.5; },
                      [](double const &&) -> bool { throw 0; }))
              == choice<bool, int>{true});
        CHECK(std::move(std::as_const(a))
                  .transform(       //
                      fn::overload( //
                          [](auto) -> int { throw 1; }, [](double &) -> bool { throw 0; },
                          [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                          [](double const &&i) -> bool { return i == 0.5; }))
              == choice<bool, int>{true});
      }
      WHEN("tag and value")
      {
        static_assert(type{0.5}.transform(fn2) == choice{8ul});
        CHECK(a.transform(      //
                  fn::overload( //
                      [](auto, auto) -> int { throw 1; },
                      [](std::in_place_type_t<double>, double &i) -> bool { return i == 0.5; },
                      [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                      [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                      [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; }))
              == choice<bool, int>{true});
        CHECK(
            std::as_const(a).transform( //
                fn::overload(           //
                    [](auto, auto) -> int { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                    [](std::in_place_type_t<double>, double const &i) -> bool { return i == 0.5; },
                    [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                    [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; }))
            == choice<bool, int>{true});
        CHECK(
            type{std::in_place_type<double>, 0.5}.transform( //
                fn::overload(                                //
                    [](auto, auto) -> int { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                    [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                    [](std::in_place_type_t<double>, double &&i) -> bool { return i == 0.5; },
                    [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; }))
            == choice<bool, int>{true});
        CHECK(std::move(std::as_const(a))
                  .transform(       //
                      fn::overload( //
                          [](auto, auto) -> int { throw 1; },
                          [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                          [](std::in_place_type_t<double>, double const &&i) -> bool { return i == 0.5; }))
              == choice<bool, int>{true});
      }
    }

    WHEN("element v1 set")
    {
      type a{std::in_place_type<int>, 42};
      CHECK(a.data.v1 == 42);

      WHEN("value only")
      {
        static_assert(type{42}.transform(fn1) == choice{4ul});
        CHECK(a.transform(      //
                  fn::overload( //
                      [](auto) -> bool { throw 1; }, [](int &i) -> bool { return i == 42; },
                      [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                      [](int const &&) -> bool { throw 0; }))
              == choice{true});
        CHECK(std::as_const(a).transform( //
                  fn::overload(           //
                      [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                      [](int const &i) -> bool { return i == 42; }, [](int &&) -> bool { throw 0; },
                      [](int const &&) -> bool { throw 0; }))
              == choice{true});
        CHECK(
            choice<int>{std::in_place_type<int>, 42}.transform( //
                fn::overload(                                   //
                    [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                    [](int &&i) -> bool { return i == 42; }, [](int const &&) -> bool { throw 0; }))
            == choice{true});
        CHECK(std::move(std::as_const(a))
                  .transform(       //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                          [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                          [](int const &&i) -> bool { return i == 42; }))
              == choice{true});
      }

      WHEN("tag and value")
      {
        static_assert(type{42}.transform(fn2) == choice{4ul});
        CHECK(a.transform(      //
                  fn::overload( //
                      [](auto, auto) -> bool { throw 1; },
                      [](std::in_place_type_t<int>, int &i) -> bool { return i == 42; },
                      [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                      [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                      [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; }))
              == choice{true});
        CHECK(std::as_const(a).transform( //
                  fn::overload(           //
                      [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                      [](std::in_place_type_t<int>, int const &i) -> bool { return i == 42; },
                      [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                      [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; }))
              == choice{true});
        CHECK(choice<int>{std::in_place_type<int>, 42}.transform( //
                  fn::overload(                                   //
                      [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                      [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                      [](std::in_place_type_t<int>, int &&i) -> bool { return i == 42; },
                      [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; }))
              == choice{true});
        CHECK(
            std::move(std::as_const(a))
                .transform(       //
                    fn::overload( //
                        [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &&i) -> bool { return i == 42; }))
            == choice{true});
      }
    }

    WHEN("element v2 set")
    {
      type a{std::in_place_type<std::string>, "bar"};
      CHECK(a.data.v2 == "bar");
      WHEN("value only")
      {
        // TODO Change single CHECK below to static_assert when supported by Clang
        CHECK(type{std::in_place_type<std::string>, "bar"}.transform(fn1) == choice{32ul});
        CHECK(a.transform(      //
                  fn::overload( //
                      [](auto) -> sum<bool, std::string> { throw 1; },
                      [](std::string &i) -> bool { return i == "bar"; }, [](std::string const &) -> bool { throw 0; },
                      [](std::string &&) -> bool { throw 0; }, [](std::string const &&) -> bool { throw 0; }))
              == choice<bool, std::string>{true});
        CHECK(std::as_const(a).transform( //
                  fn::overload(           //
                      [](auto) -> sum<bool, std::string> { throw 1; }, [](std::string &) -> bool { throw 0; },
                      [](std::string const &i) -> bool { return i == "bar"; }, [](std::string &&) -> bool { throw 0; },
                      [](std::string const &&) -> bool { throw 0; }))
              == choice<bool, std::string>{true});
        CHECK(type{std::in_place_type<std::string>, "bar"}.transform( //
                  fn::overload(                                       //
                      [](auto) -> sum<bool, std::string> { throw 1; }, [](std::string &) -> bool { throw 0; },
                      [](std::string const &) -> bool { throw 0; }, [](std::string &&i) -> bool { return i == "bar"; },
                      [](std::string const &&) -> bool { throw 0; }))
              == choice<bool, std::string>{true});
        CHECK(std::move(std::as_const(a))
                  .transform(       //
                      fn::overload( //
                          [](auto) -> sum<bool, std::string> { throw 1; }, [](std::string &) -> bool { throw 0; },
                          [](std::string const &) -> bool { throw 0; }, [](std::string &&) -> bool { throw 0; },
                          [](std::string const &&i) -> bool { return i == "bar"; }))
              == choice<bool, std::string>{true});
      }
      WHEN("tag and value")
      {
        // TODO Change single CHECK below to static_assert when supported by Clang
        CHECK(type{std::in_place_type<std::string>, "bar"}.transform(fn2) == choice{32ul});
        CHECK(a.transform(      //
                  fn::overload( //
                      [](auto, auto) -> sum<bool, std::string> { throw 1; },
                      [](std::in_place_type_t<std::string>, std::string &i) -> bool { return i == "bar"; },
                      [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                      [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                      [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; }))
              == choice<bool, std::string>{true});
        CHECK(std::as_const(a).transform( //
                  fn::overload(           //
                      [](auto, auto) -> sum<bool, std::string> { throw 1; },
                      [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                      [](std::in_place_type_t<std::string>, std::string const &i) -> bool { return i == "bar"; },
                      [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                      [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; }))
              == choice<bool, std::string>{true});
        CHECK(type{std::in_place_type<std::string>, "bar"}.transform( //
                  fn::overload(                                       //
                      [](auto, auto) -> sum<bool, std::string> { throw 1; },
                      [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                      [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                      [](std::in_place_type_t<std::string>, std::string &&i) -> bool { return i == "bar"; },
                      [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; }))
              == choice<bool, std::string>{true});
        CHECK(std::move(std::as_const(a))
                  .transform(       //
                      fn::overload( //
                          [](auto, auto) -> sum<bool, std::string> { throw 1; },
                          [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                          [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                          [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                          [](std::in_place_type_t<std::string>, std::string const &&i) -> bool { return i == "bar"; }))
              == choice<bool, std::string>{true});
      }
    }

    WHEN("element v3 set")
    {
      type a{std::in_place_type<std::string_view>, "baz"};
      CHECK(a.data.v3 == "baz");
      WHEN("value only")
      {
        static_assert(type{std::in_place_type<std::string_view>, "baz"}.transform(fn1) == choice{16ul});
        CHECK(a.transform(      //
                  fn::overload( //
                      [](auto) -> sum<int, std::string_view> { throw 1; },
                      [](std::string_view &i) -> sum<bool, int> { return {i == "baz"}; },
                      [](std::string_view const &) -> sum<bool, int> { throw 0; },
                      [](std::string_view &&) -> sum<bool, int> { throw 0; },
                      [](std::string_view const &&) -> sum<bool, int> { throw 0; }))
              == choice<bool, int, std::string_view>{true});
        CHECK(std::as_const(a).transform( //
                  fn::overload(           //
                      [](auto) -> sum<int, std::string_view> { throw 1; },
                      [](std::string_view &) -> sum<bool, int> { throw 0; },
                      [](std::string_view const &i) -> sum<bool, int> { return {i == "baz"}; },
                      [](std::string_view &&) -> sum<bool, int> { throw 0; },
                      [](std::string_view const &&) -> sum<bool, int> { throw 0; }))
              == choice<bool, int, std::string_view>{true});
        CHECK(type{std::in_place_type<std::string_view>, "baz"}.transform( //
                  fn::overload(                                            //
                      [](auto) -> sum<int, std::string_view> { throw 1; },
                      [](std::string_view &) -> sum<bool, int> { throw 0; },
                      [](std::string_view const &) -> sum<bool, int> { throw 0; },
                      [](std::string_view &&i) -> sum<bool, int> { return {i == "baz"}; },
                      [](std::string_view const &&) -> sum<bool, int> { throw 0; }))
              == choice<bool, int, std::string_view>{true});
        CHECK(std::move(std::as_const(a))
                  .transform(       //
                      fn::overload( //
                          [](auto) -> sum<int, std::string_view> { throw 1; },
                          [](std::string_view &) -> sum<bool, int> { throw 0; },
                          [](std::string_view const &) -> sum<bool, int> { throw 0; },
                          [](std::string_view &&) -> sum<bool, int> { throw 0; },
                          [](std::string_view const &&i) -> sum<bool, int> { return {i == "baz"}; }))
              == choice<bool, int, std::string_view>{true});
      }
      WHEN("tag and value")
      {
        static_assert(type{std::in_place_type<std::string_view>, "baz"}.transform(fn2) == choice{16ul});
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
            == choice<bool, int, std::string_view>{true});
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
              == choice<bool, int, std::string_view>{true});
        CHECK(
            type{std::in_place_type<std::string_view>, "baz"}.transform( //
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
            == choice<bool, int, std::string_view>{true});
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
            == choice<bool, int, std::string_view>{true});
      }
    }
  }
}
