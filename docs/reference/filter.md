---
title: "functor fn::filter"
---

##### Defined in {style: "api", badge: "#include <fn/filter.hpp>"}

---

:include-doxygen-doc: fn::filter_t

---

## Call signatures {style: "api"}

```cpp {title: "fn::filter_t::operator()"}
constexpr auto operator()(auto &&pred, auto &&on_err) const -> functor<filter_t, decltype(pred), decltype(on_err)>;  // (1)
constexpr auto operator()(auto &&pred) const                -> functor<filter_t, decltype(pred)>;                    // (2)
```

:include-doxygen-doc: fn::filter_t::operator() { args: "auto &&, auto &&" }

:include-doxygen-doc-params: fn::filter_t::operator() { args: "auto &&, auto &&", title: "parameters" }

:include-doxygen-doc: fn::filter_t::operator() { args: "auto &&" }

:include-doxygen-doc-params: fn::filter_t::operator() { args: "auto &&", title: "parameters" }

---

## Return value {style: "api"}
A monadic type of the same kind.

---

## Examples {style: "api"}

:include-template: templates/snippet.md {
    path:  "simple/main.cpp",
    surroundedBy: ["// example-error-struct", "// example-expected-filter-value"],
    desc:  "The resulting value is `42` because the filter predicate returns `true` for `42` as it is not less than `42`."
}

:include-template: templates/snippet.md {
    path:  "simple/main.cpp",
    surroundedBy: ["// example-error-struct", "// example-expected-filter-error"],
    desc:  "The error is set to `Less than 42` because the predicate returns `false` for `12` since it's less than `42`."
}

:include-template: templates/snippet.md {
    path:  "simple/main.cpp",
    surroundedBy: ["// example-optional-filter-value"],
    desc:  "The resulting value is `42` because the filter predicate returns `true` for `42` as it is not less than `42`."
}

:include-template: templates/snippet.md {
    path:  "simple/main.cpp",
    surroundedBy: ["// example-optional-filter-empty"],
    desc:  "The optional is empty because the predicate returns `false` for `12` since it's less than `42`."
}
