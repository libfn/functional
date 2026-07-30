---
title: "monad fn::optional"
---

##### Defined in {style: "api", badge: "#include <fn/optional.hpp>"}

---

:include-doxygen-doc: fn::optional

## copack_value {style: "api"}
The explicit lift into the graded world.

```cpp {title: "fn::optional::copack_value"}
constexpr auto copack_value() const &  -> optional<copack<value_type>>;  // (1)
constexpr auto copack_value() &&       -> optional<copack<value_type>>;  // (2)
constexpr auto copack_value() &        -> decltype(auto);                // (3)
constexpr auto copack_value() const &  -> decltype(auto);                // (4)
constexpr auto copack_value() &&       -> decltype(auto);                // (5)
constexpr auto copack_value() const && -> decltype(auto);                // (6)
```

:include-doxygen-doc: fn::optional::copack_value { args: "" }

:include-doxygen-doc-params: fn::optional::copack_value { args: "", title: "parameters" }
