---
title: "multidispatch fn::apply"
---

##### Defined in {style: "api", badge: "#include <fn/functional.hpp>"}

---

Elimination of the algebraic structures: `fn::apply` unpacks products and dispatches over
alternatives by ordinary C++ overload resolution, and `fn::overload` fuses per-alternative
lambdas into one overload set. Dispatch is exhaustive: an alternative without a viable arm makes
the whole call not applicable.

## apply {style: "api"}

```cpp {title: "fn::apply"}
template <typename Fn, typename... Args>
constexpr auto apply(Fn &&fn, Args &&...args) -> apply_result_t<Fn, Args...>;  // (1)
```

:include-doxygen-doc: fn::apply { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::apply { args: "Fn &&, Args &&...", title: "parameters" }

## apply_r {style: "api"}

```cpp {title: "fn::apply_r"}
template <typename Ret, typename Fn, typename... Args>
constexpr auto apply_r(Fn &&fn, Args &&...args) -> Ret;  // (1)
```

:include-doxygen-doc: fn::apply_r { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::apply_r { args: "Fn &&, Args &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::apply_r { args: "Fn &&, Args &&...", title: "parameters" }

## overload {style: "api"}

##### Defined in {style: "api", badge: "#include <fn/utility.hpp>"}

:include-doxygen-doc: fn::overload

## Applicability traits {style: "api"}

```cpp {title: "fn::apply_result_t"}
template <typename Fn, typename... Args>
using apply_result_t = typename apply_result<Fn, Args...>::type;  // (1)
```

:include-doxygen-doc: fn::apply_result_t { args: "" }

```cpp {title: "fn::is_applicable_v"}
template <typename Fn, typename... Args>
constexpr bool is_applicable_v = is_applicable<Fn, Args...>::value;  // (1)
```

:include-doxygen-doc: fn::is_applicable_v { args: "" }

```cpp {title: "fn::is_applicable_r_v"}
template <typename Ret, typename Fn, typename... Args>
constexpr bool is_applicable_r_v = is_applicable_r<Ret, Fn, Args...>::value;  // (1)
```

:include-doxygen-doc: fn::is_applicable_r_v { args: "" }

```cpp {title: "fn::is_nothrow_applicable_v"}
template <typename Fn, typename... Args>
constexpr bool is_nothrow_applicable_v = is_nothrow_applicable<Fn, Args...>::value;  // (1)
```

:include-doxygen-doc: fn::is_nothrow_applicable_v { args: "" }

```cpp {title: "fn::is_nothrow_applicable_r_v"}
template <typename Ret, typename Fn, typename... Args>
constexpr bool is_nothrow_applicable_r_v = is_nothrow_applicable_r<Ret, Fn, Args...>::value;  // (1)
```

:include-doxygen-doc: fn::is_nothrow_applicable_r_v { args: "" }

:include-doxygen-doc: fn::is_applicable_r

:include-doxygen-doc: fn::is_nothrow_applicable_r
