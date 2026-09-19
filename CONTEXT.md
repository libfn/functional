# libfn terminology

Project-specific meanings used when describing libfn's monadic facilities.

## Language

**Combinator**:
A monadic operation provided independently of a carrier and used through the `|` pipeline syntax. A combinator supports all monadic carriers or a defined subset; the term does not refer to a carrier's member functions.
_Avoid_: Pipeline functor, verb, combinator for a member function

**Bind combinator**:
`fn::and_then`, which supports every monadic carrier family and can bridge supported combinations of families. The member operation `.and_then` keeps the same carrier family.

**Carrier bridging**:
A transition between monadic carrier families through a combinator, subject to the rules for preserving the input's possible value and failure states.

**Identity carriers (identity cluster)**:
The group of monadic carriers defined by infallibility: `just`, `choice` and `expected<T, copack<>>`. None adds a failure or empty state to its payload.
_Avoid_: The identity monad as a name for this group

**Expected unit**:
`fn::expected_unit`, the alias for `expected<void, copack<>>`: infallible success with no payload. It is one member of the identity cluster, rather than a name for every infallible `expected`.
