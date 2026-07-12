# Changelog

Design history of libfn, newest first. The living documents — [README.md](README.md), [CONTRIBUTING.md](CONTRIBUTING.md), [docs/](docs/) — describe only the present state of the design; when a decision makes an earlier idea obsolete, this file is where the transition is recorded and explained.

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
