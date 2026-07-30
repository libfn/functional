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
