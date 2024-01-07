[![codecov](https://codecov.io/gh/Bronek/functional/graph/badge.svg?token=3RHT38SEU0)](https://codecov.io/gh/Bronek/functional)

# functional

Functional programming in C++

## Why

The purpose of this library is to exercise an approach to functional programming in C++ on top of the existing `std` C++ vocabulary types (such as `std::expected` and `std::optional`), with the aim of eventually extending the future versions of the C++ standard library with the functionality found to work well.

## How

The approach is to take the existing types in the C++ standard library (when appropriate) and add both syntax and operations which are useful in writing functional style programs.

This library requires a very modern implementation of C++ library which implements monadic operations in `optional` and `expected`, as defined in ISO/IEC 14882:2023. Currently, such a library is provided with [gcc 13][gcc-standard-support], which is the recommended compiler for this project. A suggested approach to access this version of the compiler (when it is not available in the operating system) is to use [containers][vscode-devcontainers] when working with this project.

[gcc-standard-support]: https://gcc.gnu.org/projects/cxx-status.html
[vscode-devcontainers]: https://code.visualstudio.com/docs/devcontainers/containers

## Coverage

In this project, 100% tests coverage does not actually mean much, because the most useful
tests cases are around compile-time language parts, such as overload resolution etc.

Nevertheless, each branch and function must have been executed in the unit tests at least once.

[![codecov-graph](https://codecov.io/gh/Bronek/functional/graphs/tree.svg?token=3RHT38SEU0)](https://codecov.io/gh/Bronek/functional)


## Acknowledgments

Gašper Ažman for providing the inspiration in ["(Fun)ctional C++ and the M-word"][gasper-functional-presentation]

[gasper-functional-presentation]: https://youtu.be/Jhggz8rtHbk?si=T-3DXPcvgE_Y5cpH
