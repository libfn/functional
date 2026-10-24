// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <util/refusing.hpp>

#include <fn/just.hpp>

#include <utility>

// A guard named <subject>_to_<fallback> holds a call on a <subject> choice whose callback refuses
// the alternative in that category: the call must not compile, rather than run the <fallback> overload.

void transform()
{
  fn::choice<int> o{1};
  fn::choice<int> const c{1};

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
  using R = fn::just<int>;
  fn::choice<int> o{1};
  fn::choice<int> const c{1};

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

// A reference result must not refer into an alternative of a temporary choice.
void transform_reference_result()
{
  constexpr auto cref = [](int const &i) -> int const & { return i; };
  fn::choice<int> o{1};
  fn::choice<int> const c{1};

  (void)o.transform(cref);
#ifdef LIBFN_REJECT_transform_rvalue_reference_result
  (void)std::move(o).transform(cref);
#endif

  (void)c.transform(cref);
#ifdef LIBFN_REJECT_transform_crvalue_reference_result
  (void)std::move(c).transform(cref);
#endif
}

// Every branch must return a just: the member is the carrier's own bind.
void and_then_not_just()
{
  fn::choice<int> o{1};

  (void)o.and_then([](int) { return fn::just{0}; });
#ifdef LIBFN_REJECT_and_then_value_result
  (void)o.and_then([](int) { return 0; });
#endif
}
