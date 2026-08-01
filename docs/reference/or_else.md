---
title: "functor fn::or_else"
---

##### Defined in {style: "api", badge: "#include <fn/or_else.hpp>"}

---

:include-doxygen-doc: fn::or_else_t

---

## The verb object {style: "api"}

```cpp {title: "fn::or_else"}
or_else_t or_else = {};  // (1)
```

:include-doxygen-doc: fn::or_else { args: "" }

## Call signatures {style: "api"}

```cpp {title: "fn::or_else_t::operator()"}
constexpr auto operator()(auto &&fn) const -> functor<or_else_t, decltype(fn)>;  // (1)
```

:include-doxygen-doc: fn::or_else_t::operator() { args: "auto &&" }

:include-doxygen-doc-params: fn::or_else_t::operator() { args: "auto &&", title: "parameters" }

---

## Return value {style: "api"}

A monadic type of the same kind.
