// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <util/refusing.hpp>

#include <fn/optional.hpp>

#include <utility>

// A guard named <subject>_to_<fallback> holds a call on a <subject> optional whose callback refuses
// the value in that category: the call must not compile, rather than run the <fallback> overload.

void transform()
{
  fn::optional<int> o{1};
  fn::optional<int> const c{1};

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

// A copack value is handed to copack::transform in the optional's own category.
void transform_copack()
{
  fn::optional<fn::copack<int>> o{fn::copack<int>{1}};

  (void)std::move(o).transform(refusing<int>{});
#ifdef LIBFN_REJECT_transform_copack_rvalue_to_clvalue
  (void)std::move(o).transform(refusing<int, int &&, int const &&>{});
#endif
}

template <class S, class F>
concept can_transform = requires(S s, F f) { s.transform(f); };

// The member asserts what it requires: a non-invocable callback is a hard error even in a dependent
// requires-expression, and fn::applicable_transform answers instead.
void transform_not_invocable()
{
  static_assert(can_transform<fn::optional<int>, refusing<int>>);
#ifdef LIBFN_REJECT_transform_not_invocable
  (void)can_transform<fn::optional<int>, int>;
#endif
}
