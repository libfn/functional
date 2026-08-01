---
title: "fold fn::disjoin"
---

##### Defined in {style: "api", badge: "#include <fn/pack.hpp>"}

---

The disjunction `a | b` keeps the first operand that worked: values sum into a `copack` and errors
multiply into a `pack`, present only when every operand failed. Each carrier's header declares its
own `|`; this header defines the n-ary fold over it. This is the operator's disjunction meaning,
told apart by its right operand — a carrier. With a pipeline functor on the right it feeds the
carrier into that operation instead.

---

## The verb object {style: "api"}

```cpp {title: "fn::disjoin"}
disjoin_t disjoin;  // (1)
```

:include-doxygen-doc: fn::disjoin { args: "" }

## disjoin {style: "api"}

:include-doxygen-doc: fn::disjoin_t

---

## The operator {style: "api"}

The binary disjunction each carrier declares; `disjoin` is its n-ary fold.

```cpp {title: "fn::operator|"}
template <typename Lh, typename Rh>
constexpr auto operator|(Lh &&lh, Rh &&rh);                      // (1)
constexpr auto operator|(Lh &&, Rh &&)     -> ::fn::just<void>;  // (2)
constexpr auto operator|(Lh &&lh, Rh &&rh);                      // (3)

template <some_expected_void Lh, some_expected_void Rh>
constexpr auto operator|(Lh &&lh, Rh &&rh);  // (4)

template <typename Lh, typename Rh>
constexpr auto operator|(Lh &&lh, Rh &&rh);  // (5)

template <some_expected_void Lh, typename Rh>
constexpr auto operator|(Lh &&lh, Rh &&rh);  // (6)

template <typename Lh, some_expected_void Rh>
constexpr auto operator|(Lh &&lh, Rh &&rh);  // (7)

template <some_optional Lh, some_optional Rh>
constexpr auto operator|(Lh &&lh, Rh &&rh);  // (8)
```

:include-doxygen-doc: fn::operator| { args: "Lh &&, Rh &&" }

:include-doxygen-doc-params: fn::operator| { args: "Lh &&, Rh &&", title: "parameters" }

---

## Call signatures {style: "api"}

```cpp {title: "fn::disjoin_t::operator()"}
template <some_monadic_type Arg>
constexpr auto operator()(Arg &&arg) const -> decltype(arg);  // (1)

template <typename Arg, typename... Args>
constexpr auto operator()(Arg &&arg, Args &&...args) const;  // (2)
```

:include-doxygen-doc: fn::disjoin_t::operator() { args: "Arg &&" }

:include-doxygen-doc-params: fn::disjoin_t::operator() { args: "Arg &&", title: "parameters" }

:include-doxygen-doc: fn::disjoin_t::operator() { args: "Arg &&, Args &&..." }

:include-doxygen-doc-params: fn::disjoin_t::operator() { args: "Arg &&, Args &&...", title: "parameters" }
