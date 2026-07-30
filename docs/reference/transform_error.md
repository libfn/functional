---
title: "functor fn::transform_error"
---

##### Defined in {style: "api", badge: "#include <fn/transform_error.hpp>"}

---

:include-doxygen-doc: fn::transform_error_t

---

## Call signatures {style: "api"}

```cpp {title: "fn::transform_error_t::operator()"}
constexpr auto operator()(auto &&fn) const -> functor<transform_error_t, decltype(fn)>;  // (1)
```

:include-doxygen-doc: fn::transform_error_t::operator() { args: "auto &&" }

:include-doxygen-doc-params: fn::transform_error_t::operator() { args: "auto &&", title: "parameters" }

---

## Return value {style: "api"}

A monadic type of the same kind.

On `optional` the operation is rejected: an `optional` has no error value to map. Use `or_else`
to act on the empty state instead.
