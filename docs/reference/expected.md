---
title: "monad fn::expected"
---

##### Defined in {style: "api", badge: "#include <fn/expected.hpp>"}

---

:include-doxygen-doc: fn::expected

## expected_unit {style: "api"}
The graded gateway: initiating a pipeline with this unit trigger opts all subsequent `and_then`
steps into graded error-set unioning, with no fake starting errors.

```cpp {title: "fn::expected_unit"}
using expected_unit = expected<void, copack<>>;  // (1)
```

:include-doxygen-doc: fn::expected_unit { args: "" }

## copack_error {style: "api"}
The explicit lift into the graded world, on the error side.

```cpp {title: "fn::expected::copack_error"}
constexpr auto copack_error() const &  -> expected<value_type, copack<error_type>>;  // (1)
constexpr auto copack_error() &&       -> expected<value_type, copack<error_type>>;  // (2)
constexpr auto copack_error() &        -> decltype(auto);                            // (3)
constexpr auto copack_error() const &  -> decltype(auto);                            // (4)
constexpr auto copack_error() &&       -> decltype(auto);                            // (5)
constexpr auto copack_error() const && -> decltype(auto);                            // (6)
```

:include-doxygen-doc: fn::expected::copack_error { args: "" }

:include-doxygen-doc-params: fn::expected::copack_error { args: "", title: "parameters" }

## copack_value {style: "api"}
The same lift, on the value side.

```cpp {title: "fn::expected::copack_value"}
constexpr auto copack_value() const &  -> expected<copack<value_type>, error_type>;  // (1)
constexpr auto copack_value() &&       -> expected<copack<value_type>, error_type>;  // (2)
constexpr auto copack_value() &        -> decltype(auto);                            // (3)
constexpr auto copack_value() const &  -> decltype(auto);                            // (4)
constexpr auto copack_value() &&       -> decltype(auto);                            // (5)
constexpr auto copack_value() const && -> decltype(auto);                            // (6)
```

:include-doxygen-doc: fn::expected::copack_value { args: "" }

:include-doxygen-doc-params: fn::expected::copack_value { args: "", title: "parameters" }

## and_then {style: "api"}

```cpp {title: "fn::expected::and_then"}
template <class F>
constexpr auto and_then(F &&f) &;         // (1)
constexpr auto and_then(F &&f) &&;        // (2)
constexpr auto and_then(F &&f) const &;   // (3)
constexpr auto and_then(F &&f) const &&;  // (4)
```

:include-doxygen-doc: fn::expected::and_then { args: "F &&" }

:include-doxygen-doc-params: fn::expected::and_then { args: "F &&", title: "parameters" }

## or_else {style: "api"}

```cpp {title: "fn::expected::or_else"}
template <class F>
constexpr auto or_else(F &&f) &;         // (1)
constexpr auto or_else(F &&f) &&;        // (2)
constexpr auto or_else(F &&f) const &;   // (3)
constexpr auto or_else(F &&f) const &&;  // (4)
```

:include-doxygen-doc: fn::expected::or_else { args: "F &&" }

:include-doxygen-doc-params: fn::expected::or_else { args: "F &&", title: "parameters" }

## transform {style: "api"}

```cpp {title: "fn::expected::transform"}
template <class F>
constexpr auto transform(F &&f) &;         // (1)
constexpr auto transform(F &&f) &&;        // (2)
constexpr auto transform(F &&f) const &;   // (3)
constexpr auto transform(F &&f) const &&;  // (4)
```

:include-doxygen-doc: fn::expected::transform { args: "F &&" }

:include-doxygen-doc-params: fn::expected::transform { args: "F &&", title: "parameters" }

## transform_error {style: "api"}

```cpp {title: "fn::expected::transform_error"}
template <class F>
constexpr auto transform_error(F &&f) &;         // (1)
constexpr auto transform_error(F &&f) &&;        // (2)
constexpr auto transform_error(F &&f) const &;   // (3)
constexpr auto transform_error(F &&f) const &&;  // (4)
```

:include-doxygen-doc: fn::expected::transform_error { args: "F &&" }

:include-doxygen-doc-params: fn::expected::transform_error { args: "F &&", title: "parameters" }

## apply {style: "api"}

```cpp {title: "fn::expected::apply"}
template <class F, class... Args>
constexpr auto apply(F &&f, Args &&...args) &;         // (1)
constexpr auto apply(F &&f, Args &&...args) &&;        // (2)
constexpr auto apply(F &&f, Args &&...args) const &;   // (3)
constexpr auto apply(F &&f, Args &&...args) const &&;  // (4)
```

:include-doxygen-doc: fn::expected::apply { args: "F &&, Args &&..." }

:include-doxygen-doc-params: fn::expected::apply { args: "F &&, Args &&...", title: "parameters" }

## apply_r {style: "api"}

```cpp {title: "fn::expected::apply_r"}
template <class Ret, class F, class... Args>
constexpr auto apply_r(F &&f, Args &&...args) &;         // (1)
constexpr auto apply_r(F &&f, Args &&...args) &&;        // (2)
constexpr auto apply_r(F &&f, Args &&...args) const &;   // (3)
constexpr auto apply_r(F &&f, Args &&...args) const &&;  // (4)
```

:include-doxygen-doc: fn::expected::apply_r { args: "F &&, Args &&..." }

:include-doxygen-doc-params: fn::expected::apply_r { args: "F &&, Args &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::expected::apply_r { args: "F &&, Args &&...", title: "parameters" }

## apply_type {style: "api"}

```cpp {title: "fn::expected::apply_type"}
template <class F, class... Args>
constexpr auto apply_type(F &&f, Args &&...args) &;         // (1)
constexpr auto apply_type(F &&f, Args &&...args) &&;        // (2)
constexpr auto apply_type(F &&f, Args &&...args) const &;   // (3)
constexpr auto apply_type(F &&f, Args &&...args) const &&;  // (4)
```

:include-doxygen-doc: fn::expected::apply_type { args: "F &&, Args &&..." }

:include-doxygen-doc-params: fn::expected::apply_type { args: "F &&, Args &&...", title: "parameters" }

## apply_type_r {style: "api"}

```cpp {title: "fn::expected::apply_type_r"}
template <class Ret, class F, class... Args>
constexpr auto apply_type_r(F &&f, Args &&...args) &;         // (1)
constexpr auto apply_type_r(F &&f, Args &&...args) &&;        // (2)
constexpr auto apply_type_r(F &&f, Args &&...args) const &;   // (3)
constexpr auto apply_type_r(F &&f, Args &&...args) const &&;  // (4)
```

:include-doxygen-doc: fn::expected::apply_type_r { args: "F &&, Args &&..." }

:include-doxygen-doc-params: fn::expected::apply_type_r { args: "F &&, Args &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::expected::apply_type_r { args: "F &&, Args &&...", title: "parameters" }
