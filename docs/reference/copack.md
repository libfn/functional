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
constexpr auto apply(Fn &&fn, Args &&...args) &        -> typename detail::_copack_apply_result<detail::_apply_autodetect_tag, Fn &&, copack &, Args &&...>::type;         // (1)
constexpr auto apply(Fn &&fn, Args &&...args) const &  -> typename detail::_copack_apply_result<detail::_apply_autodetect_tag, Fn &&, copack const &, Args &&...>::type;   // (2)
constexpr auto apply(Fn &&fn, Args &&...args) &&       -> typename detail::_copack_apply_result<detail::_apply_autodetect_tag, Fn &&, copack &&, Args &&...>::type;        // (3)
constexpr auto apply(Fn &&fn, Args &&...args) const && -> typename detail::_copack_apply_result<detail::_apply_autodetect_tag, Fn &&, copack const &&, Args &&...>::type;  // (4)
```

:include-doxygen-doc: fn::copack< Ts... >::apply { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::copack< Ts... >::apply { args: "Fn &&, Args &&...", title: "parameters" }

## Transform {style: "api"}
The self-flattening map over the alternatives: the branch results form a new normalized copack.

```cpp {title: "fn::copack< Ts... >::transform"}
template <typename Fn, typename... Args>
constexpr auto transform(Fn &&fn, Args &&...args) &        -> typename detail::_copack_apply_result<detail::_collapsing_copack_tag, Fn &&, copack &, Args &&...>::type;         // (1)
constexpr auto transform(Fn &&fn, Args &&...args) const &  -> typename detail::_copack_apply_result<detail::_collapsing_copack_tag, Fn &&, copack const &, Args &&...>::type;   // (2)
constexpr auto transform(Fn &&fn, Args &&...args) &&       -> typename detail::_copack_apply_result<detail::_collapsing_copack_tag, Fn &&, copack &&, Args &&...>::type;        // (3)
constexpr auto transform(Fn &&fn, Args &&...args) const && -> typename detail::_copack_apply_result<detail::_collapsing_copack_tag, Fn &&, copack const &&, Args &&...>::type;  // (4)
```

:include-doxygen-doc: fn::copack< Ts... >::transform { args: "Fn &&, Args &&..." }

:include-doxygen-doc-params: fn::copack< Ts... >::transform { args: "Fn &&, Args &&...", title: "parameters" }
