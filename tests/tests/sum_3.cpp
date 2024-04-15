// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/sum.hpp"

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
      using T = sum<CopyOnly, double, int>;
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
      using T = sum<MoveOnly, double, int>;
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
      using T = sum<NonCopyable, double, int>;
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
  constexpr auto fn3 = [](int &) {};
  constexpr auto fn4 = [](std::in_place_type_t<int>, int &) {};

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

    static_assert(fn::typelist_invocable<decltype(fn1), decltype(a)>);
    static_assert(fn::typelist_invocable<decltype(fn2), decltype(a)>);
    static_assert(not fn::typelist_type_invocable<decltype(fn1), decltype(a)>);
    static_assert(fn::typelist_invocable<decltype(fn2), decltype(a)>);
    static_assert(fn::typelist_invocable<decltype(fn3), decltype(a) &>);
    static_assert(fn::typelist_type_invocable<decltype(fn4), decltype(a) &>);
    static_assert(not fn::typelist_invocable<decltype(fn3), decltype(a) const &>);
    static_assert(not fn::typelist_type_invocable<decltype(fn4), decltype(a) const &>);
    static_assert(not fn::typelist_invocable<decltype(fn3), decltype(a)>);
    static_assert(not fn::typelist_type_invocable<decltype(fn4), decltype(a)>);
    static_assert(not fn::typelist_invocable<decltype(fn3), decltype(a) const>);
    static_assert(not fn::typelist_type_invocable<decltype(fn4), decltype(a) const>);
    static_assert(not fn::typelist_invocable<decltype(fn3), decltype(a) &&>);
    static_assert(not fn::typelist_type_invocable<decltype(fn4), decltype(a) &&>);
    static_assert(not fn::typelist_invocable<decltype(fn3), decltype(a) const &&>);
    static_assert(not fn::typelist_type_invocable<decltype(fn4), decltype(a) const &&>);

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
      static_assert(fn::typelist_invocable<decltype(fn5), decltype(a) &>);
      static_assert(fn::typelist_type_invocable<decltype(fn6), decltype(a) &>);

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

      static_assert(fn::typelist_invocable<decltype(fn1), decltype(a)>);
      static_assert(fn::typelist_invocable<decltype(fn2), decltype(a)>);
      static_assert(not fn::typelist_type_invocable<decltype(fn1), decltype(a)>);
      static_assert(fn::typelist_invocable<decltype(fn2), decltype(a)>);

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
      static_assert(fn::typelist_invocable<decltype(fn5), decltype(a) &>);
      static_assert(fn::typelist_type_invocable<decltype(fn6), decltype(a) &>);

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

      static_assert(fn::typelist_invocable<decltype(fn1), decltype(a)>);
      static_assert(fn::typelist_invocable<decltype(fn2), decltype(a)>);
      static_assert(not fn::typelist_type_invocable<decltype(fn1), decltype(a)>);
      static_assert(fn::typelist_invocable<decltype(fn2), decltype(a)>);

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
      static_assert(fn::typelist_invocable<decltype(fn5), decltype(a) &>);
      static_assert(fn::typelist_type_invocable<decltype(fn6), decltype(a) &>);

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
      static_assert(fn::typelist_invocable<decltype(fn5), decltype(a) &>);
      static_assert(fn::typelist_type_invocable<decltype(fn6), decltype(a) &>);

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

      static_assert(fn::typelist_invocable<decltype(fn1), decltype(a)>);
      static_assert(fn::typelist_invocable<decltype(fn2), decltype(a)>);
      static_assert(not fn::typelist_type_invocable<decltype(fn1), decltype(a)>);
      static_assert(fn::typelist_invocable<decltype(fn2), decltype(a)>);

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
      static_assert(fn::typelist_invocable<decltype(fn5), decltype(a) &>);
      static_assert(fn::typelist_type_invocable<decltype(fn6), decltype(a) &>);

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
      static_assert(fn::typelist_invocable<decltype(fn5), decltype(a) &>);
      static_assert(fn::typelist_type_invocable<decltype(fn6), decltype(a) &>);

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
      static_assert(fn::typelist_invocable<decltype(fn5), decltype(a) &>);
      static_assert(fn::typelist_type_invocable<decltype(fn6), decltype(a) &>);

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

      static_assert(fn::typelist_invocable<decltype(fn1), decltype(a)>);
      static_assert(fn::typelist_invocable<decltype(fn2), decltype(a)>);
      static_assert(not fn::typelist_type_invocable<decltype(fn1), decltype(a)>);
      static_assert(fn::typelist_invocable<decltype(fn2), decltype(a)>);

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
      static_assert(fn::typelist_invocable<decltype(fn5), decltype(a) &>);
      static_assert(fn::typelist_type_invocable<decltype(fn6), decltype(a) &>);

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
      static_assert(fn::typelist_invocable<decltype(fn5), decltype(a) &>);
      static_assert(fn::typelist_type_invocable<decltype(fn6), decltype(a) &>);

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
      static_assert(fn::typelist_invocable<decltype(fn5), decltype(a) &>);
      static_assert(fn::typelist_type_invocable<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), std::vector<int> *>);
      CHECK(a.get_ptr(element) == &a.data.more.v0);
      CHECK(a.get_ptr(std::in_place_type<double>) == nullptr);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), std::vector<int> const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.more.v0);
      CHECK(std::as_const(a).get_ptr(std::in_place_type<double>) == nullptr);
    }
  }
}
