# functional

Functional programming in C++

## Why

The purpose of this library is to exercise an approach to functional programming in C++ on top of existing C++ vocabulary types (such as `expected`, `optional` etc.), with the aim of eventually extending the future versions of the C++ standard library with the functionality that is found to work well.

## How

The approach is to take existing types in C++ and add both syntax and operations which are useful in writing a functional-style programs in C++

This library requires very modern implementation of C++ library which implements monadic operations on these types, as defined in ISO/IEC 14882:2023. At this moment such library is provided with [gcc 13][gcc-standard-support], which is the recommended compiler for this project. A suggested approach to access this version of the compiler (when it is not available in the operating system) is to use [containers][vscode-devcontainers] for the purpose of working on the project.

[gcc-standard-support]: https://gcc.gnu.org/projects/cxx-status.html
[vscode-devcontainers]: https://code.visualstudio.com/docs/devcontainers/containers

## Acknowledgments

Gašper Ažman for providing the inspiration in ["(Fun)ctional C++ and the M-word"][gasper-functional-presentation]

[gasper-functional-presentation]: https://youtu.be/Jhggz8rtHbk?si=T-3DXPcvgE_Y5cpH
