# functional

Functional programming in C++

[![codecov](https://codecov.io/gh/libfn/functional/graph/badge.svg?token=3RHT38SEU0)](https://codecov.io/gh/libfn/functional)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Flibfn%2Ffunctional.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2Flibfn%2Ffunctional?ref=badge_shield)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=libfn_functional&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=libfn_functional)

## Why

The purpose of this library is to exercise an approach to functional programming in C++ on top of the existing `std` C++ vocabulary types (such as `std::expected` and `std::optional`), with the aim of eventually extending the future versions of the C++ standard library with the functionality found to work well.

## Example

```cpp
#include <fn/and_then.hpp>
#include <fn/utility.hpp>

#include <climits>
#include <numeric>
#include <type_traits>

struct DivByZero {};
struct Overflow {};
struct Add {};
struct Mul {};
enum Side { num, den };
class Rational {
  int num_, den_;
  constexpr Rational(int n, int d) noexcept : num_(n), den_(d) {}

public:
  constexpr bool operator==(Rational const &) const noexcept = default;
  template <Side S> constexpr friend auto get(Rational const &r) noexcept -> int { return S == num ? r.num_ : r.den_; }
  // The invariant lives in the type: `make` is the only way to build one, so every Rational is reduced,
  // sign-normalized and representable — callers receive a value they never have to re-check.
  static auto make(long long n, long long d) -> fn::expected<Rational, fn::sum_for<DivByZero, Overflow>>
  {
    if (d == 0) return pfn::unexpected{fn::sum{DivByZero{}}};
    if (n == LLONG_MIN || d == LLONG_MIN) return pfn::unexpected{fn::sum{Overflow{}}};
    auto const g = (d < 0 ? -1 : 1) * std::gcd(n, d);
    n /= g;
    d /= g;
    if (n < INT_MIN || n > INT_MAX || d > INT_MAX) return pfn::unexpected{fn::sum{Overflow{}}};
    return Rational(int(n), int(d));
  }
};
// `evaluate` trusts its operands — the type guarantees them, so it never revalidates. The int64 intermediates
// cannot overflow for int32 fields, and `make` rejects any result that will not fit back, folding Overflow in:
auto evaluate(fn::expected<Rational, fn::sum_for<DivByZero, Overflow>> a, fn::sum_for<Add, Mul> op,
              fn::expected<Rational, fn::sum_for<DivByZero, Overflow>> b)
{
  return (a & b) | fn::and_then([op](Rational x, Rational y) {
           return op.invoke(fn::overload{
               [&](Add) {
                 return Rational::make(1LL * get<num>(x) * get<den>(y) + 1LL * get<num>(y) * get<den>(x),
                                       1LL * get<den>(x) * get<den>(y));
               },
               [&](Mul) { return Rational::make(1LL * get<num>(x) * get<num>(y), 1LL * get<den>(x) * get<den>(y)); }});
         });
}
// The library composes the pipeline type — a Rational, over the sum of every way construction can fail:
static_assert(std::is_same_v<decltype(evaluate(Rational::make(0, 1), Add{}, Rational::make(0, 1))),
                             fn::expected<Rational, fn::sum<DivByZero, Overflow>>>);
```

### What

The library features demonstrated by the code example above:

* **Smart constructors** — `Rational`'s constructor is private; the only way to build one is the static `make`, which enforces the invariant (reduced, representable) and returns `fn::expected`. An invalid `Rational` cannot exist, so callers stop re-checking what the type already guarantees.
* **Monadic pipelines** — `operator|` pipes `fn::expected` (and `fn::optional`) through operations: `and_then` above; also `or_else`, `transform`, `filter`, `inspect`, `recover`, `fail`, `transform_error` and more.
* **Composition** — `operator&` accumulates several monadic values into one: the two operands arrive at the next operation as separate arguments, courtesy of `fn::pack`.
* **Graded errors** — each way a step can fail contributes its own error type, and the library widens them into a single `fn::sum` — here `fn::sum<DivByZero, Overflow>`, the whole pipeline's error, composed by the library and never written by hand (pinned by a `static_assert`).
* **Multidispatch** — `fn::sum` is indexed by type, not by position like `std::variant`, and `fn::overload` dispatches over its alternatives (`Add` and `Mul`) by ordinary overload resolution — exhaustively, so a missing handler is a compile error.

Beyond the example: `fn::choice` (a monad over `fn::sum`), dispatch across any combination of `fn::pack` and `fn::sum`, and more — see [examples/](examples/) and the [API reference][docs].

## How

The library comes as two parts in one repository:

* **`pfn`** (`include/pfn`, namespace `pfn`) — a faithful polyfill of standard-library vocabulary types: `std::expected` (as specified for C++26, including `has_error()`) and `std::optional` (as specified for C++26, including monadic functions, `optional<T&>` and range support), plus smaller utilities such as `std::invoke_r` and `std::unreachable`. It adds nothing of its own on top of what's mandated by the [C++ standard](https://eel.is/c++draft/).
* **`fn`** (`include/fn`, namespace `fn`) — the functional-programming library. It extends the vocabulary types with the facilities useful in writing functional style programs — monadic operations composable with `operator|`, such as `and_then`, `transform`, `or_else`, `inspect`, `recover`, `filter` — and adds new vocabulary types: `sum`, `choice`, `pack`.

Every `fn` type with a `pfn` counterpart is a strict superset of it: switching a valid program using `pfn` types to use `fn` instead changes neither compilation nor program behaviour.

`fn` builds on `pfn`, and all of libfn requires only a C++20-compatible compiler. The minimum supported compilers are [gcc 12][gcc-standard-support] and [clang 16][clang-standard-support]; Apple Clang 16.0 and Microsoft Visual Studio 2022 (or newer) are supported as well. See [CONTRIBUTING.md](CONTRIBUTING.md) for how to set up a recent enough toolchain when your OS does not ship one.

### Implementation note

This library requires a total ordering of types, which the standard provides only from C++26 ([`std::type_order`][standardized-type-ordering]) and no compiler implements yet. The library relies on an internal, naive implementation of such a feature which is _not expected to work_ with unnamed types, types without linkage etc. When [`std::type_order`][standardized-type-ordering] is implemented in available compilers, the library will offer a compilation mode to make use of this feature.

## Using the library

The library is header-only. The CMake package exports two targets:

```cmake
find_package(libfn CONFIG REQUIRED)
target_link_libraries(main PRIVATE libfn::fn)   # or libfn::pfn for the polyfills alone
```

Packaging is provided — and exercised by CI — for [conan](conanfile.py), [vcpkg](ports/libfn) (an in-repo port), [Nix](flake.nix) and [Bazel](MODULE.bazel); plain CMake `FetchContent` or `add_subdirectory` works as well. Until the first tagged release, consume a pinned git revision — and read [Backwards compatibility](#backwards-compatibility).

Every packaging route above except Bazel also delivers the compile options the headers require. Under Bazel — and a plain copy of `include/` — these options don't arrive automatically; provide them yourself: C++20 or newer (`--cxxopt=-std=c++20` in Bazel), `-Wno-missing-braces` on clang (`fn::pack` initialization elides braces by design), and with MSVC `/permissive-` plus `_HAS_CXX23`. The authoritative set is the `INTERFACE` options in [cmake/CompilationOptions.cmake](cmake/CompilationOptions.cmake).

## Backwards compatibility

The maintainers will aim to maintain compatibility with the proposed changes in the C++ standard library, **rather than with the existing uses** of the code in this repo. In practice, this means that all code in this repo should be considered "under intensive development and unstable" until the standardization of the proposed facilities.

## Versioning and ABI

Releases are numbered `0.y.z` and will stay below `1.0.0` for the foreseeable future. [SemVer](https://semver.org/) treats any `0.y.z` version as unstable — anything may change — so libfn narrows that into a usable contract:

- a bump in **`y`** is a **breaking** change (API and/or ABI);
- a bump in **`z`** is a bug fix or a purely additive extension: the API and ABI stay compatible, but inline function definitions may change — see below.

Because the library is header-only, **use a single libfn version per binary**. Mixing versions in one program is an ODR violation — and that includes two `z` releases of the *same* `y` line, whose inline definitions may differ even though the ABI matches.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for the development environment, building, testing, the version-bump mechanics, and the pre-commit workflow. The design history — decisions and the ideas they obsoleted — is recorded in [CHANGELOG.md](CHANGELOG.md).

## Acknowledgments

* Gašper Ažman, for providing the inspiration in ["(Fun)ctional C++ and the M-word"][gasper-functional-presentation]
* Bartosz Milewski, for taking the time to explain [parametrised and graded monads][parametrised-and-graded-monads] and [effect systems][effect-systems]
* [Ripple][ripple], for allowing the main author the time to work on this library

## License

[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Flibfn%2Ffunctional.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2Flibfn%2Ffunctional?ref=badge_large)

<!-- link references -->
[docs]: https://libfn.github.io/functional/
[clang-standard-support]: https://clang.llvm.org/cxx_status.html
[gcc-standard-support]: https://gcc.gnu.org/projects/cxx-status.html
[standardized-type-ordering]: https://wg21.link/P2830
[gasper-functional-presentation]: https://youtu.be/Jhggz8rtHbk?si=T-3DXPcvgE_Y5cpH
[parametrised-and-graded-monads]: https://arxiv.org/pdf/2001.10274.pdf
[effect-systems]: https://www.doc.ic.ac.uk/~dorchard/publ/haskell14-effects.pdf
[ripple]: https://ripple.com/
