---
title: fn::and_then
---

##### Defined in {style: "api", badge: "#include <functional/and_then.hpp>"}

---

:include-doxygen-doc: fn::and_then_t

---

## Call signatures {style: "api"}
:include-doxygen-member: fn::and_then_t::operator() { signatureOnly: false, includeAllMatches: true }

---

## Return value {style: "api"}
A monadic type of the same kind.

---

## Examples {style: "api"}

:include-template: templates/snippet.md {
    path:  "and_then/expected_value.cpp", 
    desc:  "The resulting value is `13` because `ex` does not contain an `Error` and therefore `and_then` is called."
}

:include-template: templates/snippet.md {
    path: "and_then/expected_error.cpp", 
    desc: "The result is an `Error` because `ex` already contained an `Error` and therefore `and_then` is not called."
}
