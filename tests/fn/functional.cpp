// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/copack.hpp>
#include <fn/functional.hpp>
#include <fn/pack.hpp>
#include <fn/utility.hpp>
#include <pfn/tuple.hpp>

#include <catch2/catch_all.hpp>

#include <array>
#include <tuple>
#include <type_traits>
#include <utility>

TEST_CASE("apply multidispatch", "[pack][copack][apply][apply_r]")
{
  using namespace ::fn::detail;
  using ::fn::copack;
  using ::fn::pack;

  constexpr auto fn = [](auto &&...a) { return (0 + ... + static_cast<int>(a)); };

  static_assert(fn::apply(fn) == 0);
  static_assert(fn::apply(fn, 1, 2) == 3);
  static_assert(fn::apply(fn, pack{1, 2}) == 1 + 2);
  static_assert(fn::apply(fn, pack{1, 2}, 3) == 1 + 2 + 3);
  static_assert(fn::apply(fn, 1, pack{2, 3, 5}) == 1 + 2 + 3 + 5);
  static_assert(fn::apply(fn, copack<bool, int>{2}) == 2);
  static_assert(fn::apply(fn, copack<bool, int>{2}, 3) == 2 + 3);
  static_assert(fn::apply(fn, 2, copack<bool, int>{3}) == 2 + 3);
  static_assert(fn::apply(fn, 2, copack<bool, int>{3}, pack{2, 3, 5}) == 2 + 3 + 2 + 3 + 5);
  static_assert(fn::apply(fn, 2, pack{3, 5}, 7, copack<bool, int>{2}) == 2 + 3 + 5 + 7 + 2);
  static_assert(fn::apply(fn, copack<bool, int>{3}, 2, pack{2, 3, 5}) == 2 + 3 + 2 + 3 + 5);
  static_assert(fn::apply(fn, copack<bool, int>{3}, pack{2, 3, 5}, 2) == 2 + 3 + 2 + 3 + 5);
  static_assert(fn::apply(fn, pack{3, 5}, 2, 7, copack<bool, int>{2}) == 2 + 3 + 5 + 7 + 2);
  static_assert(fn::apply(fn, pack{3, 5}, copack<bool, int>{2}, 2, 7) == 2 + 3 + 5 + 7 + 2);

  static_assert(fn::apply_r<long>(fn) == 0);
  static_assert(fn::apply_r<long>(fn, 1, 2) == 3);
  static_assert(fn::apply_r<long>(fn, pack{1, 2}) == 1 + 2);
  static_assert(fn::apply_r<long>(fn, pack{1, 2}, 3) == 1 + 2 + 3);
  static_assert(fn::apply_r<long>(fn, 1, pack{2, 3, 5}) == 1 + 2 + 3 + 5);
  static_assert(fn::apply_r<long>(fn, copack<bool, int>{2}) == 2);
  static_assert(fn::apply_r<long>(fn, copack<bool, int>{2}, 3) == 2 + 3);
  static_assert(fn::apply_r<long>(fn, 2, copack<bool, int>{3}) == 2 + 3);
  static_assert(fn::apply_r<long>(fn, 2, copack<bool, int>{3}, pack{2, 3, 5}) == 2 + 3 + 2 + 3 + 5);
  static_assert(fn::apply_r<long>(fn, 2, pack{3, 5}, 7, copack<bool, int>{2}) == 2 + 3 + 5 + 7 + 2);
  static_assert(fn::apply_r<long>(fn, copack<bool, int>{3}, 2, pack{2, 3, 5}) == 2 + 3 + 2 + 3 + 5);
  static_assert(fn::apply_r<long>(fn, copack<bool, int>{3}, pack{2, 3, 5}, 2) == 2 + 3 + 2 + 3 + 5);
  static_assert(fn::apply_r<long>(fn, pack{3, 5}, 2, 7, copack<bool, int>{2}) == 2 + 3 + 5 + 7 + 2);
  static_assert(fn::apply_r<long>(fn, pack{3, 5}, copack<bool, int>{2}, 2, 7) == 2 + 3 + 5 + 7 + 2);
}

TEST_CASE("apply_result pack", "[apply_result][pack]")
{
  using fn::apply_result;
  using fn::apply_result_t;
  using fn::pack;

  constexpr pack<int, double> p{3, 14.15};
  constexpr auto fn1 = [](int i, double j) -> int { return i * 100 + (int)j; };
  static_assert(std::is_same_v<apply_result<decltype(fn1), decltype(p)>::type, int>);
  static_assert(std::is_same_v<apply_result_t<decltype(fn1), decltype(p)>, int>);
  SUCCEED();
}

TEST_CASE("is_applicable pack", "[is_applicable][pack]")
{
  using fn::is_applicable;
  using fn::is_applicable_v;
  using fn::pack;

  constexpr pack<int, double> p{3, 14.15};
  constexpr auto fn1 = [](int i, double j) -> int { return i * 100 + (int)j; };
  static_assert(is_applicable<decltype(fn1), decltype(p)>::value);
  static_assert(is_applicable_v<decltype(fn1), decltype(p)>);

  constexpr auto fn2 = [](int, double &) -> int { throw 0; };
  static_assert(not is_applicable<decltype(fn2), decltype(p)>::value);
  static_assert(not is_applicable_v<decltype(fn2), decltype(p)>);
  SUCCEED();
}

TEST_CASE("is_applicable copack", "[is_applicable][copack]")
{
  using fn::copack;
  using fn::is_applicable;
  using fn::is_applicable_v;
  using fn::overload;

  constexpr copack<double, int> p{3};
  constexpr auto fn1 = overload{[](int i) -> int { return i * 100; }, [](double j) -> int { return (int)j; }};
  static_assert(is_applicable<decltype(fn1), decltype(p)>::value);
  static_assert(is_applicable_v<decltype(fn1), decltype(p)>);
  constexpr auto fn2 = [](int &) -> int { throw 0; };
  static_assert(not is_applicable<decltype(fn2), decltype(p)>::value);
  static_assert(not is_applicable_v<decltype(fn2), decltype(p)>);
}

TEST_CASE("is_applicable_r pack", "[is_applicable_r][pack]")
{
  using fn::is_applicable_r;
  using fn::is_applicable_r_v;
  using fn::pack;

  constexpr pack<int, double> p{3, 14.15};
  constexpr auto fn1 = [](int i, double j) -> int { return i * 100 + (int)j; };
  static_assert(is_applicable_r<bool, decltype(fn1), decltype(p)>::value);
  static_assert(is_applicable_r_v<bool, decltype(fn1), decltype(p)>);
  static_assert(not is_applicable_r<int *, decltype(fn1), decltype(p)>::value);
  static_assert(not is_applicable_r_v<int *, decltype(fn1), decltype(p)>);
  constexpr auto fn2 = [](int, double &) -> int { throw 0; };
  static_assert(not is_applicable_r<bool, decltype(fn2), decltype(p)>::value);
  static_assert(not is_applicable_r_v<bool, decltype(fn2), decltype(p)>);
}

TEST_CASE("is_applicable_r copack", "[is_applicable_r][copack]")
{
  using fn::copack;
  using fn::is_applicable_r;
  using fn::is_applicable_r_v;
  using fn::overload;

  constexpr copack<double, int> p{3};
  constexpr auto fn1 = overload{[](int i) -> int { return i * 100; }, [](double j) -> int { return (int)j; }};
  static_assert(is_applicable_r<bool, decltype(fn1), decltype(p)>::value);
  static_assert(is_applicable_r_v<bool, decltype(fn1), decltype(p)>);
  static_assert(not is_applicable_r<int *, decltype(fn1), decltype(p)>::value);
  static_assert(not is_applicable_r_v<int *, decltype(fn1), decltype(p)>);
  constexpr auto fn2 = [](int &) -> int { throw 0; };
  static_assert(not is_applicable_r<decltype(fn2), decltype(p)>::value);
  static_assert(not is_applicable_r_v<decltype(fn2), decltype(p)>);
  SUCCEED();
}

TEST_CASE("apply polyfill", "[apply][polyfill]")
{
  using fn::apply;
  struct Xint final {
    int value;

    static auto fn(Xint const &self) noexcept -> int { return self.value; }
    auto fn1() & noexcept -> int { return value + 1; }
    auto fn2() const & noexcept -> int { return value + 2; }
    auto fn3() && noexcept -> int { return value + 3; }
    auto fn4() const && noexcept -> int { return value + 4; }
  };

  Xint v{12};
  CHECK(apply(Xint::fn, v) == 12);
  CHECK(apply(&Xint::fn1, v) == 13);
  CHECK(apply(&Xint::fn2, v) == 14);
  CHECK(apply(&Xint::fn2, std::as_const(v)) == 14);
  CHECK(apply(&Xint::fn4, std::move(v)) == 16);
  CHECK(apply(&Xint::fn4, std::move(std::as_const(v))) == 16);
  CHECK(apply(&Xint::fn3, std::move(v)) == 15);
}

TEST_CASE("apply pack", "[apply][pack]")
{
  using fn::apply;
  using fn::pack;

  constexpr auto fn = [](int i, double j) -> int { return i * 100 + (int)j; };
  pack<int, double> p{3, 14.15};

  CHECK(apply(fn, p) == 314);
  CHECK(apply(fn, std::as_const(p)) == 314);
  CHECK(apply(fn, std::move(std::as_const(p))) == 314);
  CHECK(apply(fn, std::move(p)) == 314);
}

TEST_CASE("apply_r pack", "[apply_r][pack]")
{
  using fn::apply_r;
  using fn::pack;

  constexpr auto fn = [](int i, double j) -> int { return i * 100 + (int)j; };
  pack<int, double> p{3, 14.15};

  CHECK(apply_r<double>(fn, p) == 314.0);
  CHECK(apply_r<double>(fn, std::as_const(p)) == 314.0);
  CHECK(apply_r<double>(fn, std::move(std::as_const(p))) == 314.0);
  CHECK(apply_r<double>(fn, std::move(p)) == 314.0);
}

TEST_CASE("apply copack", "[apply][copack]")
{
  using fn::apply;
  using fn::copack;
  using fn::overload;

  constexpr auto fn = overload{[](int i) -> int { return i * 10; }, [](double) -> int { throw 0; }};
  copack<double, int> p{3};

  CHECK(apply(fn, p) == 30);
  CHECK(apply(fn, std::as_const(p)) == 30);
  CHECK(apply(fn, std::move(std::as_const(p))) == 30);
  CHECK(apply(fn, std::move(p)) == 30);
}

TEST_CASE("apply_r copack", "[apply_r][copack]")
{
  using fn::apply_r;
  using fn::copack;
  using fn::overload;

  constexpr auto fn = overload{[](int) -> bool { throw 0; }, [](double j) -> short { return j * 100; }};
  copack<double, int> p{14.15};

  CHECK(apply_r<int>(fn, p) == 1415);
  CHECK(apply_r<int>(fn, std::as_const(p)) == 1415);
  CHECK(apply_r<int>(fn, std::move(std::as_const(p))) == 1415);
  CHECK(apply_r<int>(fn, std::move(p)) == 1415);
}

TEST_CASE("is_nothrow_applicable", "[is_nothrow_applicable][is_nothrow_applicable_r][apply][noexcept]")
{
  constexpr auto fnNothrow = [](int i) noexcept -> int { return i; };
  constexpr auto fnThrows = [](int i) noexcept(false) -> int { return i; };

  // over a plain argument the traits agree with their std counterparts
  static_assert(std::is_nothrow_invocable_v<decltype(fnNothrow), int>);
  static_assert(fn::is_nothrow_applicable_v<decltype(fnNothrow), int>);
  static_assert(not fn::is_nothrow_applicable_v<decltype(fnThrows), int>);

  static_assert(std::is_nothrow_invocable_r_v<long, decltype(fnNothrow), int>);
  static_assert(fn::is_nothrow_applicable_r_v<long, decltype(fnNothrow), int>);

  // The pack and copack dispatch paths have no std counterpart to fall back on, which is why the traits
  // exist at all: they answer by asking the apply chain itself - a pack for the call over its
  // elements, a copack for the call over every alternative.
  static_assert(fn::is_nothrow_applicable_v<decltype(fnNothrow), fn::pack<int>>);
  static_assert(fn::is_nothrow_applicable_v<decltype(fnNothrow), fn::copack<int>>);
  static_assert(not fn::is_nothrow_applicable_v<decltype(fnThrows), fn::pack<int>>);
  static_assert(not fn::is_nothrow_applicable_v<decltype(fnThrows), fn::copack<int>>);

  // so fn::apply propagates what the callable promises
  static_assert(fn::is_applicable_v<decltype(fnNothrow), int>);
  static_assert(noexcept(fn::apply(fnNothrow, 1)));
  static_assert(noexcept(fn::apply_r<long>(fnNothrow, 1)));
  static_assert(not noexcept(fn::apply(fnThrows, 1)));
  static_assert(not noexcept(fn::apply_r<long>(fnThrows, 1)));

  CHECK(fn::apply(fnNothrow, 1) == 1);
  CHECK(fn::apply_r<long>(fnNothrow, 1) == 1L);
}

TEST_CASE("apply tuple-like", "[apply][apply_r][tuple]")
{
  constexpr auto add2 = [](int a, int b) noexcept { return a + b; };
  constexpr auto arity = [](auto &&...args) noexcept { return (0 + ... + (static_cast<void>(args), 1)); };

  SECTION("std::apply's meaning on std::apply's domain")
  {
    // shared machinery with pfn::apply: the two agree by construction
    static_assert(fn::apply(add2, std::tuple{2, 3}) == pfn::apply(add2, std::tuple{2, 3}));
    static_assert(fn::apply(add2, std::pair{2, 3}) == 5);
    static_assert(fn::apply(add2, std::array{2, 3}) == 5);
    static_assert(fn::is_applicable_v<decltype(add2), std::tuple<int, int>>
                  == pfn::is_applicable_v<decltype(add2), std::tuple<int, int>>);
    static_assert(fn::is_applicable_v<decltype(add2), std::tuple<int>>
                  == pfn::is_applicable_v<decltype(add2), std::tuple<int>>); // both false: arity mismatch
    static_assert(fn::is_nothrow_applicable_v<decltype(add2), std::tuple<int, int>>);
    static_assert(std::same_as<fn::apply_result_t<decltype(add2), std::tuple<int, int>>, int>);

    // a generic callable unpacks, as std::apply does
    static_assert(fn::apply(arity, std::tuple{1, 2, 3}) == 3);

    CHECK(fn::apply(add2, std::tuple{20, 22}) == 42);
    CHECK(fn::apply(arity, std::tuple{1, 2, 3}) == 3);
  }

  SECTION("pass-whole survives where unpacking is not viable")
  {
    struct TakesTuple {
      constexpr int operator()(std::tuple<int, int> const &) const noexcept { return -1; }
    };
    static_assert(fn::apply(TakesTuple{}, std::tuple{1, 2}) == -1);
    // a tuple-like among further arguments has no unpacking route, and passes whole
    static_assert(fn::apply(arity, std::tuple{1, 2}, 0) == 2);
    CHECK(fn::apply(TakesTuple{}, std::tuple{1, 2}) == -1);
  }

  SECTION("elements are terminal")
  {
    // a copack element is handed over whole, exactly as std::apply hands it - never dispatched
    struct TakesSum {
      constexpr int operator()(fn::copack<int> const &) const noexcept { return 7; }
    };
    struct TakesAlternative {
      constexpr int operator()(int) const noexcept { return 8; }
    };
    static_assert(fn::apply(TakesSum{}, std::tuple<fn::copack<int>>{fn::copack<int>{1}}) == 7);
    static_assert(not fn::is_applicable_v<TakesAlternative, std::tuple<fn::copack<int>>>);
    static_assert(fn::is_applicable_v<TakesSum, std::tuple<fn::copack<int>>>
                  == pfn::is_applicable_v<TakesSum, std::tuple<fn::copack<int>>>);
    static_assert(fn::is_applicable_v<TakesAlternative, std::tuple<fn::copack<int>>>
                  == pfn::is_applicable_v<TakesAlternative, std::tuple<fn::copack<int>>>);
    // and a pack is not tuple-like: it keeps fn's own dispatch
    static_assert(fn::apply(arity, fn::pack{1, 2, 3}) == 3);

    CHECK(fn::apply(TakesSum{}, std::tuple<fn::copack<int>>{fn::copack<int>{1}}) == 7);
    CHECK(fn::apply(arity, fn::pack{1, 2, 3}) == 3);
  }

  SECTION("apply_r over the elements")
  {
    static_assert(fn::apply_r<long>(add2, std::tuple{2, 3}) == 5L);
    static_assert(std::same_as<decltype(fn::apply_r<long>(add2, std::tuple{2, 3})), long>);
    static_assert(fn::is_applicable_r_v<long, decltype(add2), std::tuple<int, int>>);
    static_assert(fn::is_applicable_r_v<void, decltype(add2), std::tuple<int, int>>);
    static_assert(not fn::is_applicable_r_v<char *, decltype(add2), std::tuple<int, int>>);
    CHECK(fn::apply_r<long>(add2, std::tuple{20, 22}) == 42L);
  }

  SECTION("noexcept follows the elements' call")
  {
    constexpr auto loud = [](int a, int b) { return a + b; };
    static_assert(not fn::is_nothrow_applicable_v<decltype(loud), std::tuple<int, int>>);
    static_assert(not noexcept(fn::apply(loud, std::declval<std::tuple<int, int> &>())));
    static_assert(noexcept(fn::apply(add2, std::declval<std::tuple<int, int> &>())));
    SUCCEED();
  }
}
