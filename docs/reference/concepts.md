---
title: "other fn concepts"
---

##### Defined in {style: "api", badge: "#include <fn/concepts.hpp>"}

---

The constraints the library states its own rules with, and which client code can state its
rules with too. They fall into four groups: what a type is, how two carriers relate, what a
value converts to, and whether an operation applies.

---

## What a type is {style: "api"}

### fn::some_monadic_type {style: "api"}
:include-doxygen-doc: fn::some_monadic_type

### fn::some_expected {style: "api"}
:include-doxygen-doc: fn::some_expected

### fn::some_expected_void {style: "api"}
:include-doxygen-doc: fn::some_expected_void

### fn::some_expected_non_void {style: "api"}
:include-doxygen-doc: fn::some_expected_non_void

### fn::some_optional {style: "api"}
:include-doxygen-doc: fn::some_optional

### fn::some_just {style: "api"}
:include-doxygen-doc: fn::some_just

### fn::some_choice {style: "api"}
:include-doxygen-doc: fn::some_choice

### fn::some_pack {style: "api"}
:include-doxygen-doc: fn::some_pack

### fn::some_copack {style: "api"}
:include-doxygen-doc: fn::some_copack

### fn::empty_copack {style: "api"}
:include-doxygen-doc: fn::empty_copack

### fn::some_identity {style: "api"}
:include-doxygen-doc: fn::some_identity

### fn::some_empty_error {style: "api"}
:include-doxygen-doc: fn::some_empty_error

### fn::some_empty_value {style: "api"}
:include-doxygen-doc: fn::some_empty_value

### fn::some_in_place_type {style: "api"}
:include-doxygen-doc: fn::some_in_place_type

---

## How two carriers relate {style: "api"}

### fn::same_kind {style: "api"}
:include-doxygen-doc: fn::same_kind

### fn::same_value_kind {style: "api"}
:include-doxygen-doc: fn::same_value_kind

### fn::same_monadic_type_as {style: "api"}
:include-doxygen-doc: fn::same_monadic_type_as

---

## What a value converts to {style: "api"}

### fn::convertible_to_expected {style: "api"}
:include-doxygen-doc: fn::convertible_to_expected

### fn::convertible_to_optional {style: "api"}
:include-doxygen-doc: fn::convertible_to_optional

### fn::convertible_to_choice {style: "api"}
:include-doxygen-doc: fn::convertible_to_choice

### fn::convertible_to_unexpected {style: "api"}
:include-doxygen-doc: fn::convertible_to_unexpected

### fn::convertible_to_bool {style: "api"}
:include-doxygen-doc: fn::convertible_to_bool

---

## Whether an operation applies {style: "api"}

`monadic_invocable` is the constraint `operator|` itself carries; the rest are the per-verb
constraints it dispatches to, and the ones a verb of your own would join.

### fn::monadic_invocable {style: "api"}
:include-doxygen-doc: fn::monadic_invocable

### fn::applicable_and_then {style: "api"}
:include-doxygen-doc: fn::applicable_and_then

### fn::applicable_and_then_across {style: "api"}
:include-doxygen-doc: fn::applicable_and_then_across

### fn::applicable_transform {style: "api"}
:include-doxygen-doc: fn::applicable_transform

### fn::applicable_transform_error {style: "api"}
:include-doxygen-doc: fn::applicable_transform_error

### fn::applicable_transform_promote {style: "api"}
:include-doxygen-doc: fn::applicable_transform_promote

### fn::applicable_or_else {style: "api"}
:include-doxygen-doc: fn::applicable_or_else

### fn::applicable_or_else_across {style: "api"}
:include-doxygen-doc: fn::applicable_or_else_across

### fn::applicable_recover {style: "api"}
:include-doxygen-doc: fn::applicable_recover

### fn::applicable_filter {style: "api"}
:include-doxygen-doc: fn::applicable_filter

### fn::applicable_inspect {style: "api"}
:include-doxygen-doc: fn::applicable_inspect

### fn::applicable_inspect_error {style: "api"}
:include-doxygen-doc: fn::applicable_inspect_error

### fn::applicable_fail {style: "api"}
:include-doxygen-doc: fn::applicable_fail

### fn::applicable_value_or {style: "api"}
:include-doxygen-doc: fn::applicable_value_or

---

## Whether a callable applies {style: "api"}

The constraints behind `fn::apply`, over plain arguments and over a type list alike.

### fn::applicable {style: "api"}
:include-doxygen-doc: fn::applicable

### fn::regular_applicable {style: "api"}
:include-doxygen-doc: fn::regular_applicable

### fn::typelist_applicable {style: "api"}
:include-doxygen-doc: fn::typelist_applicable

### fn::typelist_applicable_r {style: "api"}
:include-doxygen-doc: fn::typelist_applicable_r
