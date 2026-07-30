---
title: "functor fn::recover"
---

##### Defined in {style: "api", badge: "#include <fn/recover.hpp>"}

---

:include-doxygen-doc: fn::recover_t

---

## Call signatures {style: "api"}

```cpp {title: "fn::recover_t::operator()"}
constexpr auto operator()(auto &&fn) const -> functor<recover_t, decltype(fn)>;  // (1)
```

:include-doxygen-doc: fn::recover_t::operator() { args: "auto &&" }

:include-doxygen-doc-params: fn::recover_t::operator() { args: "auto &&", title: "parameters" }

---

## Return value {style: "api"}

A monadic type of the same kind.
