// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "static_check.hpp"

#include "functional/functional.hpp"
#include "functional/optional.hpp"
#include "functional/pack.hpp"
#include "functional/sum.hpp"

#include <catch2/catch_all.hpp>

#include <utility>

namespace {
struct A final {
  int v = 0;
};

template <typename V, typename Fn>
concept pack_check = requires(V v, Fn fn) { FWD(v).invoke(FWD(fn), A{}); };

} // namespace

TEST_CASE("pack", "[pack]")
{
  using fn::pack;

  using T = pack<int, int const, int &, int const &>;
  int val1 = 15;
  int const val2 = 92;
  T v{3, 14, val1, val2};
  CHECK(v.size == 4);
  static_assert(pack_check<T, decltype(([](auto &&...) {}))>);            // generic call
  static_assert(pack_check<T, decltype(([](int, int, int, int, A) {}))>); // pass everything by value
  static_assert(pack_check<T, decltype(([](int const &, int const &, int const &, int const &, A const &) {
                           }))>); // pass everything by const reference
  static_assert(pack_check<T, decltype(([](int, int, int &, int const &, A) {}))>); // bind lvalues
  static_assert(
      pack_check<T, decltype(([](int &&, int const &&, int &, int const &, A &&) {}))>); // bind rvalues and lvalues
  static_assert(pack_check<T, decltype(([](int const, int const, int const &, int const &, A const) {
                           }))>); // pass values or lvalues promoted to     const

  static_assert(not pack_check<T, decltype(([](int &, auto &&...) {}))>); // cannot bind rvalue to lvalue reference
  static_assert(
      not pack_check<T, decltype(([](int, int, int &&, int, A) {}))>); // cannot bind lvalue to rvalue reference
  static_assert(
      not pack_check<T, decltype(([](int, int, int, int &&, A) {}))>); // cannot bind const lvalue to rvalue reference
  static_assert(not pack_check<T, decltype(([](int, int, int, int const &&, A) {}))>); // cannot bind const lvalue to
                                                                                       // const rvalue      reference
  static_assert(not pack_check<T, decltype(([](int, int &&, int, int, A &&) {}))>);    // cannot bind const rvalue to
                                                                                       // non-const rvalue  reference
  static_assert(not pack_check<T, decltype(([](int, int, int, int, int) {}))>);        // bad type
  static_assert(not pack_check<T, decltype(([](auto, auto, auto, auto) {}))>);         // bad arity
  static_assert(not pack_check<T, decltype(([](auto, auto, auto, auto, auto, auto) {}))>); // bad arity

  constexpr auto fn = [](auto... args) noexcept -> int { return (0 + ... + args); };
  CHECK(v.invoke(fn) == 3 + 14 + 15 + 92);
  CHECK(fn::invoke(fn, FWD(v)) == 3 + 14 + 15 + 92);
  CHECK(v.invoke(fn, 65, 35) == 3 + 14 + 15 + 92 + 65 + 35);
  CHECK(fn::invoke(fn, FWD(v), 65, 35) == 3 + 14 + 15 + 92 + 65 + 35);
  static_assert(fn::invoke(fn, fn::pack{3, 14}, 15, 92) == 3 + 14 + 15 + 92);

  constexpr auto fn0 = [](int i, int j, int k, int l, A) noexcept -> int { return (i + j + k + l); };
  CHECK(v.invoke(fn0, A{}) == 3 + 14 + 15 + 92);
  CHECK(fn::invoke(fn0, FWD(v), A{}) == 3 + 14 + 15 + 92);
  static_assert(fn::invoke(fn0, fn::pack{3, 14, 15, 92}, A{}) == 3 + 14 + 15 + 92);

  A a;
  constexpr auto fn1 = [](int i, int j, int k, int l, A &dest) noexcept -> A & {
    dest.v = (i + j + k + l);
    return dest;
  };
  CHECK(v.invoke(fn1, a).v == 3 + 14 + 15 + 92);
  CHECK(v.invoke_r<A>(fn1, a).v == 3 + 14 + 15 + 92);
  CHECK(v.invoke_r<long>([](auto... args) noexcept -> int { return (0 + ... + args); }, 65, 35)
        == 3 + 14 + 15 + 92 + 65 + 35);
  CHECK(fn::invoke_r<long>([](auto... args) noexcept -> int { return (0 + ... + args); }, FWD(v), 65, 35)
        == 3 + 14 + 15 + 92 + 65 + 35);
  static_assert(
      fn::invoke_r<long>([](auto... args) noexcept -> int { return (0 + ... + args); }, fn::pack{3, 14}, 15, 92)
      == 3 + 14 + 15 + 92);
  CHECK(v.invoke_r<long>(fn0, A{}) == 3 + 14 + 15 + 92);
  CHECK(fn::invoke_r<long>(fn0, FWD(v), A{}) == 3 + 14 + 15 + 92);
  static_assert(fn::invoke_r<long>(fn0, fn::pack{3, 14, 15, 92}, A{}) == 3 + 14 + 15 + 92);

  static_assert(std::is_same_v<decltype(v.invoke(fn1, a)), A &>);
  static_assert(std::is_same_v<decltype(v.invoke_r<A>(fn1, a)), A>);

  constexpr auto fn2 = [](int, int, int, int, A &&dest) noexcept -> A && { return std::move(dest); };
  static_assert(std::is_same_v<decltype(v.invoke(fn2, std::move(a))), A &&>);
  static_assert(std::is_same_v<decltype(v.invoke_r<A>(fn2, std::move(a))), A>);

  constexpr auto fn3 = [](int, int, int, int, A &&dest) noexcept -> A { return dest; };
  static_assert(std::is_same_v<decltype(v.invoke(fn3, std::move(a))), A>);
  static_assert(std::is_same_v<decltype(v.invoke_r<A>(fn3, std::move(a))), A>);

  static_assert(pack<>::size == 0);

  static_assert(std::same_as<decltype(fn::pack{}), fn::pack<>>);
  static_assert(std::same_as<decltype(fn::pack{12}), fn::pack<int>>);
  static_assert(std::same_as<decltype(fn::pack{a}), fn::pack<A &>>);
  static_assert(std::same_as<decltype(fn::pack{12, a}), fn::pack<int, A &>>);
  static_assert(std::same_as<decltype(fn::pack{12, std::as_const(a)}), fn::pack<int, A const &>>);
  static_assert(std::same_as<decltype(fn::pack{12, std::move(a)}), fn::pack<int, A>>);

  constexpr auto c1 = fn::as_pack();
  static_assert(std::same_as<decltype(c1), fn::pack<> const>);
  constexpr auto c2 = fn::as_pack(true, 12);
  static_assert(std::same_as<decltype(c2), fn::pack<bool, int> const>);
  static_assert(c2.invoke([](auto i, auto j) {
    if constexpr (std::is_same_v<decltype(i), bool> && std::is_same_v<decltype(j), int>)
      return i && j == 12;
    else
      return false;
  }));
}

TEST_CASE("append value categories", "[pack][append]")
{
  using fn::pack;

  struct B {
    constexpr explicit B(int i) : v(i) {}
    constexpr B(int i, int j) : v(i * j) {}

    int v = 0;
  };

  struct C final : B {
    constexpr C() : B(30) {}
  };

  using T = pack<int, std::string_view, A>;
  T s{12, "bar", 42};

  static_assert(T::size == 3);
  constexpr auto check = [](int i, std::string_view s, A a, B const &b) noexcept -> bool {
    return i == 12 && s == std::string("bar") && a.v == 42 && b.v == 30;
  };

  WHEN("explicit type selection")
  {
    static_assert(std::same_as<decltype(s.append(std::in_place_type<B>, 5, 6)), T::append_type<B>>);
    static_assert(std::same_as<T::append_type<B>, pack<int, std::string_view, A, B>>);
    static_assert(decltype(s.append(std::in_place_type<B>, 5, 6))::size == 4);

    constexpr C c1{};
    static_assert(std::same_as<decltype(s.append(std::in_place_type<B const &>, c1)), T::append_type<B const &>>);
    static_assert(std::same_as<T::append_type<B const &>, pack<int, std::string_view, A, B const &>>);

    C c2{};
    static_assert(std::same_as<decltype(s.append(std::in_place_type<B &>, c2)), T::append_type<B &>>);
    static_assert(std::same_as<T::append_type<B &>, pack<int, std::string_view, A, B &>>);

    WHEN("constructor takes parameters")
    {
      CHECK(s.append(std::in_place_type<B>, 5, 6).invoke(check));
      CHECK(std::as_const(s).append(std::in_place_type<B>, 5, 6).invoke(check));
      CHECK(std::move(std::as_const(s)).append(std::in_place_type<B>, 5, 6).invoke(check));
      CHECK(std::move(s).append(std::in_place_type<B>, 5, 6).invoke(check));
    }

    WHEN("constructor takes parameters invoke_r")
    {
      CHECK(s.append(std::in_place_type<B>, 5, 6).invoke_r<bool>(check));
      CHECK(std::as_const(s).append(std::in_place_type<B>, 5, 6).invoke_r<bool>(check));
      CHECK(std::move(std::as_const(s)).append(std::in_place_type<B>, 5, 6).invoke_r<bool>(check));
      CHECK(std::move(s).append(std::in_place_type<B>, 5, 6).invoke_r<bool>(check));
    }

    WHEN("default constructor")
    {
      CHECK(s.append(std::in_place_type<C>).invoke(check));
      CHECK(std::as_const(s).append(std::in_place_type<C>).invoke(check));
      CHECK(std::move(std::as_const(s)).append(std::in_place_type<C>).invoke(check));
      CHECK(std::move(s).append(std::in_place_type<C>).invoke(check));
    }

    WHEN("default constructor invoke_r")
    {
      CHECK(s.append(std::in_place_type<C>).invoke_r<int>(check) == 1);
      CHECK(std::as_const(s).append(std::in_place_type<C>).invoke_r<int>(check) == 1);
      CHECK(std::move(std::as_const(s)).append(std::in_place_type<C>).invoke_r<int>(check) == 1);
      CHECK(std::move(s).append(std::in_place_type<C>).invoke_r<int>(check) == 1);
    }
  }

  WHEN("deduced type")
  {
    static_assert(std::same_as<decltype(s.append(B{5, 6})), T::append_type<B>>);

    constexpr C c1{};
    static_assert(std::same_as<decltype(s.append(c1)), T::append_type<C const &>>);
    static_assert(std::same_as<T::append_type<C const &>, pack<int, std::string_view, A, C const &>>);

    C c2{};
    static_assert(std::same_as<decltype(s.append(c2)), T::append_type<C &>>);
    static_assert(std::same_as<T::append_type<C &>, pack<int, std::string_view, A, C &>>);

    CHECK(s.append(B{30}).invoke(check));
    CHECK(std::as_const(s).append(B{30}).invoke(check));
    CHECK(std::move(std::as_const(s)).append(B{30}).invoke(check));
    CHECK(std::move(s).append(B{30}).invoke(check));
  }

  WHEN("pack on the right side, deduced")
  {
    constexpr fn::pack<bool, int, B> a{true, 3, B{14}};
    constexpr fn::pack<C, B> b{C{}, B{3, 4}};
    constexpr auto c1 = a.append(b);
    static_assert(std::same_as<decltype(c1), fn::pack<bool, int, B, C, B> const>);
    static_assert(c1.invoke([](bool i, int j, B const &b1, C const &c, B const &b2) {
      return i && j == 3 && b1.v == 14 && c.v == 30 && b2.v == 12;
    }));

    auto c2 = a.append(fn::pack{C{}, B{4, 5}});
    static_assert(std::same_as<decltype(c2), fn::pack<bool, int, B, C, B>>);
    CHECK(c2.invoke([](bool i, int j, B const &b1, C const &c, B const &b2) {
      return i && j == 3 && b1.v == 14 && c.v == 30 && b2.v == 20;
    }));
  }
}

TEST_CASE("pack with immovable data", "[pack][immovable]")
{
  using fn::pack;
  using util::static_check;

  struct ImmovableType {
    int value = 0;

    constexpr explicit ImmovableType(int i) noexcept : value(i) {}

    ImmovableType(ImmovableType const &) = delete;
    ImmovableType &operator=(ImmovableType const &) = delete;
    ImmovableType(ImmovableType &&) = delete;
    ImmovableType &operator=(ImmovableType &&) = delete;

    constexpr bool operator==(ImmovableType const &other) const noexcept { return value == other.value; }
  };

  using T = pack<ImmovableType, ImmovableType const, ImmovableType &, ImmovableType const &>;
  ImmovableType val1{15};
  ImmovableType const val2{92};
  T v{ImmovableType{3}, ImmovableType{14}, val1, val2};

  using is
      = static_check::bind<decltype([](auto &&fn) constexpr { return requires { std::declval<T>().invoke(fn); }; })>;

  static_assert(is::invocable([](auto &&...) {})); // generic call
  static_assert(is::invocable([](ImmovableType const &, ImmovableType const &, ImmovableType const &,
                                 ImmovableType const &) {})); // pass everything by const reference
  static_assert(is::invocable([](ImmovableType &&, ImmovableType const &&, ImmovableType &, ImmovableType const &) {
  }));                                                                // bind rvalues and lvalues
  static_assert(is::not_invocable([](ImmovableType, auto &&...) {})); // cannot pass immovable by value

  CHECK(v.invoke([](auto &&...args) noexcept -> int { return (0 + ... + args.value); }) == 3 + 14 + 15 + 92);
}

TEST_CASE("constexpr pack", "[pack][constexpr]")
{
  constexpr fn::pack<int, int> v2{3, 14};
  constexpr auto r2 = v2.invoke([](auto &&...args) constexpr noexcept -> int { return (0 + ... + args); });
  static_assert(r2 == 3 + 14);
  SUCCEED();
}

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
} // namespace

TEST_CASE("detail::_join on constexpr optional", "[detail][pack][sum][optional][constexpr]")
{
  WHEN("sum of packs join sum of scalars")
  {
    constexpr fn::optional<fn::sum<fn::pack<Alef, Gimel>, fn::pack<Bet, Gimel>>> lh{fn::pack{Alef{3}, Gimel{14}}};
    constexpr fn::optional<fn::sum<Heh, Vav, Zayn>> rh{Vav{15}};
    static constexpr auto efn = [](auto &&...) { return std::nullopt; };
    constexpr auto r = fn::detail::_join<fn::optional>(lh, rh, efn);
    static_assert(
        std::is_same_v<decltype(r),          //
                       fn::optional<fn::sum< //
                           fn::pack<Alef, Gimel, Heh>, fn::pack<Alef, Gimel, Vav>, fn::pack<Alef, Gimel, Zayn>,
                           fn::pack<Bet, Gimel, Heh>, fn::pack<Bet, Gimel, Vav>, fn::pack<Bet, Gimel, Zayn>>> const>);
    static_assert(r.has_value());
    static_assert(r.value().has_value<fn::pack<Alef, Gimel, Vav>>());
    static_assert(r.value().invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14 + 15);
  }

  WHEN("sum of packs join sum of packs")
  {
    constexpr fn::optional<fn::sum<fn::pack<Alef, Gimel>, fn::pack<Bet, Gimel>>> lh{fn::pack{Alef{3}, Gimel{14}}};
    constexpr fn::optional<fn::sum<fn::pack<Heh, Zayn>, fn::pack<Vav>>> rh{fn::pack{Vav{15}}};
    static constexpr auto efn = [](auto &&...) { return std::nullopt; };
    constexpr auto r = fn::detail::_join<fn::optional>(lh, rh, efn);
    static_assert(std::is_same_v<decltype(r),          //
                                 fn::optional<fn::sum< //
                                     fn::pack<Alef, Gimel, Heh, Zayn>, fn::pack<Alef, Gimel, Vav>,
                                     fn::pack<Bet, Gimel, Heh, Zayn>, fn::pack<Bet, Gimel, Vav>>> const>);
    static_assert(r.has_value());
    static_assert(r.value().has_value<fn::pack<Alef, Gimel, Vav>>());
    static_assert(r.value().invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14 + 15);
  }

  WHEN("sum of scalars join sum of scalars")
  {
    constexpr fn::optional<fn::sum<Alef, Bet, Gimel>> lh{Gimel{3}};
    constexpr fn::optional<fn::sum<Heh, Vav, Zayn>> rh{Vav{14}};
    static constexpr auto efn = [](auto &&...) { return std::nullopt; };
    constexpr auto r = fn::detail::_join<fn::optional>(lh, rh, efn);
    static_assert(
        std::is_same_v<
            decltype(r),          //
            fn::optional<fn::sum< //
                fn::pack<Alef, Heh>, fn::pack<Alef, Vav>, fn::pack<Alef, Zayn>, fn::pack<Bet, Heh>, fn::pack<Bet, Vav>,
                fn::pack<Bet, Zayn>, fn::pack<Gimel, Heh>, fn::pack<Gimel, Vav>, fn::pack<Gimel, Zayn>>> const>);
    static_assert(r.has_value());
    static_assert(r.value().has_value<fn::pack<Gimel, Vav>>());
    static_assert(r.value().invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14);
  }

  WHEN("sum of scalars join sum of packs")
  {
    constexpr fn::optional<fn::sum<Alef, Bet, Gimel>> lh{Gimel{3}};
    constexpr fn::optional<fn::sum<fn::pack<Heh, Zayn>, fn::pack<Vav>>> rh{fn::pack{Vav{14}}};
    static constexpr auto efn = [](auto &&...) { return std::nullopt; };
    constexpr auto r = fn::detail::_join<fn::optional>(lh, rh, efn);
    static_assert(std::is_same_v<decltype(r),          //
                                 fn::optional<fn::sum< //
                                     fn::pack<Alef, Heh, Zayn>, fn::pack<Alef, Vav>, fn::pack<Bet, Heh, Zayn>,
                                     fn::pack<Bet, Vav>, fn::pack<Gimel, Heh, Zayn>, fn::pack<Gimel, Vav>>> const>);
    static_assert(r.has_value());
    static_assert(r.value().has_value<fn::pack<Gimel, Vav>>());
    static_assert(r.value().invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14);
  }

  WHEN("sum of packs join scalar")
  {
    constexpr fn::optional<fn::sum<fn::pack<Alef, Gimel>, fn::pack<Bet, Gimel>>> lh{fn::pack{Alef{3}, Gimel{14}}};
    constexpr fn::optional<Vav> rh{Vav{15}};
    static constexpr auto efn = [](auto &&...) { return std::nullopt; };
    constexpr auto r = fn::detail::_join<fn::optional>(lh, rh, efn);
    static_assert(std::is_same_v<decltype(r),          //
                                 fn::optional<fn::sum< //
                                     fn::pack<Alef, Gimel, Vav>, fn::pack<Bet, Gimel, Vav>>> const>);
    static_assert(r.has_value());
    static_assert(r.value().has_value<fn::pack<Alef, Gimel, Vav>>());
    static_assert(r.value().invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14 + 15);
  }

  WHEN("sum of packs join pack")
  {
    constexpr fn::optional<fn::sum<fn::pack<Alef, Gimel>, fn::pack<Bet, Gimel>>> lh{fn::pack{Alef{3}, Gimel{14}}};
    constexpr fn::optional<fn::pack<Vav>> rh{fn::pack{Vav{15}}};
    static constexpr auto efn = [](auto &&...) { return std::nullopt; };
    constexpr auto r = fn::detail::_join<fn::optional>(lh, rh, efn);
    static_assert(std::is_same_v<decltype(r),          //
                                 fn::optional<fn::sum< //
                                     fn::pack<Alef, Gimel, Vav>, fn::pack<Bet, Gimel, Vav>>> const>);
    static_assert(r.has_value());
    static_assert(r.value().has_value<fn::pack<Alef, Gimel, Vav>>());
    static_assert(r.value().invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14 + 15);
  }

  WHEN("sum of scalars join scalar")
  {
    constexpr fn::optional<fn::sum<Alef, Bet, Gimel>> lh{Gimel{3}};
    constexpr fn::optional<Vav> rh{Vav{14}};
    static constexpr auto efn = [](auto &&...) { return std::nullopt; };
    constexpr auto r = fn::detail::_join<fn::optional>(lh, rh, efn);
    static_assert(std::is_same_v<decltype(r),          //
                                 fn::optional<fn::sum< //
                                     fn::pack<Alef, Vav>, fn::pack<Bet, Vav>, fn::pack<Gimel, Vav>>> const>);
    static_assert(r.has_value());
    static_assert(r.value().has_value<fn::pack<Gimel, Vav>>());
    static_assert(r.value().invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14);
  }

  WHEN("sum of scalars join pack")
  {
    constexpr fn::optional<fn::sum<Alef, Bet, Gimel>> lh{Gimel{3}};
    constexpr fn::optional<fn::pack<Vav>> rh{fn::pack{Vav{14}}};
    static constexpr auto efn = [](auto &&...) { return std::nullopt; };
    constexpr auto r = fn::detail::_join<fn::optional>(lh, rh, efn);
    static_assert(std::is_same_v<decltype(r),          //
                                 fn::optional<fn::sum< //
                                     fn::pack<Alef, Vav>, fn::pack<Bet, Vav>, fn::pack<Gimel, Vav>>> const>);
    static_assert(r.has_value());
    static_assert(r.value().has_value<fn::pack<Gimel, Vav>>());
    static_assert(r.value().invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14);
  }

  WHEN("pack join sum of scalars")
  {
    constexpr fn::optional<fn::pack<Alef, Gimel>> lh{fn::pack{Alef{3}, Gimel{14}}};
    constexpr fn::optional<fn::sum<Heh, Vav, Zayn>> rh{Vav{15}};
    static constexpr auto efn = [](auto &&...) { return std::nullopt; };
    constexpr auto r = fn::detail::_join<fn::optional>(lh, rh, efn);
    static_assert(std::is_same_v<
                  decltype(r),          //
                  fn::optional<fn::sum< //
                      fn::pack<Alef, Gimel, Heh>, fn::pack<Alef, Gimel, Vav>, fn::pack<Alef, Gimel, Zayn>>> const>);
    static_assert(r.has_value());
    static_assert(r.value().has_value<fn::pack<Alef, Gimel, Vav>>());
    static_assert(r.value().invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14 + 15);
  }

  WHEN("pack join sum of packs")
  {
    constexpr fn::optional<fn::pack<Alef, Gimel>> lh{fn::pack{Alef{3}, Gimel{14}}};
    constexpr fn::optional<fn::sum<fn::pack<Heh, Zayn>, fn::pack<Vav>>> rh{fn::pack{Vav{15}}};
    static constexpr auto efn = [](auto &&...) { return std::nullopt; };
    constexpr auto r = fn::detail::_join<fn::optional>(lh, rh, efn);
    static_assert(std::is_same_v<decltype(r),          //
                                 fn::optional<fn::sum< //
                                     fn::pack<Alef, Gimel, Heh, Zayn>, fn::pack<Alef, Gimel, Vav>>> const>);
    static_assert(r.has_value());
    static_assert(r.value().has_value<fn::pack<Alef, Gimel, Vav>>());
    static_assert(r.value().invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14 + 15);
  }

  WHEN("scalar join sum of scalars")
  {
    constexpr fn::optional<Alef> lh{Alef{3}};
    constexpr fn::optional<fn::sum<Heh, Vav, Zayn>> rh{Vav{14}};
    static constexpr auto efn = [](auto &&...) { return std::nullopt; };
    constexpr auto r = fn::detail::_join<fn::optional>(lh, rh, efn);
    static_assert(std::is_same_v<decltype(r),          //
                                 fn::optional<fn::sum< //
                                     fn::pack<Alef, Heh>, fn::pack<Alef, Vav>, fn::pack<Alef, Zayn>>> const>);
    static_assert(r.has_value());
    static_assert(r.value().has_value<fn::pack<Alef, Vav>>());
    static_assert(r.value().invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14);
  }

  WHEN("scalar join sum of packs")
  {
    constexpr fn::optional<Alef> lh{Alef{3}};
    constexpr fn::optional<fn::sum<fn::pack<Heh, Zayn>, fn::pack<Vav>>> rh{fn::pack{Vav{14}}};
    static constexpr auto efn = [](auto &&...) { return std::nullopt; };
    constexpr auto r = fn::detail::_join<fn::optional>(lh, rh, efn);
    static_assert(std::is_same_v<decltype(r),          //
                                 fn::optional<fn::sum< //
                                     fn::pack<Alef, Heh, Zayn>, fn::pack<Alef, Vav>>> const>);
    static_assert(r.has_value());
    static_assert(r.value().has_value<fn::pack<Alef, Vav>>());
    static_assert(r.value().invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14);
  }

  WHEN("pack join scalar")
  {
    constexpr fn::optional<fn::pack<Alef, Gimel>> lh{fn::pack{Alef{3}, Gimel{14}}};
    constexpr fn::optional<Vav> rh{Vav{15}};
    static constexpr auto efn = [](auto &&...) { return std::nullopt; };
    constexpr auto r = fn::detail::_join<fn::optional>(lh, rh, efn);
    static_assert(std::is_same_v<decltype(r), //
                                 fn::optional<fn::pack<Alef, Gimel, Vav>> const>);
    static_assert(r.has_value());
    static_assert(r.value().invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14 + 15);
  }

  WHEN("pack join pack")
  {
    constexpr fn::optional<fn::pack<Alef, Gimel>> lh{fn::pack{Alef{3}, Gimel{14}}};
    constexpr fn::optional<fn::pack<Vav>> rh{fn::pack{Vav{15}}};
    static constexpr auto efn = [](auto &&...) { return std::nullopt; };
    constexpr auto r = fn::detail::_join<fn::optional>(lh, rh, efn);
    static_assert(std::is_same_v<decltype(r), //
                                 fn::optional<fn::pack<Alef, Gimel, Vav>> const>);
    static_assert(r.has_value());
    static_assert(r.value().invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14 + 15);
  }

  WHEN("scalar join scalar")
  {
    constexpr fn::optional<Alef> lh{Alef{3}};
    constexpr fn::optional<Vav> rh{Vav{14}};
    static constexpr auto efn = [](auto &&...) { return std::nullopt; };
    constexpr auto r = fn::detail::_join<fn::optional>(lh, rh, efn);
    static_assert(std::is_same_v<decltype(r), //
                                 fn::optional<fn::pack<Alef, Vav>> const>);
    static_assert(r.has_value());
    static_assert(r.value().invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14);
  }

  WHEN("scalar join pack")
  {
    constexpr fn::optional<Alef> lh{Alef{3}};
    constexpr fn::optional<fn::pack<Vav>> rh{fn::pack{Vav{14}}};
    static constexpr auto efn = [](auto &&...) { return std::nullopt; };
    constexpr auto r = fn::detail::_join<fn::optional>(lh, rh, efn);
    static_assert(std::is_same_v<decltype(r), //
                                 fn::optional<fn::pack<Alef, Vav>> const>);
    static_assert(r.has_value());
    static_assert(r.value().invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14);
  }
}

TEST_CASE("operator &", "[pack][sum][operator_and]")
{
  WHEN("sum of packs join sum of scalars")
  {
    constexpr fn::sum<fn::pack<Alef, Gimel>, fn::pack<Bet, Gimel>> lh{fn::pack{Alef{3}, Gimel{14}}};
    constexpr fn::sum<Heh, Vav, Zayn> rh{Vav{15}};
    auto r = lh & rh;
    static_assert(
        std::is_same_v<decltype(r), //
                       fn::sum<     //
                           fn::pack<Alef, Gimel, Heh>, fn::pack<Alef, Gimel, Vav>, fn::pack<Alef, Gimel, Zayn>,
                           fn::pack<Bet, Gimel, Heh>, fn::pack<Bet, Gimel, Vav>, fn::pack<Bet, Gimel, Zayn>>>);
    CHECK(r.invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14 + 15);
  }

  WHEN("sum of packs join sum of packs")
  {
    constexpr fn::sum<fn::pack<Alef, Gimel>, fn::pack<Bet, Gimel>> lh{fn::pack{Alef{3}, Gimel{14}}};
    constexpr fn::sum<fn::pack<Heh, Zayn>, fn::pack<Vav>> rh{fn::pack{Vav{15}}};
    auto r = lh & rh;
    static_assert(std::is_same_v<decltype(r), //
                                 fn::sum<     //
                                     fn::pack<Alef, Gimel, Heh, Zayn>, fn::pack<Alef, Gimel, Vav>,
                                     fn::pack<Bet, Gimel, Heh, Zayn>, fn::pack<Bet, Gimel, Vav>>>);
    CHECK(r.invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14 + 15);
  }

  WHEN("sum of scalars join sum of scalars")
  {
    constexpr fn::sum<Alef, Bet, Gimel> lh{Gimel{3}};
    constexpr fn::sum<Heh, Vav, Zayn> rh{Vav{14}};
    auto r = lh & rh;
    static_assert(
        std::is_same_v<
            decltype(r), //
            fn::sum<     //
                fn::pack<Alef, Heh>, fn::pack<Alef, Vav>, fn::pack<Alef, Zayn>, fn::pack<Bet, Heh>, fn::pack<Bet, Vav>,
                fn::pack<Bet, Zayn>, fn::pack<Gimel, Heh>, fn::pack<Gimel, Vav>, fn::pack<Gimel, Zayn>>>);
    CHECK(r.invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14);
  }

  WHEN("sum of scalars join sum of packs")
  {
    constexpr fn::sum<Alef, Bet, Gimel> lh{Gimel{3}};
    constexpr fn::sum<fn::pack<Heh, Zayn>, fn::pack<Vav>> rh{fn::pack{Vav{14}}};
    auto r = lh & rh;
    static_assert(std::is_same_v<decltype(r), //
                                 fn::sum<     //
                                     fn::pack<Alef, Heh, Zayn>, fn::pack<Alef, Vav>, fn::pack<Bet, Heh, Zayn>,
                                     fn::pack<Bet, Vav>, fn::pack<Gimel, Heh, Zayn>, fn::pack<Gimel, Vav>>>);
    CHECK(r.invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14);
  }

  WHEN("sum of packs join scalar")
  {
    constexpr fn::sum<fn::pack<Alef, Gimel>, fn::pack<Bet, Gimel>> lh{fn::pack{Alef{3}, Gimel{14}}};
    constexpr Vav rh{Vav{15}};
    auto r = lh & rh;
    static_assert(std::is_same_v<decltype(r), //
                                 fn::sum<     //
                                     fn::pack<Alef, Gimel, Vav>, fn::pack<Bet, Gimel, Vav>>>);
    CHECK(r.invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14 + 15);
  }

  WHEN("sum of packs join pack")
  {
    constexpr fn::sum<fn::pack<Alef, Gimel>, fn::pack<Bet, Gimel>> lh{fn::pack{Alef{3}, Gimel{14}}};
    constexpr fn::pack<Vav> rh{fn::pack{Vav{15}}};
    auto r = lh & rh;
    static_assert(std::is_same_v<decltype(r), //
                                 fn::sum<     //
                                     fn::pack<Alef, Gimel, Vav>, fn::pack<Bet, Gimel, Vav>>>);
    CHECK(r.invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14 + 15);
  }

  WHEN("sum of scalars join scalar")
  {
    constexpr fn::sum<Alef, Bet, Gimel> lh{Gimel{3}};
    constexpr Vav rh{Vav{14}};
    auto r = lh & rh;
    static_assert(std::is_same_v<decltype(r), //
                                 fn::sum<     //
                                     fn::pack<Alef, Vav>, fn::pack<Bet, Vav>, fn::pack<Gimel, Vav>>>);
    CHECK(r.invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14);
  }

  WHEN("sum of scalars join pack")
  {
    constexpr fn::sum<Alef, Bet, Gimel> lh{Gimel{3}};
    constexpr fn::pack<Vav> rh{fn::pack{Vav{14}}};
    auto r = lh & rh;
    static_assert(std::is_same_v<decltype(r), //
                                 fn::sum<     //
                                     fn::pack<Alef, Vav>, fn::pack<Bet, Vav>, fn::pack<Gimel, Vav>>>);
    CHECK(r.invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14);
  }

  WHEN("pack join sum of scalars")
  {
    constexpr fn::pack<Alef, Gimel> lh{fn::pack{Alef{3}, Gimel{14}}};
    constexpr fn::sum<Heh, Vav, Zayn> rh{Vav{15}};
    auto r = lh & rh;
    static_assert(
        std::is_same_v<decltype(r), //
                       fn::sum<     //
                           fn::pack<Alef, Gimel, Heh>, fn::pack<Alef, Gimel, Vav>, fn::pack<Alef, Gimel, Zayn>>>);
    CHECK(r.invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14 + 15);
  }

  WHEN("pack join sum of packs")
  {
    constexpr fn::pack<Alef, Gimel> lh{fn::pack{Alef{3}, Gimel{14}}};
    constexpr fn::sum<fn::pack<Heh, Zayn>, fn::pack<Vav>> rh{fn::pack{Vav{15}}};
    auto r = lh & rh;
    static_assert(std::is_same_v<decltype(r), //
                                 fn::sum<     //
                                     fn::pack<Alef, Gimel, Heh, Zayn>, fn::pack<Alef, Gimel, Vav>>>);
    CHECK(r.invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14 + 15);
  }

  WHEN("pack join scalar")
  {
    constexpr fn::pack<Alef, Gimel> lh{fn::pack{Alef{3}, Gimel{14}}};
    constexpr Vav rh{Vav{15}};
    auto r = lh & rh;
    static_assert(std::is_same_v<decltype(r), //
                                 fn::pack<Alef, Gimel, Vav>>);
    CHECK(r.invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14 + 15);
  }

  WHEN("pack join pack")
  {
    constexpr fn::pack<Alef, Gimel> lh{fn::pack{Alef{3}, Gimel{14}}};
    constexpr fn::pack<Vav> rh{fn::pack{Vav{15}}};
    auto r = lh & rh;
    static_assert(std::is_same_v<decltype(r), //
                                 fn::pack<Alef, Gimel, Vav>>);
    CHECK(r.invoke([](auto &&...v) -> int { return (0 + ... + v.value); }) == 3 + 14 + 15);
  }

  constexpr auto r1 = fn::as_sum(12) & 3 & 2.5 & fn::pack{0.5, true}
                      & fn::sum_for<bool, int, fn::pack<double, int>>(fn::pack{1.5, 12});
  static_assert(std::is_same_v<                                     //
                decltype(r1),                                       //
                fn::sum_for<                                        //
                    fn::pack<int, int, double, double, bool, bool>, //
                    fn::pack<int, int, double, double, bool, int>,  //
                    fn::pack<int, int, double, double, bool, double, int>> const>);
  static_assert(r1.invoke([](auto &&...args) -> double { return (1 * ... * static_cast<double>(args)); })
                == 12. * 3 * 2.5 * 0.5 * 1 * 1.5 * 12);

  constexpr auto r2
      = fn::identity(12, 3, 2.5, fn::pack{0.5, true}, fn::sum_for<bool, int, fn::pack<double, int>>(fn::pack{1.5, 12}));
  static_assert(std::is_same_v<                                     //
                decltype(r2),                                       //
                fn::sum_for<                                        //
                    fn::pack<int, int, double, double, bool, bool>, //
                    fn::pack<int, int, double, double, bool, int>,  //
                    fn::pack<int, int, double, double, bool, double, int>> const>);
  static_assert(r2.invoke([](auto &&...args) -> double { return (1 * ... * static_cast<double>(args)); })
                == 12. * 3 * 2.5 * 0.5 * 1 * 1.5 * 12);
}
