# Type algebra and functional composition in libfn

The `libfn` library is a C++20 functional programming framework that lets the compiler derive both the values and the complete static shape of a computation. Instead of relying on type erasure, exceptions, or monolithic sum types, `libfn` tracks the precise algebraic combinations of success, alternative, and error states during composition.

The library operates on a few core vocabulary types:

- `pack`: Product type containing all fields.
- `copack`: Canonical coproduct containing exactly one alternative.
- `optional`: Computation yielding a value or empty.
- `expected`: Computation yielding success or error.
- `choice`: Never-failing computation holding one of several alternatives.
- `just`: Identity computation always yielding a single value.

Composition operations include `transform` (mapping), `and_then` (sequential monadic binding), `operator&` (simultaneous product composition), and `apply` (multidispatch elimination).

### Member vs. Pipeline Syntax

Most operations are exposed in two forms:

- **Member functions** (e.g., `.transform()`, `.and_then()`, `.apply()`) called directly on a carrier (e.g., `ex.transform(f)`).
- **Pipeline functors** in namespace `fn` (e.g., `fn::transform`, `fn::and_then`) applied via `operator|` (e.g., `ex | fn::transform(f)`).

Freestanding `fn::apply` acts as a utility (like `std::apply`) to unpack any tuple-like or pack-like structure.

In prose, we omit prefixes (writing `apply`, `transform`, `and_then`, `expected`, `pack`) when referring to both forms or core vocabulary types generally.

### Storage Shape vs. Call Shape

Although different types can behave identically during application, they remain strictly distinct in memory. For example, `pack<A, B>`, `std::tuple<A, B>`, and `std::pair<A, B>` all unpack into the same call shape `f(a, b)` during `apply`, but they are separate C++ types with distinct layouts. Application does not silently convert or unify types on the storage side.

To illustrate these concepts, the examples in this document use a reusable set of value and error types:
```cpp
struct UserId {};
struct User {};
struct FilePath {};
struct MaximumSize {};
struct BlockSize {};

struct NotANumber {};
struct OutOfRange {};
struct Missing {};
struct IoError {};
struct BadSyntax {};
struct UnknownKey {};
```

## 1. Why compose types as well as values?

In idiomatic C++, error handling usually means picking one application-wide error enumeration, a giant `std::variant`, or throwing exceptions. If a function only ever fails due to one specific error, returning a large application-wide error variant discards the precise bounds of what the function can actually do.

With `libfn`, the compiler derives an exact, graded error pipeline. Consider parsing, validating, and loading a user:

```cpp
#include <fn/expected.hpp>
#include <fn/copack.hpp>
#include <fn/and_then.hpp>
#include <concepts>
#include <string_view>

auto parse_id(std::string_view) -> fn::expected<UserId, fn::copack<NotANumber>>;
auto validate(UserId) -> fn::expected<UserId, fn::copack<OutOfRange>>;
auto load(UserId) -> fn::expected<User, fn::copack<IoError, Missing>>;

auto graded_pipeline(std::string_view sv) -> void {
    auto pipeline = parse_id(sv)
        | fn::and_then(validate)
        | fn::and_then(load);

    // The exact derived error union is recorded in the type:
    static_assert(std::same_as<
        decltype(pipeline),
        fn::expected<User, fn::copack<IoError, Missing, NotANumber, OutOfRange>>
    >);
}
```

The resulting `expected` statically records that the pipeline yields a `User` on success, or fails with exactly one of `NotANumber`, `OutOfRange`, `Missing`, or `IoError`. This exact union accumulates automatically via `and_then` composition.

### What Does "Graded" Mean?

Standard monads are rigid: an `expected<T, E>` requires every step in a pipeline to return the identical error type `E`. This forces you to define a monolithic global error union upfront.

A **graded monad** relaxes this restriction. Each operation is indexed by a "grade"—a set representing its specific possible errors (its "effects"). As you chain operations, the compiler automatically unions these grades.

The resulting error type is **graded**: it dynamically expands (or narrows during recovery) to match the *exact* subset of errors possible in the compiled path, providing strict static effect tracking (subeffecting) with zero boilerplate.

> [!TIP]
>
> ### Mathematical note — graded monads
>
> Formally, a **graded monad** (also known as an effect monad) indexes a family of monadic carriers over a partially ordered monoid (pomonoid) of effects $(\mathcal{E}, \bullet, I, \le)$.
>
> In `libfn`, this pomonoid is defined over the category of finite sets of C++ types:
>
> - **Grades ($\mathcal{E}$)**: Finite sets of alternative types (errors).
> - **Monoidal multiplication ($\bullet$)**: Set union ($\cup$), representing effect accumulation.
> - **Identity ($I$)**: The empty set ($\emptyset$), representing the zero-error/never-failing state.
> - **Partial order ($\le$)**: Subset relation ($\subseteq$), which licenses effect approximation (subeffecting / widening).
>
> For a standard monad $M$, the binding operation maps $M\langle A\rangle \to (A \to M\langle B\rangle) \to M\langle B\rangle$. In `libfn`'s graded monad, bind accumulates effects across the pomonoid:
>
> $$bind : M_E\langle A\rangle \to (A \to M_F\langle B\rangle) \to M_{E \cup F}\langle B\rangle$$
>
> This formulation enables strict static effect tracking. A lax monoidal functor maps this pomonoid $\mathcal{E}$ into the endofunctor category $[\mathcal{C}, \mathcal{C}]$, formalizing how C++ type derivations trace exact computational side effects.

Simultaneous product composition achieves the same precision for both values and errors. Using `operator&`, you can evaluate independent computations and bundle their results:

```cpp
#include <fn/expected.hpp>
#include <fn/pack.hpp>
#include <fn/copack.hpp>
#include <concepts>

auto product_composition() -> void {
    fn::expected<UserId, fn::copack<Missing>> id{};
    fn::expected<User, fn::copack<IoError>> user{};

    auto bundled = id & user;

    static_assert(std::same_as<
        decltype(bundled),
        fn::expected<fn::pack<UserId, User>, fn::copack<IoError, Missing>>
    >);
}
```

The result contains a `pack` of the successful values and a `copack` of the exact possible errors.

### The Two Cooperating Mechanisms

Behind these highly precise compiled types are two independent mechanisms that cooperate to derive and eliminate these shapes:

1. **Type algebra** records and normalizes the exact stored C++ types using `pack` and `copack` as you compose operations.
2. **The application protocol** uses `apply` and ordinary C++ overload resolution to unpack those stored values and route them to your functions or lambdas.

To handle multiple alternative paths smoothly inside `apply`, the library provides the `fn::overload` utility. This utility constructs a unified overload set from a collection of otherwise unrelated lambdas, routing the unpacked values to the correct handler at compile time via C++ overload resolution.

These derived types are the actual explanation of the library's design, not an internal template-metaprogramming implementation detail. Understanding the precise algebraic rules of this type algebra and the mechanics of application is key to mastering the library.

## 2. Types as an algebra: zero, unit, alternatives, and products

To derive strict programmatic shapes, `libfn` uses an algebraic vocabulary over types.

- `copack<>` represents **0** (Zero) - an uninhabited type.
- `pack<>` represents **1** (Unit) - a type with exactly one state.
- `copack<A, B>` represents **A + B** (Alternatives) - a coproduct where exactly one alternative is present.
- `pack<A, B>` represents **A × B** (Products) - a type where all fields are present simultaneously.

These states can also be used to express the standard vocabulary types:

- `std::optional<T>` ≅ **1 + T** (It is either empty/unit or contains `T`, similar to `copack<T, std::nullopt_t>`)
- `std::expected<T, E>` ≅ **T + E** (It contains either success `T` or error `E`, similar to `copack<T, std::unexpected<E>>`)

The symbol ≅ indicates an equivalent state shape (an information-level correspondence), not `std::same_as`. `std::optional<T>` is its own distinct C++ type, but algebraically, it behaves as `1 + T`.

### Zero is not unit

In `libfn`'s algebra, zero and unit are strictly separated:

- `copack<>` is uninhabited. You cannot construct it. Algebraically, it is `0`.
- `pack<>` is the one nullary product value. You can construct it via `pack<>{}`. Algebraically, it is `1`.

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

```cpp
#include <fn/copack.hpp>
#include <concepts>

auto test_copack_set_semantics() -> void {
    using SetA = fn::copack<NotANumber, OutOfRange>;
    using SetB = fn::copack<Missing, OutOfRange>;

    // Flattening, deduplication, and reordering happen automatically:
    using Union = fn::copack_for<SetA, SetB, Missing, Missing, fn::copack<IoError>>;

    static_assert(std::same_as<
        Union,
        fn::copack<IoError, Missing, NotANumber, OutOfRange>
    >);
}
```

> [!NOTE]
>
> ### Note — copack vs. copack_for
>
> In C++, there is no native language feature to represent a "set of types." Template parameter lists are always positional, variadic sequences. Syntactically, this means `copack<A, B>` and `copack<B, A>` would be completely distinct types—a property that directly violates the mathematical commutative law of set union.
>
> To enforce strict, mathematically sound set semantics at compile time, `libfn` defines a single, strict canonical representation and actively rejects any instantiation that diverges from it:
>
> - **`copack`** is the core storage type. It requires its template parameters to already be flat, unique, and sorted in strict lexicographical order (the order defined by C++26 `std::type_order`, which `libfn` emulates for pre-C++26 compilers). If you attempt to instantiate it manually with out-of-order parameters (such as `copack<B, A>` when `A` lexicographically precedes `B`) or with nested copacks (such as `copack<A, copack<C, D>>`), **the compiler will reject the instantiation as outright ill-formed.**
> - **`copack_for`** is the user-facing type alias utility. It acts as the compile-time "compiler gateway," accepting any raw, arbitrary list of types (out-of-order, duplicates, nested copacks), performing the complex compile-time flattening, deduplication, and lexicographical sorting automatically, and resolving directly to the validated canonical `copack` type.
>
> To the C++ programmer, they can be used interchangeably because `copack_for` always resolves directly to `copack`. However, in prose and code, `copack` represents the normalized *state shape*, while `copack_for` represents the *construction utility*. Similarly, `choice`—which is the never-failing identity carrier over a `copack`—utilizes the `choice_for` type alias utility to automatically flatten, deduplicate, and sort its alternative types at compile time.

The laws governing `copack` are:

- **Commutative**: Order of types does not change the resulting set.
- **Associative**: Nesting copacks is equivalent to flattening them.
- **Idempotent**: Duplicate types are collapsed into one.
- **Identity**: `copack<>` acts as the union unit (adding `copack<>` changes nothing).

Because canonical ordering collapses identical types, distinct types must never be silently lost if the ordering cannot distinguish them. Consequently, types combined into a `copack` should be strongly typed tag structs or distinct domain objects, not generic primitives whose semantic meaning depends on their position (e.g., `copack<bool, bool>` becomes `copack<bool>`).

### The algebra is strictly opt-in

`libfn` relies on explicit consent. It does not silently reinterpret arbitrary C++ types as products or coproducts:

- `std::tuple` remains a standard tuple.
- `std::variant` does not acquire `copack` set semantics.
- Tuple-like participation in `apply` changes call shape applicability, but it does not change the stored type identity.
- A plain `expected<T, E>` does not automatically become graded.

To invoke the algebra, you use the opt-in mechanisms provided by the library:

- Direct construction of `pack` and `copack_for`.
- Explicit conversions via `fn::as_pack` and `fn::as_copack`.
- Member helpers for explicit type lifting (detailed in Section 8).

If a side is already a `copack` or `pack`, forwarding it behaves naturally without nesting.

When a callable supplied to `and_then` needs to produce an error grade, it can explicitly lift an expected value into one with a copack error:

- A callback returning `expected<U, copack<E>>` selects `copack<E>` as the exact result spelling.
- It does **not** authorize a union with a completely different, unrelated plain error type. Union grading requires the original outer `expected`'s error side to also be a `copack`.

> [!TIP]
>
> ### Mathematical note — products and coproducts
>
> In category theory, the **product** ($A \times B$) is the limit of a diagram of two objects, equipped with projection morphisms ($\pi_1 : A \times B \to A$, $\pi_2 : A \times B \to B$). It satisfies the universal property that any pair of morphisms from an object $X$ to $A$ and $B$ factors uniquely through $A \times B$.
>
> The **coproduct** ($A + B$) is the dual colimit, equipped with injection morphisms ($\iota_1 : A \to A + B$, $\iota_2 : B \to A + B$). Its universal property states that any pair of morphisms from $A$ and $B$ to an object $Y$ factors uniquely through $A + B$. In programming, this unique factoring morphism $[f, g] : A + B \to Y$ is exactly an **overload set** (or callback) mapped over the sum—which `libfn` implements via `apply` and multidispatch.
>
> Nullary structures define the monoidal units:
>
> - The **nullary product** is the terminal object $1$ (Unit), mapped to `pack<>` or C++ `void`.
> - The **nullary coproduct** is the initial object $0$ (Zero), mapped to the uninhabited `copack<>` (with no injection morphisms).
>
> C++ types do not form a strict category due to compiler-specific equivalence relations, but `libfn` emulates these properties by enforcing type-level canonical flattening and deduplication.
>
>
## 3. The vocabulary types

### pack: all fields are present

A `pack` acts like a standard C++ tuple (`std::tuple`) by storing multiple fields and supporting `get`, structured bindings, and an `append` mechanism. However, unlike standard tuples, `libfn` packs are strictly flat: attempting to nest a `pack` inside another `pack` via `append` flattens them, as a flat pack is canonical. To explicitly lift a single scalar value into a `pack` (which is useful when conjoining a scalar with another pack or copack), use `fn::as_pack(value)`.

```cpp
#include <fn/pack.hpp>
#include <concepts>

auto test_pack() -> void {
    fn::pack p{UserId{}, User{}}; // CTAD

    auto [id, user] = p; // Structured bindings work naturally

    // Ordered, non-deduplicated fields
    using P = fn::pack<UserId, User, User>;

    // Found via ADL (like std::get)
    using std::get;
    static_assert(std::same_as<decltype(get<0>(p)), UserId&>);

    // Splicing scalars or other packs via append:
    auto row = fn::pack{UserId{}}.append(FilePath{});
    auto wider = std::move(row).append(fn::pack{true, 3});

    static_assert(std::same_as<
        decltype(wider),
        fn::pack<UserId, FilePath, bool, int>
    >);

    // Explicitly lifting a single scalar value into a pack:
    auto lifted = fn::as_pack(42);
    static_assert(std::same_as<decltype(lifted), fn::pack<int>>);
}
```

### copack: one exact alternative is present

A `copack` stores exactly one of its defined alternatives. Consider a system processing different kinds of lexer tokens or configuration values.

When you evaluate a `copack` via its member `apply` function, it expands one selected outer level only. The active alternative currently stored inside the coproduct is passed as a terminal argument to your callback, rather than recursively unpacking any internal structures. To explicitly lift a single scalar value into a single-alternative coproduct, use `fn::as_copack(value)`. When a `copack` contains **exactly one alternative**, it is singular and supports direct value extraction via the `get` utility (resolvable via ADL), which propagates references with the same semantics as `apply`.

```cpp
#include <fn/copack.hpp>
#include <fn/utility.hpp>
#include <concepts>

struct IntegerToken {};
struct StringToken {};

auto test_copack() -> void {
    constexpr fn::copack<IntegerToken, StringToken> token = IntegerToken{};

    // Member apply eliminates the copack by routing the active alternative to an overload set:
    constexpr auto value = token.apply(fn::overload{
        [](IntegerToken) { return 1; },
        [](StringToken) { return 2; }
    });

    static_assert(value == 1);

    // Singular lift and direct value extraction (only allowed for singular copacks):
    auto cp = fn::as_copack(42);
    using std::get;
    static_assert(std::same_as<decltype(get(cp)), int&>);
}
```

A fundamental safety guarantee of `copack` is **exhaustive matching**. Any operation that evaluates a `copack` (such as mapping with `transform`, binding with `and_then`, or eliminating with `apply`) eventually delegates to the same underlying multidispatch implementation. This implementation forces compile-time exhaustiveness: if your callback or overload set fails to handle even one of the possible alternatives stored in the `copack`, the compilation is rejected as ill-formed. This SFINAE-clean behavior is why direct `get` extraction is strictly constrained and disallowed for multi-alternative `copack` types, ensuring that compile-time exhaustiveness cannot be bypassed.

### The computation carriers

To model computation, `libfn` uses carrier types:

- `just`: Always contains a single successful value.
- `optional`: Contains a value or is empty.
- `expected`: Contains a value or an exact error type.
- `choice`: Always contains one of several selected alternatives, representing the complete state space of the computation.

Because `choice` implies that an alternative is always present, `choice<>` is incomplete: an always-present selected alternative requires at least one alternative to exist.

**Rule:** A carrier does not need another carrier for multidispatch. Inside an `expected` or `optional`, store your alternative states as `copack<Ts...>`. Use `choice<Ts...>` only when those alternatives are themselves the outer, never-failing computation.

> [!NOTE]
>
> ### Note — `just<copack<Ts...>>` is spelled `choice<Ts...>`
>
>
> A programmer might be tempted to represent a never-failing, multi-alternative computation by nesting a coproduct inside an identity carrier, spelling it `just<copack<Ts...>>`. In `libfn`'s type algebra, this is precisely the space filled by `choice<Ts...>`. Structurally, `choice<Ts...>` is equivalent to a `just` container of a `copack`, providing a single-layer monadic carrier that represents a never-failing computation over a coproduct.
>
> In fact, attempting to instantiate `just<copack<Ts...>>` will trigger a compile-time static assertion failure inside `just`, explicitly warning the programmer: `"a just over a copack is spelled choice"`.

These carriers impose constraints on their payloads, but reference payloads are broadly supported where sound. For example, `optional<T&>` is supported and well-defined.

## 4. Mapping values and errors

Mapping allows you to change the contained data without altering the structural success/failure shape of the computation. `libfn` uses `transform` (functor map) to operate on the successful channel, and `transform_error` for the error channel.

```cpp
#include <fn/expected.hpp>
#include <fn/copack.hpp>
#include <fn/transform.hpp>
#include <fn/transform_error.hpp>
#include <fn/utility.hpp>
#include <concepts>

auto mapping_values_and_errors() -> void {
    fn::expected<UserId, fn::copack_for<Missing, IoError>> ex{};

    auto mapped_val = ex | fn::transform([](UserId) { return User{}; });
    static_assert(std::same_as<
        decltype(mapped_val),
        fn::expected<User, fn::copack<IoError, Missing>>
    >);

    auto mapped_err = ex | fn::transform_error(fn::overload{
        [](Missing) { return BadSyntax{}; },
        [](IoError e) { return e; }
    });

    static_assert(std::same_as<
        decltype(mapped_err),
        fn::expected<UserId, fn::copack<BadSyntax, IoError>>
    >);
}
```

Key principles of mapping:

- `transform` stays strictly within the carrier type.
- Success and error states are rigidly preserved.
- A bare `copack` also has `transform`, allowing mapping across alternatives.
- Heterogeneous branch results inside `transform_error` form a normalized result `copack`.
- Applying an error-side operation like `transform_error` to a carrier that has no error side (like `just` or `choice`) is rejected by the compiler.
- If a side is uninhabited (`copack<>`), the transformation is well-formed, but vacuous (i.e., a no-op):
  - `transform_error` on `expected<T, copack<>>` is proven unreachable and a no-op.
  - `transform` on `optional<copack<>>` is proven unreachable and a no-op.

> [!TIP]
>
> ### Mathematical note — functors
>
> A covariant functor $F : \mathcal{C} \to \mathcal{D}$ maps objects $A \in \mathcal{C}$ to $F(A) \in \mathcal{D}$ and morphisms $(f : A \to B)$ to $(F(f) : F(A) \to F(B))$. It must satisfy the functor laws:
>
> - **Identity**: $F(id_A) = id_{F(A)}$
> - **Composition**: $F(g \circ f) = F(g) \circ F(f)$
>
> In `libfn`, `transform` implements this morphism mapping ($fmap$). Functorial action on the initial object $0$ (the uninhabited `copack<>`) is vacuous: since there are no morphisms originating from $0$ (except the unique initial morphism), mapping over an empty alternative set is vacuously true. The compiler leverages this by optimizing `transform` on `optional<copack<>>` into a static no-op.
>
## 5. Product composition with operator& (conjunction)

Simultaneous product composition combines independent computations. By evaluating `a & b`, you bundle the results.

```cpp
#include <fn/expected.hpp>
#include <fn/pack.hpp>
#include <fn/copack.hpp>
#include <concepts>

auto operator_and_composition() -> void {
    fn::expected<UserId, fn::copack<Missing>> a{};
    fn::expected<User, fn::copack<IoError>> b{};

    auto result = a & b;

    static_assert(std::same_as<
        decltype(result),
        fn::expected<fn::pack<UserId, User>, fn::copack<IoError, Missing>>
    >);
}
```

The runtime failure semantics are exact:

- The result type statically records all possible errors.
- At runtime, the result stores at most *one* error, not an accumulated collection of errors.
- If both operands already contain errors, standard short-circuit evaluation applies (the left error is retained).
- Normal C++ evaluation rules apply: `operator&` does not magically make I/O lazy or parallel.

When composing two `copack`s directly, `operator&` performs a Cartesian distribution, yielding a `copack` of `pack`s. The variadic entry point into these rules is `fn::conjoin(...)`. Note that bare `scalar & scalar` is not syntactically valid by itself; you must lift them with `fn::conjoin(a, b)` or `fn::as_pack(a) & b`.

```cpp
#include <fn/copack.hpp>
#include <fn/pack.hpp>
#include <concepts>

struct A {};
struct B {};
struct C {};
struct D {};

auto test_cartesian_distribution() -> void {
    constexpr fn::copack<A, B> ab = A{};
    constexpr fn::copack<C, D> cd = C{};

    // The product distributes over the coproduct: (A + B) x (C + D) -> AC + AD + BC + BD
    auto result1 = ab & cd;
    static_assert(std::same_as<
        decltype(result1),
        fn::copack_for<fn::pack<A, C>, fn::pack<A, D>, fn::pack<B, C>, fn::pack<B, D>>
    >);

    // The distributive law also works on a pack: (A × B) × (C + D) = ABC + ABD
    constexpr fn::pack<A, B> Pab = {A{}, B{}};
    auto result2 = Pab & cd;
    static_assert(std::same_as<
        decltype(result2),
        fn::copack_for<fn::pack<A, B, C>, fn::pack<A, B, D>>
    >);
}
```

### Conjunction with the Identity Cluster

When performing product composition (`operator&`), you can combine fallible carriers (like `expected` or `optional`) with any member of the **identity cluster** (detailed in Section 9):

- **Errors are unaffected**: Because identity cluster operands can never fail, they add no new types or terms to the result's error channel. The error side of the fallible operand is preserved exactly (whether plain or copack-graded).
- **Value bundling**: The value of the identity cluster operand is conjoined with the fallible operand's value channel into a `fn::pack`.
- **Unit elision**: `just<void>` and `expected<void, copack<>>` act as the product's identity unit and are completely elided from the value product (e.g., `expected<T, E> & just<void>` stays `expected<T, E>`).
- **Choice distribution**: If a `choice` operand is conjoined with a fallible carrier, the coproduct distributes through the product. This yields a `copack` of `pack`s wrapped back inside the fallible carrier.

```cpp
#include <fn/expected.hpp>
#include <fn/choice.hpp>
#include <fn/just.hpp>
#include <fn/pack.hpp>
#include <concepts>

auto test_conjunction_with_identity_cluster() -> void {
    fn::expected<int, Error> ex{42};
    fn::just<double> j{1.5};

    // Conjoining an expected with a just
    auto res1 = ex & j;
    static_assert(std::same_as<
        decltype(res1),
        fn::expected<fn::pack<int, double>, Error>
    >);

    // Conjoining with a unit (just<void>) completely elides the unit
    auto res2 = ex & fn::just<void>{};
    static_assert(std::same_as<decltype(res2), decltype(ex)>);

    // Conjoining a choice causes distribution inside the carrier
    fn::choice_for<double, bool> ch = 1.5;
    auto res3 = ex & ch;
    static_assert(std::same_as<
        decltype(res3),
        fn::expected<fn::copack_for<fn::pack<int, double>, fn::pack<int, bool>>, Error>
    >);
}
```

> [!TIP]
>
> ### Mathematical note — symmetric monoidal categories and distribution
>
> Simultaneously conjoining independent computations via `operator&` models a **symmetric monoidal category** $(\mathcal{C}, \otimes, I)$ where:
>
> - **The value tensor ($\otimes$)** corresponds to `pack` multiplication, and the unit $I$ corresponds to `pack<>`. The associator ($\alpha_{A,B,C} : (A \otimes B) \otimes C \cong A \otimes (B \otimes C)$) and unitors ($\lambda_A : I \otimes A \cong A$, $\rho_A : A \otimes I \cong A$) hold up to canonical C++ type-equivalence.
> - **Distributivity**: The tensor product $\otimes$ distributes over the coproduct $\oplus$ (represented by `copack`), yielding the canonical isomorphism:
>   $$A \otimes (B \oplus C) \cong (A \otimes B) \oplus (A \otimes C)$$
>   This is precisely the Cartesian distribution of `pack` over `copack` implemented statically by `libfn`.
> - **Error Accumulation**: For `expected<T, E>`, the error grades form a union, which corresponds to the monoidal composition of effects in the underlying monoid $(\mathcal{E}, \cup, \emptyset)$.
>
## 6. Sum composition with operator| (disjunction)

Simultaneous sum composition combines alternative computations. By evaluating `a | b`, you attempt the left computation `a`. If it succeeds, its result is preserved. If it fails, you evaluate the right computation `b` as a fallback.

```cpp
#include <fn/expected.hpp>
#include <fn/pack.hpp>
#include <fn/copack.hpp>
#include <concepts>

auto operator_or_composition() -> void {
    fn::expected<int, Error> a{};
    fn::expected<bool, OtherError> b{};

    auto result = a | b;

    static_assert(std::same_as<
        decltype(result),
        fn::expected<fn::copack_for<int, bool>, fn::pack<Error, OtherError>>
    >);
}
```

The runtime and compile-time semantics of disjunction are exact:

- **Value-side sum**:
  - If the successful value types of the operands differ, they are combined into a disjoint sum represented by `fn::copack`.
  - If the successful value types are identical, they collapse into a single bare type `T` (e.g., `expected<int, E> | expected<int, E'>` yields an `expected<int, ...>`).
  - `void` results enter a genuine sum as `pack<>`. If both operands are `void`, they collapse back to `void`.
- **Error-side product**:
  - Because the overall disjunction only fails if *both* operands fail, the error channel represents the product of both errors. This is recorded positionally inside `fn::pack<E1, E2>`.
  - If both operands contain graded error sets (`copack`s of errors), the errors distribute through the product: $(El + Er) \times (El') \to (El \times El') + (Er \times El')$. This yields a `copack` of `pack`s, representing all combinations of failure states.
- **Total Disjunction and the Identity Cluster**:
  - If at least one operand belongs to the **identity cluster** (detailed in Section 9), the disjunction is guaranteed to never fail at runtime.
  - The error side gains an uninhabited factor (`copack<>`), which collapses the error channel entirely and prevents the result from failing.
  - The result is folded into a non-failing carrier of the identity cluster: a single-valued `just<T>` if there is only one successful type, or `choice<Ts...>` if the sum is heterogeneous.

The n-ary fold of `operator|` is exposed via `fn::disjoin(...)`:

```cpp
#include <fn/expected.hpp>
#include <fn/just.hpp>
#include <fn/copack.hpp>
#include <concepts>

auto test_disjoin() -> void {
    fn::expected<int, Error> a = 12;
    fn::expected<bool, OtherError> b = true;

    // Unary disjoin forwards unchanged
    static_assert(std::same_as<decltype(fn::disjoin(a)), decltype(a)>);

    // Multiple fallible and total operands compose cleanly
    auto result = fn::disjoin(a, b, fn::just<double>{1.5});

    // Because just<double> cannot fail, the entire disjunction becomes total
    static_assert(std::same_as<
        decltype(result),
        fn::choice_for<int, bool, double>
    >);
}
```

> [!TIP]
>
> ### Mathematical note — dual properties of disjunction
>
> Disjunction (`operator|`) is the categorical dual of conjunction (`operator&`):
>
> - **Value addition (Coproduct)**: Successful value channels are combined as a coproduct ($\oplus$), forming a disjoint sum.
> - **Error multiplication (Product)**: Error channels are composed as a cartesian product ($\otimes$), yielding a `pack` of errors.
> - **Product annihilation**: Admitting an identity carrier (whose error side is the initial object $0 \cong \text{copack<>}$) annihilates the error cartesian product:
>   $$E \times 0 \cong 0$$
>   This mathematical property forces the error channel to collapse, rendering the entire disjunction total (never-failing) and folding the result into the identity cluster.
>
## 7. Sequential composition with and_then

Sequential composition chains dependent operations where the success of one feeds the input of the next. In `libfn`, this is achieved using `and_then` (monadic bind).

A monadic carrier wraps a value. A *Kleisli arrow* is the callable passed to `and_then`, which takes a plain value and returns a monadic carrier of the same kind. `and_then(f)` produces a storable operation value that can be piped.

```cpp
#include <fn/expected.hpp>
#include <fn/copack.hpp>
#include <fn/and_then.hpp>
#include <concepts>

auto parse() -> fn::expected<UserId, fn::copack<NotANumber>>;
auto load(UserId) -> fn::expected<User, fn::copack<Missing>>;

auto sequential_bind() -> void {
    auto result = parse() | fn::and_then(load);

    static_assert(std::same_as<
        decltype(result),
        fn::expected<User, fn::copack<Missing, NotANumber>>
    >);
}
```

The strict "same-kind" contract defines how types interact:

- An `optional` binds to an `optional`.
- A plain `expected<T, E>` binds to an `expected<U, E>`, retaining its exact plain error type.
- A copack-graded `expected` can union heterogeneous error sets (as demonstrated above).
- A copack-valued input can join heterogeneous successful branch types into a normalized `copack`.
- Exact branch convergence preserves the exact type without creating duplicate union states.
- All-`void` branches join cleanly to `void`, but mixed void/non-void branches are rejected.
- A bare callback result belongs to `transform`, not `and_then`.

The library formalizes this "same-kind" contract via the `fn::same_kind` concept, which lets generic templates probe whether two carrier types belong to the same monadic family:

```cpp
#include <fn/optional.hpp>
#include <fn/expected.hpp>
#include <fn/concepts.hpp>
#include <concepts>

static_assert(fn::same_kind<fn::optional<int>, fn::optional<User>>);
static_assert(fn::same_kind<fn::expected<int, IoError>, fn::expected<User, IoError>>);
static_assert(!fn::same_kind<fn::expected<int, IoError>, fn::expected<User, Missing>>);
```

> [!TIP]
>
> ### Mathematical note — monads as monoids in endofunctor categories
>
> A **monad** $(M, \eta, \mu)$ on a category $\mathcal{C}$ is a monoid in the monoidal category of endofunctors $([\mathcal{C}, \mathcal{C}], \circ, I_{\mathcal{C}})$. It comprises an endofunctor $M : \mathcal{C} \to \mathcal{C}$ and two natural transformations:
>
> - **Unit ($\eta : I_{\mathcal{C}} \implies M$)**: Lifts $A \to M(A)$.
> - **Multiplication ($\mu : M \circ M \implies M$)**: Flattens nested layers $M(M(A)) \to M(A)$.
>
> The binding operation ($bind : M(A) \to (A \to M(B)) \to M(B)$) is defined as:
>
> $$bind(x, f) = \mu_B(M(f)(x))$$
>
> The monad laws require the following diagrams to commute (expressing associativity and unit relations):
>
> $$\mu \circ M(\mu) = \mu \circ \mu_M \quad \text{and} \quad \mu \circ M(\eta) = id_M = \mu \circ \eta_M$$
>
> In C++, `and_then` implements the bind operation, while `transform` implements the endofunctor map $M(f)$. These laws are verified statically under constant evaluation in Section 13.
>
## 8. Graded expected: exact error sets

`expected` grading provides exactly bounded error sets. When an outer computation holds a coproduct of successful values, and each value requires a different operation to proceed, `libfn` derives a single, normalized `expected` shape.

Consider a configuration reader that parses a loosely typed file into specific valid structural alternatives: `MaximumSize`, `FilePath`, or `BlockSize`.

```cpp
#include <fn/expected.hpp>
#include <fn/copack.hpp>
#include <fn/and_then.hpp>
#include <fn/utility.hpp>
#include <concepts>

auto read_config() -> fn::expected<
    fn::copack_for<MaximumSize, FilePath, BlockSize>,
    fn::copack_for<BadSyntax, UnknownKey>
>;

auto config_pipeline() -> void {
    auto validated = read_config() | fn::and_then(fn::overload{
        [](MaximumSize v) { return fn::expected<MaximumSize, fn::copack<OutOfRange>>{v}; },
        [](FilePath v) { return fn::expected<FilePath, fn::copack<Missing>>{v}; },
        [](BlockSize v) { return fn::expected<BlockSize, fn::copack<OutOfRange>>{v}; }
    });

    // The result exactly bounds both the successful paths and the error paths
    static_assert(std::same_as<
        decltype(validated),
        fn::expected<
            fn::copack<BlockSize, FilePath, MaximumSize>,
            fn::copack<BadSyntax, Missing, OutOfRange, UnknownKey>
        >
    >);
}
```

Two independent joins occurred during `and_then`:

1. The successful branch values formed the normalized value copack.
2. The existing outer errors (`BadSyntax`, `UnknownKey`) and the new branch errors (`OutOfRange`, `Missing`) formed the normalized error copack.

This seamless unioning is what allows different grades of `expected` to share the same carrier family. While standard, un-graded `expected<T, E>` requires the exact same error type `E` to participate in monadic bind (meaning `expected<int, IoError>` and `expected<User, Missing>` are **not** `same_kind`), any two graded `expected` types are considered `same_kind`, regardless of how their individual error sets differ:

```cpp
#include <fn/expected.hpp>
#include <fn/copack.hpp>
#include <fn/concepts.hpp>
#include <concepts>

static_assert(fn::same_kind<
    fn::expected<int, fn::copack<IoError>>,
    fn::expected<User, fn::copack<Missing>>
>);
```

It is crucial to distinguish value joining from error grading:

- You can join branch values while retaining a plain (non-copack) error: `expected<copack_for<A, B>, E>`.
- Differing *plain* errors across branches are completely rejected.
- A `copack` on the error side is the strict opt-in to error-set unioning.

To make composition more user-friendly, `libfn` allows explicit **type promotion** during sequential composition:

- In `and_then` (success binding), a plain error type `E` is automatically promoted to `copack<E>` if the returning error type of the callback is `copack<E>`.
- In `or_else` (recovery/error binding), a plain success type `T` is automatically promoted to `copack<T>` if the returning success type of the callback is `copack<T>`.

This ensures that you can smoothly transition from a simple, un-graded computation to a graded, multi-alternative computation when entering a pipeline step that introduces alternative paths, without needing to manually wrap or lift your starting types.

If you need to perform this promotion explicitly on the carrier itself before entering a composition, `libfn` provides direct, zero-cost member helpers:

- `.copack_error()` on `expected` explicitly lifts the error, transforming `expected<T, E>` to `expected<T, copack<E>>`.
- `.copack_value()` on `expected` explicitly lifts the success value, transforming `expected<T, E>` to `expected<copack<T>, E>`.
- `.copack_value()` on `optional` symmetrically lifts the value, transforming `optional<T>` to `optional<copack<T>>`.

These helper methods provide a compact, explicit alternative to implicit pipeline promotions:

```cpp
#include <fn/expected.hpp>
#include <fn/optional.hpp>
#include <fn/copack.hpp>
#include <concepts>

auto test_explicit_lifting(fn::expected<User, IoError> result, fn::optional<User> opt) -> void {
    // Explicitly lift the error side of expected:
    auto graded_err = std::move(result).copack_error();
    static_assert(std::same_as<
        decltype(graded_err),
        fn::expected<User, fn::copack<IoError>>
    >);

    // Explicitly lift the value side of expected:
    auto graded_val = std::move(result).copack_value();
    static_assert(std::same_as<
        decltype(graded_val),
        fn::expected<fn::copack<User>, IoError>
    >);

    // Explicitly lift the value side of optional:
    auto graded_opt = std::move(opt).copack_value();
    static_assert(std::same_as<
        decltype(graded_opt),
        fn::optional<fn::copack<User>>
    >);
}
```

Recovery via `or_else` behaves symmetrically. It handles input error alternatives and joins any new errors produced by the recovery branches while preserving the already-successful value path. Heterogeneous recovery values require a suitable copack-valued input. Any original error handled by a branch does not automatically remain possible unless a branch explicitly returns it again.

### Widening is subeffecting

In accordance with the subeffecting principles of graded monads (Section 1), a narrow error set can be safely widened during composition, but narrowing requires explicit mitigation. Implicit narrowing (without handling the removed errors) is unsafe and rejected by the compiler. However, you can **safely narrow or collapse** an error grade at any point by explicitly handling and mapping the errors using `transform_error`. Because `transform_error` on a graded `expected` forces exhaustive matching over all possible alternatives, you can map multiple diverse error types into a single common error type (or a narrower `copack`), safely reducing the static error grade of your pipeline.

The bottom error grade is `copack<>`:

```cpp
fn::expected<T, fn::copack<>> cannot_fail{};
```

This computation cannot fail, but it is algebraically prepared to widen if later composition introduces possible errors.

A concrete example of this is `expected<void, copack<>>`. Because `void` represents the unit `1` and `copack<>` represents the zero `0`, this type maps algebraically to $1 + 0 \cong 1$. Having a cardinality of exactly one, it has no possible errors, can never fail, and can only succeed with a single empty trigger (`void`). This makes it structurally isomorphic to the **Unit type**.

In practice, `expected<void, copack<>>` acts as **the graded gateway** to start your pipelines. By initiating a chain with this unit trigger, you seamlessly opt-in all subsequent `and_then` bindings into graded error-set unioning, without having to invent any fake starting errors or manually wrap your initial steps. Since its starting error set is empty (`copack<>`), unioning it with subsequent steps' errors (say, `copack<IoError>`) yields exactly those errors.

> [!TIP]
>
> ### Mathematical note — lax monoidal unit and the neutral element
>
> Having established the error pomonoid $(\mathcal{E}, \cup, \emptyset, \subseteq)$ in Section 1, we can formally define `libfn`'s graded `expected` as a **lax monoidal functor** ($G : \mathcal{E} \to [\mathcal{C}, \mathcal{C}]$) from the pomonoid category $\mathcal{E}$ to the endofunctor category on C++ types (following Orchard, Wadler, and Eades, *Unifying graded and parameterised monads*).
>
> Under this formulation, the type `expected<void, copack<>>` represents the **monadic unit** ($\eta$) of the graded structure:
>
> $$\eta_A : A \to G_I(A) \cong \text{expected}\langle A, \text{copack}\langle\rangle\rangle$$
>
> operating exactly at the neutral identity element $I = \emptyset$ of the error pomonoid. Since $\emptyset \cup F = F$, initiating a computation with this unit trigger ensures that the composition's grade accumulates subsequent effects precisely without introducing spurious terms—making it the rigorous monoidal starting gateway.
>
## 9. The identity cluster

Certain operations behave like an identity functor across different carriers. Because some states correspond structurally, `libfn` licenses specific cross-carrier behavior to prevent redundant boilerplate.

Consider this cross-carrier table:

| Carrier | Algebraic State Shape |
| - | - |
| `just<T>` | **T** (A single value) |
| `choice<Ts...>` | **Ts...** (A coproduct of values) |
| `expected<T, copack<>>` | **T + 0** ≅ **T** (A value and an uninhabited error) |

These three computation carriers have canonically isomorphic state shapes—they all guarantee a successful value of some type.

Because they are equivalent, `libfn` provides a licensed pipeline operation that allows binding across these boundaries:

```cpp
#include <fn/just.hpp>
#include <fn/expected.hpp>
#include <fn/copack.hpp>
#include <fn/and_then.hpp>

auto test_identity_cross() -> void {
    fn::just<UserId> j{UserId{}};

    // Cross-carrier pipeline bind to another identity carrier
    auto result = j | fn::and_then([](UserId u) {
        return fn::expected<UserId, fn::copack<>>{u};
    });

    static_assert(std::same_as<decltype(result), fn::expected<UserId, fn::copack<>>>);
}
```

The bind operation adopts the carrier family of the provided callback. However, the member `and_then` remains strict to its own carrier family. Only the pipeline `operator|` acts as the licensed cross-carrier operation.

Furthermore, fallible types like `expected` (with inhabited error states) and `optional` cannot indiscriminately switch to other carriers, because doing so would risk silently discarding an inhabited state.

Monadic operations behave naturally around this identity cluster:

- `inspect` and `discard` remain meaningful.
- Dead-side operations like `inspect_error` or `transform_error` reject `just` and `choice`, and act vacuously on `expected<T, copack<>>`.
- `fail` and `filter` reject the identity cluster entirely, because no failure state can possibly be constructed from an identity carrier.

> [!TIP]
>
> ### Mathematical note — canonical state-shape isomorphisms
>
> The carriers in the identity cluster exhibit canonical state-shape isomorphisms in the category of types $\mathcal{C}$:
>
> - `just<T>` is the identity functor $I_{\mathcal{C}}(T) \cong T$.
> - `expected<T, copack<>>` is the coproduct of $T$ with the initial object $0$ (the uninhabited `copack<>`), yielding the isomorphism:
>   $$T + 0 \cong T$$
> - `choice<T>` is the single-alternative coproduct monad, isomorphic to $T$.
>
> While these objects are canonically isomorphic, C++ enforces strong nominal type boundaries. `libfn` respects this by refusing implicit conversions (which would pollute the compiler's overload resolution space), choosing instead to expose these isomorphisms through **licensed binds** (cross-carrier pipeline functors) that preserve the information-theoretic equivalence without introducing implicit conversion cycles.
>
## 10. choice: identity over a coproduct

The `choice` carrier represents a computation that always succeeds by selecting one of several alternatives. Structurally, it serves as the single-layer carrier for coproduct states, avoiding the invalid nested `just<copack<Ts...>>` representation discussed in Section 3.

### Decoupling via Pipeline Functors

As established in Section 9, transitions within the identity cluster are strictly restricted to pipeline-scoped functors to preserve decoupling between carriers.

For example, a pipeline-scoped `fn::transform` on a `just` that returns a `copack` is promoted automatically to a `choice`:

```cpp
#include <fn/just.hpp>
#include <fn/choice.hpp>
#include <fn/copack.hpp>
#include <fn/transform.hpp>
#include <concepts>

auto test_identity_transformation() -> void {
    fn::just<UserId> j{UserId{}};

    // Transforming a just with a callable returning a copack produces a choice
    auto mapped = j | fn::transform([](UserId) {
        return fn::copack_for<Missing, FilePath>{Missing{}};
    });

    static_assert(std::same_as<decltype(mapped), fn::choice<FilePath, Missing>>);
}
```

Similarly, a pipeline-scoped `fn::and_then` on a `just` is permitted to return a `choice` or `expected<T, copack<>>` directly.

Inside its own carrier domain, `choice` behaves differently from a bare `copack` in how it maps and binds:

- A `copack` is plain data.
- A `choice` is a never-failing outer computation over those selected alternatives. Every alternative must be handled.

Consider a scenario where different branches of a switch return different `choice` types:

```cpp
#include <fn/choice.hpp>
#include <fn/copack.hpp>
#include <fn/transform.hpp>
#include <fn/and_then.hpp>
#include <fn/utility.hpp>
#include <concepts>

auto test_choice_mapping() -> void {
    fn::choice_for<UserId, User> ch{UserId{}};

    // transform nests the returned choice as a mapped value
    auto mapped = ch | fn::transform(fn::overload{
        [](UserId) { return fn::choice<Missing>{Missing{}}; },
        [](User) { return fn::choice<FilePath>{FilePath{}}; }
    });

    static_assert(std::same_as<
        decltype(mapped),
        fn::choice_for<fn::choice<FilePath>, fn::choice<Missing>>
    >);

    // and_then joins and flattens them into a normalized superset choice
    auto bound = ch | fn::and_then(fn::overload{
        [](UserId) { return fn::choice<Missing>{Missing{}}; },
        [](User) { return fn::choice<FilePath>{FilePath{}}; }
    });

    static_assert(std::same_as<
        decltype(bound),
        fn::choice<FilePath, Missing>
    >);
}
```

Bare-value callbacks are rejected by `choice`'s `and_then`.

> [!TIP]
>
> ### Mathematical note — why copack is not a monad, but choice is
>
> Categorically, `copack<Ts...>` is an object-level **coproduct** (disjoint sum $\bigoplus T_i$), whereas `choice<Ts...>` is a **monad** representing a coproduct-bearing computation context.
>
> 1. **`copack` is self-flattening (not a monad)**:
>    Naked sums are naturally self-flattening (e.g., $(A + B) + C \cong A + B + C$). This property makes nesting impossible ($M \circ M(T) \cong M(T)$), rendering the structural `join`/`flatten` operation a trivial identity map. Because mapping and binding collapse into the same operation, self-flattening structures lose the structural depth needed to satisfy the Monad identity and associativity laws. Symmetrical in its alternatives, `copack` is pure sum data, not an endofunctor.
>
> 2. **`choice` is the monad (the "structural suspend button")**:
>    To restore monad laws, the monadic carrier `choice<Ts...>` wraps the sum in an "identity layer" to preserve structural depth: `choice<choice<T>>` $\ne$ `choice<T>`. This "structural suspend button" holds eager flattening in check.
>    Thus, `choice` acts as a lawful monad under the parameterized endofunctor $M(A) = A + \bigoplus_{j} T_j$:
>    - **Unit / return** ($\eta_A : A \to M(A)$): Canonical injection into the coproduct.
>    - **Join / flatten** ($\mu_A : M(M(A)) \to M(A)$): Strips one layer of the `choice` wrapper, allowing the underlying sum semantics to deduplicate variants (the codiagonal fold $[id, id]$, executed statically via `choice_for`).
>    - **Bind**: Composes callbacks by mapping and explicitly flattening via `join`. This explicit step grants control over *when* flattening occurs, turning a loose collection of types into a rigorous Monad.
>
>
## 11. Elimination and multidispatch

Once your computation shapes are fully derived, you must eliminate the structure to yield an ordinary C++ value. This is done via `apply` or `apply_r`.

It is vital to distinguish `transform` from `apply`:

- `transform` stays *inside* the carrier or copack, producing a new carried type.
- `apply` *eliminates* the structure entirely, requiring all branches to converge on one deduced result type.
- `apply_r<R>` permits branch results acceptable as the specific type `R`.

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

```cpp
#include <fn/expected.hpp>
#include <fn/copack.hpp>
#include <fn/utility.hpp> // for fn::overload

auto test_elimination(fn::expected<UserId, fn::copack<Missing>> ex) -> int {
    return ex.apply(fn::overload{
        [](UserId) { return 1; },
        [](Missing) { return 0; }
    });
}
```

Exhaustiveness is statically constrained. If you omit a handler for a possible type, the compilation fails. `fn::overload` is merely a helper; final selection always relies on ordinary C++ overload resolution.

### Type-tagged elimination

Because multiple structures can share the same unpacking call shape (e.g., `pack<A, B>` and `std::tuple<A, B>` both call `f(a, b)`), untagged `apply` can sometimes erase the structural context of the state. To preserve this context and prevent permissive C++ implicit conversions from accidentally conflating different states, `libfn` provides the **`apply_type`** (and `apply_type_r`) member functions.

When you eliminate a carrier using `apply_type`, the active handler receives an explicit C++ state tag or constructor tag as its first argument, followed by the unpacked payload:

- On `expected`, the success arm receives `std::in_place` followed by the success value, while the error arm receives `fn::unexpect` followed by the error.
- On `optional`, the success arm receives `std::in_place` followed by the value, while the empty arm receives `std::nullopt`.
- On `copack` and `choice`, the active alternative arm receives `std::in_place_type<T>` followed by the payload.
- On `just`, the active arm receives `std::in_place_type<T>` (or `std::in_place_type<void>` for empty/nullary states).

> [!TIP]
>
> ### Mathematical note — elimination of algebraic structures
>
> In category theory, the dual nature of products and coproducts defines how they are **eliminated** (mapped back to ordinary objects):
>
> - **Product elimination**: To eliminate a product $A \times B$, one supplies a morphism $f : A \times B \to C$ that takes all components simultaneously. In `libfn`, this is achieved by passing a multi-argument callable to a `pack`'s `apply`.
> - **Coproduct elimination**: To eliminate a coproduct $A + B$, one supplies a family of morphisms $\{f : A \to C, g : B \to C\}$ that converge on a common target. The universal property yields a unique morphism $[f, g] : A + B \to C$. In `libfn`, this maps to passing an **overload set** to a `copack`'s `apply`, where ordinary C++ overload resolution acts as the unique mediating morphism.
>
> Carrier elimination (`apply_type`) preserves the canonical injections by supplying explicit state tags (such as `std::in_place` or `std::in_place_type<T>`) alongside the payload. This ensures that the caller retains the exact information of *which* injection morphism placed the value into the structure.
>
## 12. The monadic operations map

This is a concise reference for `libfn`'s operations, organized by channel and effect:

**Success Channel**

- `transform`: Maps the successful value. Stays inside the carrier.
- `and_then`: Sequences computations. The mechanism for introducing new errors into a graded expected.
- `filter`: Enters a short-circuit state if a predicate fails. Does not widen error grades.
- `inspect`: Observes the successful value transparently.

**Error/Empty Channel**

- `transform_error`: Maps the error value. Stays inside the carrier.
- `or_else`: Sequences computations based on errors. Joins recovery values.
- `recover`: Same as `or_else`, but always wraps raw values back into a success state.
- `inspect_error`: Observes the error value transparently.
- `value_or`: Eliminates the carrier by supplying a fallback value on failure.
- `fail`: Short-circuits success into a forced failure state. Does not widen error grades.

**Neutral**

- `discard`: Unconditionally evaluates the carrier, discards the result, and returns `void`. This is used to signal to the compiler that the return value is deliberately ignored.

**Composition & Combination**

- `operator&` (conjunction): Combines independent computations (values into a `pack`, errors as a union).
- `operator|` (disjunction): Combines alternative computations (values into a `copack` disjoint sum, errors as a product `pack`).
- `fn::conjoin`: An n-ary fold of `operator&` over packs, copacks, or scalars.
- `fn::disjoin`: An n-ary fold of `operator|` over monadic carriers, supporting total disjunction with the identity cluster.

### Key Architectural Rules of the Map

To reason about how these operations affect the type algebra of your computation:

- **Graded `and_then`** is the primary mechanism for introducing a _new_ error type (widening the error grade) into your pipeline.
- **`filter` and `fail`** merely enter an _existing_ short-circuit state. They do not widen the error grade (the type must already be capable of holding the failure state).
- **Error-side monadic operations** (like `transform_error`, `or_else`, `recover`, and `inspect_error`) are only well-formed if the carrier has an appropriate error or empty side (and are rejected on identity carriers like `just` or `choice`).


## 13. Laws as C++ equalities

The algebraic laws governing `libfn` shapes are verified by the compiler where structural capabilities permit. For instance, you can observe functor identity and monad left identity in `constexpr` contexts:

```cpp
#include <fn/expected.hpp>
#include <fn/copack.hpp>
#include <fn/transform.hpp>
#include <fn/and_then.hpp>
#include <concepts>

constexpr auto test_laws() -> void {
    fn::expected<int, fn::copack<Missing>> ex{42};

    // Functor Identity: mapping with identity yields the same value
    auto id = [](auto v) { return v; };
    static_assert((ex | fn::transform(id)) == ex);

    // Monad Left Identity: pure(x) >>= f is equivalent to f(x)
    auto pure = [](int v) { return fn::expected<int, fn::copack<Missing>>{v}; };
    auto f = [](int v) { return fn::expected<int, fn::copack<Missing>>{v * 2}; };
    static_assert((pure(42) | fn::and_then(f)) == f(42));
}
```

Other properties hold structurally:
- **Functor composition**: `transform(f) | transform(g)` equals `transform(g(f(x)))`.
- **Monad right identity**: `m | and_then(pure)` equals `m`.
- **Monad associativity**: `(m | and_then(f)) | and_then(g)` equals `m | and_then(\x -> f(x) | and_then(g))`. For graded expected, both sides of the associativity derive the exact same normalized union grade.
- **Product associativity**: Holds after canonical `pack` normalization.
- **Coproduct set semantics**: Union associativity, commutativity, and idempotence apply.
- **Coherent widening**: Upcasting an error through intermediate supersets yields the same final type as upcasting directly to the broadest superset.
- **Identity cluster binds**: Laws hold across `just`, `choice`, and `expected<T, copack<>>` via the canonical payload-preserving state-shape correspondence.


## 14. C++ mechanics that preserve the algebra

To make the algebraic model reliable in everyday C++, `libfn` uses extensive compiler mechanisms to reject malformed usage and preserve performance properties.

### Constraints and exhaustiveness

Public concepts and `requires` clauses enforce correctness before instantiation. Operations are protected by applicability concepts (negative probes) that proactively reject impossible calls. This underpins the compile-time exhaustiveness guarantees of `apply` and monadic operations established in Sections 3 and 11, catching unhandled alternatives at the boundary of instantiation rather than deep inside template machinery.

### C++ value properties

`libfn` thoroughly respects C++ value mechanics:
- Core operations are fully `constexpr`.
- Types are structural if their elements permit (allowing them as non-type template parameters).
- `noexcept` is conditionally computed based on the operations provided.
- Value categories (lvalue/rvalue) propagate strictly to callbacks, avoiding unnecessary copies.
- Immovable and move-only payloads are fully supported in place.
- Reference-bearing packs and `optional<T&>` are deliberately supported. Lifetime responsibility for non-owning references remains with the caller.

```cpp
#include <fn/optional.hpp>
#include <concepts>

auto test_references() -> void {
    int x = 42;
    fn::optional<int&> opt{x};
    static_assert(std::same_as<decltype(opt.value()), int&>);
}
```

### pfn and fn

The library is divided into layers:

- `pfn` (Polyfill fn) is the standards-facing layer. It provides polyfills of `std::optional` and `std::expected`, conforming to standard C++26 (and later) shapes.
- `fn` is the strict extension layer. It introduces the `pack`/`copack` algebra, multidispatch, graded errors, `choice`, `just`, and the cross-carrier pipeline monadic operation (`operator|`).

## Functional terminology

For readers with a background in functional languages (like Haskell or OCaml), this table translates standard terminology to `libfn`'s C++ vocabulary:

| Functional Term | `libfn` Equivalent |
| --------------- | ------------------ |
| `fmap` / `map` | `transform` / `transform_error` |
| `bind` / `>>=` | `and_then` |
| `pure` / `return` | Constructor / `just` / Factory functions |
| Kleisli arrow | The callable passed to `and_then` |
| Product type | `pack` / `std::tuple` |
| Coproduct / Sum | `copack` / `choice` |
| Subeffecting | Widening an error grade / subset inclusion |

## Further reading

For formal validation of the algebraic structures modeled in `libfn`, refer to:

1. Orchard and Petricek, [“Embedding effect systems in Haskell”](https://www.doc.ic.ac.uk/~dorchard/publ/haskell14-effects.pdf) (for effect sets, union, and subeffecting).
2. Orchard, Wadler, and Eades, [“Unifying graded and parameterised monads”](https://arxiv.org/pdf/2001.10274) specifically Definition 21 (for the graded-monad interpretation).
3. McDermott and Uustalu, [“Flexibly Graded Monads and Graded Algebras”](https://dylanm.org/flexibly-graded-monads.pdf) _Note: `libfn` does not claim to fully implement their flexibly graded construction, but the work contextualizes graded structures._
