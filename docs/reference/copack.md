---
title: "type fn::copack"
---

##### Defined in {style: "api", badge: "#include <fn/copack.hpp>"}

---

:include-doxygen-doc: fn::copack

:include-doxygen-doc: fn::copack_for

## Apply {style: "api"}
Elimination: the active alternative routes into the callable, exhaustively - every alternative
must have a viable arm.

```cpp {title: "fn::copack< Ts... >::apply"}
template <typename Fn, typename... Args>
constexpr auto apply(Fn &&fn, Args &&...args) &;         // (1)
constexpr auto apply(Fn &&fn, Args &&...args) const &;   // (2)
constexpr auto apply(Fn &&fn, Args &&...args) &&;        // (3)
constexpr auto apply(Fn &&fn, Args &&...args) const &&;  // (4)
```

:include-doxygen-doc: fn::copack< Ts... >::apply { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::copack< Ts... >::apply { args: "Fn &&, Args &&...", title: "parameters" }

## Transform {style: "api"}
The self-flattening map over the alternatives: the branch results form a new normalized copack.

```cpp {title: "fn::copack< Ts... >::transform"}
template <typename Fn, typename... Args>
constexpr auto transform(Fn &&fn, Args &&...args) &;         // (1)
constexpr auto transform(Fn &&fn, Args &&...args) const &;   // (2)
constexpr auto transform(Fn &&fn, Args &&...args) &&;        // (3)
constexpr auto transform(Fn &&fn, Args &&...args) const &&;  // (4)
```

:include-doxygen-doc: fn::copack< Ts... >::transform { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::copack< Ts... >::transform { args: "Fn &&, Args &&...", title: "parameters" }

## apply_r {style: "api"}

```cpp {title: "fn::copack< Ts... >::apply_r"}
template <typename Ret, typename Fn, typename... Args>
constexpr auto apply_r(Fn &&fn, Args &&...args) &        -> Ret;  // (1)
constexpr auto apply_r(Fn &&fn, Args &&...args) const &  -> Ret;  // (2)
constexpr auto apply_r(Fn &&fn, Args &&...args) &&       -> Ret;  // (3)
constexpr auto apply_r(Fn &&fn, Args &&...args) const && -> Ret;  // (4)
```

:include-doxygen-doc: fn::copack< Ts... >::apply_r { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::copack< Ts... >::apply_r { args: "Fn &&, Args &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::copack< Ts... >::apply_r { args: "Fn &&, Args &&...", title: "parameters" }

## apply_type {style: "api"}

```cpp {title: "fn::copack< Ts... >::apply_type"}
template <typename Fn, typename... Args>
constexpr auto apply_type(Fn &&fn, Args &&...args) &;         // (1)
constexpr auto apply_type(Fn &&fn, Args &&...args) const &;   // (2)
constexpr auto apply_type(Fn &&fn, Args &&...args) &&;        // (3)
constexpr auto apply_type(Fn &&fn, Args &&...args) const &&;  // (4)
```

:include-doxygen-doc: fn::copack< Ts... >::apply_type { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::copack< Ts... >::apply_type { args: "Fn &&, Args &&...", title: "parameters" }

## apply_type_r {style: "api"}

```cpp {title: "fn::copack< Ts... >::apply_type_r"}
template <typename Ret, typename Fn, typename... Args>
constexpr auto apply_type_r(Fn &&fn, Args &&...args) &        -> Ret;  // (1)
constexpr auto apply_type_r(Fn &&fn, Args &&...args) const &  -> Ret;  // (2)
constexpr auto apply_type_r(Fn &&fn, Args &&...args) &&       -> Ret;  // (3)
constexpr auto apply_type_r(Fn &&fn, Args &&...args) const && -> Ret;  // (4)
```

:include-doxygen-doc: fn::copack< Ts... >::apply_type_r { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::copack< Ts... >::apply_type_r { args: "Fn &&, Args &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::copack< Ts... >::apply_type_r { args: "Fn &&, Args &&...", title: "parameters" }

## copack {style: "api"}

```cpp {title: "fn::copack< Ts... >::copack"}
template <typename T>
constexpr copack(T &&v);                                             // (1)
constexpr explicit copack(T &&v);                                    // (2)
constexpr explicit copack(std::in_place_type_t<T>, auto &&...args);  // (3)

template <typename... Tx>
constexpr copack(copack<Tx...> const &arg);                                     // (4)
constexpr copack(copack<Tx...> &&arg);                                          // (5)
constexpr copack(std::in_place_type_t<copack<Tx...>>, some_copack auto &&arg);  // (6)

constexpr copack(copack const &other) = default;  // (7)
constexpr copack(copack const &other);            // (8)
constexpr copack(copack &&other) = default;       // (9)
constexpr copack(copack &&other);                 // (10)
```

:include-doxygen-doc: fn::copack< Ts... >::copack { args: "T &&" }

:include-doxygen-doc-params: fn::copack< Ts... >::copack { args: "T &&", title: "parameters" }

:include-doxygen-doc: fn::copack< Ts... >::copack { args: "::std::in_place_type_t< T >, auto &&..." }

:include-doxygen-doc-params: fn::copack< Ts... >::copack { args: "::std::in_place_type_t< T >, auto &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::copack< Ts... >::copack { args: "::std::in_place_type_t< T >, auto &&...", title: "parameters" }

:include-doxygen-doc: fn::copack< Ts... >::copack { args: "copack < Tx... > const &" }

:include-doxygen-doc-params: fn::copack< Ts... >::copack { args: "copack < Tx... > const &", title: "parameters" }

:include-doxygen-doc: fn::copack< Ts... >::copack { args: "::std::in_place_type_t< copack < Tx... > >, some_copack auto &&" }

:include-doxygen-doc-params: fn::copack< Ts... >::copack { args: "::std::in_place_type_t< copack < Tx... > >, some_copack auto &&", title: "parameters" }

:include-doxygen-doc: fn::copack< Ts... >::copack { args: "copack const &" }

:include-doxygen-doc-params: fn::copack< Ts... >::copack { args: "copack const &", title: "parameters" }

:include-doxygen-doc: fn::copack< Ts... >::copack { args: "copack &&" }

:include-doxygen-doc-params: fn::copack< Ts... >::copack { args: "copack &&", title: "parameters" }

## emplace {style: "api"}

```cpp {title: "fn::copack< Ts... >::emplace"}
template <typename T>
constexpr auto emplace(auto &&...args) -> T &;  // (1)
```

:include-doxygen-doc: fn::copack< Ts... >::emplace { args: "auto &&..." }

:include-doxygen-doc-params: fn::copack< Ts... >::emplace { args: "auto &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::copack< Ts... >::emplace { args: "auto &&...", title: "parameters" }

## get_ptr {style: "api"}

```cpp {title: "fn::copack< Ts... >::get_ptr"}
template <typename T>
constexpr auto get_ptr(std::in_place_type_t<T>=std::in_place_type<T>)       -> T *;        // (1)
constexpr auto get_ptr(std::in_place_type_t<T>=std::in_place_type<T>) const -> T const *;  // (2)
```

:include-doxygen-doc: fn::copack< Ts... >::get_ptr { args: "::std::in_place_type_t< T >" }

:include-doxygen-doc-params: fn::copack< Ts... >::get_ptr { args: "::std::in_place_type_t< T >", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::copack< Ts... >::get_ptr { args: "::std::in_place_type_t< T >", title: "parameters" }

## has_type {style: "api"}

```cpp {title: "fn::copack< Ts... >::has_type"}
template <typename T>
static constexpr auto has_type -> bool;  // (1)
```

:include-doxygen-doc: fn::copack< Ts... >::has_type { args: "" }

:include-doxygen-doc-params: fn::copack< Ts... >::has_type { args: "", type: "template", title: "template parameters" }

## has_value {style: "api"}

```cpp {title: "fn::copack< Ts... >::has_value"}
template <typename T>
constexpr auto has_value(std::in_place_type_t<T>=std::in_place_type<T>) const -> bool;  // (1)
```

:include-doxygen-doc: fn::copack< Ts... >::has_value { args: "::std::in_place_type_t< T >" }

:include-doxygen-doc-params: fn::copack< Ts... >::has_value { args: "::std::in_place_type_t< T >", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::copack< Ts... >::has_value { args: "::std::in_place_type_t< T >", title: "parameters" }

## operator= {style: "api"}

```cpp {title: "fn::copack< Ts... >::operator="}
constexpr auto operator=(copack const &other) = default -> copack &;  // (1)
constexpr auto operator=(copack const &other)           -> copack &;  // (2)
constexpr auto operator=(copack &&other) = default      -> copack &;  // (3)
constexpr auto operator=(copack &&other)                -> copack &;  // (4)

template <typename... Tx>
constexpr auto operator=(copack<Tx...> const &arg) -> copack &;  // (5)
constexpr auto operator=(copack<Tx...> &&arg)      -> copack &;  // (6)

template <typename U, typename T>
constexpr auto operator=(U &&v) -> copack &;  // (7)
```

:include-doxygen-doc: fn::copack< Ts... >::operator= { args: "copack const &" }

:include-doxygen-doc-params: fn::copack< Ts... >::operator= { args: "copack const &", title: "parameters" }

:include-doxygen-doc: fn::copack< Ts... >::operator= { args: "copack &&" }

:include-doxygen-doc-params: fn::copack< Ts... >::operator= { args: "copack &&", title: "parameters" }

:include-doxygen-doc: fn::copack< Ts... >::operator= { args: "copack < Tx... > const &" }

:include-doxygen-doc-params: fn::copack< Ts... >::operator= { args: "copack < Tx... > const &", title: "parameters" }

:include-doxygen-doc: fn::copack< Ts... >::operator= { args: "copack < Tx... > &&" }

:include-doxygen-doc-params: fn::copack< Ts... >::operator= { args: "copack < Tx... > &&", title: "parameters" }

:include-doxygen-doc: fn::copack< Ts... >::operator= { args: "U &&" }

:include-doxygen-doc-params: fn::copack< Ts... >::operator= { args: "U &&", title: "parameters" }

## select_nth {style: "api"}

```cpp {title: "fn::copack< Ts... >::select_nth"}
template <std::size_t I>
using select_nth = detail::select_nth_t<I, Ts...>;  // (1)
```

:include-doxygen-doc: fn::copack< Ts... >::select_nth { args: "" }

:include-doxygen-doc-params: fn::copack< Ts... >::select_nth { args: "", type: "template", title: "template parameters" }
