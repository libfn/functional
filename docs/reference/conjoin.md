---
title: "fold fn::conjoin"
---

##### Defined in {style: "api", badge: "#include <fn/pack.hpp>"}

---

Independent computations compose side by side. The conjunction `a & b` keeps both results: values
multiply into a `pack` and errors sum into a `copack`, with the leftmost failing operand's error
held at runtime. Each carrier's header declares its own `&`; this header defines the n-ary fold
over it.

---

## conjoin {style: "api"}
:include-doxygen-doc: fn::conjoin_t

---

## Call signatures {style: "api"}
:include-doxygen-member: fn::conjoin_t::operator() { signatureOnly: false, includeAllMatches: true }
