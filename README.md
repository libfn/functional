[![codecov](https://codecov.io/gh/libfn/functional/graph/badge.svg?token=3RHT38SEU0)](https://codecov.io/gh/libfn/functional)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Flibfn%2Ffunctional.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2Flibfn%2Ffunctional?ref=badge_shield)

# functional

Functional programming in C++

## Why

The purpose of this library is to exercise an approach to functional programming in C++ on top of the existing `std` C++ vocabulary types (such as `std::expected` and `std::optional`), with the aim of eventually extending the future versions of the C++ standard library with the functionality found to work well.

## How

The approach is to take the existing `std` types in the C++ standard library (when appropriate) and extend them (via inheritance) with the facilities useful in writing functional style programs. Eventually, the proposed functionality will be (hopefully) folded into the existing `std` types and new `std` types will be added.

This library requires a very modern implementation of the C++ library which implements monadic operations in `std::optional` and `std::expected`, as defined in ISO/IEC 14882:2023. Currently, such implementations are provided with [gcc 13][gcc-standard-support] and [clang 18][clang-standard-support], which are the recommended compilers for this project. A suggested approach to access the most recent version of the compiler (when it is not available in the operating system) is to use a [devcontainer] when working with this project. Alternatively take a look at [Nix][nix] and [nix/README.md][nixmd].

### Implementation note

This library requires standardized type ordering, which currently is a [proposed C++26 feature][standardized-type-ordering] and is not implemented by any compiler. Currently, the library relies on an internal, naive implementation of such a feature which is _not expected to work_ with unnamed types, types without linkage etc.

[clang-standard-support]: https://clang.llvm.org/cxx_status.html
[gcc-standard-support]: https://gcc.gnu.org/projects/cxx-status.html
[devcontainer]: https://github.com/libfn/devcontainer
[standardized-type-ordering]: https://wg21.link/P2830
[nix]: https://nixos.org
[nixmd]: nix/README.md

## Test Coverage

In this project, 100% tests coverage does not actually mean much, because the most useful tests cases are around compile-time language elements, such as overload resolution, built-in conversions etc. Any meaningful tests must execute the same set of functions in many, subtly different ways, rather than simply execute each function and branch at least once.

## Backwards compatibility

The maintainers will aim to maintain compatibility with the proposed changes in the C++ standard library, **rather than with the existing uses** of the code in this repo. In practice, this means that all code in this repo should be considered "under intensive development and unstable" until the standardization of the proposed facilities.

### Best make your private fork from this repo and use it as you see fit.

The maintainers are unable to guarantee that no significant refactoring will ever take place. The opposite is to be expected, since the process of standardization of any additions to the C++ standard library typically involves a fair number of changes and improvements, some of them quite fundamental. This includes all kinds of interfaces and names in this library, which until the moment of standardization are only _proposed_.

## Acknowledgments

* Gašper Ažman, for providing the inspiration in ["(Fun)ctional C++ and the M-word"][gasper-functional-presentation]
* Bartosz Milewski, for taking the time to explain [parametrised and graded monads][parametrised-and-graded-monads]
* [Ripple][ripple], for allowing the main author the time to work on this library

[gasper-functional-presentation]: https://youtu.be/Jhggz8rtHbk?si=T-3DXPcvgE_Y5cpH
[parametrised-and-graded-monads]: https://arxiv.org/pdf/2001.10274.pdf
[similar-work]: https://www.doc.ic.ac.uk/~dorchard/publ/haskell14-effects.pdf
[ripple]: https://ripple.com/

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

## License

[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Flibfn%2Ffunctional.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2Flibfn%2Ffunctional?ref=badge_large)
