// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "util/helper_types.hpp"

#include <fn/choice.hpp>
#include <fn/copack.hpp>
#include <fn/expected.hpp>
#include <fn/functional.hpp>
#include <fn/just.hpp>
#include <fn/optional.hpp>
#include <fn/pack.hpp>

#include <catch2/catch_all.hpp>

#include <array>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

#include <fn/detail/macro_begin.hpp>

namespace {
struct A final {
  int v = 0;
};

template <typename V, typename Fn>
concept pack_check = requires(V v, Fn fn) { FWD(v).apply(FWD(fn), A{}); };

template <fn::pack<int, double> P> struct pack_nttp final {};
template <fn::some_pack auto P> struct some_pack_nttp final {};
template <fn::some_pack auto P> auto read_nttp() { return fn::get<0>(P); }

template <typename V, typename R, typename Fn>
concept can_invoke_r = requires(V v, Fn fn) { FWD(v).template apply_r<R>(FWD(fn)); };

template <typename V, typename T, typename... Args>
concept can_append_in_place = requires(V v, Args... args) { FWD(v).append(std::in_place_type<T>, args...); };

template <typename V, typename Arg>
concept can_append = requires(V v, Arg arg) { FWD(v).append(FWD(arg)); };

// A pack element whose const-lvalue copy and rvalue move can both throw, so the member relocating it
// must say so. Its non-const-lvalue copy is noexcept, which is what makes the promise category-wise.
using Throwing = helper_t<prop::throw_copy | prop::throw_move>;

template <typename... Args>
concept can_as_pack = requires(Args &&...args) { fn::as_pack(FWD(args)...); };

// pack declares no special member, so each one is implicit - composed from the _element bases which
// hold the data. Ask the elements, not the element types: a holder of a reference or of a const
// member answers assignment differently than the bare type would.
template <typename T> using elem = fn::detail::_element<0, T>;
template <typename... Ts> consteval bool special_members_follow_elements()
{
  using P = fn::pack<Ts...>;
  bool ok = std::is_copy_constructible_v<P> == (... && std::is_copy_constructible_v<elem<Ts>>);
  ok = ok && std::is_nothrow_copy_constructible_v<P> == (... && std::is_nothrow_copy_constructible_v<elem<Ts>>);
  ok = ok && std::is_move_constructible_v<P> == (... && std::is_move_constructible_v<elem<Ts>>);
  ok = ok && std::is_nothrow_move_constructible_v<P> == (... && std::is_nothrow_move_constructible_v<elem<Ts>>);
  ok = ok && std::is_copy_assignable_v<P> == (... && std::is_copy_assignable_v<elem<Ts>>);
  ok = ok && std::is_nothrow_copy_assignable_v<P> == (... && std::is_nothrow_copy_assignable_v<elem<Ts>>);
  ok = ok && std::is_move_assignable_v<P> == (... && std::is_move_assignable_v<elem<Ts>>);
  ok = ok && std::is_nothrow_move_assignable_v<P> == (... && std::is_nothrow_move_assignable_v<elem<Ts>>);
  ok = ok && std::is_destructible_v<P> == (... && std::is_destructible_v<elem<Ts>>);
  ok = ok && std::is_nothrow_destructible_v<P> == (... && std::is_nothrow_destructible_v<elem<Ts>>);
  ok = ok && std::is_trivially_destructible_v<P> == (... && std::is_trivially_destructible_v<elem<Ts>>);
  ok = ok && std::is_trivially_copy_constructible_v<P> == (... && std::is_trivially_copy_constructible_v<elem<Ts>>);
  ok = ok && std::is_trivially_move_constructible_v<P> == (... && std::is_trivially_move_constructible_v<elem<Ts>>);
  ok = ok && std::is_trivially_copy_assignable_v<P> == (... && std::is_trivially_copy_assignable_v<elem<Ts>>);
  ok = ok && std::is_trivially_move_assignable_v<P> == (... && std::is_trivially_move_assignable_v<elem<Ts>>);
  return ok;
}

} // namespace

// pack is an aggregate at every layer - the element holder, the implementation, and pack itself.
// Everything above rests on that: brace initialization with elision, the structural type, and
// special members composed from the elements' own. A constructor added anywhere in the stack would
// change all of it in silence; these assertions fail instead.
TEST_CASE("design: an aggregate, at every layer", "[pack][design]")
{
  using fn::pack;

  SECTION("aggregates")
  {
    static_assert(std::is_aggregate_v<fn::detail::_element<0, int>>);
    static_assert(std::is_aggregate_v<fn::detail::_element<1, int &>>);
    static_assert(std::is_aggregate_v<fn::detail::pack_impl<std::index_sequence<0, 1>, int, int &>>);
    static_assert(std::is_aggregate_v<pack<>>);
    static_assert(std::is_aggregate_v<pack<int, int &, int const, std::string>>);
    SUCCEED();
  }

  SECTION("brace elision")
  {
    // flat initializers reach the elements through two layers of subaggregate without written braces
    constexpr pack<int, double> p{3, 0.5};
    static_assert(fn::get<0>(p) == 3);
    CHECK(fn::get<1>(p) == 0.5);
  }

  SECTION("special members follow the elements")
  {
    static_assert(special_members_follow_elements<>());
    static_assert(special_members_follow_elements<int>());
    static_assert(special_members_follow_elements<int, double>());
    static_assert(special_members_follow_elements<std::string>());
    static_assert(special_members_follow_elements<std::unique_ptr<int>>());
    static_assert(special_members_follow_elements<int &>());
    static_assert(special_members_follow_elements<int const>());
    static_assert(special_members_follow_elements<int, std::string, std::unique_ptr<int>, int &>());

    struct Immovable {
      Immovable(Immovable &&) = delete;
    };
    static_assert(special_members_follow_elements<Immovable>());

    // anchors, so the equalities above cannot be satisfied by both sides being wrong at once
    static_assert(std::is_nothrow_move_constructible_v<pack<std::string>>);
    static_assert(not std::is_nothrow_copy_constructible_v<pack<std::string>>);
    static_assert(not std::is_copy_constructible_v<pack<std::unique_ptr<int>>>);
    static_assert(std::is_nothrow_move_constructible_v<pack<std::unique_ptr<int>>>);
    static_assert(not std::is_move_constructible_v<pack<Immovable>>);
    static_assert(std::is_copy_constructible_v<pack<int &>>);
    static_assert(not std::is_copy_assignable_v<pack<int &>>);
    static_assert(not std::is_copy_assignable_v<pack<int const>>);
    static_assert(std::is_trivially_destructible_v<pack<int, int &>>);
    static_assert(not std::is_trivially_destructible_v<pack<std::string>>);
    SUCCEED();
  }
}

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
  CHECK(v.apply(fn) == 3 + 14 + 15 + 92);
  CHECK(fn::apply(fn, FWD(v)) == 3 + 14 + 15 + 92);
  CHECK(v.apply(fn, 65, 35) == 3 + 14 + 15 + 92 + 65 + 35);
  CHECK(fn::apply(fn, FWD(v), 65, 35) == 3 + 14 + 15 + 92 + 65 + 35);
  static_assert(fn::apply(fn, fn::pack{3, 14}, 15, 92) == 3 + 14 + 15 + 92);

  constexpr auto fn0 = [](int i, int j, int k, int l, A) noexcept -> int { return (i + j + k + l); };
  CHECK(v.apply(fn0, A{}) == 3 + 14 + 15 + 92);
  CHECK(fn::apply(fn0, FWD(v), A{}) == 3 + 14 + 15 + 92);
  static_assert(fn::apply(fn0, fn::pack{3, 14, 15, 92}, A{}) == 3 + 14 + 15 + 92);

  A a;
  constexpr auto fn1 = [](int i, int j, int k, int l, A &dest) noexcept -> A & {
    dest.v = (i + j + k + l);
    return dest;
  };
  CHECK(v.apply(fn1, a).v == 3 + 14 + 15 + 92);
  CHECK(v.apply_r<A>(fn1, a).v == 3 + 14 + 15 + 92);
  CHECK(v.apply_r<long>([](auto... args) noexcept -> int { return (0 + ... + args); }, 65, 35)
        == 3 + 14 + 15 + 92 + 65 + 35);
  CHECK(fn::apply_r<long>([](auto... args) noexcept -> int { return (0 + ... + args); }, FWD(v), 65, 35)
        == 3 + 14 + 15 + 92 + 65 + 35);
  static_assert(
      fn::apply_r<long>([](auto... args) noexcept -> int { return (0 + ... + args); }, fn::pack{3, 14}, 15, 92)
      == 3 + 14 + 15 + 92);
  CHECK(v.apply_r<long>(fn0, A{}) == 3 + 14 + 15 + 92);
  CHECK(fn::apply_r<long>(fn0, FWD(v), A{}) == 3 + 14 + 15 + 92);
  static_assert(fn::apply_r<long>(fn0, fn::pack{3, 14, 15, 92}, A{}) == 3 + 14 + 15 + 92);

  static_assert(std::is_same_v<decltype(v.apply(fn1, a)), A &>);
  static_assert(std::is_same_v<decltype(v.apply_r<A>(fn1, a)), A>);

  constexpr auto fn2 = [](int, int, int, int, A &&dest) noexcept -> A && { return std::move(dest); };
  static_assert(std::is_same_v<decltype(v.apply(fn2, std::move(a))), A &&>);
  static_assert(std::is_same_v<decltype(v.apply_r<A>(fn2, std::move(a))), A>);

  constexpr auto fn3 = [](int, int, int, int, A &&dest) noexcept -> A { return dest; };
  static_assert(std::is_same_v<decltype(v.apply(fn3, std::move(a))), A>);
  static_assert(std::is_same_v<decltype(v.apply_r<A>(fn3, std::move(a))), A>);

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
  static_assert(c2.apply([](auto i, auto j) {
    if constexpr (std::is_same_v<decltype(i), bool> && std::is_same_v<decltype(j), int>)
      return i && j == 12;
    else
      return false;
  }));

  // pack is a structural type: a constexpr pack can be a template parameter, with
  // template-argument equivalence comparing element-wise
  constexpr pack<int, double> s1{3, 0.5};
  constexpr pack<int, double> s2{3, 0.5};
  constexpr pack<int, double> s3{4, 0.5};
  static_assert(std::is_same_v<pack_nttp<s1>, pack_nttp<s2>>);
  static_assert(not std::is_same_v<pack_nttp<s1>, pack_nttp<s3>>);
  static_assert(std::is_same_v<some_pack_nttp<s1>, some_pack_nttp<s2>>);
  static_assert(not std::is_same_v<some_pack_nttp<s1>, some_pack_nttp<s3>>);
  CHECK(read_nttp<s1>() == 3); // the template-parameter object is usable at runtime

  SECTION("element mandate")
  {
    // the algebra's own constructors are not elements; everything else is an opaque atom
    static_assert(fn::detail::_is_valid_pack_element<int>);
    static_assert(fn::detail::_is_valid_pack_element<int &>);
    static_assert(fn::detail::_is_valid_pack_element<int const>);
    static_assert(fn::detail::_is_valid_pack_element<std::tuple<int, A>>);
    static_assert(fn::detail::_is_valid_pack_element<std::array<int, 2>>);
    static_assert(fn::detail::_is_valid_pack_element<fn::choice<int>>);
    static_assert(not fn::detail::_is_valid_pack_element<fn::pack<int>>);
    static_assert(not fn::detail::_is_valid_pack_element<fn::pack<>>);
    static_assert(not fn::detail::_is_valid_pack_element<fn::copack<int>>);

    // witnesses that the permitted atoms instantiate
    static_assert(pack<std::tuple<int, int>, int>::size == 2);
    static_assert(pack<fn::choice<int>, int>::size == 2);
    SUCCEED();
  }

  SECTION("elements are terminal")
  {
    // every element is handed over whole via INVOKE - a lone tuple-like element included, exactly
    // as it is treated when siblings or extra arguments accompany it
    constexpr auto arity = [](auto &&...args) noexcept -> int { return (0 + ... + (static_cast<void>(args), 1)); };
    CHECK(pack<std::tuple<int, int>>{std::tuple{1, 2}}.apply(arity) == 1);
    CHECK(pack<std::tuple<int, int>, int>{std::tuple{1, 2}, 3}.apply(arity) == 2);
    CHECK(pack<std::tuple<int, int>>{std::tuple{1, 2}}.apply(arity, 0) == 2);
    CHECK(pack<std::tuple<int, int>>{std::tuple{1, 2}}.apply_r<long>(arity) == 1L);

    // the callable is served for the whole element, never its pieces
    constexpr auto whole = [](std::tuple<int, int> const &t) noexcept -> int { return std::get<0>(t); };
    CHECK(pack<std::tuple<int, int>>{std::tuple{1, 2}}.apply(whole) == 1);
    static_assert(not can_invoke_r<pack<std::tuple<int, int>>, int, decltype([](int, int) -> int { return 0; })>);

    SECTION("constexpr")
    {
      static_assert(pack<std::tuple<int, int>>{std::tuple{1, 2}}.apply(arity) == 1);
      static_assert(pack<std::tuple<int, int>, int>{std::tuple{1, 2}, 3}.apply(arity) == 2);
      static_assert(pack<std::tuple<int, int>>{std::tuple{1, 2}}.apply(arity, 0) == 2);
      static_assert(pack<std::tuple<int, int>>{std::tuple{1, 2}}.apply(whole) == 1);
      SUCCEED();
    }
  }

  SECTION("apply_r return conversion")
  {
    struct NotFromInt final {
      explicit NotFromInt(std::nullptr_t) {}
    };
    constexpr auto sum_fn = [](auto... args) noexcept -> int { return (0 + ... + args); };

    static_assert(can_invoke_r<T &, long, decltype(sum_fn)>);
    static_assert(not can_invoke_r<T &, NotFromInt, decltype(sum_fn)>); // int does not convert to it
    SUCCEED();
  }

  SECTION("noexcept")
  {
    // apply and apply_r weigh the callback they dispatch to, in every value category
    constexpr auto throwing = [](auto &&...) noexcept(false) -> int { return 0; };
    static_assert(not noexcept(throwing()));
    static_assert(not noexcept(v.apply(throwing)));
    static_assert(not noexcept(std::as_const(v).apply(throwing)));
    static_assert(not noexcept(std::move(v).apply(throwing)));
    static_assert(not noexcept(std::move(std::as_const(v)).apply(throwing)));
    static_assert(not noexcept(v.apply_r<long>(throwing)));
    static_assert(not noexcept(std::move(v).apply_r<long>(throwing)));

    constexpr auto nothrow = [](auto &&...) noexcept -> int { return 0; };
    static_assert(noexcept(v.apply(nothrow)));
    static_assert(noexcept(v.apply_r<long>(nothrow)));

    // an empty pack wraps nothing, so there is nothing that could throw (SECTION("as_pack") weighs
    // the arguments a non-empty one relocates)
    static_assert(noexcept(fn::as_pack()));
    SUCCEED();
  }

  SECTION("constexpr")
  {
    constexpr fn::pack<int, int> v2{3, 14};
    constexpr auto r2 = v2.apply([](auto &&...args) constexpr noexcept -> int { return (0 + ... + args); });
    static_assert(r2 == 3 + 14);
    SUCCEED();
  }
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

  SECTION("explicit type selection")
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

    SECTION("constructor takes parameters")
    {
      CHECK(s.append(std::in_place_type<B>, 5, 6).apply(check));
      CHECK(std::as_const(s).append(std::in_place_type<B>, 5, 6).apply(check));
      CHECK(std::move(std::as_const(s)).append(std::in_place_type<B>, 5, 6).apply(check));
      CHECK(std::move(s).append(std::in_place_type<B>, 5, 6).apply(check));
    }

    SECTION("constructor takes parameters apply_r")
    {
      CHECK(s.append(std::in_place_type<B>, 5, 6).apply_r<bool>(check));
      CHECK(std::as_const(s).append(std::in_place_type<B>, 5, 6).apply_r<bool>(check));
      CHECK(std::move(std::as_const(s)).append(std::in_place_type<B>, 5, 6).apply_r<bool>(check));
      CHECK(std::move(s).append(std::in_place_type<B>, 5, 6).apply_r<bool>(check));
    }

    SECTION("default constructor")
    {
      CHECK(s.append(std::in_place_type<C>).apply(check));
      CHECK(std::as_const(s).append(std::in_place_type<C>).apply(check));
      CHECK(std::move(std::as_const(s)).append(std::in_place_type<C>).apply(check));
      CHECK(std::move(s).append(std::in_place_type<C>).apply(check));
    }

    SECTION("default constructor apply_r")
    {
      CHECK(s.append(std::in_place_type<C>).apply_r<int>(check) == 1);
      CHECK(std::as_const(s).append(std::in_place_type<C>).apply_r<int>(check) == 1);
      CHECK(std::move(std::as_const(s)).append(std::in_place_type<C>).apply_r<int>(check) == 1);
      CHECK(std::move(s).append(std::in_place_type<C>).apply_r<int>(check) == 1);
    }

    SECTION("aggregate forwarding")
    {
      // braces elide through the aggregate: three ints construct the array element in place
      auto q = s.append(std::in_place_type<std::array<int, 3>>, 5, 6, 7);
      static_assert(std::same_as<decltype(q), T::append_type<std::array<int, 3>>>);
      CHECK(q.apply([](int, std::string_view, A, std::array<int, 3> const &a) //
                    { return a[0] * 100 + a[1] * 10 + a[2]; })
            == 567);
      static_assert(fn::pack<>{}.append(std::in_place_type<std::array<int, 3>>, 5, 6, 7).apply([](auto const &a) {
        return a[0] * 100 + a[1] * 10 + a[2];
      }) == 567);
    }
  }

  SECTION("deduced type")
  {
    static_assert(std::same_as<decltype(s.append(B{5, 6})), T::append_type<B>>);

    constexpr C c1{};
    static_assert(std::same_as<decltype(s.append(c1)), T::append_type<C const &>>);
    static_assert(std::same_as<T::append_type<C const &>, pack<int, std::string_view, A, C const &>>);

    C c2{};
    static_assert(std::same_as<decltype(s.append(c2)), T::append_type<C &>>);
    static_assert(std::same_as<T::append_type<C &>, pack<int, std::string_view, A, C &>>);

    CHECK(s.append(B{30}).apply(check));
    CHECK(std::as_const(s).append(B{30}).apply(check));
    CHECK(std::move(std::as_const(s)).append(B{30}).apply(check));
    CHECK(std::move(s).append(B{30}).apply(check));
  }

  SECTION("reference element")
  {
    // Evaluated, not merely decltype'd: a reference element must BIND to the argument, and the
    // element-initializing expression is only instantiated when the call is actually made
    C c{};
    auto q = s.append(std::in_place_type<B &>, c);
    static_assert(std::same_as<decltype(q), T::append_type<B &>>);
    CHECK(q.apply([&c](int, std::string_view, A, B &b) { return &b == static_cast<B *>(&c); }));

    auto r = s.append(c); // deduced, so the element type is C &
    static_assert(std::same_as<decltype(r), T::append_type<C &>>);
    CHECK(r.apply([&c](int, std::string_view, A, C &x) { return &x == &c; }));

    c.v = 77; // the same object, observed through both packs
    CHECK(q.apply([](int, std::string_view, A, B &b) { return b.v == 77; }));
    CHECK(r.apply([](int, std::string_view, A, C &x) { return x.v == 77; }));

    SECTION("constexpr")
    {
      static_assert([] {
        fn::pack<int> p{1};
        B b{5, 6};
        auto q = p.append(std::in_place_type<B &>, b);
        auto r = p.append(b);
        b.v = 9;
        return q.apply([](int, B &x) { return x.v == 9; }) && r.apply([](int, B &x) { return x.v == 9; });
      }());
      SUCCEED();
    }
  }

  SECTION("pack on the right side, deduced")
  {
    constexpr fn::pack<bool, int, B> a{true, 3, B{14}};
    constexpr fn::pack<C, B> b{C{}, B{3, 4}};
    constexpr auto c1 = a.append(b);
    static_assert(std::same_as<decltype(c1), fn::pack<bool, int, B, C, B> const>);
    static_assert(c1.apply([](bool i, int j, B const &b1, C const &c, B const &b2) {
      return i && j == 3 && b1.v == 14 && c.v == 30 && b2.v == 12;
    }));

    auto c2 = a.append(fn::pack{C{}, B{4, 5}});
    static_assert(std::same_as<decltype(c2), fn::pack<bool, int, B, C, B>>);
    CHECK(c2.apply([](bool i, int j, B const &b1, C const &c, B const &b2) {
      return i && j == 3 && b1.v == 14 && c.v == 30 && b2.v == 20;
    }));

    // a pack whose only element is tuple-like splices it whole, never unpacked
    constexpr auto c3 = a.append(fn::pack<std::tuple<int, int>>{std::tuple{2, 3}});
    static_assert(std::same_as<decltype(c3), fn::pack<bool, int, B, std::tuple<int, int>> const>);
    static_assert(std::get<0>(fn::get<3>(c3)) == 2);
  }

  SECTION("pack on the right side, tag form")
  {
    // the tag form splices too: a prebuilt matching pack relocates directly, any other arguments
    // construct the named pack first - either way the result is the flat concatenation
    constexpr fn::pack<bool, int> a{true, 3};
    constexpr auto c1 = a.append(std::in_place_type<fn::pack<C, B>>, fn::pack<C, B>{C{}, B{3, 4}});
    static_assert(std::same_as<decltype(c1), fn::pack<bool, int, C, B> const>);
    static_assert(
        c1.apply([](bool i, int j, C const &c, B const &b) { return i && j == 3 && c.v == 30 && b.v == 12; }));

    constexpr auto c2 = a.append(std::in_place_type<fn::pack<C, B>>, C{}, B{4, 5});
    static_assert(std::same_as<decltype(c2), fn::pack<bool, int, C, B> const>);
    static_assert(
        c2.apply([](bool i, int j, C const &c, B const &b) { return i && j == 3 && c.v == 30 && b.v == 20; }));

    constexpr auto c3 = a.append(std::in_place_type<fn::pack<>>);
    static_assert(std::same_as<decltype(c3), fn::pack<bool, int> const>);

    constexpr auto c4 = a.append(std::in_place_type<fn::pack<std::tuple<int, int>>>, std::tuple{2, 3});
    static_assert(std::same_as<decltype(c4), fn::pack<bool, int, std::tuple<int, int>> const>);
    static_assert(std::get<0>(fn::get<2>(c4)) == 2);

    auto c5 = a.append(std::in_place_type<fn::pack<C, B>>, C{}, B{4, 6});
    CHECK(c5.apply([](bool i, int j, C const &c, B const &b) { return i && j == 3 && c.v == 30 && b.v == 24; }));
  }

  SECTION("constraints")
  {
    static_assert(can_append_in_place<T &, B, int>);
    static_assert(can_append_in_place<T &, B, int, int>);
    static_assert(not can_append_in_place<T &, B, char const *>); // B is not constructible from it

    // the element is brace-initialized, so an aggregate is appended element-wise, exactly as `copack`
    // constructs one - a constraint spelled with is_constructible_v would reject this
    static_assert(can_append_in_place<T &, std::array<int, 3>, int, int, int>);
    static_assert(not can_append_in_place<T &, std::array<int, 3>, int, int, int, int>); // one too many
    static_assert(not can_append_in_place<T &, int, double>);                            // narrowing

    // in_place_type selects the element type, it is never itself an element: with no arguments and
    // no default constructor there is nothing to construct, and the deduced-Arg overload must not
    // pick the call up and append the tag instead
    static_assert(not can_append_in_place<T &, B>);
    static_assert(not can_append<T &, std::in_place_type_t<B> const &>);
    // C has a default constructor, so the same call still means "construct the element"
    static_assert(can_append_in_place<T &, C>);
    static_assert(std::same_as<decltype(std::declval<T &>().append(std::in_place_type<C>)), T::append_type<C>>);

    // A pack never holds a copack, in either spelling - and asking must answer, not hard-error
    static_assert(not can_append<T &, fn::copack<int>>);
    static_assert(not can_append<T &, fn::copack<int> &>);
    static_assert(not can_append_in_place<T &, fn::copack<int>, fn::copack<int>>);
    static_assert(not can_append_in_place<T &, fn::copack_for<bool, int>, fn::copack_for<bool, int>>);

    // A pack never holds a pack either: appending one splices, in every spelling and every value
    // category of the subject - the deduced form relocates the given pack's elements, the tag form
    // constructs the named pack from the arguments and splices that
    static_assert(std::same_as<T::append_type<fn::pack<B, C>>, pack<int, std::string_view, A, B, C>>);
    static_assert(can_append<T &, fn::pack<int>>);
    static_assert(can_append<T &, fn::pack<int> &>);
    static_assert(can_append<T const &, fn::pack<int>>);
    static_assert(can_append<T &&, fn::pack<int>>);
    static_assert(can_append<T const &&, fn::pack<int>>);
    static_assert(can_append_in_place<T &, fn::pack<int>, int>);
    static_assert(can_append_in_place<T const &, fn::pack<int>, int>);
    static_assert(can_append_in_place<T &&, fn::pack<int>, int>);
    static_assert(can_append_in_place<T const &&, fn::pack<int>, int>);
    static_assert(can_append_in_place<T &, fn::pack<double, int>, double, int>);
    static_assert(can_append_in_place<T &, fn::pack<int>>); // value-initialized element
    static_assert(can_append_in_place<T &, fn::pack<>>);    // zero elements contributed
    // the construction is asked the brace question, and asking must answer, not hard-error -
    // including a tag that merely NAMES an ill-formed pack
    static_assert(not can_append_in_place<T &, fn::pack<B>>);                       // B has no default constructor
    static_assert(not can_append_in_place<T &, fn::pack<B>, char const *>);         // B is not constructible from it
    static_assert(not can_append_in_place<T &, fn::pack<int>, int, int>);           // one too many
    static_assert(not can_append_in_place<T &, fn::pack<double>, fn::pack<float>>); // not the tag's pack
    static_assert(not can_append_in_place<T &, fn::pack<fn::pack<int>>, int>);
    static_assert(not can_append_in_place<T &, fn::pack<fn::copack<int>>, int>);

    // relocation is part of the question: the elements already held move into the new pack in the
    // pack's own value category, and an element which cannot make that move rejects the append
    // instead of hard-erroring inside it
    static_assert(can_append_in_place<pack<std::unique_ptr<int>> &&, int, int>);
    static_assert(not can_append_in_place<pack<std::unique_ptr<int>> &, int, int>);
    static_assert(not can_append_in_place<pack<std::unique_ptr<int>> const &, int, int>);
    static_assert(not can_append<pack<std::unique_ptr<int>> const &, int>);
    // the same guard covers both operands when appending a whole pack
    static_assert(can_append<pack<int> &, pack<std::unique_ptr<int>>>);
    static_assert(not can_append<pack<int> &, pack<std::unique_ptr<int>> const &>);
    static_assert(not can_append<pack<std::unique_ptr<int>> const &, pack<int>>);
    // const propagates through a reference element, so a const pack cannot rebind it into the new
    // pack's non-const reference element - the same pack, non-const, can
    static_assert(can_append_in_place<pack<int &> &, int, int>);
    static_assert(not can_append_in_place<pack<int &> const &, int, int>);

    SUCCEED();
  }

  SECTION("noexcept")
  {
    // append weighs both what it constructs and every element it relocates into the new pack
    using P = pack<Throwing>;
    static_assert(not std::is_nothrow_copy_constructible_v<Throwing>);
    static_assert(not std::is_nothrow_move_constructible_v<Throwing>);

    // constructing the appended element by a throwing copy
    static_assert(not noexcept(
        std::declval<pack<int> &>().append(std::in_place_type<Throwing>, std::declval<Throwing const &>())));
    // ... and relocating an existing throwing element, even where the appended one cannot throw.
    // Which relocation that is depends on the source's value category, and helper_t declares one
    // constructor per category: only its const-lvalue copy and its rvalue move can throw, while the
    // non-const-lvalue copy is noexcept - so the promise tracks the constructor actually selected.
    static_assert(noexcept(std::declval<P &>().append(std::in_place_type<int>, 1)));
    static_assert(not noexcept(std::declval<P const &>().append(std::in_place_type<int>, 1)));
    static_assert(not noexcept(std::declval<P &&>().append(std::in_place_type<int>, 1)));
    static_assert(noexcept(std::declval<P &>().append(1))); // deduced form, same relocation

    // neither constructed nor relocated element can throw here
    static_assert(noexcept(std::declval<pack<int> &>().append(std::in_place_type<int>, 1)));
    static_assert(noexcept(std::declval<pack<int> &>().append(1)));
    SUCCEED();
  }
}

TEST_CASE("pack noexcept", "[pack][noexcept]")
{
  using fn::pack;

  struct Throwy {
    Throwy() = default;
    Throwy(Throwy const &) noexcept(false) {}
    Throwy(Throwy &&) noexcept(false) {}
  };
  struct Quiet {
    Quiet() = default;
    Quiet(Quiet const &) noexcept {}
    Quiet(Quiet &&) noexcept {}
  };
  struct Evil {
    Evil() = default;
    explicit Evil(Evil &&) noexcept {}
    Evil(Evil const &) { throw std::runtime_error{"copied"}; }
  };

  SECTION("append")
  {
    static_assert(noexcept(std::declval<pack<int> &>().append(std::in_place_type<int>, 1)));
    static_assert(noexcept(std::declval<pack<Quiet> &>().append(std::in_place_type<int>, 1)));

    // the element being appended can throw on construction ...
    static_assert(
        not noexcept(std::declval<pack<int> &>().append(std::in_place_type<Throwy>, std::declval<Throwy const &>())));
    // ... and so can relocating an element the pack already holds, even where the new one cannot
    static_assert(not noexcept(std::declval<pack<Throwy> &>().append(std::in_place_type<int>, 1)));
    static_assert(not noexcept(std::declval<pack<Throwy> &>().append(1))); // deduced form

    // relocation copy-initializes ([dcl.init.aggr]/4.3), and copy-initialization cannot reach an
    // explicit constructor - a promise computed from is_nothrow_constructible_v would see the
    // explicit nothrow move and promise what the deed cannot keep: this throw must propagate
    static_assert(std::is_nothrow_constructible_v<Evil, Evil>); // the questions disagree here ...
    static_assert(not noexcept(std::declval<pack<Evil> &&>().append(std::in_place_type<int>, 1))); // ... deed answers
    pack<Evil> p{Evil{}}; // elided, nothing copied yet
    CHECK_THROWS_AS(std::move(p).append(std::in_place_type<int>, 1), std::runtime_error);
    SUCCEED();
  }

  SECTION("as_pack")
  {
    // asked of the braces as_pack performs - a question about pack's (nonexistent) constructors
    // would answer false for every argument list
    static_assert(noexcept(fn::as_pack(true, 12)));
    static_assert(std::same_as<decltype(fn::as_pack(true, 12)), fn::pack<bool, int>>);
    static_assert(not noexcept(fn::as_pack(Throwy{}))); // moving the argument in can throw ...
    Throwy t{};
    static_assert(noexcept(fn::as_pack(t))); // ... but an lvalue binds, and binding cannot
    struct NoMove {
      NoMove() = default;
      NoMove(NoMove &&) = delete;
    };
    static_assert(not can_as_pack<NoMove>); // constrained on the same initialization ...
    static_assert(can_as_pack<NoMove &>);   // ... which a binding reference element satisfies
    CHECK(fn::as_pack(t, 12).apply([&t](Throwy const &x, int i) { return &x == &t && i == 12; }));
  }

  SECTION("apply")
  {
    constexpr auto nothrow_fn = [](auto &&...) noexcept { return 0; };
    constexpr auto throwing_fn = [](auto &&...) { return 0; };
    static_assert(noexcept(std::declval<pack<int, bool> &>().apply(nothrow_fn)));
    static_assert(not noexcept(std::declval<pack<int, bool> &>().apply(throwing_fn)));
    static_assert(noexcept(std::declval<pack<int, bool> &>().template apply_r<int>(nothrow_fn)));
    static_assert(not noexcept(std::declval<pack<int, bool> &>().template apply_r<int>(throwing_fn)));

    // Ret is entered by INVOKE<R>'s implicit conversion; the promise asks that call rather than
    // is_nothrow_constructible_v, whose direct-initialization would reach the explicit move
    static_assert(std::is_nothrow_constructible_v<Evil, Evil &&>);
    pack<int> p{1};
    Evil ev{};
    auto give_evil = [&ev](int) noexcept -> Evil && { return std::move(ev); };
    static_assert(not noexcept(std::declval<pack<int> &>().template apply_r<Evil>(give_evil)));
    CHECK_THROWS_AS(p.apply_r<Evil>(give_evil), std::runtime_error);
    // exactly std::apply_r's own promise - conservative for a prvalue result (the deed elides,
    // and indeed nothing throws below), but never a lie
    auto make_evil = [](int) noexcept -> Evil { return {}; };
    static_assert(noexcept(std::declval<pack<int> &>().template apply_r<Evil>(make_evil))
                  == std::is_nothrow_invocable_r_v<Evil, decltype(make_evil) &, int &>);
    CHECK_NOTHROW(p.apply_r<Evil>(make_evil));
  }
}

TEST_CASE("pack with immovable data", "[pack][immovable]")
{
  using fn::pack;

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

  constexpr auto can_invoke = [](auto &&fn) constexpr { return requires { std::declval<T>().apply(fn); }; };

  static_assert(can_invoke([](auto &&...) {})); // generic call
  static_assert(can_invoke([](ImmovableType const &, ImmovableType const &, ImmovableType const &,
                              ImmovableType const &) {})); // pass everything by const reference
  static_assert(can_invoke([](ImmovableType &&, ImmovableType const &&, ImmovableType &, ImmovableType const &) {
  }));                                                             // bind rvalues and lvalues
  static_assert(not can_invoke([](ImmovableType, auto &&...) {})); // cannot pass immovable by value

  CHECK(v.apply([](auto &&...args) noexcept -> int { return (0 + ... + args.value); }) == 3 + 14 + 15 + 92);
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

namespace {
constexpr auto join_witness = [](auto &&...v) -> int { return (0 + ... + v.value); };

// One join algebra, two layers walking the same shape grid: the TEMPLATE_TEST_CASE below runs the
// battery through each subject, in both constant and runtime evaluation.
struct join_via_optional final { // fn::detail::_join over engaged optionals - the monads' layer
  template <typename R, typename LH, typename RH> static constexpr auto join(LH const &lh, RH const &rh)
  {
    constexpr auto efn = [](auto &&...) { return std::nullopt; };
    auto const r = fn::detail::_join<fn::optional>(fn::optional<LH>{lh}, fn::optional<RH>{rh}, efn);
    static_assert(std::is_same_v<decltype(r), fn::optional<R> const>);
    return r.value();
  }
};

struct join_via_operator final { // the public operator& over bare copacks, packs and values
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
  { // copack of packs join copack of scalars
    using R = copack_for<pack<Alef, Gimel, Heh>, pack<Alef, Gimel, Vav>, pack<Alef, Gimel, Zayn>, //
                         pack<Bet, Gimel, Heh>, pack<Bet, Gimel, Vav>, pack<Bet, Gimel, Zayn>>;
    auto const r = S::template join<R>(copack_for<pack<Alef, Gimel>, pack<Bet, Gimel>>{pack{Alef{3}, Gimel{14}}},
                                       copack<Heh, Vav, Zayn>{Vav{15}});
    ok = ok && r.template has_value<pack<Alef, Gimel, Vav>>() && r.apply(join_witness) == 3 + 14 + 15;
  }
  {                                                                            // copack of packs join copack of packs
    using R = copack_for<pack<Alef, Gimel, Heh, Zayn>, pack<Alef, Gimel, Vav>, //
                         pack<Bet, Gimel, Heh, Zayn>, pack<Bet, Gimel, Vav>>;
    auto const r = S::template join<R>(copack_for<pack<Alef, Gimel>, pack<Bet, Gimel>>{pack{Alef{3}, Gimel{14}}},
                                       copack<pack<Heh, Zayn>, pack<Vav>>{pack{Vav{15}}});
    ok = ok && r.template has_value<pack<Alef, Gimel, Vav>>() && r.apply(join_witness) == 3 + 14 + 15;
  }
  { // copack of scalars join copack of scalars
    using R = copack_for<pack<Alef, Heh>, pack<Alef, Vav>, pack<Alef, Zayn>, pack<Bet, Heh>, pack<Bet, Vav>,
                         pack<Bet, Zayn>, pack<Gimel, Heh>, pack<Gimel, Vav>, pack<Gimel, Zayn>>;
    auto const r = S::template join<R>(copack_for<Alef, Bet, Gimel>{Gimel{3}}, copack<Heh, Vav, Zayn>{Vav{14}});
    ok = ok && r.template has_value<pack<Gimel, Vav>>() && r.apply(join_witness) == 3 + 14;
  }
  { // copack of scalars join copack of packs
    using R = copack_for<pack<Alef, Heh, Zayn>, pack<Alef, Vav>, pack<Bet, Heh, Zayn>, //
                         pack<Bet, Vav>, pack<Gimel, Heh, Zayn>, pack<Gimel, Vav>>;
    auto const r = S::template join<R>(copack_for<Alef, Bet, Gimel>{Gimel{3}},
                                       copack<pack<Heh, Zayn>, pack<Vav>>{pack{Vav{14}}});
    ok = ok && r.template has_value<pack<Gimel, Vav>>() && r.apply(join_witness) == 3 + 14;
  }
  { // copack of packs join scalar
    using R = copack_for<pack<Alef, Gimel, Vav>, pack<Bet, Gimel, Vav>>;
    auto const r
        = S::template join<R>(copack_for<pack<Alef, Gimel>, pack<Bet, Gimel>>{pack{Alef{3}, Gimel{14}}}, Vav{15});
    ok = ok && r.template has_value<pack<Alef, Gimel, Vav>>() && r.apply(join_witness) == 3 + 14 + 15;
  }
  { // copack of packs join pack
    using R = copack_for<pack<Alef, Gimel, Vav>, pack<Bet, Gimel, Vav>>;
    auto const r = S::template join<R>(copack_for<pack<Alef, Gimel>, pack<Bet, Gimel>>{pack{Alef{3}, Gimel{14}}},
                                       pack<Vav>{pack{Vav{15}}});
    ok = ok && r.template has_value<pack<Alef, Gimel, Vav>>() && r.apply(join_witness) == 3 + 14 + 15;
  }
  { // copack of scalars join scalar
    using R = copack_for<pack<Alef, Vav>, pack<Bet, Vav>, pack<Gimel, Vav>>;
    auto const r = S::template join<R>(copack_for<Alef, Bet, Gimel>{Gimel{3}}, Vav{14});
    ok = ok && r.template has_value<pack<Gimel, Vav>>() && r.apply(join_witness) == 3 + 14;
  }
  { // copack of scalars join pack
    using R = copack_for<pack<Alef, Vav>, pack<Bet, Vav>, pack<Gimel, Vav>>;
    auto const r = S::template join<R>(copack_for<Alef, Bet, Gimel>{Gimel{3}}, pack<Vav>{pack{Vav{14}}});
    ok = ok && r.template has_value<pack<Gimel, Vav>>() && r.apply(join_witness) == 3 + 14;
  }
  { // pack join copack of scalars
    using R = copack<pack<Alef, Gimel, Heh>, pack<Alef, Gimel, Vav>, pack<Alef, Gimel, Zayn>>;
    auto const r = S::template join<R>(pack<Alef, Gimel>{pack{Alef{3}, Gimel{14}}}, copack<Heh, Vav, Zayn>{Vav{15}});
    ok = ok && r.template has_value<pack<Alef, Gimel, Vav>>() && r.apply(join_witness) == 3 + 14 + 15;
  }
  { // pack join copack of packs
    using R = copack<pack<Alef, Gimel, Heh, Zayn>, pack<Alef, Gimel, Vav>>;
    auto const r = S::template join<R>(pack<Alef, Gimel>{pack{Alef{3}, Gimel{14}}},
                                       copack<pack<Heh, Zayn>, pack<Vav>>{pack{Vav{15}}});
    ok = ok && r.template has_value<pack<Alef, Gimel, Vav>>() && r.apply(join_witness) == 3 + 14 + 15;
  }
  { // pack join scalar
    auto const r = S::template join<pack<Alef, Gimel, Vav>>(pack<Alef, Gimel>{pack{Alef{3}, Gimel{14}}}, Vav{15});
    ok = ok && r.apply(join_witness) == 3 + 14 + 15;
  }
  { // pack join pack
    auto const r = S::template join<pack<Alef, Gimel, Vav>>(pack<Alef, Gimel>{pack{Alef{3}, Gimel{14}}},
                                                            pack<Vav>{pack{Vav{15}}});
    ok = ok && r.apply(join_witness) == 3 + 14 + 15;
  }
  return ok;
}

// A bare value can sit on the LEFT of the monad-level join only - operator& has no overload for it,
// so these four shapes belong to join_via_optional alone.
constexpr bool join_value_lhs_battery()
{
  using fn::copack;
  using fn::pack;
  using S = join_via_optional;

  bool ok = true;
  { // scalar join copack of scalars
    using R = copack<pack<Alef, Heh>, pack<Alef, Vav>, pack<Alef, Zayn>>;
    auto const r = S::join<R>(Alef{3}, copack<Heh, Vav, Zayn>{Vav{14}});
    ok = ok && r.template has_value<pack<Alef, Vav>>() && r.apply(join_witness) == 3 + 14;
  }
  { // scalar join copack of packs
    using R = copack<pack<Alef, Heh, Zayn>, pack<Alef, Vav>>;
    auto const r = S::join<R>(Alef{3}, copack<pack<Heh, Zayn>, pack<Vav>>{pack{Vav{14}}});
    ok = ok && r.template has_value<pack<Alef, Vav>>() && r.apply(join_witness) == 3 + 14;
  }
  { // scalar join scalar
    auto const r = S::join<pack<Alef, Vav>>(Alef{3}, Vav{14});
    ok = ok && r.apply(join_witness) == 3 + 14;
  }
  { // scalar join pack
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

  // a pack whose only element is tuple-like splices whole through the join
  constexpr auto r3 = fn::as_copack(12) & fn::pack<std::tuple<int, int>>{std::tuple{1, 2}};
  static_assert(std::is_same_v<decltype(r3), fn::copack<fn::pack<int, std::tuple<int, int>>> const>);
  static_assert(r3.apply([](int i, std::tuple<int, int> const &t) { return i == 12 && std::get<0>(t) == 1; }));

  SECTION("noexcept")
  {
    // This operator& builds a pack from operands it relocates, and weighs that: nothing here can
    // throw, so it promises noexcept - as optional's and expected's joins now do.
    static_assert(noexcept(std::declval<fn::pack<int> &>() & 2));
    static_assert(noexcept(std::declval<fn::copack<int> &>() & 2));
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
  static_assert(fn::disjoin(EA{1}) == EA{1}); // unary forwards unchanged
  static_assert(fn::disjoin(EA{::fn::unexpect, FileNotFound}, EB{true}) == fn::copack{true});
  static_assert(
      fn::disjoin(fn::just<void>{}, fn::just<void>{}, fn::just<int>{7}).apply([]([[maybe_unused]] auto &&...args) {
        return sizeof...(args);
      })
      == 0); // left catch through the whole chain
  CHECK(bool(fn::disjoin(EA{::fn::unexpect, FileNotFound}, EB{true})
             == fn::copack{true})); // bool(): Catch2 decomposition re-enters the == constraint

  // a non-viable fold answers instead of erroring
  constexpr auto can = [](auto &&...args) { return requires { fn::disjoin(FWD(args)...); }; };
  static_assert(can(EA{1}, EB{true}));
  static_assert(not can(EA{1}, 42));
}

namespace {
template <typename P, std::size_t I>
concept can_get = requires(P p) { fn::get<I>(static_cast<P &&>(p)); };
} // namespace

TEST_CASE("pack get and tuple protocol", "[pack][get][tuple]")
{
  using fn::get;
  using fn::pack;

  // tuple_size / tuple_element
  static_assert(std::tuple_size_v<pack<int, double, A>> == 3);
  static_assert(std::tuple_size_v<pack<>> == 0);
  static_assert(std::same_as<std::tuple_element_t<0, pack<int, double, A>>, int>);
  static_assert(std::same_as<std::tuple_element_t<1, pack<int, double, A>>, double>);
  static_assert(std::same_as<std::tuple_element_t<2, pack<int, double, A>>, A>);
  // the library's tuple_element<I, const T> propagates const onto value elements
  static_assert(std::same_as<std::tuple_element_t<0, pack<int, double, A> const>, int const>);

  // get value categories mirror what apply passes
  pack<int, double> p{2, 4};
  static_assert(std::same_as<decltype(get<0>(p)), int &>);
  static_assert(std::same_as<decltype(get<1>(p)), double &>);
  static_assert(std::same_as<decltype(get<0>(std::as_const(p))), int const &>);
  static_assert(std::same_as<decltype(get<0>(std::move(p))), int &&>);
  static_assert(std::same_as<decltype(get<0>(std::move(std::as_const(p)))), int const &&>);

  CHECK(get<0>(p) == 2);
  CHECK(get<1>(p) == 4.0);
  get<0>(p) = 7;
  CHECK(get<0>(p) == 7);

  // accurate, unlike the members that relocate elements: get only forms a reference
  static_assert(noexcept(get<0>(p)));
  static_assert(noexcept(get<0>(std::move(p))));
  static_assert(noexcept(get<0>(std::declval<pack<Throwing> &>())));

  // out-of-range index is not viable (SFINAE-clean, not a hard error)
  static_assert(can_get<pack<int, double> &, 0>);
  static_assert(can_get<pack<int, double> &, 1>);
  static_assert(not can_get<pack<int, double> &, 2>);
  static_assert(not can_get<pack<> &, 0>);

  // reference-holding element: a non-const pack yields the stored reference
  int x = 11;
  pack<int &> r{x};
  static_assert(std::same_as<std::tuple_element_t<0, pack<int &>>, int &>);
  static_assert(std::same_as<decltype(get<0>(r)), int &>);
  get<0>(r) = 12;
  CHECK(x == 12);

  // const pack of a reference: fn propagates const through the reference (unlike
  // std::tuple<int&>, whose reference member is immune to container const), so
  // get and tuple_element agree on int const& here.
  static_assert(std::same_as<std::tuple_element_t<0, pack<int &> const>, int const &>);
  static_assert(std::same_as<decltype(get<0>(std::as_const(r))), int const &>);
  static_assert(std::same_as<decltype(get<0>(std::move(std::as_const(r)))), int const &>);
  CHECK(get<0>(std::as_const(r)) == 12);
  // a const structured binding over the const pack observes the const-propagated reference
  {
    auto const &[e0] = std::as_const(r);
    static_assert(std::same_as<decltype(e0), int const &>);
    CHECK(e0 == 12);
  }

  // structured bindings over an lvalue pack alias the elements
  auto &[a0, a1] = p;
  CHECK(a0 == 7);
  CHECK(a1 == 4.0);
  a0 = 9;
  CHECK(get<0>(p) == 9);

  // structured bindings over an rvalue pack move the elements out
  auto [b0, b1] = std::move(p);
  static_assert(std::same_as<decltype(b0), int>);
  static_assert(std::same_as<decltype(b1), double>);
  CHECK(b0 == 9);
  CHECK(b1 == 4.0);

  // generic idiom: unqualified get with std::get also in scope resolves to fn::get by ADL
  {
    using std::get;
    pack<int, double> q{3, 1.5};
    CHECK(get<0>(q) == 3);
    CHECK(get<1>(q) == 1.5);
  }

  // constexpr twins
  static_assert(std::tuple_size_v<pack<int, double>> == 2);
  static_assert([] {
    pack<int, double> q{5, 2.5};
    auto &[c0, c1] = q;
    c0 = 6;
    return get<0>(q) == 6 && get<1>(q) == 2.5;
  }());
  static_assert([] {
    int y = 3;
    pack<int &> s{y};
    get<0>(s) = 4;
    return y == 4;
  }());
  static_assert([] {
    int y = 5;
    pack<int &> s{y};
    // const-propagation twin: get on the const pack yields int const&, and
    // tuple_element agrees, so the binding is read-only yet still aliases y.
    static_assert(std::same_as<decltype(get<0>(std::as_const(s))), int const &>);
    static_assert(std::same_as<std::tuple_element_t<0, pack<int &> const>, int const &>);
    auto const &[e0] = std::as_const(s);
    return e0 == 5;
  }());
}
