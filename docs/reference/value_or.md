---
title: "functor fn::value_or"
---

##### Defined in {style: "api", badge: "#include <fn/value_or.hpp>"}

---

:include-doxygen-doc: fn::value_or_t

---

## The verb object {style: "api"}

```cpp {title: "fn::value_or"}
value_or_t value_or = {};  // (1)
```

:include-doxygen-doc: fn::value_or { args: "" }

## Return value {style: "api"}

The value of the monadic type if present; otherwise the user-provided fallback value.

## Call signatures {style: "api"}

```cpp {title: "fn::value_or_t::operator()"}
template <typename... Args>
constexpr auto operator()(Args &&...args) const -> functor<value_or_t, Args &&...>;  // (1)
```

:include-doxygen-doc: fn::value_or_t::operator() { args: "Args &&..." }

:include-doxygen-doc-params: fn::value_or_t::operator() { args: "Args &&...", title: "parameters" }
