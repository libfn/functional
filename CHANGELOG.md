# Changelog

Design history of libfn, newest first. The living documents — [README.md](README.md), [CONTRIBUTING.md](CONTRIBUTING.md), [docs/](docs/) — describe only the present state of the design; when a decision makes an earlier idea obsolete, this file is where the transition is recorded and explained.

## libfn 0.1.0: the first tagged release — 2 August 2026

libfn is a header-only C++20 functional-programming library: `fn`'s monadic composition and types, layered over `pfn`'s C++23/26 vocabulary-type polyfills. The `0.1.0` tag is the first release, opening the versioning contract SemVer's bare `0.y.z` otherwise leaves informal: a `y` bump is a breaking change (API and/or ABI), a `z` bump stays compatible — and, being header-only, a binary links against exactly one libfn version.

A first release has no prior version to diff against, so this entry presents what the library offers at `0.1.0`. The dated design history it replaces stays readable at commit [`41ac614`](https://github.com/libfn/functional/blob/41ac614/CHANGELOG.md), the last before the first release candidate.

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
