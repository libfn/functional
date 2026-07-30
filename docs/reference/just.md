---
title: "monad fn::just"
---

##### Defined in {style: "api", badge: "#include <fn/just.hpp>"}

---

:include-doxygen-doc: fn::just

## value {style: "api"}
The payload, always present: the access is total, never throwing.

```cpp {title: "fn::just::value"}
constexpr auto value() &        -> T &;         // (1)
constexpr auto value() const &  -> T const &;   // (2)
constexpr auto value() &&       -> T &&;        // (3)
constexpr auto value() const && -> T const &&;  // (4)
```

:include-doxygen-doc: fn::just::value { args: "" }

:include-doxygen-doc-params: fn::just::value { args: "", title: "parameters" }

```cpp {title: "fn::just< void >::value"}
constexpr auto value() const -> void;  // (1)
```

:include-doxygen-doc: fn::just< void >::value { args: "" }
## transform {style: "api"}

```cpp {title: "fn::just::transform"}
template <typename Fn>
constexpr auto transform(Fn &&fn) &;         // (1)
constexpr auto transform(Fn &&fn) const &;   // (2)
constexpr auto transform(Fn &&fn) &&;        // (3)
constexpr auto transform(Fn &&fn) const &&;  // (4)
```

:include-doxygen-doc: fn::just::transform { args: "Fn &&" }

:include-doxygen-doc-params: fn::just::transform { args: "Fn &&", title: "parameters" }

```cpp {title: "fn::just< void >::transform"}
template <typename Fn>
constexpr auto transform(Fn &&fn) const;  // (1)
```

:include-doxygen-doc: fn::just< void >::transform { args: "Fn &&" }

:include-doxygen-doc-params: fn::just< void >::transform { args: "Fn &&", title: "parameters" }
## and_then {style: "api"}

```cpp {title: "fn::just::and_then"}
template <typename Fn>
constexpr auto and_then(Fn &&fn) &;         // (1)
constexpr auto and_then(Fn &&fn) const &;   // (2)
constexpr auto and_then(Fn &&fn) &&;        // (3)
constexpr auto and_then(Fn &&fn) const &&;  // (4)
```

:include-doxygen-doc: fn::just::and_then { args: "Fn &&" }

:include-doxygen-doc-params: fn::just::and_then { args: "Fn &&", title: "parameters" }

```cpp {title: "fn::just< void >::and_then"}
template <typename Fn>
constexpr auto and_then(Fn &&fn) const;  // (1)
```

:include-doxygen-doc: fn::just< void >::and_then { args: "Fn &&" }

:include-doxygen-doc-params: fn::just< void >::and_then { args: "Fn &&", title: "parameters" }

## apply {style: "api"}

```cpp {title: "fn::just::apply"}
template <typename Fn, typename... Args>
constexpr auto apply(Fn &&fn, Args &&...args) &        -> decltype(auto);  // (1)
constexpr auto apply(Fn &&fn, Args &&...args) const &  -> decltype(auto);  // (2)
constexpr auto apply(Fn &&fn, Args &&...args) &&       -> decltype(auto);  // (3)
constexpr auto apply(Fn &&fn, Args &&...args) const && -> decltype(auto);  // (4)
```

:include-doxygen-doc: fn::just::apply { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::just::apply { args: "Fn &&, Args &&...", title: "parameters" }

```cpp {title: "fn::just< void >::apply"}
template <typename Fn, typename... Args>
constexpr auto apply(Fn &&fn, Args &&...args) const -> decltype(auto);  // (1)
```

:include-doxygen-doc: fn::just< void >::apply { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::just< void >::apply { args: "Fn &&, Args &&...", title: "parameters" }

## apply_r {style: "api"}

```cpp {title: "fn::just::apply_r"}
template <typename Ret, typename Fn, typename... Args>
constexpr auto apply_r(Fn &&fn, Args &&...args) &        -> Ret;  // (1)
constexpr auto apply_r(Fn &&fn, Args &&...args) const &  -> Ret;  // (2)
constexpr auto apply_r(Fn &&fn, Args &&...args) &&       -> Ret;  // (3)
constexpr auto apply_r(Fn &&fn, Args &&...args) const && -> Ret;  // (4)
```

:include-doxygen-doc: fn::just::apply_r { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::just::apply_r { args: "Fn &&, Args &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::just::apply_r { args: "Fn &&, Args &&...", title: "parameters" }

```cpp {title: "fn::just< void >::apply_r"}
template <typename Ret, typename Fn, typename... Args>
constexpr auto apply_r(Fn &&fn, Args &&...args) const -> Ret;  // (1)
```

:include-doxygen-doc: fn::just< void >::apply_r { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::just< void >::apply_r { args: "Fn &&, Args &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::just< void >::apply_r { args: "Fn &&, Args &&...", title: "parameters" }

## apply_type {style: "api"}

```cpp {title: "fn::just::apply_type"}
template <typename Fn, typename... Args>
constexpr auto apply_type(Fn &&fn, Args &&...args) &        -> decltype(auto);  // (1)
constexpr auto apply_type(Fn &&fn, Args &&...args) const &  -> decltype(auto);  // (2)
constexpr auto apply_type(Fn &&fn, Args &&...args) &&       -> decltype(auto);  // (3)
constexpr auto apply_type(Fn &&fn, Args &&...args) const && -> decltype(auto);  // (4)
```

:include-doxygen-doc: fn::just::apply_type { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::just::apply_type { args: "Fn &&, Args &&...", title: "parameters" }

```cpp {title: "fn::just< void >::apply_type"}
template <typename Fn, typename... Args>
constexpr auto apply_type(Fn &&fn, Args &&...args) const -> decltype(auto);  // (1)
```

:include-doxygen-doc: fn::just< void >::apply_type { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::just< void >::apply_type { args: "Fn &&, Args &&...", title: "parameters" }

## apply_type_r {style: "api"}

```cpp {title: "fn::just::apply_type_r"}
template <typename Ret, typename Fn, typename... Args>
constexpr auto apply_type_r(Fn &&fn, Args &&...args) &        -> Ret;  // (1)
constexpr auto apply_type_r(Fn &&fn, Args &&...args) const &  -> Ret;  // (2)
constexpr auto apply_type_r(Fn &&fn, Args &&...args) &&       -> Ret;  // (3)
constexpr auto apply_type_r(Fn &&fn, Args &&...args) const && -> Ret;  // (4)
```

:include-doxygen-doc: fn::just::apply_type_r { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::just::apply_type_r { args: "Fn &&, Args &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::just::apply_type_r { args: "Fn &&, Args &&...", title: "parameters" }

```cpp {title: "fn::just< void >::apply_type_r"}
template <typename Ret, typename Fn, typename... Args>
constexpr auto apply_type_r(Fn &&fn, Args &&...args) const -> Ret;  // (1)
```

:include-doxygen-doc: fn::just< void >::apply_type_r { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::just< void >::apply_type_r { args: "Fn &&, Args &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::just< void >::apply_type_r { args: "Fn &&, Args &&...", title: "parameters" }

## emplace {style: "api"}

```cpp {title: "fn::just::emplace"}
constexpr auto emplace(auto &&...args) -> T &;  // (1)
```

:include-doxygen-doc: fn::just::emplace { args: "auto &&..." }

:include-doxygen-doc-params: fn::just::emplace { args: "auto &&...", title: "parameters" }

## just {style: "api"}

```cpp {title: "fn::just::just"}
template <typename>
just;  // (1)

constexpr just() = default;              // (2)
constexpr just(just const &) = default;  // (3)
constexpr just(just &&) = default;       // (4)

template <typename U>
constexpr just(U &&v);           // (5)
constexpr explicit just(U &&v);  // (6)

constexpr explicit just(std::in_place_type_t<T>, auto &&...args);  // (7)

template <typename Fn>
constexpr explicit just(detail::_just_from_invoke_t, Fn &&make);  // (8)
```

:include-doxygen-doc: fn::just::just { args: "U &&" }

:include-doxygen-doc-params: fn::just::just { args: "U &&", title: "parameters" }

:include-doxygen-doc: fn::just::just { args: "::std::in_place_type_t< T >, auto &&..." }

:include-doxygen-doc-params: fn::just::just { args: "::std::in_place_type_t< T >, auto &&...", title: "parameters" }

## operator= {style: "api"}

```cpp {title: "fn::just::operator="}
constexpr auto operator=(just const &) = default -> just &;  // (1)
constexpr auto operator=(just &&) = default      -> just &;  // (2)

template <typename U>
constexpr auto operator=(U &&v) -> just &;  // (3)
```

:include-doxygen-doc: fn::just::operator= { args: "U &&" }

:include-doxygen-doc-params: fn::just::operator= { args: "U &&", title: "parameters" }
