// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <pfn/optional.hpp>

#include <type_traits>
#include <utility>

#if OPTIONAL_MONADIC_CASE == 9

struct callback {
#if OPTIONAL_MONADIC_POSITIVE
  pfn::optional<int> operator()(int &) const { return {}; }
#endif
};

static_assert(std::is_invocable_v<callback, int &> == bool(OPTIONAL_MONADIC_POSITIVE));

#if !OPTIONAL_MONADIC_FIXTURE_ONLY
template <class F>
concept can_bind = requires(pfn::optional<int &> value, F f) { value.and_then(f); };

#if OPTIONAL_MONADIC_POSITIVE
static_assert(can_bind<callback>);
#else
// Invalid invocation is a hard error even inside a dependent requires-expression.
static_assert(!can_bind<callback>);
#endif
#endif

#else

#if OPTIONAL_MONADIC_CASE <= 4
using result_type = int;
#define OPTIONAL_MONADIC_CALL transform
#else
using result_type = pfn::optional<int>;
#define OPTIONAL_MONADIC_CALL and_then
#endif

#define OPTIONAL_MONADIC_DIRECTION ((OPTIONAL_MONADIC_CASE - 1) % 4 + 1)

struct callback {
#if OPTIONAL_MONADIC_DIRECTION == 1 && !OPTIONAL_MONADIC_POSITIVE
  result_type operator()(int &) const = delete;
#else
  result_type operator()(int &) const { return {}; }
#endif
  result_type operator()(int const &) const { return {}; }
#if (OPTIONAL_MONADIC_DIRECTION == 2 || OPTIONAL_MONADIC_DIRECTION == 3) && !OPTIONAL_MONADIC_POSITIVE
  result_type operator()(int &&) const = delete;
#else
  result_type operator()(int &&) const { return {}; }
#endif
#if (OPTIONAL_MONADIC_DIRECTION == 3 || OPTIONAL_MONADIC_DIRECTION == 4) && !OPTIONAL_MONADIC_POSITIVE
  result_type operator()(int const &&) const = delete;
#else
  result_type operator()(int const &&) const { return {}; }
#endif
};

#if OPTIONAL_MONADIC_DIRECTION == 1
using selected_arg = int &;
using fallback_arg = int const &;
#elif OPTIONAL_MONADIC_DIRECTION == 2
using selected_arg = int &&;
using fallback_arg = int const &&;
#elif OPTIONAL_MONADIC_DIRECTION == 3
using selected_arg = int &&;
using fallback_arg = int const &;
#else
using selected_arg = int const &&;
using fallback_arg = int const &;
#endif

static_assert(std::is_invocable_v<callback, fallback_arg>);
static_assert(std::is_invocable_v<callback, selected_arg> == bool(OPTIONAL_MONADIC_POSITIVE));
#if OPTIONAL_MONADIC_DIRECTION == 3
static_assert(std::is_invocable_v<callback, int const &&> == bool(OPTIONAL_MONADIC_POSITIVE));
#endif

#if !OPTIONAL_MONADIC_FIXTURE_ONLY
// The selected overload must reject the call instead of falling back to another cv/ref category.
void probe_optional_monadic()
{
#if OPTIONAL_MONADIC_DIRECTION == 1
  pfn::optional<int> value;
  (void)value.OPTIONAL_MONADIC_CALL(callback{});
#elif OPTIONAL_MONADIC_DIRECTION == 4
  pfn::optional<int> const value;
  (void)std::move(value).OPTIONAL_MONADIC_CALL(callback{});
#else
  pfn::optional<int> value;
  (void)std::move(value).OPTIONAL_MONADIC_CALL(callback{});
#endif
}
#endif

#endif
