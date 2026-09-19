# libfn

**Functional programming in C++**

[![Latest Release](https://img.shields.io/github/v/release/libfn/functional?logo=github&color=blue)](https://github.com/libfn/functional/releases/latest)
[![Website](https://img.shields.io/badge/website-libfn.org-darkgreen)](https://libfn.org/)
[![codecov](https://codecov.io/gh/libfn/functional/graph/badge.svg?token=3RHT38SEU0)](https://codecov.io/gh/libfn/functional)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=libfn_functional&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=libfn_functional)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Flibfn%2Ffunctional.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2Flibfn%2Ffunctional?ref=badge_shield)

## Why

`libfn` extends C++ vocabulary types such as `expected` and `optional` and adds monadic carrier types `choice` and `just`. It provides monadic combinators such as `and_then`, `transform` and `or_else`, along with other functional facilities. Its purpose is to develop and test functional programming patterns for future C++ standardization.

## Example

This example parses rational numbers and applies arithmetic operations. Each operation returns an `expected` whose error type describes its possible failures. The [complete example](examples/readme/main.cpp) includes the parser and required headers.

<!-- sync-example-readme -->
```cpp
enum class NotANumber {};
enum class DivByZero {};
enum class Overflow {};

enum class Add {};
enum class Sub {};
enum class Mul {};
enum class Div {};

// Parse a numerator and optional denominator; without '/', the denominator is 1.
constexpr auto parse(std::string_view s) noexcept
    -> fn::expected<fn::pack<int, int>, fn::copack<NotANumber>>;

class Rational {
  int n_, d_;
  constexpr Rational(int n, int d) noexcept : n_(n), d_(d) {}

public:
  constexpr auto operator==(Rational const &) const noexcept -> bool = default;
  constexpr auto num() const noexcept -> int { return n_; }
  constexpr auto den() const noexcept -> int { return d_; }

  // Construct a reduced fraction with a positive denominator and both terms representable as int.
  static constexpr struct make_t {
    constexpr auto operator()(long long n, long long d) const noexcept
        -> fn::expected<Rational, fn::copack_for<DivByZero, Overflow>>
    {
      if (d == 0) return fn::unexpected{fn::copack{DivByZero{}}};
      // std::gcd requires both |n| and |d| to be representable as long long.
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

// Combine parsing and arithmetic errors in the deduced result type.
constexpr auto evaluate(std::string_view a, fn::copack_for<Add, Sub, Mul, Div> op,
                        std::string_view b) noexcept -> decltype(auto)
{
  auto const operation = fn::expected_unit{}.transform([op] { return op; });
  return (Rational::make(a) & operation & Rational::make(b)) //
         | fn::and_then(fn::overload{[](Rational x, Add, Rational y) { return x.add(y); },
                                     [](Rational x, Sub, Rational y) { return x.sub(y); },
                                     [](Rational x, Mul, Rational y) { return x.mul(y); },
                                     [](Rational x, Div, Rational y) { return x.div(y); }});
}

// Both results include every error type from their stages.
static_assert(
    std::is_same_v<decltype(Rational::make("1/1")),
                   fn::expected<Rational, fn::copack_for<DivByZero, NotANumber, Overflow>>>);
static_assert(
    std::is_same_v<decltype(evaluate("1/2", Add{}, "3/4")),
                   fn::expected<Rational, fn::copack_for<DivByZero, NotANumber, Overflow>>>);
// Check a successful calculation and division by zero at compile time.
static_assert(evaluate("1/2", Add{}, "1/3").value() == Rational::make(5, 6));
static_assert(evaluate("2/3", Div{}, "0/1").error().has_value<DivByZero>());
```

### What

The example combines several library features:

* **Monadic sequences** — `operator|` connects a result to a combinator such as `fn::and_then(f)`, which calls `f` on success and propagates errors otherwise.
* **Graded errors** — parsing and arithmetic can fail in different ways. The library combines their error types into a `copack`; here, the result uses `copack_for<DivByZero, NotANumber, Overflow>`. Callers do not need to combine the error types by hand.
* **Composing values** — `operator&` combines successful operands into a `pack`, preserving their order. A `pack` holds values of different types and passes them as separate arguments to the next call. For example, `parse` returns a `pack<int, int>` that `and_then` passes to the two-argument overload of `make`.
* **Composing alternatives** — a `copack` holds one of several types, identified by type rather than by position (as in `std::variant`). When a value side is a `copack`, `&` pairs each alternative with the other operand. Two copacks produce all combinations of alternatives; the library flattens, deduplicates and sorts the resulting alternative types.
* **Multidispatch** — `fn::overload` selects an arithmetic operation for the active alternative of `op`. Each handler receives the operands as separate arguments. Every possible alternative needs a matching handler, or compilation fails.
* **Identity carrier** — `expected<T, copack<>>` cannot hold a failure state. The example starts with `fn::expected_unit`, an alias for `expected<void, copack<>>`, and uses its `.transform` member to supply `op` as the value without adding a failure mode.

`make` is a *smart constructor*: it returns either a valid `Rational` or an error. A successful result is reduced, has a positive denominator, and fits in two `int` values. Because `make` is a callable object, `and_then` can accept it with both overloads intact.

The library also provides:

* Combinators for transforming, inspecting and recovering results, including `transform`, `inspect`, `or_else` and `recover`.
* Pipelines over `optional`.
* The `choice` identity carrier for applying monadic operations to a `copack`.
* The `just` identity carrier.
* Simultaneous disjunction: `operator|` selects a successful operand or combines the errors.
* `fn::disjoin` and `fn::conjoin` fold disjunction and conjunction over multiple operands.
* Tuple access for `pack`, including `get<I>(p)` and structured bindings.
* Structural `pack` and `copack` types: when their element types meet the requirements, their values can be template arguments.
* Support for immovable values and callables, and for user-defined combinators.

`libfn` performs no I/O and makes no dynamic allocations of its own. Its only explicit exception path is `value()` on a result without a value (as required by the C++ standard). Contained types and callables may allocate or throw. `libfn` does not leak resources it manages when user code throws. Selected operations, including copack assignment, provide the strong exception guarantee: if the operation throws, the destination retains its previous value. Operations support constant evaluation when their inputs and callables allow it, as the example's `static_assert`s demonstrate.

See [examples/](examples/) and the [API reference][docs] for more. [TYPE_ALGEBRA.md](TYPE_ALGEBRA.md) explains the products and sums behind `pack` and `copack`, how the monadic combinators compose, and the laws exercised by the tests.

## How

The library has two layers:

* **`pfn`** (`include/pfn`, namespace `pfn`) provides C++20 polyfills for standard-library facilities through C++26: `expected`, `optional` (including monadic operations, `optional<T&>` and range support), `invoke_r` and `unreachable`. These follow the C++ standard and accepted proposals, including `has_error()`.
* **`fn`** (`include/fn`, namespace `fn`) builds on those polyfills. It adds combinators such as `and_then`, `transform` and `recover`, along with the vocabulary types `pack`, `copack` and `choice`.

The `fn` types extend their `pfn` counterparts: switching a valid program from `pfn` types to `fn` changes neither compilation nor program behaviour, while making the types and operations defined in `fn` available.

The default mode requires C++20. Supported compilers are [GCC 12][gcc-standard-support] or later, [Clang 19][clang-standard-support] or later, Apple Clang 21.0 or later, and MSVC from Visual Studio 2022 or later. For older toolchains, use the [0.1.0 release](https://github.com/libfn/functional/releases/tag/v0.1.0). [CONTRIBUTING.md](CONTRIBUTING.md) explains how to set up a supported toolchain.

### Implementation note

The library needs a total ordering of types to normalize `copack` alternatives. Its default implementation does not support unnamed types or types without linkage, such as local types and lambdas. The ordering can also differ between GCC and Clang.

The opt-in `LIBFN_CXX26` mode uses C++26's [`std::type_order`][standardized-type-ordering] and requires a compiler that implements it, such as GCC 16. Because the modes can order alternatives differently, each gives `fn` types a distinct ABI namespace. `pfn` is independent of this setting. See [CONTRIBUTING.md](CONTRIBUTING.md) for the mode's requirements.

## Using the library

### Packaging

`libfn` is available in the [Bazel Central Registry](https://github.com/bazelbuild/bazel-central-registry/) and [vcpkg registry](https://github.com/microsoft/vcpkg/). This repository also provides packaging for [Conan](conanfile.py), [vcpkg](ports/libfn), [Nix](flake.nix) and [Bazel](MODULE.bazel), all exercised by CI. You can also use CMake's `FetchContent` or `add_subdirectory`.

Every packaging route above except Bazel propagates the library's compile options. With Bazel or a plain copy of `include/`, set them explicitly:

* Select C++20 or newer: `-std=c++20` with GCC or Clang, or `/std:c++20` with MSVC. In Bazel, pass compiler options through `--cxxopt`, for example `--cxxopt=-std=c++20`.
* With Clang and Apple Clang, use `-Wno-missing-braces` to suppress warnings about the intentional brace elision in `fn::pack` initialization.
* With MSVC, use `/permissive-` and `/D_HAS_CXX23=1` (see the [compatibility note](#msvc-compatibility-note) below).

The authoritative set is the `INTERFACE` options in [cmake/CompilationOptions.cmake](cmake/CompilationOptions.cmake).

#### MSVC compatibility note

In MSVC's C++20 mode, `<exception>` includes `<eh.h>`, which declares a global function named `unexpected`. With `using namespace pfn`, an unqualified use of `unexpected` is ambiguous, so the following code fails to compile:

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

`-DLIBFN_TESTS=OFF` avoids fetching test dependencies. The header-only package needs no build step. On Linux and macOS, the *default install prefix* is `/usr/local`, so installation may need `sudo`.

To install under your home directory, choose the *custom prefix* at install time:

```sh
cmake --install .build --prefix "$HOME/.local"
```

Or set `CMAKE_INSTALL_PREFIX` when configuring:

```sh
cmake -B .build -DLIBFN_TESTS=OFF -DCMAKE_INSTALL_PREFIX="$HOME/.local"
cmake --install .build
```

The installed package supports `find_package(libfn CONFIG REQUIRED)`. For a *custom prefix*, pass `-DCMAKE_PREFIX_PATH="$HOME/.local"` (or your chosen prefix) when configuring the consuming project.

### Exported targets

The CMake package exports `libfn::fn` and `libfn::pfn`:

```cmake
find_package(libfn CONFIG REQUIRED)
target_link_libraries(main PRIVATE libfn::fn)   # or libfn::pfn for the polyfills alone
```

For [`LIBFN_CXX26` mode](#implementation-note), use `libfn::fn_cxx26` in place of `libfn::fn`. It supplies the same headers, defines `LIBFN_CXX26`, and selects C++26. A consumer should link exactly one of these two targets. `libfn::pfn` has no separate C++26 target.

If the compiler lacks `std::type_order`, a header diagnostic names the missing feature. Keep the same mode across code that exchanges `fn` types: their ABI namespaces differ. For example, a function declared with an `fn` parameter in one mode will not link to a definition using the other mode.

### Single header

A single-header distribution contains the entire library in one file for online compilers and standalone reproducers where include paths cannot be configured.

*For regular projects, prefer the separate headers: they give more useful file paths in diagnostics.*

Choose a download according to whether you need a fixed version:

* **Versioned URL:** `https://libfn.org/v<x.y.z>/libfn.hpp` ([all versions](https://libfn.org/versions.html)). Each release keeps its own copy. [Compiler Explorer][godbolt] can include it directly by URL.
* **Release attachment:** `libfn-v<x.y.z>.hpp` on each [GitHub release](https://github.com/libfn/functional/releases). Verify its build provenance with `gh attestation verify libfn-v<x.y.z>.hpp --repo libfn/functional`.
* **Latest release:** [`https://libfn.org/libfn.hpp`](https://libfn.org/libfn.hpp). This URL changes with each release; use it for [experiments][godbolt_experiment], and pin a version when you need reproducible builds.

Replace `<x.y.z>` with a released version, such as `0.1.0`. The [compile options](#packaging) also apply to the single header. To select C++26 mode, define `LIBFN_CXX26` and compile as C++26.

## Backwards compatibility

Facilities in `include/fn` that track a C++ proposal may change names or semantics as the proposal evolves. Breaking changes increase the minor version (`y` in `0.y.z`), so pin that version when adopting the library.

## Versioning and ABI

Releases numbered `0.y.z` (including 0.1.0) are *mature releases*, not alpha versions or prereleases. Releases are expected to remain below 1.0.0 for the foreseeable future because C++ standardization may continue to reshape the API. Within [SemVer](https://semver.org/)’s `0.y.z` series, `libfn` uses the following compatibility policy:

* A change to **`y`** marks an API or ABI break.
* A change to **`z`** contains fixes or additions and is **expected** to preserve compatibility.

Use a single `libfn` version per binary to minimize compatibility risks, including One Definition Rule (ODR) violations. Patch releases are **intended, but not guaranteed**, to **preserve API and ABI compatibility**. We cannot test every valid use of the library, so users remain responsible for ensuring that their dependencies use a consistent version.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for development setup, builds, tests, version updates and pre-commit checks. [CHANGELOG.md](CHANGELOG.md) records the design decisions and release history.

## Acknowledgments

* Gašper Ažman, whose ["(Fun)ctional C++ and the M-word"][gasper-functional-presentation] inspired this library.
* Bartosz Milewski, for explaining [parametrised and graded monads][parametrised-and-graded-monads] and [effect systems][effect-systems].
* [Mykola Golubyev][mykola-golubyev], for fixes to [znai][znai] that this project needed.
* [Ripple][ripple], for giving the main author time to work on the library.

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
[godbolt_experiment]: https://godbolt.org/z/bbEcbdoG5
