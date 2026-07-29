---
title: fn::transform_error
---

##### Defined in {style: "api", badge: "#include <fn/transform_error.hpp>"}

---

:include-doxygen-doc: fn::transform_error_t

---

## Call signatures {style: "api"}
:include-doxygen-member: fn::transform_error_t::operator() { signatureOnly: false, includeAllMatches: true }

---

## Return value {style: "api"}
Not applicable: `transform_error` is rejected on `optional`, which has no error value to map.
Use `or_else` to act on the empty state instead.
