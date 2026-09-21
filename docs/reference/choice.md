---
title: "monad fn::choice"
---

##### Defined in {style: "api", badge: "#include <fn/just.hpp>"}

---

:include-doxygen-doc: fn::just< copack< Ts... > >

:include-doxygen-doc: fn::choice

## Member types {style: "api"}

```cpp {title: "fn::just< copack< Ts... > >::value_type"}
using value_type = copack<Ts...>;  // (1)
```

:include-doxygen-doc: fn::just< copack< Ts... > >::value_type { args: "" }

```cpp {title: "fn::just< copack< Ts... > >::v_"}
value_type v_;  // (1)
```

:include-doxygen-doc: fn::just< copack< Ts... > >::v_ { args: "" }

```cpp {title: "fn::just< copack< Ts... > >::size"}
static std::size_t size = sizeof...(Ts);  // (1)
```

:include-doxygen-doc: fn::just< copack< Ts... > >::size { args: "" }

```cpp {title: "fn::just< copack< Ts... > >::has_type"}
template <typename T>
static constexpr bool has_type = value_type::template has_type<T>;  // (1)
```

:include-doxygen-doc: fn::just< copack< Ts... > >::has_type { args: "" }

```cpp {title: "fn::just< copack< Ts... > >::select_nth"}
template <std::size_t I>
using select_nth = detail::select_nth_t<I, Ts...>;  // (1)
```

:include-doxygen-doc: fn::just< copack< Ts... > >::select_nth { args: "" }

## Construction {style: "api"}

From a value of one alternative, in place from arguments, widening from a `copack` over a
subset of the alternatives, or from a narrower `choice`.

```cpp {title: "fn::just< copack< Ts... > >::just"}
template <typename T>
constexpr just(T &&v);                                               // (1)
constexpr explicit just(T &&v);                                      // (2)
constexpr explicit just(std::in_place_type_t<T> d, auto &&...args);  // (3)

template <typename... Tx>
constexpr just(copack<Tx...> const &v);  // (4)
constexpr just(copack<Tx...> &&v);       // (5)

template <typename... Tx, some_copack V>
constexpr just(std::in_place_type_t<copack<Tx...>> d, V &&v);  // (6)

template <typename... Tx>
constexpr just(just<copack<Tx...>> const &other);  // (7)
constexpr just(just<copack<Tx...>> &&other);       // (8)

constexpr just(just const &) = default;  // (9)
constexpr just(just &&) = default;       // (10)
```

:include-doxygen-doc: fn::just< copack< Ts... > >::just { args: "T &&" }

:include-doxygen-doc-params: fn::just< copack< Ts... > >::just { args: "T &&", title: "parameters" }

:include-doxygen-doc: fn::just< copack< Ts... > >::just { args: "::std::in_place_type_t< T >, auto &&..." }

:include-doxygen-doc-params: fn::just< copack< Ts... > >::just { args: "::std::in_place_type_t< T >, auto &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::just< copack< Ts... > >::just { args: "::std::in_place_type_t< T >, auto &&...", title: "parameters" }

:include-doxygen-doc: fn::just< copack< Ts... > >::just { args: "copack < Tx... > const &" }

:include-doxygen-doc-params: fn::just< copack< Ts... > >::just { args: "copack < Tx... > const &", title: "parameters" }

:include-doxygen-doc: fn::just< copack< Ts... > >::just { args: "copack < Tx... > &&" }

:include-doxygen-doc: fn::just< copack< Ts... > >::just { args: "::std::in_place_type_t< copack < Tx... > >, V &&" }

:include-doxygen-doc-params: fn::just< copack< Ts... > >::just { args: "::std::in_place_type_t< copack < Tx... > >, V &&", title: "parameters" }

:include-doxygen-doc: fn::just< copack< Ts... > >::just { args: "just < copack < Tx... > > const &" }

:include-doxygen-doc-params: fn::just< copack< Ts... > >::just { args: "just < copack < Tx... > > const &", title: "parameters" }

:include-doxygen-doc: fn::just< copack< Ts... > >::just { args: "just < copack < Tx... > > &&" }

:include-doxygen-doc: fn::just< copack< Ts... > >::just { args: "just const &" }

:include-doxygen-doc: fn::just< copack< Ts... > >::just { args: "just &&" }

## Destructor {style: "api"}

```cpp {title: "fn::just< copack< Ts... > >::~just"}
constexpr ~just() = default;  // (1)
```

:include-doxygen-doc: fn::just< copack< Ts... > >::~just { args: "" }

## Assignment {style: "api"}

```cpp {title: "fn::just< copack< Ts... > >::operator="}
constexpr auto operator=(just const &) = default -> just &;  // (1)
constexpr auto operator=(just &&) = default      -> just &;  // (2)

template <typename... Tx>
constexpr auto operator=(copack<Tx...> const &arg)       -> just &;  // (3)
constexpr auto operator=(copack<Tx...> &&arg)            -> just &;  // (4)
constexpr auto operator=(just<copack<Tx...>> const &arg) -> just &;  // (5)
constexpr auto operator=(just<copack<Tx...>> &&arg)      -> just &;  // (6)

template <typename U>
constexpr auto operator=(U &&v) -> just &;  // (7)
```

:include-doxygen-doc: fn::just< copack< Ts... > >::operator= { args: "just const &" }

:include-doxygen-doc: fn::just< copack< Ts... > >::operator= { args: "just &&" }

:include-doxygen-doc: fn::just< copack< Ts... > >::operator= { args: "copack < Tx... > const &" }

:include-doxygen-doc-params: fn::just< copack< Ts... > >::operator= { args: "copack < Tx... > const &", title: "parameters" }

:include-doxygen-doc: fn::just< copack< Ts... > >::operator= { args: "copack < Tx... > &&" }

:include-doxygen-doc: fn::just< copack< Ts... > >::operator= { args: "just < copack < Tx... > > const &" }

:include-doxygen-doc-params: fn::just< copack< Ts... > >::operator= { args: "just < copack < Tx... > > const &", title: "parameters" }

:include-doxygen-doc: fn::just< copack< Ts... > >::operator= { args: "just < copack < Tx... > > &&" }

:include-doxygen-doc: fn::just< copack< Ts... > >::operator= { args: "U &&" }

:include-doxygen-doc-params: fn::just< copack< Ts... > >::operator= { args: "U &&", title: "parameters" }

## emplace {style: "api"}

```cpp {title: "fn::just< copack< Ts... > >::emplace"}
template <typename T>
constexpr auto emplace(auto &&...args) -> T &;  // (1)
```

:include-doxygen-doc: fn::just< copack< Ts... > >::emplace { args: "auto &&..." }

:include-doxygen-doc-params: fn::just< copack< Ts... > >::emplace { args: "auto &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::just< copack< Ts... > >::emplace { args: "auto &&...", title: "parameters" }

## has_value {style: "api"}

```cpp {title: "fn::just< copack< Ts... > >::has_value"}
template <typename T>
constexpr auto has_value(std::in_place_type_t<T> d=std::in_place_type<T>) const -> bool;  // (1)
```

:include-doxygen-doc: fn::just< copack< Ts... > >::has_value { args: "::std::in_place_type_t< T >" }

:include-doxygen-doc-params: fn::just< copack< Ts... > >::has_value { args: "::std::in_place_type_t< T >", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::just< copack< Ts... > >::has_value { args: "::std::in_place_type_t< T >", title: "parameters" }

## get_ptr {style: "api"}

```cpp {title: "fn::just< copack< Ts... > >::get_ptr"}
template <typename T>
constexpr auto get_ptr(std::in_place_type_t<T> d=std::in_place_type<T>)       -> T *;        // (1)
constexpr auto get_ptr(std::in_place_type_t<T> d=std::in_place_type<T>) const -> T const *;  // (2)
```

:include-doxygen-doc: fn::just< copack< Ts... > >::get_ptr { args: "::std::in_place_type_t< T >" }

:include-doxygen-doc-params: fn::just< copack< Ts... > >::get_ptr { args: "::std::in_place_type_t< T >", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::just< copack< Ts... > >::get_ptr { args: "::std::in_place_type_t< T >", title: "parameters" }

## value {style: "api"}

The alternatives as the underlying `copack`, always present.

```cpp {title: "fn::just< copack< Ts... > >::value"}
constexpr auto value() &        -> value_type &;         // (1)
constexpr auto value() const &  -> value_type const &;   // (2)
constexpr auto value() &&       -> value_type &&;        // (3)
constexpr auto value() const && -> value_type const &&;  // (4)
```

:include-doxygen-doc: fn::just< copack< Ts... > >::value { args: "" }

:include-doxygen-doc-params: fn::just< copack< Ts... > >::value { args: "", title: "parameters" }

## and_then {style: "api"}

```cpp {title: "fn::just< copack< Ts... > >::and_then"}
template <typename Fn>
constexpr auto and_then(Fn &&fn) &;         // (1)
constexpr auto and_then(Fn &&fn) const &;   // (2)
constexpr auto and_then(Fn &&fn) &&;        // (3)
constexpr auto and_then(Fn &&fn) const &&;  // (4)
```

:include-doxygen-doc: fn::just< copack< Ts... > >::and_then { args: "Fn &&" }

:include-doxygen-doc-params: fn::just< copack< Ts... > >::and_then { args: "Fn &&", title: "parameters" }

## transform {style: "api"}

```cpp {title: "fn::just< copack< Ts... > >::transform"}
template <typename Fn>
constexpr auto transform(Fn &&fn) &;         // (1)
constexpr auto transform(Fn &&fn) const &;   // (2)
constexpr auto transform(Fn &&fn) &&;        // (3)
constexpr auto transform(Fn &&fn) const &&;  // (4)
```

:include-doxygen-doc: fn::just< copack< Ts... > >::transform { args: "Fn &&" }

:include-doxygen-doc-params: fn::just< copack< Ts... > >::transform { args: "Fn &&", title: "parameters" }

## apply {style: "api"}

```cpp {title: "fn::just< copack< Ts... > >::apply"}
template <typename Fn, typename... Args>
constexpr auto apply(Fn &&fn, Args &&...args) &        -> decltype(auto);  // (1)
constexpr auto apply(Fn &&fn, Args &&...args) const &  -> decltype(auto);  // (2)
constexpr auto apply(Fn &&fn, Args &&...args) &&       -> decltype(auto);  // (3)
constexpr auto apply(Fn &&fn, Args &&...args) const && -> decltype(auto);  // (4)
```

:include-doxygen-doc: fn::just< copack< Ts... > >::apply { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::just< copack< Ts... > >::apply { args: "Fn &&, Args &&...", title: "parameters" }

## apply_r {style: "api"}

```cpp {title: "fn::just< copack< Ts... > >::apply_r"}
template <typename Ret, typename Fn, typename... Args>
constexpr auto apply_r(Fn &&fn, Args &&...args) &        -> Ret;  // (1)
constexpr auto apply_r(Fn &&fn, Args &&...args) const &  -> Ret;  // (2)
constexpr auto apply_r(Fn &&fn, Args &&...args) &&       -> Ret;  // (3)
constexpr auto apply_r(Fn &&fn, Args &&...args) const && -> Ret;  // (4)
```

:include-doxygen-doc: fn::just< copack< Ts... > >::apply_r { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::just< copack< Ts... > >::apply_r { args: "Fn &&, Args &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::just< copack< Ts... > >::apply_r { args: "Fn &&, Args &&...", title: "parameters" }

## apply_type {style: "api"}

```cpp {title: "fn::just< copack< Ts... > >::apply_type"}
template <typename Fn, typename... Args>
constexpr auto apply_type(Fn &&fn, Args &&...args) &        -> decltype(auto);  // (1)
constexpr auto apply_type(Fn &&fn, Args &&...args) const &  -> decltype(auto);  // (2)
constexpr auto apply_type(Fn &&fn, Args &&...args) &&       -> decltype(auto);  // (3)
constexpr auto apply_type(Fn &&fn, Args &&...args) const && -> decltype(auto);  // (4)
```

:include-doxygen-doc: fn::just< copack< Ts... > >::apply_type { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::just< copack< Ts... > >::apply_type { args: "Fn &&, Args &&...", title: "parameters" }

## apply_type_r {style: "api"}

```cpp {title: "fn::just< copack< Ts... > >::apply_type_r"}
template <typename Ret, typename Fn, typename... Args>
constexpr auto apply_type_r(Fn &&fn, Args &&...args) &        -> Ret;  // (1)
constexpr auto apply_type_r(Fn &&fn, Args &&...args) const &  -> Ret;  // (2)
constexpr auto apply_type_r(Fn &&fn, Args &&...args) &&       -> Ret;  // (3)
constexpr auto apply_type_r(Fn &&fn, Args &&...args) const && -> Ret;  // (4)
```

:include-doxygen-doc: fn::just< copack< Ts... > >::apply_type_r { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::just< copack< Ts... > >::apply_type_r { args: "Fn &&, Args &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::just< copack< Ts... > >::apply_type_r { args: "Fn &&, Args &&...", title: "parameters" }

## choice_for {style: "api"}

The construction alias: accepts alternatives in any order, with duplicates and nested copacks,
and resolves to the canonical `fn::choice`. Prefer it over spelling `choice` directly, so that no
spelling in your project is tied to one compiler's alternative order.

```cpp {title: "fn::choice_for"}
template <typename... Ts>
using choice_for = just<copack_for<Ts...>>;  // (1)
```

:include-doxygen-doc: fn::choice_for { args: "" }

:include-doxygen-doc-params: fn::choice_for { args: "", type: "template", title: "template parameters" }

## as_choice {style: "api"}

`as_choice(x)` constructs a choice without explicit template arguments. A value becomes a
single alternative of its decayed type; a copack becomes the payload of a choice
over its alternatives. Use this function for bare values, which the current `choice` deduction
guides do not support.

```cpp {title: "fn::as_choice"}
constexpr auto as_choice(auto &&src) -> decltype(auto);  // (1)

template <typename Src>
constexpr auto as_choice(Src &&src) -> decltype(auto);  // (2)

template <typename T>
constexpr auto as_choice(std::in_place_type_t<T>, auto &&...args) -> decltype(auto);  // (3)
```

:include-doxygen-doc: fn::as_choice { args: "auto &&" }

:include-doxygen-doc-params: fn::as_choice { args: "auto &&", title: "parameters" }

:include-doxygen-doc: fn::as_choice { args: "Src &&" }

:include-doxygen-doc-params: fn::as_choice { args: "Src &&", title: "parameters" }

:include-doxygen-doc: fn::as_choice { args: "::std::in_place_type_t< T >, auto &&..." }

:include-doxygen-doc-params: fn::as_choice { args: "::std::in_place_type_t< T >, auto &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::as_choice { args: "::std::in_place_type_t< T >, auto &&...", title: "parameters" }
