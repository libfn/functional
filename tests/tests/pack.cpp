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

#include <type_traits>
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
  static_assert(pack_check<T, decltype(([](A, auto &&...) {}))>);         // also generic call
  static_assert(pack_check<T, decltype(([](A, int, int, int, int) {}))>); // pass everything by value
  static_assert(pack_check<T, decltype(([](A const &, int const &, int const &, int const &, int const &) {
                           }))>); // pass everything by const reference
  static_assert(pack_check<T, decltype(([](A, int, int, int &, int const &) {}))>); // bind lvalues
  static_assert(
      pack_check<T, decltype(([](A &&, int &&, int const &&, int &, int const &) {}))>); // bind rvalues and lvalues
  static_assert(pack_check<T, decltype(([](A const, int const, int const, int const &, int const &) {
                           }))>); // pass values or lvalues promoted to     const

  static_assert(not pack_check<T, decltype(([](A &, auto &&...) {}))>);      // cannot bind rvalue to lvalue reference
  static_assert(not pack_check<T, decltype(([](A, int &, auto &&...) {}))>); // cannot bind rvalue to lvalue reference
  static_assert(
      not pack_check<T, decltype(([](A, int, int, int &&, int) {}))>); // cannot bind lvalue to rvalue reference
  static_assert(
      not pack_check<T, decltype(([](A, int, int, int, int &&) {}))>); // cannot bind const lvalue to rvalue reference
  static_assert(not pack_check<T, decltype(([](A, int, int, int, int const &&) {}))>); // cannot bind const lvalue to
                                                                                       // const rvalue      reference
  static_assert(not pack_check<T, decltype(([](A &&, int, int &&, int, int) {}))>);    // cannot bind const rvalue to
                                                                                       // non-const rvalue  reference
  static_assert(not pack_check<T, decltype(([](int, auto &&...) {}))>);                // bad type
  static_assert(not pack_check<T, decltype(([](auto, auto, auto, auto) {}))>);         // bad arity
  static_assert(not pack_check<T, decltype(([](auto, auto, auto, auto, auto, auto) {}))>); // bad arity
  CHECK(v.invoke([](auto... args) noexcept -> int { return (0 + ... + args); }) == 3 + 14 + 15 + 92);
  CHECK(fn::invoke([](auto... args) noexcept -> int { return (0 + ... + args); }, FWD(v)) == 3 + 14 + 15 + 92);
  CHECK(v.invoke([](auto... args) noexcept -> int { return (0 + ... + args); }, 65, 35) == 3 + 14 + 15 + 92 + 65 + 35);
  CHECK(fn::invoke([](auto... args) noexcept -> int { return (0 + ... + args); }, FWD(v), 65, 35)
        == 3 + 14 + 15 + 92 + 65 + 35);
  static_assert(fn::invoke([](auto... args) noexcept -> int { return (0 + ... + args); }, fn::pack{3, 14}, 15, 92)
                == 3 + 14 + 15 + 92);
  CHECK(v.invoke([](A, auto... args) noexcept -> int { return (0 + ... + args); }, A{}) == 3 + 14 + 15 + 92);
  CHECK(fn::invoke([](A, auto... args) noexcept -> int { return (0 + ... + args); }, FWD(v), A{}) == 3 + 14 + 15 + 92);
  static_assert(fn::invoke([](A, auto... args) noexcept -> int { return (0 + ... + args); }, fn::pack{3, 14}, A{})
                == 3 + 14);

  A a;
  constexpr auto fn1 = [](A &dest, auto... args) noexcept -> A & {
    dest.v = (0 + ... + args);
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
  CHECK(v.invoke_r<long>([](A, auto... args) noexcept -> int { return (0 + ... + args); }, A{}) == 3 + 14 + 15 + 92);
  CHECK(fn::invoke_r<long>([](A, auto... args) noexcept -> int { return (0 + ... + args); }, FWD(v), A{})
        == 3 + 14 + 15 + 92);
  static_assert(
      fn::invoke_r<long>([](A, auto... args) noexcept -> int { return (0 + ... + args); }, fn::pack{3, 14}, A{})
      == 3 + 14);

  static_assert(std::is_same_v<decltype(v.invoke(fn1, a)), A &>);
  static_assert(std::is_same_v<decltype(v.invoke_r<A>(fn1, a)), A>);

  constexpr auto fn2 = [](A &&dest, auto &&...) noexcept -> A && { return std::move(dest); };
  static_assert(std::is_same_v<decltype(v.invoke(fn2, std::move(a))), A &&>);
  static_assert(std::is_same_v<decltype(v.invoke_r<A>(fn2, std::move(a))), A>);

  constexpr auto fn3 = [](A &&dest, auto &&...) noexcept -> A { return dest; };
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
}
