// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/utility.hpp"
#include "static_check.hpp"

#include <catch2/catch_all.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

using namespace util;

namespace fn {
// clang-format off
static_assert(std::is_same_v<as_value_t<int>,          int>);
static_assert(std::is_same_v<as_value_t<int const>,    int const>);
static_assert(std::is_same_v<as_value_t<int &&>,       int>);
static_assert(std::is_same_v<as_value_t<int const &&>, int const>);
static_assert(std::is_same_v<as_value_t<int &>,        int &>);
static_assert(std::is_same_v<as_value_t<int const &>,  int const &>);

static_assert(std::is_same_v<as_value_t<std::nullopt_t>,          std::nullopt_t>);
static_assert(std::is_same_v<as_value_t<std::nullopt_t const>,    std::nullopt_t const>);
static_assert(std::is_same_v<as_value_t<std::nullopt_t &>,        std::nullopt_t>);
static_assert(std::is_same_v<as_value_t<std::nullopt_t const &>,  std::nullopt_t const>);
static_assert(std::is_same_v<as_value_t<std::nullopt_t &&>,       std::nullopt_t>);
static_assert(std::is_same_v<as_value_t<std::nullopt_t const &&>, std::nullopt_t const>);

static_assert(std::is_same_v<apply_const_lvalue_t<float,          int>,    int>);
static_assert(std::is_same_v<apply_const_lvalue_t<float const,    int>,    int const>);
static_assert(std::is_same_v<apply_const_lvalue_t<float,          int &>,  int &>);
static_assert(std::is_same_v<apply_const_lvalue_t<float const,    int &>,  int const &>);
static_assert(std::is_same_v<apply_const_lvalue_t<float,          int &&>, int &&>);
static_assert(std::is_same_v<apply_const_lvalue_t<float const,    int &&>, int const &&>);

static_assert(std::is_same_v<apply_const_lvalue_t<float &,        int>,    int&>);
static_assert(std::is_same_v<apply_const_lvalue_t<float const &,  int>,    int const&>);
static_assert(std::is_same_v<apply_const_lvalue_t<float &,        int &>,  int &>);
static_assert(std::is_same_v<apply_const_lvalue_t<float const &,  int &>,  int const &>);
static_assert(std::is_same_v<apply_const_lvalue_t<float &,        int &&>, int &>);
static_assert(std::is_same_v<apply_const_lvalue_t<float const &,  int &&>, int const &>);

static_assert(std::is_same_v<apply_const_lvalue_t<float &&,       int>,    int>);
static_assert(std::is_same_v<apply_const_lvalue_t<float const &&, int>,    int const>);
static_assert(std::is_same_v<apply_const_lvalue_t<float &&,       int &>,  int &>);
static_assert(std::is_same_v<apply_const_lvalue_t<float const &&, int &>,  int const &>);
static_assert(std::is_same_v<apply_const_lvalue_t<float &&,       int &&>, int &&>);
static_assert(std::is_same_v<apply_const_lvalue_t<float const &&, int &&>, int const &&>);

static_assert(std::is_same_v<decltype(apply_const_lvalue<float>         (std::declval<int>())),    int &&>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float const>   (std::declval<int>())),    int const &&>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float>         (std::declval<int &>())),  int &>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float const>   (std::declval<int &>())),  int const &>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float>         (std::declval<int &&>())), int &&>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float const>   (std::declval<int &&>())), int const &&>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float &>       (std::declval<int>())),    int &>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float const &> (std::declval<int>())),    int const &>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float &>       (std::declval<int &>())),  int &>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float const &> (std::declval<int &>())),  int const &>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float &>       (std::declval<int &&>())), int &>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float const &> (std::declval<int &&>())), int const &>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float &&>      (std::declval<int>())),    int &&>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float const &&>(std::declval<int>())),    int const &&>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float &&>      (std::declval<int &>())),  int &>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float const &&>(std::declval<int &>())),  int const &>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float &&>      (std::declval<int &&>())), int &&>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float const &&>(std::declval<int &&>())), int const &&>);
// clang-format on
} // namespace fn

namespace {
struct A final {
  int v = 0;
};
} // namespace

TEST_CASE("pack", "[pack]")
{
  using fn::pack;

  using T = pack<int, int const, int &, int const &>;
  int val1 = 15;
  int const val2 = 92;
  T v{3, 14, val1, val2};
  CHECK(v.size == 4);

  using is = static_check::bind<decltype([](auto &&fn) constexpr {
    return requires { std::declval<T>().invoke(fn, A{}); };
  })>;

  static_assert(is::invocable([](auto &&...) {}));            // generic call
  static_assert(is::invocable([](A, auto &&...) {}));         // also generic call
  static_assert(is::invocable([](A, int, int, int, int) {})); // pass everything by value
  static_assert(is::invocable(
      [](A const &, int const &, int const &, int const &, int const &) {})); // pass everything by const reference
  static_assert(is::invocable([](A, int, int, int &, int const &) {}));       // bind lvalues
  static_assert(is::invocable([](A &&, int &&, int const &&, int &, int const &) {})); // bind rvalues and lvalues
  static_assert(is::invocable(
      [](A const, int const, int const, int const &, int const &) {})); // pass values or lvalues promoted to const

  static_assert(is::not_invocable([](A &, auto &&...) {}));          // cannot bind rvalue to lvalue reference
  static_assert(is::not_invocable([](A, int &, auto &&...) {}));     // cannot bind rvalue to lvalue reference
  static_assert(is::not_invocable([](A, int, int, int &&, int) {})); // cannot bind lvalue to rvalue reference
  static_assert(is::not_invocable([](A, int, int, int, int &&) {})); // cannot bind const lvalue to rvalue reference
  static_assert(
      is::not_invocable([](A, int, int, int, int const &&) {})); // cannot bind const lvalue to const rvalue reference
  static_assert(
      is::not_invocable([](A &&, int, int &&, int, int) {})); // cannot bind const rvalue to non-const rvalue reference
  static_assert(is::not_invocable([](int, auto &&...) {}));   // bad type
  static_assert(is::not_invocable([](auto, auto, auto, auto) {}));             // bad arity
  static_assert(is::not_invocable([](auto, auto, auto, auto, auto, auto) {})); // bad arity

  CHECK(v.invoke([](auto... args) noexcept -> int { return (0 + ... + args); }) == 3 + 14 + 15 + 92);
  CHECK(v.invoke([](A, auto... args) noexcept -> int { return (0 + ... + args); }, A{}) == 3 + 14 + 15 + 92);

  A a;
  constexpr auto fn1 = [](A &dest, auto... args) noexcept -> A & {
    dest.v = (0 + ... + args);
    return dest;
  };
  CHECK(v.invoke(fn1, a).v == 3 + 14 + 15 + 92);
  static_assert(std::is_same_v<decltype(v.invoke(fn1, a)), A &>);

  constexpr auto fn2 = [](A &&dest, auto &&...) noexcept -> A && { return std::move(dest); };
  static_assert(std::is_same_v<decltype(v.invoke(fn2, std::move(a))), A &&>);

  constexpr auto fn3 = [](A &&dest, auto &&...) noexcept -> A { return dest; };
  static_assert(std::is_same_v<decltype(v.invoke(fn3, std::move(a))), A>);

  static_assert(pack<>::size == 0);

  static_assert(std::same_as<decltype(fn::pack{}), fn::pack<>>);
  static_assert(std::same_as<decltype(fn::pack{12}), fn::pack<int>>);
  static_assert(std::same_as<decltype(fn::pack{a}), fn::pack<A &>>);
  static_assert(std::same_as<decltype(fn::pack{12, a}), fn::pack<int, A &>>);
  static_assert(std::same_as<decltype(fn::pack{12, std::as_const(a)}), fn::pack<int, A const &>>);
  static_assert(std::same_as<decltype(fn::pack{12, std::move(a)}), fn::pack<int, A>>);
}

TEST_CASE("append value categories", "[pack][append]")
{
  using fn::pack;

  struct B {
    constexpr explicit B(int i) : v(i) {}
    constexpr B(int i, int j) : v(i * j) {}

    int v = 0;
  };

  struct C final : B {
    constexpr C() : B(30) {}
  };

  using T = pack<int, std::string_view, A>;
  T s{12, "bar", 42};

  static_assert(T::size == 3);
  constexpr auto check = [](int i, std::string_view s, A a, B const &b) noexcept -> bool {
    return i == 12 && s == std::string("bar") && a.v == 42 && b.v == 30;
  };

  WHEN("explicit type selection")
  {
    static_assert(std::same_as<decltype(s.append(std::in_place_type<B>, 5, 6)), T::append_type<B>>);
    static_assert(std::same_as<T::append_type<B>, pack<int, std::string_view, A, B>>);
    static_assert(decltype(s.append(std::in_place_type<B>, 5, 6))::size == 4);

    constexpr C c1{};
    static_assert(std::same_as<decltype(s.append(std::in_place_type<B const &>, c1)), T::append_type<B const &>>);
    static_assert(std::same_as<T::append_type<B const &>, pack<int, std::string_view, A, B const &>>);

    C c2{};
    static_assert(std::same_as<decltype(s.append(std::in_place_type<B &>, c2)), T::append_type<B &>>);
    static_assert(std::same_as<T::append_type<B &>, pack<int, std::string_view, A, B &>>);

    WHEN("constructor takes parameters")
    {
      CHECK(s.append(std::in_place_type<B>, 5, 6).invoke(check));
      CHECK(std::as_const(s).append(std::in_place_type<B>, 5, 6).invoke(check));
      CHECK(T{12, "bar", 42}.append(std::in_place_type<B>, 5, 6).invoke(check));
      CHECK(std::move(std::as_const(s)).append(std::in_place_type<B>, 5, 6).invoke(check));
    }

    WHEN("default constructor")
    {
      CHECK(s.append(std::in_place_type<C>).invoke(check));
      CHECK(std::as_const(s).append(std::in_place_type<C>).invoke(check));
      CHECK(T{12, "bar", 42}.append(std::in_place_type<C>).invoke(check));
      CHECK(std::move(std::as_const(s)).append(std::in_place_type<C>).invoke(check));
    }
  }

  WHEN("deduced type")
  {
    static_assert(std::same_as<decltype(s.append(B{5, 6})), T::append_type<B>>);

    constexpr C c1{};
    static_assert(std::same_as<decltype(s.append(c1)), T::append_type<C const &>>);
    static_assert(std::same_as<T::append_type<C const &>, pack<int, std::string_view, A, C const &>>);

    C c2{};
    static_assert(std::same_as<decltype(s.append(c2)), T::append_type<C &>>);
    static_assert(std::same_as<T::append_type<C &>, pack<int, std::string_view, A, C &>>);

    CHECK(s.append(B{30}).invoke(check));
    CHECK(std::as_const(s).append(B{30}).invoke(check));
    CHECK(T{12, "bar", 42}.append(B{30}).invoke(check));
    CHECK(std::move(std::as_const(s)).append(B{30}).invoke(check));
  }
}

TEST_CASE("pack with immovable data", "[pack][immovable]")
{
  using fn::pack;

  struct ImmovableType {
    int value = 0;

    constexpr explicit ImmovableType(int i) noexcept : value(i) {}

    ImmovableType(ImmovableType const &) = delete;
    ImmovableType &operator=(ImmovableType const &) = delete;
    ImmovableType(ImmovableType &&) = delete;
    ImmovableType &operator=(ImmovableType &&) = delete;

    constexpr bool operator==(ImmovableType const &other) const noexcept { return value == other.value; }
  };

  using T = pack<ImmovableType, ImmovableType const, ImmovableType &, ImmovableType const &>;
  ImmovableType val1{15};
  ImmovableType const val2{92};
  T v{ImmovableType{3}, ImmovableType{14}, val1, val2};

  using is
      = static_check::bind<decltype([](auto &&fn) constexpr { return requires { std::declval<T>().invoke(fn); }; })>;

  static_assert(is::invocable([](auto &&...) {})); // generic call
  static_assert(is::invocable([](ImmovableType const &, ImmovableType const &, ImmovableType const &,
                                 ImmovableType const &) {})); // pass everything by const reference
  static_assert(is::invocable([](ImmovableType &&, ImmovableType const &&, ImmovableType &, ImmovableType const &) {
  }));                                                                // bind rvalues and lvalues
  static_assert(is::not_invocable([](ImmovableType, auto &&...) {})); // cannot pass immovable by value

  CHECK(v.invoke([](auto &&...args) noexcept -> int { return (0 + ... + args.value); }) == 3 + 14 + 15 + 92);
}

TEST_CASE("constexpr pack", "[pack][constexpr]")
{
  constexpr fn::pack<int, int> v2{3, 14};
  constexpr auto r2 = v2.invoke([](auto &&...args) constexpr noexcept -> int { return (0 + ... + args); });
  static_assert(r2 == 3 + 14);
  SUCCEED();
}

TEST_CASE("overload", "[overload]")
{
  using is = static_check::bind<decltype([](auto &&fn) constexpr {
    return requires {
      fn();
      fn(1);
    };
  })>;

  // example use
  static_assert(is::invocable(fn::overload{[](auto...) {}}));       // generic
  static_assert(is::invocable(fn::overload{[]() {}, [](auto) {}})); // generic with fixed arity
  static_assert(is::invocable(fn::overload{[]() {}, [](int) {}}));  // fixed type
  static_assert(is::invocable(fn::overload{[]() {}, [](bool) {}})); // built-in conversion
  static_assert(is::invocable(
      fn::overload{[]() -> void {}, [](int i) -> std::string { return std::to_string(i); }})); // different return types

  static_assert(is::not_invocable(fn::overload{[]() {}}));    // missing int overload
  static_assert(is::not_invocable(fn::overload{[](int) {}})); // missing void overload

  struct A {
    constexpr auto operator()(int i) const noexcept -> int { return i + 1; }
  };

  // check template deduction guides
  static_assert(std::same_as<decltype(fn::overload{std::declval<A>()}), fn::overload<A>>);
  static_assert(std::same_as<decltype(fn::overload{std::declval<A &>()}), fn::overload<A>>);
  static_assert(std::same_as<decltype(fn::overload{std::declval<A &&>()}), fn::overload<A>>);
  static_assert(std::same_as<decltype(fn::overload{std::declval<A const>()}), fn::overload<A>>);
  static_assert(std::same_as<decltype(fn::overload{std::declval<A const &>()}), fn::overload<A>>);
  static_assert(std::same_as<decltype(fn::overload{std::declval<A const &&>()}), fn::overload<A>>);

  A a1 = {};
  CHECK(fn::overload{a1}(1) == 2);
  constexpr A a2 = {};
  static_assert(fn::overload{a2}(2) == 3);
  static_assert(fn::overload{A{}}(3) == 4);
}

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

TEST_CASE("sum", "[sum]")
{
  using fn::some_in_place_type;
  using fn::sum;

  WHEN("invocable")
  {
    using type = sum<TestType, int>;
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

    static_assert(sum<NonCopyable>::invocable<decltype([](auto &) {}), sum<NonCopyable> &>);
    static_assert(
        not sum<NonCopyable>::invocable<decltype([](auto) {}), NonCopyable &>); // copy-constructor not available

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

  WHEN("type_invocable")
  {
    using type = sum<TestType, int>;
    static_assert(type::is_normal);
    static_assert(type::type_invocable<decltype([](auto, auto) {}), type &>);
    static_assert(type::type_invocable<decltype([](some_in_place_type auto, auto) {}), type &>);
    static_assert(type::type_invocable<decltype([](some_in_place_type auto, auto &) {}), type &>);
    static_assert(type::type_invocable<decltype(fn::overload{[](some_in_place_type auto, int &) {},
                                                             [](some_in_place_type auto, TestType &) {}}),
                                       type &>);
    static_assert(type::type_invocable<decltype(fn::overload{[](some_in_place_type auto, int) {},
                                                             [](some_in_place_type auto, TestType) {}}),
                                       type const &>);
    static_assert(
        not type::type_invocable<decltype([](some_in_place_type auto, TestType &) {}), type &>); // missing int
    static_assert(
        not type::type_invocable<decltype([](some_in_place_type auto, int &) {}), type &>); // missing TestType
    static_assert(not type::type_invocable<decltype(fn::overload{[](some_in_place_type auto, int &&) {},
                                                                 [](some_in_place_type auto, TestType &&) {}}),
                                           type &>); // cannot bind lvalue to rvalue-reference
    static_assert(not type::type_invocable<decltype([](some_in_place_type auto, auto &) {}),
                                           type &&>);                       // cannot bind rvalue to lvalue-reference
    static_assert(not type::type_invocable<decltype([](auto) {}), type &>); // bad arity
    static_assert(not type::type_invocable<decltype(fn::overload{[](some_in_place_type auto, int &) {},
                                                                 [](some_in_place_type auto, TestType &) {}}),
                                           type const &>); // cannot bind const to non-const reference

    static_assert(
        sum<NonCopyable>::type_invocable<decltype([](some_in_place_type auto, auto &) {}), sum<NonCopyable> &>);
    static_assert(not sum<NonCopyable>::type_invocable<decltype([](some_in_place_type auto, auto) {}),
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
    static_assert(a == 12);

    constexpr sum<bool> b{false};
    static_assert(b == false);

    WHEN("CTAD")
    {
      sum a{42};
      static_assert(std::is_same_v<decltype(a), sum<int>>);
      CHECK(a == 42);

      constexpr sum b{false};
      static_assert(std::is_same_v<decltype(b), sum<bool> const>);
      static_assert(b == false);
    }
  }

  WHEN("forwarding constructors (immovable)")
  {
    sum<NonCopyable> a{std::in_place_type<NonCopyable>, 42};
    CHECK(a.invoke([](auto &i) -> bool { return i.i == 42; }));

    WHEN("CTAD")
    {
      constexpr auto a = sum{std::in_place_type<NonCopyable>, 42};
      static_assert(std::is_same_v<decltype(a), sum<NonCopyable> const>);

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
      constexpr auto a = sum{std::in_place_type<std::array<int, 3> const>, 1, 2, 3};
      static_assert(std::is_same_v<decltype(a), sum<std::array<int, 3> const> const>);

      auto b = sum{std::in_place_type<std::array<int, 3>>, 1, 2, 3};
      static_assert(std::is_same_v<decltype(b), sum<std::array<int, 3>>>);
    }
  }

  WHEN("constructor is_normal clause")
  {
    using type = sum<int, bool>;
    static_assert(not type::is_normal);
    static_assert([](auto a) constexpr -> bool {
      return not requires { type{std::in_place_type<std::remove_cvref_t<decltype(a)>>, a}; };
    }(1)); // constructor not available
    static_assert([](auto a) constexpr -> bool {
      return requires { type::make(std::in_place_type<std::remove_cvref_t<decltype(a)>>, a); };
    }(1)); // make is available
    using normal_type = sum<bool, int>;
    static_assert(normal_type::is_normal);
    static_assert(std::same_as<normal_type, decltype(type::make(std::in_place_type<int>, 0))>);
    static_assert(normal_type::has_type<int>);
    static_assert(normal_type::has_type<bool>);
  }

  WHEN("has_type type mismatch")
  {
    using type = sum<bool, int>;
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
    using type = sum<bool, int>;

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
        return not requires { a == sum(std::in_place_type<int>, 1); };
      }(a));                                              // type mismatch sum<int>
      static_assert([](auto const &a) constexpr -> bool { //
        return not requires { a != sum(std::in_place_type<int>, 1); };
      }(a)); // type mismatch sum<int>
    }
  }

  WHEN("constructor make")
  {
    using type = sum<bool, int>;
    static_assert(type::has_type<int>);
    static_assert(type::has_type<bool>);
    static_assert(not type::has_type<double>);
    static_assert(type::is_normal);

    WHEN("from smaller sum<bool>")
    {
      constexpr auto init = sum<bool>{std::in_place_type<bool>, true};
      static_assert([](auto &a) constexpr -> bool { return requires { type::make(a); }; }(init));
      auto a = type::make(init);
      static_assert(std::same_as<type, decltype(a)>);
      CHECK(a.has_value(std::in_place_type<bool>));
      CHECK(a.template has_value<bool>());
      CHECK(a.invoke([](auto &i) -> bool { return i != 0; }));

      WHEN("constexpr")
      {
        constexpr auto a = type::make(init);
        static_assert(std::same_as<type const, decltype(a)>);
        static_assert(a.has_value(std::in_place_type<bool>));
        static_assert(a.template has_value<bool>());
        static_assert(a.invoke([](auto &i) -> bool { return i != 0; }));
      }
    }

    WHEN("from smaller sum<int>")
    {
      constexpr auto init = sum<int>{std::in_place_type<int>, 42};
      static_assert([](auto &a) constexpr -> bool { return requires { type::make(a); }; }(init));
      auto a = type::make(init);
      static_assert(std::same_as<type, decltype(a)>);
      CHECK(a.has_value(std::in_place_type<int>));
      CHECK(a.template has_value<int>());
      CHECK(a.invoke([](auto &i) -> bool { return i != 0; }));

      WHEN("constexpr")
      {
        constexpr auto a = type::make(init);
        static_assert(std::same_as<type const, decltype(a)>);
        static_assert(a.has_value(std::in_place_type<int>));
        static_assert(a.template has_value<int>());
        static_assert(a.invoke([](auto &i) -> bool { return i != 0; }));
      }
    }

    WHEN("same sum")
    {
      constexpr auto init = type{std::in_place_type<int>, 42};
      static_assert([](auto &a) constexpr -> bool { return requires { type::make(a); }; }(init));
      auto a = type::make(init);
      static_assert(std::same_as<type, decltype(a)>);
      CHECK(a.has_value(std::in_place_type<int>));
      CHECK(a.template has_value<int>());
      CHECK(a.invoke([](auto &i) -> bool { return i != 0; }));

      WHEN("constexpr")
      {
        constexpr auto a = type::make(init);
        static_assert(std::same_as<type const, decltype(a)>);
        static_assert(a.has_value(std::in_place_type<int>));
        static_assert(a.template has_value<int>());
        static_assert(a.invoke([](auto &i) -> bool { return i != 0; }));
      }
    }

    WHEN("sum type mismatch")
    {
      constexpr auto init = sum<bool, double, int>{std::in_place_type<int>, 42};
      static_assert([](auto &a) constexpr -> bool {
        return not requires { type::make(a); };
      }(init)); // type is not a superset of init
    }
  }

  WHEN("invoke")
  {
    sum<int> a{std::in_place_type<int>, 42};
    WHEN("value only")
    {
      CHECK(a.invoke(fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { return true; },
                                  [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                  [](int const &&) -> bool { throw 0; })));
      CHECK(std::as_const(a).invoke(fn::overload(
          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { return true; },
          [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; })));
      CHECK(sum<int>{std::in_place_type<int>, 42}.invoke(fn::overload(
          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
          [](int &&) -> bool { return true; }, [](int const &&) -> bool { throw 0; })));
      CHECK(std::move(std::as_const(a))
                .invoke(fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                     [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                     [](int const &&) -> bool { return true; })));

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
      CHECK(sum<int>{std::in_place_type<int>, 42}.invoke(
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
}
