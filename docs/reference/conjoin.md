---
title: "fold fn::conjoin"
---

##### Defined in {style: "api", badge: "#include <fn/pack.hpp>"}

---

Independent computations compose side by side. The conjunction `a & b` keeps both results: values
multiply into a `pack` and errors sum into a `copack`, with the leftmost failing operand's error
held at runtime. Each carrier's header declares its own `&`; this header defines the n-ary fold
over it.

---

## The verb object {style: "api"}

```cpp {title: "fn::conjoin"}
conjoin_t conjoin;  // (1)
```

:include-doxygen-doc: fn::conjoin { args: "" }

## conjoin {style: "api"}

:include-doxygen-doc: fn::conjoin_t

---

## Call signatures {style: "api"}

```cpp {title: "fn::conjoin_t::operator()"}
template <typename Arg>
constexpr auto operator()(Arg &&arg) const -> decltype(arg);  // (1)

template <typename Arg, typename... Args>
constexpr auto operator()(Arg &&arg, Args &&...args) const;  // (2)
```

:include-doxygen-doc: fn::conjoin_t::operator() { args: "Arg &&" }

:include-doxygen-doc-params: fn::conjoin_t::operator() { args: "Arg &&", title: "parameters" }

:include-doxygen-doc: fn::conjoin_t::operator() { args: "Arg &&, Args &&..." }

:include-doxygen-doc-params: fn::conjoin_t::operator() { args: "Arg &&, Args &&...", title: "parameters" }
