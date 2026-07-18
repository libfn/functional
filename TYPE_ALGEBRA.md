# Type algebra and functional composition in libfn

libfn is built around a practical observation: in a compositional C++ program, the compiler should
derive not only the next value type, but also the complete shape of the computation. If parsing can fail
with `NotANumber`, validation with `OutOfRange`, and storage with `IoError`, their composition should
carry exactly those error types. If two successful operations produce an `int` and a `record`, their
composition should carry both values. If either successful value may have several exact types, the
compiler should derive every valid combination and require a handler for each one.

The vocabulary for those shapes is small:

- `fn::pack<Ts...>` is an ordered product: all of the `Ts...` are present.
- `fn::copack<Ts...>` is a coproduct: exactly one of the `Ts...` is present.
- `fn::optional<T>` adds one empty state to `T`.
- `fn::expected<T, E>` adds an error channel `E` to a successful value `T`.
- `fn::just<T>` is the minimal always-present computation.
- `fn::choice<Ts...>` is identity over a coproduct: it gives a selected alternative the operations
  needed to participate in a functional pipeline.

`transform`, `and_then`, and `operator&` compose those shapes. `apply` eliminates the final shape by
passing its contents to an ordinary C++ callable. The derived types are not an implementation detail;
they are the principal explanation of what the program can do.

The examples below assume the corresponding `fn` headers and ordinary standard headers have been
included. Error and value types are deliberately small so that the computed result types remain visible:

```cpp
struct NotANumber {};
struct OutOfRange {};
struct Missing {};
struct IoError {};

struct UserId { int value; };
struct User { UserId id; };
```

> [!IMPORTANT]
> Two layers remain separate throughout this document. The **type algebra** records exact stored C++
> types using `pack` and `copack`. The **application protocol** uses `apply` and overload resolution to
> turn a stored value into function arguments. Two stored types may have the same call shape without
> becoming the same type.

## 1. Why compose types as well as values?

Consider three conventional functions:

```cpp
auto parse_id(std::string_view) -> fn::expected<UserId, fn::copack<NotANumber>>;
auto validate(UserId) -> fn::expected<UserId, fn::copack<OutOfRange>>;
auto load(UserId) -> fn::expected<User, fn::copack_for<Missing, IoError>>;
```

Each function states only its local failure modes. A sequence composes them:

```cpp
auto find_user(std::string_view text)
{
  return parse_id(text)
       | fn::and_then(validate)
       | fn::and_then(load);
}

using find_user_result = decltype(find_user("42"));
static_assert(std::same_as<
    find_user_result,
    fn::expected<User,
                 fn::copack_for<NotANumber, OutOfRange, Missing, IoError>>>);
```

The last `static_assert` is the point. No function repeats a project-wide error enum, no caller manually
maintains a `variant`, and no conversion erases the distinction between “not a number” and “not found.”
The compiler calculates the union of the error types introduced along the path. Changing `load` to add
`PermissionDenied` changes the derived type of `find_user`; code that exhaustively handles its errors
must then acknowledge the new case.

This addresses a recurring C++ problem. `std::expected<T, E>` composes naturally when every stage uses
the same `E`, but applications often obtain that uniformity by creating one large error enum or variant.
That central type couples otherwise independent functions. It may also hide an operation's real
contract: a parser appears able to report an I/O error merely because both errors inhabit the common
type. libfn instead treats a set of possible error types as a type-level grade and computes it during
composition.

The same principle applies to successful values. `operator&` combines independent results:

```cpp
auto user = load(UserId{7});
auto quota = fn::expected<int, fn::copack<IoError>>{10};
auto both = user & quota;

static_assert(std::same_as<
    decltype(both),
    fn::expected<fn::pack<User, int>,
                 fn::copack_for<Missing, IoError>>>);
```

The result type says both successful values are present and either operation may have produced its own
error. When alternatives appear on the successful side, products distribute over them, producing the
rows needed for multidispatch.

The library rejects compositions whose contracts cannot be made honest. A callback passed to
`and_then` must return an appropriate computation carrier; a missing multidispatch arm makes `apply`
inapplicable; an error type cannot be silently dropped; and a nested libfn `pack` is rejected instead of
creating a second spelling for the same flat product. These failures occur at the expression that tries
to compose incompatible shapes.

## 2. Types as an algebra: zero, unit, alternatives, and products

For this document, an *algebra* is simply a collection of types with operations and laws. The useful
analogy is ordinary arithmetic:

```text
0        copack<>              no possible value
1        pack<>                exactly one value, containing no fields
A + B    copack_for<A, B>      either an A or a B
A × B    pack<A, B>            both an A and a B
```

The symbols describe sets of possible values, not C++ type aliases. For example:

```text
optional<T>    ≅ 1 + T
expected<T,E>  ≅ T + E
```

Here `≅` means “has the same state shape as,” not `std::same_as`. `optional<T>` has an empty state and a
`T` state. `expected<T,E>` has a success state and an error state. Constructor tags keep those states
distinct even when `T` and `E` are the same C++ type; `expected<int, int>` is therefore meaningful,
whereas a normalized type-indexed coproduct would deduplicate two `int` alternatives.

### Zero is not unit

`fn::copack<>` is uninhabited: no value of it can be constructed. `fn::pack<>` is inhabited exactly
once: there are no fields whose values could vary.

```cpp
static_assert(fn::empty_copack<fn::copack<>>);
static_assert(std::tuple_size_v<fn::pack<>> == 0);
static_assert(fn::apply([] { return 42; }, fn::pack<>{}) == 42);
```

This distinction controls edge cases throughout the library. A callback over a `copack<>` side is never
called because there is no value to supply. A callback over `pack<>` is called with zero arguments.

Consequently:

```text
expected<T, copack<>>   always has a T
expected<copack<>, E>   always has an E
optional<copack<>>      is always empty
expected<void, E>       has either a successful unit state or an E
```

`void`, `pack<>`, and `copack<>` must not be conflated. `void` is used by C++ interfaces to express the
absence of a returned object. `pack<>` is a real nullary product value. `copack<>` is a type with no
values at all.

### Copacks use set semantics

`std::variant<Ts...>` is a positional list of alternatives. Repeated types remain repeated and are
distinguished by index. libfn's coproduct is indexed by exact type. `copack_for` flattens nested
copacks, removes duplicates, and places alternatives in canonical type order:

```cpp
using errors = fn::copack_for<
    IoError,
    fn::copack_for<Missing, IoError>,
    NotANumber>;

static_assert(std::same_as<
    errors,
    fn::copack_for<NotANumber, Missing, IoError>>);
static_assert(errors::size == 3);
```

Thus coproduct union is commutative, associative, idempotent, and has `copack<>` as its unit:

```text
A ∪ B = B ∪ A
(A ∪ B) ∪ C = A ∪ (B ∪ C)
A ∪ A = A
A ∪ ∅ = A
```

The compiler rejects distinct types whose provisional sort keys collide; it must never silently erase
an alternative. Current implementations derive keys from compiler type spellings. C++26
`std::type_order`, proposed in [P2830](https://wg21.link/P2830) and motivated in part by libfn, provides
the standard facility intended to replace that provisional ordering.

> **Mathematical note — products and coproducts**
>
> A product `A × B` carries projections to `A` and `B`; a coproduct `A + B` carries injections from
> `A` and `B`. `pack<A,B>` and `copack_for<A,B>` are the corresponding programming shapes.
> `pack<>` is the nullary product (a terminal/unit object at this level), while `copack<>` is the
> nullary coproduct (an initial/empty object). libfn chooses canonical flat representations for its
> own products and normalized representations for its coproducts. The main text needs only the
> concrete C++ laws above.

## 3. The vocabulary types

The vocabulary types answer two questions: what states can exist, and what interface should a value
offer while it is being composed?

### `pack`: all fields are present

```cpp
fn::pack credentials{UserId{7}, std::string{"token"}};

auto& [id, token] = credentials;
static_assert(std::tuple_size_v<decltype(credentials)> == 2);

using std::get;
static_assert(std::same_as<decltype(get<0>(credentials)), UserId&>);
```

`pack<Ts...>` is an ordered, flat product. It follows the tuple protocol:
`std::tuple_size`, `std::tuple_element`, and the ADL-found free `fn::get<I>` make structured bindings
and generic tuple code work. Its additional purpose is composition. `append` adds a scalar or splices
another pack:

```cpp
auto row = fn::pack{UserId{7}}.append(std::string{"token"});
auto wider = std::move(row).append(fn::pack{true, 3});

static_assert(std::same_as<
    decltype(wider),
    fn::pack<UserId, std::string, bool, int>>);
```

Order is meaningful; elements are neither sorted nor deduplicated. `pack<A,B>` differs from
`pack<B,A>`, and `pack<A,A>` has two fields. A nested libfn pack is ill-formed:

```cpp
// fn::pack<fn::pack<int, double>, char> bad; // rejected
```

Use `append` for the canonical flat product. To preserve a product-shaped value as one semantic field,
wrap it in a user type or use another tuple-like vocabulary type.

### `copack`: one exact alternative is present

```cpp
struct IdentifierToken { std::string_view text; };
struct IntegerToken { long long value; };
struct StringToken { std::string_view value; };

using Token = fn::copack_for<
    IdentifierToken,
    IntegerToken,
    StringToken>;

Token token{IdentifierToken{"timeout"}};
assert(token.has_value<IdentifierToken>());
assert(token.get_ptr<IntegerToken>() == nullptr);
```

`copack` removes positional bookkeeping from variant-like code. Construction, observation, widening,
and dispatch name exact types. This is useful well beyond error channels. A lexer can return the token
type it just recognized; a configuration reader can return the exact kind of value found; and a data
decoder can retain whether an input was textual, integral, or structured. Code receiving the value
does not maintain a parallel enum or positional index—the C++ type is the discriminator.

A plain copack is appropriate while that selection is data to store, return, or inspect. When the next
operation should dispatch over the selected type as part of a functional pipeline, `choice` gives the
same coproduct-shaped value the required sequencing operations.

An alternative may itself be a `pack`, a standard tuple-like type, or an ordinary user type; those
remain distinct:

```cpp
using representations = fn::copack_for<
    UserId,
    fn::pack<UserId>,
    std::tuple<UserId>,
    std::array<UserId, 1>>;

static_assert(representations::size == 4);
```

All four can eventually apply to a unary `UserId` handler, but application shape does not affect
coproduct identity.

### The computation carriers

The remaining types put values into computations:

```cpp
fn::just<UserId> ready{UserId{7}}; // always contains UserId
fn::optional<UserId> cached{};     // empty or UserId

fn::expected<UserId, fn::copack<NotANumber>> parsed{UserId{7}};
fn::choice_for<UserId, User> selected{UserId{7}};
```

`just<T>` is minimal: one payload, no empty state, and no error channel. `just<void>` is the
payload-free unit. A `just<copack<...>>` is deliberately rejected; identity over those alternatives
has the canonical spelling `choice<...>`. The specialization `choice<>` is incomplete and cannot be
instantiated: an always-present choice requires at least one alternative. The uninhabited spelling is
`copack<>`.

`optional<T>` and `expected<T,E>` retain the standard vocabulary and standard sequencing members, extended
by libfn where type algebra is involved. `choice<Ts...>` has the same selected-alternative storage
shape as a copack but belongs in a pipeline. This separation prevents plain data from accidentally
acquiring computation semantics.

Construction tells the compiler which states are intended. Constraints reject reference, array, cv,
tag, nested-copack, or nested-pack shapes where the relevant carrier cannot represent them canonically.
The diagnostics are therefore about an invalid type shape, rather than a later ambiguous visit or
unreachable runtime state.

## 4. Mapping values and errors

`transform` changes a successful value without changing whether the computation succeeded. It is the
operation often called `map` or `fmap` in functional libraries.

```cpp
auto parse_length(std::string_view text)
    -> fn::expected<std::string, fn::copack<NotANumber>>;

auto length = parse_length("hello")
            | fn::transform([](std::string const& s) { return s.size(); });

static_assert(std::same_as<
    decltype(length),
    fn::expected<std::size_t, fn::copack<NotANumber>>>);
```

The value type changes from `std::string` to `std::size_t`; the error type is untouched. If the input
already holds `NotANumber`, the callback is not invoked. `optional`, `expected`, `just`, and `choice`
all support this value-side operation, subject to what their result carrier can represent.

`transform_error` maps the other channel:

```cpp
struct Message { std::string text; };

auto displayed = parse_id("x")
               | fn::transform_error(fn::overload{
                   [](NotANumber) { return Message{"not a number"}; }
                 });

static_assert(std::same_as<
    decltype(displayed),
    fn::expected<UserId, fn::copack<Message>>>);
```

For a copack error, the callable must cover every inhabited alternative. Different arms may produce
different exact error types; normalization forms their result copack. A missing arm makes the
operation's constraints unsatisfied. That is stronger than accepting a generic visitor that happens
to throw or return an unrelated fallback.

`transform_error` has no meaningful job on `just` or `choice`: neither has an error or empty side.
Those carriers are rejected by the verb. An `expected<T,copack<>>` does expose the expected interface,
but its error side is uninhabited; its error transformation is vacuous and does not instantiate the
callback.

> **Mathematical note — functors**
>
> A functor maps a function `f : A -> B` to a structure-preserving operation
> `map(f) : F<A> -> F<B>`. In libfn vocabulary, `map` is `transform`. It should preserve identity and
> composition:
>
> ```text
> x.transform(identity) == x
> x.transform(f).transform(g) == x.transform(g after f)
> ```
>
> These equalities concern regular values and pure callbacks. C++ callbacks with observable side
> effects, self-observing moves, or other non-regular behavior can distinguish operational details
> that the type algebra intentionally abstracts.

## 5. Product composition with `operator&`

`operator&` combines successful results. It does not call the next step; it constructs the value shape
that a later step will receive.

```cpp
auto id = fn::expected<UserId, fn::copack<NotANumber>>{UserId{7}};
auto name = fn::expected<std::string, fn::copack<IoError>>{"Ada"};

auto row = id & name;

static_assert(std::same_as<
    decltype(row),
    fn::expected<fn::pack<UserId, std::string>,
                 fn::copack_for<NotANumber, IoError>>>);
```

Two successful scalar values form a pack. Adding a third appends to the same flat product. If either
operand has already failed, the result carries an error, widened into the derived error copack when
necessary. `expected<void,E>` contributes no successful field, so it behaves as a successful unit in
the product while still contributing `E` to the possible error set.

On the payload algebra itself, products distribute over coproducts:

```cpp
struct ById {};
struct ByName {};
struct Memory {};
struct Database {};

using Query = fn::copack_for<ById, ByName>;
using Source = fn::copack_for<Memory, Database>;
using Routes = decltype(fn::identity(Query{ById{}}, Source{Memory{}}));

static_assert(std::same_as<
    Routes,
    fn::copack_for<
        fn::pack<ById, Memory>,
        fn::pack<ById, Database>,
        fn::pack<ByName, Memory>,
        fn::pack<ByName, Database>>>);
```

At runtime there is still only one selected row. The result type contains every row that may be
selected. This is the cartesian expansion needed for later multidispatch.

`fn::identity` is the variadic lift for this composition. With one argument it forwards that argument.
With several scalars it starts a pack; with a pack or copack it uses the same flattening and
distribution as `operator&`. It is not the product unit and not the identity monad. The product unit is
`pack<>`; the identity carriers are described later.

The compiler rejects a composition when no overload of `operator&` can preserve both carrier contracts,
or when an element cannot be constructed in the derived product. Association can change which
payloads are moved at each step:

```cpp
p.append(q).append(r);
p.append(q.append(r));
```

Both have the same final flat type and, for regular elements, the same ordered values. Their operational
move trees may differ.

> **Mathematical note — monoidal composition**
>
> Product composition is associative up to libfn's canonical pack flattening, with `pack<>` as unit.
> Coproduct distribution materializes
> `(A + B) × (C + D)` as the four corresponding product alternatives. For graded expected,
> `operator&` also combines grades by union. This is the monoidal presentation commonly related to
> an Applicative functor, but libfn exposes product composition directly rather than presenting a
> wrapped-function application operator.

## 6. Sequential composition with `and_then`

`and_then` is used when the next function may itself fail, be empty, or select among alternatives.
Such a type is called a *monadic carrier* here: it holds a computation's current value together with
whatever short-circuit state the particular carrier supports.
The lambda passed to it is sometimes called a *Kleisli arrow*; in C++ terms it is simply a callable
from an unpacked successful value to another monadic value.

```cpp
auto result = parse_id("42")
            | fn::and_then(validate)
            | fn::and_then(load);
```

The pipeline operator treats a verb as a value describing a step:

```cpp
auto validated = fn::and_then(validate); // stores the callable
auto result2 = parse_id("42") | validated;
```

The step runs only when a value reaches it. This spelling lets programs assemble and store operations
without hiding control flow in exceptions or callbacks owned by a framework.

For the standard-shaped case, the callback must return the same carrier family. `optional<T>` binds to
another optional; an ordinary `expected<T,E>` binds to an expected with the same `E`. The public
`same_kind` concept describes this family-level relationship:

```cpp
static_assert(fn::same_kind<
    fn::optional<int>,
    fn::optional<User>>);

static_assert(fn::same_kind<
    fn::expected<int, IoError>,
    fn::expected<User, IoError>>);

static_assert(!fn::same_kind<
    fn::expected<int, IoError>,
    fn::expected<User, Missing>>);
```

An `expected` opts into graded-error composition when its input error type is any copack, including
`copack<>`; different error sets can then be joined without loss. Section 8 explains grading, and the
identity cluster later adds one licensed cross-carrier bind.

A callback returning a bare value belongs with `transform`, not `and_then`. A callback returning an
unrelated carrier is rejected. An incomplete overload set over a copack payload is also rejected,
because no runtime alternative may escape the sequence without a defined next step. Concepts such as
`applicable_and_then` let generic code ask these questions without intentionally producing a compiler
diagnostic.

> **Mathematical note — monads**
>
> Wrapping a value is often called `pure` or `return`; `and_then` is `bind`. Bind removes one layer of
> the carrier returned by the callback:
>
> ```text
> bind : M<A> -> (A -> M<B>) -> M<B>
> ```
>
> Its laws are left identity, right identity, and associativity. In C++ they hold for lawful carrier
> operations, regular payloads, and pure callbacks. “Kleisli composition” means composing the
> callbacks accepted by `and_then`, rather than manually unwrapping their intermediate carriers.

## 7. `choice`: identity over a coproduct

`copack<Ts...>` is data. `choice<Ts...>` gives a selected coproduct alternative the monadic operations
needed to continue a computation. It is identity over a coproduct: there is no empty state or error
state, but the held exact type is selected at runtime. Token parsing is a natural example: after the
lexer has produced a value whose exact type records what it recognized, later stages can dispatch on
that type without re-reading a tag.

```cpp
using ExpressionToken = fn::choice_for<
    IdentifierToken,
    IntegerToken>;

ExpressionToken token{IdentifierToken{"count"}};

auto category = token.transform(fn::overload{
    [](IdentifierToken) { return std::string_view{"name"}; },
    [](IntegerToken) { return std::string_view{"integer"}; }
});

static_assert(std::same_as<
    decltype(category),
    fn::choice<std::string_view>>);
```

Every alternative must be handled. Different branches may return different exact value types, in which
case `transform` forms a normalized choice over those result types.

The crucial distinction appears when a branch itself returns a choice:

```cpp
struct IdentifierExpr { std::string_view name; };
struct IntegerExpr { int value; };
struct BigIntegerExpr { long long value; };

constexpr auto make_expression = fn::overload{
    [](IdentifierToken t) {
      return fn::choice<IdentifierExpr>{
          IdentifierExpr{t.text}};
    },
    [](IntegerToken t) -> fn::choice_for<IntegerExpr, BigIntegerExpr> {
      if (t.value >= -1'000 && t.value <= 1'000)
        return IntegerExpr{static_cast<int>(t.value)};
      return BigIntegerExpr{t.value};
    }
};

using mapped = decltype(
    std::declval<ExpressionToken>().transform(make_expression));
using bound = decltype(
    std::declval<ExpressionToken>().and_then(make_expression));

static_assert(std::same_as<
    mapped,
    fn::choice_for<
        fn::choice<IdentifierExpr>,
        fn::choice_for<IntegerExpr, BigIntegerExpr>>>);
static_assert(std::same_as<
    bound,
    fn::choice_for<
        IdentifierExpr,
        IntegerExpr,
        BigIntegerExpr>>);
```

`transform` treats each returned choice as the mapped value. It preserves that returned computation as
an alternative. `and_then` performs monadic join: it splices the returned alternatives into one
normalized superset choice. Overlapping alternatives are deduplicated.

A bare-value callback is rejected by `and_then`; that operation requires a choice-producing branch.
An inapplicable branch makes the member unavailable. This compile-time exhaustiveness is what permits
the result type to be derived without a fallback alternative.

> **Mathematical note — map versus join**
>
> Functor mapping has type `F<A> -> (A -> B) -> F<B>` and does not remove structure from `B`.
> Monadic bind can be decomposed into map followed by join:
>
> ```text
> join : M<M<A>> -> M<A>
> bind(x, f) = join(map(x, f))
> ```
>
> For `choice`, join is coproduct union: it flattens branch choices into their normalized superset.
> Calling `choice` “identity over a coproduct” emphasizes that no failure effect is added; the
> coproduct controls dispatch, while the identity carrier supplies sequencing.

## 8. Graded expected: exact error sets

A graded computation carries a compile-time description of its effect in addition to its value type.
In libfn the important grade is the normalized set of possible errors:

```cpp
template <typename T, typename... Es>
using result = fn::expected<T, fn::copack_for<Es...>>;
```

The successful value and the error grade are independent. A configuration reader, for example, may
return a coproduct describing which kind of setting it read while retaining a separate coproduct of
ways the read could fail:

```cpp
struct MaximumSize { std::size_t value; };
struct FilePath { std::string value; };
struct BlockSize { std::size_t value; };

struct BadSyntax {};
struct UnknownKey {};

using Setting = fn::copack_for<
    MaximumSize,
    FilePath,
    BlockSize>;

using ReadSetting = fn::expected<
    Setting,
    fn::copack_for<BadSyntax, UnknownKey>>;

auto read_setting(std::string_view) -> ReadSetting;
```

The value copack says what was read; only the error copack is the grade. Unlike `choice`, this
computation can fail—the successful payload merely happens to be coproduct-shaped. A later overloaded
stage can validate each setting type exhaustively and introduce its own local error set:

```cpp
using Validated = fn::expected<
    Setting,
    fn::copack_for<OutOfRange, Missing>>;

constexpr auto validate_setting = fn::overload{
    [](MaximumSize s) -> Validated {
      return Setting{s};
    },
    [](FilePath s) -> Validated {
      return Setting{std::move(s)};
    },
    [](BlockSize s) -> Validated {
      return Setting{s};
    }
};

using ValidatedSetting = decltype(
    std::declval<ReadSetting>()
    | fn::and_then(validate_setting));

static_assert(std::same_as<
    ValidatedSetting,
    fn::expected<
        fn::copack_for<MaximumSize, FilePath, BlockSize>,
        fn::copack_for<
            BadSyntax, UnknownKey, OutOfRange, Missing>>>);
```

The overloads converge on the same `Validated` carrier. The heterogeneous superset join described for
`choice` is specific to choice bind; an expected continuation must have one result carrier type.

The compiler preserves both dimensions: the possible validated value types remain a normalized copack,
while `and_then` unions the reader's and validator's error sets.

`operator&` performs the same error union for computations composed by product:

```cpp
using left = fn::expected<int, fn::copack<IoError>>;
using right = fn::expected<bool, fn::copack_for<IoError, Missing>>;
using combined = decltype(std::declval<left>() & std::declval<right>());

static_assert(std::same_as<
    combined,
    fn::expected<fn::pack<int, bool>,
                 fn::copack_for<IoError, Missing>>>);
```

### Widening is subeffecting

A result with fewer possible errors converts to one with a superset:

```cpp
using narrow = fn::expected<User, fn::copack<Missing>>;
using wide = fn::expected<User, fn::copack_for<Missing, IoError>>;

narrow n{User{UserId{7}}};
wide w = n;
```

The conversion does not invent an `IoError`. It changes the static approximation from “may fail with
`Missing`” to “may fail with `Missing` or `IoError`.” Narrowing is rejected because it could discard an
active alternative.

The bottom grade is `copack<>`:

```cpp
using total = fn::expected<UserId, fn::copack<>>;
static_assert(fn::some_identity<total>);
```

Its error state cannot be constructed, so the computation cannot fail. `and_then` and `operator&` may
widen it when a later stage introduces errors; union with the empty set contributes nothing.

This is more precise than a single application-wide error type. A function's signature describes the
errors reachable at that point, while widening supplies ordinary substitutability. Exhaustive error
handling sees the derived superset, so adding a new local error is visible to downstream code.

> **Mathematical note — the error pomonoid**
>
> The grades form the partially ordered monoid
> `(finite sets of types, ∪, ∅, ⊆)`. “Partially ordered” means inclusion relates grades;
> “monoid” means union is associative and has the empty set as unit. Graded bind has type
>
> ```text
> M_E<A> -> (A -> M_F<B>) -> M_(E ∪ F)<B>
> ```
>
> and widening along `E ⊆ F` is effect approximation or subeffecting. In categorical language a
> graded monad can be presented as a lax monoidal functor from this grading monoid; see
> Orchard, Wadler, and Eades, Definition 21, linked under Further reading.

## 9. The identity cluster and the zero/unit corners

libfn has three carrier spellings whose short-circuit side is uninhabited:

```cpp
fn::just<T>
fn::choice<Ts...>
fn::expected<T, fn::copack<>>
```

The public concept `fn::some_identity` names this cluster. None is *the* identity type:

- `just<T>` is the minimal always-present carrier for an ordinary payload.
- `choice<Ts...>` is identity over a selected coproduct alternative.
- `expected<T,copack<>>` is the bottom, never-failing member of the graded expected family.

Where their payload shapes correspond, conversions between their meanings lose no information because
the discarded empty/error channel has no possible value. The verb form of `and_then` is the licensed
cross-carrier operation:

```cpp
auto a = fn::just{UserId{7}}
       | fn::and_then([](UserId id) {
           return fn::expected<UserId, fn::copack<>>{id};
         });

static_assert(std::same_as<
    decltype(a),
    fn::expected<UserId, fn::copack<>>>);

auto b = fn::expected<UserId, fn::copack<>>{UserId{7}}
       | fn::and_then([](UserId id) {
           return fn::just<User>{User{id}};
         });

static_assert(std::same_as<decltype(b), fn::just<User>>);
```

The bind follows the function: the callback's identity carrier becomes the result carrier. Carrier
members remain strict about their own family; the cross-carrier rule belongs to the pipeline verb.
A fallible expected or an optional cannot use it, because dropping its live short-circuit state would
lose information.

For a coproduct payload, all branches may converge on one identity carrier or return choices that join:

```cpp
using Source = fn::expected<
    fn::copack_for<IdentifierToken, IntegerToken>,
    fn::copack<>>;

using crossed = decltype(
    std::declval<Source>()
    | fn::and_then(make_expression));

static_assert(std::same_as<
    crossed,
    fn::choice_for<
        IdentifierExpr,
        IntegerExpr,
        BigIntegerExpr>>);
```

`transform` also respects the canonical spelling. A `just` callback returning a non-empty copack cannot
produce the forbidden `just<copack<...>>`, so the verb promotes it to the corresponding `choice`.

### Which verbs make sense?

Value-side observation and termination work across the cluster. `inspect` observes the always-present
payload and passes the carrier through; `discard` ends the computation.

`just` and `choice` have no dead side, so `or_else`, `recover`, `transform_error`, `inspect_error`, and
`value_or` reject them. An identity expected retains those expected-family operations, but the dead-side
ones are vacuous: their callbacks are not instantiated and the value passes through.

`fail` and `filter` reject the entire identity cluster, including `expected<T,copack<>>`. They would
need to create a short-circuit state, but no such state exists. A vacuous implementation would lie about
the pipeline's failure modes; the constraint instead tells the programmer to widen the grade first or
choose a carrier that can represent rejection.

### Corners that are not identity

`optional<copack<>>` is always empty, not always successful. `expected<copack<>,E>` always contains an
error. They are zero-valued corners whose value-side callbacks cannot run. In contrast,
`expected<void,E>` has a real successful unit state, and its successful callbacks are nullary.

> **Mathematical note — the bottom grade and canonical isomorphisms**
>
> `copack<>` is the bottom grade `∅`. The carriers `just<T>` and
> `expected<T,copack<>>` are canonically isomorphic for corresponding payloads; `choice<Ts...>` is the
> analogous identity carrier for coproduct-shaped data. “Canonical” means the conversion has no
> choices and preserves the payload.
>
> A graded monad is a lax monoidal functor from a grading pomonoid. In one sentence each:
> `η` wraps a value at grade `∅`; `μ`/bind joins nested computations and unions their grades; effect
> approximation maps a computation along grade inclusion. Cross-carrier identity bind composes along
> the canonical isomorphisms at the bottom grade.

## 10. Elimination and multidispatch

Composition builds a precise shape; eventually a program must eliminate it. `fn::apply` extends
`std::apply` and the C++26 application trait family:

```cpp
fn::apply(f, args...);
fn::apply_r<R>(f, args...);

fn::apply_result_t<F, Args...>;
fn::is_applicable_v<F, Args...>;
fn::is_nothrow_applicable_v<F, Args...>;
```

The corresponding class templates and explicit-result `_r` traits describe the same operation.

The basic call shapes are:

```text
A                                  -> f(A)
pack<A,B>                          -> f(A,B)
pack<>                             -> f()
tuple<A,B>                         -> f(A,B)
copack<A,B> holding A              -> dispatch to f(A)
copack<pack<A,B>,C> holding pack   -> dispatch to f(A,B)
```

A lone standard tuple-like operand receives `std::apply` semantics. Pack elements are terminal: a
tuple stored as one pack field is passed whole rather than recursively unpacked. Arbitrary user wrappers
are opaque unless explicitly converted to a supported structured type.

Here is ordinary multidispatch:

```cpp
using Route = fn::copack_for<
    fn::pack<ById, Memory>,
    fn::pack<ById, Database>,
    fn::pack<ByName, Memory>,
    fn::pack<ByName, Database>>;

auto execute = fn::overload{
    [](ById, Memory) { return 1; },
    [](ById, Database) { return 2; },
    [](ByName, Memory) { return 3; },
    [](ByName, Database) { return 4; }
};

static_assert(fn::is_applicable_v<decltype(execute), Route>);
```

Remove one overload and application becomes invalid. Exhaustiveness is a constraint on the whole
operation, not a runtime default. `fn::overload` is merely the usual overload-set helper; ordinary
functions and custom callable objects work equally well.

`std::visit` receives the selected variant alternative as one object and distinguishes alternatives
positionally. `fn::apply` additionally spreads product-shaped alternatives into parameters and can fold
several operands into their distributed product. The final handler is still chosen by ordinary C++
overload resolution.

### Type-tagged elimination

Untagged application may deliberately forget the outer representation:

```text
A                 -> f(A)
pack<A>           -> f(A)
tuple<A>          -> f(A)
array<A,1>        -> f(A)
```

When origin matters, member `apply_type` supplies a constructor tag before the unpacked content:

```text
f(in_place_type_t<A>,       A)
f(in_place_type_t<pack<A>>, A)
```

For `optional` and `expected`, the state tags are `std::in_place`, `std::nullopt`, and `fn::unexpect`.
The tag never converts, so it prevents one permissive overload from silently handling a different state
or alternative. `apply_type_r<R>` provides the corresponding explicit-result form.

> **Mathematical note — elimination**
>
> A coproduct is eliminated by supplying a function for every injection; a product is eliminated by
> supplying its components. Untagged `apply` composes those eliminations and may forget which outer
> representation produced a parameter list. `apply_type` retains the coproduct injection by passing
> its exact type tag.

## 11. The verb map

The verbs are values describing pipeline steps. This table is a navigation map, not a replacement for
their API constraints:

| Channel | Role | Verb | Effect |
|---|---|---|---|
| value | map | `transform(f)` | Map a successful value; preserve the short-circuit side |
| value | sequence | `and_then(f)` | Continue with a monadic result; join its carrier/grade |
| value | guard | `filter(pred, error)` | Keep a value or create an error; optional uses only `pred` |
| value | inject failure | `fail(f)` | Turn a successful value into the carrier's short-circuit state |
| value | observe | `inspect(f)` | Observe success and pass the computation through |
| error | map | `transform_error(f)` | Change error alternatives; preserve success |
| error | sequence | `or_else(f)` | Continue with another carrier of the same value kind |
| error | handle | `recover(f)` | Turn an error/empty state into a successful value |
| error | observe | `inspect_error(f)` | Observe failure and pass the computation through |
| both | end with value | `value_or(args...)` | Extract the value or construct a fallback |
| both | end | `discard` | Terminate the monadic sequence and discard its carried state |

The constraints are part of the vocabulary. `and_then` requires a monadic return; `recover` requires a
value that can construct the success side; `filter` and `fail` require a live short-circuit side;
error-side verbs refuse carriers with no error side. These rejections prevent an operation name from
pretending to do work that the carrier cannot represent.

## 12. Laws as C++ equalities

The laws are design constraints, not runtime tests performed by the library. For regular equality-
comparable values and pure callbacks, representative forms are:

```cpp
// Functor identity
static_assert(fn::just{3}.transform([](int x) { return x; })
              == fn::just{3});

// Functor composition
constexpr auto f = [](int x) { return x + 1; };
constexpr auto g = [](int x) { return x * 2; };
static_assert(fn::just{3}.transform(f).transform(g)
              == fn::just{3}.transform([=](int x) { return g(f(x)); }));

// Monad left identity
constexpr auto k = [](int x) { return fn::just{x + 1}; };
static_assert((fn::just{3} | fn::and_then(k)) == k(3));

// Monad right identity
static_assert((fn::just{3} | fn::and_then(
                [](int x) { return fn::just{x}; }))
              == fn::just{3});
```

Associativity compares `m.and_then(f).and_then(g)` with
`m.and_then([&](auto x) { return f(x).and_then(g); })`. For graded expected, both sides derive the same
normalized union grade. For identity-cluster binds, the carrier spellings may differ at intermediate
points; the law is then interpreted through the canonical payload-preserving isomorphism.

Product composition is associative at the type level after canonical pack flattening. Coproduct union
is associative, commutative, and idempotent. Widening directly or through intermediate supersets has the
same selected alternative for regular payloads.

`constexpr static_assert` examples witness these laws for specific values and callbacks; they cannot
prove a law for every user-defined C++ type. A callback with side effects or a payload whose copy/move
operations change its logical value falls outside the regular, pure model. The library preserves C++
value categories and observable operations rather than pretending those effects do not exist.

## 13. C++ mechanics that preserve the algebra

The algebra is useful only if the C++ mechanics do not quietly undermine it.

### Constraints and exhaustiveness

Public concepts and requires-clauses ask whether a composition is valid before its body is instantiated.
Negative generic probes should use facilities such as `is_applicable_v`, `applicable_and_then`, or an
appropriate requires-expression. A missing dispatch arm, incompatible callback return, invalid
widening, or unrepresentable carrier result removes the operation from consideration or triggers a
focused type mandate.

### `constexpr` and structural values

The vocabulary and operations are designed for constant evaluation. `pack`, `copack`, and `just`
preserve triviality and structural-type properties whenever their elements permit it; a structural
value can serve as a non-type template argument. Compile-time evaluation therefore checks both values
and error alternatives:

```cpp
constexpr fn::expected<int, fn::copack<NotANumber>> answer{42};
static_assert(answer.value() == 42);
```

### Computed `noexcept`

`noexcept` is derived from the operations actually performed: callback invocation, payload
construction, widening, and relocation. The member and piped spellings agree. A throwing callback is
not accidentally promoted to an unconditionally `noexcept` pipeline, and an unreachable `copack<>`
arm contributes no fictitious exception.

### Value categories and immovable payloads

Ref-qualified members propagate lvalue, const, and rvalue access to callbacks. Direct construction from
an invocation result avoids unnecessary intermediate moves, allowing operations to work with immovable
values where C++ can construct the destination in place. Conversely, an lvalue pipeline cannot
silently move a move-only payload; the relevant operation is constrained away.

`append` and differently associated `operator&` expressions may perform different legal moves even
when their final algebraic type and regular value are the same. The library fixes the type laws while
preserving the programmer's control over C++ object operations.

### `pfn` and `fn`

`pfn` is the standards-facing layer: it polyfills the specified standard vocabulary, including the
C++26 `apply` trait family. `fn` builds on it. On standard shapes, `fn` preserves the corresponding
standard meaning; it then adds pack/copack algebra, multiple-operand composition, graded errors,
multidispatch, pipeline verbs, `choice`, and `just`.

This separation is useful when reading diagnostics. A standard-shaped operation bottoms out in `pfn`
or the standard invocation rules. A libfn-shaped operation first composes or eliminates the additional
type structure, then reaches those same rules.

## A compact model

The design can be remembered as these rules:

1. `pack` is a flat ordered product; `pack<>` is its nullary unit.
2. `copack_for` is a normalized set of exact alternatives; `copack<>` is uninhabited.
3. `transform` maps one channel without joining a returned computation.
4. `and_then` sequences computations and joins the callback's result.
5. Copack-graded expected unions error sets and widens along set inclusion.
6. `operator&` composes successful values into products and distributes products over alternatives.
7. `choice` is identity over a coproduct; its bind joins branch choices into their superset.
8. `just`, `choice`, and `expected<T,copack<>>` form the identity cluster where their payloads
   correspond.
9. `apply` eliminates the selected structure through exhaustive ordinary C++ overload resolution;
   `apply_type` retains exact origin tags.

## Functional terminology

| libfn/C++ | Common functional-programming term |
|---|---|
| `transform(f)` | `map f` or `fmap f` |
| `and_then(f)` | bind |
| the callable passed to `and_then` | Kleisli arrow |
| wrapping a value in a carrier | `pure` or `return` |
| `pack` | product |
| `copack` | coproduct or sum |
| `operator&` | product composition |
| `copack_for<E...,F...>` | union of effect grades |
| widening to a larger copack | subeffecting or effect approximation |

## Further reading

The programming model of effect sets, union, subeffecting, and graded bind follows Dominic Orchard and
Tomas Petricek,
[“Embedding effect systems in Haskell”](https://www.doc.ic.ac.uk/~dorchard/publ/haskell14-effects.pdf),
especially Sections 1–3.

For the categorical definition of a graded monad as a lax monoidal functor, and its relationship to
parameterised monads, see Dominic Orchard, Philip Wadler, and Harley Eades III,
[“Unifying graded and parameterised monads”](https://arxiv.org/pdf/2001.10274), especially Definition
21.

The family `E -> copack<E...>` together with widening resembles a graded object: each exact copack is
one component, while inclusions carry structure between components. Relevant definitions appear in
Dylan McDermott and Tarmo Uustalu,
[“Flexibly Graded Monads and Graded Algebras”](https://dylanm.org/flexibly-graded-monads.pdf),
Definitions 1, 2, and 9. This is validation vocabulary; libfn does not claim to instantiate that
paper's flexibly graded construction.

C++26 type ordering is proposed in
[P2830, “Type and variable template argument ordering”](https://wg21.link/P2830). Its motivation cites
libfn's need to canonicalize sets of types.
