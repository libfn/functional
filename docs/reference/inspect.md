---
title: "functor fn::inspect"
---

##### Defined in {style: "api", badge: "#include <fn/inspect.hpp>"}

---

:include-doxygen-doc: fn::inspect_t

---

## Call signatures {style: "api"}

```cpp {title: "fn::inspect_t::operator()"}
constexpr auto operator()(auto &&fn) const -> functor<inspect_t, decltype(fn)>;  // (1)
```

:include-doxygen-doc: fn::inspect_t::operator() { args: "auto &&" }

:include-doxygen-doc-params: fn::inspect_t::operator() { args: "auto &&", title: "parameters" }

---

## Return value {style: "api"}
A monadic type of the same kind.
