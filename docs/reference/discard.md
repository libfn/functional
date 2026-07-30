---
title: "functor fn::discard"
---

##### Defined in {style: "api", badge: "#include <fn/discard.hpp>"}

---

:include-doxygen-doc: fn::discard_t

---

## The verb object {style: "api"}

```cpp {title: "fn::discard"}
discard_t discard = {};  // (1)
```

:include-doxygen-doc: fn::discard { args: "" }

## Call signatures {style: "api"}

```cpp {title: "fn::discard_t::operator()"}
constexpr auto operator()() const -> functor<discard_t>;  // (1)
```

:include-doxygen-doc: fn::discard_t::operator() { args: "" }

:include-doxygen-doc-params: fn::discard_t::operator() { args: "", title: "parameters" }

---

## Return value {style: "api"}

void

---

## Examples {style: "api"}

:include-template: templates/snippet.md {
    path:  "simple/main.cpp",
    surroundedBy: ["// example-error-struct", "// example-expected-discard"],
    desc:  "`42` is observed by `inspect` and the value is discarded by `discard` (no warning for discarded result of `inspect`)."
}

:include-template: templates/snippet.md {
    path:  "simple/main.cpp",
    surroundedBy: ["// example-error-struct", "// example-optional-discard"],
    desc:  "`42` is observed by `inspect` and the value is discarded by `discard` (no warning for discarded result of `inspect`)."
}
