[![codecov](https://codecov.io/gh/libfn/functional/graph/badge.svg?token=3RHT38SEU0)](https://codecov.io/gh/libfn/functional)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Flibfn%2Ffunctional.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2Flibfn%2Ffunctional?ref=badge_shield)

# functional

Functional programming in C++

## Why

The purpose of this library is to exercise an approach to functional programming in C++ on top of the existing `std` C++ vocabulary types (such as `std::expected` and `std::optional`), with the aim of eventually extending the future versions of the C++ standard library with the functionality found to work well.

## How

The approach is to take the existing types in the C++ standard library (when appropriate) and add both syntax and operations which are useful in writing functional style programs.

This library requires a very modern implementation of the C++ library which implements monadic operations in `optional` and `expected`, as defined in ISO/IEC 14882:2023. Currently, such implementations are provided with [gcc 13][gcc-standard-support] and [clang 18][clang-standard-support], which are the recommended compilers for this project. A suggested approach to access the most recent version of the compiler (when it is not available in the operating system) is to use a [devcontainer] when working with this project.

[clang-standard-support]: https://clang.llvm.org/cxx_status.html
[gcc-standard-support]: https://gcc.gnu.org/projects/cxx-status.html
[devcontainer]: https://github.com/libfn/devcontainer

## Coverage

In this project, 100% tests coverage does not actually mean much, because the most useful tests cases are around compile-time language elements, such as overload resolution, built-in conversions etc. Any meaningful tests must execute the same set of functions in many, subtly different ways, rather than simply execute each function and branch at least once.

## Acknowledgments

Gašper Ažman for providing the inspiration in ["(Fun)ctional C++ and the M-word"][gasper-functional-presentation]

[gasper-functional-presentation]: https://youtu.be/Jhggz8rtHbk?si=T-3DXPcvgE_Y5cpH


## License
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Flibfn%2Ffunctional.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2Flibfn%2Ffunctional?ref=badge_large)
