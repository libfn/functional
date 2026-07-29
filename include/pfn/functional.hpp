// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_PFN_FUNCTIONAL
#define INCLUDE_PFN_FUNCTIONAL

#include <functional>
#include <libfn_version.hpp>
#include <type_traits>
#include <utility>

namespace pfn {
inline namespace LIBFN_VERSION_BASE {

/**
 * @brief Invokes a callable and implicitly converts its result to `R`: `std::invoke_r` as
 *        specified for C++23 ([func.invoke]), for C++20 compilers
 *
 * When `R` is `void` the result is discarded, so any invocable can be called for its effects
 * alone.
 *
 * @tparam R Type the result converts to; must be spelled explicitly
 * @param f Callable to invoke
 * @param args Arguments to pass
 * @return The callable's result, converted to `R`
 */
template <class R, class F, class... Args>
  requires ::std::is_invocable_r_v<R, F, Args...>
constexpr R invoke_r(F &&f, Args &&...args) noexcept(::std::is_nothrow_invocable_r_v<R, F, Args...>)
{
  if constexpr (::std::is_void_v<R>)
    static_cast<void>(::std::invoke(static_cast<F &&>(f), static_cast<Args &&>(args)...));
  else
    return ::std::invoke(static_cast<F &&>(f), static_cast<Args &&>(args)...);
}

} // namespace LIBFN_VERSION_BASE
} // namespace pfn

#endif // INCLUDE_PFN_FUNCTIONAL
