---
title: "type fn::pack"
---

##### Defined in {style: "api", badge: "#include <fn/pack.hpp>"}

---

:include-doxygen-doc: fn::pack

## Member types {style: "api"}

```cpp {title: "fn::pack::append_type"}
template <typename T>
using append_type = _impl::template append_type<T>;  // (1)
```

:include-doxygen-doc: fn::pack::append_type { args: "" }

## operator== {style: "api"}

```cpp {title: "fn::pack::operator=="}
constexpr auto operator==(pack const &other) const -> bool;  // (1)
```

:include-doxygen-doc: fn::pack::operator== { args: "pack const &" }

## operator<=> {style: "api"}

```cpp {title: "fn::pack::operator<=>"}
constexpr auto operator<=>(pack const &other) const;  // (1)
```

:include-doxygen-doc: fn::pack::operator<=> { args: "pack const &" }

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

## as_pack {style: "api"}

```cpp {title: "fn::as_pack"}
constexpr auto as_pack() -> pack<>;  // (1)

template <typename... Explicit, typename T, typename... Args>
constexpr auto as_pack(T &&src, Args &&...args) -> pack<T, Args...>;  // (2)

template <typename T, typename... Args>
constexpr auto as_pack(std::type_identity_t<T> src, std::type_identity_t<Args>... args) -> pack<T, Args...>;  // (3)
```

:include-doxygen-doc: fn::as_pack { args: "" }

:include-doxygen-doc-params: fn::as_pack { args: "", title: "parameters" }

:include-doxygen-doc: fn::as_pack { args: "T &&, Args &&..." }

:include-doxygen-doc-params: fn::as_pack { args: "T &&, Args &&...", title: "parameters" }

:include-doxygen-doc: fn::as_pack { args: "::std::type_identity_t< T >, ::std::type_identity_t< Args >..." }

:include-doxygen-doc-params: fn::as_pack { args: "::std::type_identity_t< T >, ::std::type_identity_t< Args >...", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::as_pack { args: "::std::type_identity_t< T >, ::std::type_identity_t< Args >...", title: "parameters" }

## get {style: "api"}

```cpp {title: "fn::get"}
template <typename Cp>
constexpr auto get(Cp &&c) -> decltype(auto);  // (1)

template <std::size_t I, some_pack P>
constexpr auto get(P &&p) -> decltype(auto);  // (2)
```

:include-doxygen-doc: fn::get { args: "Cp &&" }

:include-doxygen-doc-params: fn::get { args: "Cp &&", title: "parameters" }

:include-doxygen-doc: fn::get { args: "P &&" }

:include-doxygen-doc-params: fn::get { args: "P &&", type: "template", title: "template parameters" }

:include-doxygen-doc-params: fn::get { args: "P &&", title: "parameters" }
