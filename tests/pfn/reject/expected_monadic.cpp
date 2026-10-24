// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <util/refusing.hpp>

#include <pfn/expected.hpp>

#include <utility>

// A guard named <subject>_to_<fallback> holds a call on a <subject> expected whose callback refuses
// the value or error in that category: the call must not compile, rather than run the <fallback>
// overload.

template <class O, class F>
concept can_and_then = requires(O o, F f) { o.and_then(f); };
template <class O, class F>
concept can_transform = requires(O o, F f) { o.transform(f); };
template <class O, class F>
concept can_or_else = requires(O o, F f) { o.or_else(f); };
template <class O, class F>
concept can_transform_error = requires(O o, F f) { o.transform_error(f); };

// As in the draft, a non-invocable callback is a hard error even in a dependent requires-expression.

void and_then()
{
  using O = pfn::expected<int, int>;
  using R = pfn::expected<long, int>;
  O o;
  O const c;

  (void)o.and_then(refusing<R>{});
#ifdef LIBFN_REJECT_and_then_lvalue_to_clvalue
  (void)o.and_then(refusing<R, int &>{});
#endif

  (void)std::move(o).and_then(refusing<R>{});
#ifdef LIBFN_REJECT_and_then_rvalue_to_crvalue
  (void)std::move(o).and_then(refusing<R, int &&>{});
#endif
#ifdef LIBFN_REJECT_and_then_rvalue_to_clvalue
  (void)std::move(o).and_then(refusing<R, int &&, int const &&>{});
#endif

  (void)std::move(c).and_then(refusing<R>{});
#ifdef LIBFN_REJECT_and_then_crvalue_to_clvalue
  (void)std::move(c).and_then(refusing<R, int const &&>{});
#endif

  static_assert(can_and_then<O, refusing<R>>);
#ifdef LIBFN_REJECT_and_then_not_invocable
  (void)can_and_then<O, int>;
#endif
}

void transform()
{
  using O = pfn::expected<int, int>;
  O o;
  O const c;

  (void)o.transform(refusing<int>{});
#ifdef LIBFN_REJECT_transform_lvalue_to_clvalue
  (void)o.transform(refusing<int, int &>{});
#endif

  (void)std::move(o).transform(refusing<int>{});
#ifdef LIBFN_REJECT_transform_rvalue_to_crvalue
  (void)std::move(o).transform(refusing<int, int &&>{});
#endif
#ifdef LIBFN_REJECT_transform_rvalue_to_clvalue
  (void)std::move(o).transform(refusing<int, int &&, int const &&>{});
#endif

  (void)std::move(c).transform(refusing<int>{});
#ifdef LIBFN_REJECT_transform_crvalue_to_clvalue
  (void)std::move(c).transform(refusing<int, int const &&>{});
#endif

  static_assert(can_transform<O, refusing<int>>);
#ifdef LIBFN_REJECT_transform_not_invocable
  (void)can_transform<O, int>;
#endif
}

void or_else()
{
  using O = pfn::expected<int, int>;
  using R = pfn::expected<int, long>;
  O o;
  O const c;

  (void)o.or_else(refusing<R>{});
#ifdef LIBFN_REJECT_or_else_lvalue_to_clvalue
  (void)o.or_else(refusing<R, int &>{});
#endif

  (void)std::move(o).or_else(refusing<R>{});
#ifdef LIBFN_REJECT_or_else_rvalue_to_crvalue
  (void)std::move(o).or_else(refusing<R, int &&>{});
#endif
#ifdef LIBFN_REJECT_or_else_rvalue_to_clvalue
  (void)std::move(o).or_else(refusing<R, int &&, int const &&>{});
#endif

  (void)std::move(c).or_else(refusing<R>{});
#ifdef LIBFN_REJECT_or_else_crvalue_to_clvalue
  (void)std::move(c).or_else(refusing<R, int const &&>{});
#endif

  static_assert(can_or_else<O, refusing<R>>);
#ifdef LIBFN_REJECT_or_else_not_invocable
  (void)can_or_else<O, int>;
#endif
}

void transform_error()
{
  using O = pfn::expected<int, int>;
  O o;
  O const c;

  (void)o.transform_error(refusing<int>{});
#ifdef LIBFN_REJECT_transform_error_lvalue_to_clvalue
  (void)o.transform_error(refusing<int, int &>{});
#endif

  (void)std::move(o).transform_error(refusing<int>{});
#ifdef LIBFN_REJECT_transform_error_rvalue_to_crvalue
  (void)std::move(o).transform_error(refusing<int, int &&>{});
#endif
#ifdef LIBFN_REJECT_transform_error_rvalue_to_clvalue
  (void)std::move(o).transform_error(refusing<int, int &&, int const &&>{});
#endif

  (void)std::move(c).transform_error(refusing<int>{});
#ifdef LIBFN_REJECT_transform_error_crvalue_to_clvalue
  (void)std::move(c).transform_error(refusing<int, int const &&>{});
#endif

  static_assert(can_transform_error<O, refusing<int>>);
#ifdef LIBFN_REJECT_transform_error_not_invocable
  (void)can_transform_error<O, int>;
#endif
}

void void_and_then()
{
  using O = pfn::expected<void, int>;
  using R = pfn::expected<int, int>;
  using F = decltype([] { return R{}; });

  static_assert(can_and_then<O, F>);
#ifdef LIBFN_REJECT_void_and_then_not_invocable
  (void)can_and_then<O, int>;
#endif
}

void void_transform()
{
  using O = pfn::expected<void, int>;
  using F = decltype([] { return 0; });

  static_assert(can_transform<O, F>);
#ifdef LIBFN_REJECT_void_transform_not_invocable
  (void)can_transform<O, int>;
#endif
}

void void_or_else()
{
  using O = pfn::expected<void, int>;
  using R = pfn::expected<void, long>;
  O o;
  O const c;

  (void)o.or_else(refusing<R>{});
#ifdef LIBFN_REJECT_void_or_else_lvalue_to_clvalue
  (void)o.or_else(refusing<R, int &>{});
#endif

  (void)std::move(o).or_else(refusing<R>{});
#ifdef LIBFN_REJECT_void_or_else_rvalue_to_crvalue
  (void)std::move(o).or_else(refusing<R, int &&>{});
#endif
#ifdef LIBFN_REJECT_void_or_else_rvalue_to_clvalue
  (void)std::move(o).or_else(refusing<R, int &&, int const &&>{});
#endif

  (void)std::move(c).or_else(refusing<R>{});
#ifdef LIBFN_REJECT_void_or_else_crvalue_to_clvalue
  (void)std::move(c).or_else(refusing<R, int const &&>{});
#endif

  static_assert(can_or_else<O, refusing<R>>);
#ifdef LIBFN_REJECT_void_or_else_not_invocable
  (void)can_or_else<O, int>;
#endif
}

void void_transform_error()
{
  using O = pfn::expected<void, int>;
  O o;
  O const c;

  (void)o.transform_error(refusing<int>{});
#ifdef LIBFN_REJECT_void_transform_error_lvalue_to_clvalue
  (void)o.transform_error(refusing<int, int &>{});
#endif

  (void)std::move(o).transform_error(refusing<int>{});
#ifdef LIBFN_REJECT_void_transform_error_rvalue_to_crvalue
  (void)std::move(o).transform_error(refusing<int, int &&>{});
#endif
#ifdef LIBFN_REJECT_void_transform_error_rvalue_to_clvalue
  (void)std::move(o).transform_error(refusing<int, int &&, int const &&>{});
#endif

  (void)std::move(c).transform_error(refusing<int>{});
#ifdef LIBFN_REJECT_void_transform_error_crvalue_to_clvalue
  (void)std::move(c).transform_error(refusing<int, int const &&>{});
#endif

  static_assert(can_transform_error<O, refusing<int>>);
#ifdef LIBFN_REJECT_void_transform_error_not_invocable
  (void)can_transform_error<O, int>;
#endif
}
