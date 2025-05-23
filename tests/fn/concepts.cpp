// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/concepts.hpp>

#include <catch2/catch_all.hpp>

namespace fn {
static_assert(some_expected<expected<int, bool>>);
static_assert(some_expected<expected<int, bool> const>);
static_assert(some_expected<expected<int, bool> &>);
static_assert(some_expected<expected<int, bool> const &>);
static_assert(some_expected<expected<int, bool> &&>);
static_assert(some_expected<expected<int, bool> const &&>);

static_assert(some_expected_void<expected<void, bool>>);
static_assert(some_expected_void<expected<void, bool> const>);
static_assert(some_expected_void<expected<void, bool> &>);
static_assert(some_expected_void<expected<void, bool> const &>);
static_assert(some_expected_void<expected<void, bool> &&>);
static_assert(some_expected_void<expected<void, bool> const &&>);
static_assert(not some_expected_void<expected<int, bool>>);
static_assert(not some_expected_void<expected<int, bool> const>);
static_assert(not some_expected_void<expected<int, bool> &>);
static_assert(not some_expected_void<expected<int, bool> const &>);
static_assert(not some_expected_void<expected<int, bool> &&>);
static_assert(not some_expected_void<expected<int, bool> const &&>);

static_assert(some_expected_non_void<expected<int, bool>>);
static_assert(some_expected_non_void<expected<int, bool> const>);
static_assert(some_expected_non_void<expected<int, bool> &>);
static_assert(some_expected_non_void<expected<int, bool> const &>);
static_assert(some_expected_non_void<expected<int, bool> &&>);
static_assert(some_expected_non_void<expected<int, bool> const &&>);
static_assert(not some_expected_non_void<expected<void, bool>>);
static_assert(not some_expected_non_void<expected<void, bool> const>);
static_assert(not some_expected_non_void<expected<void, bool> &>);
static_assert(not some_expected_non_void<expected<void, bool> const &>);
static_assert(not some_expected_non_void<expected<void, bool> &&>);
static_assert(not some_expected_non_void<expected<void, bool> const &&>);

namespace {
struct Error {};
struct Xerror final : Error {};
struct Value final {};
} // namespace

static_assert(some_optional<optional<int>>);
static_assert(some_optional<optional<int> const>);
static_assert(some_optional<optional<int> &>);
static_assert(some_optional<optional<int> const &>);
static_assert(some_optional<optional<int> &&>);
static_assert(some_optional<optional<int> const &&>);

static_assert(some_monadic_type<expected<int, bool>>);
static_assert(some_monadic_type<expected<int, bool> const>);
static_assert(some_monadic_type<expected<int, bool> &>);
static_assert(some_monadic_type<expected<int, bool> const &>);
static_assert(some_monadic_type<expected<int, bool> &&>);
static_assert(some_monadic_type<expected<int, bool> const &&>);
static_assert(some_monadic_type<optional<int>>);
static_assert(some_monadic_type<optional<int> const>);
static_assert(some_monadic_type<optional<int> &>);
static_assert(some_monadic_type<optional<int> const &>);
static_assert(some_monadic_type<optional<int> &&>);
static_assert(some_monadic_type<optional<int> const &&>);

// clang-format off
static_assert(same_kind<optional<bool>, optional<Value>>);
static_assert(not same_kind<optional<bool>, expected<void, bool>>);
static_assert(not same_kind<optional<int>, expected<int, Error>>);
static_assert(not same_kind<optional<Error>, expected<void, Error>>);
static_assert(same_kind<expected<Value, Error>, expected<void, Error>>);
static_assert(same_kind<expected<void, Error>, expected<void, Error>>);
static_assert(same_kind<expected<void, Error>, expected<int, Error>>);
static_assert(same_kind<expected<int, Error>, expected<void, Error>>);
static_assert(same_kind<expected<int, Error>, expected<Value, Error>>);
static_assert(same_kind<expected<void, Error>, expected<Value, Error>>);
static_assert(same_kind<expected<Value, fn::sum<Error>>, expected<void, fn::sum<Error>>>);
static_assert(same_kind<expected<Value, fn::sum<Error>>, expected<void, fn::sum<int>>>);
static_assert(same_kind<expected<Value, fn::sum<Error>>, expected<void, fn::sum<Error, int>>>);
static_assert(same_kind<expected<Value, fn::sum<Error, int>>, expected<void, fn::sum<Error>>>);
static_assert(same_kind<expected<Value, fn::sum<Error>>, expected<void, fn::sum<Xerror>>>);
static_assert(same_kind<expected<Value, fn::sum<Error>>, expected<void, fn::sum<Error, Xerror>>>);
static_assert(same_kind<expected<Value, fn::sum<int>>, expected<void, fn::sum<Error, Xerror>>>);
static_assert(not same_kind<expected<Value, Error>, expected<void, Xerror>>);
static_assert(not same_kind<expected<void, Error>, expected<void, Xerror>>);
static_assert(not same_kind<expected<void, Error>, expected<int, Xerror>>);
static_assert(not same_kind<expected<int, Error>, expected<void, Xerror>>);
static_assert(not same_kind<expected<int, Error>, expected<Value, Xerror>>);
static_assert(not same_kind<expected<void, Error>, expected<Value, Xerror>>);
static_assert(not same_kind<expected<Value, fn::sum<Error>>, expected<void, Error>>);
static_assert(not same_kind<expected<Value, fn::sum<Error>>, expected<void, Xerror>>);
static_assert(not same_kind<expected<Value, Error>, expected<void, fn::sum<Error>>>);
static_assert(not same_kind<expected<Value, Xerror>, expected<void, fn::sum<Error>>>);

static_assert(same_kind<optional<int>          , optional<Value>>);
static_assert(same_kind<optional<int>          , optional<Value> const>);
static_assert(same_kind<optional<int>          , optional<Value> &>);
static_assert(same_kind<optional<int>          , optional<Value> const &>);
static_assert(same_kind<optional<int>          , optional<Value> &&>);
static_assert(same_kind<optional<int>          , optional<Value> const &&>);
static_assert(same_kind<optional<int> const    , optional<Value>>);
static_assert(same_kind<optional<int> const    , optional<Value> const>);
static_assert(same_kind<optional<int> const    , optional<Value> &>);
static_assert(same_kind<optional<int> const    , optional<Value> const &>);
static_assert(same_kind<optional<int> const    , optional<Value> &&>);
static_assert(same_kind<optional<int> const    , optional<Value> const &&>);
static_assert(same_kind<optional<int> const    , optional<Value>>);
static_assert(same_kind<optional<int> &        , optional<Value>>);
static_assert(same_kind<optional<int> &        , optional<Value> const>);
static_assert(same_kind<optional<int> &        , optional<Value> &>);
static_assert(same_kind<optional<int> &        , optional<Value> const &>);
static_assert(same_kind<optional<int> &        , optional<Value> &&>);
static_assert(same_kind<optional<int> &        , optional<Value> const &&>);
static_assert(same_kind<optional<int> const &  , optional<Value>>);
static_assert(same_kind<optional<int> const &  , optional<Value> const>);
static_assert(same_kind<optional<int> const &  , optional<Value> &>);
static_assert(same_kind<optional<int> const &  , optional<Value> const &>);
static_assert(same_kind<optional<int> const &  , optional<Value> &&>);
static_assert(same_kind<optional<int> const &  , optional<Value> const &&>);
static_assert(same_kind<optional<int> const &  , optional<Value>>);
static_assert(same_kind<optional<int> &&       , optional<Value>>);
static_assert(same_kind<optional<int> &&       , optional<Value> const>);
static_assert(same_kind<optional<int> &&       , optional<Value> &>);
static_assert(same_kind<optional<int> &&       , optional<Value> const &>);
static_assert(same_kind<optional<int> &&       , optional<Value> &&>);
static_assert(same_kind<optional<int> &&       , optional<Value> const &&>);
static_assert(same_kind<optional<int> const && , optional<Value>>);
static_assert(same_kind<optional<int> const && , optional<Value> const>);
static_assert(same_kind<optional<int> const && , optional<Value> &>);
static_assert(same_kind<optional<int> const && , optional<Value> const &>);
static_assert(same_kind<optional<int> const && , optional<Value> &&>);
static_assert(same_kind<optional<int> const && , optional<Value> const &&>);
static_assert(same_kind<optional<int> const && , optional<Value>>);

static_assert(same_kind<expected<int, Error>          , expected<Value, Error>>);
static_assert(same_kind<expected<int, Error>          , expected<Value, Error> const>);
static_assert(same_kind<expected<int, Error>          , expected<Value, Error> &>);
static_assert(same_kind<expected<int, Error>          , expected<Value, Error> const &>);
static_assert(same_kind<expected<int, Error>          , expected<Value, Error> &&>);
static_assert(same_kind<expected<int, Error>          , expected<Value, Error> const &&>);
static_assert(same_kind<expected<int, Error> const    , expected<Value, Error>>);
static_assert(same_kind<expected<int, Error> const    , expected<Value, Error> const>);
static_assert(same_kind<expected<int, Error> const    , expected<Value, Error> &>);
static_assert(same_kind<expected<int, Error> const    , expected<Value, Error> const &>);
static_assert(same_kind<expected<int, Error> const    , expected<Value, Error> &&>);
static_assert(same_kind<expected<int, Error> const    , expected<Value, Error> const &&>);
static_assert(same_kind<expected<int, Error> &        , expected<Value, Error>>);
static_assert(same_kind<expected<int, Error> &        , expected<Value, Error> const>);
static_assert(same_kind<expected<int, Error> &        , expected<Value, Error> &>);
static_assert(same_kind<expected<int, Error> &        , expected<Value, Error> const &>);
static_assert(same_kind<expected<int, Error> &        , expected<Value, Error> &&>);
static_assert(same_kind<expected<int, Error> &        , expected<Value, Error> const &&>);
static_assert(same_kind<expected<int, Error> const &  , expected<Value, Error>>);
static_assert(same_kind<expected<int, Error> const &  , expected<Value, Error> const>);
static_assert(same_kind<expected<int, Error> const &  , expected<Value, Error> &>);
static_assert(same_kind<expected<int, Error> const &  , expected<Value, Error> const &>);
static_assert(same_kind<expected<int, Error> const &  , expected<Value, Error> &&>);
static_assert(same_kind<expected<int, Error> const &  , expected<Value, Error> const &&>);
static_assert(same_kind<expected<int, Error> &&       , expected<Value, Error>>);
static_assert(same_kind<expected<int, Error> &&       , expected<Value, Error> const>);
static_assert(same_kind<expected<int, Error> &&       , expected<Value, Error> &>);
static_assert(same_kind<expected<int, Error> &&       , expected<Value, Error> const &>);
static_assert(same_kind<expected<int, Error> &&       , expected<Value, Error> &&>);
static_assert(same_kind<expected<int, Error> &&       , expected<Value, Error> const &&>);
static_assert(same_kind<expected<int, Error> const && , expected<Value, Error>>);
static_assert(same_kind<expected<int, Error> const && , expected<Value, Error> const>);
static_assert(same_kind<expected<int, Error> const && , expected<Value, Error> &>);
static_assert(same_kind<expected<int, Error> const && , expected<Value, Error> const &>);
static_assert(same_kind<expected<int, Error> const && , expected<Value, Error> &&>);
static_assert(same_kind<expected<int, Error> const && , expected<Value, Error> const &&>);

static_assert(not same_kind<expected<int, Error>          , expected<int, Xerror>>);
static_assert(not same_kind<expected<int, Error>          , expected<int, Xerror> const>);
static_assert(not same_kind<expected<int, Error>          , expected<int, Xerror> &>);
static_assert(not same_kind<expected<int, Error>          , expected<int, Xerror> const &>);
static_assert(not same_kind<expected<int, Error>          , expected<int, Xerror> &&>);
static_assert(not same_kind<expected<int, Error>          , expected<int, Xerror> const &&>);
static_assert(not same_kind<expected<int, Error> const    , expected<int, Xerror>>);
static_assert(not same_kind<expected<int, Error> const    , expected<int, Xerror> const>);
static_assert(not same_kind<expected<int, Error> const    , expected<int, Xerror> &>);
static_assert(not same_kind<expected<int, Error> const    , expected<int, Xerror> const &>);
static_assert(not same_kind<expected<int, Error> const    , expected<int, Xerror> &&>);
static_assert(not same_kind<expected<int, Error> const    , expected<int, Xerror> const &&>);
static_assert(not same_kind<expected<int, Error> &        , expected<int, Xerror>>);
static_assert(not same_kind<expected<int, Error> &        , expected<int, Xerror> const>);
static_assert(not same_kind<expected<int, Error> &        , expected<int, Xerror> &>);
static_assert(not same_kind<expected<int, Error> &        , expected<int, Xerror> const &>);
static_assert(not same_kind<expected<int, Error> &        , expected<int, Xerror> &&>);
static_assert(not same_kind<expected<int, Error> &        , expected<int, Xerror> const &&>);
static_assert(not same_kind<expected<int, Error> const &  , expected<int, Xerror>>);
static_assert(not same_kind<expected<int, Error> const &  , expected<int, Xerror> const>);
static_assert(not same_kind<expected<int, Error> const &  , expected<int, Xerror> &>);
static_assert(not same_kind<expected<int, Error> const &  , expected<int, Xerror> const &>);
static_assert(not same_kind<expected<int, Error> const &  , expected<int, Xerror> &&>);
static_assert(not same_kind<expected<int, Error> const &  , expected<int, Xerror> const &&>);
static_assert(not same_kind<expected<int, Error> &&       , expected<int, Xerror>>);
static_assert(not same_kind<expected<int, Error> &&       , expected<int, Xerror> const>);
static_assert(not same_kind<expected<int, Error> &&       , expected<int, Xerror> &>);
static_assert(not same_kind<expected<int, Error> &&       , expected<int, Xerror> const &>);
static_assert(not same_kind<expected<int, Error> &&       , expected<int, Xerror> &&>);
static_assert(not same_kind<expected<int, Error> &&       , expected<int, Xerror> const &&>);
static_assert(not same_kind<expected<int, Error> const && , expected<int, Xerror>>);
static_assert(not same_kind<expected<int, Error> const && , expected<int, Xerror> const>);
static_assert(not same_kind<expected<int, Error> const && , expected<int, Xerror> &>);
static_assert(not same_kind<expected<int, Error> const && , expected<int, Xerror> const &>);
static_assert(not same_kind<expected<int, Error> const && , expected<int, Xerror> &&>);
static_assert(not same_kind<expected<int, Error> const && , expected<int, Xerror> const &&>);

static_assert(same_kind<expected<void, Error>          , expected<Value, Error>>);
static_assert(same_kind<expected<void, Error>          , expected<Value, Error> const>);
static_assert(same_kind<expected<void, Error>          , expected<Value, Error> &>);
static_assert(same_kind<expected<void, Error>          , expected<Value, Error> const &>);
static_assert(same_kind<expected<void, Error>          , expected<Value, Error> &&>);
static_assert(same_kind<expected<void, Error>          , expected<Value, Error> const &&>);
static_assert(same_kind<expected<void, Error> const    , expected<Value, Error>>);
static_assert(same_kind<expected<void, Error> const    , expected<Value, Error> const>);
static_assert(same_kind<expected<void, Error> const    , expected<Value, Error> &>);
static_assert(same_kind<expected<void, Error> const    , expected<Value, Error> const &>);
static_assert(same_kind<expected<void, Error> const    , expected<Value, Error> &&>);
static_assert(same_kind<expected<void, Error> const    , expected<Value, Error> const &&>);
static_assert(same_kind<expected<void, Error> &        , expected<Value, Error>>);
static_assert(same_kind<expected<void, Error> &        , expected<Value, Error> const>);
static_assert(same_kind<expected<void, Error> &        , expected<Value, Error> &>);
static_assert(same_kind<expected<void, Error> &        , expected<Value, Error> const &>);
static_assert(same_kind<expected<void, Error> &        , expected<Value, Error> &&>);
static_assert(same_kind<expected<void, Error> &        , expected<Value, Error> const &&>);
static_assert(same_kind<expected<void, Error> const &  , expected<Value, Error>>);
static_assert(same_kind<expected<void, Error> const &  , expected<Value, Error> const>);
static_assert(same_kind<expected<void, Error> const &  , expected<Value, Error> &>);
static_assert(same_kind<expected<void, Error> const &  , expected<Value, Error> const &>);
static_assert(same_kind<expected<void, Error> const &  , expected<Value, Error> &&>);
static_assert(same_kind<expected<void, Error> const &  , expected<Value, Error> const &&>);
static_assert(same_kind<expected<void, Error> &&       , expected<Value, Error>>);
static_assert(same_kind<expected<void, Error> &&       , expected<Value, Error> const>);
static_assert(same_kind<expected<void, Error> &&       , expected<Value, Error> &>);
static_assert(same_kind<expected<void, Error> &&       , expected<Value, Error> const &>);
static_assert(same_kind<expected<void, Error> &&       , expected<Value, Error> &&>);
static_assert(same_kind<expected<void, Error> &&       , expected<Value, Error> const &&>);
static_assert(same_kind<expected<void, Error> const && , expected<Value, Error>>);
static_assert(same_kind<expected<void, Error> const && , expected<Value, Error> const>);
static_assert(same_kind<expected<void, Error> const && , expected<Value, Error> &>);
static_assert(same_kind<expected<void, Error> const && , expected<Value, Error> const &>);
static_assert(same_kind<expected<void, Error> const && , expected<Value, Error> &&>);
static_assert(same_kind<expected<void, Error> const && , expected<Value, Error> const &&>);

static_assert(not same_kind<expected<void, Error>          , expected<void, Xerror>>);
static_assert(not same_kind<expected<void, Error>          , expected<void, Xerror> const>);
static_assert(not same_kind<expected<void, Error>          , expected<void, Xerror> &>);
static_assert(not same_kind<expected<void, Error>          , expected<void, Xerror> const &>);
static_assert(not same_kind<expected<void, Error>          , expected<void, Xerror> &&>);
static_assert(not same_kind<expected<void, Error>          , expected<void, Xerror> const &&>);
static_assert(not same_kind<expected<void, Error> const    , expected<void, Xerror>>);
static_assert(not same_kind<expected<void, Error> const    , expected<void, Xerror> const>);
static_assert(not same_kind<expected<void, Error> const    , expected<void, Xerror> &>);
static_assert(not same_kind<expected<void, Error> const    , expected<void, Xerror> const &>);
static_assert(not same_kind<expected<void, Error> const    , expected<void, Xerror> &&>);
static_assert(not same_kind<expected<void, Error> const    , expected<void, Xerror> const &&>);
static_assert(not same_kind<expected<void, Error> &        , expected<void, Xerror>>);
static_assert(not same_kind<expected<void, Error> &        , expected<void, Xerror> const>);
static_assert(not same_kind<expected<void, Error> &        , expected<void, Xerror> &>);
static_assert(not same_kind<expected<void, Error> &        , expected<void, Xerror> const &>);
static_assert(not same_kind<expected<void, Error> &        , expected<void, Xerror> &&>);
static_assert(not same_kind<expected<void, Error> &        , expected<void, Xerror> const &&>);
static_assert(not same_kind<expected<void, Error> const &  , expected<void, Xerror>>);
static_assert(not same_kind<expected<void, Error> const &  , expected<void, Xerror> const>);
static_assert(not same_kind<expected<void, Error> const &  , expected<void, Xerror> &>);
static_assert(not same_kind<expected<void, Error> const &  , expected<void, Xerror> const &>);
static_assert(not same_kind<expected<void, Error> const &  , expected<void, Xerror> &&>);
static_assert(not same_kind<expected<void, Error> const &  , expected<void, Xerror> const &&>);
static_assert(not same_kind<expected<void, Error> &&       , expected<void, Xerror>>);
static_assert(not same_kind<expected<void, Error> &&       , expected<void, Xerror> const>);
static_assert(not same_kind<expected<void, Error> &&       , expected<void, Xerror> &>);
static_assert(not same_kind<expected<void, Error> &&       , expected<void, Xerror> const &>);
static_assert(not same_kind<expected<void, Error> &&       , expected<void, Xerror> &&>);
static_assert(not same_kind<expected<void, Error> &&       , expected<void, Xerror> const &&>);
static_assert(not same_kind<expected<void, Error> const && , expected<void, Xerror>>);
static_assert(not same_kind<expected<void, Error> const && , expected<void, Xerror> const>);
static_assert(not same_kind<expected<void, Error> const && , expected<void, Xerror> &>);
static_assert(not same_kind<expected<void, Error> const && , expected<void, Xerror> const &>);
static_assert(not same_kind<expected<void, Error> const && , expected<void, Xerror> &&>);
static_assert(not same_kind<expected<void, Error> const && , expected<void, Xerror> const &&>);
// clang-format on
} // namespace fn

TEST_CASE("Dummy") { SUCCEED(); }
