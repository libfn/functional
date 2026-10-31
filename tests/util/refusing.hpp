// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_TESTS_UTIL_REFUSING
#define INCLUDE_TESTS_UTIL_REFUSING

#include <type_traits>

// Callback accepting its argument in every cv/ref category except the Refused ones, which select a
// deleted overload instead of falling back to another category.
template <class R, class... Refused> struct refusing {
  template <class T> constexpr R operator()(T &&) const { return R{}; }
  template <class T>
    requires(std::is_same_v<T &&, Refused> || ...)
  R operator()(T &&) const = delete;
};

static_assert(std::is_invocable_v<refusing<int>, int &>);
static_assert(std::is_invocable_v<refusing<int>, int const &>);
static_assert(std::is_invocable_v<refusing<int>, int &&>);
static_assert(std::is_invocable_v<refusing<int>, int const &&>);

static_assert(not std::is_invocable_v<refusing<int, int &>, int &>);
static_assert(std::is_invocable_v<refusing<int, int &>, int const &>);
static_assert(not std::is_invocable_v<refusing<int, int &&>, int &&>);
static_assert(std::is_invocable_v<refusing<int, int &&>, int const &&>);
static_assert(not std::is_invocable_v<refusing<int, int &&, int const &&>, int &&>);
static_assert(not std::is_invocable_v<refusing<int, int &&, int const &&>, int const &&>);
static_assert(std::is_invocable_v<refusing<int, int &&, int const &&>, int const &>);
static_assert(not std::is_invocable_v<refusing<int, int const &&>, int const &&>);
static_assert(std::is_invocable_v<refusing<int, int const &&>, int const &>);

#endif // INCLUDE_TESTS_UTIL_REFUSING
