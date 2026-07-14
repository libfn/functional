// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/detail/variadic_union.hpp>
#include <fn/sum.hpp>

#include <catch2/catch_all.hpp>

#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

using fn::detail::invoke_type_variadic_union;
using fn::detail::invoke_variadic_union;
using fn::detail::make_variadic_union;
using fn::detail::ptr_variadic_union;
using fn::detail::variadic_union;

namespace {

struct NonCopyable final {
  int v;

  constexpr operator int() const { return v; }
  constexpr NonCopyable(int i) noexcept : v(i) {}
  NonCopyable(NonCopyable const &) = delete;
  NonCopyable &operator=(NonCopyable const &) = delete;
};

// Never an alternative of any union below, so every negative probe can name it.
struct Absent final {};

// Type-keyed rather than taking a value: the probe is used inside a constexpr function, whose locals
// are not constant expressions and so cannot be fed to a static_assert.
template <typename T, typename U>
concept can_ptr = requires(U &u) { ptr_variadic_union<T, U>(u); };

// Recovers the alternative pack from the union type, so a test can be written once and ranged over
// the unions themselves rather than over a parallel list of their contents.
template <typename U> struct alternatives;
template <typename... Ts> struct alternatives<variadic_union<Ts...>> {
  static constexpr std::size_t size = sizeof...(Ts);
  template <std::size_t I> using nth = fn::detail::select_nth_t<I, Ts...>;
};

// A distinct witness value per alternative type, so a round trip through the union can be checked
// for identity and not merely for compiling.
template <typename T> struct witness;
template <> struct witness<bool> {
  static constexpr bool value = true;
};
template <> struct witness<int> {
  static constexpr int value = 42;
};
template <> struct witness<double> {
  static constexpr double value = 0.5;
};
template <> struct witness<float> {
  static constexpr float value = 1.5f;
};
template <> struct witness<std::string_view> {
  static constexpr std::string_view value = "hello";
};

// The whole battery for ONE alternative of ONE union: construction, typed access, and both dispatch
// functions in their void and non-void flavours - which the header specializes separately per union
// size, so every arity must run all of it. constexpr so a single definition serves both the
// compile-time assertion and the runtime one (a static_assert alone would leave gcov holes).
template <typename U, std::size_t I> constexpr auto check_alternative() -> bool
{
  using T = typename alternatives<U>::template nth<I>;
  constexpr T v = witness<T>::value;

  static_assert(U::template has_type<T>);

  U const u = make_variadic_union<T, U>(v);
  U m = make_variadic_union<T, U>(v);

  // constness of the union propagates to the pointer, and an absent type is not accessible at all
  static_assert(std::same_as<decltype(ptr_variadic_union<T, U>(u)), T const *>);
  static_assert(std::same_as<decltype(ptr_variadic_union<T, U>(m)), T *>);
  static_assert(can_ptr<T, U>);
  static_assert(not can_ptr<Absent, U>);

  bool ok = *ptr_variadic_union<T, U>(u) == v && *ptr_variadic_union<T, U>(m) == v;

  // invoke passes the alternative alone; the result converts to whatever R asks for
  constexpr auto size_of = [](auto x) -> std::size_t { return sizeof(x); };
  static_assert(std::same_as<std::size_t, decltype(invoke_variadic_union<std::size_t, U>(u, I, size_of))>);
  static_assert(std::same_as<long, decltype(invoke_variadic_union<long, U>(u, I, size_of))>);
  ok = ok && invoke_variadic_union<std::size_t, U>(u, I, size_of) == sizeof(T);
  ok = ok && invoke_variadic_union<long, U>(u, I, size_of) == static_cast<long>(sizeof(T));

  // invoke_type additionally passes in_place_type<T>, naming the alternative it dispatched to - and
  // passes exactly those two arguments
  // [[maybe_unused]]: only the pack's arity is read, and MSVC /W4 flags each expanded parameter
  constexpr auto arity = []([[maybe_unused]] auto &&...args) -> std::size_t { return sizeof...(args); };
  ok = ok && invoke_type_variadic_union<std::size_t, U>(u, I, arity) == 2;
  ok = ok && invoke_type_variadic_union<bool, U>(u, I, []<typename X>(std::in_place_type_t<X>, auto) -> bool {
         return std::same_as<X, T>;
       });

  // both dispatchers have a separate void-returning overload per union size
  static_assert(std::same_as<void, decltype(invoke_variadic_union<void, U>(u, I, size_of))>);
  std::size_t seen = 0;
  invoke_variadic_union<void, U>(u, I, [&seen](auto x) { seen = sizeof(x); });
  ok = ok && seen == sizeof(T);

  static_assert(std::same_as<void, decltype(invoke_type_variadic_union<void, U>(u, I, arity))>);
  bool named = false;
  invoke_type_variadic_union<void, U>(u, I, [&named]<typename X>(std::in_place_type_t<X>, auto) { //
    named = std::same_as<X, T>;
  });
  ok = ok && named;

  return ok;
}

template <typename U> constexpr auto check_union() -> bool
{
  return []<std::size_t... Is>(std::index_sequence<Is...>) { //
    return (check_alternative<U, Is>() && ...);
  }(std::make_index_sequence<alternatives<U>::size>{});
}

// Named, because a template argument list carries commas and TEMPLATE_TEST_CASE is a macro: the
// commas inside variadic_union<bool, int> would be read as further macro arguments.
using U1 = variadic_union<bool>;
using U2 = variadic_union<bool, int>;
using U3 = variadic_union<bool, int, double>;
using U4 = variadic_union<bool, int, double, float>;
using U5 = variadic_union<bool, int, double, float, std::string_view>;

} // namespace

// One battery, run against every specialization the header defines: sizes one through four, then the
// recursive one that chains a nested union past the fourth alternative.
TEMPLATE_TEST_CASE("variadic_union",
                   "[variadic_union][make_variadic_union][ptr_variadic_union]"
                   "[invoke_variadic_union][invoke_type_variadic_union]",
                   U1, U2, U3, U4, U5)
{
  using U = TestType;

  static_assert(U::size == alternatives<U>::size);
  static_assert(not U::template has_type<Absent>);
  static_assert(not U::template has_type<std::string>);

  // The same battery twice over. Folded into one assertion at compile time; then run again per
  // alternative, both so that coverage sees the lines execute and so that a failure names the
  // alternative that broke rather than only the union it belongs to.
  static_assert(check_union<U>());

  []<std::size_t... Is>(std::index_sequence<Is...>) {
    auto const one = []<std::size_t I>(std::integral_constant<std::size_t, I>) {
      INFO("alternative " << I);
      CHECK(check_alternative<U, I>());
    };
    (one(std::integral_constant<std::size_t, Is>{}), ...);
  }(std::make_index_sequence<alternatives<U>::size>{});
}

TEST_CASE("variadic_union with a non-copyable alternative", "[variadic_union][make_variadic_union][ptr_variadic_union]")
{
  // The templated battery above copies its witness into the union, so a type that cannot be copied
  // needs its own case: it is constructed in place from the arguments instead.
  using T1 = variadic_union<NonCopyable const, int>;
  constexpr T1 a1 = make_variadic_union<NonCopyable const, T1>(7);
  static_assert(ptr_variadic_union<NonCopyable const, T1>(a1)->v == 7);
  static_assert(T1::has_type<NonCopyable const>);
  static_assert(not T1::has_type<Absent>);

#ifndef _MSC_VER
  // A const alternative can sit alongside a non-const one of the same underlying type, which the
  // union keeps distinct - except on MSVC, which deems the cv-twin arms' tagged constructors
  // redeclarations of one another (C2535), so the type does not compile there at all.
  using T2 = variadic_union<NonCopyable, NonCopyable const>;
  constexpr T2 a2 = make_variadic_union<NonCopyable, T2>(12);
  static_assert(ptr_variadic_union<NonCopyable, T2>(a2)->v == 12);
  constexpr T2 a3 = make_variadic_union<NonCopyable const, T2>(36);
  static_assert(ptr_variadic_union<NonCopyable const, T2>(a3)->v == 36);
  static_assert(T2::has_type<NonCopyable>);
  static_assert(T2::has_type<NonCopyable const>);
#endif

  // and in the recursive specialization, where it lands past the fourth alternative
  using T6 = variadic_union<int, bool, double, float, NonCopyable>;
  constexpr T6 a4 = make_variadic_union<NonCopyable, T6>(42);
  static_assert(ptr_variadic_union<NonCopyable, T6>(a4)->v == 42);
  // by const reference: a by-value callback would copy the alternative, which NonCopyable forbids
  static_assert(invoke_variadic_union<int, T6>(a4, 4, [](auto const &i) -> int { return static_cast<int>(i); }) == 42);

  CHECK(ptr_variadic_union<NonCopyable, T6>(a4)->v == 42);
}
