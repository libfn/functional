---
title: "other fn comparisons"
---

Each carrier is compared where its contents are, and against the states it can be in. These are
declared alongside the carrier they serve rather than in one header, so reach them by including
the carrier's own.

An empty or failed operand equals nothing and orders before every value, so a comparison answers
rather than throwing; where a contained type has no such operator the comparison is simply not
viable, not ill-formed.

## operator== {style: "api"}

```cpp {title: "fn::operator=="}
template <typename... Ts, typename... Tx>
constexpr auto operator==(choice<Ts...> const &lh, choice<Tx...> const &rh) -> bool;  // (1)
constexpr auto operator==(copack<Ts...> const &lh, copack<Tx...> const &rh) -> bool;  // (2)

template <typename T, typename Err, typename T2>
constexpr auto operator==(expected<T, Err> const &x, T2 const &v) -> bool;  // (3)

template <typename T, typename U>
constexpr auto operator==(just<T> const &lh, just<U> const &rh) -> bool;  // (4)
constexpr auto operator==(just<T> const &lh, U const &rh)       -> bool;  // (5)

template <class T, class U>
constexpr auto operator==(optional<T> const &x, optional<U> const &y) -> bool;  // (6)

template <class T>
constexpr auto operator==(optional<T> const &x, std::nullopt_t) -> bool;  // (7)

template <class T, class U>
constexpr auto operator==(optional<T> const &x, U const &v) -> bool;  // (8)
constexpr auto operator==(T const &v, optional<U> const &x) -> bool;  // (9)
```

:include-doxygen-doc: fn::operator== { args: "choice < Ts... > const &, choice < Tx... > const &" }

:include-doxygen-doc-params: fn::operator== { args: "choice < Ts... > const &, choice < Tx... > const &", title: "parameters" }

:include-doxygen-doc: fn::operator== { args: "copack < Ts... > const &, copack < Tx... > const &" }

:include-doxygen-doc-params: fn::operator== { args: "copack < Ts... > const &, copack < Tx... > const &", title: "parameters" }

:include-doxygen-doc: fn::operator== { args: "just < T > const &, just < U > const &" }

:include-doxygen-doc: fn::operator== { args: "just < T > const &, U const &" }

:include-doxygen-doc: fn::operator== { args: "optional < T > const &, optional < U > const &" }

:include-doxygen-doc: fn::operator== { args: "optional < T > const &, ::std::nullopt_t" }

:include-doxygen-doc: fn::operator== { args: "optional < T > const &, U const &" }

:include-doxygen-doc: fn::operator== { args: "T const &, optional < U > const &" }

## operator!= {style: "api"}

```cpp {title: "fn::operator!="}
template <typename... Ts, typename... Tx>
constexpr auto operator!=(choice<Ts...> const &lh, choice<Tx...> const &rh) -> bool;  // (1)

template <class T, class U>
constexpr auto operator!=(optional<T> const &x, optional<U> const &y) -> bool;  // (2)
constexpr auto operator!=(optional<T> const &x, U const &v)           -> bool;  // (3)
constexpr auto operator!=(T const &v, optional<U> const &x)           -> bool;  // (4)
```

:include-doxygen-doc: fn::operator!= { args: "choice < Ts... > const &, choice < Tx... > const &" }

:include-doxygen-doc-params: fn::operator!= { args: "choice < Ts... > const &, choice < Tx... > const &", title: "parameters" }

:include-doxygen-doc: fn::operator!= { args: "optional < T > const &, optional < U > const &" }

:include-doxygen-doc: fn::operator!= { args: "optional < T > const &, U const &" }

:include-doxygen-doc: fn::operator!= { args: "T const &, optional < U > const &" }

## operator< {style: "api"}

```cpp {title: "fn::operator<"}
template <class T, class U>
constexpr auto operator<(optional<T> const &x, optional<U> const &y) -> bool;  // (1)
constexpr auto operator<(optional<T> const &x, U const &v)           -> bool;  // (2)
constexpr auto operator<(T const &v, optional<U> const &x)           -> bool;  // (3)
```

:include-doxygen-doc: fn::operator< { args: "optional < T > const &, optional < U > const &" }

:include-doxygen-doc: fn::operator< { args: "optional < T > const &, U const &" }

:include-doxygen-doc: fn::operator< { args: "T const &, optional < U > const &" }

## operator<= {style: "api"}

```cpp {title: "fn::operator<="}
template <class T, class U>
constexpr auto operator<=(optional<T> const &x, optional<U> const &y) -> bool;  // (1)
constexpr auto operator<=(optional<T> const &x, U const &v)           -> bool;  // (2)
constexpr auto operator<=(T const &v, optional<U> const &x)           -> bool;  // (3)
```

:include-doxygen-doc: fn::operator<= { args: "optional < T > const &, optional < U > const &" }

:include-doxygen-doc: fn::operator<= { args: "optional < T > const &, U const &" }

:include-doxygen-doc: fn::operator<= { args: "T const &, optional < U > const &" }

## operator> {style: "api"}

```cpp {title: "fn::operator>"}
template <class T, class U>
constexpr auto operator>(optional<T> const &x, optional<U> const &y) -> bool;  // (1)
constexpr auto operator>(optional<T> const &x, U const &v)           -> bool;  // (2)
constexpr auto operator>(T const &v, optional<U> const &x)           -> bool;  // (3)
```

:include-doxygen-doc: fn::operator> { args: "optional < T > const &, optional < U > const &" }

:include-doxygen-doc: fn::operator> { args: "optional < T > const &, U const &" }

:include-doxygen-doc: fn::operator> { args: "T const &, optional < U > const &" }

## operator>= {style: "api"}

```cpp {title: "fn::operator>="}
template <class T, class U>
constexpr auto operator>=(optional<T> const &x, optional<U> const &y) -> bool;  // (1)
constexpr auto operator>=(optional<T> const &x, U const &v)           -> bool;  // (2)
constexpr auto operator>=(T const &v, optional<U> const &x)           -> bool;  // (3)
```

:include-doxygen-doc: fn::operator>= { args: "optional < T > const &, optional < U > const &" }

:include-doxygen-doc: fn::operator>= { args: "optional < T > const &, U const &" }

:include-doxygen-doc: fn::operator>= { args: "T const &, optional < U > const &" }

## operator<=> {style: "api"}

```cpp {title: "fn::operator<=>"}
template <class T, std::three_way_comparable_with<T> U>
auto operator<=>(optional<T> const &x, optional<U> const &y) -> std::compare_three_way_result_t<T, U>;  // (1)

template <class T>
constexpr auto operator<=>(optional<T> const &x, std::nullopt_t) -> std::strong_ordering;  // (2)

template <class T, class U>
auto operator<=>(optional<T> const &x, U const &v) -> std::compare_three_way_result_t<T, U>;  // (3)
```

:include-doxygen-doc: fn::operator<=> { args: "optional < T > const &, optional < U > const &" }

:include-doxygen-doc: fn::operator<=> { args: "optional < T > const &, ::std::nullopt_t" }

:include-doxygen-doc: fn::operator<=> { args: "optional < T > const &, U const &" }

## swap {style: "api"}

```cpp {title: "fn::swap"}
template <class T>
constexpr auto swap(optional<T> &x, optional<T> &y) -> void;  // (1)
```

:include-doxygen-doc: fn::swap { args: "optional < T > &, optional < T > &" }
