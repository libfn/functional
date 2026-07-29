---
title: "type fn::copack"
---

##### Defined in {style: "api", badge: "#include <fn/copack.hpp>"}

---

:include-doxygen-doc: fn::copack

:include-doxygen-doc: fn::copack_for

## Apply {style: "api"}
Elimination: the active alternative routes into the callable, exhaustively - every alternative
must have a viable arm.

:include-doxygen-member: fn::copack< Ts... >::apply { signatureOnly: false, includeAllMatches: true }

## Transform {style: "api"}
The self-flattening map over the alternatives: the branch results form a new normalized copack.

:include-doxygen-member: fn::copack< Ts... >::transform { signatureOnly: false, includeAllMatches: true }
