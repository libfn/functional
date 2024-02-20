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

} // namespace

TEST_CASE("choice non-monadic functionality", "[choice]")
{
  // NOTE This test looks very similar to test of sum in utility.cpp - for good reason.

  using fn::choice;
  using fn::some_in_place_type;

  WHEN("invocable")
  {
    using type = choice<TestType, int>;
    static_assert(type::is_normal);
    static_assert(type::invocable<decltype([](auto) {}), type &>);
    static_assert(type::invocable<decltype([](auto &) {}), type &>);
    static_assert(type::invocable<decltype([](auto const &) {}), type &>);
    static_assert(type::invocable<decltype(fn::overload{[](int &) {}, [](TestType &) {}}), type &>);
    static_assert(type::invocable<decltype(fn::overload{[](int) {}, [](TestType) {}}), type const &>);

    static_assert(not type::invocable<decltype([](TestType &) {}), type &>); // missing int
    static_assert(not type::invocable<decltype([](int &) {}), type &>);      // missing TestType
    static_assert(not type::invocable<decltype(fn::overload{[](int &&) {}, [](TestType &&) {}}),
                                      type &>);                                // cannot bind lvalue to rvalue-reference
    static_assert(not type::invocable<decltype([](auto &) {}), type &&>);      // cannot bind rvalue to lvalue-reference
    static_assert(not type::invocable<decltype([](auto, auto &) {}), type &>); // bad arity
    static_assert(not type::invocable<decltype(fn::overload{[](int &) {}, [](TestType &) {}}),
                                      type const &>); // cannot bind const to non-const reference

    static_assert(choice<NonCopyable>::invocable<decltype([](auto &) {}), NonCopyable &>);
    static_assert(
        not choice<NonCopyable>::invocable<decltype([](auto) {}), NonCopyable &>); // copy-constructor not available

    static_assert(std::is_same_v<type::invoke_result<decltype([](auto) -> int { return 0; }), type &>::type, int>);
    static_assert(std::is_same_v<type::invoke_result_t<decltype([](auto) -> int { return 0; }), type &>, int>);

    static_assert(std::is_same_v<        //
                  type::invoke_result_t< //
                      decltype(          //
                          fn::overload{[](auto &) -> std::integral_constant<int, 0> { return {}; },
                                       [](auto const &) -> std::integral_constant<int, 1> { return {}; },
                                       [](auto &&) -> std::integral_constant<int, 2> { return {}; },
                                       [](auto const &&) -> std::integral_constant<int, 3> { return {}; }}),
                      type &>,
                  std::integral_constant<int, 0>>);
    static_assert(std::is_same_v<        //
                  type::invoke_result_t< //
                      decltype(          //
                          fn::overload{[](auto &) -> std::integral_constant<int, 0> { return {}; },
                                       [](auto const &) -> std::integral_constant<int, 1> { return {}; },
                                       [](auto &&) -> std::integral_constant<int, 2> { return {}; },
                                       [](auto const &&) -> std::integral_constant<int, 3> { return {}; }}),
                      type const &>,
                  std::integral_constant<int, 1>>);
    static_assert(std::is_same_v<        //
                  type::invoke_result_t< //
                      decltype(          //
                          fn::overload{[](auto &) -> std::integral_constant<int, 0> { return {}; },
                                       [](auto const &) -> std::integral_constant<int, 1> { return {}; },
                                       [](auto &&) -> std::integral_constant<int, 2> { return {}; },
                                       [](auto const &&) -> std::integral_constant<int, 3> { return {}; }}),
                      type &&>,
                  std::integral_constant<int, 2>>);
    static_assert(std::is_same_v<        //
                  type::invoke_result_t< //
                      decltype(          //
                          fn::overload{[](auto &) -> std::integral_constant<int, 0> { return {}; },
                                       [](auto const &) -> std::integral_constant<int, 1> { return {}; },
                                       [](auto &&) -> std::integral_constant<int, 2> { return {}; },
                                       [](auto const &&) -> std::integral_constant<int, 3> { return {}; }}),
                      type const &&>,
                  std::integral_constant<int, 3>>);
  }

  WHEN("invocable_typed")
  {
    using type = choice<TestType, int>;
    static_assert(type::is_normal);
    static_assert(type::invocable_typed<decltype([](auto, auto) {}), type &>);
    static_assert(type::invocable_typed<decltype([](some_in_place_type auto, auto) {}), type &>);
    static_assert(type::invocable_typed<decltype([](some_in_place_type auto, auto &) {}), type &>);
    static_assert(type::invocable_typed<decltype(fn::overload{[](some_in_place_type auto, int &) {},
                                                              [](some_in_place_type auto, TestType &) {}}),
                                        type &>);
    static_assert(type::invocable_typed<decltype(fn::overload{[](some_in_place_type auto, int) {},
                                                              [](some_in_place_type auto, TestType) {}}),
                                        type const &>);
    static_assert(
        not type::invocable_typed<decltype([](some_in_place_type auto, TestType &) {}), type &>); // missing int
    static_assert(
        not type::invocable_typed<decltype([](some_in_place_type auto, int &) {}), type &>); // missing TestType
    static_assert(not type::invocable_typed<decltype(fn::overload{[](some_in_place_type auto, int &&) {},
                                                                  [](some_in_place_type auto, TestType &&) {}}),
                                            type &>); // cannot bind lvalue to rvalue-reference
    static_assert(not type::invocable_typed<decltype([](some_in_place_type auto, auto &) {}),
                                            type &&>);                       // cannot bind rvalue to lvalue-reference
    static_assert(not type::invocable_typed<decltype([](auto) {}), type &>); // bad arity
    static_assert(not type::invocable_typed<decltype(fn::overload{[](some_in_place_type auto, int &) {},
                                                                  [](some_in_place_type auto, TestType &) {}}),
                                            type const &>); // cannot bind const to non-const reference

    static_assert(
        choice<NonCopyable>::invocable_typed<decltype([](some_in_place_type auto, auto &) {}), NonCopyable &>);
    static_assert(not choice<NonCopyable>::invocable_typed<decltype([](some_in_place_type auto, auto) {}),
                                                           NonCopyable &>); // copy-constructor not available

    static_assert(
        choice<NonCopyable>::invocable_typed<decltype([](some_in_place_type auto, auto &) {}), NonCopyable &>);
    static_assert(not choice<NonCopyable>::invocable_typed<decltype([](some_in_place_type auto, auto) {}),
                                                           NonCopyable &>); // copy-constructor not available

    static_assert(
        std::is_same_v<type::invoke_result<decltype([](auto, auto) -> int { return 0; }), type &>::type, int>);
    static_assert(std::is_same_v<type::invoke_result_t<decltype([](auto, auto) -> int { return 0; }), type &>, int>);
    static_assert(
        std::is_same_v<            //
            type::invoke_result_t< //
                decltype(          //
                    fn::overload{
                        [](some_in_place_type auto, auto &) -> std::integral_constant<int, 0> { return {}; },
                        [](some_in_place_type auto, auto const &) -> std::integral_constant<int, 1> { return {}; },
                        [](some_in_place_type auto, auto &&) -> std::integral_constant<int, 2> { return {}; },
                        [](some_in_place_type auto, auto const &&) -> std::integral_constant<int, 3> { return {}; }}),
                type &>,
            std::integral_constant<int, 0>>);
    static_assert(
        std::is_same_v<            //
            type::invoke_result_t< //
                decltype(          //
                    fn::overload{
                        [](some_in_place_type auto, auto &) -> std::integral_constant<int, 0> { return {}; },
                        [](some_in_place_type auto, auto const &) -> std::integral_constant<int, 1> { return {}; },
                        [](some_in_place_type auto, auto &&) -> std::integral_constant<int, 2> { return {}; },
                        [](some_in_place_type auto, auto const &&) -> std::integral_constant<int, 3> { return {}; }}),
                type const &>,
            std::integral_constant<int, 1>>);
    static_assert(
        std::is_same_v<            //
            type::invoke_result_t< //
                decltype(          //
                    fn::overload{
                        [](some_in_place_type auto, auto &) -> std::integral_constant<int, 0> { return {}; },
                        [](some_in_place_type auto, auto const &) -> std::integral_constant<int, 1> { return {}; },
                        [](some_in_place_type auto, auto &&) -> std::integral_constant<int, 2> { return {}; },
                        [](some_in_place_type auto, auto const &&) -> std::integral_constant<int, 3> { return {}; }}),
                type &&>,
            std::integral_constant<int, 2>>);
    static_assert(
        std::is_same_v<            //
            type::invoke_result_t< //
                decltype(          //
                    fn::overload{
                        [](some_in_place_type auto, auto &) -> std::integral_constant<int, 0> { return {}; },
                        [](some_in_place_type auto, auto const &) -> std::integral_constant<int, 1> { return {}; },
                        [](some_in_place_type auto, auto &&) -> std::integral_constant<int, 2> { return {}; },
                        [](some_in_place_type auto, auto const &&) -> std::integral_constant<int, 3> { return {}; }}),
                type const &&>,
            std::integral_constant<int, 3>>);
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
    static_assert(a == 12);

    constexpr choice<bool> b{false};
    static_assert(b == false);

    WHEN("CTAD")
    {
      choice a{42};
      static_assert(std::is_same_v<decltype(a), choice<int>>);
      CHECK(a == 42);

      constexpr choice b{false};
      static_assert(std::is_same_v<decltype(b), choice<bool> const>);
      static_assert(b == false);
    }
  }

  WHEN("forwarding constructors (immovable)")
  {
    choice<NonCopyable> a{std::in_place_type<NonCopyable>, 42};
    CHECK(a.invoke([](auto &i) -> bool { return i.i == 42; }));

    WHEN("CTAD")
    {
      constexpr auto a = choice{std::in_place_type<NonCopyable>, 42};
      static_assert(std::is_same_v<decltype(a), choice<NonCopyable> const>);

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
      CHECK(a.invoke([](auto &i) -> bool {
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
      static_assert(a.invoke([](auto &i) -> bool {
        return std::same_as<std::array<int, 3> const &, decltype(i)> && i.size() == 3 && i[0] == 1 && i[1] == 2
               && i[2] == 3;
      }));
    }

    WHEN("CTAD")
    {
      constexpr auto a = choice{std::in_place_type<std::array<int, 3> const>, 1, 2, 3};
      static_assert(std::is_same_v<decltype(a), choice<std::array<int, 3> const> const>);

      auto b = choice{std::in_place_type<std::array<int, 3>>, 1, 2, 3};
      static_assert(std::is_same_v<decltype(b), choice<std::array<int, 3>>>);
    }
  }

  WHEN("constructor is_normal clause")
  {
    using type = choice<int, bool>;
    static_assert(not type::is_normal);
    static_assert([](auto a) constexpr -> bool {
      return not requires { type{std::in_place_type<std::remove_cvref_t<decltype(a)>>, a}; };
    }(1)); // constructor not available
    static_assert([](auto a) constexpr -> bool {
      return requires { type::make(std::in_place_type<std::remove_cvref_t<decltype(a)>>, a); };
    }(1)); // make is available
    using normal_type = choice<bool, int>;
    static_assert(normal_type::is_normal);
    static_assert(std::same_as<normal_type, decltype(type::make(std::in_place_type<int>, 0))>);
    static_assert(normal_type::has_type<int>);
    static_assert(normal_type::has_type<bool>);
  }

  WHEN("has_type type mismatch")
  {
    using type = choice<bool, int>;
    static_assert(type::has_type<int>);
    static_assert(type::has_type<bool>);
    static_assert(not type::has_type<double>);
    auto a = type::make(std::in_place_type<int>, 42);
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
    CHECK(a == 42);
    CHECK(a != 41);
    CHECK(a != false);

    CHECK(a == type::make(std::in_place_type<int>, 42));
    CHECK(a != type::make(std::in_place_type<int>, 41));
    CHECK(a != type::make(std::in_place_type<bool>, true));
    CHECK(not(a == type::make(std::in_place_type<int>, 41)));
    CHECK(not(a == type::make(std::in_place_type<bool>, true)));

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
      static_assert(a == type::make(std::in_place_type<int>, 42));
      static_assert(a != type::make(std::in_place_type<int>, 41));
      static_assert(a != type::make(std::in_place_type<bool>, true));
      static_assert(not(a == type::make(std::in_place_type<int>, 41)));
      static_assert(not(a == type::make(std::in_place_type<bool>, true)));

      static_assert([](auto const &a) constexpr -> bool { //
        return not requires { a == 0.5; };
      }(a));                                              // double is not a type member
      static_assert([](auto const &a) constexpr -> bool { //
        return not requires { a != 0.5; };
      }(a));                                              // double is not a type member
      static_assert([](auto const &a) constexpr -> bool { //
        return not requires { a == choice(std::in_place_type<int>, 1); };
      }(a));                                              // type mismatch choice<int>
      static_assert([](auto const &a) constexpr -> bool { //
        return not requires { a != choice(std::in_place_type<int>, 1); };
      }(a)); // type mismatch choice<int>
    }
  }

  WHEN("constructor make_from")
  {
    using type = choice<bool, int>;
    static_assert(type::has_type<int>);
    static_assert(type::has_type<bool>);
    static_assert(not type::has_type<double>);
    static_assert(type::is_normal);

    WHEN("from smaller choice<bool>")
    {
      constexpr auto init = choice<bool>{std::in_place_type<bool>, true};
      static_assert([](auto &a) constexpr -> bool { return requires { type::make_from(a); }; }(init));
      auto a = type::make_from(init);
      static_assert(std::same_as<type, decltype(a)>);
      CHECK(a.has_value(std::in_place_type<bool>));
      CHECK(a.template has_value<bool>());
      CHECK(a.invoke([](auto &i) -> bool { return i != 0; }));

      WHEN("constexpr")
      {
        constexpr auto a = type::make_from(init);
        static_assert(std::same_as<type const, decltype(a)>);
        static_assert(a.has_value(std::in_place_type<bool>));
        static_assert(a.template has_value<bool>());
        static_assert(a.invoke([](auto &i) -> bool { return i != 0; }));
      }
    }

    WHEN("from smaller choice<int>")
    {
      constexpr auto init = choice<int>{std::in_place_type<int>, 42};
      static_assert([](auto &a) constexpr -> bool { return requires { type::make_from(a); }; }(init));
      auto a = type::make_from(init);
      static_assert(std::same_as<type, decltype(a)>);
      CHECK(a.has_value(std::in_place_type<int>));
      CHECK(a.template has_value<int>());
      CHECK(a.invoke([](auto &i) -> bool { return i != 0; }));

      WHEN("constexpr")
      {
        constexpr auto a = type::make_from(init);
        static_assert(std::same_as<type const, decltype(a)>);
        static_assert(a.has_value(std::in_place_type<int>));
        static_assert(a.template has_value<int>());
        static_assert(a.invoke([](auto &i) -> bool { return i != 0; }));
      }
    }

    WHEN("same choice")
    {
      constexpr auto init = type{std::in_place_type<int>, 42};
      static_assert([](auto &a) constexpr -> bool { return requires { type::make_from(a); }; }(init));
      auto a = type::make_from(init);
      static_assert(std::same_as<type, decltype(a)>);
      CHECK(a.has_value(std::in_place_type<int>));
      CHECK(a.template has_value<int>());
      CHECK(a.invoke([](auto &i) -> bool { return i != 0; }));

      WHEN("constexpr")
      {
        constexpr auto a = type::make_from(init);
        static_assert(std::same_as<type const, decltype(a)>);
        static_assert(a.has_value(std::in_place_type<int>));
        static_assert(a.template has_value<int>());
        static_assert(a.invoke([](auto &i) -> bool { return i != 0; }));
      }
    }

    WHEN("choice type mismatch")
    {
      constexpr auto init = choice<bool, double, int>{std::in_place_type<int>, 42};
      static_assert([](auto &a) constexpr -> bool {
        return not requires { type::make_from(a); };
      }(init)); // type is not a superset of init
    }
  }

  WHEN("invoke")
  {
    choice<int> a{std::in_place_type<int>, 42};
    WHEN("value only")
    {
      CHECK(a.invoke(fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { return true; },
                                  [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                  [](int const &&) -> bool { throw 0; })));
      CHECK(std::as_const(a).invoke(fn::overload(
          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { return true; },
          [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; })));
      CHECK(choice<int>{std::in_place_type<int>, 42}.invoke(fn::overload(
          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
          [](int &&) -> bool { return true; }, [](int const &&) -> bool { throw 0; })));
      CHECK(std::move(std::as_const(a))
                .invoke(fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                     [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                     [](int const &&) -> bool { return true; })));

      WHEN("constexpr")
      {
        constexpr choice<int> a{std::in_place_type<int>, 42};
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
      CHECK(choice<int>{std::in_place_type<int>, 42}.invoke(
          fn::overload([](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                       [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                       [](std::in_place_type_t<int>, int &&) -> bool { return true; },
                       [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
      CHECK(std::move(std::as_const(a))
                .invoke(fn::overload([](auto, auto) -> bool { throw 1; },
                                     [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                                     [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                                     [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                     [](std::in_place_type_t<int>, int const &&) -> bool { return true; })));
      WHEN("constexpr")
      {
        constexpr choice<int> a{std::in_place_type<int>, 42};
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
        == true);
  CHECK(std::as_const(s).and_then(                                   //
            fn::overload([](bool) -> fn::choice<bool> { throw 1; },  //
                         [](int &) -> fn::choice<bool> { throw 0; }, //
                         [](int const &i) -> fn::choice<bool> {      //
                           return {i == 12};
                         },
                         [](int &&) -> fn::choice<bool> { throw 0; },        //
                         [](int const &&) -> fn::choice<bool> { throw 0; })) //
        == true);
  CHECK(type{init, 12}.and_then(                                           //
            fn::overload([](bool) -> fn::choice<bool> { throw 1; },        //
                         [](int &) -> fn::choice<bool> { throw 0; },       //
                         [](int const &) -> fn::choice<bool> { throw 0; }, //
                         [](int &&i) -> fn::choice<bool> {                 //
                           return {i == 12};
                         },
                         [](int const &&) -> fn::choice<bool> { throw 0; }))
        == true);
  CHECK(std::move(std::as_const(s))
            .and_then(                                                         //
                fn::overload([](bool) -> fn::choice<bool> { throw 1; },        //
                             [](int &) -> fn::choice<bool> { throw 0; },       //
                             [](int const &) -> fn::choice<bool> { throw 0; }, //
                             [](int &&) -> fn::choice<bool> { throw 0; },      //
                             [](int const &&i) -> fn::choice<bool> { return {i == 12}; }))
        == true);

  constexpr type a{std::in_place_type<int>, 42};
  constexpr auto fn = fn::overload([](bool) -> fn::choice<bool> { throw 1; },  //
                                   [](int &) -> fn::choice<bool> { throw 0; }, //
                                   [](int const &i) -> fn::choice<bool> {      //
                                     return {i == 42};
                                   },
                                   [](int &&) -> fn::choice<bool> { throw 0; }, //
                                   [](int const &&) -> fn::choice<bool> { throw 0; });
  static_assert(std::is_same_v<fn::choice<bool>, decltype(a.and_then(fn))>);
  static_assert(a.and_then(fn) == true);
  static_assert(std::move(a).and_then(                                             //
                    fn::overload([](bool) -> fn::choice<bool> { throw 1; },        //
                                 [](int &) -> fn::choice<bool> { throw 0; },       //
                                 [](int const &) -> fn::choice<bool> { throw 0; }, //
                                 [](int &&) -> fn::choice<bool> { throw 0; },      //
                                 [](int const &&i) -> fn::choice<bool> { return {i == 42}; }))
                == true);
}

TEST_CASE("choice transform", "[choice][transform]")
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
        == 1.5);
  CHECK(std::as_const(s).transform(                        //
            fn::overload([](bool) -> double { throw 1; },  //
                         [](int &) -> double { throw 0; }, //
                         [](int const &i) -> double {      //
                           return i / 8.0;
                         },
                         [](int &&) -> double { throw 0; },        //
                         [](int const &&) -> double { throw 0; })) //
        == 1.5);
  CHECK(type{init, 12}.transform(                                //
            fn::overload([](bool) -> double { throw 1; },        //
                         [](int &) -> double { throw 0; },       //
                         [](int const &) -> double { throw 0; }, //
                         [](int &&i) -> double {                 //
                           return i / 8.0;
                         },
                         [](int const &&) -> double { throw 0; }))
        == 1.5);
  CHECK(std::move(std::as_const(s))
            .transform(                                              //
                fn::overload([](bool) -> double { throw 1; },        //
                             [](int &) -> double { throw 0; },       //
                             [](int const &) -> double { throw 0; }, //
                             [](int &&) -> double { throw 0; },      //
                             [](int const &&i) -> double { return i / 8.0; }))
        == 1.5);

  constexpr type a{std::in_place_type<int>, 42};
  constexpr auto fn = fn::overload([](bool) -> double { throw 1; },  //
                                   [](int &) -> double { throw 0; }, //
                                   [](int const &i) -> double {      //
                                     return i / 8.0;
                                   },
                                   [](int &&) -> double { throw 0; }, //
                                   [](int const &&) -> double { throw 0; });
  static_assert(std::is_same_v<fn::choice<bool, double, int>, decltype(a.transform(fn))>);
  static_assert(a.transform(fn) == 5.25);
  static_assert(std::move(a).transform(                                  //
                    fn::overload([](bool) -> double { throw 1; },        //
                                 [](int &) -> double { throw 0; },       //
                                 [](int const &) -> double { throw 0; }, //
                                 [](int &&) -> double { throw 0; },      //
                                 [](int const &&i) -> double { return i / 8.0; }))
                == 5.25);
}
