---
title: Multidispatch
---

##### Defined in {style: "api", badge: "#include <fn/functional.hpp>"}

---

Elimination of the algebraic structures: `fn::apply` unpacks products and dispatches over
alternatives by ordinary C++ overload resolution, and `fn::overload` fuses per-alternative
lambdas into one overload set. Dispatch is exhaustive: an alternative without a viable arm makes
the whole call not applicable.

## apply {style: "api"}
:include-doxygen-member: fn::apply { signatureOnly: false, includeAllMatches: true }

## apply_r {style: "api"}
:include-doxygen-member: fn::apply_r { signatureOnly: false, includeAllMatches: true }

## overload {style: "api"}
:include-doxygen-doc: fn::overload
