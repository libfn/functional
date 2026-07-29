---
title: "other fn::functor"
---

##### Defined in {style: "api", badge: "#include <fn/functor.hpp>"}

---

What a verb call such as `fn::and_then(f)` returns, and the point at which the pipeline is open to
extension: a verb of your own, defined outside the library, pipes exactly like the built-in ones.
A verb is an empty, default-constructible type whose `operator()` returns a `functor` over itself,
and whose nested `apply` does the work; `fn::discard_t` is the smallest example to read.

---

:include-doxygen-doc: fn::functor

---

## Feeding a carrier into a step {style: "api"}
:include-doxygen-member: fn::functor::operator| { signatureOnly: false, includeAllMatches: true }
