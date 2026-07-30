---
title: "monad fn::expected"
---

##### Defined in {style: "api", badge: "#include <fn/expected.hpp>"}

---

:include-doxygen-doc: fn::expected

## Member types {style: "api"}

```cpp {title: "fn::expected::value_type"}
using value_type = T;  // (1)
```

:include-doxygen-doc: fn::expected::value_type { args: "" }

```cpp {title: "fn::expected::error_type"}
using error_type = Err;  // (1)
```

:include-doxygen-doc: fn::expected::error_type { args: "" }

```cpp {title: "fn::expected::unexpected_type"}
using unexpected_type = ::fn::unexpected<Err>;  // (1)
```

:include-doxygen-doc: fn::expected::unexpected_type { args: "" }

```cpp {title: "fn::expected::rebind"}
template <class U>
using rebind = expected<U, error_type>;  // (1)
```

:include-doxygen-doc: fn::expected::rebind { args: "" }

```cpp {title: "fn::expected< void, Err >::value_type"}
using value_type = void;  // (1)
```

:include-doxygen-doc: fn::expected< void, Err >::value_type { args: "" }

```cpp {title: "fn::expected< void, Err >::error_type"}
using error_type = Err;  // (1)
```

:include-doxygen-doc: fn::expected< void, Err >::error_type { args: "" }

```cpp {title: "fn::expected< void, Err >::unexpected_type"}
using unexpected_type = ::fn::unexpected<Err>;  // (1)
```

:include-doxygen-doc: fn::expected< void, Err >::unexpected_type { args: "" }

```cpp {title: "fn::expected< void, Err >::rebind"}
template <class U>
using rebind = expected<U, error_type>;  // (1)
```

:include-doxygen-doc: fn::expected< void, Err >::rebind { args: "" }

## Construction {style: "api"}

```cpp {title: "fn::expected::expected"}
constexpr expected();  // (1)

template <class U, class G>
constexpr explicit expected(expected<U, G> const &s);  // (2)
constexpr explicit expected(expected<U, G> &&s);       // (3)

template <class U>
constexpr explicit expected(U &&v);  // (4)

template <class G>
constexpr explicit expected(::fn::unexpected<G> const &g);  // (5)
constexpr explicit expected(::fn::unexpected<G> &&g);       // (6)

template <class... Args>
constexpr explicit expected(std::in_place_t, Args &&...a);  // (7)

template <class U, class... Args>
constexpr explicit expected(std::in_place_t, std::initializer_list<U> il, Args &&...a);  // (8)

template <class... Args>
constexpr explicit expected(::fn::unexpect_t, Args &&...a);  // (9)

template <class U, class... Args>
constexpr explicit expected(::fn::unexpect_t, std::initializer_list<U> il, Args &&...a);  // (10)

constexpr expected(expected const &) = delete;        // (11)
constexpr expected(expected const &s) = default;      // (12)
constexpr expected(expected const &s);                // (13)
constexpr expected(expected &&s) noexcept = default;  // (14)
constexpr expected(expected &&s);                     // (15)

template <class Tag, class Fn, class... Args>
constexpr explicit expected(::pfn::detail::_expected_from_invoke_t tag, Tag which, Fn &&fn, Args &&...args);  // (16)
```

:include-doxygen-doc: fn::expected::expected { args: "" }

:include-doxygen-doc: fn::expected::expected { args: "U &&" }

:include-doxygen-doc: fn::expected::expected { args: "::fn::unexpected< G > const &" }

:include-doxygen-doc: fn::expected::expected { args: "::fn::unexpected< G > &&" }

:include-doxygen-doc: fn::expected::expected { args: "::std::in_place_t, Args &&..." }

:include-doxygen-doc: fn::expected::expected { args: "::std::in_place_t, ::std::initializer_list< U >, Args &&..." }

:include-doxygen-doc: fn::expected::expected { args: "::fn::unexpect_t, Args &&..." }

:include-doxygen-doc: fn::expected::expected { args: "::fn::unexpect_t, ::std::initializer_list< U >, Args &&..." }

:include-doxygen-doc: fn::expected::expected { args: "expected const &" }

:include-doxygen-doc: fn::expected::expected { args: "expected &&" }

```cpp {title: "fn::expected::~expected"}
constexpr ~expected() = default;  // (1)
```

:include-doxygen-doc: fn::expected::~expected { args: "" }

```cpp {title: "fn::expected< void, Err >::expected"}
constexpr expected();  // (1)

template <class U, class G>
constexpr explicit expected(expected<U, G> const &s);  // (2)
constexpr explicit expected(expected<U, G> &&s);       // (3)

template <class G>
constexpr explicit expected(::fn::unexpected<G> const &g);  // (4)
constexpr explicit expected(::fn::unexpected<G> &&g);       // (5)

constexpr explicit expected(std::in_place_t);  // (6)

template <class... Args>
constexpr explicit expected(::fn::unexpect_t, Args &&...a);  // (7)

template <class U, class... Args>
constexpr explicit expected(::fn::unexpect_t, std::initializer_list<U> il, Args &&...a);  // (8)

constexpr expected(expected const &) = delete;        // (9)
constexpr expected(expected const &) = default;       // (10)
constexpr expected(expected const &s);                // (11)
constexpr expected(expected &&s) noexcept = default;  // (12)
constexpr expected(expected &&s);                     // (13)

template <class Tag, class Fn, class... Args>
constexpr explicit expected(::pfn::detail::_expected_from_invoke_t tag, Tag which, Fn &&fn, Args &&...args);  // (14)
```

:include-doxygen-doc: fn::expected< void, Err >::expected { args: "" }

:include-doxygen-doc: fn::expected< void, Err >::expected { args: "::fn::unexpected< G > const &" }

:include-doxygen-doc: fn::expected< void, Err >::expected { args: "::fn::unexpected< G > &&" }

:include-doxygen-doc: fn::expected< void, Err >::expected { args: "::std::in_place_t" }

:include-doxygen-doc: fn::expected< void, Err >::expected { args: "::fn::unexpect_t, Args &&..." }

:include-doxygen-doc: fn::expected< void, Err >::expected { args: "::fn::unexpect_t, ::std::initializer_list< U >, Args &&..." }

:include-doxygen-doc: fn::expected< void, Err >::expected { args: "expected const &" }

:include-doxygen-doc: fn::expected< void, Err >::expected { args: "expected &&" }

```cpp {title: "fn::expected< void, Err >::~expected"}
constexpr ~expected() = default;  // (1)
```

:include-doxygen-doc: fn::expected< void, Err >::~expected { args: "" }

## Assignment {style: "api"}

```cpp {title: "fn::expected::operator="}
template <class U>
constexpr auto operator=(U &&s) -> expected &;  // (1)

template <class G>
constexpr auto operator=(::fn::unexpected<G> const &s) -> expected &;  // (2)
constexpr auto operator=(::fn::unexpected<G> &&s)      -> expected &;  // (3)

constexpr auto operator=(expected const &) = delete  -> expected &;  // (4)
constexpr auto operator=(expected const &) = default -> expected &;  // (5)
constexpr auto operator=(expected const &s)          -> expected &;  // (6)
constexpr auto operator=(expected &&) = default      -> expected &;  // (7)
constexpr auto operator=(expected &&s)               -> expected &;  // (8)
```

:include-doxygen-doc: fn::expected::operator= { args: "U &&" }

:include-doxygen-doc: fn::expected::operator= { args: "::fn::unexpected< G > const &" }

:include-doxygen-doc: fn::expected::operator= { args: "::fn::unexpected< G > &&" }

:include-doxygen-doc: fn::expected::operator= { args: "expected const &" }

:include-doxygen-doc: fn::expected::operator= { args: "expected &&" }

```cpp {title: "fn::expected< void, Err >::operator="}
template <class G>
constexpr auto operator=(::fn::unexpected<G> const &s) -> expected &;  // (1)
constexpr auto operator=(::fn::unexpected<G> &&s)      -> expected &;  // (2)

constexpr auto operator=(expected const &) = delete  -> expected &;  // (3)
constexpr auto operator=(expected const &) = default -> expected &;  // (4)
constexpr auto operator=(expected const &s)          -> expected &;  // (5)
constexpr auto operator=(expected &&) = default      -> expected &;  // (6)
constexpr auto operator=(expected &&s)               -> expected &;  // (7)
```

:include-doxygen-doc: fn::expected< void, Err >::operator= { args: "::fn::unexpected< G > const &" }

:include-doxygen-doc: fn::expected< void, Err >::operator= { args: "::fn::unexpected< G > &&" }

:include-doxygen-doc: fn::expected< void, Err >::operator= { args: "expected const &" }

:include-doxygen-doc: fn::expected< void, Err >::operator= { args: "expected &&" }

## swap {style: "api"}

```cpp {title: "fn::expected::swap"}
constexpr auto swap(expected &rhs) -> void;  // (1)
```

:include-doxygen-doc: fn::expected::swap { args: "expected &" }

```cpp {title: "fn::expected< void, Err >::swap"}
constexpr auto swap(expected &rhs) -> void;  // (1)
```

:include-doxygen-doc: fn::expected< void, Err >::swap { args: "expected &" }

## expected_unit {style: "api"}
The graded gateway: initiating a pipeline with this unit trigger opts all subsequent `and_then`
steps into graded error-set unioning, with no fake starting errors.

```cpp {title: "fn::expected_unit"}
using expected_unit = expected<void, copack<>>;  // (1)
```

:include-doxygen-doc: fn::expected_unit { args: "" }

## copack_error {style: "api"}
The explicit lift into the graded world, on the error side.

```cpp {title: "fn::expected::copack_error"}
constexpr auto copack_error() const &  -> expected<value_type, copack<error_type>>;  // (1)
constexpr auto copack_error() &&       -> expected<value_type, copack<error_type>>;  // (2)
constexpr auto copack_error() &        -> decltype(auto);                            // (3)
constexpr auto copack_error() const &  -> decltype(auto);                            // (4)
constexpr auto copack_error() &&       -> decltype(auto);                            // (5)
constexpr auto copack_error() const && -> decltype(auto);                            // (6)
```

:include-doxygen-doc: fn::expected::copack_error { args: "" }

:include-doxygen-doc-params: fn::expected::copack_error { args: "", title: "parameters" }

```cpp {title: "fn::expected< void, Err >::copack_error"}
constexpr auto copack_error() const &  -> expected<value_type, copack<error_type>>;  // (1)
constexpr auto copack_error() &&       -> expected<value_type, copack<error_type>>;  // (2)
constexpr auto copack_error() &        -> decltype(auto);                            // (3)
constexpr auto copack_error() const &  -> decltype(auto);                            // (4)
constexpr auto copack_error() &&       -> decltype(auto);                            // (5)
constexpr auto copack_error() const && -> decltype(auto);                            // (6)
```

:include-doxygen-doc: fn::expected< void, Err >::copack_error { args: "" }

:include-doxygen-doc-params: fn::expected< void, Err >::copack_error { args: "", title: "parameters" }

## copack_value {style: "api"}
The same lift, on the value side.

```cpp {title: "fn::expected::copack_value"}
constexpr auto copack_value() const &  -> expected<copack<value_type>, error_type>;  // (1)
constexpr auto copack_value() &&       -> expected<copack<value_type>, error_type>;  // (2)
constexpr auto copack_value() &        -> decltype(auto);                            // (3)
constexpr auto copack_value() const &  -> decltype(auto);                            // (4)
constexpr auto copack_value() &&       -> decltype(auto);                            // (5)
constexpr auto copack_value() const && -> decltype(auto);                            // (6)
```

:include-doxygen-doc: fn::expected::copack_value { args: "" }

:include-doxygen-doc-params: fn::expected::copack_value { args: "", title: "parameters" }

## and_then {style: "api"}

```cpp {title: "fn::expected::and_then"}
template <class F>
constexpr auto and_then(F &&f) &;         // (1)
constexpr auto and_then(F &&f) &&;        // (2)
constexpr auto and_then(F &&f) const &;   // (3)
constexpr auto and_then(F &&f) const &&;  // (4)
```

:include-doxygen-doc: fn::expected::and_then { args: "F &&" }

:include-doxygen-doc-params: fn::expected::and_then { args: "F &&", title: "parameters" }

```cpp {title: "fn::expected< void, Err >::and_then"}
template <class F>
constexpr auto and_then(F &&f) &;         // (1)
constexpr auto and_then(F &&f) &&;        // (2)
constexpr auto and_then(F &&f) const &;   // (3)
constexpr auto and_then(F &&f) const &&;  // (4)
```

:include-doxygen-doc: fn::expected< void, Err >::and_then { args: "F &&" }

:include-doxygen-doc-params: fn::expected< void, Err >::and_then { args: "F &&", title: "parameters" }

## or_else {style: "api"}

```cpp {title: "fn::expected::or_else"}
template <class F>
constexpr auto or_else(F &&f) &;         // (1)
constexpr auto or_else(F &&f) &&;        // (2)
constexpr auto or_else(F &&f) const &;   // (3)
constexpr auto or_else(F &&f) const &&;  // (4)
```

:include-doxygen-doc: fn::expected::or_else { args: "F &&" }

:include-doxygen-doc-params: fn::expected::or_else { args: "F &&", title: "parameters" }

```cpp {title: "fn::expected< void, Err >::or_else"}
template <class F>
constexpr auto or_else(F &&f) &;         // (1)
constexpr auto or_else(F &&f) &&;        // (2)
constexpr auto or_else(F &&f) const &;   // (3)
constexpr auto or_else(F &&f) const &&;  // (4)
```

:include-doxygen-doc: fn::expected< void, Err >::or_else { args: "F &&" }

:include-doxygen-doc-params: fn::expected< void, Err >::or_else { args: "F &&", title: "parameters" }

## transform {style: "api"}

```cpp {title: "fn::expected::transform"}
template <class F>
constexpr auto transform(F &&f) &;         // (1)
constexpr auto transform(F &&f) &&;        // (2)
constexpr auto transform(F &&f) const &;   // (3)
constexpr auto transform(F &&f) const &&;  // (4)
```

:include-doxygen-doc: fn::expected::transform { args: "F &&" }

:include-doxygen-doc-params: fn::expected::transform { args: "F &&", title: "parameters" }

```cpp {title: "fn::expected< void, Err >::transform"}
template <class F>
constexpr auto transform(F &&f) &;         // (1)
constexpr auto transform(F &&f) &&;        // (2)
constexpr auto transform(F &&f) const &;   // (3)
constexpr auto transform(F &&f) const &&;  // (4)
```

:include-doxygen-doc: fn::expected< void, Err >::transform { args: "F &&" }

:include-doxygen-doc-params: fn::expected< void, Err >::transform { args: "F &&", title: "parameters" }

## transform_error {style: "api"}

```cpp {title: "fn::expected::transform_error"}
template <class F>
constexpr auto transform_error(F &&f) &;         // (1)
constexpr auto transform_error(F &&f) &&;        // (2)
constexpr auto transform_error(F &&f) const &;   // (3)
constexpr auto transform_error(F &&f) const &&;  // (4)
```

:include-doxygen-doc: fn::expected::transform_error { args: "F &&" }

:include-doxygen-doc-params: fn::expected::transform_error { args: "F &&", title: "parameters" }

```cpp {title: "fn::expected< void, Err >::transform_error"}
template <class F>
constexpr auto transform_error(F &&f) &;         // (1)
constexpr auto transform_error(F &&f) &&;        // (2)
constexpr auto transform_error(F &&f) const &;   // (3)
constexpr auto transform_error(F &&f) const &&;  // (4)
```

:include-doxygen-doc: fn::expected< void, Err >::transform_error { args: "F &&" }

:include-doxygen-doc-params: fn::expected< void, Err >::transform_error { args: "F &&", title: "parameters" }

## apply {style: "api"}

```cpp {title: "fn::expected::apply"}
template <class F, class... Args>
constexpr auto apply(F &&f, Args &&...args) &;         // (1)
constexpr auto apply(F &&f, Args &&...args) &&;        // (2)
constexpr auto apply(F &&f, Args &&...args) const &;   // (3)
constexpr auto apply(F &&f, Args &&...args) const &&;  // (4)
```

:include-doxygen-doc: fn::expected::apply { args: "F &&, Args &&..." }

:include-doxygen-doc-params: fn::expected::apply { args: "F &&, Args &&...", title: "parameters" }

```cpp {title: "fn::expected< void, Err >::apply"}
template <class F, class... Args>
constexpr auto apply(F &&f, Args &&...args) &;         // (1)
constexpr auto apply(F &&f, Args &&...args) &&;        // (2)
constexpr auto apply(F &&f, Args &&...args) const &;   // (3)
constexpr auto apply(F &&f, Args &&...args) const &&;  // (4)
```

:include-doxygen-doc: fn::expected< void, Err >::apply { args: "F &&, Args &&..." }

:include-doxygen-doc-params: fn::expected< void, Err >::apply { args: "F &&, Args &&...", title: "parameters" }

## apply_r {style: "api"}

```cpp {title: "fn::expected::apply_r"}
template <class Ret, class F, class... Args>
constexpr auto apply_r(F &&f, Args &&...args) &;         // (1)
constexpr auto apply_r(F &&f, Args &&...args) &&;        // (2)
constexpr auto apply_r(F &&f, Args &&...args) const &;   // (3)
constexpr auto apply_r(F &&f, Args &&...args) const &&;  // (4)
```

:include-doxygen-doc: fn::expected::apply_r { args: "F &&, Args &&..." }

:include-doxygen-doc-params: fn::expected::apply_r { args: "F &&, Args &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::expected::apply_r { args: "F &&, Args &&...", title: "parameters" }

```cpp {title: "fn::expected< void, Err >::apply_r"}
template <class Ret, class F, class... Args>
constexpr auto apply_r(F &&f, Args &&...args) &;         // (1)
constexpr auto apply_r(F &&f, Args &&...args) &&;        // (2)
constexpr auto apply_r(F &&f, Args &&...args) const &;   // (3)
constexpr auto apply_r(F &&f, Args &&...args) const &&;  // (4)
```

:include-doxygen-doc: fn::expected< void, Err >::apply_r { args: "F &&, Args &&..." }

:include-doxygen-doc-params: fn::expected< void, Err >::apply_r { args: "F &&, Args &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::expected< void, Err >::apply_r { args: "F &&, Args &&...", title: "parameters" }

## apply_type {style: "api"}

```cpp {title: "fn::expected::apply_type"}
template <class F, class... Args>
constexpr auto apply_type(F &&f, Args &&...args) &;         // (1)
constexpr auto apply_type(F &&f, Args &&...args) &&;        // (2)
constexpr auto apply_type(F &&f, Args &&...args) const &;   // (3)
constexpr auto apply_type(F &&f, Args &&...args) const &&;  // (4)
```

:include-doxygen-doc: fn::expected::apply_type { args: "F &&, Args &&..." }

:include-doxygen-doc-params: fn::expected::apply_type { args: "F &&, Args &&...", title: "parameters" }

```cpp {title: "fn::expected< void, Err >::apply_type"}
template <class F, class... Args>
constexpr auto apply_type(F &&f, Args &&...args) &;         // (1)
constexpr auto apply_type(F &&f, Args &&...args) &&;        // (2)
constexpr auto apply_type(F &&f, Args &&...args) const &;   // (3)
constexpr auto apply_type(F &&f, Args &&...args) const &&;  // (4)
```

:include-doxygen-doc: fn::expected< void, Err >::apply_type { args: "F &&, Args &&..." }

:include-doxygen-doc-params: fn::expected< void, Err >::apply_type { args: "F &&, Args &&...", title: "parameters" }

## apply_type_r {style: "api"}

```cpp {title: "fn::expected::apply_type_r"}
template <class Ret, class F, class... Args>
constexpr auto apply_type_r(F &&f, Args &&...args) &;         // (1)
constexpr auto apply_type_r(F &&f, Args &&...args) &&;        // (2)
constexpr auto apply_type_r(F &&f, Args &&...args) const &;   // (3)
constexpr auto apply_type_r(F &&f, Args &&...args) const &&;  // (4)
```

:include-doxygen-doc: fn::expected::apply_type_r { args: "F &&, Args &&..." }

:include-doxygen-doc-params: fn::expected::apply_type_r { args: "F &&, Args &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::expected::apply_type_r { args: "F &&, Args &&...", title: "parameters" }

```cpp {title: "fn::expected< void, Err >::apply_type_r"}
template <class Ret, class F, class... Args>
constexpr auto apply_type_r(F &&f, Args &&...args) &;         // (1)
constexpr auto apply_type_r(F &&f, Args &&...args) &&;        // (2)
constexpr auto apply_type_r(F &&f, Args &&...args) const &;   // (3)
constexpr auto apply_type_r(F &&f, Args &&...args) const &&;  // (4)
```

:include-doxygen-doc: fn::expected< void, Err >::apply_type_r { args: "F &&, Args &&..." }

:include-doxygen-doc-params: fn::expected< void, Err >::apply_type_r { args: "F &&, Args &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::expected< void, Err >::apply_type_r { args: "F &&, Args &&...", title: "parameters" }
