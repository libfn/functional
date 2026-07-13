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
// sum_for, not sum<Error,int>: the alternative order is platform-specific (see sum.cpp) — same_kind ignores it.
static_assert(same_kind<expected<Value, fn::sum<Error>>, expected<void, fn::sum_for<Error, int>>>);
static_assert(same_kind<expected<Value, fn::sum_for<Error, int>>, expected<void, fn::sum<Error>>>);
static_assert(same_kind<expected<Value, fn::sum<Error>>, expected<void, fn::sum<Xerror>>>);
static_assert(same_kind<expected<Value, fn::sum<Error>>, expected<void, fn::sum_for<Error, Xerror>>>);
static_assert(same_kind<expected<Value, fn::sum<int>>, expected<void, fn::sum_for<Error, Xerror>>>);
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

// void can never convert to any of these - and each concept must answer so, rather than
// instantiate unexpected<void>, optional<void> or choice<void>, whose validity mandates are hard
// errors. Contrast expected, where a void value is legitimate and carries its own arm.
static_assert(not convertible_to_unexpected<void>);
static_assert(not convertible_to_optional<void>);
static_assert(not convertible_to_choice<void>);
static_assert(convertible_to_unexpected<int>);
static_assert(convertible_to_optional<int>);
static_assert(convertible_to_choice<int>);
static_assert(convertible_to_expected<void, Error>); // the legitimate void: expected<void, E>
static_assert(convertible_to_expected<int, Error>);
// cv-qualified void is still void - remove_cvref_t folds it back - and every answer must match
static_assert(not convertible_to_unexpected<void const>);
static_assert(not convertible_to_optional<void const>);
static_assert(not convertible_to_choice<void const>);
static_assert(not convertible_to_unexpected<void const volatile>);
static_assert(not convertible_to_optional<void const volatile>);
static_assert(not convertible_to_choice<void const volatile>);
static_assert(convertible_to_expected<void const, Error>);
static_assert(convertible_to_expected<void const volatile, Error>);
} // namespace fn

TEST_CASE("Dummy") { SUCCEED(); }
