---
title: "monad fn::optional"
---

##### Defined in {style: "api", badge: "#include <fn/optional.hpp>"}

---

:include-doxygen-doc: fn::optional

## copack_value {style: "api"}
The explicit lift into the graded world.

```cpp {title: "fn::optional::copack_value"}
constexpr auto copack_value() const &  -> optional<copack<value_type>>;  // (1)
constexpr auto copack_value() &&       -> optional<copack<value_type>>;  // (2)
constexpr auto copack_value() &        -> decltype(auto);                // (3)
constexpr auto copack_value() const &  -> decltype(auto);                // (4)
constexpr auto copack_value() &&       -> decltype(auto);                // (5)
constexpr auto copack_value() const && -> decltype(auto);                // (6)
```

:include-doxygen-doc: fn::optional::copack_value { args: "" }

:include-doxygen-doc-params: fn::optional::copack_value { args: "", title: "parameters" }

## and_then {style: "api"}

```cpp {title: "fn::optional::and_then"}
template <class F>
constexpr auto and_then(F &&f) &;         // (1)
constexpr auto and_then(F &&f) &&;        // (2)
constexpr auto and_then(F &&f) const &;   // (3)
constexpr auto and_then(F &&f) const &&;  // (4)
```

:include-doxygen-doc: fn::optional::and_then { args: "F &&" }

:include-doxygen-doc-params: fn::optional::and_then { args: "F &&", title: "parameters" }

```cpp {title: "fn::optional< T & >::and_then"}
template <class F>
constexpr auto and_then(F &&f) const;  // (1)
```

:include-doxygen-doc: fn::optional< T & >::and_then { args: "F &&" }

:include-doxygen-doc-params: fn::optional< T & >::and_then { args: "F &&", title: "parameters" }

## or_else {style: "api"}

```cpp {title: "fn::optional::or_else"}
template <class F>
constexpr auto or_else(F &&f) const &;  // (1)
constexpr auto or_else(F &&f) &&;       // (2)
```

:include-doxygen-doc: fn::optional::or_else { args: "F &&" }

:include-doxygen-doc-params: fn::optional::or_else { args: "F &&", title: "parameters" }

```cpp {title: "fn::optional< T & >::or_else"}
template <class F>
constexpr auto or_else(F &&f) const;  // (1)
```

:include-doxygen-doc: fn::optional< T & >::or_else { args: "F &&" }

:include-doxygen-doc-params: fn::optional< T & >::or_else { args: "F &&", title: "parameters" }

## transform {style: "api"}

```cpp {title: "fn::optional::transform"}
template <class F>
constexpr auto transform(F &&f) &;         // (1)
constexpr auto transform(F &&f) &&;        // (2)
constexpr auto transform(F &&f) const &;   // (3)
constexpr auto transform(F &&f) const &&;  // (4)
```

:include-doxygen-doc: fn::optional::transform { args: "F &&" }

:include-doxygen-doc-params: fn::optional::transform { args: "F &&", title: "parameters" }

```cpp {title: "fn::optional< T & >::transform"}
template <class F>
constexpr auto transform(F &&f) const;  // (1)
```

:include-doxygen-doc: fn::optional< T & >::transform { args: "F &&" }

:include-doxygen-doc-params: fn::optional< T & >::transform { args: "F &&", title: "parameters" }

## apply {style: "api"}

```cpp {title: "fn::optional::apply"}
template <class F, class... Args>
constexpr auto apply(F &&f, Args &&...args) &;         // (1)
constexpr auto apply(F &&f, Args &&...args) &&;        // (2)
constexpr auto apply(F &&f, Args &&...args) const &;   // (3)
constexpr auto apply(F &&f, Args &&...args) const &&;  // (4)
```

:include-doxygen-doc: fn::optional::apply { args: "F &&, Args &&..." }

:include-doxygen-doc-params: fn::optional::apply { args: "F &&, Args &&...", title: "parameters" }

```cpp {title: "fn::optional< T & >::apply"}
template <class F, class... Args>
constexpr auto apply(F &&f, Args &&...args) const;  // (1)
```

:include-doxygen-doc: fn::optional< T & >::apply { args: "F &&, Args &&..." }

:include-doxygen-doc-params: fn::optional< T & >::apply { args: "F &&, Args &&...", title: "parameters" }

## apply_r {style: "api"}

```cpp {title: "fn::optional::apply_r"}
template <class Ret, class F, class... Args>
constexpr auto apply_r(F &&f, Args &&...args) &;         // (1)
constexpr auto apply_r(F &&f, Args &&...args) &&;        // (2)
constexpr auto apply_r(F &&f, Args &&...args) const &;   // (3)
constexpr auto apply_r(F &&f, Args &&...args) const &&;  // (4)
```

:include-doxygen-doc: fn::optional::apply_r { args: "F &&, Args &&..." }

:include-doxygen-doc-params: fn::optional::apply_r { args: "F &&, Args &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::optional::apply_r { args: "F &&, Args &&...", title: "parameters" }

```cpp {title: "fn::optional< T & >::apply_r"}
template <class Ret, class F, class... Args>
constexpr auto apply_r(F &&f, Args &&...args) const;  // (1)
```

:include-doxygen-doc: fn::optional< T & >::apply_r { args: "F &&, Args &&..." }

:include-doxygen-doc-params: fn::optional< T & >::apply_r { args: "F &&, Args &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::optional< T & >::apply_r { args: "F &&, Args &&...", title: "parameters" }

## apply_type {style: "api"}

```cpp {title: "fn::optional::apply_type"}
template <class F, class... Args>
constexpr auto apply_type(F &&f, Args &&...args) &;         // (1)
constexpr auto apply_type(F &&f, Args &&...args) &&;        // (2)
constexpr auto apply_type(F &&f, Args &&...args) const &;   // (3)
constexpr auto apply_type(F &&f, Args &&...args) const &&;  // (4)
```

:include-doxygen-doc: fn::optional::apply_type { args: "F &&, Args &&..." }

:include-doxygen-doc-params: fn::optional::apply_type { args: "F &&, Args &&...", title: "parameters" }

```cpp {title: "fn::optional< T & >::apply_type"}
template <class F, class... Args>
constexpr auto apply_type(F &&f, Args &&...args) const;  // (1)
```

:include-doxygen-doc: fn::optional< T & >::apply_type { args: "F &&, Args &&..." }

:include-doxygen-doc-params: fn::optional< T & >::apply_type { args: "F &&, Args &&...", title: "parameters" }

## apply_type_r {style: "api"}

```cpp {title: "fn::optional::apply_type_r"}
template <class Ret, class F, class... Args>
constexpr auto apply_type_r(F &&f, Args &&...args) &;         // (1)
constexpr auto apply_type_r(F &&f, Args &&...args) &&;        // (2)
constexpr auto apply_type_r(F &&f, Args &&...args) const &;   // (3)
constexpr auto apply_type_r(F &&f, Args &&...args) const &&;  // (4)
```

:include-doxygen-doc: fn::optional::apply_type_r { args: "F &&, Args &&..." }

:include-doxygen-doc-params: fn::optional::apply_type_r { args: "F &&, Args &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::optional::apply_type_r { args: "F &&, Args &&...", title: "parameters" }

```cpp {title: "fn::optional< T & >::apply_type_r"}
template <class Ret, class F, class... Args>
constexpr auto apply_type_r(F &&f, Args &&...args) const;  // (1)
```

:include-doxygen-doc: fn::optional< T & >::apply_type_r { args: "F &&, Args &&..." }

:include-doxygen-doc-params: fn::optional< T & >::apply_type_r { args: "F &&, Args &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::optional< T & >::apply_type_r { args: "F &&, Args &&...", title: "parameters" }
