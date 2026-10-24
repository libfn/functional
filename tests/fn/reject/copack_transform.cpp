// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <util/refusing.hpp>

#include <fn/copack.hpp>

#include <utility>

// A guard named <subject>_to_<fallback> holds a call on a <subject> copack whose callback refuses
// the alternative in that category: the call must not compile, rather than run the <fallback> overload.

void transform()
{
  fn::copack<int> o{1};
  fn::copack<int> const c{1};

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
}

// A reference result must not refer into an alternative of a temporary copack.
void transform_reference_result()
{
  constexpr auto cref = [](int const &i) -> int const & { return i; };
  fn::copack<int> o{1};
  fn::copack<int> const c{1};

  (void)o.transform(cref);
#ifdef LIBFN_REJECT_transform_rvalue_reference_result
  (void)std::move(o).transform(cref);
#endif

  (void)c.transform(cref);
#ifdef LIBFN_REJECT_transform_crvalue_reference_result
  (void)std::move(c).transform(cref);
#endif
}

// A result no copack can hold.
void transform_unholdable_result()
{
  fn::copack<int> o{1};

  (void)o.transform([](int) { return 0; });
#ifdef LIBFN_REJECT_transform_unholdable_result
  (void)o.transform([](int) { return std::in_place_type<int>; });
#endif
}

template <class S, class F>
concept can_transform = requires(S s, F f) { s.transform(f); };

// The member asserts what it requires: a non-invocable callback is a hard error even in a dependent
// requires-expression, and fn::typelist_applicable answers instead.
void transform_not_invocable()
{
  static_assert(can_transform<fn::copack<int>, refusing<int>>);
#ifdef LIBFN_REJECT_transform_not_invocable
  (void)can_transform<fn::copack<int>, int>;
#endif
}
