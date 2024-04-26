// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/detail/variadic_union.hpp"
#include "functional/sum.hpp"

#include <catch2/catch_all.hpp>

#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace {

struct NonCopyable final {
  int v;

  constexpr operator int() const { return v; }
  constexpr NonCopyable(int i) noexcept : v(i) {}
  NonCopyable(NonCopyable const &) = delete;
  NonCopyable &operator=(NonCopyable const &) = delete;
};
} // namespace

TEST_CASE(
    "variadic_union",
    "[variadic_union][invoke_variadic_union][invoke_type_variadic_union][ptr_variadic_union][make_variadic_union]")
{
  using fn::detail::invoke_variadic_union;
  using fn::detail::make_variadic_union;
  using fn::detail::ptr_variadic_union;
  using fn::detail::variadic_union;

  using T2 = variadic_union<NonCopyable, NonCopyable const>;
  constexpr T2 a2 = make_variadic_union<NonCopyable, T2>(12);
  static_assert(ptr_variadic_union<NonCopyable, T2>(a2)->v == 12);
  constexpr T2 a3 = make_variadic_union<NonCopyable const, T2>(36);
  static_assert(ptr_variadic_union<NonCopyable const, T2>(a3)->v == 36);

  using T6 = variadic_union<int, bool, double, float, NonCopyable>;
  constexpr T6 a4 = make_variadic_union<NonCopyable, T6>(42);
  static_assert(ptr_variadic_union<NonCopyable, T6>(a4)->v == 42);

  using U1 = variadic_union<bool>;
  constexpr U1 b1 = make_variadic_union<bool, U1>(true);
  static_assert(decltype(b1)::has_type<bool>);
  static_assert(not decltype(b1)::has_type<int>);
  static_assert([](auto &&b) -> bool { return not requires { ptr_variadic_union<int, U1>(b); }; }(b1));
  static_assert(std::same_as<decltype(ptr_variadic_union<bool, U1>(b1)), bool const *>);
  static_assert(std::same_as<decltype(ptr_variadic_union<bool, U1>(U1{.v0 = false})), bool *>);
  static_assert(*ptr_variadic_union<bool, U1>(b1));
  static_assert(invoke_variadic_union<std::size_t, U1>(b1, 0, [](auto i) -> std::size_t { return sizeof(i); }) == 1);
  static_assert(invoke_type_variadic_union<bool, U1>(b1, 0, []<typename T>(std::in_place_type_t<T>, auto i) -> bool {
    if constexpr (std::same_as<T, bool>)
      return i;
    return false;
  }));

  using U2 = variadic_union<bool, int>;
  constexpr U2 b2 = make_variadic_union<int, U2>(42);
  static_assert(decltype(b2)::has_type<bool>);
  static_assert(decltype(b2)::has_type<int>);
  static_assert(not decltype(b2)::has_type<double>);
  static_assert([](auto &&b) -> bool { return not requires { ptr_variadic_union<double, U2>(b); }; }(b2));
  static_assert(std::same_as<decltype(ptr_variadic_union<bool, U2>(b2)), bool const *>);
  static_assert(std::same_as<decltype(ptr_variadic_union<int, U2>(b2)), int const *>);
  static_assert(std::same_as<decltype(ptr_variadic_union<bool, U2>(U2{.v0 = false})), bool *>);
  static_assert(std::same_as<decltype(ptr_variadic_union<int, U2>(U2{.v1 = 12})), int *>);
  static_assert(*ptr_variadic_union<int, U2>(b2) == 42);
  static_assert(invoke_variadic_union<std::size_t, U2>(b2, 1, [](auto i) -> std::size_t { return sizeof(i); }) == 4);
  static_assert(invoke_type_variadic_union<int, U2>(b2, 1,
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
  static_assert([](auto &&b) -> bool { return not requires { ptr_variadic_union<float, U3>(b); }; }(b3));
  static_assert(std::same_as<decltype(ptr_variadic_union<bool, U3>(b3)), bool const *>);
  static_assert(std::same_as<decltype(ptr_variadic_union<int, U3>(b3)), int const *>);
  static_assert(std::same_as<decltype(ptr_variadic_union<double, U3>(b3)), double const *>);
  static_assert(std::same_as<decltype(ptr_variadic_union<bool, U3>(U3{.v0 = false})), bool *>);
  static_assert(std::same_as<decltype(ptr_variadic_union<int, U3>(U3{.v1 = 12})), int *>);
  static_assert(*ptr_variadic_union<double, U3>(b3) == 0.5);
  static_assert(invoke_variadic_union<std::size_t, U3>(b3, 2, [](auto i) -> std::size_t { return sizeof(i); }) == 8);
  static_assert(invoke_type_variadic_union<int, U3>(b3, 2,
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
  static_assert([](auto &&b) -> bool { return not requires { ptr_variadic_union<std::string_view, U4>(b); }; }(b4));
  static_assert(std::same_as<decltype(ptr_variadic_union<bool, U4>(b4)), bool const *>);
  static_assert(std::same_as<decltype(ptr_variadic_union<int, U4>(b4)), int const *>);
  static_assert(std::same_as<decltype(ptr_variadic_union<double, U4>(b4)), double const *>);
  static_assert(std::same_as<decltype(ptr_variadic_union<float, U4>(b4)), float const *>);
  static_assert(std::same_as<decltype(ptr_variadic_union<bool, U4>(U4{.v0 = false})), bool *>);
  static_assert(std::same_as<decltype(ptr_variadic_union<int, U4>(U4{.v1 = 12})), int *>);
  static_assert(*ptr_variadic_union<float, U4>(b4) == 1.5f);
  static_assert(invoke_variadic_union<std::size_t, U4>(b4, 3, [](auto i) -> std::size_t { return sizeof(i); }) == 4);
  static_assert(invoke_type_variadic_union<int, U4>(b4, 3,
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
  static_assert([](auto &&b) -> bool { return not requires { ptr_variadic_union<std::string, U5>(b); }; }(b5));
  static_assert(std::same_as<decltype(ptr_variadic_union<bool, U5>(b5)), bool const *>);
  static_assert(std::same_as<decltype(ptr_variadic_union<int, U5>(b5)), int const *>);
  static_assert(std::same_as<decltype(ptr_variadic_union<double, U5>(b5)), double const *>);
  static_assert(std::same_as<decltype(ptr_variadic_union<float, U5>(b5)), float const *>);
  static_assert(std::same_as<decltype(ptr_variadic_union<std::string_view, U5>(b5)), std::string_view const *>);
  static_assert(std::same_as<decltype(ptr_variadic_union<bool, U5>(U5{.v0 = false})), bool *>);
  static_assert(std::same_as<decltype(ptr_variadic_union<int, U5>(U5{.v1 = 12})), int *>);
  static_assert(*ptr_variadic_union<std::string_view, U5>(b5) == std::string_view{"hello"});
  static_assert(invoke_variadic_union<std::size_t, U5>(b5, 4, [](auto i) -> std::size_t { return sizeof(i); }) == 16);
  static_assert(invoke_type_variadic_union<int, U5>(b5, 4,
                                                    []<typename T>(std::in_place_type_t<T>, auto i) -> int {
                                                      if constexpr (std::same_as<T, std::string_view>)
                                                        return i.size();
                                                      return 0;
                                                    })
                == 5);

  SUCCEED();
}

TEST_CASE("variadic_unionc invoke only", "[variadic_union][invoke_variadic_union][invoke_type_variadic_union]")
{
  using fn::detail::invoke_variadic_union;
  using fn::detail::make_variadic_union;
  using fn::detail::variadic_union;

  constexpr auto fn1 = [](auto i) { return static_cast<short>(i); };
  int total = 0;
  auto fn1L = [&total](auto i) { total += static_cast<int>(i); };
  constexpr auto fn2 = [](fn::some_in_place_type auto, auto i) { return static_cast<short>(i * 2); };
  auto fn2L = [&total](fn::some_in_place_type auto, auto i) { total += static_cast<int>(2 * i); };
  auto fnAll = [](auto &&...a) { return static_cast<int>(sizeof...(a)); };

  WHEN("size == 1")
  {
    using type = variadic_union<int>;
    auto const v0 = make_variadic_union<int, type>(42);
    static_assert(std::is_same_v<void, decltype(invoke_variadic_union<void, type>(v0, 0, fn1L))>);
    static_assert(std::is_same_v<void, decltype(invoke_type_variadic_union<void, type>(v0, 0, fn2L))>);
    static_assert(std::is_same_v<int, decltype(invoke_variadic_union<int, type>(v0, 0, fn1))>);
    static_assert(std::is_same_v<long, decltype(invoke_variadic_union<long, type>(v0, 0, fn1))>);
    static_assert(std::is_same_v<int, decltype(invoke_type_variadic_union<int, type>(v0, 0, fn2))>);
    static_assert(std::is_same_v<long, decltype(invoke_type_variadic_union<long, type>(v0, 0, fn2))>);

    constexpr type a = make_variadic_union<int, type>(7);
    static_assert(std::is_same_v<void, decltype(invoke_variadic_union<void, type>(a, 0, fnAll))>);
    static_assert(invoke_type_variadic_union<int, type>(a, 0, fnAll) == 2);

    CHECK(invoke_variadic_union<int, type>(v0, 0, fn1) == 42);
    CHECK(invoke_type_variadic_union<int, type>(v0, 0, fn2) == 84);
    auto const before = total;
    invoke_variadic_union<void, type>(v0, 0, fn1L);
    CHECK(total == before + 42);
    invoke_type_variadic_union<void, type>(v0, 0, fn2L);
    CHECK(total == before + 42 + 84);
  }

  WHEN("size == 2")
  {
    using type = variadic_union<int, short>;

    static_assert(std::is_same_v<void, decltype(invoke_variadic_union<void, type>(std::declval<type>(), 0, fn1L))>);
    static_assert(
        std::is_same_v<void, decltype(invoke_type_variadic_union<void, type>(std::declval<type>(), 0, fn2L))>);
    static_assert(std::is_same_v<int, decltype(invoke_variadic_union<int, type>(std::declval<type>(), 0, fn1))>);
    static_assert(std::is_same_v<long, decltype(invoke_variadic_union<long, type>(std::declval<type>(), 0, fn1))>);
    static_assert(std::is_same_v<int, decltype(invoke_type_variadic_union<int, type>(std::declval<type>(), 0, fn2))>);
    static_assert(std::is_same_v<long, decltype(invoke_type_variadic_union<long, type>(std::declval<type>(), 0, fn2))>);

    constexpr type a = make_variadic_union<int, type>(7);
    static_assert(std::is_same_v<void, decltype(invoke_variadic_union<void, type>(a, 0, fnAll))>);
    static_assert(invoke_type_variadic_union<int, type>(a, 0, fnAll) == 2);

    WHEN("v0 set")
    {
      auto const v0 = make_variadic_union<int, type>(42);
      CHECK(invoke_variadic_union<int, type>(v0, 0, fn1) == 42);
      CHECK(invoke_type_variadic_union<int, type>(v0, 0, fn2) == 84);
      auto const before = total;
      invoke_variadic_union<void, type>(v0, 0, fn1L);
      CHECK(total == before + 42);
      invoke_type_variadic_union<void, type>(v0, 0, fn2L);
      CHECK(total == before + 42 + 84);
    }

    WHEN("v1 set")
    {
      auto const v1 = make_variadic_union<short, type>((short)26);
      CHECK(invoke_variadic_union<short, type>(v1, 1, fn1) == 26);
      CHECK(invoke_type_variadic_union<short, type>(v1, 1, fn2) == 52);
      auto const before = total;
      invoke_variadic_union<void, type>(v1, 1, fn1L);
      CHECK(total == before + 26);
      invoke_type_variadic_union<void, type>(v1, 1, fn2L);
      CHECK(total == before + 26 + 52);
    }
  }

  WHEN("size == 3")
  {
    using type = variadic_union<int, short, long>;

    static_assert(std::is_same_v<void, decltype(invoke_variadic_union<void, type>(std::declval<type>(), 0, fn1L))>);
    static_assert(
        std::is_same_v<void, decltype(invoke_type_variadic_union<void, type>(std::declval<type>(), 0, fn2L))>);
    static_assert(std::is_same_v<int, decltype(invoke_variadic_union<int, type>(std::declval<type>(), 0, fn1))>);
    static_assert(std::is_same_v<long, decltype(invoke_variadic_union<long, type>(std::declval<type>(), 0, fn1))>);
    static_assert(std::is_same_v<int, decltype(invoke_type_variadic_union<int, type>(std::declval<type>(), 0, fn2))>);
    static_assert(std::is_same_v<long, decltype(invoke_type_variadic_union<long, type>(std::declval<type>(), 0, fn2))>);

    constexpr type a = make_variadic_union<int, type>(7);
    static_assert(std::is_same_v<void, decltype(invoke_variadic_union<void, type>(a, 0, fnAll))>);
    static_assert(invoke_type_variadic_union<int, type>(a, 0, fnAll) == 2);

    WHEN("v0 set")
    {
      auto const v0 = make_variadic_union<int, type>(42);
      CHECK(invoke_variadic_union<int, type>(v0, 0, fn1) == 42);
      CHECK(invoke_type_variadic_union<int, type>(v0, 0, fn2) == 84);
      auto const before = total;
      invoke_variadic_union<void, type>(v0, 0, fn1L);
      CHECK(total == before + 42);
      invoke_type_variadic_union<void, type>(v0, 0, fn2L);
      CHECK(total == before + 42 + 84);
    }

    WHEN("v1 set")
    {
      auto const v1 = make_variadic_union<short, type>((short)26);
      CHECK(invoke_variadic_union<short, type>(v1, 1, fn1) == 26);
      CHECK(invoke_type_variadic_union<short, type>(v1, 1, fn2) == 52);
      auto const before = total;
      invoke_variadic_union<void, type>(v1, 1, fn1L);
      CHECK(total == before + 26);
      invoke_type_variadic_union<void, type>(v1, 1, fn2L);
      CHECK(total == before + 26 + 52);
    }

    WHEN("v2 set")
    {
      auto const v2 = make_variadic_union<long, type>(12);
      CHECK(invoke_variadic_union<long, type>(v2, 2, fn1) == 12);
      CHECK(invoke_type_variadic_union<long, type>(v2, 2, fn2) == 24);
      auto const before = total;
      invoke_variadic_union<void, type>(v2, 2, fn1L);
      CHECK(total == before + 12);
      invoke_type_variadic_union<void, type>(v2, 2, fn2L);
      CHECK(total == before + 12 + 24);
    }
  }

  WHEN("size == 4")
  {
    using type = variadic_union<int, short, long, double>;

    static_assert(std::is_same_v<void, decltype(invoke_variadic_union<void, type>(std::declval<type>(), 0, fn1L))>);
    static_assert(
        std::is_same_v<void, decltype(invoke_type_variadic_union<void, type>(std::declval<type>(), 0, fn2L))>);
    static_assert(std::is_same_v<int, decltype(invoke_variadic_union<int, type>(std::declval<type>(), 0, fn1))>);
    static_assert(std::is_same_v<long, decltype(invoke_variadic_union<long, type>(std::declval<type>(), 0, fn1))>);
    static_assert(std::is_same_v<int, decltype(invoke_type_variadic_union<int, type>(std::declval<type>(), 0, fn2))>);
    static_assert(std::is_same_v<long, decltype(invoke_type_variadic_union<long, type>(std::declval<type>(), 0, fn2))>);

    constexpr type a = make_variadic_union<int, type>(7);
    static_assert(std::is_same_v<void, decltype(invoke_variadic_union<void, type>(a, 0, fnAll))>);
    static_assert(invoke_type_variadic_union<int, type>(a, 0, fnAll) == 2);

    WHEN("v0 set")
    {
      auto const v0 = make_variadic_union<int, type>(42);
      CHECK(invoke_variadic_union<int, type>(v0, 0, fn1) == 42);
      CHECK(invoke_type_variadic_union<int, type>(v0, 0, fn2) == 84);
      auto const before = total;
      invoke_variadic_union<void, type>(v0, 0, fn1L);
      CHECK(total == before + 42);
      invoke_type_variadic_union<void, type>(v0, 0, fn2L);
      CHECK(total == before + 42 + 84);
    }

    WHEN("v1 set")
    {
      auto const v1 = make_variadic_union<short, type>((short)26);
      CHECK(invoke_variadic_union<short, type>(v1, 1, fn1) == 26);
      CHECK(invoke_type_variadic_union<short, type>(v1, 1, fn2) == 52);
      auto const before = total;
      invoke_variadic_union<void, type>(v1, 1, fn1L);
      CHECK(total == before + 26);
      invoke_type_variadic_union<void, type>(v1, 1, fn2L);
      CHECK(total == before + 26 + 52);
    }

    WHEN("v2 set")
    {
      auto const v2 = make_variadic_union<long, type>(12);
      CHECK(invoke_variadic_union<long, type>(v2, 2, fn1) == 12);
      CHECK(invoke_type_variadic_union<long, type>(v2, 2, fn2) == 24);
      auto const before = total;
      invoke_variadic_union<void, type>(v2, 2, fn1L);
      CHECK(total == before + 12);
      invoke_type_variadic_union<void, type>(v2, 2, fn2L);
      CHECK(total == before + 12 + 24);
    }

    WHEN("v3 set")
    {
      auto const v3 = make_variadic_union<double, type>(7.5);
      CHECK(invoke_variadic_union<int, type>(v3, 3, fn1) == 7);
      CHECK(invoke_type_variadic_union<int, type>(v3, 3, fn2) == 15);
      auto const before = total;
      invoke_variadic_union<void, type>(v3, 3, fn1L);
      CHECK(total == before + 7);
      invoke_type_variadic_union<void, type>(v3, 3, fn2L);
      CHECK(total == before + 7 + 15);
    }
  }

  WHEN("size == 5")
  {
    using type = variadic_union<int, short, long, double, float>;

    static_assert(std::is_same_v<void, decltype(invoke_variadic_union<void, type>(std::declval<type>(), 0, fn1L))>);
    static_assert(
        std::is_same_v<void, decltype(invoke_type_variadic_union<void, type>(std::declval<type>(), 0, fn2L))>);
    static_assert(std::is_same_v<int, decltype(invoke_variadic_union<int, type>(std::declval<type>(), 0, fn1))>);
    static_assert(std::is_same_v<long, decltype(invoke_variadic_union<long, type>(std::declval<type>(), 0, fn1))>);
    static_assert(std::is_same_v<int, decltype(invoke_type_variadic_union<int, type>(std::declval<type>(), 0, fn2))>);
    static_assert(std::is_same_v<long, decltype(invoke_type_variadic_union<long, type>(std::declval<type>(), 0, fn2))>);

    constexpr type a = make_variadic_union<int, type>(7);
    static_assert(std::is_same_v<void, decltype(invoke_variadic_union<void, type>(a, 0, fnAll))>);
    static_assert(invoke_type_variadic_union<int, type>(a, 0, fnAll) == 2);

    WHEN("v0 set")
    {
      auto const v0 = make_variadic_union<int, type>(42);
      CHECK(invoke_variadic_union<int, type>(v0, 0, fn1) == 42);
      CHECK(invoke_type_variadic_union<int, type>(v0, 0, fn2) == 84);
      auto const before = total;
      invoke_variadic_union<void, type>(v0, 0, fn1L);
      CHECK(total == before + 42);
      invoke_type_variadic_union<void, type>(v0, 0, fn2L);
      CHECK(total == before + 42 + 84);
    }

    WHEN("v1 set")
    {
      auto const v1 = make_variadic_union<short, type>((short)26);
      CHECK(invoke_variadic_union<short, type>(v1, 1, fn1) == 26);
      CHECK(invoke_type_variadic_union<short, type>(v1, 1, fn2) == 52);
      auto const before = total;
      invoke_variadic_union<void, type>(v1, 1, fn1L);
      CHECK(total == before + 26);
      invoke_type_variadic_union<void, type>(v1, 1, fn2L);
      CHECK(total == before + 26 + 52);
    }

    WHEN("v2 set")
    {
      auto const v2 = make_variadic_union<long, type>(12);
      CHECK(invoke_variadic_union<long, type>(v2, 2, fn1) == 12);
      CHECK(invoke_type_variadic_union<long, type>(v2, 2, fn2) == 24);
      auto const before = total;
      invoke_variadic_union<void, type>(v2, 2, fn1L);
      CHECK(total == before + 12);
      invoke_type_variadic_union<void, type>(v2, 2, fn2L);
      CHECK(total == before + 12 + 24);
    }

    WHEN("v3 set")
    {
      auto const v3 = make_variadic_union<double, type>(7.5);
      CHECK(invoke_variadic_union<int, type>(v3, 3, fn1) == 7);
      CHECK(invoke_type_variadic_union<int, type>(v3, 3, fn2) == 15);
      auto const before = total;
      invoke_variadic_union<void, type>(v3, 3, fn1L);
      CHECK(total == before + 7);
      invoke_type_variadic_union<void, type>(v3, 3, fn2L);
      CHECK(total == before + 7 + 15);
    }

    WHEN("more set")
    {
      auto const v4 = make_variadic_union<float, type>(1.5f);
      CHECK(invoke_variadic_union<int, type>(v4, 4, fn1) == 1);
      CHECK(invoke_type_variadic_union<int, type>(v4, 4, fn2) == 3);
      auto const before = total;
      invoke_variadic_union<void, type>(v4, 4, fn1L);
      CHECK(total == before + 1);
      invoke_type_variadic_union<void, type>(v4, 4, fn2L);
      CHECK(total == before + 1 + 3);
    }
  }
}
