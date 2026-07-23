// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_DETAIL_FWD
#define INCLUDE_FN_DETAIL_FWD

#include <libfn_version.hpp>

namespace fn {
inline namespace LIBFN_VERSION {
// NOTE Some forward declarations can lead to hard to troubleshoot compilation
//      errors. Only declare select, useful datatypes here.

// functors
struct and_then_t;
struct discard_t;
struct transform_t;
struct transform_error_t;
struct or_else_t;
struct recover_t;
struct fail_t;
struct filter_t;
struct inspect_t;

// expected monad (Either a | b)
template <typename T, typename Err> class expected;
namespace detail {
template <typename T> constexpr bool _is_some_expected = false;
template <typename T, typename Err> constexpr bool _is_some_expected<::fn::expected<T, Err> &> = true;
template <typename T, typename Err> constexpr bool _is_some_expected<::fn::expected<T, Err> const &> = true;
template <typename T>
concept _some_expected = _is_some_expected<T &>;
} // namespace detail

// optional monad (Maybe a)
template <typename T> class optional;
namespace detail {
template <typename T> constexpr bool _is_some_optional = false;
template <typename T> constexpr bool _is_some_optional<::fn::optional<T> &> = true;
template <typename T> constexpr bool _is_some_optional<::fn::optional<T> const &> = true;
template <typename T>
concept _some_optional = _is_some_optional<T &>;
} // namespace detail

// choice monad (Copack a | ...)
template <typename... Ts> struct choice;
namespace detail {
template <typename... Ts> constexpr bool _is_some_choice = false;
template <typename... Ts> constexpr bool _is_some_choice<::fn::choice<Ts...> &> = true;
template <typename... Ts> constexpr bool _is_some_choice<::fn::choice<Ts...> const &> = true;
template <typename T>
concept _some_choice = _is_some_choice<T &>;
} // namespace detail

// identity carrier (Just a)
template <typename T> struct just;
namespace detail {
template <typename T> constexpr bool _is_some_just = false;
template <typename T> constexpr bool _is_some_just<::fn::just<T> &> = true;
template <typename T> constexpr bool _is_some_just<::fn::just<T> const &> = true;
template <typename T>
concept _some_just = _is_some_just<T &>;
} // namespace detail

// product of types
template <typename... Ts> struct pack;
namespace detail {
template <typename... Ts> constexpr bool _is_some_pack = false;
template <typename... Ts> constexpr bool _is_some_pack<::fn::pack<Ts...> &> = true;
template <typename... Ts> constexpr bool _is_some_pack<::fn::pack<Ts...> const &> = true;
template <typename T>
concept _some_pack = detail::_is_some_pack<T &>;
} // namespace detail

// co-product of types
template <typename... Ts> struct copack;
namespace detail {
template <typename... Ts> constexpr bool _is_copack = false;
template <typename... Ts> constexpr bool _is_copack<::fn::copack<Ts...> &> = true;
template <typename... Ts> constexpr bool _is_copack<::fn::copack<Ts...> const &> = true;
template <typename T>
concept _some_copack = detail::_is_copack<T &>;
} // namespace detail
} // namespace LIBFN_VERSION
} // namespace fn

#endif // INCLUDE_FN_DETAIL_FWD
