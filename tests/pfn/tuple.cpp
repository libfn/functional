// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "catch2/catch_test_macros.hpp"

#ifndef PFN_TEST_NESTED

#include <pfn/tuple.hpp>

namespace subject = pfn;

#endif
// When nested via PFN_TEST_NESTED (e.g. tuple_validation.cpp), the wrapper TU already includes
// the necessary header(s) and defines the `subject` namespace alias to select the right set of
// entities as the subject under test. A namespace alias rather than the sibling TUs'
// using-declarations: an unqualified `apply` on std arguments would find C++20's std::apply
// through ADL, whose deduced return type is a hard error - not a substitution failure - for
// every negative probe below. That is the defect P1317R2 fixes, and qualification dodges it.

#include <catch2/catch_all.hpp>

#include <array>
#include <complex>
#include <concepts>
#include <memory>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
template <typename Fn, typename Tuple>
concept has_apply_result = requires { typename subject::apply_result_t<Fn, Tuple>; };

template <typename Fn, typename Tuple>
concept can_apply = requires(Fn &&fn, Tuple &&t) { subject::apply(static_cast<Fn &&>(fn), static_cast<Tuple &&>(t)); };

template <typename T> constexpr bool has_tuple_size = requires { std::tuple_size<T>::value; };

// Models the tuple protocol without being one of [tuple.like]'s enumerated specializations:
// the boundary witness that tuple-like is an enumeration, not the protocol.
struct protocol_t {
  int v;
};
template <std::size_t I> constexpr int get(protocol_t const &p) noexcept { return p.v; }
} // anonymous namespace

template <> struct std::tuple_size<protocol_t> : std::integral_constant<std::size_t, 1> {};
template <> struct std::tuple_element<0, protocol_t> {
  using type = int;
};

namespace {
using F2 = int (*)(int, int);
constexpr auto quiet = [](int a, int b) noexcept { return a + b; };
constexpr auto loud = [](int a, int b) { return a + b; };
struct rval_only_t {
  void operator()(int &&) const noexcept;
};
} // anonymous namespace

TEST_CASE("is_applicable", "[tuple][polyfill][is_applicable]")
{
  SECTION("enumerated tuple-like types")
  {
    static_assert(subject::is_applicable_v<F2, std::tuple<int, int>>);
    static_assert(subject::is_applicable_v<F2, std::pair<int, int>>);
    static_assert(subject::is_applicable_v<F2, std::array<int, 2>>);
    static_assert(subject::is_applicable_v<F2, std::array<int, 2> const &>);
    using sub_t = std::ranges::subrange<std::vector<int>::iterator>;
    struct from_iters_t {
      void operator()(std::vector<int>::iterator, std::vector<int>::iterator) const;
    };
    static_assert(subject::is_applicable_v<from_iters_t, sub_t>);

    static_assert(not subject::is_applicable_v<F2, std::tuple<int>>);         // arity mismatch
    static_assert(not subject::is_applicable_v<F2, std::tuple<char *, int>>); // type mismatch
    static_assert(not subject::is_applicable_v<F2, std::array<char *, 2>>);
    SUCCEED();
  }

  SECTION("not tuple-like")
  {
    static_assert(not subject::is_applicable_v<F2, int>);
    static_assert(not subject::is_applicable_v<int (*)(), void>);

    // the tuple protocol does not qualify: [tuple.like] is an enumeration
    static_assert(has_tuple_size<protocol_t>);
    static_assert(not subject::is_applicable_v<void (*)(int), protocol_t>);
    SUCCEED();
  }

  SECTION("value categories")
  {
    // the elements carry the tuple's value category, as get does
    static_assert(subject::is_applicable_v<rval_only_t, std::tuple<int>>);
    static_assert(subject::is_applicable_v<rval_only_t, std::tuple<int> &&>);
    static_assert(not subject::is_applicable_v<rval_only_t, std::tuple<int> &>);
    static_assert(not subject::is_applicable_v<rval_only_t, std::tuple<int> const &>);
    static_assert(not subject::is_applicable_v<rval_only_t, std::tuple<int> const &&>);
    SUCCEED();
  }

  SECTION("complex")
  {
    // enumerated tuple-like, but its tuple protocol is C++26 (P2819): the trait's answer opens
    // with the underlying standard library
    struct f2d_t {
      void operator()(double, double) const;
    };
    static_assert(subject::is_applicable_v<f2d_t, std::complex<double> &> == has_tuple_size<std::complex<double>>);
    static_assert(not subject::is_applicable_v<F2, std::complex<double> &>); // wrong callable regardless
    SUCCEED();
  }
}

TEST_CASE("is_nothrow_applicable", "[tuple][polyfill][is_nothrow_applicable]")
{
  static_assert(subject::is_nothrow_applicable_v<decltype(quiet), std::tuple<int, int>>);
  static_assert(not subject::is_nothrow_applicable_v<decltype(loud), std::tuple<int, int>>);
  // false, not an error, where is_applicable is false
  static_assert(not subject::is_nothrow_applicable_v<decltype(quiet), std::tuple<int>>);
  static_assert(not subject::is_nothrow_applicable_v<decltype(quiet), int>);
  SUCCEED();
}

TEST_CASE("apply_result", "[tuple][polyfill][apply_result]")
{
  static_assert(std::same_as<subject::apply_result_t<F2, std::tuple<int, int>>, int>);
  static_assert(std::same_as<subject::apply_result_t<decltype(quiet), std::pair<int, int>>, int>);

  struct ref_result_t {
    int &operator()(int &) const;
  };
  static_assert(std::same_as<subject::apply_result_t<ref_result_t, std::tuple<int &>>, int &>);

  // SFINAE-friendly: no member type, no hard error - the reason P1317R2 exists
  static_assert(has_apply_result<F2, std::tuple<int, int>>);
  static_assert(not has_apply_result<F2, std::tuple<int>>);
  static_assert(not has_apply_result<F2, int>);
  static_assert(not has_apply_result<F2, protocol_t>);
  SUCCEED();
}

TEST_CASE("apply", "[tuple][polyfill][apply]")
{
  SECTION("basic")
  {
    CHECK(subject::apply(quiet, std::tuple{2, 3}) == 5);
    CHECK(subject::apply(quiet, std::pair{2, 3}) == 5);
    CHECK(subject::apply(quiet, std::array{2, 3}) == 5);
    static_assert(subject::apply(quiet, std::tuple{2, 3}) == 5);
    static_assert(subject::apply(quiet, std::pair{2, 3}) == 5);
    static_assert(subject::apply(quiet, std::array{2, 3}) == 5);

    constexpr auto lval = [] {
      auto t = std::tuple{2, 3};
      return subject::apply(quiet, t) == 5;
    };
    CHECK(lval());
    static_assert(lval());

    constexpr auto empty = [] { return subject::apply([]() { return 7; }, std::tuple<>{}) == 7; };
    CHECK(empty());
    static_assert(empty());
  }

  SECTION("value categories")
  {
    // a reference result refers into the tuple's element
    constexpr auto through = [] {
      int x = 1;
      std::tuple<int &> t{x};
      subject::apply([](int &r) -> int & { return r; }, t) = 7;
      return x == 7;
    };
    CHECK(through());
    static_assert(through());

    // move-only elements are delivered from an rvalue tuple
    auto q = subject::apply([](std::unique_ptr<int> &&v) { return std::move(v); },
                            std::tuple<std::unique_ptr<int>>{std::make_unique<int>(42)});
    CHECK(q != nullptr);
    CHECK(*q == 42);
  }

  SECTION("subrange")
  {
    std::vector<int> v{1, 2, 3};
    auto const sub = std::ranges::subrange(v.begin(), v.end());
    CHECK(subject::apply([](auto b, auto e) { return static_cast<int>(e - b); }, sub) == 3);
  }

  SECTION("noexcept")
  {
    static_assert(noexcept(subject::apply(quiet, std::declval<std::tuple<int, int> &>()))); // required
    static_assert(not noexcept(subject::apply(loud, std::declval<std::tuple<int, int> &>())));
    SUCCEED();
  }

  SECTION("constraints")
  {
    static_assert(can_apply<decltype(quiet), std::tuple<int, int>>);
    static_assert(not can_apply<decltype(quiet), std::tuple<int>>); // a substitution failure, not an error
    static_assert(not can_apply<decltype(quiet), int>);
    static_assert(not can_apply<decltype(quiet), protocol_t>);
    SUCCEED();
  }
}
