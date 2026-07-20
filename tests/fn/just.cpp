// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/copack.hpp>
#include <fn/just.hpp>
#include <fn/utility.hpp>

#include <catch2/catch_all.hpp>

#include <string>
#include <tuple>
#include <utility>

#include <fn/detail/macro_begin.hpp>

namespace {

struct Immovable final {
  int x;
  constexpr explicit Immovable(int i) noexcept : x(i) {}
  Immovable(Immovable const &) = delete;
  Immovable(Immovable &&) = delete;
};

// Plain local types for exception specifications: every relevant construction potentially throwing
struct ThrowingCtor final {
  int x{};
  ThrowingCtor() = default;
  explicit ThrowingCtor(int i) : x(i)
  {
    if (i < 0)
      throw i;
  }
  ThrowingCtor(ThrowingCtor &&other) noexcept : x(other.x) {}
  ThrowingCtor &operator=(ThrowingCtor &&) = delete;
  bool operator==(ThrowingCtor const &) const = default;
};

template <typename S, typename Fn>
concept can_and_then = requires(S s, Fn fn) { FWD(s).and_then(fn); };
template <typename S, typename Fn>
concept can_transform = requires(S s, Fn fn) { FWD(s).transform(fn); };
template <typename S, typename Fn, typename... Args>
concept can_apply = requires(S s, Fn fn, Args... args) { FWD(s).apply(fn, FWD(args)...); };
template <typename S, typename Fn, typename... Args>
concept can_apply_type = requires(S s, Fn fn, Args... args) { FWD(s).apply_type(fn, FWD(args)...); };

} // namespace

TEST_CASE("just", "[just]")
{
  using T = fn::just<int>;

  SECTION("constructors and deduction")
  {
    static_assert(std::is_same_v<decltype(fn::just{13}), fn::just<int>>);
    static_assert(std::is_same_v<decltype(fn::just(std::in_place_type<std::string>, "foo")), fn::just<std::string>>);
    static_assert(std::is_same_v<decltype(fn::just{}), fn::just<void>>);
    static_assert(std::is_same_v<decltype(fn::just(std::in_place_type<void>)), fn::just<void>>);

    T a{13};
    CHECK(a.value() == 13);
    fn::just s(std::in_place_type<std::string>, "foo");
    CHECK(s.value() == "foo");
    // the defaulted constructor value-initializes through the member initializer
    CHECK(T{}.value() == 0);
    static_assert(T{}.value() == 0);

    // implicit only where the payload's construction is; in-place always explicit
    static_assert(std::is_convertible_v<int, T>);
    static_assert(not std::is_constructible_v<T, std::in_place_type_t<long>>); // the tag names T alone
    static_assert(not std::is_constructible_v<T, std::in_place_type_t<int>, int, int>);

    SECTION("constexpr")
    {
      constexpr T c{42};
      static_assert(c.value() == 42);
      static_assert(fn::just(std::in_place_type<int>, 7).value() == 7);
      SUCCEED();
    }
  }

  SECTION("special members follow the payload")
  {
    static_assert(std::is_trivially_copy_constructible_v<T>);
    static_assert(std::is_trivially_move_constructible_v<T>);
    static_assert(std::is_trivially_copy_assignable_v<T>);
    static_assert(std::is_trivially_move_assignable_v<T>);
    static_assert(std::is_trivially_destructible_v<T>);
    static_assert(std::is_trivially_copyable_v<T>);
    // the member initializer makes the defaulted default constructor non-trivial, never deleted
    static_assert(not std::is_trivially_default_constructible_v<T>);
    static_assert(std::is_default_constructible_v<T>);
    static_assert(std::is_nothrow_default_constructible_v<T>);

    using M = fn::just<Immovable>;
    static_assert(not std::is_copy_constructible_v<M>);
    static_assert(not std::is_move_constructible_v<M>);
    static_assert(std::is_constructible_v<M, std::in_place_type_t<Immovable>, int>);

    SUCCEED();
  }

  SECTION("assignment")
  {
    T a{13};
    a = 42;
    CHECK(a.value() == 42);
    // assigning the payload to itself is a no-op by address identity
    a = a.value();
    CHECK(a.value() == 42);
    a = std::move(a).value();
    CHECK(a.value() == 42);

    static_assert(std::is_assignable_v<T &, int>);
    static_assert(std::is_nothrow_assignable_v<T &, int>);
    static_assert(not std::is_assignable_v<T &, std::string>);

    SECTION("constexpr")
    {
      static_assert([] {
        T j{5};
        j = 7;
        return j.value() == 7;
      }());
      SUCCEED();
    }
  }

  SECTION("emplace")
  {
    fn::just<std::string> s(std::in_place_type<std::string>, "foo");
    s.emplace("bar");
    CHECK(s.value() == "bar");

    // the mutation path for a payload that does not support assignment
    fn::just<Immovable> m(std::in_place_type<Immovable>, 1);
    static_assert(not std::is_assignable_v<fn::just<Immovable> &, Immovable>);
    CHECK(m.emplace(2).x == 2);

    // strong guarantee: a throwing construction leaves the payload unchanged
    fn::just<ThrowingCtor> t(std::in_place_type<ThrowingCtor>, 5);
    CHECK_THROWS_AS(t.emplace(-1), int);
    CHECK(t.value().x == 5); // the throw was absorbed by the temporary
    CHECK(t.emplace(6).x == 6);
    static_assert(not noexcept(t.emplace(1)));
    fn::just<int> n{1};
    static_assert(noexcept(n.emplace(2)));

    SECTION("constexpr")
    {
      static_assert([] {
        fn::just<int> j{5};
        j.emplace(7);
        return j.value() == 7;
      }());
      SUCCEED();
    }
  }

  SECTION("value")
  {
    T a{13};
    static_assert(std::is_same_v<decltype(a.value()), int &>);
    static_assert(std::is_same_v<decltype(std::as_const(a).value()), int const &>);
    static_assert(std::is_same_v<decltype(std::move(a).value()), int &&>);
    static_assert(std::is_same_v<decltype(std::move(std::as_const(a)).value()), int const &&>);
    static_assert(noexcept(a.value()) && noexcept(std::as_const(a).value()) && noexcept(std::move(a).value()));
    CHECK(a.value() == 13);
  }

  SECTION("transform")
  {
    T a{3};
    auto r1 = a.transform([](int &i) { return i + 1; });
    static_assert(std::is_same_v<decltype(r1), fn::just<int>>);
    CHECK(r1.value() == 4);
    auto r2 = std::as_const(a).transform([](int const &i) { return long{i}; });
    static_assert(std::is_same_v<decltype(r2), fn::just<long>>);
    CHECK(r2.value() == 3L);
    auto r3 = std::move(a).transform([](int &&i) { return i * 2; });
    CHECK(r3.value() == 6);

    // a void result wraps as the void carrier; an immovable result is constructed in place
    static_assert(std::is_same_v<decltype(a.transform([](int) {})), fn::just<void>>);
    CHECK(a.transform([](int i) { return Immovable{i}; }).value().x == 3);

    // a copack result keeps the member viable-but-loud, like the family's members - the body's
    // static_assert names the requirement on use; the transform VERB promotes it to choice instead
    constexpr auto fnCopack = [](int) { return fn::copack<int>{1}; };
    static_assert(can_transform<T &, decltype(fnCopack)>);

    // noexcept from the callback's applicability
    static_assert(noexcept(a.transform([](int) noexcept { return 1; })));
    static_assert(not noexcept(a.transform([](int) { return 1; })));

    SECTION("constexpr")
    {
      static_assert(T{3}.transform([](int i) { return i + 1; }) == fn::just<int>{4});
      static_assert(T{3}.transform([](int i) { return Immovable{i}; }).value().x == 3);
      SUCCEED();
    }
  }

  SECTION("and_then")
  {
    T a{3};
    auto r1 = a.and_then([](int i) { return fn::just<bool>{i != 0}; });
    static_assert(std::is_same_v<decltype(r1), fn::just<bool>>);
    CHECK(r1.value());
    CHECK(std::move(a).and_then([](int &&i) { return fn::just<long>{i + 1}; }).value() == 4L);

    // an inapplicable callback drops and_then; a value-returning one stays viable - its rejection
    // is the deliberately loud static_assert on instantiation, not a constraint
    constexpr auto fnString = [](std::string const &) { return fn::just<int>{1}; };
    static_assert(not can_and_then<T &, decltype(fnString)>);
    static_assert(can_and_then<fn::just<std::string> &, decltype(fnString)>);
    constexpr auto fnValue = [](int i) { return i; };
    static_assert(can_and_then<T &, decltype(fnValue)>);

    static_assert(noexcept(a.and_then([](int) noexcept { return fn::just<int>{1}; })));
    static_assert(not noexcept(a.and_then([](int) { return fn::just<int>{1}; })));

    SECTION("constexpr")
    {
      static_assert(T{3}.and_then([](int i) { return fn::just<bool>{i != 0}; }).value());
      SUCCEED();
    }
  }

  SECTION("apply family")
  {
    T a{3};
    CHECK(a.apply([](int i) { return -i; }) == -3);
    CHECK(a.apply([](int i, int x) { return i + x; }, 4) == 7);
    CHECK(a.apply_r<long>([](int i) { return i; }) == 3L);
    CHECK(a.apply_type([](std::in_place_type_t<int>, int i) { return -i; }) == -3);
    CHECK(a.apply_type([](std::in_place_type_t<int>, int i, int x) { return i + x; }, 4) == 7);
    CHECK(a.apply_type_r<long>([](std::in_place_type_t<int>, int i) { return i; }) == 3L);

    // a tuple-like payload goes by elements, its elements form the tagged row's one signature
    fn::just<std::tuple<int, int>> p(std::in_place_type<std::tuple<int, int>>, 1, 2);
    CHECK(p.apply([](int x, int y) { return x + y; }) == 3);
    CHECK(p.apply_type([](std::in_place_type_t<std::tuple<int, int>>, int x, int y) { return x + y; }) == 3);

    // the tag never converts: an untagged arm does not serve apply_type
    constexpr auto untagged = [](int i) { return i; };
    static_assert(not can_apply_type<T &, decltype(untagged)>);
    static_assert(can_apply<T &, decltype(untagged)>);

    SECTION("constexpr")
    {
      static_assert(T{3}.apply([](int i, int x) { return i + x; }, 4) == 7);
      static_assert(T{3}.apply_type([](std::in_place_type_t<int>, int i) { return -i; }) == -3);
      SUCCEED();
    }
  }

  SECTION("equality")
  {
    static_assert(T{3} == T{3});
    static_assert(T{3} != T{4});
    static_assert(T{3} == fn::just<long>{3L});
    static_assert(T{3} == 3);
    static_assert(3 == T{3});
    static_assert(T{3} != 4);
    static_assert(noexcept(T{3} == T{3}));
    CHECK(T{3} == 3);
    SUCCEED();
  }
}

TEST_CASE("just of void", "[just]")
{
  using V = fn::just<void>;

  static_assert(std::is_empty_v<V>);
  static_assert(std::is_trivially_copyable_v<V>);
  static_assert(std::is_trivially_default_constructible_v<V>);
  static_assert(std::is_same_v<V::value_type, void>);

  constexpr V v{};
  v.value();
  static_assert(v == V{});
  static_assert(noexcept(v == V{}));

  SECTION("transform and and_then")
  {
    static_assert(v.transform([] { return 5; }) == fn::just<int>{5});
    static_assert(std::is_same_v<decltype(v.transform([] {})), V>);
    static_assert(v.and_then([] { return fn::just<int>{9}; }) == fn::just<int>{9});
    CHECK(v.transform([] { return 5; }).value() == 5);

    static_assert(noexcept(v.transform([]() noexcept { return 1; })));
    static_assert(not noexcept(v.transform([] { return 1; })));
    constexpr auto fnInt = [](int i) { return fn::just<int>{i}; };
    static_assert(not can_and_then<V const &, decltype(fnInt)>);
    static_assert(not can_transform<V const &, decltype(fnInt)>);
    SUCCEED();
  }

  SECTION("apply family")
  {
    CHECK(v.apply([] { return 1; }) == 1);
    CHECK(v.apply([](int x) { return x; }, 8) == 8);
    CHECK(v.apply_r<long>([] { return 1; }) == 1L);
    CHECK(v.apply_type([](std::in_place_type_t<void>, int x) { return x; }, 8) == 8);
    CHECK(v.apply_type_r<long>([](std::in_place_type_t<void>) { return 2; }) == 2L);

    static_assert(v.apply([] { return 1; }) == 1);
    static_assert(v.apply_type([](std::in_place_type_t<void>) { return 2; }) == 2);
  }
}
