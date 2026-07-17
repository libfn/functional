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
static_assert(same_kind<expected<Value, fn::copack<Error>>, expected<void, fn::copack<Error>>>);
static_assert(same_kind<expected<Value, fn::copack<Error>>, expected<void, fn::copack<int>>>);
// copack_for, not copack<Error,int>: the alternative order is platform-specific (see copack.cpp) — same_kind ignores it.
static_assert(same_kind<expected<Value, fn::copack<Error>>, expected<void, fn::copack_for<Error, int>>>);
static_assert(same_kind<expected<Value, fn::copack_for<Error, int>>, expected<void, fn::copack<Error>>>);
static_assert(same_kind<expected<Value, fn::copack<Error>>, expected<void, fn::copack<Xerror>>>);
static_assert(same_kind<expected<Value, fn::copack<Error>>, expected<void, fn::copack_for<Error, Xerror>>>);
static_assert(same_kind<expected<Value, fn::copack<int>>, expected<void, fn::copack_for<Error, Xerror>>>);
static_assert(not same_kind<expected<Value, Error>, expected<void, Xerror>>);
static_assert(not same_kind<expected<void, Error>, expected<void, Xerror>>);
static_assert(not same_kind<expected<void, Error>, expected<int, Xerror>>);
static_assert(not same_kind<expected<int, Error>, expected<void, Xerror>>);
static_assert(not same_kind<expected<int, Error>, expected<Value, Xerror>>);
static_assert(not same_kind<expected<void, Error>, expected<Value, Xerror>>);
static_assert(not same_kind<expected<Value, fn::copack<Error>>, expected<void, Error>>);
static_assert(not same_kind<expected<Value, fn::copack<Error>>, expected<void, Xerror>>);
static_assert(not same_kind<expected<Value, Error>, expected<void, fn::copack<Error>>>);
static_assert(not same_kind<expected<Value, Xerror>, expected<void, fn::copack<Error>>>);

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

namespace {
struct Immovable final {
  Immovable() = default;
  Immovable(Immovable const &) = delete;
  Immovable(Immovable &&) = delete;
};

struct Boolish final {
  explicit operator bool() const noexcept { return true; }
};
} // namespace

// same_value_kind is the symmetric twin of same_kind above: it pins the VALUE type and lets the error
// vary, where same_kind pins the error and lets the value vary.
static_assert(same_value_kind<expected<int, Error>, expected<int, Xerror>>);      // the error may differ
static_assert(not same_value_kind<expected<int, Error>, expected<Value, Error>>); // the value may not
static_assert(not same_kind<expected<int, Error>, expected<int, Xerror>>);        // ... and the other way round
static_assert(same_kind<expected<int, Error>, expected<Value, Error>>);

// a copack-valued expected matches any other copack-valued one, whatever the copacks hold - the arm that lets
// the value widen
static_assert(same_value_kind<expected<copack<int>, Error>, expected<copack<Value>, Xerror>>);
static_assert(not same_value_kind<expected<copack<int>, Error>, expected<int, Error>>);

static_assert(same_value_kind<optional<int>, optional<int>>);
static_assert(not same_value_kind<optional<int>, optional<Value>>);
static_assert(same_value_kind<optional<copack<int>>, optional<copack<Value>>>);

static_assert(same_value_kind<choice<int>, choice<Value>>);              // any two choices, as for same_kind
static_assert(not same_value_kind<optional<int>, expected<int, Error>>); // but never across kinds
static_assert(same_value_kind<expected<int, Error> const &, expected<int, Xerror> &&>); // cvref makes no difference

// same_monadic_type_as is the conjunction of the two: value and error must both agree
static_assert(same_monadic_type_as<expected<int, Error>, expected<int, Error>>);
static_assert(not same_monadic_type_as<expected<int, Error>, expected<int, Xerror>>);  // the error differs
static_assert(not same_monadic_type_as<expected<int, Error>, expected<Value, Error>>); // the value differs
static_assert(same_monadic_type_as<optional<int>, optional<int>>);
static_assert(not same_monadic_type_as<optional<int>, optional<Value>>);

// convertible_to_bool is what filter holds its predicate to
static_assert(convertible_to_bool<bool>);
static_assert(convertible_to_bool<int>);
static_assert(convertible_to_bool<int *>);
static_assert(convertible_to_bool<Boolish>); // an explicit operator bool suffices - the concept casts
static_assert(not convertible_to_bool<Error>);
static_assert(not convertible_to_bool<Value>);

// the convertible_to_* family each ask whether the monad can be built from the value by a cast, so an
// operand with nothing to move or copy into it satisfies none of them
static_assert(convertible_to_unexpected<Error>);
static_assert(not convertible_to_unexpected<Immovable>);

static_assert(convertible_to_expected<int, Error>);
static_assert(convertible_to_expected<void, Error>); // void is admitted outright, by its own arm
static_assert(not convertible_to_expected<Immovable, Error>);

static_assert(convertible_to_optional<int>);
static_assert(not convertible_to_optional<Immovable>);

static_assert(convertible_to_choice<int>);
static_assert(not convertible_to_choice<Immovable>);

// void can never convert to any of these - and each concept must answer so, rather than
// instantiate unexpected<void>, optional<void> or choice<void>, whose validity mandates are hard
// errors. Contrast expected, where a void value is legitimate and carries its own arm.
static_assert(not convertible_to_unexpected<void>);
static_assert(not convertible_to_optional<void>);
static_assert(not convertible_to_choice<void>);
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

TEST_CASE("concepts", "[concepts]") { SUCCEED(); }
