// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/detail/variadic_union.hpp"
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

TEST_CASE("variadic_union", "[variadic_union][invoke_variadic_union][ptr_variadic_union][make_variadic_union]")
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
  static_assert(invoke_variadic_union<bool, U1>(b1, 0, []<typename T>(std::in_place_type_t<T>, auto i) -> bool {
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
  static_assert(invoke_variadic_union<int, U2>(b2, 1,
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
  static_assert(invoke_variadic_union<int, U3>(b3, 2,
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
  static_assert(invoke_variadic_union<int, U4>(b4, 3,
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
  static_assert(invoke_variadic_union<int, U5>(b5, 4,
                                               []<typename T>(std::in_place_type_t<T>, auto i) -> int {
                                                 if constexpr (std::same_as<T, std::string_view>)
                                                   return i.size();
                                                 return 0;
                                               })
                == 5);

  SUCCEED();
}
