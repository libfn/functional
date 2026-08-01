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

Members are not restated here. A `pfn` type is the standard's type, member for member, so each
one below names its standard counterpart and links to where it is specified; a second copy of
that specification would only be a second thing to keep true. What this page documents is what
differs: the deviations above, and the entities `pfn` adds because C++20 has no equivalent.

---

## expected {style: "api"}

##### Defined in {style: "api", badge: "#include <pfn/expected.hpp>"}

:include-doxygen-doc: pfn::expected

Its members are specified as [`std::expected`](https://en.cppreference.com/w/cpp/utility/expected).

### expected over void {style: "api"}
The partial specialization serving computations which succeed with no value.

:include-doxygen-doc: pfn::expected< void, E >

Its members are specified as [`std::expected<void, E>`](https://en.cppreference.com/w/cpp/utility/expected).

### unexpected {style: "api"}

:include-doxygen-doc: pfn::unexpected

Its members are specified as [`std::unexpected`](https://en.cppreference.com/w/cpp/utility/expected).

### unexpect {style: "api"}

:include-doxygen-doc: pfn::unexpect_t

Its members are specified as [`std::unexpect_t`](https://en.cppreference.com/w/cpp/utility/expected).

### bad_expected_access {style: "api"}

:include-doxygen-doc: pfn::bad_expected_access

Its members are specified as [`std::bad_expected_access`](https://en.cppreference.com/w/cpp/utility/expected).

:include-doxygen-doc: pfn::bad_expected_access< void >

---

## optional {style: "api"}

##### Defined in {style: "api", badge: "#include <pfn/optional.hpp>"}

:include-doxygen-doc: pfn::optional

Its members are specified as [`std::optional`](https://en.cppreference.com/w/cpp/utility/optional).

### optional over a reference {style: "api"}

:include-doxygen-doc: pfn::optional< T & >

Its members are specified as [`std::optional<T&>`](https://en.cppreference.com/w/cpp/utility/optional).

---

## apply {style: "api"}

##### Defined in {style: "api", badge: "#include <pfn/tuple.hpp>"}

```cpp {title: "pfn::apply"}
template <typename Fn, detail::_tuple_like Tuple>
constexpr auto apply(Fn &&fn, Tuple &&t) -> apply_result_t<Fn, Tuple>;  // (1)
```

:include-doxygen-doc: pfn::apply { args: "Fn &&, Tuple &&" }

:include-doxygen-doc-params: pfn::apply { args: "Fn &&, Tuple &&", title: "parameters" }

### Applicability traits {style: "api"}
The C++26 traits `apply` is specified through; each also comes in its `_v` (for the two
predicates) or `_t` (for the result) form.

:include-doxygen-doc: pfn::is_applicable

:include-doxygen-doc: pfn::is_nothrow_applicable

:include-doxygen-doc: pfn::apply_result

---

## invoke_r {style: "api"}

##### Defined in {style: "api", badge: "#include <pfn/functional.hpp>"}

```cpp {title: "pfn::invoke_r"}
template <class R, class F, class... Args>
constexpr auto invoke_r(F &&f, Args &&...args);  // (1)
```

:include-doxygen-doc: pfn::invoke_r { args: "F &&, Args &&..." }

:include-doxygen-doc-params: pfn::invoke_r { args: "F &&, Args &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: pfn::invoke_r { args: "F &&, Args &&...", title: "parameters" }

---

## unreachable {style: "api"}

##### Defined in {style: "api", badge: "#include <pfn/utility.hpp>"}

```cpp {title: "pfn::unreachable"}
auto unreachable() -> void;  // (1)
```

:include-doxygen-doc: pfn::unreachable { args: "" }

## More applicability traits {style: "api"}

```cpp {title: "pfn::apply_result_t"}
template <typename Fn, typename Tuple>
using apply_result_t = typename apply_result<Fn, Tuple>::type;  // (1)
```

:include-doxygen-doc: pfn::apply_result_t { args: "" }

```cpp {title: "pfn::is_applicable_v"}
template <typename Fn, typename Tuple>
constexpr bool is_applicable_v = is_applicable<Fn, Tuple>::value;  // (1)
```

:include-doxygen-doc: pfn::is_applicable_v { args: "" }

```cpp {title: "pfn::is_nothrow_applicable_v"}
template <typename Fn, typename Tuple>
constexpr bool is_nothrow_applicable_v = is_nothrow_applicable<Fn, Tuple>::value;  // (1)
```

:include-doxygen-doc: pfn::is_nothrow_applicable_v { args: "" }

## Comparison against a value {style: "api"}

```cpp {title: "pfn::operator=="}
template <class T, class E, class T2>
constexpr auto operator==(expected<T, E> const &x, T2 const &v) -> bool;  // (1)

template <class T, class U>
constexpr auto operator==(optional<T> const &, optional<U> const &) -> bool;  // (2)

template <class T>
constexpr auto operator==(optional<T> const &, std::nullopt_t) -> bool;  // (3)

template <class T, class U>
constexpr auto operator==(optional<T> const &, U const &) -> bool;  // (4)
constexpr auto operator==(T const &, optional<U> const &) -> bool;  // (5)
```
