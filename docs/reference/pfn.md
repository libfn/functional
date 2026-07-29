---
title: "polyfills pfn"
---

The library is layered: namespace `pfn` is a faithful polyfill of standard vocabulary types and
utilities as specified for C++26, available to a C++20 compiler, and namespace `fn` builds the
functional-programming extensions on top of it. Every `fn` type with a `pfn` counterpart is a
strict superset of it: a valid program switching from `pfn` to `fn` changes neither compilation
nor behaviour.

`pfn` polyfills only what C++20 lacks: all of `<expected>`, the C++23 and C++26 additions to
`std::optional` (the monadic operations, iterator support, `optional<T&>`), `std::apply` in its
SFINAE-friendly C++26 shape together with its applicability traits, `std::invoke_r` and
`std::unreachable`. Names C++20 already has — `std::nullopt`, `std::in_place`,
`std::bad_optional_access` — are used directly and not mirrored.

The polyfills track the C++ working draft, deviating deliberately in three ways, each noted on
the entity it concerns:

* where the standard leaves a member's `noexcept` specification unstated, one is derived from
  the underlying types; every such clause is marked `// extension` in the source
* the draft's hardened preconditions are checked with an assertion, customizable by defining
  `LIBFN_ASSERT` before inclusion
* `expected`'s comparison against a value is declared at namespace scope rather than as a
  hidden friend, keeping its constraint deducible

---

## expected {style: "api"}

##### Defined in {style: "api", badge: "#include <pfn/expected.hpp>"}

:include-doxygen-doc: pfn::expected

### expected over void {style: "api"}
The partial specialization serving computations which succeed with no value.

:include-doxygen-doc: pfn::expected< void, E >

### unexpected {style: "api"}

:include-doxygen-doc: pfn::unexpected

### unexpect {style: "api"}

:include-doxygen-doc: pfn::unexpect_t

### bad_expected_access {style: "api"}

:include-doxygen-doc: pfn::bad_expected_access

:include-doxygen-doc: pfn::bad_expected_access< void >

---

## optional {style: "api"}

##### Defined in {style: "api", badge: "#include <pfn/optional.hpp>"}

:include-doxygen-doc: pfn::optional

### optional over a reference {style: "api"}

:include-doxygen-doc: pfn::optional< T & >

---

## apply {style: "api"}

##### Defined in {style: "api", badge: "#include <pfn/tuple.hpp>"}

:include-doxygen-member: pfn::apply { signatureOnly: false, includeAllMatches: true }

### Applicability traits {style: "api"}
The C++26 traits `apply` is specified through; each also comes in its `_v` (for the two
predicates) or `_t` (for the result) form.

:include-doxygen-doc: pfn::is_applicable

:include-doxygen-doc: pfn::is_nothrow_applicable

:include-doxygen-doc: pfn::apply_result

---

## invoke_r {style: "api"}

##### Defined in {style: "api", badge: "#include <pfn/functional.hpp>"}

:include-doxygen-member: pfn::invoke_r { signatureOnly: false, includeAllMatches: true }

---

## unreachable {style: "api"}

##### Defined in {style: "api", badge: "#include <pfn/utility.hpp>"}

:include-doxygen-member: pfn::unreachable { signatureOnly: false, includeAllMatches: true }
