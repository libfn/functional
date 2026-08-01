---
title: "fold fn::conjoin"
---

##### Defined in {style: "api", badge: "#include <fn/pack.hpp>"}

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

The binary conjunction each carrier declares; `conjoin` is its n-ary fold.

```cpp {title: "fn::operator&"}
template <typename Lh, typename Rh>
constexpr auto operator&(Lh &&lh, Rh &&rh);                             // (1)
constexpr auto operator&(Lh &&, Rh &&rh)   -> std::remove_cvref_t<Rh>;  // (2)
constexpr auto operator&(Lh &&lh, Rh &&)   -> std::remove_cvref_t<Lh>;  // (3)
constexpr auto operator&(Lh &&lh, Rh &&rh);                             // (4)

template <typename Lh, some_expected Rh>
constexpr auto operator&(Lh &&lh, Rh &&rh);  // (5)

template <some_expected Lh, typename Rh>
constexpr auto operator&(Lh &&lh, Rh &&rh);  // (6)

template <typename Lh, some_expected_void Rh>
constexpr auto operator&(Lh &&lh, Rh &&rh) -> expected<typename std::remove_cvref_t<Lh>::value_type, typename std::remove_cvref_t<Rh>::error_type>;  // (7)

template <some_expected_void Lh, typename Rh>
constexpr auto operator&(Lh &&lh, Rh &&rh) -> expected<typename std::remove_cvref_t<Rh>::value_type, typename std::remove_cvref_t<Lh>::error_type>;  // (8)

template <typename Lh, some_expected Rh>
constexpr auto operator&(Lh &&, Rh &&rh);  // (9)

template <some_expected Lh, typename Rh>
constexpr auto operator&(Lh &&lh, Rh &&);  // (10)

template <typename Lh, typename Rh>
constexpr auto operator&(Lh &&lh, Rh &&rh);                             // (11)
constexpr auto operator&(Lh &&, Rh &&rh)   -> std::remove_cvref_t<Rh>;  // (12)
constexpr auto operator&(Lh &&lh, Rh &&)   -> std::remove_cvref_t<Lh>;  // (13)

template <some_optional Lh, some_optional Rh>
constexpr auto operator&(Lh &&lh, Rh &&rh);  // (14)

template <typename Lh, some_optional Rh>
constexpr auto operator&(Lh &&lh, Rh &&rh);  // (15)

template <some_optional Lh, typename Rh>
constexpr auto operator&(Lh &&lh, Rh &&rh);  // (16)

template <typename Lh, some_optional Rh>
constexpr auto operator&(Lh &&, Rh &&rh);  // (17)

template <some_optional Lh, typename Rh>
constexpr auto operator&(Lh &&lh, Rh &&);  // (18)

constexpr auto operator&(auto &&lh, auto &&rh);  // (19)
```

:include-doxygen-doc: fn::operator& { args: "Lh &&, Rh &&" }

:include-doxygen-doc-params: fn::operator& { args: "Lh &&, Rh &&", title: "parameters" }

:include-doxygen-doc: fn::operator& { args: "auto &&, auto &&" }

:include-doxygen-doc-params: fn::operator& { args: "auto &&, auto &&", title: "parameters" }

---

## Call signatures {style: "api"}

```cpp {title: "fn::conjoin_t::operator()"}
template <typename Arg>
constexpr auto operator()(Arg &&arg) const -> decltype(arg);  // (1)

template <typename Arg, typename... Args>
constexpr auto operator()(Arg &&arg, Args &&...args) const;  // (2)
```

:include-doxygen-doc: fn::conjoin_t::operator() { args: "Arg &&" }

:include-doxygen-doc-params: fn::conjoin_t::operator() { args: "Arg &&", title: "parameters" }

:include-doxygen-doc: fn::conjoin_t::operator() { args: "Arg &&, Args &&..." }

:include-doxygen-doc-params: fn::conjoin_t::operator() { args: "Arg &&, Args &&...", title: "parameters" }
