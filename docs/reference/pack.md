---
title: "type fn::pack"
---

##### Defined in {style: "api", badge: "#include <fn/pack.hpp>"}

---

:include-doxygen-doc: fn::pack

## Append {style: "api"}
Grows the product without nesting: appending a value adds one field, and appending a pack
splices its fields in.
:include-doxygen-member: fn::pack::append { signatureOnly: false, includeAllMatches: true }

## Apply {style: "api"}
Elimination: the elements spread into a callable as separate arguments.
:include-doxygen-member: fn::pack::apply { signatureOnly: false, includeAllMatches: true }
