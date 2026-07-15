# Type algebra and multidispatch in libfn

libfn lets a program compose not only values, but also the *shapes* of those values. `fn::pack` combines
values that are all present; `fn::sum` represents one value selected from several possible types; and
`operator&` combines the two by distributing products over alternatives. The resulting values can be
passed to ordinary overloaded functions through `fn::apply`.

This document explains that model for a C++ programmer. The small mathematical notes are included to
make the design precise and reviewable; no category theory is needed to use the library.

> [!NOTE]
> The `apply` name describes the interface being implemented by issue
> [#84](https://github.com/libfn/functional/issues/84). It replaces the older `invoke` spelling and is
> intended to extend `std::apply`. The normal-form constraints for `choice` and nested `pack` are tracked
> by [#323](https://github.com/libfn/functional/issues/323) and
> [#328](https://github.com/libfn/functional/issues/328). Examples below describe the settled interface,
> including those changes where the current release branch has not yet caught up.

## The two layers

The most important distinction in the design is between two layers:

1. The **type algebra** stores and composes exact C++ types. Its vocabulary is `sum` and `pack`.
2. The **application protocol** exposes a stored value as arguments to a function. Its vocabulary is
   `apply` and ordinary overload resolution.

These layers deliberately answer different questions. The algebra asks, “Which exact type is stored, and
how was it composed?” Application asks, “Which arguments should this value supply to this function?”

Distinct types are never merged merely because they apply in the same way:

```cpp
struct A {};

using representations = fn::sum_for<
    fn::pack<A>,
    std::tuple<A>,
    std::array<A, 1>>;

static_assert(representations::size == 3);
```

All three supported structured types apply as one `A` argument, but they do not become the same type.
Information may be forgotten when the programmer chooses to eliminate a value; it is never forgotten
while normalizing the stored type.

## `pack`: an ordered product

A `pack<Ts...>` holds one value of every listed type, in order:

```cpp
fn::pack<int, std::string> p{42, "answer"};
```

It resembles a tuple, but its intended role is a flat, composable argument sequence. Applying it spreads
its elements into a call:

```cpp
auto n = fn::apply(
    [](int i, std::string const& s) { return i + static_cast<int>(s.size()); },
    p);
```

### Composition by `append`

Appending an ordinary value adds one element. Appending another pack splices all of its elements:

```cpp
auto a = fn::pack{1, 2.0};
auto b = a.append('x');                    // pack<int, double, char>
auto c = a.append(fn::pack{'x', false});   // pack<int, double, char, bool>
```

At the type level, pack composition is ordered concatenation:

```text
pack<Ts...> · pack<Us...> = pack<Ts..., Us...>
```

Order is meaningful, so `pack<A, B>` and `pack<B, A>` are different types. There is no `pack_for`
analogue of `sum_for`: a product does not sort or deduplicate its elements.

`append` is a value operation rather than just a type computation. Its ref-qualified overloads can move
from an rvalue pack, which makes association a useful performance choice:

```cpp
p.append(q).append(r);
p.append(q.append(r));
```

Both expressions have the same final flat type and, for regular element types, the same ordered values.
They need not perform the same moves. A programmer can choose the expression tree that avoids repeatedly
moving the most expensive group of elements.

### The empty product

`pack<>` is the unit of concatenation and the natural argument list of a nullary function:

```cpp
static_assert(fn::apply([] { return 42; }, fn::pack<>{}) == 42);

fn::pack<>{}.append(fn::pack<int>{1}); // pack<int>
fn::pack<int>{1}.append(fn::pack<>{}); // pack<int>
```

It is the *unit* or *empty product*, not an identity function or identity monad.

### Flat libfn packs

The canonical libfn product is flat. A raw nested pack is rejected:

```cpp
fn::pack<fn::pack<int, double>, char> // ill-formed
```

Composition spells the intended flat result:

```cpp
fn::pack<int, double>{1, 2.0}.append('x') // pack<int, double, char>
```

If a product-shaped value must remain one semantic field, give it a distinct type:

```cpp
struct coordinates {
  fn::pack<double, double> value;
};

fn::pack<std::string, coordinates> named_position;
```

Foreign structured types remain legal elements. A supported standard tuple-like type such as
`std::tuple` or `std::array` is recognized by `apply`, but it does not become an inner `fn::pack` in the
algebra. An arbitrary user wrapper remains opaque: `apply` passes the wrapper itself as one argument
unless the program explicitly converts it to a supported structured type.

Mathematically, products satisfy associativity and unit laws up to canonical isomorphism:

```text
(A × B) × C ≅ A × (B × C)
A × 1 ≅ A ≅ 1 × A
```

The flat representation chooses one spelling for these association and unit variants *inside pack
composition*. It does not identify a one-element `pack<A>` with bare `A`; those remain distinct C++
types.

## `sum`: a normalized set of alternatives

A `sum<Ts...>` holds exactly one value whose type is one of `Ts...`. Unlike `std::variant`, alternatives
are identified by type rather than position:

```cpp
using result = fn::sum_for<int, std::string>;

result x{42};
assert(x.has_value<int>());
```

`sum_for` is the normalizing maker. It flattens nested sum inputs, removes repeated exact types, and puts
alternatives in a canonical type order:

```cpp
static_assert(std::same_as<
    fn::sum_for<int, fn::sum<bool, int>>,
    fn::sum_for<bool, int>>);
```

The operation is set union:

```text
sum_for<S...> ∪ sum_for<T...> = sum_for<S..., T...>
```

It is therefore:

- commutative: `A ∪ B = B ∪ A`;
- associative: `(A ∪ B) ∪ C = A ∪ (B ∪ C)`;
- idempotent: `A ∪ A = A`;
- unital, with the empty alternative set `sum<>`.

`sum<>` and `pack<>` are opposites in an important practical sense. `sum<>` has no possible value and
cannot dispatch. `pack<>` is a valid value with zero elements and applies to a nullary function.

### Arbitrary exact types are alternatives

A sum is not required to contain packs. Bare types, packs, standard tuple-like types, and user types may
coexist:

```cpp
using alternatives = fn::sum_for<
    int,
    fn::pack<int>,
    std::tuple<int>,
    std::array<int, 1>>;

static_assert(alternatives::size == 4);
```

These remain four alternatives even though applying each one produces a call with one `int`.
The normalization invariant uses exact type identity; it does not inspect application shape. The current
provisional implementation does not yet satisfy that invariant when two compiler-generated sort keys
collide, which is the defect tracked by #326 below.

The implementation needs a canonical order for types. C++26 supplies `std::type_order`, motivated in
part by libfn's use case. Until compilers provide it, libfn uses compiler type spellings. Issue
[#326](https://github.com/libfn/functional/issues/326) tracks the required collision diagnostic: two
distinct types must never be silently deduplicated because their provisional sort keys happen to match.

### Widening

A value can be widened from a smaller alternative set to a larger one without changing its selected
alternative:

```cpp
fn::sum<int> narrow{42};
fn::sum_for<int, std::string> wide = narrow;
```

This is safe because the larger sum accepts every type accepted by the smaller sum. In set notation,
`E ⊆ F` permits `sum<E> -> sum<F>`.

For ordinary value-like types, direct and stepwise widening have the same final value. C++ still runs the
payload's constructors, so a type that observes or changes itself during copying can observe different
numbers of steps. The type-level inclusion is guaranteed; value-level algebraic laws assume regular
payload behavior.

## `choice`: computation rather than payload algebra

`choice<Ts...>` wraps a sum-shaped value so that it can occupy the computation position in a pipeline.
The distinction is intentional:

- `sum<Ts...>` is data in the type algebra;
- `choice<Ts...>` is an identity-like computation over that data.

A choice may be stored as the value of `optional` or `expected`, but it is not an element of `sum`,
`pack`, or another `choice`. `choice_for` unwraps and flattens choice inputs where a choice result is
being constructed. These rules prevent a computation wrapper from entering the payload algebra and
silently bypassing sum flattening or product distribution.

## `apply`: structural elimination

`fn::apply` extends the role of `std::apply`. On the standard tuple-like domain it has the standard
meaning; libfn adds its own structural types and multidispatch:

```text
value A                   -> f(A)
pack<A, B>                -> f(A, B)
pack<>                    -> f()
tuple<A, B>               -> f(A, B)
array<A, 2>               -> f(A, A)
sum<A, B> holding A       -> apply f to that A
sum<pack<A,B>, C> holding pack<A,B> -> f(A, B)
```

The same protocol therefore covers standard tuple unpacking, libfn product unpacking, runtime selection
from a sum, and combinations of those operations.

Application is intentionally non-injective. For example, all of the following may have the same unary
call shape:

```text
A             -> (A)
pack<A>       -> (A)
tuple<A>      -> (A)
array<A, 1>   -> (A)
```

An untagged overload `f(A)` can handle them uniformly. This is often the desired, representation-
independent behavior. It does not mean that the stored alternatives have become one type.

When the exact selected alternative matters, a type-tagged application mode can pass
`std::in_place_type_t<Alternative>` before the applied arguments. For a `sum<A, pack<A>>`, its two
conceptual call shapes are then:

```text
f(in_place_type_t<A>,       A)
f(in_place_type_t<pack<A>>, A)
```

The tag names the exact outer sum alternative, before structural unpacking. Today the same information is
also available through `has_value<T>()` and `get_ptr<T>()`; tagged application provides the exhaustive,
single-visitor form.

In category-theory language, untagged application is a deliberately forgetful interpretation from stored
alternatives to argument-list shapes. Type-tagged application retains the coproduct injection.

## How products and alternatives compose

`operator&` combines successful values. Scalars form a pack:

```cpp
fn::optional<int>{1} & fn::optional<double>{2.0}
// optional<pack<int, double>>
```

Adding another value appends it to the flat product:

```cpp
fn::optional<fn::pack<int, double>>{fn::pack{1, 2.0}}
    & fn::optional<char>{'x'}
// optional<pack<int, double, char>>
```

When an operand contains a sum, product distributes over its alternatives. For example:

```text
sum<A, B> × sum<C, D>

  = sum<pack<A, C>,
        pack<A, D>,
        pack<B, C>,
        pack<B, D>>
```

At runtime only one row is held. Its exact pack type records the selected combination; `apply` spreads
that pack into an ordinary overloaded call:

```cpp
auto handle = fn::overload{
    [](A, C) { /* ... */ },
    [](A, D) { /* ... */ },
    [](B, C) { /* ... */ },
    [](B, D) { /* ... */ },
};
```

This is multidispatch without a separate dispatch table. The cartesian expansion happens in the result
type, runtime sum selection chooses a row, and C++ overload resolution chooses the handler.

The sum of flat packs shown above is the canonical output of this distributive composition. It is not a
restriction on sums in general: `sum<A, pack<A>, tuple<A>>` is valid and preserves all three exact
alternatives.

Mathematically, this is the familiar distributive correspondence:

```text
(A + B) × (C + D)
  ≅ (A × C) + (A × D) + (B × C) + (B × D)
```

libfn materializes the right-hand side as a normalized sum of flat packs.

## Graded errors

An ordinary monad has a type constructor `M<A>`, a pure operation, and a sequencing operation usually
called `bind` or `and_then`:

```text
pure : A -> M<A>
bind : M<A> -> (A -> M<B>) -> M<B>
```

A *graded monad* adds a type-level description of an effect:

```text
pure : A -> M_empty<A>
bind : M_E<A> -> (A -> M_F<B>) -> M_(E union F)<B>
```

In libfn, the clean instance is an expected value whose error is a sum:

```cpp
fn::expected<A, fn::sum_for<E...>>
```

The error alternatives are the grade. A successful computation with no possible error has error type
`sum<>`; sequencing two stages unions their error sets:

```cpp
auto parse(std::string_view)
    -> fn::expected<int, fn::sum<parse_error>>;

auto lookup(int)
    -> fn::expected<record, fn::sum<missing, io_error>>;

auto result = parse(text) | fn::and_then(lookup);
// expected<record, sum_for<parse_error, missing, io_error>>
```

The programmer writes each local failure type once. Composition derives the complete error set.

The grade algebra is the same normalized union used by `sum_for`. Repeating an error type does not create
a second positional alternative:

```text
E union E = E
```

Widening is *effect approximation*: a computation known to fail only with `E` may safely be viewed as one
that can fail with `E` or `F`.

`fn::optional` remains an ordinary Maybe-like monad with sum-aware extensions; `fn::choice` is an
identity-like computation wrapper over a sum payload. Neither has the separate `M_E<A>` shape that makes
the expected error channel a graded monad in the precise sense used above.

For programmers, the practical rule is simple: `and_then` composes computations sequentially and grows
the error type by union, while `operator&` composes successful values in parallel into packs and
distributes their alternatives for multidispatch.

## The compact model

The whole design can be remembered as five rules:

1. `sum_for` forms a canonical set of exact alternative types by union.
2. `pack` forms a flat ordered sequence; `append` concatenates packs without sorting or deduplication.
3. `pack<>` is the empty product and nullary argument list; `sum<>` is the uninhabited empty alternative
   set.
4. `operator&` distributes products over sums, producing the combinations needed by multidispatch.
5. `apply` separately turns the selected stored value into function arguments; equal call shapes never
   imply equal stored types.

For `expected<A, sum<Es...>>`, a sixth rule connects this algebra to effects: `and_then` unions the error
sets contributed by sequential stages.

## Further reading

The programming model of effect sets, union, subeffecting, and graded bind follows Dominic Orchard and
Tomas Petricek, [“Embedding effect systems in Haskell”](https://www.doc.ic.ac.uk/~dorchard/publ/haskell14-effects.pdf),
especially Sections 1–3.

For the categorical definition of a graded monad as a lax monoidal functor, and its relationship to
parameterised monads, see Dominic Orchard, Philip Wadler, and Harley Eades III,
[“Unifying graded and parameterised monads”](https://arxiv.org/pdf/2001.10274), especially Definition 21.

The whole family `E -> sum<E...>` together with its widening conversions resembles a graded object: the
individual `sum<A,B>` is one component, while the inclusions between components carry the structure. The
relevant definitions of graded objects and law-governed coercions appear in Dylan McDermott and Tarmo
Uustalu, [“Flexibly Graded Monads and Graded Algebras”](https://dylanm.org/flexibly-graded-monads.pdf),
Definitions 1, 2, and 9. This is useful validation vocabulary; libfn does not claim to instantiate that
paper's flexibly graded construction.

Finally, C++26 type ordering is proposed in
[P2830, “Type and variable template argument ordering”](https://wg21.link/P2830). Its motivation cites
libfn's need to canonicalize sets of types.
