// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <util/allow_deprecated.hpp>

#include <fn/copack.hpp>
#include <fn/pack.hpp>

#include <utility>

ALLOW_DEPRECATED_BEGIN

namespace {
auto f(int i) -> int { return i; }
} // namespace

// A reference to a pack, an array or a function is no alternative, so the lift refuses such an
// lvalue rather than copy it.
void as_copack_lvalue_pack()
{
  fn::pack<int> p{1};

  (void)fn::as_copack(std::move(p));
#ifdef LIBFN_REJECT_as_copack_lvalue_pack
  (void)fn::as_copack(p);
#endif
}

void as_copack_lvalue_array()
{
  int x = 1;
  (void)fn::as_copack(x);
#ifdef LIBFN_REJECT_as_copack_lvalue_array
  int arr[2] = {1, 2};
  (void)fn::as_copack(arr);
#endif
}

void as_copack_lvalue_function()
{
  (void)fn::as_copack(&f);
#ifdef LIBFN_REJECT_as_copack_lvalue_function
  (void)fn::as_copack(f);
#endif
}

ALLOW_DEPRECATED_END
