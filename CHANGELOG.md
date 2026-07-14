# Changelog

Design history of libfn, newest first. The living documents — [README.md](README.md), [CONTRIBUTING.md](CONTRIBUTING.md), [docs/](docs/) — describe only the present state of the design; when a decision makes an earlier idea obsolete, this file is where the transition is recorded and explained.

## `sum` and `choice` assign across widening — 14 July 2026

- **A narrower `sum` now assigns into a wider one on the widening constructors' terms** ([#310](https://github.com/libfn/functional/issues/310)): `operator=` is constrained on the alternatives the source can actually deliver, and the incoming alternative is assigned in place when it is the one held, or replaces it by construction otherwise — the per-alternative decision same-type assignment already makes. Previously `wide = narrow` existed only by routing through the widening constructor: a whole temporary sum, a copy plus a move where one copy suffices, and viability gated on the destination's every alternative — an uninvolved alternative with no safe replacement arm forbade assignments it took no part in.
- **`choice` declares its own widening assignment and delegates to `sum`'s.** A declared `operator=` hides every base overload, so without its own pair `choice` would have been left out silently; the delegation also admits a `sum` over the same alternatives, which previously paid for a temporary despite having nothing to widen.

## `sum` is as trivial as its alternatives permit — 14 July 2026

- **Every special member of `sum` (and through it `choice`) is now trivial exactly when every alternative permits it** — the gates `std::variant` uses ([#309](https://github.com/libfn/functional/issues/309)). A `sum` of fundamentals, or of trivially copyable `pack`s, is trivially copyable — register-passed and `memcpy`-safe — where previously no `sum` supported any trivial operation. The sum-of-packs case is the one that pays: joining sum-valued monads yields the cartesian product, and that is what a multidispatch pipeline copies at every stage. Trivially copyable sums change ABI (register passing), which is why this lands before the first tagged release.
- **`variadic_union` carries the propagation.** Its copy, move and assignment exist only as constrained defaults: the machinery above constructs through a tagged constructor, reads through `ptr_variadic_union` and destroys per member, so the non-trivial cases need none of them. The tagged constructor replaces aggregate designated-initializer construction — a union with declared constructors is not an aggregate — with `make_variadic_union` keeping its interface and its brace-initialization semantics.

## `sum` assignment requires assignable alternatives — 14 July 2026

- **`sum::operator=` (and through it `choice`'s) now requires every alternative to be assignable, and assigns the held alternative in place whenever the incoming one is the same type; only a change of alternative reconstructs.** As introduced the day before ([#304](https://github.com/libfn/functional/pull/304)), assignment always destroyed the alternative in hand and constructed the incoming one over it, deliberately asking nothing of the alternatives' own `operator=` — which proved unsound: destroy-and-reconstruct synthesizes an operation the alternative may have refused. `fn::pack<int, int&>` deletes its assignment because C++ cannot rebind a reference, and `optional` and `expected` of that pack honour the refusal — yet a `sum` of it was assignable, quietly rebinding the reference ([#311](https://github.com/libfn/functional/issues/311)). A sum of packs is the normal form of this library's type algebra, so this sat in the middle of the library, not at an edge.
- **The strong exception guarantee stays, on both paths, and there is still no valueless state.** An unchanged alternative is assigned directly where its assignment cannot throw, through a temporary where its move assignment cannot, and under a nothrow snapshot with rollback otherwise; a replacement still builds a throwing copy into a temporary before the alternative in hand is destroyed.
- **Assignment as destructive replacement — other languages' default — remains a possible future direction, deliberately not taken.** In C++ the type system lets a type refuse assignment outright, and that refusal can be load-bearing; an operation the language may one day define (destructive relocation) is not one a container should synthesize meanwhile.

## Honest `noexcept`, honest constraints — 13 July 2026

The release-0.1 bug backlog landed as thirteen fixes ([#295](https://github.com/libfn/functional/pull/295)–[#307](https://github.com/libfn/functional/pull/307)), closing nineteen issues. The common thread: what the library declares — `noexcept`, constraints, concept answers — is now derived from what its bodies do.

- **Every `noexcept` specification is computed, none assumed** ([#298](https://github.com/libfn/functional/pull/298), [#300](https://github.com/libfn/functional/pull/300), [#301](https://github.com/libfn/functional/pull/301), [#302](https://github.com/libfn/functional/pull/302)). The internal nothrow-invocability traits were hardcoded `true`; they are now real, and on that basis the `sum`/`pack`/`choice` constructors and dispatch, the verbs, `operator|` and `operator&` derive their specifications from the layers they delegate to. `v | fn::and_then(f)` was unconditionally `noexcept` where `v.and_then(f)` told the truth — the pipeline spelling turned a throwing callback into `std::terminate`; a join whose error is `sum<>` now promises the "cannot fail" its type spells; and `sum_value`/`sum_error` gained specifications and `constexpr`, completing constant-evaluated pipelines across the graded lift.
- **Constraints ask the question the body performs** ([#296](https://github.com/libfn/functional/pull/296), [#297](https://github.com/libfn/functional/pull/297), [#303](https://github.com/libfn/functional/pull/303), [#305](https://github.com/libfn/functional/pull/305)). The storage brace-initializes, and `is_constructible_v` spells parenthesized initialization — the two disagree on aggregates and narrowing — so construction constraints ask `T{args...}`, and ask it of the function that stores (`make_variadic_union`) rather than restating it in a trait free to drift. Sum-case callbacks are constrained in the immediate context, so a category-partial visitor sheds its non-viable overloads instead of poisoning the call; and the relocating verbs (`value_or`, `recover`, `fail`, `filter`) ask whether the side they carry unchanged can be carried in the value category it arrives in.
- **`sum` and `choice` became assignable — strong exception guarantee, no valueless state** ([#304](https://github.com/libfn/functional/pull/304), [#183](https://github.com/libfn/functional/issues/183)); the entry "`sum` assignment requires assignable alternatives" above reverses its destroy-and-reconstruct form in favour of the alternatives' own `operator=`.
- **Edges answer instead of erroring** ([#299](https://github.com/libfn/functional/pull/299), [#306](https://github.com/libfn/functional/pull/306), [#307](https://github.com/libfn/functional/pull/307)). `convertible_to_unexpected`, `convertible_to_optional` and `convertible_to_choice` answer `false` for `void`; a callback whose result no `sum` can hold drops its caller from the overload set; and `sum`'s hand-written `operator!=` is gone — C++20 rewriting synthesizes it from `==`, so their constraints cannot drift apart.

## Codecov reads coverage from the `gcovr` aggregate — 12 July 2026

- **Codecov ingests an aggregated cobertura report rather than the raw `.gcov` text.** `gcov` reports template code once per instantiation, and codecov's parser counted the unexecuted instantiation records as missed lines: it reported ~98.9% where the aggregate over the same `.gcda` reports every line of `include/` covered. For a library which is almost entirely templates that is not a rounding error, and it is not fixable from this side — reported upstream in April 2024 and still unanswered: [codecov/feedback#334](https://github.com/codecov/feedback/issues/334). The aggregating is `--merge-lines`: since gcovr 8 a line record is kept per instantiation, which would have carried the same defect into the new report.
- **SonarCloud keeps reading the raw `.gcov` files.** Given the same cobertura report it counts 267 covered lines as missed, because its own parse of the sources marks as executable many lines for which no runtime code is ever emitted — template and constant-evaluated code that gcov cannot report on. So the two tools read different inputs, and the `.gcov` text is generated for sonarcloud alone. What an accurate SonarCloud report would need is not yet understood.
- **Branch coverage no longer counts the exception paths.** The aggregate excludes throw and unreachable branches, so the condition count reflects decisions written in the source rather than artefacts of the exception model. `parsers.cobertura.partials_as_hits` states for the parser in use the same lines-not-branches policy that the retired `parsers.gcov.branch_detection` block encoded.

## `expected`'s associated types joined namespace `fn` — 10 July 2026

- **`fn` re-exports `unexpected`, `unexpect`, `unexpect_t` and `bad_expected_access` from `pfn`.** Nothing is added — these are using-declarations, so both spellings name the same types — but the `<expected>` vocabulary is complete under one namespace, and constructing the error state in otherwise pure-`fn` code no longer reaches into `pfn` (previously every example returned `pfn::unexpected`). No `optional` counterparts: `optional`'s associated types are already in C++20's `std`, so no polyfills and nothing to bring from `pfn` to `fn` namespace.

## `fn::pack` gained the tuple protocol — 9 July 2026

- **`fn::pack` now models the tuple protocol** — `std::tuple_size`, `std::tuple_element`, and an ADL-found `get<I>`, so a pack works with structured bindings and the generic `using std::get; get<I>(p)` idiom.
- **A const pack propagates const onto its reference elements.** `get<I>` carries the pack's const through to the element, so `get<0>` on a `const pack<T&>` yields `T const&`. This diverges from `std::tuple<T&>`, whose reference members ignore container const — a deliberate difference: it lets a const pack hand out read-only views of referenced data, which a caller passing a pack of references by const reference generally wants. `std::tuple_element<I, pack const>` is specialized to match `get` rather than defer to the generic `tuple_element<I, const T>` wrapper.

## `operator&` composes the `sum<>` unit error — 8 July 2026

- **A non-void `fn::expected` carrying the `sum<>` unit error now composes under `operator&`.** A never-erroring operand — `expected<T, sum<>>`, the "cannot fail" grade — folds into a fallible `&`-chain, adding its value to the pack and no alternative to the widened error. The different-error overload had been ill-formed here: its error lambda deduced `void` for the `sum<>` side, poisoning the pack join's return-type deduction. The README example follows the fix, collapsing from a two-stage pipeline into a single `and_then` over the cartesian `(a & op & b)` dispatch table, its operator honestly typed `expected<sum_for<Add, Mul>, sum<>>`.

## Documentation realigned — 7 July 2026

- **README.md caught up with the two-library reality.** Its founding description — `fn` extends the `std` vocabulary types via inheritance and requires a C++23 standard library (gcc 13 / clang 18) — predated `pfn` (September 2025) and the C++20 rebase (June 2026). Replaced by the present state: `fn` builds on `pfn`, everything is C++20, minimums gcc 12 / clang 16. A "Using the library" section names the supported packaging routes.
- **The private-fork recommendation retired.** README advised consuming libfn via a private fork, a hedge against pre-standardization refactoring; packaging, the versioning contract and the approaching first tagged release replaced it with pinned-revision consumption.
- **This file introduced.** Living documents stay in timeless present tense; design transitions are recorded here, dated, in the same change that makes them.

## Design update — 6 July 2026

- **`fn::optional` was rebased onto `pfn`'s implementation, and the superset principle became load-bearing** (#253): every `fn` type with a `pfn` counterpart is a strict superset of it — switching a valid `pfn` program to `fn` changes neither compilation nor behaviour, `noexcept` included; where the two shapes conflict, `pfn` wins. Enforced by `tests/fn/expected_polyfill.cpp` and `tests/fn/optional_polyfill.cpp`, which run the whole `pfn` suite against the `fn` types. Obsoletes `fn::optional`'s standalone implementation.
- **Range support (P3168) landed in both `pfn::optional` and `fn::optional`** (#257). Each library mints its own minimal iterator — a pointer wrapper exposing only what the standard mandates, a Hyrum's-law defence — mirrored per library rather than shared, because the iterator concepts demand exact-type signatures and no `pfn` type may be reachable through `fn`'s interface.
- **The in-place constructors of `sum` and `choice` became `explicit`** (#256).

## `optional` polyfill — 3 July 2026

- **`pfn::optional` implemented** (#176): a C++20 polyfill of `std::optional` as specified for C++26, including `optional<T&>`.

## C++20 became the sole export surface — 29 June 2026

- **`fn` moved off the C++23 standard library and onto `pfn`** (#202): `fn::expected` derives from `pfn`'s implementation instead of `std::expected` (`fn::optional` followed on 6 July). Obsoletes the founding design — extending `std` types via inheritance — and with it the C++23 standard-library requirement and the gcc 13 / clang 18 floor; the minimum toolchain became gcc 12 / clang 16.
- **MSVC became a supported compiler** (Visual Studio 2022 and 2026), joining gcc, clang and Apple Clang.
- **C++23 became an internal validation tier**: the test-only CMake option `VALIDATE_CXX23` additionally builds the tests and examples as C++23 on compilers with solid C++23 support (gcc 15+, clang 21+; rejected with MSVC). Obsoletes the packaging-facing `DISABLE_CXX23` knob from 30 May — packaging carries no standard-mode knob at all.

## Fork-friendly analysis workflows — 23 June 2026

- **codecov and SonarCloud runs work from forks** (#240): a producer → artifact → consumer split keeps secrets away from PR-triggered code while forks report against their own accounts. Obsoletes upstream-only scanning.

## Packaging and the versioning contract — 30 May 2026

- **`VERSION` became the single source of truth** (#212): mirrored into `ports/libfn/vcpkg.json` and `MODULE.bazel` by a pre-commit hook. The release contract: versions are `0.y.z`, a bump in `y` is breaking, a bump in `z` is a compatible fix or addition — but inline definitions may change, so one libfn version per binary.
- **Packaging landed** (#212, refined in #218): conan recipe (components `fn` and `pfn`), in-repo vcpkg port (`ports/libfn`) and Bazel module, joining the Nix flake (2024) — all exercised by consumer-side CI. CMake exports the `libfn::fn` and `libfn::pfn` targets.

## `pfn` — 21 September 2025

- **`pfn` was born** (#155): a C++20 polyfill of `<expected>`, a second library beside `fn`. The start of the permanent two-library split: `pfn` polyfills what the committee has already standardized — specification fidelity is its whole point — while `fn` carries the novel extensions. At this point `fn` still extended `std::expected`/`std::optional` directly and required C++23; the rebase onto `pfn` came in June 2026.
