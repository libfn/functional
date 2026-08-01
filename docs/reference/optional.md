---
title: "monad fn::optional"
---

##### Defined in {style: "api", badge: "#include <fn/optional.hpp>"}

---

:include-doxygen-doc: fn::optional

## Member types {style: "api"}

```cpp {title: "fn::optional::value_type"}
using value_type = T;  // (1)
```

:include-doxygen-doc: fn::optional::value_type { args: "" }

```cpp {title: "fn::optional::iterator"}
using iterator = detail::_optional_iterator<T>;  // (1)
```

:include-doxygen-doc: fn::optional::iterator { args: "" }

```cpp {title: "fn::optional::const_iterator"}
using const_iterator = detail::_optional_iterator<T const>;  // (1)
```

:include-doxygen-doc: fn::optional::const_iterator { args: "" }

```cpp {title: "fn::optional< T & >::value_type"}
using value_type = T;  // (1)
```

:include-doxygen-doc: fn::optional< T & >::value_type { args: "" }

```cpp {title: "fn::optional< T & >::iterator"}
using iterator = detail::_optional_iterator<T>;  // (1)
```

:include-doxygen-doc: fn::optional< T & >::iterator { args: "" }

## Construction {style: "api"}

```cpp {title: "fn::optional::optional"}
constexpr optional();                // (1)
constexpr optional(std::nullopt_t);  // (2)

template <class U>
constexpr explicit optional(optional<U> const &s);  // (3)
constexpr explicit optional(optional<U> &&s);       // (4)
constexpr explicit optional(U &&v);                 // (5)

template <class... Args>
constexpr explicit optional(std::in_place_t, Args &&...a);  // (6)

template <class U, class... Args>
constexpr explicit optional(std::in_place_t, std::initializer_list<U> il, Args &&...a);  // (7)

constexpr optional(optional const &) = delete;       // (8)
constexpr optional(optional const &s) = default;     // (9)
constexpr optional(optional const &s);               // (10)
constexpr optional(optional &&) noexcept = default;  // (11)
constexpr optional(optional &&s);                    // (12)

template <class Fn, class... Args>
constexpr explicit optional(::pfn::detail::_optional_from_invoke_t tag, Fn &&fn, Args &&...args);  // (13)
```

:include-doxygen-doc: fn::optional::optional { args: "" }

:include-doxygen-doc: fn::optional::optional { args: "::std::nullopt_t" }

:include-doxygen-doc: fn::optional::optional { args: "optional < U > const &" }

:include-doxygen-doc: fn::optional::optional { args: "optional < U > &&" }

:include-doxygen-doc: fn::optional::optional { args: "U &&" }

:include-doxygen-doc: fn::optional::optional { args: "::std::in_place_t, Args &&..." }

:include-doxygen-doc: fn::optional::optional { args: "::std::in_place_t, ::std::initializer_list< U >, Args &&..." }

:include-doxygen-doc: fn::optional::optional { args: "optional const &" }

:include-doxygen-doc: fn::optional::optional { args: "optional &&" }

```cpp {title: "fn::optional< T & >::optional"}
constexpr optional() noexcept = default;                     // (1)
constexpr optional(std::nullopt_t);                          // (2)
constexpr optional(optional const &rhs) noexcept = default;  // (3)

template <class Arg>
constexpr explicit optional(std::in_place_t, Arg &&arg);  // (4)

template <class U>
constexpr explicit optional(U &&u);                    // (5)
constexpr explicit optional(optional<U> &rhs);         // (6)
constexpr explicit optional(optional<U> const &rhs);   // (7)
constexpr explicit optional(optional<U> &&rhs);        // (8)
constexpr explicit optional(optional<U> const &&rhs);  // (9)

template <class Fn, class... Args>
constexpr explicit optional(::pfn::detail::_optional_from_invoke_t tag, Fn &&fn, Args &&...args);  // (10)
```

:include-doxygen-doc: fn::optional< T & >::optional { args: "" }

:include-doxygen-doc: fn::optional< T & >::optional { args: "::std::nullopt_t" }

:include-doxygen-doc: fn::optional< T & >::optional { args: "optional const &" }

:include-doxygen-doc: fn::optional< T & >::optional { args: "::std::in_place_t, Arg &&" }

:include-doxygen-doc: fn::optional< T & >::optional { args: "U &&" }

:include-doxygen-doc: fn::optional< T & >::optional { args: "optional < U > &" }

:include-doxygen-doc: fn::optional< T & >::optional { args: "optional < U > const &" }

:include-doxygen-doc: fn::optional< T & >::optional { args: "optional < U > &&" }

:include-doxygen-doc: fn::optional< T & >::optional { args: "optional < U > const &&" }

## Destructor {style: "api"}

```cpp {title: "fn::optional::~optional"}
constexpr ~optional() = default;  // (1)
```

:include-doxygen-doc: fn::optional::~optional { args: "" }

```cpp {title: "fn::optional< T & >::~optional"}
constexpr ~optional() = default;  // (1)
```

:include-doxygen-doc: fn::optional< T & >::~optional { args: "" }

## Assignment {style: "api"}

```cpp {title: "fn::optional::operator="}
constexpr auto operator=(std::nullopt_t)             -> optional &;  // (1)
constexpr auto operator=(optional const &) = delete  -> optional &;  // (2)
constexpr auto operator=(optional const &) = default -> optional &;  // (3)
constexpr auto operator=(optional const &s)          -> optional &;  // (4)
constexpr auto operator=(optional &&) = default      -> optional &;  // (5)
constexpr auto operator=(optional &&s)               -> optional &;  // (6)

template <class U>
constexpr auto operator=(U &&v)                -> optional &;  // (7)
constexpr auto operator=(optional<U> const &s) -> optional &;  // (8)
constexpr auto operator=(optional<U> &&s)      -> optional &;  // (9)
```

:include-doxygen-doc: fn::optional::operator= { args: "::std::nullopt_t" }

:include-doxygen-doc: fn::optional::operator= { args: "optional const &" }

:include-doxygen-doc: fn::optional::operator= { args: "optional &&" }

:include-doxygen-doc: fn::optional::operator= { args: "U &&" }

:include-doxygen-doc: fn::optional::operator= { args: "optional < U > const &" }

:include-doxygen-doc: fn::optional::operator= { args: "optional < U > &&" }

```cpp {title: "fn::optional< T & >::operator="}
constexpr auto operator=(std::nullopt_t)                         -> optional &;  // (1)
constexpr auto operator=(optional const &rhs) noexcept = default -> optional &;  // (2)
```

:include-doxygen-doc: fn::optional< T & >::operator= { args: "::std::nullopt_t" }

:include-doxygen-doc: fn::optional< T & >::operator= { args: "optional const &" }

## swap {style: "api"}

```cpp {title: "fn::optional::swap"}
constexpr auto swap(optional &rhs) -> void;  // (1)
```

:include-doxygen-doc: fn::optional::swap { args: "optional &" }

```cpp {title: "fn::optional< T & >::swap"}
constexpr auto swap(optional &rhs) -> void;  // (1)
```

:include-doxygen-doc: fn::optional< T & >::swap { args: "optional &" }

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
