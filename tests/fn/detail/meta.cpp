// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/detail/meta.hpp>

#include <catch2/catch_all.hpp>

#include <concepts>
#include <type_traits>

TEST_CASE("select nth", "[select_nth]")
{
  // NOTE Not needed once https://wg21.link/P2662 is available
  using namespace fn::detail;

  static_assert(std::same_as<select_nth_t<0, int>, int>);
  static_assert(std::same_as<select_nth_t<0, void, int>, void>);
  static_assert(std::same_as<select_nth_t<1, void, int>, int>);
  static_assert(std::same_as<select_nth_t<0, void, int, double>, void>);
  static_assert(std::same_as<select_nth_t<1, void, int, double>, int>);
  static_assert(std::same_as<select_nth_t<2, void, int, double>, double>);

  SUCCEED();
}

TEST_CASE("type index", "[type_index]")
{
  using namespace fn::detail;

  static_assert(type_index<int, int> == 0);
  static_assert(type_index<int, bool, int> == 1);
  static_assert(type_index<int, int, bool> == 0);
  static_assert(type_index<int, void, bool, int> == 2);
  static_assert(type_index<int, void, int, bool> == 1);
  static_assert(type_index<int, int, void, bool> == 0);

  SUCCEED();
}

namespace {
template <typename... Ts> struct type_list {};
template <typename... Ts> struct types {};

// two lambdas from different scopes: key-distinct on every supported pretty-printing path
[[maybe_unused]] auto lambda1()
{
  return [] {};
}
[[maybe_unused]] auto lambda2()
{
  return [] {};
}

#if defined(__GNUC__) && !defined(__clang__)
// gcc keys a local type by its scope, so the same-named pair clang collides on is distinct here
[[maybe_unused]] auto local1()
{
  struct X final {};
  return X{};
}
[[maybe_unused]] auto local2()
{
  struct X final {};
  return X{};
}
#endif
} // namespace

TEST_CASE("normalized", "[normalized]")
{
  using namespace fn::detail;

  static_assert(std::same_as<type_list<>, normalized<>::apply<type_list>>);
  static_assert(normalized<>::size == 0);
  static_assert(std::same_as<type_list<void>, normalized<void>::apply<type_list>>);
  static_assert(normalized<int>::size == 1);
  static_assert(std::same_as<type_list<int>, normalized<int, int>::apply<type_list>>);
  static_assert(normalized<int, int>::size == 1);
  static_assert(std::same_as<type_list<void>, normalized<void, void>::apply<type_list>>);
  static_assert(std::same_as<type_list<int, void>, normalized<void, int>::apply<type_list>>);
  static_assert(std::same_as<type_list<int, void>, normalized<void, int, int, void>::apply<type_list>>);
  static_assert(normalized<void, int, int, void>::size == 2);
  static_assert(std::same_as<types<int, void>, normalized<void, int, int, void>::apply<types>>);
  static_assert(std::same_as<types<double, int>, normalized<double, int, int>::apply<types>>);

  static_assert(is_normal<>::value);
  static_assert(is_normal_v<>);
  static_assert(is_normal<int>::value);
  static_assert(is_normal_v<int>);
  static_assert(is_normal<bool, int>::value);
  static_assert(is_normal_v<int, void>);
  static_assert(is_normal<int, void>::value);
  static_assert(is_normal<bool, int, void>::value);
  static_assert(not is_normal<void, int>::value);
  static_assert(not is_normal_v<void, int>);

  static_assert(is_superset_of<types<>, types<>>);
  static_assert(not is_superset_of<types<>, types<bool>>);
  static_assert(is_superset_of<types<bool>, types<>>);
  static_assert(is_superset_of<types<bool>, types<bool>>);
  static_assert(not is_superset_of<types<bool>, types<bool, int>>);
  static_assert(is_superset_of<normalized<>, normalized<>>);
  static_assert(is_superset_of<normalized<bool, int>, normalized<>>);
  static_assert(is_superset_of<normalized<bool, int>, normalized<int>>);
  static_assert(is_superset_of<normalized<bool, int>, normalized<bool, int>>);
  static_assert(not is_superset_of<normalized<bool, int>, normalized<int, void>>);
  static_assert(not is_superset_of<normalized<bool, int>, normalized<bool, int, void>>);
  static_assert(not is_superset_of<normalized<bool>, normalized<bool, int, void>>);
  static_assert(not is_superset_of<normalized<>, normalized<bool, int>>);
  static_assert(not is_superset_of<normalized<>, normalized<bool>>);

  static_assert(type_sortkey_v<int> == "int");
  static_assert(type_sortkey_v<decltype(0)> == "int");
  // Sortkey is the compiler's type spelling: MSVC __FUNCSIG__ carries the `struct` elaboration and no
  // comma spaces, GCC/Clang don't (fundamentals like `int` above spell identically). See sum.cpp.
#ifdef _MSC_VER
  static_assert(type_sortkey_v<_ts<bool, int>> == "struct fn::detail::_ts<bool,int>");
#else
  static_assert(type_sortkey_v<_ts<bool, int>> == "fn::detail::_ts<bool, int>");
#endif

  // The collision floor (#326): dedup and canonical order both rest on key injectivity, so
  // normalized asserts distinct-type count == distinct-key count. The known colliders (same-scope
  // lambdas on gcc, same-named function-local types on clang) are mandate hard errors by design and
  // cannot appear here; these pin the counting and each printer's neighbouring good cases.
  static_assert(_distinct_types<> == 0);
  static_assert(_distinct_types<int> == 1);
  static_assert(_distinct_types<int, int> == 1);
  static_assert(_distinct_types<int, bool, int> == 2);
  static_assert(_distinct_types<int, int const, int &> == 3);

  static_assert(type_sortkey_v<decltype(lambda1())> != type_sortkey_v<decltype(lambda2())>);
  static_assert(normalized<decltype(lambda1()), decltype(lambda2())>::size == 2);

#ifdef __clang__
  // clang keys a lambda by position (file:line:col), so the same-scope pair gcc collides on is
  // distinct here
  [[maybe_unused]] auto l1 = [] {};
  [[maybe_unused]] auto l2 = [] {};
  static_assert(type_sortkey_v<decltype(l1)> != type_sortkey_v<decltype(l2)>);
  static_assert(normalized<decltype(l1), decltype(l2)>::size == 2);
#endif
#if defined(__GNUC__) && !defined(__clang__)
  static_assert(type_sortkey_v<decltype(local1())> != type_sortkey_v<decltype(local2())>);
  static_assert(normalized<decltype(local1()), decltype(local2())>::size == 2);
#endif

  SUCCEED();
}
