---
title: "other fn utilities"
---

##### Defined in {style: "api", badge: "#include <fn/utility.hpp>"}

---

The small helpers the library exposes for use alongside the carriers: how an argument is stored
when a pipeline step holds it, how to forward with a borrowed value category, how to fuse
callables into one overload set, and how to lift a value into a type that prefers braces.

---

## overload {style: "api"}
Fuses per-alternative lambdas into a single overload set, which `fn::apply` and the verbs then
dispatch over by ordinary overload resolution.

:include-doxygen-member: fn::overload { signatureOnly: false, includeAllMatches: true }

---

## as_value_t {style: "api"}
:include-doxygen-member: fn::as_value_t { signatureOnly: false, includeAllMatches: true }

---

## apply_const_lvalue {style: "api"}
:include-doxygen-member: fn::apply_const_lvalue { signatureOnly: false, includeAllMatches: true }

---

## make {style: "api"}
:include-doxygen-member: fn::make { signatureOnly: false, includeAllMatches: true }
