# functional

Functional programming in C++

## Why

The purpose of this library is to exercise an approach to functional programming in C++ on top of the existing standard vocabulary types (such as `std::expected` and `std::optional`), with the aim of eventually extending future revisions of the C++ standard library with the functionality found to work well.

## How

The library comes as two parts: `pfn` (namespace `pfn`) is a faithful polyfill of the standard vocabulary types as specified for C++26, available to a C++20 compiler; `fn` (namespace `fn`) extends them with the facilities useful in writing functional style programs and adds new vocabulary types. Every `fn` type with a `pfn` counterpart is a strict superset of it: switching a valid program from `pfn` types to `fn` changes neither compilation nor behaviour.

## What

The library provides the following utilities:

* functors - extensible system of encapsulation of monadic operations, expressed with a pipe `operator |`
* copack - coproduct of types (a sum of types), similar to `std::variant` but indexed by type rather than order, composes with the product of types
* choice monad - monad built on top of the coproduct of types, dispatch by overloading rules
* pack - product of types, similar to `std::tuple`, composes with the coproduct of types
* composition - monadic computations combined side by side: conjunction with `operator &`, disjunction with `operator |`, and their n-ary folds `fn::conjoin` and `fn::disjoin`
* multidispatch - dispatch any valid combination of product(s) and coproduct(s) to a function, based on overloading rules
* graded monad - integrate coproduct into `optional` and `expected` monads, enables extensible `expected` error types
* ... and more

## Acknowledgments

* Gašper Ažman, for providing the inspiration in ["(Fun)ctional C++ and the M-word"][gasper-functional-presentation]
* Bartosz Milewski, for taking the time to explain [parametrised and graded monads][parametrised-and-graded-monads] and [effect systems][effect-systems]
* [Ripple][ripple], for allowing the main author the time to work on this library

[gasper-functional-presentation]: https://youtu.be/Jhggz8rtHbk?si=T-3DXPcvgE_Y5cpH
[parametrised-and-graded-monads]: https://arxiv.org/pdf/2001.10274.pdf
[effect-systems]: https://www.doc.ic.ac.uk/~dorchard/publ/haskell14-effects.pdf
[ripple]: https://ripple.com/
