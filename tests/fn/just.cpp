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

// Referent sources whose binding goes through a conversion function: explicit and nothrow, or
// implicit and potentially throwing
struct ExplicitRef final {
  int x;
  constexpr explicit operator int &() noexcept { return x; }
};
struct ImplicitRef final {
  int x;
  bool fail = false;
  constexpr operator int &() // NOLINT(google-explicit-constructor)
  {
    if (fail)
      throw 0;
    return x;
  }
};

constexpr int five = 5;
template <fn::just<int const &> J> constexpr int nttp_value = J.value();

template <typename S, typename Fn>
concept can_and_then = requires(S s, Fn fn) { FWD(s).and_then(fn); };
template <typename S, typename Fn>
concept can_transform = requires(S s, Fn fn) { FWD(s).transform(fn); };
template <typename S, typename Fn, typename... Args>
concept can_apply = requires(S s, Fn fn, Args... args) { FWD(s).apply(fn, FWD(args)...); };
template <typename S, typename Fn, typename... Args>
concept can_apply_type = requires(S s, Fn fn, Args... args) { FWD(s).apply_type(fn, FWD(args)...); };
template <typename S, typename Ret, typename Fn>
concept can_apply_r = requires(S s, Fn fn) { FWD(s).template apply_r<Ret>(fn); };
template <typename S, typename Ret, typename Fn>
concept can_apply_type_r = requires(S s, Fn fn) { FWD(s).template apply_type_r<Ret>(fn); };
template <typename T>
concept implicitly_default_constructible = requires(void (&sink)(T)) { sink({}); };
template <typename T>
concept complete = requires { sizeof(T); };

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
    static_assert(std::is_same_v<decltype(fn::just{std::in_place}), fn::just<void>>);

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

  SECTION("copack payload")
  {
    // a copack payload selects the choice specialization, its alternatives dispatched branch-wise;
    // the empty copack alone is out, there being nothing to select - just<copack<>> is incomplete
    static_assert(fn::detail::_just_payload<fn::copack<int>>);
    static_assert(not fn::detail::_just_payload<fn::copack<>>);
    static_assert(fn::detail::_just_payload<int>);
    static_assert(std::is_same_v<fn::just<fn::copack<int>>, fn::choice<int>>);
    static_assert(fn::some_choice<fn::just<fn::copack<int>>> && fn::some_just<fn::choice<int>>);
    static_assert(not fn::some_choice<T> && not fn::some_choice<fn::just<void>>);
    static_assert(complete<fn::choice<int>>);
    static_assert(not complete<fn::just<fn::copack<>>>);
    static_assert(not complete<fn::choice<>>);
    // a choice is an atom, and just may box one
    static_assert(fn::detail::_just_payload<fn::choice<int>>);
    fn::just<fn::choice<int>> j{fn::choice<int>{1}};
    CHECK(j.value() == fn::choice<int>{1});
    static_assert(fn::just<fn::choice<int>>{fn::choice<int>{1}}.value() == fn::choice<int>{1});
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
    // an lvalue-reference result is a view of what the callback returned, here the payload
    auto r4 = a.transform([](int &i) -> int & { return i; });
    static_assert(std::is_same_v<decltype(r4), fn::just<int &>>);
    CHECK(&r4.value() == &a.value());
    // ... but not from an rvalue carrier, whose payload expires with it: the member stays
    // viable-but-loud and yields no just to compose
    constexpr auto fnView = [](int const &i) -> int const & { return i; };
    static_assert(can_transform<T &&, decltype(fnView)>);
    static_assert(not fn::some_just<decltype(std::move(a).transform(fnView))>);
    static_assert(not fn::some_just<decltype(std::move(std::as_const(a)).transform(fnView))>);
    static_assert(fn::some_just<decltype(std::as_const(a).transform(fnView))>);

    // a copack result lands on the choice over its alternatives - the same carrier family
    constexpr auto fnCopack
        = [](int i) { return i > 0 ? fn::copack_for<bool, int>{true} : fn::copack_for<bool, int>{i}; };
    static_assert(std::is_same_v<decltype(a.transform(fnCopack)), fn::choice_for<bool, int>>);
    CHECK(a.transform(fnCopack) == fn::choice<bool>{true});
    CHECK(T{-1}.transform(fnCopack) == fn::choice<int>{-1});
    // ... and the empty copack keeps the member viable-but-loud, like the family's other
    // inadmissible results - the body's static_assert names the requirement on use
    constexpr auto fnEmpty = [](int) -> fn::copack<> { throw 0; };
    static_assert(can_transform<T &, decltype(fnEmpty)>);

    // noexcept from the callback's applicability
    static_assert(noexcept(a.transform([](int) noexcept { return 1; })));
    static_assert(not noexcept(a.transform([](int) { return 1; })));

    SECTION("constexpr")
    {
      static_assert(T{3}.transform([](int i) { return i + 1; }) == fn::just<int>{4});
      static_assert(T{3}.transform([](int i) { return Immovable{i}; }).value().x == 3);
      static_assert(T{3}.transform(fnCopack) == fn::choice<bool>{true});
      static_assert([] {
        T t{3};
        auto v = t.transform([](int &i) -> int & { return i; });
        return &v.value() == &t.value();
      }());
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

    // the callback returns a just of any payload: a choice, or just<void>
    constexpr auto fnChoice
        = [](int i) { return i > 0 ? fn::choice_for<bool, int>{i} : fn::choice_for<bool, int>{false}; };
    static_assert(std::is_same_v<decltype(a.and_then(fnChoice)), fn::choice_for<bool, int>>);
    CHECK(a.and_then(fnChoice) == fn::as_choice(3));
    static_assert(std::is_same_v<decltype(a.and_then([](int) { return fn::just<void>{}; })), fn::just<void>>);

    SECTION("constexpr")
    {
      static_assert(T{3}.and_then([](int i) { return fn::just<bool>{i != 0}; }).value());
      static_assert(T{3}.and_then(fnChoice) == fn::as_choice(3));
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

    // A payload whose ADL reaches an expected must answer like any other: comparing the payloads
    // brings that expected's own comparisons into the candidate set, where one of them used to ask
    // a question that depended on itself - which is a hard error, not an answer.
    using F = fn::expected<int, bool> (*)(int);
    static_assert(fn::just<F>{nullptr} == fn::just<F>{nullptr});
    static_assert(not(fn::just<F>{nullptr} != fn::just<F>{nullptr}));
    SUCCEED();
  }

  SECTION("operator &")
  {
    // just & just folds the payloads and stays just; just<void> is the product's unit and elides
    static_assert(std::is_same_v<decltype(T{1} & T{2}), fn::just<fn::pack<int, int>>>);
    static_assert(std::is_same_v<decltype(T{1} & fn::just<void>{}), T>);
    static_assert(std::is_same_v<decltype(fn::just<void>{} & T{2}), T>);
    static_assert(std::is_same_v<decltype(fn::just<void>{} & fn::just<void>{}), fn::just<void>>);

    static_assert((T{1} & T{2}).value().apply([](int a, int b) { return a == 1 && b == 2; }));
    static_assert((fn::just<void>{} & T{7}).value() == 7);
    static_assert((T{7} & fn::just<void>{}).value() == 7);
    CHECK((T{1} & T{2}).value().apply([](int a, int b) { return a == 1 && b == 2; }));
    CHECK((fn::just<void>{} & T{7}).value() == 7);

    static_assert(noexcept(T{1} & T{2}));
    struct throwing_copy {
      throwing_copy() = default;
      // defined, not just declared: the instantiated fold references it
      throwing_copy(throwing_copy const &) noexcept(false) {}
    };
    static_assert(not noexcept(std::declval<fn::just<throwing_copy> &>() & std::declval<T &>())); // copies
  }
}

TEST_CASE("just of reference", "[just]")
{
  using R = fn::just<int &>;
  using C = fn::just<int const &>;

  SECTION("constructors and deduction")
  {
    int i = 13;
    R a{i};
    CHECK(&a.value() == &i);
    R b(std::in_place_type<int &>, i);
    CHECK(&b.value() == &i);
    C c{i};
    CHECK(&c.value() == &i);
    ExplicitRef e{7};
    CHECK(&R{e}.value() == &e.x);

    // Value deduction owns the payload; an explicit type tag can request a reference.
    static_assert(std::is_same_v<decltype(fn::just{i}), fn::just<int>>);
    static_assert(std::is_same_v<decltype(fn::just(std::in_place_type<int &>, i)), R>);

    // a reference is born bound
    static_assert(not std::is_default_constructible_v<R>);
    // Implicit construction follows implicit reference conversion.
    static_assert(std::is_convertible_v<int &, R>);
    static_assert(std::is_convertible_v<int &, C>);
    static_assert(std::is_convertible_v<ImplicitRef &, R>);
    static_assert(std::is_constructible_v<R, ExplicitRef &>);
    static_assert(not std::is_convertible_v<ExplicitRef &, R>);
    static_assert(std::is_constructible_v<R, std::in_place_type_t<int &>, int &>);
    static_assert(not std::is_convertible_v<std::in_place_type_t<int &>, R>);
    // noexcept follows the binding
    static_assert(std::is_nothrow_constructible_v<R, int &>);
    static_assert(std::is_nothrow_constructible_v<R, ExplicitRef &>);
    static_assert(not std::is_nothrow_constructible_v<R, ImplicitRef &>);
    // what int& cannot bind is refused, and another just is never unwrapped
    static_assert(not std::is_constructible_v<R, int>);
    static_assert(not std::is_constructible_v<R, int const &>);
    static_assert(not std::is_constructible_v<R, fn::just<int> &>);
    static_assert(not std::is_constructible_v<R, std::in_place_type_t<int>, int &>);

    SECTION("throwing conversion")
    {
      ImplicitRef good{3};
      CHECK(&R{good}.value() == &good.x);
      ImplicitRef bad{3, true};
      CHECK_THROWS_AS(R{bad}, int);
      CHECK_THROWS_AS(R(std::in_place_type<int &>, bad), int);
    }

    SECTION("constexpr")
    {
      static_assert([] {
        int x = 1;
        R r{x};
        R s(std::in_place_type<int &>, x);
        ExplicitRef e{7};
        R t{e};
        ImplicitRef g{3};
        R u{g};
        return &r.value() == &x && &s.value() == &x && &t.value() == &e.x && &u.value() == &g.x;
      }());
      SUCCEED();
    }
  }

  SECTION("special members")
  {
    static_assert(std::is_trivially_copyable_v<R>);
    static_assert(std::is_trivially_copy_constructible_v<R>);
    static_assert(std::is_trivially_move_constructible_v<R>);
    static_assert(std::is_trivially_copy_assignable_v<R>);
    static_assert(std::is_trivially_move_assignable_v<R>);
    static_assert(std::is_trivially_destructible_v<R>);
    static_assert(std::is_same_v<R::value_type, int> && std::is_same_v<C::value_type, int const>);
    // a structural type: a constant just<T&> is a template argument
    static_assert(nttp_value<C{five}> == 5);
    SUCCEED();
  }

  SECTION("referent")
  {
    // Scalar references may be cv-qualified; rvalue, array, function, and tag references are rejected.
    static_assert(fn::detail::_just_payload<int &>);
    static_assert(fn::detail::_just_payload<int const &>);
    static_assert(not fn::detail::_just_payload<int &&>);
    static_assert(not fn::detail::_just_payload<int (&)[2]>);
    static_assert(not fn::detail::_just_payload<void (&)()>);
    static_assert(not fn::detail::_just_payload<std::in_place_type_t<int> &>);
    // never a copack: dispatch would follow the referent's active alternative (issue #434)
    static_assert(not fn::detail::_just_payload<fn::copack<int> &>);
    static_assert(not fn::detail::_just_payload<fn::copack<int> const &>);
    static_assert(not fn::detail::_just_payload<fn::copack<> &>);
    // A borrowed choice is passed to the callable as a whole.
    static_assert(fn::detail::_just_payload<fn::choice<int> &>);
    fn::choice<int> ch{1};
    fn::just<fn::choice<int> &> j{ch};
    CHECK(j.transform([](fn::choice<int> &c) { return c == fn::choice<int>{1}; }).value());
    static_assert([] {
      fn::choice<int> c{1};
      return fn::just<fn::choice<int> &>{c}.transform([](fn::choice<int> &v) { return &v; }).value() == &c;
    }());
  }

  SECTION("assignment")
  {
    int x = 1;
    int y = 2;
    R a{x};
    // assignment rebinds, never assigning through: the previous referent is untouched
    a = R{y};
    CHECK(&a.value() == &y);
    CHECK(x == 1);
    a = x;
    CHECK(&a.value() == &x);
    CHECK(y == 2);
    // a throwing conversion leaves the binding as it was
    ImplicitRef bad{3, true};
    CHECK_THROWS_AS(a = bad, int);
    CHECK(&a.value() == &x);

    static_assert(std::is_nothrow_assignable_v<R &, int &>);
    static_assert(not std::is_nothrow_assignable_v<R &, ImplicitRef &>);
    static_assert(not std::is_assignable_v<R &, int>);
    static_assert(not std::is_assignable_v<R &, fn::just<int> &>);

    SECTION("constexpr")
    {
      static_assert([] {
        int x = 1;
        int y = 2;
        R r{x};
        r = R{y};
        bool const rebound = &r.value() == &y && x == 1;
        r = x;
        return rebound && &r.value() == &x && y == 2;
      }());
      SUCCEED();
    }
  }

  SECTION("emplace")
  {
    int x = 1;
    int y = 2;
    R a{x};
    // emplace rebinds too, and returns the new referent
    int &r = a.emplace(y);
    CHECK(&r == &y);
    CHECK(&a.value() == &y);
    CHECK(x == 1);
    ImplicitRef t{3};
    CHECK(&a.emplace(t) == &t.x);
    // a throwing conversion leaves the binding as it was
    ImplicitRef bad{4, true};
    CHECK_THROWS_AS(a.emplace(bad), int);
    CHECK(&a.value() == &t.x);

    static_assert(std::is_same_v<decltype(a.emplace(y)), int &>);
    static_assert(noexcept(a.emplace(y)));
    static_assert(not noexcept(a.emplace(t)));

    SECTION("constexpr")
    {
      static_assert([] {
        int x = 1;
        int y = 2;
        R r{x};
        ImplicitRef g{3};
        return &r.emplace(y) == &y && &r.value() == &y && x == 1 && &r.emplace(g) == &g.x;
      }());
      SUCCEED();
    }
  }

  SECTION("value")
  {
    int x = 1;
    R a{x};
    // Access returns an lvalue reference even through a const or rvalue carrier.
    static_assert(std::is_same_v<decltype(a.value()), int &>);
    static_assert(std::is_same_v<decltype(std::as_const(a).value()), int &>);
    static_assert(std::is_same_v<decltype(std::move(a).value()), int &>);
    static_assert(std::is_same_v<decltype(std::move(std::as_const(a)).value()), int &>);
    static_assert(std::is_same_v<decltype(std::declval<C &&>().value()), int const &>);
    static_assert(noexcept(a.value()));
    std::as_const(a).value() = 5;
    CHECK(x == 5);
    static_assert([] {
      int y = 1;
      R const r{y};
      r.value() = 5;
      return y == 5;
    }());
  }

  SECTION("transform")
  {
    int x = 3;
    R a{x};
    constexpr auto fnLvalue = [](int &i) { return i + 1; };

    SECTION("value category")
    {
      // the callable receives int& for every category of the carrier, never an rvalue
      CHECK(a.transform(fnLvalue).value() == 4);
      CHECK(std::as_const(a).transform(fnLvalue).value() == 4);
      CHECK(std::move(a).transform(fnLvalue).value() == 4);
      CHECK(std::move(std::as_const(a)).transform(fnLvalue).value() == 4);
      constexpr auto fnRvalue = [](int &&i) { return i; };
      static_assert(not can_transform<R &&, decltype(fnRvalue)>);
      static_assert(can_transform<fn::just<int> &&, decltype(fnRvalue)>);
    }

    SECTION("result")
    {
      // a reference result keeps a view, a value result owns, a void result is the unit
      auto r1 = a.transform([](int &i) -> int & { return i; });
      static_assert(std::is_same_v<decltype(r1), R>);
      CHECK(&r1.value() == &x);
      auto r2 = a.transform([](int const &i) -> int const & { return i; });
      static_assert(std::is_same_v<decltype(r2), C>);
      CHECK(&r2.value() == &x);
      auto r3 = a.transform([](int i) { return long{i}; });
      static_assert(std::is_same_v<decltype(r3), fn::just<long>>);
      CHECK(r3.value() == 3L);
      static_assert(std::is_same_v<decltype(a.transform([](int) {})), fn::just<void>>);
      // As with owning just, probing accepts this result, but calling transform triggers a static_assert.
      constexpr auto fnXvalue = [](int &i) -> int && { return std::move(i); };
      static_assert(can_transform<R &, decltype(fnXvalue)>);
    }

    SECTION("noexcept")
    {
      static_assert(noexcept(a.transform([](int &) noexcept { return 1; })));
      static_assert(not noexcept(a.transform([](int &) { return 1; })));
      SUCCEED();
    }

    SECTION("constexpr")
    {
      static_assert([] {
        int x = 3;
        R r{x};
        return &r.transform([](int &i) -> int & { return i; }).value() == &x
               && std::move(r).transform([](int &i) { return i + 1; }).value() == 4;
      }());
      SUCCEED();
    }
  }

  SECTION("and_then")
  {
    int x = 3;
    R a{x};
    // the callable receives int& for every category of the carrier and returns any just
    auto r1 = std::move(a).and_then([](int &i) { return R{i}; });
    static_assert(std::is_same_v<decltype(r1), R>);
    CHECK(&r1.value() == &x);
    auto r2 = std::as_const(a).and_then([](int &i) { return fn::just<bool>{i != 0}; });
    static_assert(std::is_same_v<decltype(r2), fn::just<bool>>);
    CHECK(r2.value());

    constexpr auto fnRvalue = [](int &&) { return fn::just<int>{1}; };
    static_assert(not can_and_then<R &&, decltype(fnRvalue)>);
    static_assert(can_and_then<fn::just<int> &&, decltype(fnRvalue)>);
    static_assert(noexcept(a.and_then([](int &) noexcept { return fn::just<int>{1}; })));
    static_assert(not noexcept(a.and_then([](int &) { return fn::just<int>{1}; })));

    SECTION("constexpr")
    {
      static_assert([] {
        int x = 3;
        return &R{x}.and_then([](int &i) { return R{i}; }).value() == &x;
      }());
      SUCCEED();
    }
  }

  SECTION("apply family")
  {
    int x = 3;
    R a{x};
    CHECK(std::move(a).apply([](int &i, int y) { return i + y; }, 4) == 7);
    CHECK(a.apply_r<long>([](int &i) { return i; }) == 3L);
    // the tag names the payload type, a reference
    CHECK(std::move(a).apply_type([](std::in_place_type_t<int &>, int &i) { return -i; }) == -3);
    CHECK(a.apply_type_r<long>([](std::in_place_type_t<int &>, int &i, int y) { return i + y; }, 1) == 4L);
    constexpr auto ownedTag = [](std::in_place_type_t<int>, int i) { return i; };
    static_assert(not can_apply_type<R &, decltype(ownedTag)>);
    static_assert(can_apply_type<fn::just<int> &, decltype(ownedTag)>);

    // a tuple-like referent goes by elements
    std::tuple<int, int> t{1, 2};
    fn::just<std::tuple<int, int> &> p{t};
    CHECK(p.apply([](int &l, int &r) { return l + r; }) == 3);

    // the result conversion is a constraint
    constexpr auto fnInt = [](int &i) { return i; };
    constexpr auto tagInt = [](std::in_place_type_t<int &>, int &i) { return i; };
    static_assert(can_apply_r<R &, long, decltype(fnInt)>);
    static_assert(not can_apply_r<R &, std::string, decltype(fnInt)>);
    static_assert(can_apply_type_r<R &, long, decltype(tagInt)>);
    static_assert(not can_apply_type_r<R &, std::string, decltype(tagInt)>);

    static_assert(noexcept(a.apply([](int &) noexcept { return 1; })));
    static_assert(not noexcept(a.apply([](int &) { return 1; })));
    static_assert(noexcept(a.apply_r<long>([](int &) noexcept { return 1; })));
    static_assert(not noexcept(a.apply_r<long>([](int &) { return 1; })));
    static_assert(noexcept(a.apply_type([](std::in_place_type_t<int &>, int &) noexcept { return 1; })));
    static_assert(not noexcept(a.apply_type([](std::in_place_type_t<int &>, int &) { return 1; })));
    static_assert(noexcept(a.apply_type_r<long>([](std::in_place_type_t<int &>, int &) noexcept { return 1; })));
    static_assert(not noexcept(a.apply_type_r<long>([](std::in_place_type_t<int &>, int &) { return 1; })));

    SECTION("constexpr")
    {
      static_assert([] {
        int x = 3;
        R r{x};
        return std::move(r).apply([](int &i, int y) { return i + y; }, 4) == 7
               && r.apply_r<long>([](int &i) { return i; }) == 3L
               && r.apply_type([](std::in_place_type_t<int &>, int &i) { return -i; }) == -3
               && r.apply_type_r<long>([](std::in_place_type_t<int &>, int &i, int y) { return i + y; }, 1) == 4L;
      }());
      static_assert([] {
        std::tuple<int, int> t{1, 2};
        return fn::just<std::tuple<int, int> &>{t}.apply([](int &l, int &r) { return l + r; }) == 3;
      }());
      SUCCEED();
    }
  }

  SECTION("equality")
  {
    // the referents compare, not the addresses
    int x = 1;
    int y = 1;
    int z = 2;
    CHECK(R{x} == R{y});
    CHECK(R{x} != R{z});
    CHECK(R{x} == fn::just<long>{1L});
    CHECK(R{x} == 1);
    static_assert([] {
      int x = 1;
      int y = 1;
      return R{x} == R{y} && R{x} == fn::just<long>{1L} && R{x} == 1 && R{x} != 2;
    }());
  }

  SECTION("operators")
  {
    // the product and the sum the operators build hold a copy of the referent
    int x = 1;
    R a{x};
    auto p = a & fn::just<double>{2.0};
    static_assert(std::is_same_v<decltype(p), fn::just<fn::pack<int, double>>>);
    auto d = a | fn::just<int>{7};
    static_assert(std::is_same_v<decltype(d), fn::just<int>>);
    // ... while eliding the unit just<void> returns the other operand itself, a reference included
    static_assert(std::is_same_v<decltype(fn::just<void>{} & a), R>);
    static_assert(std::is_same_v<decltype(a & fn::just<void>{}), R>);
    CHECK(&(fn::just<void>{} & a).value() == &x);
    CHECK(&(a & fn::just<void>{}).value() == &x);
    x = 5;
    CHECK(p.value().apply([](int i, double) { return i; }) == 1);
    CHECK(d.value() == 1);
    static_assert([] {
      int x = 1;
      auto p = R{x} & fn::just<double>{2.0};
      auto d = R{x} | fn::just<int>{7};
      x = 5;
      return p.value().apply([](int i, double) { return i; }) == 1 && d.value() == 1
             && &(fn::just<void>{} & R{x}).value() == &x && &(R{x} & fn::just<void>{}).value() == &x;
    }());
  }
}

TEST_CASE("just of void", "[just]")
{
  using V = fn::just<void>;

  static_assert(std::is_empty_v<V>);
  static_assert(std::is_trivially_copyable_v<V>);
  static_assert(std::is_trivially_default_constructible_v<V>);
  static_assert(std::is_same_v<V::value_type, void>);

  // implicit default construction, there being nothing to convert from; the in-place tags stay
  // explicit, following expected<void, E>
  static_assert(implicitly_default_constructible<V>);
  static_assert(std::is_constructible_v<V, std::in_place_t>);
  static_assert(not std::is_convertible_v<std::in_place_t, V>);
  static_assert(std::is_constructible_v<V, std::in_place_type_t<void>>);
  static_assert(not std::is_convertible_v<std::in_place_type_t<void>, V>);
  static_assert(V{std::in_place} == V{std::in_place_type<void>});

  constexpr V v{};
  v.value();
  static_assert(v == V{});
  static_assert(noexcept(v == V{}));

  SECTION("transform and and_then")
  {
    static_assert(v.transform([] { return 5; }) == fn::just<int>{5});
    static_assert(std::is_same_v<decltype(v.transform([] {})), V>);
    static_assert(v.transform([] { return fn::copack<int>{9}; }) == fn::choice<int>{9});
    static_assert(v.and_then([] { return fn::just<int>{9}; }) == fn::just<int>{9});
    static_assert(v.and_then([] { return fn::choice<int>{9}; }) == fn::as_choice(9));
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
