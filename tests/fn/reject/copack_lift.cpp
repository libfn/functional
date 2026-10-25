// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <util/allow_deprecated.hpp>

#include <fn/copack.hpp>
#include <fn/pack.hpp>

#include <utility>

ALLOW_DEPRECATED_BEGIN

// A reference to a pack is no alternative, so the lift refuses an lvalue pack rather than copy it.
void as_copack_lvalue_pack()
{
  fn::pack<int> p{1};

  (void)fn::as_copack(std::move(p));
#ifdef LIBFN_REJECT_as_copack_lvalue_pack
  (void)fn::as_copack(p);
#endif
}

ALLOW_DEPRECATED_END
