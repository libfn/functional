# Contributing to libfn

This is for working *on* libfn; to *use* the library, see the [README](README.md).

## Development environment

Building and testing libfn needs a recent C++23 toolchain (the monadic `std::optional`/`std::expected` of ISO/IEC 14882:2023). The recommended compilers are [gcc 13][gcc-standard-support] and [clang 18][clang-standard-support]; if your OS does not ship one recent enough, use the [devcontainer] or [Nix][nix] (see [nix/README.md][nixmd]).

## Building locally

`fn` (`include/fn`) requires C++23; `pfn` (`include/pfn`) targets C++20. The CMake option `DISABLE_CXX23` builds only the C++20-compatible `pfn` subset. For a quick check of a single example without the full CMake/Catch2 setup:

```bash
g++ -std=c++23 -Iinclude examples/polygon/main.cpp -o /tmp/polygon
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

<!-- link references -->
[clang-standard-support]: https://clang.llvm.org/cxx_status.html
[gcc-standard-support]: https://gcc.gnu.org/projects/cxx-status.html
[devcontainer]: https://github.com/libfn/devcontainer
[nix]: https://nixos.org
[nixmd]: nix/README.md
