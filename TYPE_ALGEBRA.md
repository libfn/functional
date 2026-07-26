# Type algebra and functional composition in libfn

A C++20 functional programming library, `libfn` lets the compiler derive the static shape of a computation alongside its values. Rather than collapsing failure into one wide error type, `libfn` tracks the precise algebraic combinations of success, alternative, and error states during composition.

The library operates on two payload types and four computation carriers:

- `pack`: Product type containing all fields; a tuple-like data structure.
- `copack`: Canonical coproduct containing exactly one alternative; a variant-like disjoint set of types.
- `optional`: Computation yielding a value or empty.
- `expected`: Computation yielding success or error.
- `choice`: Never-failing computation holding one of several alternatives.
- `just`: Never-failing computation yielding a single value or a `void`.

Composition operations include `transform` (mapping), `transform_error` (error mapping), `and_then` (sequential monadic binding), `or_else` (recovery), `operator&` (conjunction / simultaneous product composition), `operator|` (disjunction / simultaneous sum composition), and the n-ary folds `fn::conjoin` and `fn::disjoin`. Elimination is `apply` (multidispatch).

### Member vs. Pipeline Syntax

Some operations are exposed in two forms:

- **Member functions** (e.g., `.transform()`, `.and_then()`, `.apply()`) called directly on a carrier or payload (e.g., `ex.transform(f)`, `cp.apply(f)`).
- **Pipeline functors** in namespace `fn` (e.g., `fn::transform`, `fn::and_then`) applied via `operator|` (e.g., `ex | fn::transform(f)`).

There are also pipeline functors (`recover`, `fail`, `filter`, `inspect`, `inspect_error`, `discard`) which have no member spelling, and member functions (`apply`, `apply_r`, `apply_type`) which have no pipeline spelling.

The `operator|` carries two meanings, told apart by its right operand: a pipeline functor on the right feeds the carrier into that operation, while another carrier on the right is disjunction (explained in Section 7).

Freestanding `fn::apply(f, args...)` is the general multidispatch entry point: it accepts any mix of scalars, tuple-like structures, `pack`s and `copack`s, unpacking products and dispatching over alternatives in a single call. Do not confuse with `pfn::apply`, which is a polyfill for the C++26 `std::apply`, meant for C++20 compilers. The `fn::apply` is an extension on top of `pfn::apply`.

In prose, we omit prefixes (writing `apply`, `transform`, `and_then`, `expected`, `pack`) when referring to both forms or core vocabulary types generally.

### Storage Shape vs. Call Shape

Although different types can behave identically during application, they remain strictly distinct in memory. For example, `pack<A, B>`, `std::tuple<A, B>`, and `std::pair<A, B>` all unpack into the same call shape `f(a, b)` during `apply`, but they are separate C++ types with distinct layouts. Application does not silently convert or unify types on the storage side.

To illustrate these concepts, the examples in this document use a reusable set of value and error types:
<!-- note: keep the following self-explanatory types out of the code quotation block below:
     Error, OtherError, A, B, C, D -->
<!-- sync-example-types-def -->
```cpp
struct UserId {};
struct User {};
struct FilePath {};
struct MaximumSize {};
struct BlockSize {};

struct NotANumber {};
struct OutOfRange {};
struct Missing {
  auto operator<=>(Missing const &) const = default;
};
struct IoError {};
struct BadSyntax {};
struct UnknownKey {};
```

## 1. Why compose types as well as values?

In idiomatic C++, error handling usually means picking one application-wide error enumeration, a giant `std::variant`, or throwing exceptions. If a function only ever fails due to one specific error, returning a large application-wide error variant discards the precise bounds of what the function can actually do.

With `libfn`, the compiler derives an exact, graded error pipeline. Consider parsing, validating, and loading a user:

<!-- sync-example-graded-pipeline -->
```cpp
auto parse_id(std::string_view) -> fn::expected<UserId, fn::copack<NotANumber>>;
auto validate(UserId) -> fn::expected<UserId, fn::copack<OutOfRange>>;
auto load(UserId) -> fn::expected<User, fn::copack_for<IoError, Missing>>;

auto graded_pipeline(std::string_view sv) -> void
{
  auto pipeline = parse_id(sv) | fn::and_then(validate) | fn::and_then(load);

  // The exact derived error union is recorded in the type:
  static_assert(
      std::same_as<decltype(pipeline),
                   fn::expected<User, fn::copack_for<IoError, Missing, NotANumber, OutOfRange>>>);
}
```

The resulting `expected` statically records that the pipeline yields a `User` on success, or fails with exactly one of `NotANumber`, `OutOfRange`, `Missing`, or `IoError`. This exact union accumulates automatically via `and_then` composition.

### What Does "Graded" Mean?

Standard monads are rigid: an `expected<T, E>` requires every step in a pipeline to return the identical error type `E`. This forces you to define a monolithic global error union upfront.

A **graded monad** relaxes this restriction. Each operation is indexed by a "grade"—a set representing all its specific possible errors (its "effects") by means of `copack`, which is a disjoint set of types. As you chain operations, the compiler automatically adds these grades to the set.
The resulting error type is **graded**: it expands (or narrows during recovery) to match the *exact* subset of errors possible in the compiled path, providing strict static effect tracking (subeffecting) with zero boilerplate.

You may also use `copack` on a value side of most carriers (except for `just<copack<Ts...>>`, which must be spelled `choice<Ts...>`). Grading is opt-in for chaining: a `copack` on the error side enrols an `expected` in this union arithmetic, while a plain `expected<T, E>` holds every step to the identical error type `E`. A `copack` on the value side enrols an `expected` or `optional` into the same arithmetic on values.

> [!TIP]
>
> ### Mathematical note — graded monads
>
> Formally, a graded monad (also known as an effect monad) indexes a family of monadic carriers over a partially ordered monoid (pomonoid) of effects $(\mathcal{E}, \bullet, I, \le)$.
>
> In `libfn` that pomonoid is carried by the finite sets of C++ types:
>
> - **Grades ($\mathcal{E}$)**: Finite sets of alternative types (errors).
> - **Monoidal multiplication ($\bullet$)**: Set union ($\cup$), representing effect accumulation.
> - **Identity ($I$)**: The empty set ($\emptyset$), representing the zero-error/never-failing state.
> - **Partial order ($\le$)**: Subset relation ($\subseteq$), which licenses effect approximation (subeffecting / widening).
>
> Taking union as the multiplication makes this monoid commutative and idempotent — a join-semilattice — whose order is the one it induces, $E \le F \iff E \cup F = F$. Those are exactly the `copack` laws of Section 2, and they are why widening is coherent: grades accumulated in any order, through any intermediate supersets, land on the same normalized set.
>
> For a standard monad $M$, the binding operation maps $M\langle A\rangle \to (A \to M\langle B\rangle) \to M\langle B\rangle$. In `libfn`'s graded monad, *bind* accumulates effects across the pomonoid:
>
> $$bind : M_E\langle A\rangle \to (A \to M_F\langle B\rangle) \to M_{E \cup F}\langle B\rangle$$
>
> Section 9 identifies the lax monoidal functor this determines.

The same type precision extends to computations combined side by side rather than in sequence. Conjunction (`operator&`) evaluates independent computations and bundles them into a single carrier holding a `pack` of the successful values over a `copack` of the exact possible errors; disjunction (`operator|`) is its dual. Sections 6 and 7 cover both.

### The Two Cooperating Mechanisms

Behind these precise compiled types are two independent mechanisms that cooperate to derive and eliminate these shapes:

1. **Type algebra** records and normalizes the exact stored C++ types using `pack` and `copack` as you compose operations.
2. **The application protocol** uses `apply` and ordinary C++ overload resolution to unpack those stored values and route them to your functions or lambdas.

To route multiple alternative paths inside `apply`, the library provides `fn::overload`, which fuses unrelated lambdas into a single overload set.

These derived types are the explanation of the library's design, not an internal template-metaprogramming implementation detail.

## 2. Types as an algebra: zero, unit, alternatives, and products

To derive strict programmatic shapes, `libfn` uses an algebraic vocabulary over types.

- `copack<>` represents **0** (Zero) - an uninhabited type.
- `pack<>` represents **1** (Unit) - a type with exactly one state.
- `copack<A, B>` represents **A + B** (Alternatives) - a coproduct where exactly one alternative is present.
- `pack<A, B>` represents **A × B** (Products) - a type where all fields are present simultaneously.

These states can also be used to express the standard vocabulary types:

- `std::optional<T>` ≅ **1 + T** (It is either empty/unit or contains `T`, similar to `copack_for<T, std::nullopt_t>`)
- `std::expected<T, E>` ≅ **T + E** (It contains either success `T` or error `E`, similar to `copack_for<T, fn::unexpected<E>>`)

The symbol ≅ indicates an equivalent state shape (an information-level correspondence), not `std::same_as`. `std::optional<T>` is its own distinct C++ type, but algebraically, it behaves as `1 + T`.

### Zero is not unit

In `libfn`'s algebra, zero and unit are strictly separated:

- `copack<>` is uninhabited: you cannot construct it. Algebraically, it is `0`.
- `pack<>` is the one nullary product value. You can construct it via `pack<>{}` or `fn::as_pack()`. Algebraically, it is `1`.

Because `pack<>` exists, applying a callable to it invokes a nullary function. Because `copack<>` is uninhabited, providing a callback over `copack<>` is statically proven to be unreachable code (dead code).

In C++, `void` is often conflated with empty state, but algebraically, `void` is a unit type `1`, similar to `pack<>`.

Consider the difference in these carrier states:

| Computation | Meaning |
| ----------- | ------- |
| `expected<T, copack<>>` | An expected value that **cannot fail** because its error state is uninhabited. |
| `expected<copack<>, E>` | An expected value that **cannot succeed** because its success state is uninhabited. |
| `optional<copack<>>` | An optional that **must be empty**, as its value state is uninhabited. |
| `expected<void, E>` | An expected that yields **no value on success**, but can fail with `E`. |

### Copacks use set semantics

A major feature of `libfn` is that `copack` forms canonical sets of types, in contrast to the positional indexing of `std::variant`. When you combine types into a coproduct, `fn::copack_for` guarantees deduplication, flattening, and a canonical ordering.

<!-- sync-example-copack-set-semantics -->
```cpp
auto test_copack_set_semantics() -> void
{
  using SetA = fn::copack_for<NotANumber, OutOfRange>;
  using SetB = fn::copack_for<Missing, OutOfRange>;

  // Flattening, deduplication, and reordering happen automatically:
  using Union = fn::copack_for<SetA, SetB, Missing, Missing, fn::copack<IoError>>;

  static_assert(std::same_as<Union, fn::copack_for<IoError, Missing, NotANumber, OutOfRange>>);
}
```

> [!NOTE]
>
> ### Note — copack vs. copack_for
>
> In C++, there is no native language feature to represent a "set of types." Template parameter lists are always positional, variadic sequences. Syntactically, this means `copack<A, B>` and `copack<B, A>` would be completely distinct types—a property that directly violates the mathematical commutative law of set union.
>
> To enforce set semantics at compile time, `libfn` defines one canonical representation and rejects any instantiation that diverges from it:
>
> - **`copack`** is the core storage type. It requires its template parameters to already be flat, unique, and sorted in a strict total order over types. The order is derived from the compiler's own spelling of each type; a build targeting C++26 with `LIBFN_CXX26` set will use `std::type_order` to derive the order of types, while the default build uses a type sorting mechanism based on compiler-specific type names (since these two orders may differ, each defines a distinct ABI namespace). If you attempt to instantiate `copack` manually with out-of-order parameters (such as `copack<B, A>` when `A` precedes `B` in that order) or with nested copacks (such as `copack<A, copack<C, D>>`), the compiler will reject the instantiation as outright ill-formed.
>
> - **`copack_for`** is the user-facing type alias. It accepts any list of types (out-of-order, duplicates, nested copacks), performs the flattening, deduplication, and canonical sorting, and resolves to the validated `copack`.
>
> In an API signature the two are the same type, since the alias resolves to `copack`; in prose and code, `copack` names the normalized *state shape* and `copack_for` the *construction utility*. `choice` and `choice_for` stand in the same relation, over the alternatives of the underlying `copack`.
>
> **Best practice**: spell `copack_for` / `choice_for` rather than `copack` / `choice`, so that no spelling in your project is tied to one compiler's ordering.

The laws governing `copack` are:

- **Commutative**: Order of types does not change the resulting set.
- **Associative**: Nesting copacks is equivalent to flattening them.
- **Idempotent**: Duplicate types are collapsed into one.
- **Identity**: `copack<>` acts as the union unit (adding `copack<>` changes nothing).

Idempotence means a set cannot carry positional meaning: `copack_for<bool, bool>` is one alternative, `copack<bool>`, not two. Types combined into a `copack` should therefore be strongly typed tag structs or distinct domain objects, never generic primitives whose meaning depends on where they sit. Two *distinct* types that the ordering mechanism cannot distinguish are a different matter: the library rejects them outright via a compile-time assertion rather than silently merging them, guaranteeing that no type is ever silently lost.

### The algebra is strictly opt-in

The library relies on explicit consent. It does not silently reinterpret arbitrary C++ types as products or coproducts:

- `std::tuple` remains a standard tuple.
- `std::variant` does not acquire `copack` set semantics.
- Tuple-like participation in `apply` changes call shape applicability, but it does not change the stored type identity.
- A plain `expected<T, E>` does not automatically become graded.

To invoke the algebra, you use the opt-in mechanisms provided by the library:

- Direct construction of `pack` and `copack_for`.
- Explicit conversions via `fn::as_pack` and `fn::as_copack`.
- Member helpers for explicit type lifting (detailed in Section 9).

If a side is already a `copack` or `pack`, forwarding it behaves naturally without nesting. A `copack` on an `expected`'s error side is the opt-in to error-set unioning: the fundamental monadic operations introduce no grade of their own, so `and_then` widens an error side that is graded already and rejects a plain one differing from its callback's. Section 9 gives the exact promotion rules.

## 3. The computation carriers

To model computation and manage control flow (success, failure, alternatives, and empty states), `libfn` uses **computation carriers** (often called "monadic types"). The library defines exactly four carrier families, divided by their fallibility and payload capacity:

### The fallible carriers

- **`optional<T>`** (representing $T + 1$): A carrier that either holds a successful value of type `T` or is empty (`std::nullopt`).
- **`expected<T, E>`** (representing $T + E$): A carrier that either holds a successful value of type `T` or an error of type `E`.
  - As explained below, `expected` can be also infallible, if its error side is a `copack<>`.

*(Note: `optional` and `expected` are the `fn` extensions of the standards-conforming `pfn` polyfills; Section 15 covers the two layers.)*

### The infallible (identity) carriers

- **`just<T>`**: Always contains a single successful value of type `T`.
- **`choice<Ts...>`**: Always contains one of several selected alternatives, representing the complete state space of the computation.

Because `choice` implies that an alternative is always present, `choice<>` is incomplete: an always-present selected alternative requires at least one alternative to exist.

Additionally, the infallible state **`expected<T, copack<>>`** (representing $T + 0 \cong T$) can never fail because `copack<>` represents the initial zero object **0** (the uninhabited type). Lacking any possible error alternatives, it acts as an infallible, graded unit context. Since it is a specialized state of `expected` rather than a unique template, it is classified under the same computation carrier.

Together, `just`, `choice` and `expected<T, copack<>>` form the **identity cluster**.

> [!NOTE]
>
> ### Note — `just<copack<Ts...>>` is spelled `choice<Ts...>`
>
> To carry several alternatives that cannot fail, use `choice<Ts...>`: a computation that always succeeds, with a result that is one of `Ts...`. Spelling the same shape as `just<copack<Ts...>>` does not compile — `just` rejects a `copack` payload with `"a just over a copack is spelled choice"` — so the shape has one canonical spelling, exactly as `copack` has one canonical form for its alternatives.

These carriers constrain their payloads. While `optional<T&>` is supported as a standard-conforming exception, other carriers reject raw reference types outright; references must be wrapped inside a `pack` (detailed in Section 4).

### Carriers have control flow; raw data does not

Raw type algebraic constructs—such as a product `std::tuple` or a sum `std::variant`—are purely passive data layouts. They contain no intrinsic control flow, no concept of short-circuiting, and no built-in notion of "success" versus "failure."

To compose computations, we must wrap these values inside computation carriers. When we perform product composition (conjunction) or sum composition (disjunction) later in this document, we are not combining raw data; we are composing carriers. The carrier manages the propagation of success values and the short-circuiting of failures.

### Carrier Bridging: Interoperable Pipelines

Because these carriers represent different computational contexts, pipelines often need to transition between them. `libfn` licenses explicit **cross-carrier bridging** via pipeline-scoped operations using `operator|`.

Standard fallible carriers can bridge to each other on their error/empty recovery paths via pipeline functor `fn::or_else` (e.g., `expected` to `optional`, or vice versa). This is safe because on the success path, the successful value is preserved and bypasses the recovery callback. The transition only occurs on the handled failure branch, allowing you to gracefully convert a missing value into a concrete error, or decay a detailed error into an empty state:

<!-- sync-example-test-failure-bridge -->
```cpp
auto test_failure_bridge(fn::expected<int, IoError> ex, fn::optional<int> opt) -> void
{
  // Fallible carriers can bridge to each other on the failure/empty recovery path
  auto expected_to_optional = ex | fn::or_else([](IoError) { return fn::optional<int>{}; });
  static_assert(std::same_as<decltype(expected_to_optional), fn::optional<int>>);

  auto optional_to_expected = opt | fn::or_else([]() { return fn::expected<int, IoError>{100}; });
  static_assert(std::same_as<decltype(optional_to_expected), fn::expected<int, IoError>>);
}
```

The member function `.or_else` (or `.and_then`, see Sections 8 and 10) cannot be used for cross-carrier bridging, as such operation would require coupling between different carriers. Since pipeline functors are a layer above carriers, they can provide such operation without undue coupling.

Identity carriers bridge in the other direction, on the success path; Section 10 covers that together with the identity cluster.

## 4. The sum and product payloads: pack and copack

While the computation carriers manage control flow and fallibility, modeling more complex algebraic structures—such as multi-field products or multi-alternative disjoint sums—requires specialized payload types. `libfn` provides two core vocabulary types for this:

### pack: all fields are present

A `pack` acts like a standard C++ tuple (`std::tuple`) by storing multiple fields and supporting standard tuple protocol (`get`, `tuple_size`, `tuple_element`, structured bindings), and an `append` mechanism. However, unlike standard tuples, `libfn` packs are strictly flat: a `pack` is not a valid element of a `pack`, so `append`-ing one splices its fields into the outer pack rather than nesting it.

To explicitly lift values into a `pack` (which is useful when conjoining scalars with other packs or copacks), use `fn::as_pack(...)`. When called without template parameters, `as_pack` is deduction-only and preserves the value category of its arguments: `as_pack(42)` yields `pack<int>`, whereas calling `as_pack(x)` on an lvalue `x` yields `pack<int&>` (a reference rather than a copy).

Spelling the template parameters instead (e.g., `as_pack<bool, int>(x, d)`) takes deduction out of the picture: each argument is passed as the type you named, enabling implicit conversions to happen at the call boundary. A reference element becomes something you ask for explicitly — `as_pack<int const&>(x)` yields `pack<int const&>`. Note that partial template spelling is not supported; all element types must be spelled out explicitly if template parameters are specified.

<!-- sync-example-test-pack -->
```cpp
auto test_pack(int x = 12, double d = 3.14) -> void
{
  fn::pack p{UserId{}, User{}};         // CTAD
  [[maybe_unused]] auto [id, user] = p; // Structured bindings work naturally

  // Ordered, non-deduplicated fields
  using P = fn::pack<UserId, User, User>;
  static_assert(std::tuple_size_v<P> == 3);

  // Found via ADL (like std::get)
  using std::get;
  static_assert(std::same_as<decltype(get<0>(p)), UserId &>);

  // Splicing scalars or other packs via append:
  auto row = fn::pack{UserId{}}.append(FilePath{});
  auto wider = std::move(row).append(fn::pack{true, 3});

  static_assert(std::same_as<decltype(wider), fn::pack<UserId, FilePath, bool, int>>);

  // Explicitly lifting a single scalar value into a pack:
  auto lifted_lvalue = fn::as_pack(x);
  static_assert(std::same_as<decltype(lifted_lvalue), fn::pack<int &>>);

  auto lifted_rvalue = fn::as_pack(42);
  static_assert(std::same_as<decltype(lifted_rvalue), fn::pack<int>>);

  // Spelling the element type explicitly can be used to opt out of reference preservation:
  auto copied = fn::as_pack<int>(x);
  static_assert(std::same_as<decltype(copied), fn::pack<int>>);
  // ... or to force a specific reference type (subject to parameter binding rules):
  auto referenced = fn::as_pack<int const &>(x);
  static_assert(std::same_as<decltype(referenced), fn::pack<int const &>>);

  // The explicit form also coerces - the argument converts at the call boundary:
  auto coerced = fn::as_pack<bool, int>(x, d);
  static_assert(std::same_as<decltype(coerced), fn::pack<bool, int>>);
}
```

### copack: one exact alternative is present

As a payload, a `copack` models variant-like structures — i.e. a discriminated union of types.

When you evaluate a `copack` via its member `apply` function, it selects the active alternative stored inside the coproduct and passes it to your callback. Because `copack` is self-flattening, you are guaranteed that there is never a nested `copack` inside. However, a selected alternative that is itself tuple-like—a `pack`, `std::tuple`, or `std::array`—is unpacked one level into its immediate constituents, which reach your callback as separate arguments. Because normalized shapes are sums of products, one level is all they need: your callback receives the product's fields directly as function arguments.

To explicitly lift a single scalar value into a single-alternative coproduct, use `fn::as_copack(value)`. Unlike `as_pack`, it always decays: a `copack` alternative can never be a reference. When a `copack` contains exactly one alternative, it is **singular** and supports direct value extraction via the `get` utility (resolvable via ADL), which propagates references with the same semantics as `apply`.

You can lift a `pack` into a `copack` (including `pack` holding references), however you cannot store `copack` inside a `pack`. There is algebraic equivalence between a hypothetical `pack` containing a `copack` (which is disallowed) and a specific shape of `copack` containing a `pack` — see Section 6 for details.

<!-- sync-example-test-copack -->
```cpp
struct IntegerToken {};
struct StringToken {};

auto test_copack() -> void
{
  static constexpr fn::copack_for<IntegerToken, StringToken> token = IntegerToken{};

  // Member apply eliminates the copack by routing the active alternative to an overload set:
  static constexpr auto value
      = token.apply(fn::overload{[](IntegerToken) { return 1; }, [](StringToken) { return 2; }});
  static_assert(value == 1);

  // Storing a pack inside a copack is allowed, including a pack holding a reference:
  auto cpr = fn::as_copack(fn::as_pack<int const &>(value));
  static_assert(std::same_as<decltype(cpr), fn::copack<fn::pack<int const &>>>);

  // Singular lift and direct value extraction (only allowed for singular copacks):
  auto cp = fn::as_copack(42);
  using std::get;
  static_assert(std::same_as<decltype(get(cp)), int &>);
}
```

A fundamental safety guarantee of `copack` is **exhaustive matching**. Every operation that evaluates a `copack` — mapping with `transform`, eliminating with `apply` — delegates to the same underlying multidispatch implementation. This implementation forces compile-time exhaustiveness: if your callback or overload set fails to handle even one of the possible alternatives stored in the `copack`, the compilation is rejected as ill-formed. Direct `get` extraction is disallowed for multi-alternative `copack` types for a related reason: which alternative is active is a run-time fact, so a `get` over several alternatives has no single static result type to return. Extraction has to go through dispatch, and dispatch is exhaustive.

## 5. Mapping values and errors

Mapping allows you to change the contained data without altering the structural success/failure shape of the computation. `libfn` uses `transform` (functor *map*) to operate on the successful channel, and `transform_error` for the error channel.

<!-- sync-example-mapping-values-and-errors -->
```cpp
auto mapping_values_and_errors() -> void
{
  fn::expected<UserId, fn::copack_for<Missing, IoError>> ex{};

  auto mapped_val = ex | fn::transform([](UserId) { return User{}; });
  static_assert(
      std::same_as<decltype(mapped_val), fn::expected<User, fn::copack_for<IoError, Missing>>>);

  auto mapped_err = ex
                    | fn::transform_error(fn::overload{[](Missing) { return BadSyntax{}; },
                                                       [](IoError e) { return e; }});

  static_assert(
      std::same_as<decltype(mapped_err), fn::expected<UserId, fn::copack_for<BadSyntax, IoError>>>);
}
```

Key principles of mapping:

- `transform` stays within the carrier: the member form never leaves its own carrier family.
- Success and error states are rigidly preserved.
- A bare `copack` has a member `transform`, allowing mapping across alternatives; being data rather than a carrier, it takes no pipeline functor.
- Heterogeneous branch results inside `transform_error` or `transform` form a normalized result `copack`.
- Applying an error-side operation like `transform_error` to a carrier that has no error side (like `just` or `choice`) is rejected by the compiler.
- If a side is uninhabited (`copack<>`), the transformation is well-formed but vacuous: both the member and the pipeline form are proven unreachable, and the callback is never instantiated. This covers `optional<copack<>>` and `expected<copack<>, E>` on the value side, and `expected<T, copack<>>` on the error side.

> [!TIP]
>
> ### Mathematical note — functorial action on the initial object
>
> In `libfn`, `transform` implements the functorial map ($fmap$). Its action on the initial object $0$ — the uninhabited `copack<>` — is forced rather than chosen: for every object $U$ there is exactly one morphism $0 \to U$, so a callback out of $0$ carries no information. Any two candidates denote the same morphism, and the result is determined without consulting either; Haskell spells this unique morphism `absurd :: Void -> a`.
>
> Nothing can therefore be asked of the callback — not even that it be callable, the same vacuity Section 10 describes for `or_else`.
>
## 6. Product composition with operator& (conjunction)

Conjunction runs independent computations and keeps both results. `a & b` succeeds only if both operands succeed: the values multiply into a `pack`, and the errors add into a `copack`. Where both name the same error type, the error side is left as it was.

<!-- sync-example-operator-and-composition -->
```cpp
auto operator_and_composition(fn::expected<int, Error> a, fn::expected<bool, OtherError> b) -> void
{
  auto result = a & b;
  static_assert(std::same_as<decltype(result),
                             fn::expected<fn::pack<int, bool>, fn::copack_for<Error, OtherError>>>);
}
```

The runtime semantics are exact:

- The result type records every error either operand can produce; at runtime it holds at most *one* of them — the leftmost failing operand's.
- Both operands are fully constructed before `operator&` runs, because C++ evaluates operands eagerly. This is an error-selection rule, not short-circuiting: the operator makes nothing lazy or parallel.

### Conjunction over data

Unlike disjunction (Section 7), `operator&` also applies to the payload types. When either operand is a `copack`, it performs a Cartesian distribution, yielding a `copack` of `pack`s; a `pack` on the opposite side simply widens each of those `pack`s.

<!-- sync-example-cartesian-distribution -->
```cpp
auto test_cartesian_distribution(fn::copack_for<A, B> ab, fn::copack_for<C, D> cd) -> void
{
  // Cartesian distribution of copacks: (A + B) x (C + D) = (A x C) + (A x D) + (B x C) + (B x D)
  auto result1 = ab & cd;
  static_assert(
      std::same_as<decltype(result1),
                   fn::copack_for<fn::pack<A, C>, fn::pack<A, D>, fn::pack<B, C>, fn::pack<B, D>>>);

  // Cartesian distribution of a pack and a copack: (A x B) x (C + D) = (A x B x C) + (A x B x D)
  constexpr fn::pack<A, B> Pab = {A{}, B{}};
  auto result2 = Pab & cd;
  static_assert(
      std::same_as<decltype(result2), fn::copack_for<fn::pack<A, B, C>, fn::pack<A, B, D>>>);
}
```

A bare `scalar & scalar` is not part of the algebra: for class types it fails to compile, and for built-in types like `int` it resolves to the built-in bitwise `AND`. Lift one side first — `fn::as_pack(a) & b` — as `operator&` dispatches on its left operand.

The n-ary fold `fn::conjoin(...)` covers both worlds, in two modes:

- if **all** of its arguments are computation carriers, it folds them as a monadic conjunction, producing exactly what cascading `operator&` produces;
- if **none** of them are carriers, it conjoins them as a data-level product.

Mixing the two in one call is rejected.

### Conjunction with the Identity Cluster

When one operand belongs to the identity cluster (detailed in Section 10), it contributes a value but never a failure:

- **Errors are unaffected**: Because identity cluster operands can never fail, they add no new alternative to the resulting error channel. A `just` or `choice` operand preserves the fallible operand's error side exactly as it was (plain or graded). An `expected<T, copack<>>` operand instead contributes its uninhabited grade to the error union: no active alternative is added, though the resulting type's error channel is promoted to a graded copack (e.g. mapping `E` to `copack<E>`).
- **Value bundling**: The value of the identity cluster operand is conjoined with the fallible operand's value channel into a `fn::pack`.
- **Unit elision**: `just<void>` and `expected<void, copack<>>` act as the product's identity unit and are completely elided from the value product (e.g., `expected<T, E> & just<void>` stays `expected<T, E>`).
- **Choice distribution**: If a `choice` operand is conjoined with a fallible carrier, the coproduct distributes through the product. This yields a `copack` of `pack`s wrapped back inside the fallible carrier.

<!-- sync-example-conjunction-with-identity-cluster -->
```cpp
auto test_conjunction_with_identity_cluster(fn::expected<int, Error> ex, fn::just<double> j) -> void
{
  // Conjoining an expected with a just
  auto res1 = ex & j;
  static_assert(std::same_as<decltype(res1), fn::expected<fn::pack<int, double>, Error>>);

  // Conjoining with a unit (just<void>) completely elides the unit
  auto res2 = ex & fn::just<void>{};
  static_assert(std::same_as<decltype(res2), decltype(ex)>);

  // Conjoining a choice causes distribution inside the carrier
  fn::choice<bool, double> ch = 1.5;
  auto res3 = ex & ch;
  static_assert(std::same_as<
                decltype(res3),
                fn::expected<fn::copack_for<fn::pack<int, double>, fn::pack<int, bool>>, Error>>);
}
```

> [!TIP]
>
> ### Mathematical note — strict monoidal structure and distribution
>
> Conjoining independent computations via `operator&` models a symmetric monoidal category $(\mathcal{C}, \otimes, I)$, with `pack` as the tensor $\otimes$ and `pack<>` as the unit $I$.
>
> - **Strictness**: because packs are normalized flat, there are no distinct-but-isomorphic spellings for the coherence maps to mediate between. $(A \otimes B) \otimes C$ and $A \otimes (B \otimes C)$ are *the same C++ type*, as are $I \otimes A$, $A \otimes I$ and $A$. The associator and unitors are therefore identities, and the structure is **strict** rather than merely coherent.
> - **Symmetry**: for the sum it is strict too — canonical ordering makes $A \oplus B$ and $B \oplus A$ literally the same `copack`, so the braiding is an identity. For the product it is not: a `pack` keeps its fields ordered and undeduplicated, so $A \otimes B$ and $B \otimes A$ are distinct types related by a genuine swap.
> - **Distributivity**: the tensor distributes over the coproduct $\oplus$ (represented by `copack`), yielding $A \otimes (B \oplus C) \cong (A \otimes B) \oplus (A \otimes C)$. This is the Cartesian distribution of `pack` over `copack` implemented statically by `libfn`.
>
## 7. Sum composition with operator| (disjunction)

Disjunction runs alternative computations and keeps the first that worked. `a | b` fails only if both operands fail: dual to conjunction, the values add into a `copack`, and the errors multiply into a `pack`. Where both name the same value type, the value side is left as it was; otherwise a `void` operand enters the sum as `pack<>`.

If either error side is graded, the product distributes over it — $(E_1 + E_2) \times F \to (E_1 \times F) + (E_2 \times F)$, the full Cartesian product when both are — yielding a canonical `copack` of `pack`s.

<!-- sync-example-operator-or-composition -->
```cpp
auto operator_or_composition(fn::expected<int, Error> a, fn::expected<bool, OtherError> b) -> void
{
  auto result = a | b;
  static_assert(std::same_as<decltype(result),
                             fn::expected<fn::copack_for<int, bool>, fn::pack<Error, OtherError>>>);
}
```

The runtime semantics are exact:

- The result type records every combination of failures; at runtime the leftmost operand holding a value wins.
- Both operands are fully constructed before `operator|` runs, because C++ evaluates operands eagerly. This is a value-selection rule, not a lazy fallback.

### Disjunction over carriers only

Unlike conjunction, disjunction has no data-level form. Neither `operator|` nor the n-ary fold `fn::disjoin(...)` accepts a `pack`, a `copack`, or a scalar, so a built-in operation such as bitwise OR on integers can never be mistaken for a disjunction.

### Disjunction with the Identity Cluster

When at least one operand belongs to the identity cluster (detailed in Section 10), the disjunction can never fail:

- The error side gains an uninhabited factor (`copack<>`), which annihilates the error product and collapses the channel entirely.
- The result is folded into a non-failing carrier: a single-valued `just<T>` if there is only one successful type, or `choice<Ts...>` if the sum is heterogeneous.

<!-- sync-example-test-disjoin -->
```cpp
auto test_disjoin(fn::expected<int, Error> a, fn::expected<bool, OtherError> b) -> void
{
  // Multiple fallible operands compose cleanly
  auto res1 = fn::disjoin(a, b);
  static_assert(std::same_as<decltype(res1),
                             fn::expected<fn::copack_for<int, bool>, fn::pack<Error, OtherError>>>);

  // Because just<double> cannot fail, the entire disjunction with infallible operands becomes total
  auto res2 = fn::disjoin(a, b, fn::just<double>{1.5});
  static_assert(std::same_as<decltype(res2), fn::choice<bool, double, int>>);
}
```

> [!TIP]
>
> ### Mathematical note — annihilation and totality
>
> Disjunction is the categorical dual of conjunction: values add along $\oplus$ where conjunction multiplies along $\otimes$, and errors multiply where conjunction unions. Both structures are strict in the sense Section 6 describes.
>
> What does *not* dualize is the role of the uninhabited `copack<>`, and that is what makes an identity operand behave so differently on the two sides. Under conjunction it is the unit of the error union, $E \cup 0 = E$, so an identity operand leaves the error channel as it found it. Under disjunction it is the **annihilator** of the error product, $E \otimes 0 \cong 0$, so a single identity operand empties the channel outright.
>
> The resulting error side is uninhabited by construction — not merely empty at runtime, but incapable of holding a value — which is what renders the whole disjunction total and folds it into the identity cluster.
>
## 8. Sequential composition with and_then

Sequential composition chains dependent operations where the success of one feeds the input of the next. In `libfn`, this is achieved using `and_then` (monadic *bind*).

A monadic carrier wraps a value. A *Kleisli arrow* is the callable passed to `and_then`, which takes a plain value and returns a monadic carrier of the same kind — or, from an infallible input, either an infallible carrier or a fallible kind it bridges to (Section 10).

The graded pipeline of Section 1 is this *bind* chained; here is the rule on its own:

<!-- sync-example-sequential-bind -->
```cpp
auto parse_numeric() -> fn::expected<UserId, fn::copack<NotANumber>>;
auto load_user(UserId) -> fn::expected<User, fn::copack<Missing>>;

auto sequential_bind() -> void
{
  auto result = parse_numeric() | fn::and_then(load_user);
  static_assert(
      std::same_as<decltype(result), fn::expected<User, fn::copack_for<Missing, NotANumber>>>);
}
```

Because the member `.and_then` cannot change carrier family (Section 3), the *Kleisli arrow* it accepts must satisfy a strict "same-kind" rule on its return type:

- An `optional` binds to an `optional`.
- A plain `expected<T, E>` binds to an `expected<U, E>` (retaining its exact plain error type) or, via singular lift, to an `expected<U, copack<E>>` (which transitions it into a graded context, detailed in Section 9).
- A copack-graded `expected` can union heterogeneous error sets (as demonstrated above).
- A copack-valued input can join heterogeneous successful branch types into a normalized `copack`.
- Exact branch convergence preserves the exact type without creating duplicate union states.
- All-`void` branches join cleanly to `void`, but mixed void/non-void branches are rejected.
- A bare callback result belongs to `transform`, not `and_then`.

The library formalizes this "same-kind" contract via the `fn::same_kind` concept, which lets generic templates probe whether two carrier types belong to the same monadic family:

<!-- sync-example-test-same-kind -->
```cpp
static_assert(fn::same_kind<fn::optional<int>, fn::optional<User>>);
static_assert(fn::same_kind<fn::expected<int, IoError>, fn::expected<User, IoError>>);
static_assert(!fn::same_kind<fn::expected<int, IoError>, fn::expected<User, Missing>>);
```

## 9. Graded expected: exact error sets

Grading an `expected` provides exactly bounded error sets. When an outer computation holds a coproduct of successful values, and each value requires a different operation to proceed, `libfn` derives a single, normalized `expected` shape.

Consider a configuration reader that parses a loosely typed file into specific valid structural alternatives: `MaximumSize`, `FilePath`, or `BlockSize`.

<!-- sync-example-config-pipeline -->
```cpp
auto read_config() -> fn::expected<fn::copack_for<MaximumSize, FilePath, BlockSize>,
                                   fn::copack_for<BadSyntax, UnknownKey>>;

auto config_pipeline() -> void
{
  auto validated
      = read_config()
        | fn::and_then(fn::overload{
            [](MaximumSize v) { return fn::expected<MaximumSize, fn::copack<OutOfRange>>{v}; },
            [](FilePath v) { return fn::expected<FilePath, fn::copack<Missing>>{v}; },
            [](BlockSize v) { return fn::expected<BlockSize, fn::copack<OutOfRange>>{v}; }});

  // The result exactly bounds both the successful paths and the error paths
  static_assert(
      std::same_as<decltype(validated),
                   fn::expected<fn::copack_for<BlockSize, FilePath, MaximumSize>,
                                fn::copack_for<BadSyntax, Missing, OutOfRange, UnknownKey>>>);
}
```

Two independent joins occurred during `and_then`:

1. The successful branch values formed the normalized value copack.
2. The existing outer errors (`BadSyntax`, `UnknownKey`) and the new branch errors (`OutOfRange`, `Missing`) formed the normalized error copack.

This unioning is what allows different grades of `expected` to share the same carrier family. While standard, un-graded `expected<T, E>` admits only the identical error type `E` (or its singular lift `copack<E>`) to participate in monadic *bind* (meaning `expected<int, IoError>` and `expected<User, Missing>` are **not** `same_kind`), any two **graded** `expected` types are considered `same_kind` regardless of how their individual error sets differ, as the compiler can always derive their union:

<!-- sync-example-test-same-kind-graded -->
```cpp
static_assert(
    fn::same_kind<fn::expected<int, fn::copack<IoError>>, fn::expected<User, fn::copack<Missing>>>);
```

Value joining and error grading are independent: branch values can join while the error side stays plain, as in `expected<copack_for<A, B>, E>`.

During sequential composition, `libfn` derives the promoted type from the `copack` you supply:

- In `and_then` (success binding), a plain error type `E` is promoted to `copack<E>` if the returning **error type** of the callback is `copack<E>`.
- In `or_else` (recovery/error binding), a plain success type `T` is promoted to `copack<T>` if the returning **success type** of the callback is `copack<T>`.

An un-graded computation thus enters a graded pipeline without manual lifting.

If you need to perform this promotion explicitly on the carrier itself before entering a composition, `libfn` provides direct member helpers:

- `.copack_error()` on `expected` explicitly lifts the error, transforming `expected<T, E>` to `expected<T, copack<E>>`.
- `.copack_value()` on `expected` explicitly lifts the success value, transforming `expected<T, E>` to `expected<copack<T>, E>`.
- `.copack_value()` on `optional` symmetrically lifts the value, transforming `optional<T>` to `optional<copack<T>>`.

These helper methods provide a compact, explicit alternative to the pipeline promotions:

<!-- sync-example-test-explicit-lifting -->
```cpp
auto test_explicit_lifting(fn::expected<User, IoError> result, fn::optional<User> opt) -> void
{
  // Explicitly lift the error side of expected:
  auto graded_err = std::move(result).copack_error();
  static_assert(std::same_as<decltype(graded_err), fn::expected<User, fn::copack<IoError>>>);

  // Explicitly lift the value side of expected:
  auto graded_val = std::move(result).copack_value();
  static_assert(std::same_as<decltype(graded_val), fn::expected<fn::copack<User>, IoError>>);

  // Explicitly lift the value side of optional:
  auto graded_opt = std::move(opt).copack_value();
  static_assert(std::same_as<decltype(graded_opt), fn::optional<fn::copack<User>>>);
}
```

Recovery via `or_else` behaves symmetrically. It handles input error alternatives and joins any new errors produced by the recovery branches, while preserving the successful value path. Heterogeneous recovery values require a suitable copack-valued input. Any original error handled by a branch is removed from the resulting grade unless a branch explicitly re-returns it. When there are no error alternatives to handle—such as in `expected<T, copack<>>`—there is nothing to recover from: the callback is neither invoked nor instantiated.

### Widening is subeffecting

In accordance with the subeffecting principles of graded monads (Section 1), a narrow error set can be safely widened during composition, but narrowing requires explicit mitigation. Implicit narrowing (without handling the removed errors) is unsafe and rejected by the compiler.
However, you can safely narrow or collapse an error grade at any point by explicitly handling and mapping the errors using `transform_error`. Because `transform_error` on a graded `expected` forces exhaustive matching over all possible alternatives, you can map multiple diverse error types into one common error type — the grade collapses to the singular `copack` of it — or into a narrower `copack`, safely reducing the static error grade of your pipeline.

The bottom error grade is `copack<>`:

```cpp
template <typename T>
using cannot_fail_t = fn::expected<T, fn::copack<>>;
```

This computation cannot fail, but it is algebraically prepared to widen if later composition introduces possible errors.

A concrete example of this is `expected<void, copack<>>` (aliased as `fn::expected_unit` in the library). Because `void` represents the unit `1` and `copack<>` represents the zero `0`, this type maps algebraically to $1 + 0 \cong 1$. Having a cardinality of exactly one, it has no possible errors, can never fail, and can only succeed with a single empty trigger (`void`). This makes it structurally isomorphic to the **unit type**.

In practice, `expected<void, copack<>>` acts as **the graded gateway** to start your pipelines. By initiating a chain with this unit trigger, you opt in all subsequent `and_then` bindings to graded error-set unioning, without having to invent any fake starting errors or manually wrap your initial steps. Since its starting error set is empty (`copack<>`), unioning it with subsequent steps' errors (say, `copack<IoError>`) yields exactly those errors. There is also an alternative unit type without error channel, spelled `just<void>` (see Section 10).

> [!TIP]
>
> ### Mathematical note — the graded functor and its two units
>
> Having established the error pomonoid $(\mathcal{E}, \cup, \emptyset, \subseteq)$ in Section 1, `libfn`'s graded `expected` is the **lax monoidal functor** $G : \mathcal{E} \to [\mathcal{C}, \mathcal{C}]$ from the pomonoid category $\mathcal{E}$ to the endofunctor category on C++ types, with $G_E(A) \cong \text{expected}\langle A, E\rangle$ (following Orchard, Wadler, and Eades, *Unifying graded and parameterised monads*). Its unit is
>
> $$\eta_A : A \to G_I(A), \qquad I = \emptyset$$
>
> Two different units meet in the gateway type `expected<void, copack<>>`: the neutral grade $I = \emptyset$ of the error pomonoid, and the unit object $1$ of the value category, spelled `void`. It is precisely $G_I(1)$.
>
## 10. The identity cluster

Certain operations behave like an identity functor across different carriers. Because some states correspond structurally, `libfn` licenses specific cross-carrier behavior to prevent redundant boilerplate.

Consider this cross-carrier table:

| Carrier | Algebraic State Shape |
| - | - |
| `just<T>` | **T** (A single value) |
| `choice<Ts...>` | **Ts...** (A coproduct of values) |
| `expected<T, copack<>>` | **T + 0** ≅ **T** (A value and an uninhabited error) |

Each of these carriers is canonically isomorphic to its own payload: none of them adds a failure or an empty state, so a successful value is always present.

Because none of them can hide an inhabited failure state, `libfn` provides a licensed pipeline operation that allows binding across these boundaries:

<!-- sync-example-test-identity-cross -->
```cpp
auto test_identity_cross() -> void
{
  fn::just<UserId> j{UserId{}};

  // Cross-carrier pipeline bind to another identity carrier
  auto result = j | fn::and_then([](UserId u) { return fn::expected<UserId, fn::copack<>>{u}; });

  static_assert(std::same_as<decltype(result), fn::expected<UserId, fn::copack<>>>);
}
```

The *bind* operation adopts the carrier family of the provided callback — a crossing only the pipeline-scoped functors are licensed to make (Section 3).

### Success-Path Bridging

Fallible types like `expected` (with inhabited error states — `expected<T, copack<>>` excluded) and `optional` cannot switch to infallible carriers, because doing so would risk silently discarding an inhabited (i.e. error) state.

However, identity carriers are licensed to bridge to any fallible carrier via the pipeline `fn::and_then`. Because an identity carrier is statically proven infallible, transitioning to `optional` or a standard `expected` merely introduces potential failure downstream. No pre-existing failure state is discarded, because none can exist upstream:

<!-- sync-example-test-success-bridge -->
```cpp
auto test_success_bridge(fn::just<int> j) -> void
{
  // An identity carrier can bridge to fallible carriers on the success path
  auto to_opt = j | fn::and_then([](int i) { return fn::optional<int>{i}; });
  static_assert(std::same_as<decltype(to_opt), fn::optional<int>>);

  auto to_exp = j | fn::and_then([](int i) { return fn::expected<int, IoError>{i}; });
  static_assert(std::same_as<decltype(to_exp), fn::expected<int, IoError>>);

  // Bridging a multi-alternative choice to fallible optional with heterogeneous success join:
  auto choice_to_opt
      = fn::choice_for<int, bool>{true}
        | fn::and_then(fn::overload{[](int) -> fn::optional<char> { return {'a'}; },
                                    [](bool) -> fn::optional<long> { return {2L}; }});
  static_assert(std::same_as<decltype(choice_to_opt), fn::optional<fn::copack_for<char, long>>>);
}
```

All three cluster members (`just`, `choice`, and `expected<T, copack<>>`) can bridge to fallible carriers.

Monadic operations behave naturally around this identity cluster:

- **Success mapping (`transform`)**: Remains meaningful, and the member always stays inside its own carrier family. The pipeline `fn::transform` adds one licensed crossing: a `copack` returned from a callable mapped over a `just` is promoted to the `choice` over the same alternatives (as detailed in Section 11).
- **Sequential binding (`and_then`)**: Allows cross-carrier transitions *within* the identity cluster (e.g., `just` to `expected<U, copack<>>`) when using pipeline-scoped `fn::and_then`.
- **Recovery / dead-side mapping (`transform_error`, `or_else`, `recover`, `inspect_error`)**: Because `just` and `choice` have no error side, these are rejected at compile time. On `expected<T, copack<>>`, they are vacuously well-formed but statically proven unreachable (to allow generic code on `expected` to compile).
- **Short-circuiting (`fail`, `filter`)**: Rejected for all identity cluster carriers, because no failure state (an inhabited error or empty state) can possibly be constructed from a never-failing identity context.
- **Elimination fallbacks (`value_or`)**: Rejected on `just` and `choice`, which can never fail. On `expected<T, copack<>>` it stays well-formed so that generic code on `expected` compiles — but the fallback is still constrained: it must be a valid initializer for `T`, and only its branch is statically dead. Where the value side is `void`, `value_or()` takes no fallback at all.
- **Neutral observation (`inspect`, `discard`)**: Supported and behave normally.

> [!NOTE]
>
> ### Note — the vacuous `or_else` asks nothing
>
> For instance, the following function compiles successfully, even though the recovery handler is a plain `int` (not a callable at all):
>
> <!-- sync-example-vacuous-or-else -->
> ```cpp
> auto test_vacuous_or_else() -> void
> {
>   using type = decltype(fn::expected<void, fn::copack<>>{} | fn::or_else(std::declval<int>()));
>   static_assert(std::same_as<type, fn::expected<void, fn::copack<>>>);
> }
> ```
>
> In contrast, if the error grade is inhabited (or on `optional`, where the empty state is a genuine inhabited state), `or_else(42)` is loudly rejected.
>
> The `or_else` operation evaluates the callback over the error alternatives. Over the uninhabited `copack<>`, there are zero alternatives, so the underlying fold has zero inputs. The operation trivially collapses to the identity mapping, the callback contributes nothing, and no questions about the callback — not even invocability — are formable. Demanding a constraint on the callback would be an arbitrary invention rather than a logical derivation.
>
> This serves a load-bearing design principle: generic code remains closed under all error grades. If `or_else` were rejected on `expected<T, copack<>>`, a recovery step would become ill-formed simply because an upstream stage statically proved that failure is impossible, breaking generic composition. Instead, the recovery step stays writable everywhere—and does nothing where failure is impossible.

> [!TIP]
>
> ### Mathematical note — isomorphic yet nominally distinct
>
> The three shapes in the table are isomorphic in $\mathcal{C}$, so they carry the same information. C++ is nominal, and `libfn` keeps it that way rather than exposing those isomorphisms as implicit conversions: doing so would drop three mutually convertible types into every overload set that mentions any one of them, and the conversions would compose into cycles. Each isomorphism is instead reachable as a **licensed crossing** — a pipeline functor you invoke — so the equivalence is available exactly where it is asked for.
>
> That boundary is not only a restriction: in Section 11 it is what makes `choice` a monad rather than a bare coproduct.
>
## 11. choice: identity over a coproduct

The `choice` carrier represents a computation that always succeeds by selecting one of several alternatives. Structurally, it serves as the single-layer carrier for coproduct states, avoiding the invalid nested `just<copack<Ts...>>` representation discussed in Section 3.

### Promotion via Pipeline Functors

A pipeline-scoped `fn::transform` on a `just` that returns a `copack` is promoted automatically to a `choice`:

<!-- sync-example-test-identity-transformation -->
```cpp
auto test_identity_transformation(fn::just<UserId> j) -> void
{
  // Transforming a just with a callable returning a copack produces a choice
  auto mapped
      = j | fn::transform([](UserId) { return fn::copack_for<Missing, FilePath>{Missing{}}; });

  static_assert(std::same_as<decltype(mapped), fn::choice_for<FilePath, Missing>>);
}
```

Similarly, a pipeline-scoped `fn::and_then` on a `just` is permitted to return a `choice` or `expected<T, copack<>>` directly.

Inside its own carrier domain, `choice` behaves differently from a bare `copack` in how it maps and binds:

- A `copack` is plain data, and it is self-flattening: a `copack` returned from a branch dissolves into the result.
- A `choice` is a never-failing outer computation over those alternatives, and it is an atom: a `choice` returned from a branch survives as one alternative unless `and_then` explicitly joins it away.

Consider a scenario where different branches of a switch return different `choice` types:

<!-- sync-example-test-choice-mapping -->
```cpp
auto test_choice_mapping(fn::choice<User, UserId> ch) -> void
{
  constexpr auto mapper = fn::overload{[](UserId) { return fn::choice<Missing>{Missing{}}; },
                                       [](User) { return fn::choice<FilePath>{FilePath{}}; }};

  // transform nests the returned choice as a mapped value
  auto mapped = ch | fn::transform(mapper);
  static_assert(
      std::same_as<decltype(mapped), fn::choice_for<fn::choice<FilePath>, fn::choice<Missing>>>);

  // and_then joins and flattens them into a normalized superset choice
  auto bound = ch | fn::and_then(mapper);
  static_assert(std::same_as<decltype(bound), fn::choice_for<FilePath, Missing>>);
}
```

A callback returning a bare value belongs to `transform`, not `and_then`: `choice`'s `and_then` rejects it with a named diagnostic.

> [!TIP]
>
> ### Mathematical note — why copack is not a monad, but choice is
>
> Categorically, `copack<Ts...>` is an object-level **coproduct** (disjoint sum $\bigoplus T_i$), whereas `choice<Ts...>` is a **monad** representing a coproduct-bearing computation context.
>
> 1. **`copack` is self-flattening (not a monad)**:
>    Naked sums are naturally self-flattening (e.g., $(A + B) + C \cong A + B + C$), and `libfn` enforces it syntactically: a nested `copack` is ill-formed by design. But the multiplication $\mu_A : M(M(A)) \to M(A)$ presupposes that $M \circ M$ is expressible as a type, so *join* has no domain to act on. The constraint is intentional: it allows `transform` on `copack` to collapse every branch's results into a flat, deduplicated set instead of an ever-nesting type.
>
> 2. **`choice` is the monad (the "structural suspend button")**:
>    To restore monad laws, the monadic carrier `choice<Ts...>` wraps the sum in an "identity layer" to preserve structural depth: `choice<choice<T>>` $\ne$ `choice<T>`. That layer holds eager flattening in check.
>    Thus, `choice` acts as a lawful monad under $M(A) = \text{choice}\langle A\rangle$ over coproduct objects $A = \bigoplus_{j} T_j$ — the *nominal* identity wrapper, which Haskell spells as the `Identity` newtype. Because $M(A) \cong A$ yet is a distinct C++ type, the unit and the multiplication are exactly the wrapping and unwrapping that nominal typing makes observable:
>    - **Unit / return** $\eta_A : A \to M(A)$ : wraps the coproduct in the `choice` layer. Building a `choice` from a single alternative composes this with the coproduct's own injection $\iota_i : T_i \to \bigoplus_j T_j$.
>    - **Join / flatten** $\mu_A : M(M(A)) \to M(A)$ : strips one `choice` layer, letting the underlying sum deduplicate its alternatives (the codiagonal fold $[id, id]$, executed statically via `choice_for`).
>    - **Bind**: maps, then flattens explicitly via *join*. Making that step explicit is what grants control over *when* flattening occurs.
>
>
## 12. Elimination and multidispatch

Once your computation shapes are fully derived, you may want to eliminate the structure to yield an ordinary C++ value. This is typically done via `apply` or `apply_r`. You can also use `get` on a singular `copack` (as explained in Section 4); or directly read `.value()` from a `just`, where it is total. On fallible carriers `.value()` is partial: it yields the value if there is one, and otherwise throws (`bad_expected_access`, `bad_optional_access`).

The operations `transform` and `apply` differ in whether the structure survives:

- `transform` stays *inside* the carrier or copack, producing a new carried type.
- `apply` *eliminates* the structure entirely: the result type is deduced from the branches, which must then all yield that one same type.
- `apply_r<R>` permits branch results acceptable as the specific type `R`.

> [!NOTE]
>
> ### Note — eliminating with heterogeneous branches
>
> Because `apply` leaves the algebra for an ordinary C++ value, it must deduce one result type, and branches that disagree are rejected. When they naturally disagree, name a `copack` as the target instead: `apply_r<copack_for<A, B, C>>` accepts branches returning `A`, `B` or `C`, since every alternative converts implicitly into its parent `copack`.

Application expands one selected level only. The call shapes are straightforward:

| Type | Eliminated Call Shape |
| - | - |
| `A` | `f(A)` |
| `pack<A, B>` | `f(A, B)` |
| `pack<>` | `f()` |
| `std::tuple<A, B>` | `f(A, B)` |
| `copack<A, B>` | `f(A)` or `f(B)` |
| `copack<pack<A, B>, C>` | `f(A, B)` or `f(C)` |

A whole-carrier `expected` application cleanly handles both success and error paths into one result type:

<!-- sync-example-test-elimination -->
```cpp
auto test_elimination(fn::expected<UserId, fn::copack<Missing>> ex) -> int
{
  return ex.apply(fn::overload{[](UserId) { return 1; }, [](Missing) { return 0; }});
}
```

Exhaustiveness is statically constrained. If you omit a handler for a possible type, the compilation fails. `fn::overload` is merely a helper; final selection always relies on ordinary C++ overload resolution.

> [!WARNING]
>
> ### Warning — catch-all handlers defeat exhaustiveness
>
> Exhaustiveness is checked against the types your handlers name. An unconstrained `[](auto)` names all of them, so a `copack` that later gains an alternative still compiles — the new alternative routes into the catch-all instead of failing where its branch is missing. Where one handler should serve several alternatives, name a concept instead of `auto` — for example `[](MyConcept auto &&)` — and the check still fires for anything the concept does not admit.

### Type-tagged elimination

Because storage shape and call shape are distinct, untagged `apply` can sometimes erase the structural context of the state (for example from `expected<int, int>`). To preserve this context and prevent permissive C++ implicit conversions from accidentally conflating different states, `libfn` provides the **`apply_type`** (and `apply_type_r`) member functions.

When you eliminate a carrier using `apply_type`, the active handler receives an explicit C++ state tag or constructor tag as its first argument, followed by the unpacked payload:

- On `expected`, the success arm receives `std::in_place` followed by the success value — `std::in_place` alone when the value type is `void` — while the error arm receives `fn::unexpect` followed by the error.
- On `optional`, the success arm receives `std::in_place` followed by the value, while the empty arm receives `std::nullopt`.
- On `copack` and `choice`, the active alternative arm receives `std::in_place_type<T>` followed by the payload.
- On `just`, the arm receives `std::in_place_type<T>` followed by the value. Symmetrically, `just<void>`'s arm receives `std::in_place_type<void>` alone — representing a nullary unit payload (never an empty or uninitialized state).

> [!TIP]
>
> ### Mathematical note — elimination of algebraic structures
>
> The two eliminations are not dual to one another — the dual of coproduct elimination is product *introduction*, pairing — and they differ in how much the category determines.
>
> - **Coproduct**: eliminating $A + B$ is canonical. Supplying $f : A \to C$ and $g : B \to C$ determines a *unique* mediating morphism $[f, g] : A + B \to C$ by the universal property, and in `libfn` ordinary C++ overload resolution is what computes it.
> - **Product**: eliminating $A \times B$ invokes no universal property — a morphism out of a product is just a morphism. What `apply` supplies is the bridge between the product as *stored*, a `pack`, and the product as an argument list, which C++ keeps distinct: it uncurries.
>
> Carrier elimination (`apply_type`) additionally preserves the injections: the state tag — such as `std::in_place`, `fn::unexpect` or `std::in_place_type<T>` — tells the handler *which* injection morphism placed the value into the structure.
>
## 13. The monadic operations map

This is a concise reference for `libfn`'s operations, organized by channel and effect:

**Success Channel**

- `transform`: Maps the successful value. Stays inside the carrier.
- `and_then`: Sequences success-path computations. The mechanism for introducing new errors into a graded expected.
- `filter`: Enters a short-circuit state if a predicate fails.
- `inspect`: Observes the successful value transparently.
- `fail`: Intercepts success and forces a transition to a failure state.

**Error/Empty Channel**

- `transform_error`: Maps the error value. Stays inside the carrier, and is the one operation that can narrow a graded error set (Section 9).
- `or_else`: Sequences computations based on errors. Joins recovery values.
- `recover`: Intercepts failure and forces a transition back to a success state.
- `inspect_error`: Observes the error value transparently.
- `value_or`: Supplies a fallback for the failure state. The member `.value_or(x)` eliminates the carrier and yields the value; the pipeline `fn::value_or(x)` keeps the carrier, returning it engaged with either its own value or the fallback.

**Neutral**

- `discard`: Unconditionally evaluates the carrier, discards the result, and returns `void`. This is used to signal to the compiler that the return value is deliberately ignored.

**Elimination**

- `apply`: Routes the stored state to an overload set, leaving the algebra with an ordinary C++ value.
- `apply_type`: The same elimination, keyed by an explicit state tag.

**Composition & Combination**

- `operator&` (conjunction): Combines independent computations (values into a `pack`, errors as a union).
- `operator|` (disjunction): Combines alternative computations (values into a `copack` disjoint sum, errors as a product `pack`).
- `fn::conjoin`: An n-ary fold of `operator&`, over monadic carriers or over packs, copacks and scalars — not a mix of the two.
- `fn::disjoin`: An n-ary fold of `operator|` over monadic carriers, supporting total disjunction with the identity cluster.

### Key Architectural Rules of the Map

To reason about how these operations affect the type algebra of your computation:

- **`fail` and `recover` are duals**: `fail` intercepts a success-path value and forces a transition to the failure state ($Success \implies Failure$). `recover` intercepts a failure-path error and forces a transition back to the success state ($Failure \implies Success$). Neither operation widens the error set of a graded carrier.
- **Graded `and_then`** is the primary mechanism for introducing a _new_ error type (widening the error grade) into your pipeline.
- **`filter` and `fail`** merely enter an *existing* short-circuit state: the carrier must already be capable of holding the failure state.
- **Error-side monadic operations** (like `transform_error`, `or_else`, `recover`, and `inspect_error`) require a carrier with an error or empty side. They are rejected on `just` and `choice`, and stay vacuously well-formed on `expected<T, copack<>>`, whose error side exists but is uninhabited (Section 10).

## 14. Laws as C++ equalities

Where the carried types compare equal in a constant expression, the laws are checked by the compiler itself. Functor identity and monad left identity are machine-checked below; the remaining laws hold structurally, by construction of the derived types:

<!-- sync-example-test-laws -->
```cpp
constexpr auto test_laws() -> void
{
  constexpr fn::expected<int, fn::copack<Missing>> ex{42};

  // Functor Identity: mapping with identity yields the same value
  constexpr auto id = [](auto v) { return v; };
  static_assert((ex | fn::transform(id)) == ex);

  // Monad Left Identity: pure(x) >>= f is equivalent to f(x)
  constexpr auto pure = [](int v) { return fn::expected<int, fn::copack<Missing>>{v}; };
  constexpr auto f = [](int v) { return fn::expected<int, fn::copack<Missing>>{v * 2}; };
  static_assert((pure(42) | fn::and_then(f)) == f(42));
}
```

Other properties hold structurally:

- **Functor composition**: `m | transform(f) | transform(g)` equals `m | transform([](auto v) { return g(f(v)); })`.
- **Monad right identity**: `m | and_then(pure)` equals `m`.
- **Monad associativity**: `(m | and_then(f)) | and_then(g)` equals `m | and_then([](auto v) { return f(v) | and_then(g); })`. For graded expected, both sides of the associativity derive the same normalized union grade.
- **Product associativity**: Holds after canonical `pack` normalization.
- **Coproduct set semantics**: Union associativity, commutativity, and idempotence apply.
- **Coherent widening**: Upcasting an error through intermediate supersets yields the same final type as upcasting directly to the broadest superset.
- **Identity cluster binds**: Laws hold across `just`, `choice`, and `expected<T, copack<>>` via the canonical payload-preserving state-shape correspondence.

## 15. C++ mechanics that preserve the algebra

To make the algebraic model reliable in everyday C++, `libfn` uses extensive compiler mechanisms to reject malformed usage and preserve performance properties.

### Constraints and exhaustiveness

Public concepts and `requires` clauses enforce correctness before instantiation. Operations are protected by public applicability concepts (`fn::applicable_transform`, `fn::applicable_and_then`, …) that answer *false* for an impossible call instead of erroring deep inside template machinery. This underpins the compile-time exhaustiveness guarantees of `apply` and monadic operations established in Sections 4 and 12, catching unhandled alternatives at the boundary of instantiation.

### C++ value properties

The library respects C++ value mechanics:

- Core operations are fully `constexpr`.
- The algebra's own types — `pack`, `copack`, `just` and `choice` — are structural when their elements are, so a `constexpr` value of one can be used as a template parameter.
- `noexcept` is conditionally computed based on the operations provided.
- Value categories (lvalue/rvalue) propagate strictly to callbacks, avoiding unnecessary copies.
- Immovable and move-only payloads are supported in place.
- Reference-bearing `pack<T&...>` and `optional<T&>` are supported. Lifetime responsibility for non-owning references remains with the caller.
- `pack` compares element-wise, like `std::tuple`, supporting both equality and three-way comparison; for a reference-bearing `pack<T&...>` this compares the referents rather than the references themselves.

> [!NOTE]
>
> ### Note — reference payloads
>
> Raw reference payloads are disallowed on the carriers `expected`, `just` and `choice`, and as `copack` alternatives. `expected` stores its payload in a union, and C++ forbids a union member of reference type; the algebra's own types refuse them so that every alternative is dispatched the same way, whatever it holds. `optional<T&>` is the deliberate exception — the standard specifies it, and `libfn` polyfills it. If you want to propagate references inside the other carriers, wrap them in a `pack` (e.g. `expected<pack<T&>, E>`).

<!-- sync-example-test-references -->
```cpp
auto test_references() -> void
{
  int x = 42;

  // optional supports references directly
  fn::optional<int &> opt{x};
  static_assert(std::same_as<decltype(opt.value()), int &>);

  // expected must wrap references inside a pack
  fn::expected<fn::pack<int &>, Error> ex{fn::as_pack(x)};
  static_assert(std::same_as<decltype(ex.value()), fn::pack<int &> &>);
}
```

### pfn and fn

The library is divided into layers:

- `pfn` (Polyfill fn) is the standards-facing layer. It provides `std::optional` and `std::expected` in their C++26 shape — monadic member functions, `optional<T&>`, range support — plus smaller utilities such as `std::invoke_r` and `std::unreachable`, all available to a C++20 compiler.
- `fn` is the strict extension layer. It introduces the `pack`/`copack` algebra, multidispatch, graded errors, `choice`, `just`, the pipeline functors, and the composition operators `&` and `|`.

Every `fn` type with a `pfn` counterpart is a strict superset of it: switching a valid program from `pfn` to `fn` changes neither compilation nor behaviour.

## Functional terminology

For readers with a background in functional languages (like Haskell or OCaml), this table translates standard terminology to `libfn`'s C++ vocabulary:

| Functional Term | `libfn` Equivalent |
| --------------- | ------------------ |
| `fmap` / `map` | `transform` / `transform_error` |
| `bind` / `>>=` | `and_then` |
| `pure` / `return` | `just<T>{v}` / `expected<T, copack<>>{v}` — a carrier constructor |
| Lift / inject | `fn::as_pack` / `fn::as_copack` |
| Kleisli arrow | The callable passed to `and_then` |
| Product type | `pack` / `std::tuple` |
| Coproduct / Sum | `copack` (the sum itself) / `choice` (the never-failing carrier over a sum) |
| Subeffecting | Widening an error grade / subset inclusion |

## Further reading

For formal validation of the algebraic structures modeled in `libfn`, refer to:

1. Orchard and Petricek, [“Embedding effect systems in Haskell”](https://www.doc.ic.ac.uk/~dorchard/publ/haskell14-effects.pdf) (for effect sets, union, and subeffecting).
2. Orchard, Wadler, and Eades, [“Unifying graded and parameterised monads”](https://arxiv.org/pdf/2001.10274) specifically Definition 21 (for the graded-monad interpretation).
3. McDermott and Uustalu, [“Flexibly Graded Monads and Graded Algebras”](https://dylanm.org/flexibly-graded-monads.pdf) _Note: `libfn` does not claim to fully implement their flexibly graded construction, but the work contextualizes graded structures._
