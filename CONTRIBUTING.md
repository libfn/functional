# Contributing to libfn

This is for working *on* libfn; to *use* the library, see the [README](README.md).

## Development environment

Building and testing `libfn` requires a C++20 toolchain. The `pfn` namespace polyfills C++23/26 standard library utilities (`expected`, `optional`, `invoke_r`, `unreachable`), which the `fn` layer builds upon.

The minimum supported compilers are [gcc 12][gcc-standard-support] and [clang 16][clang-standard-support]. If your host OS lacks these, use the [devcontainer] or [Nix][nix] environment (see [nix/README.md][nixmd]). Apple Clang 16.0 or MSVC 2022 (or newer) are also supported.

Exceptions to the C++20 baseline:
* **C++23 Validation Lane** (CMake option `VALIDATE_CXX23`): Requires a compiler with solid C++23 support (such as GCC 15 or Clang 21) and is unsupported on MSVC.
* **C++26 Validation Lane** (CMake option `VALIDATE_CXX26`): Requires a compiler implementing `std::type_order` (such as GCC 16) and is unsupported on MSVC.

### Standard-mode feature reliance

By default, `libfn` headers rely strictly on C++20. The C++23 validation lane compiles the same sources under `-std=c++23` without relying on C++23 features. The `LIBFN_CXX26` mode relies on a single C++26 feature [`std::type_order`](https://cppreference.com/cpp/utility/compare/type_order) (feature-test macro `__cpp_lib_type_order`). If the compiler lacks this feature, compilation fails with an `#error` naming the requirement. `pfn` never relies on post-C++20 features in any configuration.

## Building locally

By default, the CMake build compiles the library, unit tests, and examples in C++20 mode:

```bash
mkdir .build && cd .build
cmake ..
cmake --build .
ctest --output-on-failure
```

Subsequent build configuration examples vary this basic `cmake` command and reuse the same build directory.

To compile and run a single example directly without CMake:

```bash
g++ -std=c++20 -Iinclude examples/polygon/main.cpp -o /tmp/polygon
```

### C++23 and C++26

On modern compilers (GCC 15 or Clang 21), enabling `VALIDATE_CXX23=ON` (requires `LIBFN_TESTS=ON`) compiles unit tests and examples in C++23 mode. This enables `tests/pfn/expected_validation.cpp` and `tests/pfn/optional_validation.cpp` to validate that `pfn` polyfills behave identically to standard `std::expected` and `std::optional`.

```bash
cmake -DVALIDATE_CXX23=ON ..
```

Enabling `LIBFN_CXX26=ON` activates C++26 type-ordering via `std::type_order`. This option does **not** inject a compiler language standard flag. You need to select C++26 separately, for example with:

* Explicit choice via `target_compile_features(<your target> INTERFACE cxx_std_26)`
* Dependency on exported target `libfn::fn_cxx26` (which propagates `cxx_std_26`)
* Dependency on CMake native target `include_fn_cxx26` (which propagates `cxx_std_26`)
* The `CMAKE_CXX_STANDARD=26` CMake variable
* The `CXXFLAGS=-std=c++26` environment variable

The C++26 mode requires a compiler supporting C++26, such as GCC 16. If installed but not configured as the default, select it via the `CC` and `CXX` environment variables before running CMake. Ensure **both** are exported: e.g., `export CC=/usr/bin/gcc-16; export CXX=/usr/bin/g++-16` (on Ubuntu 26.04).

Because `LIBFN_CXX26=ON` changes the layout of `copack` and `choice` alternatives, the library uses a distinct ABI namespace to prevent binary compatibility issues.

Enabling `VALIDATE_CXX26=ON` (requires `LIBFN_TESTS=ON` and `LIBFN_CXX26=ON`) builds tests and examples in C++26 mode (no need to enable it separately, compatible compiler required).

```bash
cmake -DLIBFN_CXX26=ON -DVALIDATE_CXX26=ON ..
```

### Sanitizers

Sanitizers require `CMAKE_BUILD_TYPE=Debug` and are enabled by default, on supported platforms. To disable sanitizers in a debug build:

```bash
cmake -DLIBFN_SANITIZERS=OFF -DCMAKE_BUILD_TYPE=Debug ..
```

Supported platforms:

* **Linux (GCC / Clang)**: Enables Address (ASan), Leak (LSan), and Undefined Behavior (UBSan).
* **macOS (Apple Clang)**: Enables ASan and UBSan (LSan is unsupported).

**Unsupported Environments**:

* MSVC
* macOS with Homebrew GCC (due to `libasan` path resolution issues)
* macOS with Homebrew Clang (due to runtime process hangs from library mismatches)

On the above platforms, the build rejects sanitizer flags (causing a CMake configuration error).

### Coverage

Enabling `LIBFN_COVERAGE=ON` (requires `LIBFN_TESTS=ON`, GCC or Clang) adds a `coverage` target to generate a `coverage.xml` report:

```bash
cmake -DLIBFN_COVERAGE=ON ..
cmake --build .
ctest -L 'tests_.*'
cmake --build . --target coverage
```

Requirements:

* **gcovr 8.4+**: The coverage build requires `gcovr`'s `--merge-lines` option. You may install it via a Python virtual environment (e.g. `pip` or `uv` under your `$HOME` directory) if your package manager distributes an older version.
* **Compiler Compatibility**: The underlying coverage tool is detected automatically:
  * GCC: Uses `gcov`.
  * Clang: Uses `llvm-cov gcov`.
  * Apple Clang: Uses `llvm-cov` via `xcrun -f`.

### Documentation

Enabling `LIBFN_DOCS=ON` (requires `LIBFN_TESTS=ON`) adds the `export_docs` target to generate the API reference in the build directory's `docs/` folder:

```bash
cmake -DLIBFN_DOCS=ON ..
cmake --build . --target export_docs
```

The exported tree contains the site root and a versioned copy under `v<version>/` (derived from `VERSION`), with internal links adjusted for their serving paths. The live site combines this output with archived versions hosted on [libfn/website][website].

Requirements:

* **Doxygen** 1.12.0 (note, newer version 1.17.0 is known to be incompatible)
* **[`znai`][znai]** 1.91 (requires Java/OpenJDK 21 or newer)
* **Graphviz** (providing `dot`)
* See [`ci/docs/Dockerfile`](ci/docs/Dockerfile) for the exact reference environment.

The documentation build fails on outdated docs or broken references, verifying that:

* Every documented entity is present on the generated site.
* All documented API signatures match the C++ headers exactly.

## Unit tests

While unit-test coverage target is 100%, execution of every line and branch is insufficient. Crucial guarantees — including overload resolution, conversions, constraints, and `noexcept` specifications — are compile-time properties, and must be asserted in unit tests as well. Tests must exercise interface dimensions in combination, rather than merely executing lines of code.

### Structure

Consistent structure exposes these testing dimensions:

* **Test Layout**: Use one `TEST_CASE` per file-level entity (class, specialization, or overload set) and a top-level `SECTION` for each member function.
* **Hierarchy**: Nest sections to test combinations: value category first, followed by type properties, with constraint checks last. A test's placement in the section hierarchy must define its purpose — avoid catch-all sections.
* **Test Dimensions**: Test across relevant dimensions, including the four cv/ref value categories (`&`, `const &`, `&&`, `const &&`) and type traits (such as triviality, throwing behavior, or support for default/copy/move construction and assignment).
* **Conciseness**: Keep section names short. Use plain `SECTION` instead of BDD macros (`GIVEN`, `WHEN`, `THEN`).
* **Scope**: Declare subject type aliases and reusable probe lambdas once at `TEST_CASE` scope to avoid redundant declarations in nested sections.
* **Templates**: Use `TEMPLATE_TEST_CASE` only when the exact same test battery applies to multiple types. Alias template arguments containing commas to avoid macro expansion errors.
* **Assertion Counts**: Verify that restructuring tests does not alter Catch2 assertion or test counts. `SUCCEED()` adds one assertion per leaf section, whereas parent-section assertions execute once per nested leaf.

### Assertions

Match the assertion to the tested property:

* **Compiler-Decided Facts**: Use `static_assert` without a runtime twin for properties decided purely by the compiler (such as `noexcept` specifications, overload viability, concept satisfaction, result types, or constructor triviality). Do not use runtime checks (like `CHECK`) on compile-time constants (e.g., `noexcept(f())`).
* **Program Behavior**: Test dynamic behavior—computed values, side effects, or state changes—using `CHECK` at runtime and a corresponding `static_assert` twin in constant evaluation. Runtime tests provide sanitizer coverage, while `static_assert` twins verify `constexpr` compatibility and let the compiler diagnose undefined behavior.
* **Exceptions**: Test throwing paths at runtime with `CHECK_THROWS_AS`, and assert the state of any observable operands after the exception. Always pair throwing-path tests with a non-throwing case (at runtime and in constant evaluation) to ensure the code paths run to completion. To isolate throwing paths in move/relocation machinery, use a local fixture configured to throw on its n-th operation.
* **Empty Sections**: End sections containing only compile-time assertions with `SUCCEED()` to satisfy Catch2's warnings and mark compile-only branches clearly.

### Compile-time probes

* **Conditional Specifications**: Pair negative assertions with their positive converses (e.g., pair `static_assert(not noexcept(expr))` with a witness for which it is `noexcept`). This proves that the specification is conditional rather than unconditionally false. Apply the same positive-control discipline to negative constraint or viability probes.
* **Substitution Contexts**: Ensure expressions in negative viability probes are dependent on a template parameter. A `requires`-expression over concrete types is not a substitution context, meaning an invalid requirement triggers a hard error rather than a SFINAE failure. Enclose the dependent probe in a generic lambda or a type-keyed concept to yield `false` on invalid substitution.
* **Exception Fixtures**: Select exception-proving fixtures carefully. The library's `helper_t` implements a separate constructor for each value category, and its non-const-lvalue copy constructor is always `noexcept`. A `helper_t` configured to throw is therefore not throwing for every value category, which can hide incorrect `noexcept` specifications. Use a simple, local type if the test requires every relevant constructor to be potentially throwing.

## Client code

Code written against the library (such as examples, documentation snippets, and reproducers) serves as user guidance:

* **Graded Error Types**: Except for the most trivial cases, do not spell a multi-alternative `copack<...>` or `choice<...>` manually. The canonical alternative order is internal and compiler-dependent. Use `copack_for<...>` and `choice_for<...>`, and use `static_assert` to verify the deduced type.
* **Constexpr Twins**: Pair dynamic code with a corresponding `static_assert` twin that replays the same operations. Because undefined behavior is ill-formed in constant expressions, the compiler will diagnose issues that runtime execution might silently miss.
* **Monadic Composition**: Prefer monadic composition over unchecked access. `.value()` is the library's only throwing path. Do not promote an anti-pattern by using it where `and_then` or `transform` or `apply` can safely propagate values.
* **Requires Probes**: Do not rely on bare `requires`-probes of `libfn` calls. Several compile-time rejections (such as mismatched branch types in `apply` or grade mismatches in `and_then`) are enforced via internal `static_assert`s, which causes the probe to evaluate to `true` while the instantiation fails with a hard compiler error. Verify the call by compiling it, and use dependent viability probes.
* **Type Ordering**: Outside of `LIBFN_CXX26`, the internal type ordering does **not** support unnamed types or types without linkage, and is **not** portable between GCC and Clang. In portable code, do not use lambdas or local types as `copack` alternatives and do not mix GCC with Clang (both using `libfn`) in a single binary.
* **ABI & Versions**: Link exactly one `libfn` version per binary. Because the library is header-only, mixing different versions (including distinct patch releases) may cause ODR violation in your program. The authors will strive to ensure that each potentially incompatible version uses a unique namespace to prevent ODR violations, but **validating dependency consistency remains the user's responsibility**. Builds with and without `LIBFN_CXX26` use different namespaces and are link-incompatible by design.
* **Reproducers**: Standalone bug reproducers must be entirely free of undefined behavior. Validate reproducers against UBSan and ASan, requiring an empty standard error output rather than merely relying on a zero exit code. The library currently contains workarounds for a known [Clang miscompile error][llvmbug].

## Header layering

The directory `include/` is structured into four header trees, strictly restricted to downward-only dependencies:

* `fn/`: May include `fn/detail` and `pfn`.
* `fn/detail/`: May include `pfn`, but must never include `fn`.
* `pfn/`: The C++23/26 polyfill; standalone except for the version header (below).
* `libfn_version.hpp`: The root version header, with zero dependencies.
* `fn/detail/macro_begin.hpp` and `fn/detail/macro_end.hpp`: Macros scoping headers, with zero dependencies.

To share an `fn`-level facility with `fn/detail`, hoist it: move the implementation into `fn/detail/X.hpp` as `fn::detail::_name` (detail headers are internal and lack Doxygen comments), and expose it in `fn/X.hpp` as a thin wrapper re-exporting `fn::name` (following the pattern of `fn/functional.hpp`).

## Versioning

The `VERSION` file in the repository root is the sole source of truth for the library version. The `scripts/sync_versions.py` pre-commit hook automatically propagates this version to `ports/libfn/vcpkg.json`, `MODULE.bazel`, and `include/libfn_version.hpp`. The version header defines the `LIBFN_VERSION` macro (the inline namespace wrapping `fn`) and its mode-less sibling `LIBFN_VERSION_BASE` (wrapping `pfn`). Do not modify these version literals manually — edit `VERSION` and let the hook synchronize them.

Inline namespaces are derived dynamically:

* **Minor Releases (0.y with y ≥ 1)**: Share the `v0_<y>` namespace (patch releases are **intended** to be ABI-compatible).
* **Prereleases**: SemVer prerelease tags append directly (e.g., `-dev` becomes `_dev`) — unless tagged on `release` branch, these will be **incompatible within single version**.
* **C++26 Twin**: Selected via `LIBFN_CXX26`, keeping the `_cxx26` suffix last.

The `pfn` namespace is mode-less; its data layouts do not depend on the C++26 type ordering or language features. It remains wrapped in `LIBFN_VERSION_BASE` (the base version without the `_cxx26` suffix) to ensure link compatibility across compilation modes. The `scripts/check_namespace_wrap.py` pre-commit hook enforces this layering by verifying that every `namespace fn` block uses `inline namespace LIBFN_VERSION` and every `namespace pfn` block uses `inline namespace LIBFN_VERSION_BASE`.

## Releasing

A release is defined by a GPG-signed tag on a `main` commit fast-forwarded onto the `release` branch. This branch is the source for documentation deploys and single-header publishing.

Summarize `CHANGELOG.md` immediately before a release or release candidate: collapse the dated entries into a compact, dateless summary of changes. The summary must reference the commit SHA of the last detailed list (the commit immediately preceding the first release candidate) so the full historic log remains accessible.

### The procedure

```sh
# 1. Verify that the candidate commit is green across the build matrix.
gh api repos/libfn/functional/commits/<main-sha>/check-runs \
  --jq '.check_runs[] | select(.conclusion != "success") | .name + " " + .conclusion'

# 2. Fast-forward the release branch onto the candidate and tag it.
git checkout release
git merge --ff-only <main-sha>
git tag -s v<version> -m 'libfn <version>' <main-sha>

# 3. Push the branch and the tag atomically.
git push --atomic origin release:release refs/tags/v<version>

# 4. Once the documentation deploy is green, create the GitHub Release.
gh release create v<version> --verify-tag --generate-notes --draft

# 5. Review release notes, press "Publish release"
# 6. Bump VERSION on main branch to open the next cycle, via a PR
```

### Version cadence

`VERSION` increments sequentially based on the active phase:

* **Release Candidate**: `x.y.z-rcN` transitions to `x.y.z` (the release) or `x.y.z-rc(N+1)` (a subsequent candidate).
* **Development**: Immediately after tagging `x.y.z`, the version on `main` bumps to `x.y.(z+1)-dev`.
* **Breaking Cycles**: If a `-dev` cycle introduces breaking changes, the version bumps to `x.(y+1).0-dev`.

The immediate bump to `-dev` renames the inline ABI namespace (e.g., `v0_1` to `v0_1_dev`), preventing builds of unreleased `main` code from linking with the official release.

### Key step rationales

* **Fast-Forward & Tag Location**: The `release` branch requires linear history. Fast-forwarding onto the candidate commit ensures that the tag sits on a commit reachable from both `main` and `release`. This ensures `git describe` resolves properly across all pull requests and branches.
* **Tag Before Push**: The documentation build derives the single-header banner from `git describe`. The tag must exist on the commit before pushing, otherwise the generated artifact will use an incorrect name or fallback description.
* **Atomic Push**: Using `git push --atomic` updates both the branch reference and the tag in a single transaction. This prevents GitHub Actions workflows from triggering on a branch update before the corresponding tag is visible.
* **GitHub Release**: Creating the Release object triggers the `single-header` workflow's `publish` job, which generates and attaches `libfn-v<version>.hpp` to the release. The `--generate-notes` flag automatically populates the PR log.
* **Provenance**: The `publish` job attests the single-header asset before upload, binding its cryptographic digest to this repository, the tagged commit, and the workflow run (enforcing SLSA build provenance signed via Sigstore). Running `gh attestation verify libfn-v<version>.hpp --repo libfn/functional` confirms that a downloaded copy is the authentic artifact built by this repository.

The release push triggers the `docs` workflow, which rebuilds the site, archives the tree in [libfn/website][website], and deploys.

A release is complete when the live banner on [libfn.org][libfn] reads `Revision: v<version>`, `versions.html` lists `/v<version>/`, the GitHub Release attaches `libfn-v<version>.hpp`, and the `VERSION` bump to the **next** version is **merged** on `main`.

### Repository prerequisites

The release workflow depends on three external repository configurations:

* **GitHub Pages**: The `github-pages` environment must permit deployments from the `release` branch.
* **Branch Protection**: The `release` branch must enforce linear history, but must *not* require a pull request or require deployments to succeed (which would create a deadlock blocking the push).
* **Credentials**: The `WEBSITE_PUSH_TOKEN` repository secret (used to archive documentation in [libfn/website][website]) must be a valid, unexpired personal access token.

## Pre-commit

This repository uses [pre-commit](https://pre-commit.com/) to enforce formatting of the C++ source code and perform other checks. The details can be seen in `.pre-commit-config.yaml`. To install git commit hooks, which will run checks on the repository as you commit changes:

* configure a Python virtual environment (e.g. with `pip` or `uv` under your `$HOME`)
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

The `codecov` and `sonarcloud` workflows are fork-aware, targeting upstream `libfn` only when executed from `libfn/functional`, and otherwise routing reports to your fork's accounts. To configure these in your fork, set the following parameters under **Settings → Secrets and variables → Actions**:

* **Codecov**: Operates tokenless with zero configuration. Because tokenless uploads are rate-limited, set the `CODECOV_TOKEN` secret to avoid rate-limiting on busy forks.
* **SonarCloud**: Requires the `SONAR_TOKEN` secret, as well as the `SONAR_ORGANIZATION` and `SONAR_PROJECT_KEY` repository variables. Without the token, SonarCloud scans are skipped (the build succeeds, but no report is uploaded). If the token is set but variables are missing, the build fails fast to point to the misconfiguration.

## GitHub Actions workflow pitfalls

Follow these conventions for files under `.github/workflows/`:

* **Checkout Refs**: Avoid setting `ref:` on `actions/checkout` without explicit justification. The action defaults to `github.sha` on `push`, `pull_request`, and `workflow_dispatch` events, making explicit refs redundant.
* **Action Pinning**: Pin all actions to a commit SHA rather than a tag (e.g., `uses: actions/checkout@11bd71901b... # v4.2.2`) to secure the supply chain. Dependabot maintains these pins, and `zizmor` enforces them at commit time.
* **Job Isolation**: Isolate separate concerns into distinct jobs using GitHub build matrices. Do not combine multiple install, build, and test sequences into a single job that requires manual step-by-step cleanup. Use matrix exclusions or conditional gates (`if: matrix.name == 'X'`) where necessary.
* **Monolithic Build Slots**: If a test suite requires compiling all combinations, assign a dedicated slot in the build matrix (e.g., `matrix.include` with `mode: all`) and gate other steps with `if: matrix.mode != 'all'`. Avoid inline clean-and-rebuild patterns within a single job runner.
* **Multi-Line Commands**: Use the literal-block form (`run: |`) and line continuations (`\`) for multi-line shell commands. This ensures scripts are easily copy-pasteable from diffs directly to local terminals.
* **Shell linting**: Install `shellcheck` locally. `actionlint` runs `shellcheck` via pre-commit on every inline shell block if it is available in your `PATH`; missing linters are silently skipped.
* **Draft Pull Requests**: Skip expensive actions on draft pull requests by adding the condition `if: github.event.pull_request.draft != true` to each job, and explicitly include `ready_for_review` in the `pull_request` trigger `types`. Cheap checks (pre-commit, license validation, and documentation builds) should still run on drafts. Skipped jobs resolve as `skipped` rather than `success`, preventing downstream scan triggers from executing prematurely.

<!-- link references -->
[clang-standard-support]: https://clang.llvm.org/cxx_status.html
[gcc-standard-support]: https://gcc.gnu.org/projects/cxx-status.html
[devcontainer]: https://github.com/libfn/devcontainer
[nix]: https://nixos.org
[nixmd]: nix/README.md
[website]: https://github.com/libfn/website
[znai]: https://github.com/testingisdocumenting/znai
[llvmbug]: https://github.com/llvm/llvm-project/issues/196520
[libfn]: https://libfn.org/license/index
