# functional

Functional programming in C++

[![codecov](https://codecov.io/gh/libfn/functional/graph/badge.svg?token=3RHT38SEU0)](https://codecov.io/gh/libfn/functional)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Flibfn%2Ffunctional.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2Flibfn%2Ffunctional?ref=badge_shield)

## Why

The purpose of this library is to exercise an approach to functional programming in C++ on top of the existing `std` C++ vocabulary types (such as `std::expected` and `std::optional`), with the aim of eventually extending the future versions of the C++ standard library with the functionality found to work well.

## How

The approach is to take the existing `std` types in the C++ standard library (when appropriate) and extend them (via inheritance) with the facilities useful in writing functional style programs. Eventually, the proposed functionality will be (hopefully) folded into the existing `std` types and new `std` types will be added.

This library requires a very modern implementation of the C++ library which implements monadic operations in `std::optional` and `std::expected`, as defined in ISO/IEC 14882:2023. Currently, such implementations are provided with [gcc 13][gcc-standard-support] and [clang 18][clang-standard-support], which are the recommended compilers for this project. See [CONTRIBUTING.md](CONTRIBUTING.md) for how to set up a recent enough toolchain when your OS does not ship one.

### Implementation note

This library requires standardized type ordering, which currently is a [proposed C++26 feature][standardized-type-ordering] and is not implemented by any compiler. Currently, the library relies on an internal, naive implementation of such a feature which is _not expected to work_ with unnamed types, types without linkage etc.

## Backwards compatibility

The maintainers will aim to maintain compatibility with the proposed changes in the C++ standard library, **rather than with the existing uses** of the code in this repo. In practice, this means that all code in this repo should be considered "under intensive development and unstable" until the standardization of the proposed facilities.

### Best make your private fork from this repo and use it as you see fit.

The maintainers are unable to guarantee that no significant refactoring will ever take place. The opposite is to be expected, since the process of standardization of any additions to the C++ standard library typically involves a fair number of changes and improvements, some of them quite fundamental. This includes all kinds of interfaces and names in this library, which until the moment of standardization are only _proposed_.

## Versioning and ABI

Releases are numbered `0.y.z` and will stay below `1.0.0` for the foreseeable future. [SemVer](https://semver.org/) treats any `0.y.z` version as unstable — anything may change — so libfn narrows that into a usable contract:

- a bump in **`y`** is a **breaking** change (API and/or ABI);
- a bump in **`z`** is a bug fix or a purely additive extension: the API and ABI stay compatible, but **inline function definitions may change**.

Because the library is header-only, **use a single libfn version per binary**. Mixing versions in one program is an ODR violation — and that includes two `z` releases of the *same* `y` line, whose inline definitions may differ even though the ABI matches.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for the development environment, building, testing, the version-bump mechanics, and the pre-commit workflow.

## Acknowledgments

* Gašper Ažman, for providing the inspiration in ["(Fun)ctional C++ and the M-word"][gasper-functional-presentation]
* Bartosz Milewski, for taking the time to explain [parametrised and graded monads][parametrised-and-graded-monads] and [effect systems][effect-systems]
* [Ripple][ripple], for allowing the main author the time to work on this library

## License

[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Flibfn%2Ffunctional.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2Flibfn%2Ffunctional?ref=badge_large)

<!-- link references -->
[clang-standard-support]: https://clang.llvm.org/cxx_status.html
[gcc-standard-support]: https://gcc.gnu.org/projects/cxx-status.html
[standardized-type-ordering]: https://wg21.link/P2830
[gasper-functional-presentation]: https://youtu.be/Jhggz8rtHbk?si=T-3DXPcvgE_Y5cpH
[parametrised-and-graded-monads]: https://arxiv.org/pdf/2001.10274.pdf
[effect-systems]: https://www.doc.ic.ac.uk/~dorchard/publ/haskell14-effects.pdf
[ripple]: https://ripple.com/
