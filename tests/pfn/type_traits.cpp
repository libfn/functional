// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "catch2/catch_test_macros.hpp"

#ifndef PFN_TEST_NESTED

#include <pfn/type_traits.hpp>

namespace subject = pfn;

#endif
// When nested via PFN_TEST_NESTED (type_traits_validation.cpp), the wrapper TU includes the
// standard header and aliases `subject` to `std`, so the expectations below are checked against
// the standard library's own traits.

#include <type_traits>

// The standard's answer for a class source whose conversion creates the temporary, which the
// emulation does not see: expected wherever the answer comes from the builtin.
#if defined(PFN_TEST_VALIDATION)
constexpr bool exact = true;
#elif defined(_MSC_VER)
constexpr bool exact = false;
#elif __has_builtin(__reference_constructs_from_temporary) && __has_builtin(__reference_converts_from_temporary)
constexpr bool exact = true;
#else
constexpr bool exact = false;
#endif

namespace {
struct Base {};
struct Derived : Base {};
struct Text {
  Text(char const *); // NOLINT(google-explicit-constructor)
};
struct ByValue {
  operator int() const; // NOLINT(google-explicit-constructor)
};
struct ByRef {
  operator int const &() const; // NOLINT(google-explicit-constructor)
};
// Ambiguous only for a target reference to volatile, which a library check would have to ask
struct Ambiguous {
  operator int const &() const;    // NOLINT(google-explicit-constructor)
  operator int volatile &() const; // NOLINT(google-explicit-constructor)
};
struct RvalueOnly {
  operator int const &() &&; // NOLINT(google-explicit-constructor)
};
struct Explicit {
  explicit operator int() const;
};
} // namespace

TEST_CASE("reference_constructs_from_temporary", "[type_traits][polyfill][reference_constructs_from_temporary]")
{
  using subject::reference_constructs_from_temporary;
  using subject::reference_constructs_from_temporary_v;

  SECTION("prvalue")
  {
    SECTION("reference-compatible scalar")
    {
      static_assert(reference_constructs_from_temporary_v<int const &, int>);
      static_assert(reference_constructs_from_temporary_v<int &&, int>);
      SUCCEED();
    }

    SECTION("converted scalar")
    {
      static_assert(reference_constructs_from_temporary_v<int const &, long>);
      static_assert(reference_constructs_from_temporary_v<Text const &, char const *>);
      SUCCEED();
    }

    SECTION("reference-related class")
    {
      static_assert(reference_constructs_from_temporary_v<Base const &, Base>);
      static_assert(reference_constructs_from_temporary_v<Base const &, Derived>);
      static_assert(reference_constructs_from_temporary_v<Base &&, Derived>);
      SUCCEED();
    }

    SECTION("conversion function")
    {
      static_assert(reference_constructs_from_temporary_v<int const &, ByValue> == exact);
      SUCCEED();
    }
  }

  SECTION("xvalue")
  {
    SECTION("reference-compatible scalar")
    {
      static_assert(not reference_constructs_from_temporary_v<int const &, int &&>);
      static_assert(not reference_constructs_from_temporary_v<int &&, int &&>);
      SUCCEED();
    }

    SECTION("converted scalar")
    {
      static_assert(reference_constructs_from_temporary_v<int const &, long &&>);
      static_assert(reference_constructs_from_temporary_v<int &&, long &&>);
      SUCCEED();
    }

    SECTION("reference-related class")
    {
      static_assert(not reference_constructs_from_temporary_v<Base const &, Derived &&>);
      static_assert(not reference_constructs_from_temporary_v<Base &&, Derived &&>);
      SUCCEED();
    }

    SECTION("conversion function")
    {
      static_assert(not reference_constructs_from_temporary_v<int const &, RvalueOnly &&>);
      SUCCEED();
    }
  }

  SECTION("lvalue")
  {
    SECTION("reference-compatible scalar")
    {
      static_assert(not reference_constructs_from_temporary_v<int const &, int &>);
      static_assert(not reference_constructs_from_temporary_v<int const &, int const &>);
      SUCCEED();
    }

    SECTION("converted scalar")
    {
      static_assert(reference_constructs_from_temporary_v<int const &, long &>);
      static_assert(reference_constructs_from_temporary_v<int const &, double const &>);
      SUCCEED();
    }

    SECTION("reference-related class")
    {
      static_assert(not reference_constructs_from_temporary_v<Base const &, Derived &>);
      SUCCEED();
    }

    SECTION("conversion function")
    {
      static_assert(not reference_constructs_from_temporary_v<int const &, ByRef &>);
      static_assert(not reference_constructs_from_temporary_v<int const &, Ambiguous &>);
      static_assert(reference_constructs_from_temporary_v<int const &, ByValue &> == exact);
      SUCCEED();
    }

    SECTION("explicit conversion function")
    {
      // Compilers disagree whether direct-initialization of a reference considers an explicit
      // conversion function; wherever it does, the binding is to a temporary.
      static_assert(reference_constructs_from_temporary_v<int const &, Explicit &>
                    == (exact && std::is_constructible_v<int const &, Explicit &>));
      SUCCEED();
    }
  }

  SECTION("not bindable")
  {
    static_assert(not reference_constructs_from_temporary_v<int &, int>);
    static_assert(not reference_constructs_from_temporary_v<int &, long &>);
    static_assert(not reference_constructs_from_temporary_v<Derived const &, Base>);
    SUCCEED();
  }

  SECTION("non-reference")
  {
    static_assert(not reference_constructs_from_temporary_v<int, int>);
    static_assert(not reference_constructs_from_temporary_v<int, long &>);
    static_assert(not reference_constructs_from_temporary_v<Base, Derived>);
    SUCCEED();
  }

  SECTION("class form")
  {
    static_assert(std::is_base_of_v<std::true_type, reference_constructs_from_temporary<int const &, int>>);
    static_assert(std::is_base_of_v<std::false_type, reference_constructs_from_temporary<int const &, int &>>);
    SUCCEED();
  }
}

TEST_CASE("reference_converts_from_temporary", "[type_traits][polyfill][reference_converts_from_temporary]")
{
  using subject::reference_converts_from_temporary;
  using subject::reference_converts_from_temporary_v;

  SECTION("prvalue")
  {
    SECTION("reference-compatible scalar")
    {
      static_assert(reference_converts_from_temporary_v<int const &, int>);
      static_assert(reference_converts_from_temporary_v<int &&, int>);
      SUCCEED();
    }

    SECTION("converted scalar")
    {
      static_assert(reference_converts_from_temporary_v<int const &, long>);
      static_assert(reference_converts_from_temporary_v<Text const &, char const *>);
      SUCCEED();
    }

    SECTION("reference-related class")
    {
      static_assert(reference_converts_from_temporary_v<Base const &, Base>);
      static_assert(reference_converts_from_temporary_v<Base const &, Derived>);
      static_assert(reference_converts_from_temporary_v<Base &&, Derived>);
      SUCCEED();
    }

    SECTION("conversion function")
    {
      static_assert(reference_converts_from_temporary_v<int const &, ByValue> == exact);
      SUCCEED();
    }
  }

  SECTION("xvalue")
  {
    SECTION("reference-compatible scalar")
    {
      static_assert(not reference_converts_from_temporary_v<int const &, int &&>);
      static_assert(not reference_converts_from_temporary_v<int &&, int &&>);
      SUCCEED();
    }

    SECTION("converted scalar")
    {
      static_assert(reference_converts_from_temporary_v<int const &, long &&>);
      static_assert(reference_converts_from_temporary_v<int &&, long &&>);
      SUCCEED();
    }

    SECTION("reference-related class")
    {
      static_assert(not reference_converts_from_temporary_v<Base const &, Derived &&>);
      static_assert(not reference_converts_from_temporary_v<Base &&, Derived &&>);
      SUCCEED();
    }

    SECTION("conversion function")
    {
      static_assert(not reference_converts_from_temporary_v<int const &, RvalueOnly &&>);
      SUCCEED();
    }
  }

  SECTION("lvalue")
  {
    SECTION("reference-compatible scalar")
    {
      static_assert(not reference_converts_from_temporary_v<int const &, int &>);
      static_assert(not reference_converts_from_temporary_v<int const &, int const &>);
      SUCCEED();
    }

    SECTION("converted scalar")
    {
      static_assert(reference_converts_from_temporary_v<int const &, long &>);
      static_assert(reference_converts_from_temporary_v<int const &, double const &>);
      SUCCEED();
    }

    SECTION("reference-related class")
    {
      static_assert(not reference_converts_from_temporary_v<Base const &, Derived &>);
      SUCCEED();
    }

    SECTION("conversion function")
    {
      static_assert(not reference_converts_from_temporary_v<int const &, ByRef &>);
      static_assert(not reference_converts_from_temporary_v<int const &, Ambiguous &>);
      static_assert(reference_converts_from_temporary_v<int const &, ByValue &> == exact);
      SUCCEED();
    }

    SECTION("explicit conversion function")
    {
      // Copy-initialization never considers an explicit conversion function
      static_assert(not std::is_convertible_v<Explicit &, int const &>);
      static_assert(not reference_converts_from_temporary_v<int const &, Explicit &>);
      SUCCEED();
    }
  }

  SECTION("not bindable")
  {
    static_assert(not reference_converts_from_temporary_v<int &, int>);
    static_assert(not reference_converts_from_temporary_v<int &, long &>);
    static_assert(not reference_converts_from_temporary_v<Derived const &, Base>);
    SUCCEED();
  }

  SECTION("non-reference")
  {
    static_assert(not reference_converts_from_temporary_v<int, int>);
    static_assert(not reference_converts_from_temporary_v<int, long &>);
    static_assert(not reference_converts_from_temporary_v<Base, Derived>);
    SUCCEED();
  }

  SECTION("class form")
  {
    static_assert(std::is_base_of_v<std::true_type, reference_converts_from_temporary<int const &, int>>);
    static_assert(std::is_base_of_v<std::false_type, reference_converts_from_temporary<int const &, int &>>);
    SUCCEED();
  }
}

#ifndef PFN_TEST_VALIDATION

namespace {
// The emulation answers for the public traits only where the compiler lacks the builtin; naming it
// directly runs it on every compiler. Its one deliberate divergence from the standard is the miss
// marked below: never a temporary reported where none is created.
template <typename T, typename U>
constexpr bool emulated = [] {
  constexpr bool constructs = pfn::detail::_emulated_from_temporary<T, U, false>();
  static_assert(constructs == pfn::detail::_emulated_from_temporary<T, U, true>());
  return constructs;
}();
} // namespace

TEST_CASE("reference_from_temporary emulation", "[type_traits][polyfill][emulation]")
{
  SECTION("prvalue")
  {
    static_assert(emulated<int const &, int>);
    static_assert(emulated<int &&, int>);
    static_assert(emulated<int const &, long>);
    static_assert(emulated<Text const &, char const *>);
    static_assert(emulated<Base const &, Base>);
    static_assert(emulated<Base const &, Derived>);
    static_assert(emulated<Base &&, Derived>);
    static_assert(not emulated<int const &, ByValue>); // the standard's answer is true
    SUCCEED();
  }

  SECTION("xvalue")
  {
    static_assert(not emulated<int const &, int &&>);
    static_assert(not emulated<int &&, int &&>);
    static_assert(emulated<int const &, long &&>);
    static_assert(emulated<int &&, long &&>);
    static_assert(not emulated<Base const &, Derived &&>);
    static_assert(not emulated<Base &&, Derived &&>);
    static_assert(not emulated<int const &, RvalueOnly &&>);
    SUCCEED();
  }

  SECTION("lvalue")
  {
    static_assert(not emulated<int const &, int &>);
    static_assert(not emulated<int const &, int const &>);
    static_assert(emulated<int const &, long &>);
    static_assert(emulated<int const &, double const &>);
    static_assert(not emulated<Base const &, Derived &>);
    static_assert(not emulated<int const &, ByRef &>);
    static_assert(not emulated<int const &, Ambiguous &>);
    static_assert(not emulated<int const &, ByValue &>); // the standard's answer is true
    SUCCEED();
  }

  SECTION("not bindable")
  {
    static_assert(not emulated<int &, int>);
    static_assert(not emulated<int &, long &>);
    static_assert(not emulated<Derived const &, Base>);
    SUCCEED();
  }

  SECTION("non-reference")
  {
    static_assert(not emulated<int, int>);
    static_assert(not emulated<int, long &>);
    static_assert(not emulated<Base, Derived>);
    SUCCEED();
  }
}

#endif // PFN_TEST_VALIDATION
