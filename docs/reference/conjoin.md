---
title: "fold fn::conjoin"
---

##### Defined in {style: "api", badge: "#include <fn/algebra.hpp>"}

---

Independent computations compose side by side. The conjunction `a & b` keeps both results: values
multiply into a `pack` and errors sum into a `copack`, with the leftmost failing operand's error
held at runtime. Each carrier's header declares its own `&`; this header defines the n-ary fold
over it.

---

## The verb object {style: "api"}

```cpp {title: "fn::conjoin"}
conjoin_t conjoin;  // (1)
```

:include-doxygen-doc: fn::conjoin { args: "" }

## conjoin {style: "api"}

:include-doxygen-doc: fn::conjoin_t

---

## The operator {style: "api"}

Binary conjunction of data or carriers; `conjoin` is its n-ary fold.

```cpp {title: "fn::operator&"}
constexpr auto operator&(auto &&lh, auto &&rh);  // (1)

template <typename Lh, typename Rh>
constexpr auto operator&(Lh &&lh, Rh &&rh);  // (2)

template <typename Lh, some_expected Rh>
constexpr auto operator&(Lh &&lh, Rh &&rh);  // (3)

template <some_expected Lh, typename Rh>
constexpr auto operator&(Lh &&lh, Rh &&rh);  // (4)

template <typename Lh, some_expected_void Rh>
constexpr auto operator&(Lh &&, Rh &&rh);  // (5)

template <some_expected_void Lh, typename Rh>
constexpr auto operator&(Lh &&lh, Rh &&);  // (6)

template <typename Lh, typename Rh>
constexpr auto operator&(Lh &&lh, Rh &&rh);                // (7)
constexpr auto operator&(Lh &&, Rh &&)     -> just<void>;  // (8)

template <some_optional Lh, some_optional Rh>
constexpr auto operator&(Lh &&lh, Rh &&rh);  // (9)

template <typename Lh, some_optional Rh>
constexpr auto operator&(Lh &&lh, Rh &&rh);  // (10)

template <some_optional Lh, typename Rh>
constexpr auto operator&(Lh &&lh, Rh &&rh);  // (11)

template <typename Lh, some_optional Rh>
constexpr auto operator&(Lh &&lh, Rh &&rh);  // (12)

template <some_optional Lh, typename Rh>
constexpr auto operator&(Lh &&lh, Rh &&rh);  // (13)
```

:include-doxygen-doc: fn::operator& { args: "auto &&, auto &&" }

:include-doxygen-doc-params: fn::operator& { args: "auto &&, auto &&", title: "parameters" }

:include-doxygen-doc: fn::operator& { args: "Lh &&, Rh &&" }

:include-doxygen-doc-params: fn::operator& { args: "Lh &&, Rh &&", title: "parameters" }

---

## Call signatures {style: "api"}

```cpp {title: "fn::conjoin_t::operator()"}
template <some_monadic_type Arg>
constexpr auto operator()(Arg &&arg) const;  // (1)

template <typename Arg, typename... Args>
constexpr auto operator()(Arg &&arg, Args &&...args) const;  // (2)
```

:include-doxygen-doc: fn::conjoin_t::operator() { args: "Arg &&" }

:include-doxygen-doc-params: fn::conjoin_t::operator() { args: "Arg &&", title: "parameters" }

:include-doxygen-doc: fn::conjoin_t::operator() { args: "Arg &&, Args &&..." }

:include-doxygen-doc-params: fn::conjoin_t::operator() { args: "Arg &&, Args &&...", title: "parameters" }
