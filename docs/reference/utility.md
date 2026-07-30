---
title: "other fn utilities"
---

##### Defined in {style: "api", badge: "#include <fn/utility.hpp>"}

---

The small helpers the library exposes for use alongside the carriers: how an argument is stored
when a pipeline step holds it, how to forward with a borrowed value category, how to fuse
callables into one overload set, and how to lift a value into a type that prefers braces.

---

## overload {style: "api"}
Fuses per-alternative lambdas into a single overload set, which `fn::apply` and the verbs then
dispatch over by ordinary overload resolution.

```cpp {title: "fn::overload"}
template <typename... Ts>
overload(Ts const &...) -> overload<Ts...>;  // (1)
```

---

## as_value_t {style: "api"}

```cpp {title: "fn::as_value_t"}
template <typename T>
using as_value_t = decltype(detail::_as_value<T>);  // (1)
```

:include-doxygen-doc: fn::as_value_t { args: "" }

:include-doxygen-doc-params: fn::as_value_t { args: "", type: "template", title: "template parameters" }

---

## apply_const_lvalue {style: "api"}

```cpp {title: "fn::apply_const_lvalue"}
template <typename T>
constexpr auto apply_const_lvalue(auto &&v) -> decltype(auto);  // (1)
```

:include-doxygen-doc: fn::apply_const_lvalue { args: "auto &&" }

:include-doxygen-doc-params: fn::apply_const_lvalue { args: "auto &&", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::apply_const_lvalue { args: "auto &&", title: "parameters" }

---

## make {style: "api"}

```cpp {title: "fn::make"}
template <typename T, typename... Args>
constexpr auto make(Args &&...args) -> T;  // (1)
```

:include-doxygen-doc: fn::make { args: "Args &&..." }

:include-doxygen-doc-params: fn::make { args: "Args &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::make { args: "Args &&...", title: "parameters" }
