# Changelog

Design history of libfn, newest first. The living documents — [README.md](README.md), [CONTRIBUTING.md](CONTRIBUTING.md), [docs/](docs/) — describe only the present state of the design; when a decision makes an earlier idea obsolete, this file is where the transition is recorded and explained.

## `optional<T&>` refuses pack and copack referents — 3 October 2026

`fn::optional<T&>` no longer accepts a `pack` or `copack` referent, matching `just<T&>`. `fn::optional<copack<Ts...>&>` dispatched on its referent's active alternative, and `fn::optional<pack<Ts...>&>` expanded its referent's fields. How a reference to a product or a sum takes part in conjunction, disjunction and grading is an open question, so both are refused until the type algebra answers it; issue #434 discusses the copack case. `pfn::optional<T&>` does not reject them, but `fn` and `pfn` types are not meant to be used together.

## `<fn/monadic.hpp>` becomes `<fn/traits.hpp>`; `monadic_invocable` rejects an incomplete functor — 27 September 2026

Replace includes of `<fn/monadic.hpp>` with `<fn/traits.hpp>`; no compatibility header is provided. `some_in_place_type` moves to `<fn/traits.hpp>` and remains available through `<fn/copack.hpp>`.

`monadic_invocable` requires a complete functor type and a complete nested `apply`. Querying a forward-declared library or user-defined functor, or one whose `apply` is only declared, now fails to compile. Previously, its answer could change after the definition, making the program ill-formed with no diagnostic required. Include the functor's defining header before querying.

## `apply` traits reject an incomplete `pack` or `copack` operand — 27 September 2026

`apply`, `apply_r` and their traits reject incomplete `pack` or `copack` operands. Previously, trait queries could cache `false` (or `void` for `apply_result`) even after the type definition became available. Include `<fn/pack.hpp>` or `<fn/copack.hpp>` before querying. `typelist_applicable` still works from template arguments alone.

## `pack` and `copack` operations move to `<fn/algebra.hpp>` — 27 September 2026

`operator&` over `pack` and `copack`, `conjoin`, and `disjoin` move to `<fn/algebra.hpp>`. Include this header when using these operations without a carrier header. The `expected`, `optional`, and `just` headers include it.

`<fn/copack.hpp>` includes `<fn/pack.hpp>`; the reverse dependency is removed. A copack transform returning `void` therefore needs only `<fn/copack.hpp>`. Code using `copack` through `<fn/pack.hpp>` must include `<fn/copack.hpp>` explicitly.

## `&` takes a `void` side as the unit factor `pack<>` — 27 September 2026

`&` treats a `void` value as the unit factor `pack<>`. For example, `expected<int, E> & just<void>` yields `expected<pack<int>, E>`; previously it yielded `expected<int, E>`. This breaking change applies across carrier pairings in either order. Copack values distribute into packs, and `optional<T&> & just<void>` yields an owning `optional<pack<T>>`. Two `void` values still yield `void`.

## `pack::append` builds its result in place — 27 September 2026

`pack::append` constructs the final pack directly. Initializing its base from a temporary could otherwise relocate elements again and terminate if a move threw inside a `noexcept` call. Bracing each aggregate layer also prevents an element's conversion operator from initializing a whole layer in place of the element.

## `transform` over a copack maps a `void` result to `pack<>` — 27 September 2026

`transform` over a copack represents `void` callback results as `pack<>`. Mixed `void` and `int` results yield `copack_for<pack<>, int>`; all-`void` results yield `copack<pack<>>`. This applies through `expected`, `optional` and `choice`, and to `transform_error` over a copack error. Plain error types still reject `void` results. Mutable operands can select a `void` overload where they previously fell back to a valued `const` overload.

Results are converted before spliced arguments expire, fixing a dangling reference. The `noexcept` specification includes the selected overload's explicit result conversion. Rvalues spliced before a `pack` argument are stored by value, fixing a compilation failure.

`expected<void, E>::copack_value()` lifts to `expected<copack<pack<>>, E>`.

## `and_then` joins `void` and valued branches through `pack<>` — 26 September 2026

`and_then` accepts branches returning both `void`-valued and valued `expected` results. For example, `A -> expected<void, E>` and `B -> expected<int, E>` join as `expected<copack_for<pack<>, int>, E>`; this previously failed to compile. An all-`void` join stays `void`.

## `void` recovery joins a value grade as `pack<>` — 26 September 2026

`or_else` accepts a `void`-valued recovery into a copack value side, representing success as `pack<>`. For example, recovering `expected<copack<V>, E>` through `expected<void, G>` yields `expected<copack_for<V, pack<>>, G>`; this previously failed to compile. This also applies to branches over a copack error and pipeline recovery from `optional<copack<V>>`. An all-`void` join stays `void`.

`void` lifts to `copack<pack<>>`, including in `same_value_kind` and pipeline recovery into `optional`. A recovery returned by reference from `optional` has its copy included in the `noexcept` specification.

## `just` holds lvalue references — 24 September 2026

`just<T&>` now supports lvalue-reference payloads (issue #417), providing an always-engaged counterpart to `optional<T&>`. Referents must be object types other than arrays, in-place type tags, packs, or copacks. Rvalue-reference payloads remain unsupported, as in `optional` and `pack`.

`just<T&>` follows `optional<T&>`: copy assignment and `emplace` rebind, comparisons compare the referents, and `value_type` is `T`. `value()` returns `T&` regardless of the carrier's value category or constness. Callable operations use that same reference, expanding tuple-like referents as `fn::apply` does. `apply_type` tags the payload with `std::in_place_type<T&>`.

There is no default constructor. For an `int x`, `just{x}` still deduces `just<int>`; the explicit type tag in `just(std::in_place_type<int&>, x)` instead deduces `just<int&>`. As with the library's `optional<T&>`, binding to a temporary is not yet rejected and can leave a dangling reference.

Composition follows these rules:

- A `transform` callback returning a supported lvalue reference `U&` produces `just<U&>`, referring to the returned object. An rvalue owning `just` rejects such a result: it may refer into the payload, which expires with the carrier. `optional<T>` accepts it, as the standard specifies; `just` deliberately diverges.
- Member and pipeline `and_then` accept callbacks returning `just<U&>`. When every branch of a choice returns the same `just<U&>` type, the result retains that type. A join of different result types is rejected if any is a reference `just`, because references cannot be alternatives of the resulting choice. This matches the restriction on reference payloads in optional joins.
- The products and sums that `&` and `|` build hold a copy of the referent, as they do for `optional<T&>`. This includes a product with the unit `just<void>`: `just<void>{} & just<T&>{x}` is `just<pack<T>>`.
- Copack references remain unsupported. `just<copack<Ts...>&>` would dispatch on the referent's active alternative, introducing control flow based on state the carrier does not own. Issue #434 discusses the tradeoffs and open questions. `transform` also rejects copack-reference results.

## Singular copacks support the tuple protocol — 24 September 2026

A `copack` with exactly one alternative supports the tuple protocol: `std::tuple_size_v<copack<T>>` is 1, `std::tuple_element_t<0, copack<T>>` is `T`, and `get<0>` returns the same reference as the index-less `get`. This supports structured bindings and generic code that uses tuple traits with ADL `get`. A structured binding over a singular copack, including a singular choice's `value()`, now binds its alternative. Previously, it bound the public `data` and `index` members.

The protocol is limited to singular copacks. A wider copack selects among different alternative types at run time, so there is no single element type to expose. `copack<>` remains outside the protocol: it has no values, whereas the empty product `pack<>` represents the unit.

For `copack<pack<A, B>>`, the tuple element is the entire `pack`. `fn::apply` still dispatches the copack and passes the pack's fields as separate arguments; `pfn::apply` does not accept copacks.

Both `get` overloads exclude volatile copacks through their constraints. Previously, probing the index-less `get` with a volatile copack could produce an error in its body.

## `choice` becomes `just` over a copack — 21 September 2026

`choice<Ts...>` is now an alias of `just<copack<Ts...>>`. The specialization forwards operations on alternatives to its `copack` payload. The `<fn/choice.hpp>` header is gone; `<fn/just.hpp>` declares `choice`, `choice_for` and `some_choice`.

Version `0.1.0` rejected copack payloads in `just` to avoid duplicating `choice`. The alias now gives both spellings the same type. A `transform` callback returning a nonempty copack produces a choice through the member operation as well as the pipeline operation. The separate pipeline promotion overloads and their `applicable_transform_promote` concept are removed.

Deduction guides on `just` restore value and in-place deduction for the `choice` alias: `choice{42}` deduces `choice<int>`, as does `choice{std::in_place_type<int>, 42}`. `as_choice` lifts a value into a single alternative or wraps a copack as the choice over its alternatives.

The consequences are breaking:

- `some_just` admits a choice, `some_identity` is `some_just` or the identity `expected`, and `same_kind` holds between any two `just`s. Generic code that read `some_just` as a single-branch payload must test the payload.
- `choice::value()` returns the `copack` payload, no longer `*this` as its base, and a choice does not convert to a copack. The copack's alternative-wise surface — construction, assignment, `emplace`, `has_value`, `get_ptr` and the `apply` family — is forwarded instead, and a narrower choice widens into a wider one by construction and assignment.
- The choice comparisons are `just`'s, over the payloads; their `noexcept` follows the alternatives' comparisons instead of being promised unconditionally.
- Member `and_then` accepts `just` results, including choices and `just<void>`. Choice branches may return different `just` types: their payloads form a normalized choice, with returned choices contributing their alternatives and `just<void>` contributing `pack<>`. If the result types agree after cv/ref removal, the result keeps that `just` type. Pipeline `fn::and_then` uses the same payload join when an identity `expected` with a copack payload binds to `just` results.
- For `just<T>` with a non-copack payload, pipeline `transform` rejects callbacks returning a copack reference. The removed promotion overloads copied such results into an owning choice. A choice's per-alternative `transform` still normalizes copack reference results into an owning choice.
- Diagnostics spell `just<copack<...>>`.
- Version `0.2.0-dev` opens the breaking release cycle, with inline ABI namespace `v0_2_dev` (`v0_2_dev_cxx26` in C++26 mode).

## Visual Studio 2022 returns as the MSVC floor — 13 September 2026

Bazel's [minimal version selection](https://bazel.build/external/module#version-selection) can select a later `libfn` release when another dependency requests it, even if the consumer still requests `0.1.0`. Keeping Visual Studio 2022 support avoids requiring those consumers to upgrade their compiler for this reason. Microsoft still [supports Visual Studio 2022](https://learn.microsoft.com/en-us/lifecycle/products/visual-studio-2022).

The build workflow restores C++20 Debug and Release builds and tests on `windows-2022`, alongside the Visual Studio 2026 jobs. **Visual Studio 2022 is again the minimum supported Visual Studio version**, superseding the MSVC change in the entry below. The GCC, Clang and Apple Clang requirements are unchanged.

## Compiler support narrows — 23 August 2026

A supported compiler is one whose upstream still takes fixes: LLVM maintains only its latest major release, and Apple cannot be expected to backport fixes for this library's template-metaprogramming demands into older Xcode toolchains. The floors therefore move to what CI actually proves, obsoleting the `0.1.0` documentation:

- **clang 19 is the minimum supported clang** (was 16); the Linux lanes for clang 16 to 18 retire.
- **Apple Clang 21.0 replaces 16.0**; the macOS 15 CI lanes retire, leaving macOS 26 with Apple Clang, brew clang 21 and gcc 15.
- **Visual Studio 2026 replaces 2022** as the MSVC floor.
- **The clang 15–18 storage-poison workaround is removed** from `fn::optional` and `fn::expected`: the affected paths return their result prvalue directly again, no longer forcing a move around the miscompiled copy-elision.
- **A preprocessor guard refuses clang older than 19** (in `libfn_version.hpp`, included on every entry path, the single header too): the storage-poison miscompile is silent — corrupted values at `-O1` and above, no diagnostic — so affected compilers now fail loudly at inclusion instead of emitting wrong code.

Users on a dropped toolchain can stay on the `0.1.0` release.

## libfn 0.1.0 — 23 August 2026

The first tagged release, described in full by the `0.1.0-rc1` entry below. Changes from `0.1.0-rc1`:

- **The documentation site is versioned**: every release keeps an immutable copy of its docs and single header under `/v<x.y.z>/` ([all versions](https://libfn.org/versions.html)).
- **The single-header release asset carries signed build provenance**, and the README states each distribution channel's contract.
- **The release procedure and version cadence are documented** in CONTRIBUTING; immediately after each tag, `main` opens the next `-dev` cycle under a renamed ABI namespace.
- **Draft pull requests run only the cheap correctness gates**; the full CI matrix returns on ready-for-review.

## libfn 0.1.0-rc1 — 18 August 2026

libfn is a header-only C++20 functional-programming library: `fn`'s monadic composition and types, layered over `pfn`'s C++23/26 vocabulary-type polyfills. The `0.1.0` tag is the first release, opening the versioning contract that SemVer's bare `0.y.z` otherwise leaves informal: a `y` bump is a breaking change (API and/or ABI), a `z` bump stays compatible — and, being header-only, a binary links against exactly one libfn version.

A first release has no prior version to diff against, so this entry presents what the library offers at `0.1.0`.

### The monadic vocabulary

- **Value/error channel pairs**: `and_then`/`or_else`, `transform`/`transform_error`, `inspect`/`inspect_error`.
- **`fail`, `recover` and `filter` round out the set**: force an error, clear one, or test the value.
- **`just`, `choice` and an error-unit `expected`** form one identity cluster around a shared bind.
- **`and_then` and `or_else` join heterogeneous** `expected`/`optional` branches into their lossless superset.
- **The same two verbs also convert directly** between `expected` and `optional` carriers.
- **`value_or` extracts the value or a fallback**; `discard` drops a result kept only for its side effects.
- **`copack_value`, `copack_error`, `as_copack` and `as_pack`** lift plain data into the monadic types.

### The type algebra: pack and copack

- **`pack` is the product**: a move-friendly, tuple-like holder for a fixed set of values.
- **`pack` carries multiple values** through `expected`, `optional`, `choice` and every monadic operation.
- **`pack` supports structured bindings and `std::get`** via the tuple protocol; a nested `pack` element flattens.
- **`pack` compares element-wise**: `==` and lexicographic `<=>`, each computed from the elements' own operators.
- **`copack` is the co-product**: a tagged union; `choice` wraps it with `and_then`, `transform` and `inspect`.
- **`copack<>` is the empty co-product**: uninhabited, the identity grade every error union builds from.
- **The error channel is graded**: sequencing derives each pipeline's exact `copack` of failure modes.
- **`choice::and_then` joins branches** of differing `choice` types into their combined superset.
- **`copack` and `choice` construct an alternative in place**, and assign from a value matching exactly one.
- **`emplace` reconstructs a copack/choice alternative** in place, when assignment itself refuses.

### Multidispatch: apply

- **`apply`/`apply_r` dispatch one callable** across `pack`, `copack`, `choice`, `optional` and `expected` uniformly.
- **`apply_type` and `apply_type_r` add exhaustive**, type-indexed dispatch across all four carriers.
- **Every `apply`-family member takes trailing arguments**, appended after each arm's unpacked content.
- **`fn::overload` builds an ad hoc visitor** by combining several callables into one.
- **`pfn` polyfills the C++26 apply trait family**: `is_applicable`, `is_nothrow_applicable`, `apply_result`.

### Composing pipelines: operator& and operator|

- **`operator&` conjoins two carriers**: values become a `pack`; the leftmost failure supplies the error.
- **`operator|` disjoins two carriers**: the leftmost success wins; both failing keeps every error in a `pack`.
- **`conjoin` and `disjoin` are the n-ary folds** of `&` and `|`.
- **`conjoin` also folds plain data** into a `pack`; a call mixing data and carriers matches nothing.
- **Both operators admit the identity cluster** and compose two ungraded error types directly.

### Standard-library polyfills: pfn

- **`pfn::expected` is a spec-faithful C++20 polyfill** of `std::expected`, plus `has_error()`.
- **`pfn::optional` is a full C++26-shaped polyfill** on C++20: `optional<T&>`, plus a range interface.
- **`fn::expected` and `fn::optional` build on the pfn versions**; `fn` is a strict superset of `pfn`.
- **`unexpected`, `unexpect`, `unexpect_t` and `bad_expected_access`** are available directly in namespace `fn`.
- **`expected` supports move-only value and error types**, in construction, assignment and comparison.

### Correctness guarantees

- **Every monadic operation is concept-constrained, constexpr**, with a computed `noexcept`.
- **The `|` and `&` pipelines carry a verb's computed `noexcept`** through the whole expression.
- **A monadic result is `[[nodiscard]]`**; `discard` spells the deliberate drop.
- **`copack`, `choice`, `optional` and `expected` assign** with the strong exception guarantee, never valueless.
- **Copy/move assignment is trivial** exactly when every held alternative's own assignment is.
- **A constraint asks the question its body performs**: every rejection is substitution-visible.

### Type ordering & ABI stability

- **`fn` and `pfn` live in an ABI-versioned namespace**; a mismatched version fails to link, never collides.
- **Type ordering is total by construction**; a sort-key collision is a compile error.
- **`pfn` keeps one ABI namespace across language modes**; only `fn`'s `copack`/`choice` layout varies by mode.

### Standards, compilers & packaging

- **Targets C++20 as the sole baseline** across every `fn`/`pfn` header; no later standard is required.
- **Supported compilers**: gcc 12+, clang 16+, Apple Clang 16+, and MSVC 2022+.
- **An opt-in `LIBFN_CXX26` mode** switches type ordering onto the standard `std::type_order` (gcc 16+).
- **`libfn::fn_cxx26` selects the mode with one link line**, carrying its define and its C++26 language requirement.
- **Installs as plain header-only CMake**, or as a tested Conan, vcpkg, Nix flake or Bazel module package.
- **Public headers live under** `include/fn` and `include/pfn`.

### Documentation & examples

- **`TYPE_ALGEBRA.md` works the library's type algebra** from first principles.
- **The API reference is generated from Doxygen comments**; the docs site also carries usage guides.
- **Worked examples** — a compiled RPN calculator and a polygon library — show the pipeline end to end.
- **The README's own worked example** is a compiled, tested source file, kept in sync with the prose.

### Project history

- **Project inception**: a handful of direct commits set up the repository, before the pull-request history begins.

### Previous changelog

- The dated entries this summary replaces remain readable at [324f335](https://github.com/libfn/functional/blob/324f3358165be1187fda978e83ba17d3263f25ae/CHANGELOG.md).
