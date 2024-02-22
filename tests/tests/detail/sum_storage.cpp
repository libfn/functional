// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/detail/sum_storage.hpp"
#include "functional/utility.hpp"

#include <catch2/catch_all.hpp>

#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

struct NonCopyable final {
  int v;

  constexpr operator int() const { return v; }
  constexpr NonCopyable(int i) noexcept : v(i) {}
  NonCopyable(NonCopyable const &) = delete;
  NonCopyable &operator=(NonCopyable const &) = delete;
};
} // namespace

TEST_CASE("variadic_union", "[variadic_union][apply_variadic_union][get_variadic_union][make_variadic_union]")
{
  using fn::detail::apply_variadic_union;
  using fn::detail::get_variadic_union;
  using fn::detail::make_variadic_union;
  using fn::detail::variadic_union;

  using T2 = variadic_union<NonCopyable, NonCopyable const>;
  constexpr T2 a2 = make_variadic_union<NonCopyable, T2>(12);
  static_assert(get_variadic_union<NonCopyable, T2>(a2)->v == 12);
  constexpr T2 a3 = make_variadic_union<NonCopyable const, T2>(36);
  static_assert(get_variadic_union<NonCopyable const, T2>(a3)->v == 36);

  using T6 = variadic_union<int, bool, double, float, NonCopyable>;
  constexpr T6 a4 = make_variadic_union<NonCopyable, T6>(42);
  static_assert(get_variadic_union<NonCopyable, T6>(a4)->v == 42);

  using U1 = variadic_union<bool>;
  constexpr U1 b1 = make_variadic_union<bool, U1>(true);
  static_assert(decltype(b1)::has_type<bool>);
  static_assert(not decltype(b1)::has_type<int>);
  static_assert([](auto &&b) -> bool { return not requires { get_variadic_union<int, U1>(b); }; }(b1));
  static_assert(std::same_as<decltype(get_variadic_union<bool, U1>(b1)), bool const *>);
  static_assert(std::same_as<decltype(get_variadic_union<bool, U1>(U1{.v0 = false})), bool *>);
  static_assert(*get_variadic_union<bool, U1>(b1));
  static_assert(apply_variadic_union<U1>(b1, 0, [](auto i) -> std::size_t { return sizeof(i); }) == 1);
  static_assert(apply_variadic_union<U1>(b1, 0, []<typename T>(std::in_place_type_t<T>, auto i) -> bool {
    if constexpr (std::same_as<T, bool>)
      return i;
    return false;
  }));

  using U2 = variadic_union<bool, int>;
  constexpr U2 b2 = make_variadic_union<int, U2>(42);
  static_assert(decltype(b2)::has_type<bool>);
  static_assert(decltype(b2)::has_type<int>);
  static_assert(not decltype(b2)::has_type<double>);
  static_assert([](auto &&b) -> bool { return not requires { get_variadic_union<double, U2>(b); }; }(b2));
  static_assert(std::same_as<decltype(get_variadic_union<bool, U2>(b2)), bool const *>);
  static_assert(std::same_as<decltype(get_variadic_union<int, U2>(b2)), int const *>);
  static_assert(std::same_as<decltype(get_variadic_union<bool, U2>(U2{.v0 = false})), bool *>);
  static_assert(std::same_as<decltype(get_variadic_union<int, U2>(U2{.v1 = 12})), int *>);
  static_assert(*get_variadic_union<int, U2>(b2) == 42);
  static_assert(apply_variadic_union<U2>(b2, 1, [](auto i) -> std::size_t { return sizeof(i); }) == 4);
  static_assert(apply_variadic_union<U2>(b2, 1,
                                         []<typename T>(std::in_place_type_t<T>, auto i) -> int {
                                           if constexpr (std::same_as<T, int>)
                                             return i / 2;
                                           return 0;
                                         })
                == 21);

  using U3 = variadic_union<bool, int, double>;
  constexpr U3 b3 = make_variadic_union<double, U3>(0.5);
  static_assert(decltype(b3)::has_type<bool>);
  static_assert(decltype(b3)::has_type<int>);
  static_assert(decltype(b3)::has_type<double>);
  static_assert(not decltype(b3)::has_type<float>);
  static_assert([](auto &&b) -> bool { return not requires { get_variadic_union<float, U3>(b); }; }(b3));
  static_assert(std::same_as<decltype(get_variadic_union<bool, U3>(b3)), bool const *>);
  static_assert(std::same_as<decltype(get_variadic_union<int, U3>(b3)), int const *>);
  static_assert(std::same_as<decltype(get_variadic_union<double, U3>(b3)), double const *>);
  static_assert(std::same_as<decltype(get_variadic_union<bool, U3>(U3{.v0 = false})), bool *>);
  static_assert(std::same_as<decltype(get_variadic_union<int, U3>(U3{.v1 = 12})), int *>);
  static_assert(*get_variadic_union<double, U3>(b3) == 0.5);
  static_assert(apply_variadic_union<U3>(b3, 2, [](auto i) -> std::size_t { return sizeof(i); }) == 8);
  static_assert(apply_variadic_union<U3>(b3, 2,
                                         []<typename T>(std::in_place_type_t<T>, auto i) -> int {
                                           if constexpr (std::same_as<T, double>)
                                             return i * 4;
                                           return 0;
                                         })
                == 2);

  using U4 = variadic_union<bool, int, double, float>;
  constexpr U4 b4 = make_variadic_union<float, U4>(1.5f);
  static_assert(decltype(b4)::has_type<bool>);
  static_assert(decltype(b4)::has_type<int>);
  static_assert(decltype(b4)::has_type<double>);
  static_assert(decltype(b4)::has_type<float>);
  static_assert(not decltype(b4)::has_type<std::string_view>);
  static_assert([](auto &&b) -> bool { return not requires { get_variadic_union<std::string_view, U4>(b); }; }(b4));
  static_assert(std::same_as<decltype(get_variadic_union<bool, U4>(b4)), bool const *>);
  static_assert(std::same_as<decltype(get_variadic_union<int, U4>(b4)), int const *>);
  static_assert(std::same_as<decltype(get_variadic_union<double, U4>(b4)), double const *>);
  static_assert(std::same_as<decltype(get_variadic_union<float, U4>(b4)), float const *>);
  static_assert(std::same_as<decltype(get_variadic_union<bool, U4>(U4{.v0 = false})), bool *>);
  static_assert(std::same_as<decltype(get_variadic_union<int, U4>(U4{.v1 = 12})), int *>);
  static_assert(*get_variadic_union<float, U4>(b4) == 1.5f);
  static_assert(apply_variadic_union<U4>(b4, 3, [](auto i) -> std::size_t { return sizeof(i); }) == 4);
  static_assert(apply_variadic_union<U4>(b4, 3,
                                         []<typename T>(std::in_place_type_t<T>, auto i) -> int {
                                           if constexpr (std::same_as<T, float>)
                                             return i * 4;
                                           return 0;
                                         })
                == 6);

  using U5 = variadic_union<bool, int, double, float, std::string_view>;
  constexpr U5 b5 = make_variadic_union<std::string_view, U5>("hello");
  static_assert(decltype(b5)::has_type<bool>);
  static_assert(decltype(b5)::has_type<int>);
  static_assert(decltype(b5)::has_type<double>);
  static_assert(decltype(b5)::has_type<float>);
  static_assert(decltype(b5)::has_type<std::string_view>);
  static_assert(not decltype(b5)::has_type<std::string>);
  static_assert([](auto &&b) -> bool { return not requires { get_variadic_union<std::string, U5>(b); }; }(b5));
  static_assert(std::same_as<decltype(get_variadic_union<bool, U5>(b5)), bool const *>);
  static_assert(std::same_as<decltype(get_variadic_union<int, U5>(b5)), int const *>);
  static_assert(std::same_as<decltype(get_variadic_union<double, U5>(b5)), double const *>);
  static_assert(std::same_as<decltype(get_variadic_union<float, U5>(b5)), float const *>);
  static_assert(std::same_as<decltype(get_variadic_union<std::string_view, U5>(b5)), std::string_view const *>);
  static_assert(std::same_as<decltype(get_variadic_union<bool, U5>(U5{.v0 = false})), bool *>);
  static_assert(std::same_as<decltype(get_variadic_union<int, U5>(U5{.v1 = 12})), int *>);
  static_assert(*get_variadic_union<std::string_view, U5>(b5) == std::string_view{"hello"});
  static_assert(apply_variadic_union<U5>(b5, 4, [](auto i) -> std::size_t { return sizeof(i); }) == 16);
  static_assert(apply_variadic_union<U5>(b5, 4,
                                         []<typename T>(std::in_place_type_t<T>, auto i) -> int {
                                           if constexpr (std::same_as<T, std::string_view>)
                                             return i.size();
                                           return 0;
                                         })
                == 5);

  SUCCEED();
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
} // namespace

TEST_CASE("sum_storage move and copy", "[sum_storage][has_value][get_ptr]")
{
  using fn::detail::sum_storage;

  WHEN("move and copy")
  {
    WHEN("one type only")
    {
      using T = sum_storage<std::string>;
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
      using T = sum_storage<std::string, std::string_view>;
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
      using T = sum_storage<CopyOnly>;
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
      using T = sum_storage<CopyOnly, double, int>;
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
      using T = sum_storage<MoveOnly>;
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
      using T = sum_storage<MoveOnly, double, int>;
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
      using T = sum_storage<NonCopyable>;
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
      using T = sum_storage<NonCopyable, double, int>;
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

TEST_CASE("sum_storage", "[sum_storage][has_value][get_ptr]")
{
  // NOTE We have 5 different specializations, need to test each

  using namespace fn::detail;
  constexpr auto fn1 = [](auto) {};
  constexpr auto fn2 = [](auto...) {};
  constexpr auto fn3 = [](int &) {};
  constexpr auto fn4 = [](std::in_place_type_t<int>, int &) {};

  constexpr sum_storage<std::array<int, 3>> a{std::in_place_type<std::array<int, 3>>, 3, 14, 15};
  static_assert(a.index == 0);
  static_assert(decltype(a)::has_type<std::array<int, 3>>);
  static_assert(not decltype(a)::has_type<int>);
  static_assert(a.has_value(std::in_place_type<std::array<int, 3>>));
  static_assert(a.template has_value<std::array<int, 3>>());
  static_assert(a.invoke([](auto &i) -> bool {
    return std::same_as<std::array<int, 3> const &, decltype(i)> && i.size() == 3 && i[0] == 3 && i[1] == 14
           && i[2] == 15;
  }));

  WHEN("size 1")
  {
    using T = sum_storage<int>;
    T a{std::in_place_type<int>, 42};
    static_assert(T::size == 1);
    static_assert(std::is_same_v<T::template type<0>, int>);
    static_assert(T::has_type<int>);
    static_assert(not T::has_type<bool>);
    CHECK(a.index == 0);
    CHECK(a.template has_value<int>());
    CHECK(a.has_value(std::in_place_type<int>));

    static_assert(T::invocable<decltype(fn1), decltype(a)>);
    static_assert(T::invocable<decltype(fn2), decltype(a)>);
    static_assert(not T::type_invocable<decltype(fn1), decltype(a)>);
    static_assert(T::invocable<decltype(fn2), decltype(a)>);
    static_assert(T::invocable<decltype(fn3), decltype(a) &>);
    static_assert(T::type_invocable<decltype(fn4), decltype(a) &>);
    static_assert(not T::invocable<decltype(fn3), decltype(a) const &>);
    static_assert(not T::type_invocable<decltype(fn4), decltype(a) const &>);
    static_assert(not T::invocable<decltype(fn3), decltype(a)>);
    static_assert(not T::type_invocable<decltype(fn4), decltype(a)>);
    static_assert(not T::invocable<decltype(fn3), decltype(a) const>);
    static_assert(not T::type_invocable<decltype(fn4), decltype(a) const>);
    static_assert(not T::invocable<decltype(fn3), decltype(a) &&>);
    static_assert(not T::type_invocable<decltype(fn4), decltype(a) &&>);
    static_assert(not T::invocable<decltype(fn3), decltype(a) const &&>);
    static_assert(not T::type_invocable<decltype(fn4), decltype(a) const &&>);

    static_assert(std::is_same_v<decltype(a.get_ptr(std::in_place_type<int>)), int *>);
    CHECK(a.get_ptr(std::in_place_type<int>) == &a.data.v0);
    static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(std::in_place_type<int>)), int const *>);
    CHECK(std::as_const(a).get_ptr(std::in_place_type<int>) == &a.data.v0);

    constexpr auto a1 = sum_storage<int>{std::in_place_type<int>, 12};
    static_assert(*a1.get_ptr(std::in_place_type<int>) == 12);
  }

  WHEN("size 2")
  {
    using T = sum_storage<double, int>;
    static_assert(T::size == 2);
    static_assert(std::is_same_v<T::template type<0>, double>);
    static_assert(std::is_same_v<T::template type<1>, int>);
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
      static_assert(T::invocable<decltype(fn5), decltype(a) &>);
      static_assert(T::type_invocable<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), double *>);
      CHECK(a.get_ptr(element) == &a.data.v0);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), double const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v0);
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

      static_assert(T::invocable<decltype(fn1), decltype(a)>);
      static_assert(T::invocable<decltype(fn2), decltype(a)>);
      static_assert(not T::type_invocable<decltype(fn1), decltype(a)>);
      static_assert(T::invocable<decltype(fn2), decltype(a)>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), int *>);
      CHECK(a.get_ptr(element) == &a.data.v1);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), int const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v1);
    }
  }
  WHEN("size 3")
  {
    using T = sum_storage<double, int, std::string_view>;
    static_assert(T::size == 3);
    static_assert(std::is_same_v<T::template type<0>, double>);
    static_assert(std::is_same_v<T::template type<1>, int>);
    static_assert(std::is_same_v<T::template type<2>, std::string_view>);
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
      static_assert(T::invocable<decltype(fn5), decltype(a) &>);
      static_assert(T::type_invocable<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), double *>);
      CHECK(a.get_ptr(element) == &a.data.v0);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), double const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v0);
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

      static_assert(T::invocable<decltype(fn1), decltype(a)>);
      static_assert(T::invocable<decltype(fn2), decltype(a)>);
      static_assert(not T::type_invocable<decltype(fn1), decltype(a)>);
      static_assert(T::invocable<decltype(fn2), decltype(a)>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), int *>);
      CHECK(a.get_ptr(element) == &a.data.v1);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), int const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v1);
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
      static_assert(T::invocable<decltype(fn5), decltype(a) &>);
      static_assert(T::type_invocable<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), std::string_view *>);
      CHECK(a.get_ptr(element) == &a.data.v2);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), std::string_view const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v2);
    }
  }

  WHEN("size 4")
  {
    using T = sum_storage<double, int, std::string, std::string_view>;
    static_assert(T::size == 4);
    static_assert(std::is_same_v<T::template type<0>, double>);
    static_assert(std::is_same_v<T::template type<1>, int>);
    static_assert(std::is_same_v<T::template type<2>, std::string>);
    static_assert(std::is_same_v<T::template type<3>, std::string_view>);
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
      static_assert(T::invocable<decltype(fn5), decltype(a) &>);
      static_assert(T::type_invocable<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), double *>);
      CHECK(a.get_ptr(element) == &a.data.v0);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), double const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v0);
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

      static_assert(T::invocable<decltype(fn1), decltype(a)>);
      static_assert(T::invocable<decltype(fn2), decltype(a)>);
      static_assert(not T::type_invocable<decltype(fn1), decltype(a)>);
      static_assert(T::invocable<decltype(fn2), decltype(a)>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), int *>);
      CHECK(a.get_ptr(element) == &a.data.v1);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), int const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v1);
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
      static_assert(T::invocable<decltype(fn5), decltype(a) &>);
      static_assert(T::type_invocable<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), std::string *>);
      CHECK(a.get_ptr(element) == &a.data.v2);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), std::string const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v2);
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
      static_assert(T::invocable<decltype(fn5), decltype(a) &>);
      static_assert(T::type_invocable<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), std::string_view *>);
      CHECK(a.get_ptr(element) == &a.data.v3);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), std::string_view const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v3);
    }
  }

  WHEN("size 5")
  {
    using T = sum_storage<double, int, std::string, std::string_view, std::vector<int>>;
    static_assert(T::size == 5);
    static_assert(std::is_same_v<T::template type<0>, double>);
    static_assert(std::is_same_v<T::template type<1>, int>);
    static_assert(std::is_same_v<T::template type<2>, std::string>);
    static_assert(std::is_same_v<T::template type<3>, std::string_view>);
    static_assert(std::is_same_v<T::template type<4>, std::vector<int>>);
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
      static_assert(T::invocable<decltype(fn5), decltype(a) &>);
      static_assert(T::type_invocable<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), double *>);
      CHECK(a.get_ptr(element) == &a.data.v0);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), double const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v0);
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

      static_assert(T::invocable<decltype(fn1), decltype(a)>);
      static_assert(T::invocable<decltype(fn2), decltype(a)>);
      static_assert(not T::type_invocable<decltype(fn1), decltype(a)>);
      static_assert(T::invocable<decltype(fn2), decltype(a)>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), int *>);
      CHECK(a.get_ptr(element) == &a.data.v1);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), int const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v1);
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
      static_assert(T::invocable<decltype(fn5), decltype(a) &>);
      static_assert(T::type_invocable<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), std::string *>);
      CHECK(a.get_ptr(element) == &a.data.v2);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), std::string const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v2);
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
      static_assert(T::invocable<decltype(fn5), decltype(a) &>);
      static_assert(T::type_invocable<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), std::string_view *>);
      CHECK(a.get_ptr(element) == &a.data.v3);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), std::string_view const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.v3);
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
      static_assert(T::invocable<decltype(fn5), decltype(a) &>);
      static_assert(T::type_invocable<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), std::vector<int> *>);
      CHECK(a.get_ptr(element) == &a.data.more.v0);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), std::vector<int> const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.data.more.v0);
    }
  }
}

TEST_CASE("sum_storage functions", "[sum_storage][invoke]")
{
  // NOTE We have 5 different specializations, need to test each. This test is
  // ridiculously long to exercise the value-category preserving FWD(v) in apply_variadic_union

  using namespace fn::detail;
  constexpr auto fn1 = [](auto i) noexcept -> std::size_t { return sizeof(i); };
  constexpr auto fn2 = [](auto, auto i) noexcept -> std::size_t { return sizeof(i); };

  WHEN("size 1")
  {
    sum_storage<int> a{std::in_place_type<int>, 42};
    static_assert(decltype(a)::size == 1);
    CHECK(a.data.v0 == 42);

    WHEN("value only")
    {
      CHECK(a.invoke(fn1) == 4);
      CHECK(a.invoke(   //
          fn::overload( //
              [](auto) -> bool { throw 1; }, [](int &i) -> bool { return i == 42; },
              [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
              [](int const &&) -> bool { throw 0; })));
      CHECK(std::as_const(a).invoke( //
          fn::overload(              //
              [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
              [](int const &i) -> bool { return i == 42; }, [](int &&) -> bool { throw 0; },
              [](int const &&) -> bool { throw 0; })));
      CHECK(sum_storage<int>{std::in_place_type<int>, 42}.invoke( //
          fn::overload(                                           //
              [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
              [](int &&i) -> bool { return i == 42; }, [](int const &&) -> bool { throw 0; })));
      CHECK(std::move(std::as_const(a))
                .invoke(          //
                    fn::overload( //
                        [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                        [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                        [](int const &&i) -> bool { return i == 42; })));
    }

    WHEN("tag and value")
    {
      CHECK(a.invoke(fn2) == 4);
      CHECK(a.invoke(   //
          fn::overload( //
              [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &i) -> bool { return i == 42; },
              [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
              [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
              [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
      CHECK(std::as_const(a).invoke( //
          fn::overload(              //
              [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
              [](std::in_place_type_t<int>, int const &i) -> bool { return i == 42; },
              [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
              [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
      CHECK(sum_storage<int>{std::in_place_type<int>, 42}.invoke( //
          fn::overload(                                           //
              [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
              [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
              [](std::in_place_type_t<int>, int &&i) -> bool { return i == 42; },
              [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
      CHECK(std::move(std::as_const(a))
                .invoke(          //
                    fn::overload( //
                        [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                        [](std::in_place_type_t<int>, int const &&i) -> bool { return i == 42; })));
    }
  }

  WHEN("size 2")
  {
    using type = sum_storage<double, int>;
    static_assert(type::size == 2);

    WHEN("element v0 set")
    {
      type a{std::in_place_type<double>, 0.5};
      CHECK(a.data.v0 == 0.5);
      WHEN("value only")
      {
        CHECK(a.invoke(fn1) == 8);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](double &i) -> bool { return i == 0.5; },
                [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                [](double const &i) -> bool { return i == 0.5; }, [](double &&) -> bool { throw 0; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<double>, 0.5}.invoke( //
            fn::overload(                                   //
                [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                [](double const &) -> bool { throw 0; }, [](double &&i) -> bool { return i == 0.5; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                          [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                          [](double const &&i) -> bool { return i == 0.5; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 8);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<double>, double &i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<double>, 0.5}.invoke( //
            fn::overload(                                   //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double &&i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
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
        CHECK(a.invoke(fn1) == 4);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](int &i) -> bool { return i == 42; },
                [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                [](int const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                [](int const &i) -> bool { return i == 42; }, [](int &&) -> bool { throw 0; },
                [](int const &&) -> bool { throw 0; })));
        CHECK(sum_storage<int>{std::in_place_type<int>, 42}.invoke( //
            fn::overload(                                           //
                [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                [](int &&i) -> bool { return i == 42; }, [](int const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                          [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                          [](int const &&i) -> bool { return i == 42; })));
      }

      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 4);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(sum_storage<int>{std::in_place_type<int>, 42}.invoke( //
            fn::overload(                                           //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int &&i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke(          //
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
    using type = sum_storage<double, int, std::string_view>;
    static_assert(type::size == 3);

    WHEN("element v0 set")
    {
      type a{std::in_place_type<double>, 0.5};
      CHECK(a.data.v0 == 0.5);
      WHEN("value only")
      {
        CHECK(a.invoke(fn1) == 8);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](double &i) -> bool { return i == 0.5; },
                [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                [](double const &i) -> bool { return i == 0.5; }, [](double &&) -> bool { throw 0; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<double>, 0.5}.invoke( //
            fn::overload(                                   //
                [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                [](double const &) -> bool { throw 0; }, [](double &&i) -> bool { return i == 0.5; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                          [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                          [](double const &&i) -> bool { return i == 0.5; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 8);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<double>, double &i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<double>, 0.5}.invoke( //
            fn::overload(                                   //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double &&i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
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
        CHECK(a.invoke(fn1) == 4);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](int &i) -> bool { return i == 42; },
                [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                [](int const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                [](int const &i) -> bool { return i == 42; }, [](int &&) -> bool { throw 0; },
                [](int const &&) -> bool { throw 0; })));
        CHECK(sum_storage<int>{std::in_place_type<int>, 42}.invoke( //
            fn::overload(                                           //
                [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                [](int &&i) -> bool { return i == 42; }, [](int const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                          [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                          [](int const &&i) -> bool { return i == 42; })));
      }

      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 4);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(sum_storage<int>{std::in_place_type<int>, 42}.invoke( //
            fn::overload(                                           //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int &&i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke(          //
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
        CHECK(a.invoke(fn1) == 16);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](std::string_view &i) -> bool { return i == "baz"; },
                [](std::string_view const &) -> bool { throw 0; }, [](std::string_view &&) -> bool { throw 0; },
                [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                [](std::string_view const &i) -> bool { return i == "baz"; },
                [](std::string_view &&) -> bool { throw 0; }, [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string_view>, "baz"}.invoke( //
            fn::overload(                                               //
                [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                [](std::string_view const &) -> bool { throw 0; },
                [](std::string_view &&i) -> bool { return i == "baz"; },
                [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke(          //
                    fn::overload( //
                        [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                        [](std::string_view const &) -> bool { throw 0; }, [](std::string_view &&) -> bool { throw 0; },
                        [](std::string_view const &&i) -> bool { return i == "baz"; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 16);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string_view>, "baz"}.invoke( //
            fn::overload(
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke( //
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
    using type = sum_storage<double, int, std::string, std::string_view>;
    static_assert(type::size == 4);

    WHEN("element v0 set")
    {
      type a{std::in_place_type<double>, 0.5};
      CHECK(a.data.v0 == 0.5);
      WHEN("value only")
      {
        CHECK(a.invoke(fn1) == 8);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](double &i) -> bool { return i == 0.5; },
                [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                [](double const &i) -> bool { return i == 0.5; }, [](double &&) -> bool { throw 0; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<double>, 0.5}.invoke( //
            fn::overload(                                   //
                [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                [](double const &) -> bool { throw 0; }, [](double &&i) -> bool { return i == 0.5; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                          [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                          [](double const &&i) -> bool { return i == 0.5; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 8);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<double>, double &i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<double>, 0.5}.invoke( //
            fn::overload(                                   //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double &&i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
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
        CHECK(a.invoke(fn1) == 4);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](int &i) -> bool { return i == 42; },
                [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                [](int const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                [](int const &i) -> bool { return i == 42; }, [](int &&) -> bool { throw 0; },
                [](int const &&) -> bool { throw 0; })));
        CHECK(sum_storage<int>{std::in_place_type<int>, 42}.invoke( //
            fn::overload(                                           //
                [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                [](int &&i) -> bool { return i == 42; }, [](int const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                          [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                          [](int const &&i) -> bool { return i == 42; })));
      }

      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 4);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(sum_storage<int>{std::in_place_type<int>, 42}.invoke( //
            fn::overload(                                           //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int &&i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke(          //
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
        CHECK(a.invoke(fn1) == 32);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](std::string &i) -> bool { return i == "bar"; },
                [](std::string const &) -> bool { throw 0; }, [](std::string &&) -> bool { throw 0; },
                [](std::string const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](std::string &) -> bool { throw 0; },
                [](std::string const &i) -> bool { return i == "bar"; }, [](std::string &&) -> bool { throw 0; },
                [](std::string const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string>, "bar"}.invoke( //
            fn::overload(                                          //
                [](auto) -> bool { throw 1; }, [](std::string &) -> bool { throw 0; },
                [](std::string const &) -> bool { throw 0; }, [](std::string &&i) -> bool { return i == "bar"; },
                [](std::string const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](std::string &) -> bool { throw 0; },
                          [](std::string const &) -> bool { throw 0; }, [](std::string &&) -> bool { throw 0; },
                          [](std::string const &&i) -> bool { return i == "bar"; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 32);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string>, std::string &i) -> bool { return i == "bar"; },
                [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string const &i) -> bool { return i == "bar"; },
                [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string>, "bar"}.invoke( //
            fn::overload(                                          //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string &&i) -> bool { return i == "bar"; },
                [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke(          //
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
        CHECK(a.invoke(fn1) == 16);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](std::string_view &i) -> bool { return i == "baz"; },
                [](std::string_view const &) -> bool { throw 0; }, [](std::string_view &&) -> bool { throw 0; },
                [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                [](std::string_view const &i) -> bool { return i == "baz"; },
                [](std::string_view &&) -> bool { throw 0; }, [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string_view>, "baz"}.invoke( //
            fn::overload(                                               //
                [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                [](std::string_view const &) -> bool { throw 0; },
                [](std::string_view &&i) -> bool { return i == "baz"; },
                [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke(          //
                    fn::overload( //
                        [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                        [](std::string_view const &) -> bool { throw 0; }, [](std::string_view &&) -> bool { throw 0; },
                        [](std::string_view const &&i) -> bool { return i == "baz"; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 16);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string_view>, "baz"}.invoke( //
            fn::overload(
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke( //
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
    using type = sum_storage<double, int, std::string, std::string_view, std::vector<int>>;
    static_assert(type::size == 5);

    WHEN("element v0 set")
    {
      type a{std::in_place_type<double>, 0.5};
      CHECK(a.data.v0 == 0.5);
      WHEN("value only")
      {
        CHECK(a.invoke(fn1) == 8);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](double &i) -> bool { return i == 0.5; },
                [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                [](double const &i) -> bool { return i == 0.5; }, [](double &&) -> bool { throw 0; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<double>, 0.5}.invoke( //
            fn::overload(                                   //
                [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                [](double const &) -> bool { throw 0; }, [](double &&i) -> bool { return i == 0.5; },
                [](double const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                          [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                          [](double const &&i) -> bool { return i == 0.5; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 8);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<double>, double &i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<double>, 0.5}.invoke( //
            fn::overload(                                   //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                [](std::in_place_type_t<double>, double &&i) -> bool { return i == 0.5; },
                [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
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
        CHECK(a.invoke(fn1) == 4);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](int &i) -> bool { return i == 42; },
                [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                [](int const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                [](int const &i) -> bool { return i == 42; }, [](int &&) -> bool { throw 0; },
                [](int const &&) -> bool { throw 0; })));
        CHECK(sum_storage<int>{std::in_place_type<int>, 42}.invoke( //
            fn::overload(                                           //
                [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                [](int &&i) -> bool { return i == 42; }, [](int const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                          [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                          [](int const &&i) -> bool { return i == 42; })));
      }

      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 4);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(sum_storage<int>{std::in_place_type<int>, 42}.invoke( //
            fn::overload(                                           //
                [](auto, auto) -> bool { throw 1; }, [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                [](std::in_place_type_t<int>, int &&i) -> bool { return i == 42; },
                [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke(          //
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
        CHECK(a.invoke(fn1) == 32);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](std::string &i) -> bool { return i == "bar"; },
                [](std::string const &) -> bool { throw 0; }, [](std::string &&) -> bool { throw 0; },
                [](std::string const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](std::string &) -> bool { throw 0; },
                [](std::string const &i) -> bool { return i == "bar"; }, [](std::string &&) -> bool { throw 0; },
                [](std::string const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string>, "bar"}.invoke( //
            fn::overload(                                          //
                [](auto) -> bool { throw 1; }, [](std::string &) -> bool { throw 0; },
                [](std::string const &) -> bool { throw 0; }, [](std::string &&i) -> bool { return i == "bar"; },
                [](std::string const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke(          //
                      fn::overload( //
                          [](auto) -> bool { throw 1; }, [](std::string &) -> bool { throw 0; },
                          [](std::string const &) -> bool { throw 0; }, [](std::string &&) -> bool { throw 0; },
                          [](std::string const &&i) -> bool { return i == "bar"; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 32);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string>, std::string &i) -> bool { return i == "bar"; },
                [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string const &i) -> bool { return i == "bar"; },
                [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string>, "bar"}.invoke( //
            fn::overload(                                          //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string>, std::string &&i) -> bool { return i == "bar"; },
                [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke(          //
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
        CHECK(a.invoke(fn1) == 16);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [](std::string_view &i) -> bool { return i == "baz"; },
                [](std::string_view const &) -> bool { throw 0; }, [](std::string_view &&) -> bool { throw 0; },
                [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                [](std::string_view const &i) -> bool { return i == "baz"; },
                [](std::string_view &&) -> bool { throw 0; }, [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string_view>, "baz"}.invoke( //
            fn::overload(                                               //
                [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                [](std::string_view const &) -> bool { throw 0; },
                [](std::string_view &&i) -> bool { return i == "baz"; },
                [](std::string_view const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke(          //
                    fn::overload( //
                        [](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                        [](std::string_view const &) -> bool { throw 0; }, [](std::string_view &&) -> bool { throw 0; },
                        [](std::string_view const &&i) -> bool { return i == "baz"; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 16);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::string_view>, "baz"}.invoke( //
            fn::overload(
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&i) -> bool { return i == "baz"; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke( //
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
        CHECK(a.invoke(fn1) == 24);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto) -> bool { throw 1; }, [&](std::vector<int> &i) -> bool { return i == foo; },
                [](std::vector<int> const &) -> bool { throw 0; }, [](std::vector<int> &&) -> bool { throw 0; },
                [](std::vector<int> const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(              //
                [](auto) -> bool { throw 1; }, [](std::vector<int> &) -> bool { throw 0; },
                [&](std::vector<int> const &i) -> bool { return i == foo; },
                [](std::vector<int> &&) -> bool { throw 0; }, [](std::vector<int> const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::vector<int>>, foo}.invoke( //
            fn::overload(                                             //
                [](auto) -> bool { throw 1; }, [](std::vector<int> &) -> bool { throw 0; },
                [](std::vector<int> const &) -> bool { throw 0; },
                [&](std::vector<int> &&i) -> bool { return i == foo; },
                [](std::vector<int> const &&) -> bool { throw 0; })));
        CHECK(
            std::move(std::as_const(a))
                .invoke(          //
                    fn::overload( //
                        [](auto) -> bool { throw 1; }, [](std::vector<int> &) -> bool { throw 0; },
                        [](std::vector<int> const &) -> bool { throw 0; }, [](std::vector<int> &&) -> bool { throw 0; },
                        [&](std::vector<int> const &&i) -> bool { return i == foo; })));
      }
      WHEN("tag and value")
      {
        CHECK(a.invoke(fn2) == 24);
        CHECK(a.invoke(   //
            fn::overload( //
                [](auto, auto) -> bool { throw 1; },
                [&](std::in_place_type_t<std::vector<int>>, std::vector<int> &i) -> bool { return i == foo; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> const &&) -> bool { throw 0; })));
        CHECK(std::as_const(a).invoke( //
            fn::overload(
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> &) -> bool { throw 0; },
                [&](std::in_place_type_t<std::vector<int>>, std::vector<int> const &i) -> bool { return i == foo; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> const &&) -> bool { throw 0; })));
        CHECK(type{std::in_place_type<std::vector<int>>, foo}.invoke( //
            fn::overload(                                             //
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> &) -> bool { throw 0; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> const &) -> bool { throw 0; },
                [&](std::in_place_type_t<std::vector<int>>, std::vector<int> &&i) -> bool { return i == foo; },
                [](std::in_place_type_t<std::vector<int>>, std::vector<int> const &&) -> bool { throw 0; })));
        CHECK(std::move(std::as_const(a))
                  .invoke( //
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
