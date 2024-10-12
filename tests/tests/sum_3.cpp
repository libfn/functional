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
  constexpr auto fn3 = overload([](int &) {}, [](double &) {});

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

  constexpr auto a1 = sum<int>{std::in_place_type<int>, 12};
  static_assert(*a1.get_ptr(std::in_place_type<int>) == 12);
}
