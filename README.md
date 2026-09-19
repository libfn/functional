# libfn

**Functional programming in C++**

[![Latest Release](https://img.shields.io/github/v/release/libfn/functional?logo=github&color=blue)](https://github.com/libfn/functional/releases/latest)
[![Website](https://img.shields.io/badge/website-libfn.org-darkgreen)](https://libfn.org/)
[![codecov](https://codecov.io/gh/libfn/functional/graph/badge.svg?token=3RHT38SEU0)](https://codecov.io/gh/libfn/functional)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=libfn_functional&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=libfn_functional)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Flibfn%2Ffunctional.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2Flibfn%2Ffunctional?ref=badge_shield)

## Why

This library implements a functional programming layer over standard C++ vocabulary types (such as `std::expected` and `std::optional`), with the goal of proposing successful patterns for future C++ standardization.

## Example

<!-- sync-example-readme -->
```cpp
// Various error types.
enum class NotANumber {};
enum class DivByZero {};
enum class Overflow {};

// Operations on rational numbers.
enum class Add {};
enum class Sub {};
enum class Mul {};
enum class Div {};

// `parse` turns a '/' delimited string into a pair of numbers (a numerator and denominator)
constexpr auto parse(std::string_view s) noexcept
    -> fn::expected<fn::pack<int, int>, fn::copack<NotANumber>>;

class Rational {
  int n_, d_;
  constexpr Rational(int n, int d) noexcept : n_(n), d_(d) {}

public:
  constexpr auto operator==(Rational const &) const noexcept -> bool = default;
  constexpr auto num() const noexcept -> int { return n_; }
  constexpr auto den() const noexcept -> int { return d_; }

  // The invariants live in the type: `make` is the only way to build a `Rational`, and every one is
  // reduced, sign-normalized and representable. Callers receive a value they never need re-check.
  static constexpr struct make_t {
    constexpr auto operator()(long long n, long long d) const noexcept
        -> fn::expected<Rational, fn::copack_for<DivByZero, Overflow>>
    {
      if (d == 0) return fn::unexpected{fn::copack{DivByZero{}}};
      // Note, std::gcd precondition is that `|n|` and `|d|` must both be representable.
      if (n == std::numeric_limits<long long>::min() || d == std::numeric_limits<long long>::min())
        return fn::unexpected{fn::copack{Overflow{}}};

      auto const g = (d < 0 ? -1 : 1) * std::gcd(n, d);
      n /= g;
      d /= g;
      if (n < std::numeric_limits<int>::min() || n > std::numeric_limits<int>::max()
          || d > std::numeric_limits<int>::max()) {
        return fn::unexpected{fn::copack{Overflow{}}};
      }

      return Rational(static_cast<int>(n), static_cast<int>(d));
    }

    constexpr auto operator()(std::string_view s) const noexcept -> decltype(auto)
    {
      return parse(s) | fn::and_then(*this);
    }
  } make{};

  constexpr auto neg() const noexcept -> decltype(auto) { return make(-1LL * n_, d_); }
  constexpr auto inv() const noexcept -> decltype(auto) { return make(d_, n_); }
  constexpr auto add(Rational const &other) const noexcept -> decltype(auto)
  {
    return make(1LL * n_ * other.d_ + 1LL * other.n_ * d_, //
                1LL * d_ * other.d_);
  }
  constexpr auto sub(Rational const &other) const noexcept -> decltype(auto)
  {
    return other.neg() | fn::and_then([*this](Rational y) { return add(y); });
  }
  constexpr auto mul(Rational const &other) const noexcept -> decltype(auto)
  {
    return make(1LL * n_ * other.n_, 1LL * d_ * other.d_);
  }
  constexpr auto div(Rational const &other) const noexcept -> decltype(auto)
  {
    return other.inv() | fn::and_then([*this](Rational y) { return mul(y); });
  }
};

// `evaluate` parses each operand, applies the operator, and lets `make` re-check the result.
// Each stage fails its own way, and the library folds error types into one co-product.
constexpr auto evaluate(std::string_view a, fn::copack_for<Add, Sub, Mul, Div> op,
                        std::string_view b) noexcept -> decltype(auto)
{
  using Op = fn::expected<decltype(op), fn::copack<>>;
  return (Rational::make(a) & Op{op} & Rational::make(b)) //
         | fn::and_then(fn::overload{[](Rational x, Add, Rational y) { return x.add(y); },
                                     [](Rational x, Sub, Rational y) { return x.sub(y); },
                                     [](Rational x, Mul, Rational y) { return x.mul(y); },
                                     [](Rational x, Div, Rational y) { return x.div(y); }});
}

// The error type of a sequence is the derived copack of all failure modes, never spelled by hand:
static_assert(
    std::is_same_v<decltype(Rational::make("1/1")),
                   fn::expected<Rational, fn::copack_for<DivByZero, NotANumber, Overflow>>>);
static_assert(
    std::is_same_v<decltype(evaluate("1/2", Add{}, "3/4")),
                   fn::expected<Rational, fn::copack_for<DivByZero, NotANumber, Overflow>>>);
// Constant evaluated calculations used to verify both values and errors during compilation:
static_assert(evaluate("1/2", Add{}, "1/3").value() == Rational::make(5, 6));
static_assert(evaluate("2/3", Div{}, "0/1").error().has_value<DivByZero>());
```

### What

The library features demonstrated by the code example above:

* **Monadic sequences** — `operator|` pipes an `expected` (or `optional`) through operations: `and_then` and `transform` act on the value, `or_else`, `recover` and `transform_error` on the error, with `filter`, `inspect`, `fail` and more besides.
* **Graded errors** — each stage fails its own way — a malformed string, a zero denominator, an out-of-range result — and the library folds these into one `copack` whose type it derives for you: here `copack<DivByZero, NotANumber, Overflow>`, never spelled by hand.
* **Composing values** — `operator&` gathers successful operands left to right: two values become a `pack`, a third appends to it. A `pack` is a heterogeneous product — the operands as one value, spread into the next call; for example in `make`'s `string_view` overload, where a `pack<int, int>` returned from `parse` is passed to an overload taking two numbers.
* **Composing alternatives** — when a side is a `copack` (a co-product — one of several types, indexed by type, not by position like `std::variant`), `&` distributes over it, pairing every alternative with the other operand. Two copacks yield the full cartesian product. The result type is flattened, deduplicated and sorted for you.
* **Multidispatch** — the pack (or copack of packs) flows into the next stage as separate arguments. An `fn::overload` — or any function — dispatches on the runtime alternative by ordinary overload resolution. Dispatch is exhaustive: a missing handler is a compile error.
* **Identity monad** — `expected<T, copack<>>` cannot hold an error (enforced at compile time), a spelling of the identity monad; the example lifts `op` into it as `Op`.
* **No surprises** — libfn throws no exceptions of its own (only `value()`, as the standard mandates), and composes safely with callables that do; it allocates no memory of its own and performs no I/O. Being fully `constexpr`, it can drive a program evaluated entirely at compile time, where the compiler diagnoses any undefined behaviour.

The example also demonstrates how well libfn works with general programming idioms. `make` is a *smart constructor* — the only way to build a `Rational` — enforcing the type's invariants and returning `expected`: callers never need to re-check what the type guarantees. Treating *callables as values* lets operations such as `and_then` accept `make` whole, carrying its overload set.

Beyond what the code example demonstrates, the library also offers:

* `choice`, a monad over `copack`.
* The same operations that work on `expected` also work on `optional`.
* Simultaneous disjunction, which uses `operator|` to fall back from one monadic computation to another, along with its `fn::disjoin` fold.
* `fn::conjoin`, for simultaneous product folds.
* The tuple protocol in `pack`, so you can write `get<I>(p)` or use structured bindings.
* `pack` and `copack` are both structural types — a `constexpr` value that can be used as a template parameter.
* Support for immovable values and callables.
* An extensible pipeline: a verb defined outside the library pipes exactly like the built-in ones.
* And more — see [examples/](examples/) and the [API reference][docs].

None of this is ad hoc: [TYPE_ALGEBRA.md](TYPE_ALGEBRA.md) derives the entire design from first principles — the algebra of products and sums behind `pack` and `copack`, the logic of monadic composition, and the compiler-checked laws that the library obeys.

## How

The library comes as two parts in one repository:

* **`pfn`** (`include/pfn`, namespace `pfn`) — a faithful polyfill of standard-library vocabulary types as specified for C++26: `std::expected`, `std::optional` (including the monadic functions, `optional<T&>` and range support), plus smaller utilities such as `std::invoke_r` and `std::unreachable`. It adds nothing of its own on top of what's mandated by the [C++ standard](https://eel.is/c++draft/) or accepted for a future revision — such as `has_error()`.
* **`fn`** (`include/fn`, namespace `fn`) — the functional-programming library. It extends the vocabulary types with the facilities useful in writing functional style programs — monadic operations composable with `operator|`, such as `and_then`, `transform`, `or_else`, `inspect`, `recover`, `filter` — and adds new vocabulary types: `copack`, `choice`, `pack`.

Every `fn` type with a `pfn` counterpart is a strict superset of it: switching a valid program using `pfn` types to use `fn` instead changes neither compilation nor program behaviour.

`fn` builds on `pfn`, and all of libfn requires only a C++20-compatible compiler. The minimum supported compilers are [gcc 12][gcc-standard-support] and [clang 19][clang-standard-support]; Apple Clang 21.0 or later and MSVC supplied with Visual Studio 2022 or later are supported as well. For older toolchains, use the [0.1.0 release](https://github.com/libfn/functional/releases/tag/v0.1.0). See [CONTRIBUTING.md](CONTRIBUTING.md) for how to set up a recent enough toolchain when your OS does not ship one.

### Implementation note

This library requires a total ordering of types, which C++26 provides via [`std::type_order`][standardized-type-ordering]. By default, the library uses an internal, naive implementation of type ordering. This internal fallback does not support unnamed types or types without linkage (such as local types or lambdas), and is not portable between GCC and Clang. On compilers implementing C++26 [`std::type_order`][standardized-type-ordering] (such as GCC 16), the opt-in `LIBFN_CXX26` mode uses the standard feature instead. The two modes may order types differently, so `fn` types live in a distinct ABI namespace per mode and the two modes never link as one (`pfn` is mode-independent) — see [CONTRIBUTING.md](CONTRIBUTING.md) for the mode's requirements.

## Using the library

### Packaging

`libfn` is available in the [Bazel Central Registry](https://github.com/bazelbuild/bazel-central-registry/) and [vcpkg registry](https://github.com/microsoft/vcpkg/). This repository also provides packaging for [Conan](conanfile.py), [vcpkg](ports/libfn), [Nix](flake.nix) and [Bazel](MODULE.bazel), all exercised by CI. You can also use CMake's `FetchContent` or `add_subdirectory`.

Every packaging route above except Bazel propagates the library's compile options. With Bazel or a plain copy of `include/`, set them explicitly:

* Select C++20 or newer: `-std=c++20` with GCC or Clang, or `/std:c++20` with MSVC. In Bazel, pass compiler options through `--cxxopt`, for example `--cxxopt=-std=c++20`.
* With Clang and Apple Clang, use `-Wno-missing-braces` to suppress warnings about the intentional brace elision in `fn::pack` initialization.
* With MSVC, use `/permissive-` and `/D_HAS_CXX23=1` (see the [compatibility note](#msvc-compatibility-note) below).

The authoritative set is the `INTERFACE` options in [cmake/CompilationOptions.cmake](cmake/CompilationOptions.cmake).

#### MSVC compatibility note

In MSVC's C++20 mode, `<exception>` includes `<eh.h>`, which declares a global function named `unexpected`. This conflicts with a using-declaration that brings `pfn::unexpected` into the global namespace, so the following code fails to compile:

```cpp
#include <pfn/expected.hpp>

using namespace pfn;

int main() {
  return unexpected(20) == unexpected(19);
}
```

To suppress the legacy declaration, use one of these options:

* Select `/std:c++latest` or, where supported, [`/std:c++23preview`](https://learn.microsoft.com/en-us/cpp/build/reference/std-specify-language-standard-version). With CMake, use `-DCMAKE_CXX_STANDARD=23` to select C++23 mode.
* To keep C++20 mode, define `_HAS_CXX23=1` for the project. The exported CMake targets provide this definition automatically.

### Local install

To install from a source checkout or an unpacked release tarball, run from the project directory:

```sh
cmake -B .build -DLIBFN_TESTS=OFF
cmake --install .build
```

`-DLIBFN_TESTS=OFF` avoids fetching test dependencies. The header-only package needs no build step. On Linux and macOS, the default install prefix is `/usr/local`, so installation may need `sudo`.

To install under your home directory, choose the prefix at install time:

```sh
cmake --install .build --prefix "$HOME/.local"
```

Or set `CMAKE_INSTALL_PREFIX` when configuring:

```sh
cmake -B .build -DLIBFN_TESTS=OFF -DCMAKE_INSTALL_PREFIX="$HOME/.local"
cmake --install .build
```

The installed package supports `find_package(libfn CONFIG REQUIRED)`. For a custom prefix, pass `-DCMAKE_PREFIX_PATH="$HOME/.local"` (or your chosen prefix) when configuring the consuming project.

### Exported targets

The CMake package exports `libfn::fn` and `libfn::pfn`:

```cmake
find_package(libfn CONFIG REQUIRED)
target_link_libraries(main PRIVATE libfn::fn)   # or libfn::pfn for the polyfills alone
```

A third target, `libfn::fn_cxx26`, enters the same headers as `libfn::fn`, but with the [`LIBFN_CXX26` mode](#implementation-note) selected, carrying both the mode and its C++26 language requirement, so a target opts in with a single dependency. Add exactly one of `libfn::fn` **or** `libfn::fn_cxx26` to your project, depending on the available compiler. `libfn::pfn` is mode-independent and has no such variant.

With `libfn::fn_cxx26`, a compiler that does not implement `std::type_order` stops at the first libfn header, with an `#error` naming the feature. Mixing the two entry points in one binary stops at the linker, on an undefined reference whose type names differ from the definition's by the `_cxx26` ABI namespace. Both are loud by design: the namespaces are separate so that two layouts cannot merge unnoticed — the invariant [TYPE_ALGEBRA.md](TYPE_ALGEBRA.md) calls one normalization order per program.

### Single header

A single-header distribution contains the entire library in one file for online compilers and standalone reproducers where include paths cannot be configured.

For regular projects, prefer the separate headers: they give more useful file paths in diagnostics.

The single header is distributed through three channels, whose contracts differ:

* `https://libfn.org/v<x.y.z>/libfn.hpp` ([all versions](https://libfn.org/versions.html)) — one copy per release, immutable once published; [Compiler Explorer][godbolt] can include it directly by URL. This is the URL to pin.
* `libfn-v<x.y.z>.hpp`, attached to each [GitHub release](https://github.com/libfn/functional/releases) — the same file for download, immutable, with signed build provenance: `gh attestation verify libfn-v<x.y.z>.hpp --repo libfn/functional` confirms a downloaded copy is the authentic artifact built by this repository.
* [`https://libfn.org/libfn.hpp`](https://libfn.org/libfn.hpp) — the latest release's copy, moving with each release: convenient in a [throwaway experiment][godbolt_experiment], unusable as a dependency.

In the examples above, use the actual released version instead of `<x.y.z>` (e.g., `0.1.0`). The compile options above apply; to select the C++26 mode, define `LIBFN_CXX26` and compile as C++26 (see also [CONTRIBUTING.md](CONTRIBUTING.md)).

## Backwards compatibility

The maintainers aim for compatibility with the proposed changes to the C++ standard library, **rather than with the existing uses** of the code in this repo. A facility proposed in `include/fn` therefore tracks its paper: names and semantics may change when the paper does. Such a change bumps **`y`**, and so arrives only with a deliberate upgrade.

## Versioning and ABI

Releases are numbered `0.y.z` and will stay below `1.0.0` for the foreseeable future. [SemVer](https://semver.org/) treats any `0.y.z` version as unstable — anything may change — so libfn narrows that into a usable contract:

* a bump in **`y`** is a **breaking** change (API and/or ABI);
* a bump in **`z`** is a bug fix or a purely additive extension: upgrading is **expected** to never break a consumer.

Because the library is header-only, **use a single libfn version per binary**. Mixing versions in one program is an ODR violation — and that includes two `z` releases of the *same* `y` line, whose inline definitions may differ even though the ABI matches.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for the development environment, building, testing, the version-bump mechanics, and the pre-commit workflow. The design history — decisions and the ideas they obsoleted — is recorded in [CHANGELOG.md](CHANGELOG.md).

## Acknowledgments

* Gašper Ažman, for providing the inspiration in ["(Fun)ctional C++ and the M-word"][gasper-functional-presentation]
* Bartosz Milewski, for taking the time to explain [parametrised and graded monads][parametrised-and-graded-monads] and [effect systems][effect-systems]
* [Mykola Golubyev][mykola-golubyev], for implementing fixes in [znai][znai] needed by this project
* [Ripple][ripple], for allowing the main author the time to work on this library

## License

Distributed under the ISC License; see [LICENSE.md](LICENSE.md) for the terms.

[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Flibfn%2Ffunctional.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2Flibfn%2Ffunctional?ref=badge_large)

<!-- link references -->
[docs]: https://libfn.github.io/functional/
[clang-standard-support]: https://clang.llvm.org/cxx_status.html
[gcc-standard-support]: https://gcc.gnu.org/projects/cxx-status.html
[standardized-type-ordering]: https://cppreference.com/cpp/utility/compare/type_order
[gasper-functional-presentation]: https://youtu.be/Jhggz8rtHbk?si=T-3DXPcvgE_Y5cpH
[parametrised-and-graded-monads]: https://arxiv.org/pdf/2001.10274.pdf
[effect-systems]: https://www.doc.ic.ac.uk/~dorchard/publ/haskell14-effects.pdf
[ripple]: https://ripple.com/
[mykola-golubyev]: https://github.com/MykolaGolubyev
[znai]: https://github.com/testingisdocumenting/znai
[godbolt]: https://godbolt.org/
[godbolt_experiment]: https://godbolt.org/z/MrbYTGKv4
