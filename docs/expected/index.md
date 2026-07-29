---
title: Expected monad
---

##### Defined in {style: "api", badge: "#include <fn/expected.hpp>"}

---

:include-doxygen-doc: fn::expected

## expected_unit {style: "api"}
The graded gateway: initiating a pipeline with this unit trigger opts all subsequent `and_then`
steps into graded error-set unioning, with no fake starting errors.

:include-doxygen-member: fn::expected_unit { signatureOnly: false, includeAllMatches: true }

## copack_error {style: "api"}
The explicit lift into the graded world, on the error side.

:include-doxygen-member: fn::expected::copack_error { signatureOnly: false, includeAllMatches: true }

## copack_value {style: "api"}
The same lift, on the value side.

:include-doxygen-member: fn::expected::copack_value { signatureOnly: false, includeAllMatches: true }
