// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/concepts.hpp"

#include <catch2/catch_all.hpp>

#include <expected>
#include <optional>

namespace fn {
static_assert(some_expected<std::expected<int, bool>>);
static_assert(some_expected<std::expected<int, bool> const>);
static_assert(some_expected<std::expected<int, bool> &>);
static_assert(some_expected<std::expected<int, bool> const &>);
static_assert(some_expected<std::expected<int, bool> &&>);
static_assert(some_expected<std::expected<int, bool> const &&>);

static_assert(some_optional<std::optional<int>>);
static_assert(some_optional<std::optional<int> const>);
static_assert(some_optional<std::optional<int> &>);
static_assert(some_optional<std::optional<int> const &>);
static_assert(some_optional<std::optional<int> &&>);
static_assert(some_optional<std::optional<int> const &&>);

static_assert(some_monadic_type<std::expected<int, bool>>);
static_assert(some_monadic_type<std::expected<int, bool> const>);
static_assert(some_monadic_type<std::expected<int, bool> &>);
static_assert(some_monadic_type<std::expected<int, bool> const &>);
static_assert(some_monadic_type<std::expected<int, bool> &&>);
static_assert(some_monadic_type<std::expected<int, bool> const &&>);
static_assert(some_monadic_type<std::optional<int>>);
static_assert(some_monadic_type<std::optional<int> const>);
static_assert(some_monadic_type<std::optional<int> &>);
static_assert(some_monadic_type<std::optional<int> const &>);
static_assert(some_monadic_type<std::optional<int> &&>);
static_assert(some_monadic_type<std::optional<int> const &&>);
} // namespace fn
