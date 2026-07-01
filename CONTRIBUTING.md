# Contributing to libfn

This is for working *on* libfn; to *use* the library, see the [README](README.md).

## Development environment

Building and testing libfn needs a C++20 toolchain (with one exception). `pfn` polyfills C++23/26 standard-library utilities (`pfn::expected`, `pfn::optional`, `pfn::invoke_r`, `pfn::unreachable`); `fn` builds on `pfn` rather than the newer standard library. The minimum supported compilers are [gcc 12][gcc-standard-support] and [clang 16][clang-standard-support]; if your OS does not ship one recent enough, use the [devcontainer] or [Nix][nix] (see [nix/README.md][nixmd]). You may also use Apple Clang 16.0 or Microsoft Visual Studio 2022 or newer.

When working with `tests/pfn/expected.cpp` you will need a C++23 compiler, in order to enable the `VALIDATE_CXX23` option for `expected_validation.cpp` tests.

## Building locally

Both `fn` (`include/fn`) and `pfn` (`include/pfn`) target C++20. The unit tests and examples build in C++20 by default. If you have a recent enough compiler, use the CMake option `VALIDATE_CXX23=ON` to additionally build them in C++23 (requires `LIBFN_TESTS=ON`).

For a quick check of a single example without the full CMake/Catch2 setup:

```bash
g++ -std=c++20 -Iinclude examples/polygon/main.cpp -o /tmp/polygon
```

## Test coverage

In this project, 100% tests coverage does not actually mean much, because the most useful tests cases are around compile-time language elements, such as overload resolution, built-in conversions etc. Any meaningful tests must execute the same set of functions in many, subtly different ways, rather than simply execute each function and branch at least once.

## Versioning

`VERSION` (in the repository root) is the single source of truth for the project version. A pre-commit hook (`scripts/sync_versions.py`) mirrors it into `ports/libfn/vcpkg.json` (`version-semver`) and `MODULE.bazel`. Do **not** hand-edit those version literals — edit `VERSION` and let the hook sync them.

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
# Now install the pre-commit hooks locally
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
