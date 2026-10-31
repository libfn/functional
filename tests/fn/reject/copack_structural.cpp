// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/copack.hpp>

template <fn::some_copack auto S> struct nttp final {};

void reference_alternative_template_argument()
{
  static int const s = 3;
  (void)nttp<fn::copack<int const &>{s}>{};
#ifdef LIBFN_REJECT_automatic_referent
  int const a = 3;
  (void)nttp<fn::copack<int const &>{a}>{};
#endif
}
