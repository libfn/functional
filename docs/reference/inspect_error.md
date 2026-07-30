---
title: "functor fn::inspect_error"
---

##### Defined in {style: "api", badge: "#include <fn/inspect_error.hpp>"}

---

:include-doxygen-doc: fn::inspect_error_t

---

## Call signatures {style: "api"}

```cpp {title: "fn::inspect_error_t::operator()"}
constexpr auto operator()(auto &&fn) const -> functor<inspect_error_t, decltype(fn)>;  // (1)
```

:include-doxygen-doc: fn::inspect_error_t::operator() { args: "auto &&" }

:include-doxygen-doc-params: fn::inspect_error_t::operator() { args: "auto &&", title: "parameters" }

---

## Return value {style: "api"}

A monadic type of the same kind.
