// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "util/helper_types.hpp"

#include <fn/algebra.hpp>
#include <fn/copack.hpp>
#include <fn/expected.hpp>
#include <fn/just.hpp>
#include <fn/optional.hpp>
#include <fn/pack.hpp>

#include <catch2/catch_all.hpp>

#include <concepts>
#include <tuple>
#include <type_traits>
#include <utility>

#include <fn/detail/macro_begin.hpp>

namespace {
struct Alef final {
  int value;
};
struct Bet final {
  int value;
};
struct Gimel final {
  int value;
};
struct Heh final {
  int value;
};
struct Vav final {
  int value;
};
struct Zayn final {
  int value;
};

// Namespace scope: local classes cannot have member templates.
struct EfnLvalueNothrow {
  constexpr auto operator()(auto const &) & noexcept -> std::nullopt_t { return std::nullopt; }
  auto operator()(auto const &) && noexcept(false) -> std::nullopt_t { throw 0; }
};
struct EfnRvalueNothrow {
  auto operator()(auto const &) & noexcept(false) -> std::nullopt_t { throw 0; }
  constexpr auto operator()(auto const &) && noexcept -> std::nullopt_t { return std::nullopt; }
};
struct EfnLvalueOnly {
  constexpr auto operator()(auto const &) & noexcept -> std::nullopt_t { return std::nullopt; }
  auto operator()(auto const &) && -> std::nullopt_t = delete;
};
} // namespace

namespace {
constexpr auto join_witness = [](auto &&...v) -> int { return (0 + ... + v.value); };

struct join_via_optional final {
  template <typename R, typename LH, typename RH> static constexpr auto join(LH const &lh, RH const &rh)
  {
    constexpr auto efn = [](auto &&...) { return std::nullopt; };
    auto const r = fn::detail::_join<fn::optional>(fn::optional<LH>{lh}, fn::optional<RH>{rh}, efn);
    static_assert(std::is_same_v<decltype(r), fn::optional<R> const>);
    return r.value();
  }
};

struct join_via_operator final {
  template <typename R, typename LH, typename RH> static constexpr auto join(LH const &lh, RH const &rh)
  {
    auto const r = lh & rh;
    static_assert(std::is_same_v<decltype(r), R const>);
    return r;
  }
};

template <typename S> constexpr bool join_battery()
{
  using fn::copack;
  using fn::copack_for;
  using fn::pack;

  bool ok = true;
  {
    using R = copack_for<pack<Alef, Gimel, Heh>, pack<Alef, Gimel, Vav>, pack<Alef, Gimel, Zayn>, //
                         pack<Bet, Gimel, Heh>, pack<Bet, Gimel, Vav>, pack<Bet, Gimel, Zayn>>;
    auto const r = S::template join<R>(copack_for<pack<Alef, Gimel>, pack<Bet, Gimel>>{pack{Alef{3}, Gimel{14}}},
                                       copack<Heh, Vav, Zayn>{Vav{15}});
    ok = ok && r.template has_value<pack<Alef, Gimel, Vav>>() && r.apply(join_witness) == 3 + 14 + 15;
  }
  {
    using R = copack_for<pack<Alef, Gimel, Heh, Zayn>, pack<Alef, Gimel, Vav>, //
                         pack<Bet, Gimel, Heh, Zayn>, pack<Bet, Gimel, Vav>>;
    auto const r = S::template join<R>(copack_for<pack<Alef, Gimel>, pack<Bet, Gimel>>{pack{Alef{3}, Gimel{14}}},
                                       copack<pack<Heh, Zayn>, pack<Vav>>{pack{Vav{15}}});
    ok = ok && r.template has_value<pack<Alef, Gimel, Vav>>() && r.apply(join_witness) == 3 + 14 + 15;
  }
  {
    using R = copack_for<pack<Alef, Heh>, pack<Alef, Vav>, pack<Alef, Zayn>, pack<Bet, Heh>, pack<Bet, Vav>,
                         pack<Bet, Zayn>, pack<Gimel, Heh>, pack<Gimel, Vav>, pack<Gimel, Zayn>>;
    auto const r = S::template join<R>(copack_for<Alef, Bet, Gimel>{Gimel{3}}, copack<Heh, Vav, Zayn>{Vav{14}});
    ok = ok && r.template has_value<pack<Gimel, Vav>>() && r.apply(join_witness) == 3 + 14;
  }
  {
    using R = copack_for<pack<Alef, Heh, Zayn>, pack<Alef, Vav>, pack<Bet, Heh, Zayn>, //
                         pack<Bet, Vav>, pack<Gimel, Heh, Zayn>, pack<Gimel, Vav>>;
    auto const r = S::template join<R>(copack_for<Alef, Bet, Gimel>{Gimel{3}},
                                       copack<pack<Heh, Zayn>, pack<Vav>>{pack{Vav{14}}});
    ok = ok && r.template has_value<pack<Gimel, Vav>>() && r.apply(join_witness) == 3 + 14;
  }
  {
    using R = copack_for<pack<Alef, Gimel, Vav>, pack<Bet, Gimel, Vav>>;
    auto const r
        = S::template join<R>(copack_for<pack<Alef, Gimel>, pack<Bet, Gimel>>{pack{Alef{3}, Gimel{14}}}, Vav{15});
    ok = ok && r.template has_value<pack<Alef, Gimel, Vav>>() && r.apply(join_witness) == 3 + 14 + 15;
  }
  {
    using R = copack_for<pack<Alef, Gimel, Vav>, pack<Bet, Gimel, Vav>>;
    auto const r = S::template join<R>(copack_for<pack<Alef, Gimel>, pack<Bet, Gimel>>{pack{Alef{3}, Gimel{14}}},
                                       pack<Vav>{pack{Vav{15}}});
    ok = ok && r.template has_value<pack<Alef, Gimel, Vav>>() && r.apply(join_witness) == 3 + 14 + 15;
  }
  {
    using R = copack_for<pack<Alef, Vav>, pack<Bet, Vav>, pack<Gimel, Vav>>;
    auto const r = S::template join<R>(copack_for<Alef, Bet, Gimel>{Gimel{3}}, Vav{14});
    ok = ok && r.template has_value<pack<Gimel, Vav>>() && r.apply(join_witness) == 3 + 14;
  }
  {
    using R = copack_for<pack<Alef, Vav>, pack<Bet, Vav>, pack<Gimel, Vav>>;
    auto const r = S::template join<R>(copack_for<Alef, Bet, Gimel>{Gimel{3}}, pack<Vav>{pack{Vav{14}}});
    ok = ok && r.template has_value<pack<Gimel, Vav>>() && r.apply(join_witness) == 3 + 14;
  }
  {
    using R = copack<pack<Alef, Gimel, Heh>, pack<Alef, Gimel, Vav>, pack<Alef, Gimel, Zayn>>;
    auto const r = S::template join<R>(pack<Alef, Gimel>{pack{Alef{3}, Gimel{14}}}, copack<Heh, Vav, Zayn>{Vav{15}});
    ok = ok && r.template has_value<pack<Alef, Gimel, Vav>>() && r.apply(join_witness) == 3 + 14 + 15;
  }
  {
    using R = copack<pack<Alef, Gimel, Heh, Zayn>, pack<Alef, Gimel, Vav>>;
    auto const r = S::template join<R>(pack<Alef, Gimel>{pack{Alef{3}, Gimel{14}}},
                                       copack<pack<Heh, Zayn>, pack<Vav>>{pack{Vav{15}}});
    ok = ok && r.template has_value<pack<Alef, Gimel, Vav>>() && r.apply(join_witness) == 3 + 14 + 15;
  }
  {
    auto const r = S::template join<pack<Alef, Gimel, Vav>>(pack<Alef, Gimel>{pack{Alef{3}, Gimel{14}}}, Vav{15});
    ok = ok && r.apply(join_witness) == 3 + 14 + 15;
  }
  {
    auto const r = S::template join<pack<Alef, Gimel, Vav>>(pack<Alef, Gimel>{pack{Alef{3}, Gimel{14}}},
                                                            pack<Vav>{pack{Vav{15}}});
    ok = ok && r.apply(join_witness) == 3 + 14 + 15;
  }
  return ok;
}

// Scalar left operands are supported only by the carrier join.
constexpr bool join_value_lhs_battery()
{
  using fn::copack;
  using fn::pack;
  using S = join_via_optional;

  bool ok = true;
  {
    using R = copack<pack<Alef, Heh>, pack<Alef, Vav>, pack<Alef, Zayn>>;
    auto const r = S::join<R>(Alef{3}, copack<Heh, Vav, Zayn>{Vav{14}});
    ok = ok && r.template has_value<pack<Alef, Vav>>() && r.apply(join_witness) == 3 + 14;
  }
  {
    using R = copack<pack<Alef, Heh, Zayn>, pack<Alef, Vav>>;
    auto const r = S::join<R>(Alef{3}, copack<pack<Heh, Zayn>, pack<Vav>>{pack{Vav{14}}});
    ok = ok && r.template has_value<pack<Alef, Vav>>() && r.apply(join_witness) == 3 + 14;
  }
  {
    auto const r = S::join<pack<Alef, Vav>>(Alef{3}, Vav{14});
    ok = ok && r.apply(join_witness) == 3 + 14;
  }
  {
    auto const r = S::join<pack<Alef, Vav>>(Alef{3}, pack<Vav>{pack{Vav{14}}});
    ok = ok && r.apply(join_witness) == 3 + 14;
  }
  return ok;
}
} // namespace

TEMPLATE_TEST_CASE("join of copacks, packs and values", "[pack][copack][detail][optional][operator_and]",
                   join_via_optional, join_via_operator)
{
  static_assert(join_battery<TestType>());
  REQUIRE(join_battery<TestType>());
}

TEST_CASE("detail::_join with value operands", "[detail][pack][copack][optional]")
{
  static_assert(join_value_lhs_battery());
  REQUIRE(join_value_lhs_battery());
}

TEST_CASE("detail::_join with an error continuation", "[detail][optional][noexcept]")
{
  using Rh = fn::optional<int>;

  using fn::detail::_join;
  static_assert(noexcept(_join<fn::optional>(std::declval<Rh &>(), std::declval<Rh &>(), EfnLvalueNothrow{})));
  static_assert(not noexcept(_join<fn::optional>(std::declval<Rh &>(), std::declval<Rh &>(), EfnRvalueNothrow{})));
  static_assert(noexcept(_join<fn::optional>(std::declval<Rh &>(), std::declval<Rh &>(), EfnLvalueOnly{})));
  static_assert(
      noexcept(_join<fn::optional>(std::declval<Rh &>(), std::declval<Rh &>(), std::declval<EfnLvalueNothrow &>())));
  static_assert(not noexcept(
      _join<fn::optional>(std::declval<Rh &>(), std::declval<Rh &>(), std::declval<EfnRvalueNothrow &>())));

  CHECK(not _join<fn::optional>(Rh{std::nullopt}, Rh{12}, EfnLvalueNothrow{}).has_value());
  CHECK(not _join<fn::optional>(Rh{12}, Rh{std::nullopt}, EfnLvalueOnly{}).has_value());
  CHECK(_join<fn::optional>(Rh{3}, Rh{4}, EfnLvalueOnly{}).has_value());
  static_assert(not _join<fn::optional>(Rh{std::nullopt}, Rh{12}, EfnLvalueNothrow{}).has_value());
  static_assert(not _join<fn::optional>(Rh{12}, Rh{std::nullopt}, EfnLvalueOnly{}).has_value());
  static_assert(_join<fn::optional>(Rh{3}, Rh{4}, EfnLvalueOnly{}).has_value());
}

TEST_CASE("operator &", "[pack][copack][operator_and]")
{
  constexpr auto r1 = fn::as_copack(12) & 3 & 2.5 & fn::pack{0.5, true}
                      & fn::copack_for<bool, int, fn::pack<double, int>>(fn::pack{1.5, 12});
  static_assert(std::is_same_v<                                     //
                decltype(r1),                                       //
                fn::copack_for<                                     //
                    fn::pack<int, int, double, double, bool, bool>, //
                    fn::pack<int, int, double, double, bool, int>,  //
                    fn::pack<int, int, double, double, bool, double, int>> const>);
  static_assert(r1.apply([](auto &&...args) -> double { return (1 * ... * static_cast<double>(args)); })
                == 12. * 3 * 2.5 * 0.5 * 1 * 1.5 * 12);

  constexpr auto r2 = fn::conjoin(12, 3, 2.5, fn::pack{0.5, true},
                                  fn::copack_for<bool, int, fn::pack<double, int>>(fn::pack{1.5, 12}));
  static_assert(std::is_same_v<                                     //
                decltype(r2),                                       //
                fn::copack_for<                                     //
                    fn::pack<int, int, double, double, bool, bool>, //
                    fn::pack<int, int, double, double, bool, int>,  //
                    fn::pack<int, int, double, double, bool, double, int>> const>);
  static_assert(r2.apply([](auto &&...args) -> double { return (1 * ... * static_cast<double>(args)); })
                == 12. * 3 * 2.5 * 0.5 * 1 * 1.5 * 12);

  constexpr auto r3 = fn::as_copack(12) & fn::pack<std::tuple<int, int>>{std::tuple{1, 2}};
  static_assert(std::is_same_v<decltype(r3), fn::copack<fn::pack<int, std::tuple<int, int>>> const>);
  static_assert(r3.apply([](int i, std::tuple<int, int> const &t) { return i == 12 && std::get<0>(t) == 1; }));

  SECTION("noexcept")
  {
    static_assert(noexcept(std::declval<fn::pack<int> &>() & 2));
    static_assert(noexcept(std::declval<fn::copack<int> &>() & 2));
    SUCCEED();
  }

  SECTION("an element is held as itself")
  {
    constexpr auto held = [] {
      fn::pack<Atom> const src{{{Atom{7}}}};
      return fn::get<0>(src & fn::pack{9}).n;
    };
    static_assert(held() == 7);
    CHECK(held() == 7);
  }

  SECTION("a reference alternative stays a reference element")
  {
    // the product holds what the alternative holds, as a lifted lvalue does in as_pack; the factor is
    // the alternative's type, so a const copack contributes the same reference
    constexpr auto battery = [] {
      int x = 1;
      auto r = fn::copack<int &>{x} & fn::pack<long>{2};
      static_assert(std::is_same_v<decltype(r), fn::copack<fn::pack<int &, long>>>);
      auto l = fn::pack<long>{2} & fn::copack<int &>{x};
      static_assert(std::is_same_v<decltype(l), fn::copack<fn::pack<long, int &>>>);
      fn::copack<int &> const c{x};
      auto rc = c & fn::pack<long>{2};
      auto lc = fn::pack<long>{2} & c;
      auto cc = c & c;
      static_assert(std::is_same_v<decltype(rc), fn::copack<fn::pack<int &, long>>>);
      static_assert(std::is_same_v<decltype(lc), fn::copack<fn::pack<long, int &>>>);
      static_assert(std::is_same_v<decltype(cc), fn::copack<fn::pack<int &, int &>>>);
      return &fn::get<0>(fn::get(r)) == &x && &fn::get<1>(fn::get(l)) == &x && &fn::get<0>(fn::get(rc)) == &x
             && &fn::get<1>(fn::get(lc)) == &x && &fn::get<1>(fn::get(cc)) == &x;
    };
    static_assert(battery());
    CHECK(battery());
  }

  SECTION("the data fold takes data, never a carrier")
  {
    constexpr auto can = [](auto &&...args) { return requires { fn::conjoin(FWD(args)...); }; };
    static_assert(can(12, 2.5));
    static_assert(can(fn::pack{1, 2}, 3));
    static_assert(can(fn::as_copack(12), 3));
    static_assert(not can(fn::expected<int, bool>{1}, 3));
    static_assert(not can(12, fn::expected<int, bool>{1}));
    static_assert(not can(fn::pack{1, 2}, fn::expected<int, bool>{1}));
    static_assert(not can(fn::as_copack(12), fn::optional<int>{1}));
    static_assert(not can(fn::choice<int>{1}, 2));
    static_assert(can(fn::expected<int, bool>{1}));
    static_assert(fn::conjoin(fn::expected<int, bool>{1}) == fn::expected<int, bool>{1});
    SUCCEED();
  }

  SECTION("all carriers instead, and the fold is the carrier conjunction")
  {
    using EA = fn::expected<int, bool>;
    using EB = fn::expected<double, bool>;
    static_assert(std::same_as<decltype(fn::conjoin(std::declval<EA>(), std::declval<EB>())),
                               decltype(std::declval<EA>() & std::declval<EB>())>);
    constexpr auto is_1_2_5 = [](int i, double d) { return i == 1 && d == 2.5; };
    static_assert(fn::conjoin(EA{1}, EB{2.5}).value().apply(is_1_2_5));
    static_assert(fn::conjoin(EA{fn::unexpect, true}, EB{2.5}).error() == true);

    using EC = fn::expected<double, int>;
    static_assert(std::same_as<decltype(fn::conjoin(std::declval<EA>(), std::declval<EC>())),
                               fn::expected<fn::pack<int, double>, fn::copack_for<bool, int>>>);
    static_assert(fn::conjoin(EA{1}, EC{2.5}).value().apply(is_1_2_5));
    static_assert(fn::conjoin(EA{fn::unexpect, true}, EC{2.5}).error() == fn::copack_for<bool, int>{true});
    static_assert(fn::conjoin(EA{1}, EC{fn::unexpect, 7}).error() == fn::copack_for<bool, int>{7});
    CHECK(fn::conjoin(EA{1}, EC{2.5}).value().apply(is_1_2_5));
    CHECK(bool(fn::conjoin(EA{1}, EC{fn::unexpect, 7}).error() == fn::copack_for<bool, int>{7}));

    constexpr auto is_1_true_2 = [](int a, bool b, int c) { return a == 1 && b && c == 2; };
    static_assert(fn::conjoin(fn::just<int>{1}, fn::just<bool>{true}, fn::just<int>{2}).value().apply(is_1_true_2));
    constexpr auto is_1_true = [](int i, bool b) { return i == 1 && b; };
    static_assert(fn::conjoin(fn::optional<int>{1}, fn::optional<bool>{true}).value().apply(is_1_true));
    CHECK(fn::conjoin(fn::optional<int>{1}, fn::optional<bool>{true}).value().apply(is_1_true));
    static_assert(not fn::conjoin(fn::optional<int>{}, fn::optional<bool>{true}).has_value());

    constexpr auto can = [](auto &&...args) { return requires { fn::conjoin(FWD(args)...); }; };
    static_assert(can(fn::optional<int>{1}, fn::optional<bool>{true}));
    static_assert(not can(fn::optional<int>{1}, true));
    static_assert(not can(fn::optional<int>{1}, fn::expected<int, bool>{1}));
    SUCCEED();
  }
}

TEST_CASE("disjoin", "[disjoin][pack][expected][just]")
{
  enum Error : int { FileNotFound };
  using EA = fn::expected<int, Error>;
  using EB = fn::expected<bool, int>;

  static_assert(std::same_as<decltype(fn::disjoin(std::declval<EA>(), std::declval<EB>())),
                             decltype(std::declval<EA>() | std::declval<EB>())>);
  static_assert(fn::disjoin(EA{1}) == EA{1});
  static_assert(fn::disjoin(EA{::fn::unexpect, FileNotFound}, EB{true}) == fn::copack{true});
  static_assert(
      fn::disjoin(fn::just<void>{}, fn::just<void>{}, fn::just<int>{7}).apply([]([[maybe_unused]] auto &&...args) {
        return sizeof...(args);
      })
      == 0);
  CHECK(bool(fn::disjoin(EA{::fn::unexpect, FileNotFound}, EB{true})
             == fn::copack{true})); // bool(): Catch2 decomposition re-enters the == constraint

  constexpr auto can = [](auto &&...args) { return requires { fn::disjoin(FWD(args)...); }; };
  static_assert(can(EA{1}, EB{true}));
  static_assert(not can(EA{1}, 42));

  static_assert(not can(1, 2));
  static_assert(not can(42));
  static_assert(can(EA{1}));
}
