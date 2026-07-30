---
title: "type fn::pack"
---

##### Defined in {style: "api", badge: "#include <fn/pack.hpp>"}

---

:include-doxygen-doc: fn::pack

## Append {style: "api"}
Grows the product without nesting: appending a value adds one field, and appending a pack
splices its fields in.

```cpp {title: "fn::pack::append"}
template <typename T>
constexpr auto append(std::in_place_type_t<T>, auto &&...args) &        -> append_type<T>;  // (1)
constexpr auto append(std::in_place_type_t<T>, auto &&...args) const &  -> append_type<T>;  // (2)
constexpr auto append(std::in_place_type_t<T>, auto &&...args) &&       -> append_type<T>;  // (3)
constexpr auto append(std::in_place_type_t<T>, auto &&...args) const && -> append_type<T>;  // (4)

template <typename Arg>
constexpr auto append(Arg &&arg) &        -> append_type<Arg>;  // (5)
constexpr auto append(Arg &&arg) const &  -> append_type<Arg>;  // (6)
constexpr auto append(Arg &&arg) &&       -> append_type<Arg>;  // (7)
constexpr auto append(Arg &&arg) const && -> append_type<Arg>;  // (8)
```

:include-doxygen-doc: fn::pack::append { args: "::std::in_place_type_t< T >, auto &&..." }

:include-doxygen-doc-params: fn::pack::append { args: "::std::in_place_type_t< T >, auto &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::pack::append { args: "::std::in_place_type_t< T >, auto &&...", title: "parameters" }

:include-doxygen-doc: fn::pack::append { args: "Arg &&" }

:include-doxygen-doc-params: fn::pack::append { args: "Arg &&", title: "parameters" }

## Apply {style: "api"}
Elimination: the elements spread into a callable as separate arguments.

```cpp {title: "fn::pack::apply"}
template <typename Fn>
constexpr auto apply(Fn &&fn, auto &&...args) &        -> decltype(auto);  // (1)
constexpr auto apply(Fn &&fn, auto &&...args) const &  -> decltype(auto);  // (2)
constexpr auto apply(Fn &&fn, auto &&...args) &&       -> decltype(auto);  // (3)
constexpr auto apply(Fn &&fn, auto &&...args) const && -> decltype(auto);  // (4)
```

:include-doxygen-doc: fn::pack::apply { args: "Fn &&, auto &&..." }

:include-doxygen-doc-params: fn::pack::apply { args: "Fn &&, auto &&...", title: "parameters" }

## apply_r {style: "api"}

```cpp {title: "fn::pack::apply_r"}
template <typename Ret, typename Fn>
constexpr auto apply_r(Fn &&fn, auto &&...args) &        -> Ret;  // (1)
constexpr auto apply_r(Fn &&fn, auto &&...args) const &  -> Ret;  // (2)
constexpr auto apply_r(Fn &&fn, auto &&...args) &&       -> Ret;  // (3)
constexpr auto apply_r(Fn &&fn, auto &&...args) const && -> Ret;  // (4)
```

:include-doxygen-doc: fn::pack::apply_r { args: "Fn &&, auto &&..." }

:include-doxygen-doc-params: fn::pack::apply_r { args: "Fn &&, auto &&...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::pack::apply_r { args: "Fn &&, auto &&...", title: "parameters" }

## operator== {style: "api"}

```cpp {title: "fn::pack::operator=="}
constexpr auto operator==(pack const &other) const -> bool;  // (1)
```

:include-doxygen-doc: fn::pack::operator== { args: "pack const &" }
