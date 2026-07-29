---
title: "fold fn::disjoin"
---

##### Defined in {style: "api", badge: "#include <fn/pack.hpp>"}

---

The disjunction `a | b` keeps the first operand that worked: values sum into a `copack` and errors
multiply into a `pack`, present only when every operand failed. Each carrier's header declares its
own `|`; this header defines the n-ary fold over it. This is the operator's disjunction meaning,
told apart by its right operand — a carrier. With a pipeline functor on the right it feeds the
carrier into that operation instead.

---

## disjoin {style: "api"}
:include-doxygen-doc: fn::disjoin_t

---

## Call signatures {style: "api"}
:include-doxygen-member: fn::disjoin_t::operator() { signatureOnly: false, includeAllMatches: true }
