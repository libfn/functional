// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/detail/functional.hpp>
#include <fn/pack.hpp>
#include <fn/utility.hpp>

#include <catch2/catch_all.hpp>

#include <concepts>
#include <type_traits>

namespace {
template <typename...> struct types;

constexpr auto sum_two = [](int i, double d) { return i + d; };
} // namespace

TEST_CASE("_apply_result", "[functional][apply_result]")
{
  using fn::detail::_apply_result;
  // plain invocation matches std::invoke_result
  static_assert(std::same_as<_apply_result<decltype(sum_two), int, double>::type, double>);
  // pack arg unpacks element types into Fn
  static_assert(std::same_as<_apply_result<decltype(sum_two), fn::pack<int, double> &>::type, double>);
  // non-applicable falls back to void (the trait's _result helper return)
  static_assert(std::same_as<_apply_result<decltype(sum_two), int *>::type, void>);
  SUCCEED();
}

TEST_CASE("_is_applicable", "[functional][is_applicable]")
{
  using fn::detail::_is_applicable;
  static_assert(_is_applicable<decltype(sum_two), int, double>::value);
  // pack arg dispatches through pack::apply
  static_assert(_is_applicable<decltype(sum_two), fn::pack<int, double> &>::value);
  // negative: incompatible argument
  static_assert(not _is_applicable<decltype(sum_two), int, int *>::value);
  // negative: wrong arity
  static_assert(not _is_applicable<decltype(sum_two), int>::value);
  SUCCEED();
}

TEST_CASE("_is_applicable_r", "[functional][is_applicable_r]")
{
  using fn::detail::_is_applicable_r;
  // exact return type
  static_assert(_is_applicable_r<double, decltype(sum_two), int, double>::value);
  // convertible return type
  static_assert(_is_applicable_r<long, decltype(sum_two), int, double>::value);
  // pack arg dispatches through pack::apply_r
  static_assert(_is_applicable_r<double, decltype(sum_two), fn::pack<int, double> &>::value);
  // negative: non-convertible return type
  static_assert(not _is_applicable_r<int *, decltype(sum_two), int, double>::value);
  SUCCEED();
}

TEST_CASE("_is_nothrow_applicable", "[functional][is_nothrow_applicable]")
{
  using fn::detail::_is_nothrow_applicable;
  static_assert(not _is_nothrow_applicable<decltype(sum_two), int, double>::value); // sum_two can throw
  static_assert(_is_nothrow_applicable<decltype([]() noexcept { return 0; })>::value);
  static_assert(not _is_nothrow_applicable<decltype([]() { return 0; })>::value);
  static_assert(not _is_nothrow_applicable<decltype([](int) noexcept { return 0; })>::value); // not applicable at all

  // it composes through the dispatch: a pack answers for the call over its elements, a sum for the
  // call over every alternative, since which one runs is only known at run time
  constexpr auto nothrow_generic = [](auto &&...) noexcept { return 0; };
  constexpr auto throwing_generic = [](auto &&...) { return 0; };
  static_assert(_is_nothrow_applicable<decltype(nothrow_generic), fn::pack<int, bool> &>::value);
  static_assert(not _is_nothrow_applicable<decltype(throwing_generic), fn::pack<int, bool> &>::value);
  static_assert(_is_nothrow_applicable<decltype(nothrow_generic), fn::sum<bool, int> &>::value);
  static_assert(not _is_nothrow_applicable<decltype(throwing_generic), fn::sum<bool, int> &>::value);

  // one throwing alternative is enough
  constexpr auto mixed = fn::overload{[](int &) noexcept { return 0; }, [](bool &) { return 0; }};
  static_assert(not _is_nothrow_applicable<decltype(mixed), fn::sum<bool, int> &>::value);
  SUCCEED();
}

TEST_CASE("_is_nothrow_applicable_r", "[functional][is_nothrow_applicable_r]")
{
  using fn::detail::_is_nothrow_applicable_r;
  static_assert(not _is_nothrow_applicable_r<double, decltype(sum_two), int, double>::value);
  static_assert(_is_nothrow_applicable_r<int, decltype([]() noexcept { return 0; })>::value);
  static_assert(not _is_nothrow_applicable_r<int, decltype([]() { return 0; })>::value);
  static_assert(not _is_nothrow_applicable_r<int *, decltype([]() noexcept { return 0; })>::value); // not convertible

  constexpr auto nothrow_generic = [](auto &&...) noexcept { return 0; };
  static_assert(_is_nothrow_applicable_r<int, decltype(nothrow_generic), fn::pack<int, bool> &>::value);
  static_assert(_is_nothrow_applicable_r<int, decltype(nothrow_generic), fn::sum<bool, int> &>::value);
  SUCCEED();
}

TEST_CASE("_apply", "[functional][apply]")
{
  using fn::detail::_apply;
  // plain invocation
  static_assert(_apply(sum_two, 1, 0.5) == 1.5);
  static_assert(std::same_as<decltype(_apply(sum_two, 1, 0.5)), double>);
  // pack arg: dispatches through pack::apply
  constexpr fn::pack<int, double> p{2, 0.25};
  static_assert(_apply(sum_two, p) == 2.25);
  SUCCEED();
}

TEST_CASE("_apply_r", "[functional][apply_r]")
{
  using fn::detail::_apply_r;
  // return-type conversion
  static_assert(_apply_r<long>(sum_two, 1, 0.5) == 1);
  static_assert(std::same_as<decltype(_apply_r<long>(sum_two, 1, 0.5)), long>);
  // pack arg: dispatches through pack::apply_r
  constexpr fn::pack<int, double> p{3, 0.5};
  static_assert(_apply_r<int>(sum_two, p) == 3);
  SUCCEED();
}

TEST_CASE("_typelist_applicable", "[functional][typelist_applicable]")
{
  using fn::detail::_typelist_applicable;

  // satisfied when Fn is applicable with each typelist element under matching qualifier
  static_assert(_typelist_applicable<decltype([](auto) {}), types<int, double> &>);
  static_assert(_typelist_applicable<decltype([](auto const &) {}), types<int, double> const &>);
  static_assert(_typelist_applicable<decltype([](auto &&) {}), types<int, double> &&>);

  // extra trailing args are forwarded after the element type
  static_assert(_typelist_applicable<decltype([](auto, int) {}), types<int, double> &, int>);

  // negative: one element is not applicable
  static_assert(not _typelist_applicable<decltype([](int) {}), types<int, char const *> &>);
  // negative: T is not a typelist (no Tpl<Ts...> match)
  static_assert(not _typelist_applicable<decltype([](auto) {}), int>);
  SUCCEED();
}

TEST_CASE("_typelist_applicable_r", "[functional][typelist_applicable_r]")
{
  using fn::detail::_typelist_applicable_r;

  // satisfied when each invocation's result converts to R
  static_assert(_typelist_applicable_r<long, decltype([](auto v) { return v; }), types<int, short> &>);
  // negative: result of one element does not convert to R
  static_assert(not _typelist_applicable_r<int *, decltype([](auto v) { return v; }), types<int, short> &>);
  SUCCEED();
}
