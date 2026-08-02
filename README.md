# libfn

**Functional programming in C++**

[![codecov](https://codecov.io/gh/libfn/functional/graph/badge.svg?token=3RHT38SEU0)](https://codecov.io/gh/libfn/functional)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Flibfn%2Ffunctional.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2Flibfn%2Ffunctional?ref=badge_shield)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=libfn_functional&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=libfn_functional)

## Why

The purpose of this library is to exercise an approach to functional programming in C++ on top of the existing standard vocabulary types (such as `std::expected` and `std::optional`), with the aim of eventually extending future revisions of the C++ standard library with the functionality found to work well.

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

* **Monadic sequences** — `operator|` pipes a `expected` (or `optional`) through operations: `and_then` and `transform` act on the value, `or_else`, `recover` and `transform_error` on the error, with `filter`, `inspect`, `fail` and more besides.
* **Graded errors** — each stage fails its own way — a malformed string, a zero denominator, an out-of-range result — and the library folds these into one `copack` whose type it derives for you: here `copack<DivByZero, NotANumber, Overflow>`, never spelled by hand.
* **Composing values** — `operator&` gathers successful operands left to right: two values become a `pack`, a third appends to it. A `pack` is a heterogeneous product — the operands as one value, spread into the next call; for example in `make`, where a `pack<int, int>` returned from `parse` is passed to an overload taking two numbers.
* **Composing alternatives** — when a side is a `copack` (a co-product — one of several types, indexed by type, not by position like `std::variant`), `&` distributes over it, pairing every alternative with the other operand. Two copacks yield the full cartesian product. The result type is flattened, deduplicated and sorted for you.
* **Multidispatch** — the pack (or copack of packs) flows into the next stage as separate arguments. An `fn::overload` — or any function — dispatches on the runtime alternative by ordinary overload resolution. Dispatch is exhaustive: a missing handler is a compile error.
* **Identity monad** — `expected<T, copack<>>` cannot hold an error (enforced at compile time), a spelling of the identity monad; the example lifts `op` into it as `Op`.
* **No surprises** — libfn throws no exceptions of its own (only `value()`, as the standard mandates), and composes safely with callables that do; it allocates no memory of its own and performs no I/O. Being fully `constexpr`, it can drive a program evaluated entirely at compile time, where the compiler diagnoses any undefined behaviour.

The example also demonstrates how well libfn works with general programming idioms. `make` is a *smart constructor* — the only way to build a `Rational` — enforcing the type's invariants and returning `expected`: callers never need to re-check what the type guarantees. Treating *callables as values* lets operations such as `and_then` accept `make` whole, carrying its overload set.

Beyond the example: `choice` (a monad over `copack`); the same operations over `optional` as over `expected`; simultaneous disjunction (using `operator|` to fallback-combine monadic computations) and its `fn::disjoin` fold; `fn::conjoin` for simultaneous product folds; tuple protocol in `pack` (`get<I>(p)` or structured bindings); `pack` and `copack` are both structural types (a `constexpr` value which may be used as a template parameter); support for immovable values and callables; an extensible pipeline, where a verb defined outside the library pipes exactly like the built-in ones; and more — see [examples/](examples/) and the [API reference][docs].

None of this is ad hoc: [TYPE_ALGEBRA.md](TYPE_ALGEBRA.md) derives the entire design from first principles — the algebra of products and sums behind `pack` and `copack`, the logic of monadic composition, and the compiler-checked laws that the library obeys.

## How

The library comes as two parts in one repository:

* **`pfn`** (`include/pfn`, namespace `pfn`) — a faithful polyfill of standard-library vocabulary types as specified for C++26: `std::expected`, `std::optional` (including the monadic functions, `optional<T&>` and range support), plus smaller utilities such as `std::invoke_r` and `std::unreachable`. It adds nothing of its own on top of what's mandated by the [C++ standard](https://eel.is/c++draft/) or accepted for a future revision — such as `has_error()`.
* **`fn`** (`include/fn`, namespace `fn`) — the functional-programming library. It extends the vocabulary types with the facilities useful in writing functional style programs — monadic operations composable with `operator|`, such as `and_then`, `transform`, `or_else`, `inspect`, `recover`, `filter` — and adds new vocabulary types: `copack`, `choice`, `pack`.

Every `fn` type with a `pfn` counterpart is a strict superset of it: switching a valid program using `pfn` types to use `fn` instead changes neither compilation nor program behaviour.

`fn` builds on `pfn`, and all of libfn requires only a C++20-compatible compiler. The minimum supported compilers are [gcc 12][gcc-standard-support] and [clang 16][clang-standard-support]; Apple Clang 16.0 and Microsoft Visual Studio 2022 (or newer) are supported as well. See [CONTRIBUTING.md](CONTRIBUTING.md) for how to set up a recent enough toolchain when your OS does not ship one.

### Implementation note

This library requires a total ordering of types, which the standard provides from C++26 ([`std::type_order`][standardized-type-ordering]). By default the library relies on an internal, naive implementation of such a feature which is _not expected to work_ with unnamed types, types without linkage etc. On a compiler implementing C++26 [`std::type_order`][standardized-type-ordering] (gcc 16 or newer), the opt-in `LIBFN_CXX26` mode uses the standard feature instead. The two modes may order types differently, so `fn` types live in a distinct ABI namespace per mode and the two modes never link as one (`pfn` is mode-independent) — see [CONTRIBUTING.md](CONTRIBUTING.md) for the mode's requirements.

## Using the library

The library is header-only. The CMake package exports `libfn::fn` and `libfn::pfn`:

```cmake
find_package(libfn CONFIG REQUIRED)
target_link_libraries(main PRIVATE libfn::fn)   # or libfn::pfn for the polyfills alone
```

A third target, `libfn::fn_cxx26`, is the same headers entered with the [`LIBFN_CXX26` mode](#implementation-note) selected; it carries both the mode and its C++26 language requirement, so a target opts in with one link line. Link exactly one of `libfn::fn` or `libfn::fn_cxx26` per target. `pfn` is mode-independent and has no such variant.

With `libfn::fn_cxx26`, a compiler that does not implement `std::type_order` stops at the first libfn header, with an `#error` naming the feature. Mixing the two entry points in one binary stops at the linker, on an undefined reference whose type names differ from the definition's by the `_cxx26` ABI namespace. Both are loud by design: the namespaces are separate so that two layouts cannot merge unnoticed — the invariant [TYPE_ALGEBRA.md](TYPE_ALGEBRA.md) calls one normalization order per program.

Packaging is provided — and exercised by CI — for [conan](conanfile.py), [vcpkg](ports/libfn) (an in-repo port), [Nix](flake.nix) and [Bazel](MODULE.bazel); plain CMake `FetchContent` or `add_subdirectory` works as well. Consume a tagged release — and read [Backwards compatibility](#backwards-compatibility).

Every packaging route above except Bazel also delivers the compile options the headers require. Under Bazel — and a plain copy of `include/` — these options don't arrive automatically; provide them yourself: C++20 or newer (`--cxxopt=-std=c++20` in Bazel), `-Wno-missing-braces` on clang (`fn::pack` initialization elides braces by design), and with MSVC `/permissive-` plus `_HAS_CXX23`. The authoritative set is the `INTERFACE` options in [cmake/CompilationOptions.cmake](cmake/CompilationOptions.cmake).

## Backwards compatibility

The maintainers aim for compatibility with the proposed changes to the C++ standard library, **rather than with the existing uses** of the code in this repo. A facility proposed in `include/fn` therefore tracks its paper: names and semantics may change when the paper does. Such a change bumps **`y`**, and so arrives only with a deliberate upgrade.

## Versioning and ABI

Releases are numbered `0.y.z` and will stay below `1.0.0` for the foreseeable future. [SemVer](https://semver.org/) treats any `0.y.z` version as unstable — anything may change — so libfn narrows that into a usable contract:

- a bump in **`y`** is a **breaking** change (API and/or ABI);
- a bump in **`z`** is a bug fix or a purely additive extension: upgrading never breaks a consumer.

Because the library is header-only, **use a single libfn version per binary**. Mixing versions in one program is an ODR violation — and that includes two `z` releases of the *same* `y` line, whose inline definitions may differ even though the ABI matches.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for the development environment, building, testing, the version-bump mechanics, and the pre-commit workflow. The design history — decisions and the ideas they obsoleted — is recorded in [CHANGELOG.md](CHANGELOG.md).

## Acknowledgments

* Gašper Ažman, for providing the inspiration in ["(Fun)ctional C++ and the M-word"][gasper-functional-presentation]
* Bartosz Milewski, for taking the time to explain [parametrised and graded monads][parametrised-and-graded-monads] and [effect systems][effect-systems]
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
