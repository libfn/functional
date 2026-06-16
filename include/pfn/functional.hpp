// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_PFN_FUNCTIONAL
#define INCLUDE_PFN_FUNCTIONAL

#include <functional>
#include <type_traits>
#include <utility>

namespace pfn {

template <class R, class F, class... Args>
  requires ::std::is_invocable_r_v<R, F, Args...>
constexpr R invoke_r(F &&f, Args &&...args) noexcept(::std::is_nothrow_invocable_r_v<R, F, Args...>)
{
  if constexpr (::std::is_void_v<R>)
    static_cast<void>(::std::invoke(static_cast<F &&>(f), static_cast<Args &&>(args)...));
  else
    return ::std::invoke(static_cast<F &&>(f), static_cast<Args &&>(args)...);
}

} // namespace pfn

#endif // INCLUDE_PFN_FUNCTIONAL
