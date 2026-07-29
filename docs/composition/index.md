---
title: Composition
---

##### Defined in {style: "api", badge: "#include <fn/pack.hpp>"}

---

Independent computations compose side by side. The conjunction `a & b` keeps both results:
values multiply into a `pack` and errors sum into a `copack`, with the leftmost failing operand's
error held at runtime. The disjunction `a | b` keeps the first that worked: values sum into a
`copack` and errors multiply into a `pack`, present only when every operand failed. Both
operators have n-ary folds.

## conjoin {style: "api"}
:include-doxygen-doc: fn::conjoin_t

## conjoin call signatures {style: "api"}
:include-doxygen-member: fn::conjoin_t::operator() { signatureOnly: false, includeAllMatches: true }

## disjoin {style: "api"}
:include-doxygen-doc: fn::disjoin_t

## disjoin call signatures {style: "api"}
:include-doxygen-member: fn::disjoin_t::operator() { signatureOnly: false, includeAllMatches: true }
