// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "util/static_check.hpp"

#include <fn/functor.hpp>

#include <catch2/catch_all.hpp>

#include <type_traits>
#include <utility>

using namespace util;

namespace {
constexpr inline struct dummy_t final {
  auto operator()(auto &&fn) const noexcept -> fn::functor<dummy_t, decltype(fn)> { return {FWD(fn)}; }

  struct apply final {
    auto operator()(fn::some_monadic_type auto &&v, auto &&fn) const noexcept -> decltype(auto)
      requires requires { fn(v.value()); }
    {
      return FWD(v).transform([&fn](auto &&v) noexcept { return FWD(fn)(FWD(v)); });
    }
  };
} dummy = {};

// The same verb, but honestly potentially-throwing. Piping it isolates the machinery's own promise
// from the verb's: whatever operator| says about this one, the verb did not say it.
constexpr inline struct throwing_dummy_t final {
  auto operator()(auto &&fn) const noexcept -> fn::functor<throwing_dummy_t, decltype(fn)> { return {FWD(fn)}; }

  struct apply final {
    auto operator()(fn::some_monadic_type auto &&v, auto &&fn) const noexcept(false) -> decltype(auto)
      requires requires { fn(v.value()); }
    {
      return FWD(v).transform([&fn](auto &&v) { return FWD(fn)(FWD(v)); });
    }
  };
} throwing_dummy = {};

constexpr auto fn1 = [](int i) constexpr -> int { return i + 1; };
constexpr auto fn2 = []() constexpr -> int { return 1; };

namespace check_expected {
using operand_t = fn::expected<int, bool>;
using is = monadic_static_check<dummy_t, operand_t>;

static_assert(is::invocable_with_any(fn1));
static_assert(is::not_invocable_with_any(fn2)); // arity mismatch
} // namespace check_expected

namespace check_optional {
using operand_t = fn::optional<int>;
using is = monadic_static_check<dummy_t, operand_t>;

static_assert(is::invocable_with_any(fn1));
static_assert(is::not_invocable_with_any(fn2)); // arity mismatch
} // namespace check_optional
} // namespace

TEST_CASE("user-defined monadic operation", "[functor]")
{
  CHECK((fn::expected<int, std::runtime_error>{12} | dummy(fn1)).value() == 13);
  CHECK((fn::optional{42} | dummy(fn1)).value() == 43);

  CHECK((fn::expected<int, std::runtime_error>{12} | throwing_dummy(fn1)).value() == 13);
  CHECK((fn::optional{42} | throwing_dummy(fn1)).value() == 43);

  WHEN("noexcept")
  {
    using O = fn::optional<int>;

    // GAP #285: the pipeline is unconditionally noexcept at every step - functor::operator| and the
    // pack's _swap_invoke it dispatches through - and imposes that on whatever verb is piped into
    // it. throwing_dummy's apply says plainly that it may throw, and operator| overrides it: an
    // exception escaping the verb would cross the noexcept boundary and terminate. This is the
    // machinery's promise, not any one verb's, which is why it is asserted here rather than in each
    // verb's own tests.
    static_assert(not noexcept(throwing_dummy_t::apply{}(std::declval<O &>(), fn1)));
    static_assert(noexcept(std::declval<O &>() | throwing_dummy(fn1)));
    static_assert(noexcept(std::declval<O &&>() | throwing_dummy(fn1)));

    // Building the functor copies the callable into its pack, and that copy can throw as well -
    // yet the nielbloid's operator() is noexcept too.
    struct ThrowingCopy final {
      ThrowingCopy() = default;
      ThrowingCopy(ThrowingCopy const &) noexcept(false) {}
      ThrowingCopy(ThrowingCopy &&) noexcept(false) {}
      auto operator()(int i) const noexcept -> int { return i + 1; }
    };
    static_assert(not std::is_nothrow_copy_constructible_v<ThrowingCopy>);
    static_assert(noexcept(dummy(std::declval<ThrowingCopy const &>())));
    static_assert(noexcept(std::declval<O &>() | dummy(std::declval<ThrowingCopy const &>())));

    SUCCEED();
  }
}
