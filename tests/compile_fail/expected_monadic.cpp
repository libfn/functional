// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <pfn/expected.hpp>

#include <type_traits>
#include <utility>

#if EXPECTED_MONADIC_VOID
using value_type = void;
#else
using value_type = int;
#endif
using subject_type = pfn::expected<value_type, int>;

#if EXPECTED_MONADIC_OP == 0
using result_type = pfn::expected<int, int>;
#define EXPECTED_MONADIC_CALL and_then
#elif EXPECTED_MONADIC_OP == 1
using result_type = int;
#define EXPECTED_MONADIC_CALL transform
#elif EXPECTED_MONADIC_OP == 2
using result_type = pfn::expected<value_type, int>;
#define EXPECTED_MONADIC_CALL or_else
#else
using result_type = int;
#define EXPECTED_MONADIC_CALL transform_error
#endif

#if EXPECTED_MONADIC_DIRECTION == 0

struct callback {
#if EXPECTED_MONADIC_POSITIVE
#if EXPECTED_MONADIC_VOID && EXPECTED_MONADIC_OP <= 1
  result_type operator()() const { return {}; }
#else
  result_type operator()(int &) const { return {}; }
#endif
#endif
};

#if EXPECTED_MONADIC_VOID && EXPECTED_MONADIC_OP <= 1
static_assert(std::is_invocable_v<callback> == bool(EXPECTED_MONADIC_POSITIVE));
#else
static_assert(std::is_invocable_v<callback, int &> == bool(EXPECTED_MONADIC_POSITIVE));
static_assert(!std::is_invocable_v<callback, int const &>);
static_assert(!std::is_invocable_v<callback, int &&>);
static_assert(!std::is_invocable_v<callback, int const &&>);
#endif

#if !EXPECTED_MONADIC_FIXTURE_ONLY
template <class F>
concept can_call = requires(subject_type value, F f) { value.EXPECTED_MONADIC_CALL(f); };

// Invalid invocation is a hard error even inside a dependent requires-expression.
static_assert(can_call<callback> == bool(EXPECTED_MONADIC_POSITIVE));
#endif

#else

struct callback {
#if EXPECTED_MONADIC_DIRECTION == 1 && !EXPECTED_MONADIC_POSITIVE
  result_type operator()(int &) const = delete;
#else
  result_type operator()(int &) const { return {}; }
#endif
  result_type operator()(int const &) const { return {}; }
#if (EXPECTED_MONADIC_DIRECTION == 2 || EXPECTED_MONADIC_DIRECTION == 3) && !EXPECTED_MONADIC_POSITIVE
  result_type operator()(int &&) const = delete;
#else
  result_type operator()(int &&) const { return {}; }
#endif
#if (EXPECTED_MONADIC_DIRECTION == 3 || EXPECTED_MONADIC_DIRECTION == 4) && !EXPECTED_MONADIC_POSITIVE
  result_type operator()(int const &&) const = delete;
#else
  result_type operator()(int const &&) const { return {}; }
#endif
};

#if EXPECTED_MONADIC_DIRECTION == 1
using selected_arg = int &;
using fallback_arg = int const &;
#elif EXPECTED_MONADIC_DIRECTION == 2
using selected_arg = int &&;
using fallback_arg = int const &&;
#elif EXPECTED_MONADIC_DIRECTION == 3
using selected_arg = int &&;
using fallback_arg = int const &;
#else
using selected_arg = int const &&;
using fallback_arg = int const &;
#endif

static_assert(std::is_invocable_v<callback, fallback_arg>);
static_assert(std::is_invocable_v<callback, selected_arg> == bool(EXPECTED_MONADIC_POSITIVE));
#if EXPECTED_MONADIC_DIRECTION == 3
static_assert(std::is_invocable_v<callback, int const &&> == bool(EXPECTED_MONADIC_POSITIVE));
#endif

#if !EXPECTED_MONADIC_FIXTURE_ONLY
// The selected overload must reject the call instead of falling back to another cv/ref category.
void probe_expected_monadic()
{
#if EXPECTED_MONADIC_DIRECTION == 1
  subject_type value;
  (void)value.EXPECTED_MONADIC_CALL(callback{});
#elif EXPECTED_MONADIC_DIRECTION == 4
  subject_type const value;
  (void)std::move(value).EXPECTED_MONADIC_CALL(callback{});
#else
  subject_type value;
  (void)std::move(value).EXPECTED_MONADIC_CALL(callback{});
#endif
}
#endif

#endif
