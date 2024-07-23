---
title: fn::filter
---

##### Defined in {style: "api", badge: "#include <functional/filter.hpp>"}

---

:include-doxygen-doc: fn::filter_t

---

## Call signatures {style: "api"}
:include-doxygen-member: fn::filter_t::operator() { signatureOnly: false, includeAllMatches: true }

---

## Return value {style: "api"}
A monadic type of the same kind.

---

## Examples {style: "api"}

:include-template: templates/snippet.md {
    path:  "examples/simple.cpp", 
    surroundedBy: ["// example-optional-filter-value"],
    desc:  "The resulting value is `42` because the filter predicate returns `true` for `42` as it is not less than `42`."
}

:include-template: templates/snippet.md {
    path:  "examples/simple.cpp", 
    surroundedBy: ["// example-optional-filter-empty"],
    desc:  "The optional is empty because the predicate returns `false` for `12` since it's less than `42`."
}
