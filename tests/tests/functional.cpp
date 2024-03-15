// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/functional.hpp"
#include "functional/pack.hpp"
#include "functional/sum.hpp"
#include "functional/utility.hpp"

#include <catch2/catch_all.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

TEST_CASE("invoke_result pack", "[invoke_result][pack]")
{
  using fn::invoke_result;
  using fn::invoke_result_t;
  using fn::pack;

  constexpr pack<int, double> p{3, 14.15};
  constexpr auto fn1 = [](int i, double j) -> int { return i * 100 + (int)j; };
  static_assert(std::is_same_v<invoke_result<decltype(fn1), decltype(p)>::type, int>);
  static_assert(std::is_same_v<invoke_result_t<decltype(fn1), decltype(p)>, int>);
  SUCCEED();
}

TEST_CASE("invoke_result sum", "[invoke_result][sum]")
{
  using fn::invoke_result;
  using fn::invoke_result_t;
  using fn::overload;
  using fn::sum;

  constexpr sum<double, int> p{3};
  constexpr auto fn1 = overload{[](int i) -> int { return i * 100; }, [](double j) -> int { return (int)j; }};
  static_assert(std::is_same_v<invoke_result<decltype(fn1), decltype(p)>::type, int>);
  static_assert(std::is_same_v<invoke_result_t<decltype(fn1), decltype(p)>, int>);
  SUCCEED();
}

TEST_CASE("is_invocable pack", "[is_invocable][pack]")
{
  using fn::is_invocable;
  using fn::is_invocable_v;
  using fn::pack;

  constexpr pack<int, double> p{3, 14.15};
  constexpr auto fn1 = [](int i, double j) -> int { return i * 100 + (int)j; };
  static_assert(is_invocable<decltype(fn1), decltype(p)>::value);
  static_assert(is_invocable_v<decltype(fn1), decltype(p)>);
  constexpr auto fn2 = [](int, double &) -> int { throw 0; };
  static_assert(not is_invocable<decltype(fn2), decltype(p)>::value);
  static_assert(not is_invocable_v<decltype(fn2), decltype(p)>);
  SUCCEED();
}

TEST_CASE("is_invocable sum", "[is_invocable][sum]")
{
  using fn::is_invocable;
  using fn::is_invocable_v;
  using fn::overload;
  using fn::sum;

  constexpr sum<double, int> p{3};
  constexpr auto fn1 = overload{[](int i) -> int { return i * 100; }, [](double j) -> int { return (int)j; }};
  static_assert(is_invocable<decltype(fn1), decltype(p)>::value);
  static_assert(is_invocable_v<decltype(fn1), decltype(p)>);
  constexpr auto fn2 = [](int &) -> int { throw 0; };
  static_assert(not is_invocable<decltype(fn2), decltype(p)>::value);
  static_assert(not is_invocable_v<decltype(fn2), decltype(p)>);
}

TEST_CASE("is_invocable_r pack", "[is_invocable_r][pack]")
{
  using fn::is_invocable_r;
  using fn::is_invocable_r_v;
  using fn::pack;

  constexpr pack<int, double> p{3, 14.15};
  constexpr auto fn1 = [](int i, double j) -> int { return i * 100 + (int)j; };
  static_assert(is_invocable_r<bool, decltype(fn1), decltype(p)>::value);
  static_assert(is_invocable_r_v<bool, decltype(fn1), decltype(p)>);
  static_assert(not is_invocable_r<int *, decltype(fn1), decltype(p)>::value);
  static_assert(not is_invocable_r_v<int *, decltype(fn1), decltype(p)>);
  constexpr auto fn2 = [](int, double &) -> int { throw 0; };
  static_assert(not is_invocable_r<bool, decltype(fn2), decltype(p)>::value);
  static_assert(not is_invocable_r_v<bool, decltype(fn2), decltype(p)>);
}

TEST_CASE("is_invocable_r sum", "[is_invocable_r][sum]")
{
  using fn::is_invocable_r;
  using fn::is_invocable_r_v;
  using fn::overload;
  using fn::sum;

  constexpr sum<double, int> p{3};
  constexpr auto fn1 = overload{[](int i) -> int { return i * 100; }, [](double j) -> int { return (int)j; }};
  static_assert(is_invocable_r<bool, decltype(fn1), decltype(p)>::value);
  static_assert(is_invocable_r_v<bool, decltype(fn1), decltype(p)>);
  static_assert(not is_invocable_r<int *, decltype(fn1), decltype(p)>::value);
  static_assert(not is_invocable_r_v<int *, decltype(fn1), decltype(p)>);
  constexpr auto fn2 = [](int &) -> int { throw 0; };
  static_assert(not is_invocable_r<decltype(fn2), decltype(p)>::value);
  static_assert(not is_invocable_r_v<decltype(fn2), decltype(p)>);
  SUCCEED();
}

TEST_CASE("invoke polyfill", "[invoke][polyfill]")
{
  using fn::invoke;
  struct Xint final {
    int value;

    static auto fn(Xint const &self) noexcept -> int { return self.value; }
    auto fn1() & noexcept -> int { return value + 1; }
    auto fn2() const & noexcept -> int { return value + 2; }
    auto fn3() && noexcept -> int { return value + 3; }
    auto fn4() const && noexcept -> int { return value + 4; }
  };

  Xint v{12};
  CHECK(invoke(Xint::fn, v) == 12);
  CHECK(invoke(&Xint::fn1, v) == 13);
  CHECK(invoke(&Xint::fn2, v) == 14);
  CHECK(invoke(&Xint::fn2, std::as_const(v)) == 14);
  CHECK(invoke(&Xint::fn4, std::move(v)) == 16);
  CHECK(invoke(&Xint::fn4, std::move(std::as_const(v))) == 16);
  CHECK(invoke(&Xint::fn3, std::move(v)) == 15);
}

TEST_CASE("invoke pack", "[invoke][pack]")
{
  using fn::invoke;
  using fn::pack;

  constexpr auto fn = [](int i, double j) -> int { return i * 100 + (int)j; };
  pack<int, double> p{3, 14.15};

  CHECK(invoke(fn, p) == 314);
  CHECK(invoke(fn, std::as_const(p)) == 314);
  CHECK(invoke(fn, std::move(std::as_const(p))) == 314);
  CHECK(invoke(fn, std::move(p)) == 314);
}

TEST_CASE("invoke sum", "[invoke][sum]") {}
