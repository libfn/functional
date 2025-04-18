---
title: fn::discard
---

##### Defined in {style: "api", badge: "#include <fn/discard.hpp>"}

---

:include-doxygen-doc: fn::discard_t

---

## Call signatures {style: "api"}
:include-doxygen-member: fn::discard_t::operator() { signatureOnly: false, includeAllMatches: true }

---

## Return value {style: "api"}
void

---

## Examples {style: "api"}

:include-template: templates/snippet.md {
    path:  "examples/simple.cpp",
    surroundedBy: ["// example-error-struct", "// example-expected-discard"],
    desc:  "`42` is observed by `inspect` and the value is discarded by `discard` (no warning for discarded result of `inspect`)."
}
