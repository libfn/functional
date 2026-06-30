// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/detail/functional.hpp>
#include <fn/pack.hpp>

#include <catch2/catch_all.hpp>

#include <concepts>
#include <type_traits>

namespace {
template <typename...> struct types;

constexpr auto sum_two = [](int i, double d) { return i + d; };
} // namespace

TEST_CASE("_invoke_result", "[functional][invoke_result]")
{
  using fn::detail::_invoke_result;
  // plain invocation matches std::invoke_result
  static_assert(std::same_as<_invoke_result<decltype(sum_two), int, double>::type, double>);
  // pack arg unpacks element types into Fn
  static_assert(std::same_as<_invoke_result<decltype(sum_two), fn::pack<int, double> &>::type, double>);
  // non-invocable falls back to void (the trait's _result helper return)
  static_assert(std::same_as<_invoke_result<decltype(sum_two), int *>::type, void>);
  SUCCEED();
}

TEST_CASE("_is_invocable", "[functional][is_invocable]")
{
  using fn::detail::_is_invocable;
  static_assert(_is_invocable<decltype(sum_two), int, double>::value);
  // pack arg dispatches through pack::invoke
  static_assert(_is_invocable<decltype(sum_two), fn::pack<int, double> &>::value);
  // negative: incompatible argument
  static_assert(not _is_invocable<decltype(sum_two), int, int *>::value);
  // negative: wrong arity
  static_assert(not _is_invocable<decltype(sum_two), int>::value);
  SUCCEED();
}

TEST_CASE("_is_invocable_r", "[functional][is_invocable_r]")
{
  using fn::detail::_is_invocable_r;
  // exact return type
  static_assert(_is_invocable_r<double, decltype(sum_two), int, double>::value);
  // convertible return type
  static_assert(_is_invocable_r<long, decltype(sum_two), int, double>::value);
  // pack arg dispatches through pack::invoke_r
  static_assert(_is_invocable_r<double, decltype(sum_two), fn::pack<int, double> &>::value);
  // negative: non-convertible return type
  static_assert(not _is_invocable_r<int *, decltype(sum_two), int, double>::value);
  SUCCEED();
}

TEST_CASE("_is_nothrow_invocable", "[functional][is_nothrow_invocable]")
{
  // Hardcoded to false pending https://github.com/libfn/functional/issues/45
  using fn::detail::_is_nothrow_invocable;
  static_assert(not _is_nothrow_invocable<decltype(sum_two), int, double>::value);
  static_assert(not _is_nothrow_invocable<decltype([]() noexcept { return 0; })>::value);
  SUCCEED();
}

TEST_CASE("_is_nothrow_invocable_r", "[functional][is_nothrow_invocable_r]")
{
  // Hardcoded to false pending https://github.com/libfn/functional/issues/45
  using fn::detail::_is_nothrow_invocable_r;
  static_assert(not _is_nothrow_invocable_r<double, decltype(sum_two), int, double>::value);
  static_assert(not _is_nothrow_invocable_r<int, decltype([]() noexcept { return 0; })>::value);
  SUCCEED();
}

TEST_CASE("_invoke", "[functional][invoke]")
{
  using fn::detail::_invoke;
  // plain invocation
  static_assert(_invoke(sum_two, 1, 0.5) == 1.5);
  static_assert(std::same_as<decltype(_invoke(sum_two, 1, 0.5)), double>);
  // pack arg: dispatches through pack::invoke
  constexpr fn::pack<int, double> p{2, 0.25};
  static_assert(_invoke(sum_two, p) == 2.25);
  SUCCEED();
}

TEST_CASE("_invoke_r", "[functional][invoke_r]")
{
  using fn::detail::_invoke_r;
  // return-type conversion
  static_assert(_invoke_r<long>(sum_two, 1, 0.5) == 1);
  static_assert(std::same_as<decltype(_invoke_r<long>(sum_two, 1, 0.5)), long>);
  // pack arg: dispatches through pack::invoke_r
  constexpr fn::pack<int, double> p{3, 0.5};
  static_assert(_invoke_r<int>(sum_two, p) == 3);
  SUCCEED();
}

TEST_CASE("_typelist_invocable", "[functional][typelist_invocable]")
{
  using fn::detail::_typelist_invocable;

  // satisfied when Fn is invocable with each typelist element under matching qualifier
  static_assert(_typelist_invocable<decltype([](auto) {}), types<int, double> &>);
  static_assert(_typelist_invocable<decltype([](auto const &) {}), types<int, double> const &>);
  static_assert(_typelist_invocable<decltype([](auto &&) {}), types<int, double> &&>);

  // extra trailing args are forwarded after the element type
  static_assert(_typelist_invocable<decltype([](auto, int) {}), types<int, double> &, int>);

  // negative: one element is not invocable
  static_assert(not _typelist_invocable<decltype([](int) {}), types<int, char const *> &>);
  // negative: T is not a typelist (no Tpl<Ts...> match)
  static_assert(not _typelist_invocable<decltype([](auto) {}), int>);
  SUCCEED();
}

TEST_CASE("_typelist_invocable_r", "[functional][typelist_invocable_r]")
{
  using fn::detail::_typelist_invocable_r;

  // satisfied when each invocation's result converts to R
  static_assert(_typelist_invocable_r<long, decltype([](auto v) { return v; }), types<int, short> &>);
  // negative: result of one element does not convert to R
  static_assert(not _typelist_invocable_r<int *, decltype([](auto v) { return v; }), types<int, short> &>);
  SUCCEED();
}
