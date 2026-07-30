---
title: "functor fn::fail"
---

##### Defined in {style: "api", badge: "#include <fn/fail.hpp>"}

---

:include-doxygen-doc: fn::fail_t

---

## Call signatures {style: "api"}

```cpp {title: "fn::fail_t::operator()"}
constexpr auto operator()(auto &&fn) const -> functor<fail_t, decltype(fn)>;  // (1)
```

:include-doxygen-doc: fn::fail_t::operator() { args: "auto &&" }

:include-doxygen-doc-params: fn::fail_t::operator() { args: "auto &&", title: "parameters" }

---

## Return value {style: "api"}
A monadic type of the same kind.
