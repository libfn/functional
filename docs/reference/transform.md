---
title: "functor fn::transform"
---

##### Defined in {style: "api", badge: "#include <fn/transform.hpp>"}

---

:include-doxygen-doc: fn::transform_t

---

## The verb object {style: "api"}

```cpp {title: "fn::transform"}
transform_t transform = {};  // (1)
```

:include-doxygen-doc: fn::transform { args: "" }

## Call signatures {style: "api"}

```cpp {title: "fn::transform_t::operator()"}
constexpr auto operator()(auto &&fn) const -> functor<transform_t, decltype(fn)>;  // (1)
```

:include-doxygen-doc: fn::transform_t::operator() { args: "auto &&" }

:include-doxygen-doc-params: fn::transform_t::operator() { args: "auto &&", title: "parameters" }

---

## Return value {style: "api"}

A monadic type of the same kind.
