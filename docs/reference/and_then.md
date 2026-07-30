---
title: "functor fn::and_then"
---

##### Defined in {style: "api", badge: "#include <fn/and_then.hpp>"}

---

:include-doxygen-doc: fn::and_then_t

---

## Call signatures {style: "api"}

```cpp {title: "fn::and_then_t::operator()"}
constexpr auto operator()(auto &&fn) const -> functor<and_then_t, decltype(fn)>;  // (1)
```

:include-doxygen-doc: fn::and_then_t::operator() { args: "auto &&" }

:include-doxygen-doc-params: fn::and_then_t::operator() { args: "auto &&", title: "parameters" }

---

## Return value {style: "api"}
A monadic type of the same kind.

---

## Examples {style: "api"}

:include-template: templates/snippet.md {
    path:  "simple/main.cpp",
    surroundedBy: ["// example-error-struct", "// example-expected-and_then-value"],
    desc:  "The resulting value is `13` because `ex` does not contain an `Error` and therefore `and_then` is called."
}

:include-template: templates/snippet.md {
    path:  "simple/main.cpp",
    surroundedBy: ["// example-error-struct", "// example-expected-and_then-error"],
    desc:  "The result is an `Error` because `ex` already contained an `Error` and therefore `and_then` is not called."
}

:include-template: templates/snippet.md {
    path:  "simple/main.cpp",
    surroundedBy: ["// example-optional-and_then-value"],
    desc:  "The resulting value is `13` because `op` is not a `nullopt` and therefore `and_then` is called."
}

:include-template: templates/snippet.md {
    path:  "simple/main.cpp",
    surroundedBy: ["// example-optional-and_then-empty"],
    desc:  "The result is a `nullopt` because `op` was already a `nullopt` and therefore `and_then` is not called."
}

:include-template: templates/snippet.md {
    path:  "simple/main.cpp",
    surroundedBy: ["// example-choice-parse", "// example-choice-checks"]
}
