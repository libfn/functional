// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_DETAIL_FWD
#define INCLUDE_FN_DETAIL_FWD

namespace fn {
// NOTE Some forward declarations can lead to hard to troubleshoot compilation
//      errors. Only declare select, useful datatypes here.

// functors
struct and_then_t;
struct transform_t;
struct transform_error_t;
struct or_else_t;
struct recover_t;
struct fail_t;
struct filter_t;
struct inspect_t;

// expected monad (Either a | b)
template <typename T, typename Err> struct expected;
namespace detail {
template <typename T> constexpr bool _is_some_expected = false;
template <typename T, typename Err> constexpr bool _is_some_expected<::fn::expected<T, Err> &> = true;
template <typename T, typename Err> constexpr bool _is_some_expected<::fn::expected<T, Err> const &> = true;
template <typename T>
concept _some_expected = _is_some_expected<T &>;
} // namespace detail

// optional monad (Maybe a)
template <typename T> struct optional;
namespace detail {
template <typename T> constexpr bool _is_some_optional = false;
template <typename T> constexpr bool _is_some_optional<::fn::optional<T> &> = true;
template <typename T> constexpr bool _is_some_optional<::fn::optional<T> const &> = true;
template <typename T>
concept _some_optional = _is_some_optional<T &>;
} // namespace detail

// choice monad (Sum a | ...)
template <typename... Ts> struct choice;
namespace detail {
template <typename... Ts> constexpr bool _is_some_choice = false;
template <typename... Ts> constexpr bool _is_some_choice<::fn::choice<Ts...> &> = true;
template <typename... Ts> constexpr bool _is_some_choice<::fn::choice<Ts...> const &> = true;
template <typename T>
concept _some_choice = _is_some_choice<T &>;
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
template <typename... Ts> struct sum;
namespace detail {
template <typename... Ts> constexpr bool _is_sum = false;
template <typename... Ts> constexpr bool _is_sum<::fn::sum<Ts...> &> = true;
template <typename... Ts> constexpr bool _is_sum<::fn::sum<Ts...> const &> = true;
template <typename T>
concept _some_sum = detail::_is_sum<T &>;
} // namespace detail
} // namespace fn

#endif // INCLUDE_FUNCTIONAL_DETAIL_FWD
