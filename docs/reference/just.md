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

## transform {style: "api"}

```cpp {title: "fn::just::transform"}
template <typename Fn>
constexpr auto transform(Fn &&fn) &        -> typename detail::_just_transform_result<Fn, T &>::type;         // (1)
constexpr auto transform(Fn &&fn) const &  -> typename detail::_just_transform_result<Fn, T const &>::type;   // (2)
constexpr auto transform(Fn &&fn) &&       -> typename detail::_just_transform_result<Fn, T &&>::type;        // (3)
constexpr auto transform(Fn &&fn) const && -> typename detail::_just_transform_result<Fn, T const &&>::type;  // (4)
```

:include-doxygen-doc: fn::just::transform { args: "Fn &&" }

:include-doxygen-doc-params: fn::just::transform { args: "Fn &&", title: "parameters" }

## and_then {style: "api"}

```cpp {title: "fn::just::and_then"}
template <typename Fn>
constexpr auto and_then(Fn &&fn) &        -> std::remove_cvref_t<typename detail::_apply_result<Fn, T &>::type>;         // (1)
constexpr auto and_then(Fn &&fn) const &  -> std::remove_cvref_t<typename detail::_apply_result<Fn, T const &>::type>;   // (2)
constexpr auto and_then(Fn &&fn) &&       -> std::remove_cvref_t<typename detail::_apply_result<Fn, T &&>::type>;        // (3)
constexpr auto and_then(Fn &&fn) const && -> std::remove_cvref_t<typename detail::_apply_result<Fn, T const &&>::type>;  // (4)
```

:include-doxygen-doc: fn::just::and_then { args: "Fn &&" }

:include-doxygen-doc-params: fn::just::and_then { args: "Fn &&", title: "parameters" }
