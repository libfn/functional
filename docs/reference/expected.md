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
