// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "refusing.hpp"

#include <pfn/optional.hpp>

#include <utility>

// A guard named <subject>_to_<fallback> holds a call on a <subject> optional whose callback refuses
// the payload in that category: the call must not compile, rather than run the <fallback> overload.

void transform()
{
  pfn::optional<int> o;
  pfn::optional<int> const c;

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

void and_then()
{
  using R = pfn::optional<int>;
  pfn::optional<int> o;
  pfn::optional<int> const c;

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
}

template <class O, class F>
concept can_and_then = requires(O o, F f) { o.and_then(f); };

// As in the draft, a non-invocable callback is a hard error even in a dependent requires-expression.
void reference_and_then()
{
  using O = pfn::optional<int &>;
  static_assert(can_and_then<O, refusing<pfn::optional<int>>>);
#ifdef LIBFN_REJECT_reference_and_then_not_invocable
  (void)can_and_then<O, int>;
#endif
}
