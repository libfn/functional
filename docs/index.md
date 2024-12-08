# functional

Functional programming in C++

## Why

The purpose of this library is to exercise an approach to functional programming in C++ on top of the existing `std` C++ `std::expected` and `std::optional`, with the aim of eventually extending the future versions of the C++ standard library with the functionality found to work well.

## How

The approach is to take the existing `std` types in the C++ standard library (when appropriate) and extend them (via inheritance) with the facilities useful in writing functional style programs. Eventually, the proposed functionality will be (hopefully) folded into the existing `std` types and new `std` types will be added.

## What

The library provides following utilities:

* functors - extensible system of encapsulation of monadic operations, expressed with a pipe `operator |`
* sum of types - coproduct of types, similar to `std::variant` but indexed by type rather than order, composes with the product of types
* choice monad - monad built on top of the coproduct of types, dispatch by overloading rules
* pack - product of types, similar to `std::tuple`, composes with the coproduct of types
* composition - monadic types containing arbitrary types, products or coproducts can be combined with `operator &`
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
