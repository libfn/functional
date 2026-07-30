---
title: "monad fn::choice"
---

##### Defined in {style: "api", badge: "#include <fn/choice.hpp>"}

---

:include-doxygen-doc: fn::choice

## choice_for {style: "api"}
The construction alias: accepts alternatives in any order, with duplicates and nested copacks,
and resolves to the canonical `fn::choice`. Prefer it over spelling `choice` directly, so that no
spelling in your project is tied to one compiler's alternative order.

```cpp {title: "fn::choice_for"}
template <typename... Ts>
using choice_for = detail::_collapsing_copack::normalized<::fn::choice, detail::_collapsing_copack::flattened<Ts...>>::type;  // (1)
```

:include-doxygen-doc: fn::choice_for { args: "" }

:include-doxygen-doc-params: fn::choice_for { args: "", type: "template", title: "template parameters" }

## choice {style: "api"}
Construction: from a value of one alternative, in place from arguments, or widening from a
`copack` over a subset of the alternatives.

```cpp {title: "fn::choice"}
template <typename T>
choice(std::in_place_type_t<T>, auto &&...) -> choice<T>;  // (1)
choice(T) -> choice<T>;                                    // (2)
```

```cpp {title: "fn::choice< Ts... >::choice"}
template <typename T>
constexpr choice(T &&v);                                               // (1)
constexpr explicit choice(T &&v);                                      // (2)
constexpr explicit choice(std::in_place_type_t<T> d, auto &&...args);  // (3)

template <typename... Tx>
constexpr choice(copack<Tx...> const &v);                                     // (4)
constexpr choice(copack<Tx...> &&v);                                          // (5)
constexpr choice(std::in_place_type_t<copack<Tx...>>, some_copack auto &&v);  // (6)

constexpr choice(choice const &other) = default;  // (7)
constexpr choice(choice &&other) = default;       // (8)
```

:include-doxygen-doc: fn::choice< Ts... >::choice { args: "T &&" }

:include-doxygen-doc-params: fn::choice< Ts... >::choice { args: "T &&", title: "parameters" }

:include-doxygen-doc: fn::choice< Ts... >::choice { args: "::std::in_place_type_t< T >, auto &&..." }

:include-doxygen-doc-params: fn::choice< Ts... >::choice { args: "::std::in_place_type_t< T >, auto &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::choice< Ts... >::choice { args: "::std::in_place_type_t< T >, auto &&...", title: "parameters" }

:include-doxygen-doc: fn::choice< Ts... >::choice { args: "copack < Tx... > const &" }

:include-doxygen-doc-params: fn::choice< Ts... >::choice { args: "copack < Tx... > const &", title: "parameters" }

:include-doxygen-doc: fn::choice< Ts... >::choice { args: "::std::in_place_type_t< copack < Tx... > >, some_copack auto &&" }

:include-doxygen-doc-params: fn::choice< Ts... >::choice { args: "::std::in_place_type_t< copack < Tx... > >, some_copack auto &&", title: "parameters" }
## value {style: "api"}
The alternatives as the underlying `copack`, always present.

```cpp {title: "fn::choice< Ts... >::value"}
constexpr auto value() &        -> value_type &;         // (1)
constexpr auto value() const &  -> value_type const &;   // (2)
constexpr auto value() &&       -> value_type &&;        // (3)
constexpr auto value() const && -> value_type const &&;  // (4)
```

:include-doxygen-doc: fn::choice< Ts... >::value { args: "" }

:include-doxygen-doc-params: fn::choice< Ts... >::value { args: "", title: "parameters" }

## and_then {style: "api"}

```cpp {title: "fn::choice< Ts... >::and_then"}
template <typename Fn>
constexpr auto and_then(Fn &&fn) &;         // (1)
constexpr auto and_then(Fn &&fn) const &;   // (2)
constexpr auto and_then(Fn &&fn) &&;        // (3)
constexpr auto and_then(Fn &&fn) const &&;  // (4)
```

:include-doxygen-doc: fn::choice< Ts... >::and_then { args: "Fn &&" }

:include-doxygen-doc-params: fn::choice< Ts... >::and_then { args: "Fn &&", title: "parameters" }

## transform {style: "api"}

```cpp {title: "fn::choice< Ts... >::transform"}
template <typename Fn>
constexpr auto transform(Fn &&fn) &;         // (1)
constexpr auto transform(Fn &&fn) const &;   // (2)
constexpr auto transform(Fn &&fn) &&;        // (3)
constexpr auto transform(Fn &&fn) const &&;  // (4)
```

:include-doxygen-doc: fn::choice< Ts... >::transform { args: "Fn &&" }

:include-doxygen-doc-params: fn::choice< Ts... >::transform { args: "Fn &&", title: "parameters" }

## apply {style: "api"}

```cpp {title: "fn::choice< Ts... >::apply"}
template <typename Fn, typename... Args>
constexpr auto apply(Fn &&fn, Args &&...args) &;         // (1)
constexpr auto apply(Fn &&fn, Args &&...args) const &;   // (2)
constexpr auto apply(Fn &&fn, Args &&...args) &&;        // (3)
constexpr auto apply(Fn &&fn, Args &&...args) const &&;  // (4)
```

:include-doxygen-doc: fn::choice< Ts... >::apply { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::choice< Ts... >::apply { args: "Fn &&, Args &&...", title: "parameters" }

## apply_r {style: "api"}

```cpp {title: "fn::choice< Ts... >::apply_r"}
template <typename T, typename Fn, typename... Args>
constexpr auto apply_r(Fn &&fn, Args &&...args) &;         // (1)
constexpr auto apply_r(Fn &&fn, Args &&...args) const &;   // (2)
constexpr auto apply_r(Fn &&fn, Args &&...args) &&;        // (3)
constexpr auto apply_r(Fn &&fn, Args &&...args) const &&;  // (4)
```

:include-doxygen-doc: fn::choice< Ts... >::apply_r { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::choice< Ts... >::apply_r { args: "Fn &&, Args &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::choice< Ts... >::apply_r { args: "Fn &&, Args &&...", title: "parameters" }

## apply_type {style: "api"}

```cpp {title: "fn::choice< Ts... >::apply_type"}
template <typename Fn, typename... Args>
constexpr auto apply_type(Fn &&fn, Args &&...args) &;         // (1)
constexpr auto apply_type(Fn &&fn, Args &&...args) const &;   // (2)
constexpr auto apply_type(Fn &&fn, Args &&...args) &&;        // (3)
constexpr auto apply_type(Fn &&fn, Args &&...args) const &&;  // (4)
```

:include-doxygen-doc: fn::choice< Ts... >::apply_type { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::choice< Ts... >::apply_type { args: "Fn &&, Args &&...", title: "parameters" }

## apply_type_r {style: "api"}

```cpp {title: "fn::choice< Ts... >::apply_type_r"}
template <typename Ret, typename Fn, typename... Args>
constexpr auto apply_type_r(Fn &&fn, Args &&...args) &        -> Ret;  // (1)
constexpr auto apply_type_r(Fn &&fn, Args &&...args) const &  -> Ret;  // (2)
constexpr auto apply_type_r(Fn &&fn, Args &&...args) &&       -> Ret;  // (3)
constexpr auto apply_type_r(Fn &&fn, Args &&...args) const && -> Ret;  // (4)
```

:include-doxygen-doc: fn::choice< Ts... >::apply_type_r { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::choice< Ts... >::apply_type_r { args: "Fn &&, Args &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::choice< Ts... >::apply_type_r { args: "Fn &&, Args &&...", title: "parameters" }
