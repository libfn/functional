# Contributing to libfn

This is for working *on* libfn; to *use* the library, see the [README](README.md).

## Development environment

Building and testing libfn needs a C++20 toolchain (with one exception). `pfn` polyfills C++23/26 standard-library utilities (`pfn::expected`, `pfn::optional`, `pfn::invoke_r`, `pfn::unreachable`); `fn` builds on `pfn` rather than the newer standard library. The minimum supported compilers are [gcc 12][gcc-standard-support] and [clang 16][clang-standard-support]; if your OS does not ship one recent enough, use the [devcontainer] or [Nix][nix] (see [nix/README.md][nixmd]). You may also use Apple Clang 16.0 or Microsoft Visual Studio 2022 or newer.

The exception: the C++23 validation lane (CMake option `VALIDATE_CXX23`, below) needs a compiler with solid C++23 support — gcc 15 or clang 21, or newer — and is rejected with MSVC. Likewise the C++26 lane (`VALIDATE_CXX26`, below) needs a compiler implementing `std::type_order` and is rejected with MSVC.

### Standard-mode feature reliance

The headers rely on C++20 only, in every default-mode build. The C++23 validation lane compiles the same sources under `-std=c++23` and relies on no C++23 feature. The `LIBFN_CXX26` mode relies on exactly one C++26 feature: [`std::type_order`](https://cppreference.com/cpp/utility/compare/type_order) (feature-test macro `__cpp_lib_type_order`; implemented by gcc 16). A compiler without it rejects the mode with an `#error` naming this requirement — if your build fails there, either drop `LIBFN_CXX26` or switch to a compiler that implements the feature. `pfn` never relies on post-C++20 features in any mode: it is a polyfill precisely for compilers without the newer standard library.

## Building locally

Both `fn` (`include/fn`) and `pfn` (`include/pfn`) target C++20. By default, with no options configured, the build compiles the library, the unit tests, and the examples in C++20:

```bash
mkdir .build && cd .build
cmake ..
cmake --build .
ctest --output-on-failure
```

The sections below describe other build configurations. They vary that `cmake` line and assume the same build directory. `CMAKE_BUILD_TYPE` behaves as usual in `cmake`, so it is omitted from the examples unless an option constrains it.

For a quick check of a single example without the full CMake/Catch2 setup:

```bash
g++ -std=c++20 -Iinclude examples/polygon/main.cpp -o /tmp/polygon
```

### C++23 and C++26

The unit tests and examples build in C++20 by default. If your compiler is modern enough (GCC 15; Clang 21), use the CMake option `VALIDATE_CXX23=ON` to also build them in C++23 (requires `LIBFN_TESTS=ON`). This enables `tests/pfn/expected_validation.cpp` and `tests/pfn/optional_validation.cpp`. These run the `pfn` test suites against the standard library's `std::expected` and `std::optional` to validate that the polyfills (and the tests themselves) behave exactly like the standard library implementations.

```bash
cmake -DVALIDATE_CXX23=ON ..
```

Setting the CMake option `LIBFN_CXX26=ON` enables C++26 compilation mode, which switches the library's internal type ordering to use C++26's `std::type_order` (see the feature-reliance note above). Because this can change the layout of `copack` and `choice` alternatives, the library uses a distinct ABI namespace to prevent binary compatibility issues. `LIBFN_CXX26=ON` does not select a language version in the compilation options; it needs to be set separately, e.g. with `CMAKE_CXX_STANDARD`, `CXXFLAGS` or in a consumer project.

Setting `VALIDATE_CXX26=ON` (requires `LIBFN_TESTS=ON` and `LIBFN_CXX26=ON`) additionally builds the tests and examples in C++26 mode, selecting the C++26 language version. Since `std::type_order` is only implemented by GCC 16, this requires GCC 16. If GCC 16 is not your default compiler, select it via `CMAKE_CXX_COMPILER` and `CMAKE_C_COMPILER`, or with both `CC` and `CXX` environment variables:

```bash
cmake -DLIBFN_CXX26=ON -DVALIDATE_CXX26=ON ..
```

### Sanitizers

Sanitizers require `CMAKE_BUILD_TYPE=Debug`. They are enabled by default for a top-level build when the build type is `Debug` or left unset (including the default build steps above). To build `Debug` without sanitizers:

```bash
cmake -DLIBFN_SANITIZERS=OFF -DCMAKE_BUILD_TYPE=Debug ..
```

The availability of sanitizers varies by platform and compiler:

* **Linux (GCC & Clang):** Enables Address (ASan), Leak (LSan), and Undefined Behavior (UBSan) sanitizers.
* **macOS (Apple Clang):** Enables ASan and UBSan. LSan is not supported on macOS.
* **Unsupported configurations:** The option is rejected (causes a CMake error) on:
  * MSVC
  * macOS with Homebrew GCC (due to `libasan` library path issues)
  * macOS with Homebrew Clang (due to library mismatches that hang the process)

### Coverage

Setting `LIBFN_COVERAGE=ON` adds a `coverage` target that writes a `coverage.xml` report based on previously run tests. This requires `LIBFN_TESTS=ON`, a top-level build, and GCC or Clang:

```bash
cmake -DLIBFN_COVERAGE=ON ..
cmake --build .
ctest -L 'tests_.*'
cmake --build . --target coverage
```

**Requirements:**

* **gcovr 8.4+:** The coverage build uses the `--merge-lines` option, which was added in `gcovr` 8.4. Since package managers often distribute older versions, using a Python virtual environment (`venv`) is recommended as the easiest way to install a newer version.
* **Coverage tool:** The coverage tool must match your compiler:
  * GCC uses `gcov`.
  * Clang uses `llvm-cov gcov`.
  * Apple Clang uses `llvm-cov` resolved via `xcrun -f`.
  These are detected and configured automatically.

### Documentation

Setting `LIBFN_DOCS=ON` adds an `export_docs` target that generates the API reference in the `docs/` folder of your build directory. This requires `LIBFN_TESTS=ON` and a top-level build:

```bash
cmake -DLIBFN_DOCS=ON ..
cmake --build . --target export_docs
```

**Requirements:**
The following tools must be installed and available in your `PATH`:

* **Doxygen** 1.12.0
* **znai** 1.91 (requires Java/OpenJDK 21 or newer)
* **Graphviz** (provides the `dot` tool used by Doxygen)

For a reference environment with these exact tool versions, see [`ci/docs/Dockerfile`](ci/docs/Dockerfile).

**Validation:**
The documentation build does more than just generate HTML. It also verifies that:

* Every documented entity is present on the generated site.
* The API signatures in the documentation exactly match the C++ headers.

The target will fail if the documentation is out-of-date or has broken references.

## Unit tests

Although we aim for 100% unit-test coverage, executing every line and branch tells only part of the story in this project. Many important guarantees are compile-time properties, such as overload resolution, conversions, constraints and `noexcept` specifications. Tests must therefore exercise an interface with combinations of dimensions, not merely execute every line.

### Structure

A consistent test shape helps make those combinations visible. The guidelines below are a living standard and may evolve when a clearer structure reveals coverage more effectively.

* Use one `TEST_CASE` for each file-level entity (a class, a specialisation, an overload set, etc.) and one top-level `SECTION` for each member function or overload set.
* Use nested sections for the combinations that the member supports: normally value category first, then type properties. Put constraint checks last. A test's place in the `TEST_CASE`/`SECTION` hierarchy should say what it tests; do not put it in the nearest convenient section or a catch-all case.
* Choose the dimensions relevant to the interface under test. Common dimensions include the four cv/ref value categories (`&`, `const &`, `&&` and `const &&`) and properties of the participating types, such as whether they support default, copy or move construction and assignment, and whether those operations are trivial or nothrow. These examples are not exhaustive; they illustrate the range of cases a thorough unit test may need to cover.
* Keep section names short. Use `SECTION`, not the BDD macros (`GIVEN`, `WHEN` and `THEN`).
* Declare the subject alias and reusable probe lambdas once, at `TEST_CASE` scope, rather than redeclaring them in nested sections.
* Use `TEMPLATE_TEST_CASE` only when the same test battery genuinely applies to several subjects. Because it is a macro, give template arguments containing commas a named alias before passing them to it.
* When restructuring tests, run them and record Catch2's test-case and assertion counts before and after the change. The counts should not change accidentally: `SUCCEED()` contributes one assertion per leaf section, while an assertion in a parent section runs once for every leaf below it.

### Assertions

Match the assertion to the kind of fact being tested.

* Use `static_assert` without a runtime twin for a fact decided by the compiler: a `noexcept` specification, overload viability, concept satisfaction, a result type, or a property of default, copy or move construction and assignment, including whether an operation is trivial or nothrow. Do not apply a runtime assertion such as `CHECK` to a compiler-generated constant (for example, `noexcept(f())`); it adds neither coverage nor useful information.
* Test behaviour performed by the program — a computed value, side effect or state change — both at runtime with `CHECK` and in constant evaluation with a `static_assert` twin that repeats the same operations. The runtime test provides coverage and lets sanitizers observe the execution. The `static_assert` verifies that users can perform the same operation in constant evaluation, allowing them to use the compiler to diagnose undefined behaviour.
* For code that may throw, test the expected exception at runtime with `CHECK_THROWS_AS`, and where an operand remains observable after the throw, assert its state. Also exercise the same operation in a non-throwing case, both at runtime and in constant evaluation: a throwing-path test alone never lets that code run to completion. To reach a throw inside relocation machinery, arm a local fixture whose n-th copy or move throws, pre-constructing the operands so that only the operation under test consumes the count.
* End a `SECTION` containing only compile-time assertions with `SUCCEED()`. This satisfies Catch2's no-assertion warning and makes such sections easy to find.

### Compile-time probes

* Pair every negative assertion with its converse. For example, `static_assert(not noexcept(expr))` also passes if the specification is unconditionally false. Add a witness for which it is true, so the pair proves that the specification is conditional. Give a negative constraint or viability probe a positive control for the same reason, choosing witnesses that exercise the relevant corner cases.
* Make the expression in a negative viability probe dependent on a template parameter. A requires-expression over concrete types is not a substitution context and an invalid requirement is a hard error. Put the dependent probe in a generic lambda or a type-keyed concept so that invalid substitution produces `false`.

Choose fixtures with care when probing exception specifications. `helper_t` has a separate constructor for each value category, and its non-const-lvalue copy constructor is always `noexcept`. A `helper_t` configured to throw is therefore not throwing for every value category, and a specification that remains `noexcept` for that constructor may be correct. Use a plain local type when the test needs every relevant construction to be potentially throwing.

## Client code

Code written against the library — examples, documentation snippets, reproducers, anything a user of libfn might imitate:

* Let the library derive graded error types. Never spell a multi-alternative `copack<...>` by hand — the canonical alternative order is internal and platform-dependent (MSVC orders class types after fundamentals); use `copack_for<...>`, and pin a deduced type with `static_assert` where the type is the point.
* Give behaviour a constant-evaluation twin: a `static_assert` replaying the same operations. UB is ill-formed in a constant expression, so the compiler diagnoses what a runtime run may silently get wrong — users rely on the library in constexpr for exactly this.
* Prefer monadic composition to unchecked access. `value()` is the library's only throw; reaching for it where `and_then`/`transform` would carry the value teaches the wrong idiom.
* Don't trust a bare `requires`-probe of a libfn call: several refusals (`apply` on mismatched branches, `and_then` on grade mismatches) are `static_assert`s inside the callee, so the probe answers true and instantiation hard-errors. Compile the call to know; negative viability probes must be dependent (see `### Compile-time probes` above).
* Outside `LIBFN_CXX26`, the internal type ordering is not expected to work with unnamed types or types without linkage — no lambdas or local types as `copack` alternatives in portable code.
* One libfn version per binary: header-only makes mixing versions an ODR violation, including two `z` releases of the same `y` line; default and `LIBFN_CXX26` builds never link as one, by design — see README `## Versioning and ABI`.
* A standalone reproducer (compiler or library bug) proves nothing until it is UB-free: gate it on UBSan and ASan with empty stderr as the criterion, not the exit code.

## Header layering

Four header trees under `include/`, each depending only on those below it:

* `fn` — may use `fn/detail` and `pfn`
* `fn/detail` — may use `pfn`, never `fn`
* `pfn` — the C++23/26 polyfill; standalone except for the version header
* `libfn_version.hpp` — the sole root header, no dependencies

To make an `fn`-level facility available to `fn/detail`, hoist it: the implementation moves into `fn/detail/X.hpp` as `fn::detail::_name` (detail headers are not user-facing and carry no Doxygen); `fn/X.hpp` remains a thin public wrapper re-exporting it as `fn::name` (pattern: `fn/functional.hpp`).

## Versioning

`VERSION` (in the repository root) is the single source of truth for the project version. A pre-commit hook (`scripts/sync_versions.py`) mirrors it into `ports/libfn/vcpkg.json` (`version-semver`), `MODULE.bazel`, and `include/libfn_version.hpp` — the header defining the `LIBFN_VERSION` macro that names the ABI-versioning inline namespace wrapping `fn`, and its mode-less sibling `LIBFN_VERSION_BASE` wrapping `pfn` (the layer rule below). Do **not** hand-edit those version literals — edit `VERSION` and let the hook sync them.

The namespace spelling is derived, not copied: 0.y lines with y ≥ 1 share `v0_<y>` (z bumps are ABI-compatible), the 0.0.z line versions per patch, a SemVer prerelease is appended (`-dev` → `_dev`), and the `_cxx26` twin (selected by defining `LIBFN_CXX26`) keeps `_cxx26` last. `pfn` is mode-less: its layouts never depend on the C++26 type ordering or other language features, so it wraps in `LIBFN_VERSION_BASE` — the plain spelling regardless of mode — and its types stay link-compatible across modes. A second hook (`scripts/check_namespace_wrap.py`) verifies the layer rule: every `namespace fn` opening in `include/` carries `inline namespace LIBFN_VERSION`, every `namespace pfn` opening `inline namespace LIBFN_VERSION_BASE`.

## Pre-commit

This repository uses [pre-commit](https://pre-commit.com/) to enforce formatting of the C++ source code and perform other checks. The details can be seen in `.pre-commit-config.yaml`. To install git commit hooks, which will run checks on the repository as you commit changes:

* configure a Python virtual environment
* install the requirements from `ci/pre-commit/requirements.txt`
* run `pre-commit install` in your local repository

```bash
# Set up a virtual environment to install pre-commit
python3 -m venv .venv
source .venv/bin/activate
pip install -r ci/pre-commit/requirements.txt
# Install the pre-commit hooks locally
pre-commit install
```

You can run all checks manually as follows:

```bash
# Source the virtual environment to access pre-commit
source .venv/bin/activate
# Run pre-commit on local files.
pre-commit run --all-files
```

If a hook modifies files (e.g. clang-format, or the version sync above), the commit is aborted — re-stage the changes and commit again.

## Running CI on a fork

The `codecov` and `sonarcloud` workflows are fork-aware: they target the upstream `libfn` projects only when run from `libfn/functional`, and otherwise report against your own accounts. Set these in your fork (Settings → Secrets and variables → Actions):

* **Codecov** works with no configuration — uploads are tokenless. They are heavily throttled, which a low-traffic fork will not notice; set the `CODECOV_TOKEN` *secret* to lift the throttle.

* **SonarCloud** needs all three of: the `SONAR_TOKEN` *secret*, and the `SONAR_ORGANIZATION` and `SONAR_PROJECT_KEY` *repository variables*. Without the token the SonarCloud scan steps no-op — the build still runs, but no analysis is uploaded. With the token set but a variable missing, no SonarCloud analysis is produced, and it never falls back to the upstream project (a misconfigured push fails fast with a pointer here).

## GitHub Actions workflow pitfalls

A few conventions for files under `.github/workflows/`:

* **Don't pin `ref:` on `actions/checkout` without a reason.** The default already pins to `github.sha` for `push`, `pull_request`, and `workflow_dispatch`, so `ref: ${{ github.sha }}` is redundant in the common case. If you do need it (e.g. so the working tree matches a downstream nix input's `?rev=${{ github.sha }}`), leave a comment saying so.

* **Pin actions to a commit SHA, not a tag.** `uses: owner/repo@v1` is a supply-chain trust decision — whoever can move the tag can execute code in your workflow. Pin the SHA with a trailing version comment (e.g. `actions/checkout@34e1148… # v4.3.1`); Dependabot keeps it current and `zizmor` enforces it via pre-commit.

* **One concern per job; matrix-ify variations.** Don't pack several install/build/test sequences into one job, especially if they need an inter-step cleanup (`vcpkg remove`, `cmake --build . --target clean`). Each variation should be its own matrix entry; gate per-variation steps with `if: matrix.name == 'X'` when needed.

* **The "build everything" run gets its own matrix slot.** If one combination should run a superset of the per-mode targets, add it via `matrix.include` (e.g. `mode: all`) and gate the mode-specific step with `if: matrix.mode != 'all'`. Avoid the build → clean → rebuild dance inside a single job.

* **Multi-line `run:` uses `|` and `\`.** The plain-scalar form (`run: cmd` followed by indented continuation lines) folds into one shell line, which is awkward to copy from a diff into a terminal. The literal-block form (`run: |` + lines ending in `\`) reads and pastes as a shell snippet.

* **Insecure shell inside `run:` blocks.** `actionlint` runs `shellcheck` via pre-commit on every `run:` block if `shellcheck` is on `$PATH`; without it the check is silently skipped. Install `shellcheck` to avoid surprises when CI runs against your PR.

<!-- link references -->
[clang-standard-support]: https://clang.llvm.org/cxx_status.html
[gcc-standard-support]: https://gcc.gnu.org/projects/cxx-status.html
[devcontainer]: https://github.com/libfn/devcontainer
[nix]: https://nixos.org
[nixmd]: nix/README.md
