---
title: Choice monad
---

##### Defined in {style: "api", badge: "#include <fn/choice.hpp>"}

---

:include-doxygen-doc: fn::choice

## choice_for {style: "api"}
The construction alias: accepts alternatives in any order, with duplicates and nested copacks,
and resolves to the canonical `fn::choice`. Prefer it over spelling `choice` directly, so that no
spelling in your project is tied to one compiler's alternative order.

:include-doxygen-member: fn::choice_for { signatureOnly: false, includeAllMatches: true }

## choice {style: "api"}
Construction: from a value of one alternative, in place from arguments, or widening from a
`copack` over a subset of the alternatives.

:include-doxygen-member: fn::choice { signatureOnly: false, includeAllMatches: true }

## value {style: "api"}
The alternatives as the underlying `copack`, always present.

:include-doxygen-member: fn::choice< Ts... >::value { signatureOnly: false, includeAllMatches: true }
