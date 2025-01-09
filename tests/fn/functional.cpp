// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/functional.hpp>
#include <fn/pack.hpp>
#include <fn/sum.hpp>
#include <fn/utility.hpp>

#include <catch2/catch_all.hpp>

#include <type_traits>
#include <utility>

TEST_CASE("invoke multidispatch", "[pack][sum][invoke][invoke_r]")
{
  using namespace ::fn::detail;
  using ::fn::pack;
  using ::fn::sum;

  constexpr auto fn = [](auto &&...a) { return (0 + ... + static_cast<int>(a)); };

  static_assert(fn::invoke(fn) == 0);
  static_assert(fn::invoke(fn, 1, 2) == 3);
  static_assert(fn::invoke(fn, pack{1, 2}) == 1 + 2);
  static_assert(fn::invoke(fn, pack{1, 2}, 3) == 1 + 2 + 3);
  static_assert(fn::invoke(fn, 1, pack{2, 3, 5}) == 1 + 2 + 3 + 5);
  static_assert(fn::invoke(fn, sum<bool, int>{2}) == 2);
  static_assert(fn::invoke(fn, sum<bool, int>{2}, 3) == 2 + 3);
  static_assert(fn::invoke(fn, 2, sum<bool, int>{3}) == 2 + 3);
  static_assert(fn::invoke(fn, 2, sum<bool, int>{3}, pack{2, 3, 5}) == 2 + 3 + 2 + 3 + 5);
  static_assert(fn::invoke(fn, 2, pack{3, 5}, 7, sum<bool, int>{2}) == 2 + 3 + 5 + 7 + 2);
  static_assert(fn::invoke(fn, sum<bool, int>{3}, 2, pack{2, 3, 5}) == 2 + 3 + 2 + 3 + 5);
  static_assert(fn::invoke(fn, sum<bool, int>{3}, pack{2, 3, 5}, 2) == 2 + 3 + 2 + 3 + 5);
  static_assert(fn::invoke(fn, pack{3, 5}, 2, 7, sum<bool, int>{2}) == 2 + 3 + 5 + 7 + 2);
  static_assert(fn::invoke(fn, pack{3, 5}, sum<bool, int>{2}, 2, 7) == 2 + 3 + 5 + 7 + 2);

  static_assert(fn::invoke_r<long>(fn) == 0);
  static_assert(fn::invoke_r<long>(fn, 1, 2) == 3);
  static_assert(fn::invoke_r<long>(fn, pack{1, 2}) == 1 + 2);
  static_assert(fn::invoke_r<long>(fn, pack{1, 2}, 3) == 1 + 2 + 3);
  static_assert(fn::invoke_r<long>(fn, 1, pack{2, 3, 5}) == 1 + 2 + 3 + 5);
  static_assert(fn::invoke_r<long>(fn, sum<bool, int>{2}) == 2);
  static_assert(fn::invoke_r<long>(fn, sum<bool, int>{2}, 3) == 2 + 3);
  static_assert(fn::invoke_r<long>(fn, 2, sum<bool, int>{3}) == 2 + 3);
  static_assert(fn::invoke_r<long>(fn, 2, sum<bool, int>{3}, pack{2, 3, 5}) == 2 + 3 + 2 + 3 + 5);
  static_assert(fn::invoke_r<long>(fn, 2, pack{3, 5}, 7, sum<bool, int>{2}) == 2 + 3 + 5 + 7 + 2);
  static_assert(fn::invoke_r<long>(fn, sum<bool, int>{3}, 2, pack{2, 3, 5}) == 2 + 3 + 2 + 3 + 5);
  static_assert(fn::invoke_r<long>(fn, sum<bool, int>{3}, pack{2, 3, 5}, 2) == 2 + 3 + 2 + 3 + 5);
  static_assert(fn::invoke_r<long>(fn, pack{3, 5}, 2, 7, sum<bool, int>{2}) == 2 + 3 + 5 + 7 + 2);
  static_assert(fn::invoke_r<long>(fn, pack{3, 5}, sum<bool, int>{2}, 2, 7) == 2 + 3 + 5 + 7 + 2);
}

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

TEST_CASE("invoke_r pack", "[invoke_r][pack]")
{
  using fn::invoke_r;
  using fn::pack;

  constexpr auto fn = [](int i, double j) -> int { return i * 100 + (int)j; };
  pack<int, double> p{3, 14.15};

  CHECK(invoke_r<double>(fn, p) == 314.0);
  CHECK(invoke_r<double>(fn, std::as_const(p)) == 314.0);
  CHECK(invoke_r<double>(fn, std::move(std::as_const(p))) == 314.0);
  CHECK(invoke_r<double>(fn, std::move(p)) == 314.0);
}

TEST_CASE("invoke sum", "[invoke][sum]")
{
  using fn::invoke;
  using fn::overload;
  using fn::sum;

  constexpr auto fn = overload{[](int i) -> int { return i * 10; }, [](double) -> int { throw 0; }};
  sum<double, int> p{3};

  CHECK(invoke(fn, p) == 30);
  CHECK(invoke(fn, std::as_const(p)) == 30);
  CHECK(invoke(fn, std::move(std::as_const(p))) == 30);
  CHECK(invoke(fn, std::move(p)) == 30);
}

TEST_CASE("invoke_r sum", "[invoke_r][sum]")
{
  using fn::invoke_r;
  using fn::overload;
  using fn::sum;

  constexpr auto fn = overload{[](int) -> bool { throw 0; }, [](double j) -> short { return j * 100; }};
  sum<double, int> p{14.15};

  CHECK(invoke_r<int>(fn, p) == 1415);
  CHECK(invoke_r<int>(fn, std::as_const(p)) == 1415);
  CHECK(invoke_r<int>(fn, std::move(std::as_const(p))) == 1415);
  CHECK(invoke_r<int>(fn, std::move(p)) == 1415);
}
