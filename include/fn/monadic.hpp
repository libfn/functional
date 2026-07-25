// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_MONADIC
#define INCLUDE_FN_MONADIC

#include <fn/detail/monadic.hpp>
#include <libfn_version.hpp>

namespace fn {
inline namespace LIBFN_VERSION {

/**
 * @brief Checks if a type is one of the library's carriers - what a pipeline flows through
 *
 * The four kinds a verb may be applied to: `expected`, `optional`, `choice` and `just`. A `pack` or
 * a `copack` is data a carrier holds, not a carrier, and answers false.
 *
 * @tparam T Type to check, possibly cv-ref qualified
 */
template <typename T>
concept some_monadic_type = detail::_some_monadic_type<T>;

/**
 * @brief Checks if a verb applies to a carrier - the constraint `operator|` itself carries
 *
 * The question asked of the verb's own `apply` object, so every arm it offers counts. This is what
 * a pipeline asks, and it differs from the `applicable_` concept of an individual verb, which asks
 * whether the callback is used to serve the operand: an operation over an uninhabited side consults
 * no callback at all, and so applies while no callback is applicable to it.
 *
 * @tparam Functor The verb, such as `fn::transform_t`
 * @tparam V The carrier, possibly cv-ref qualified
 * @tparam Args The verb's arguments as its functor holds them, typically the callback
 */
template <typename Functor, typename V, typename... Args>
concept monadic_invocable = detail::_monadic_invocable<Functor, V, Args...>;

} // namespace LIBFN_VERSION
} // namespace fn

#endif // INCLUDE_FN_MONADIC
