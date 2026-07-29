---
title: Polyfill layer
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
