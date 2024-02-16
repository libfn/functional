// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/detail/sum_storage.hpp"
#include "functional/utility.hpp"

#include <catch2/catch_all.hpp>

#include <bitset>
#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

TEST_CASE("sum_storage", "[sum_storage]")
{
  // NOTE We have 5 different specializations, need to test each

  using namespace fn::detail;
  constexpr auto fn1 = [](auto) {};
  constexpr auto fn2 = [](auto...) {};
  constexpr auto fn3 = [](int &) {};
  constexpr auto fn4 = [](std::in_place_type_t<int>, int &) {};

  WHEN("size 1")
  {
    sum_storage<int> a{std::in_place_type<int>, 42};
    CHECK(a.v0 == 42);
    static_assert(a.size == 1);
    static_assert(std::is_same_v<sum_storage<int>::template type<0>, int>);

    static_assert(decltype(a)::invocable<decltype(fn1), decltype(a)>);
    static_assert(decltype(a)::invocable<decltype(fn2), decltype(a)>);
    static_assert(not decltype(a)::invocable_typed<decltype(fn1), decltype(a)>);
    static_assert(decltype(a)::invocable<decltype(fn2), decltype(a)>);
    static_assert(decltype(a)::invocable<decltype(fn3), decltype(a) &>);
    static_assert(decltype(a)::invocable_typed<decltype(fn4), decltype(a) &>);
    static_assert(not decltype(a)::invocable<decltype(fn3), decltype(a) const &>);
    static_assert(not decltype(a)::invocable_typed<decltype(fn4), decltype(a) const &>);
    static_assert(not decltype(a)::invocable<decltype(fn3), decltype(a)>);
    static_assert(not decltype(a)::invocable_typed<decltype(fn4), decltype(a)>);
    static_assert(not decltype(a)::invocable<decltype(fn3), decltype(a) const>);
    static_assert(not decltype(a)::invocable_typed<decltype(fn4), decltype(a) const>);
    static_assert(not decltype(a)::invocable<decltype(fn3), decltype(a) &&>);
    static_assert(not decltype(a)::invocable_typed<decltype(fn4), decltype(a) &&>);
    static_assert(not decltype(a)::invocable<decltype(fn3), decltype(a) const &&>);
    static_assert(not decltype(a)::invocable_typed<decltype(fn4), decltype(a) const &&>);

    static_assert(std::is_same_v<decltype(a.get_ptr(std::in_place_type<int>)), int *>);
    CHECK(a.get_ptr(std::in_place_type<int>) == &a.v0);
    static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(std::in_place_type<int>)), int const *>);
    CHECK(std::as_const(a).get_ptr(std::in_place_type<int>) == &a.v0);

    constexpr auto a1 = sum_storage<int>{std::in_place_type<int>, 12};
    static_assert(*a1.get_ptr(std::in_place_type<int>) == 12);
  }

  WHEN("size 2")
  {
    using type = sum_storage<int, double>;
    static_assert(type::size == 2);
    static_assert(std::is_same_v<type::template type<0>, int>);
    static_assert(std::is_same_v<type::template type<1>, double>);

    WHEN("element v0 set")
    {
      constexpr auto element = std::in_place_type<int>;
      type a{element, 42};
      CHECK(a.v0 == 42);

      static_assert(decltype(a)::invocable<decltype(fn1), decltype(a)>);
      static_assert(decltype(a)::invocable<decltype(fn2), decltype(a)>);
      static_assert(not decltype(a)::invocable_typed<decltype(fn1), decltype(a)>);
      static_assert(decltype(a)::invocable<decltype(fn2), decltype(a)>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), int *>);
      CHECK(a.get_ptr(element) == &a.v0);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), int const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.v0);
    }

    WHEN("element v1 set")
    {
      constexpr auto element = std::in_place_type<double>;
      type a{element, 0.5};
      CHECK(a.v1 == 0.5);

      constexpr auto fn5 = [](auto &) {};
      constexpr auto fn6 = [](some_in_place_type auto, auto...) {};
      static_assert(decltype(a)::invocable<decltype(fn5), decltype(a) &>);
      static_assert(decltype(a)::invocable_typed<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), double *>);
      CHECK(a.get_ptr(element) == &a.v1);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), double const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.v1);
    }
  }

  WHEN("size 3")
  {
    using type = sum_storage<int, double, std::string_view>;
    static_assert(type::size == 3);
    static_assert(std::is_same_v<type::template type<0>, int>);
    static_assert(std::is_same_v<type::template type<1>, double>);
    static_assert(std::is_same_v<type::template type<2>, std::string_view>);

    WHEN("element v0 set")
    {
      constexpr auto element = std::in_place_type<int>;
      type a{element, 42};
      CHECK(a.v0 == 42);

      static_assert(decltype(a)::invocable<decltype(fn1), decltype(a)>);
      static_assert(decltype(a)::invocable<decltype(fn2), decltype(a)>);
      static_assert(not decltype(a)::invocable_typed<decltype(fn1), decltype(a)>);
      static_assert(decltype(a)::invocable<decltype(fn2), decltype(a)>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), int *>);
      CHECK(a.get_ptr(element) == &a.v0);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), int const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.v0);
    }

    WHEN("element v1 set")
    {
      constexpr auto element = std::in_place_type<double>;
      type a{element, 0.5};
      CHECK(a.v1 == 0.5);

      constexpr auto fn5 = [](auto &) {};
      constexpr auto fn6 = [](some_in_place_type auto, auto...) {};
      static_assert(decltype(a)::invocable<decltype(fn5), decltype(a) &>);
      static_assert(decltype(a)::invocable_typed<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), double *>);
      CHECK(a.get_ptr(element) == &a.v1);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), double const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.v1);
    }

    WHEN("element v2 set")
    {
      constexpr auto element = std::in_place_type<std::string_view>;
      type a{element, "baz"};
      CHECK(a.v2 == "baz");

      constexpr auto fn5 = [](auto &) {};
      constexpr auto fn6 = [](some_in_place_type auto, auto...) {};
      static_assert(decltype(a)::invocable<decltype(fn5), decltype(a) &>);
      static_assert(decltype(a)::invocable_typed<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), std::string_view *>);
      CHECK(a.get_ptr(element) == &a.v2);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), std::string_view const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.v2);
    }
  }

  WHEN("size 4")
  {
    using type = sum_storage<int, double, std::string_view, std::string>;
    static_assert(type::size == 4);
    static_assert(std::is_same_v<type::template type<0>, int>);
    static_assert(std::is_same_v<type::template type<1>, double>);
    static_assert(std::is_same_v<type::template type<2>, std::string_view>);
    static_assert(std::is_same_v<type::template type<3>, std::string>);

    WHEN("element v0 set")
    {
      constexpr auto element = std::in_place_type<int>;
      type a{element, 42};
      CHECK(a.v0 == 42);

      static_assert(decltype(a)::invocable<decltype(fn1), decltype(a)>);
      static_assert(decltype(a)::invocable<decltype(fn2), decltype(a)>);
      static_assert(not decltype(a)::invocable_typed<decltype(fn1), decltype(a)>);
      static_assert(decltype(a)::invocable<decltype(fn2), decltype(a)>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), int *>);
      CHECK(a.get_ptr(element) == &a.v0);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), int const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.v0);
    }

    WHEN("element v1 set")
    {
      constexpr auto element = std::in_place_type<double>;
      type a{element, 0.5};
      CHECK(a.v1 == 0.5);

      constexpr auto fn5 = [](auto &) {};
      constexpr auto fn6 = [](some_in_place_type auto, auto...) {};
      static_assert(decltype(a)::invocable<decltype(fn5), decltype(a) &>);
      static_assert(decltype(a)::invocable_typed<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), double *>);
      CHECK(a.get_ptr(element) == &a.v1);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), double const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.v1);
    }

    WHEN("element v2 set")
    {
      constexpr auto element = std::in_place_type<std::string_view>;
      type a{element, "baz"};
      CHECK(a.v2 == "baz");

      constexpr auto fn5 = [](auto &) {};
      constexpr auto fn6 = [](some_in_place_type auto, auto...) {};
      static_assert(decltype(a)::invocable<decltype(fn5), decltype(a) &>);
      static_assert(decltype(a)::invocable_typed<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), std::string_view *>);
      CHECK(a.get_ptr(element) == &a.v2);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), std::string_view const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.v2);
    }

    WHEN("element v3 set")
    {
      constexpr auto element = std::in_place_type<std::string>;
      type a{element, "bar"};
      CHECK(a.v3 == "bar");

      constexpr auto fn5 = [](auto &) {};
      constexpr auto fn6 = [](some_in_place_type auto, auto...) {};
      static_assert(decltype(a)::invocable<decltype(fn5), decltype(a) &>);
      static_assert(decltype(a)::invocable_typed<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), std::string *>);
      CHECK(a.get_ptr(element) == &a.v3);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), std::string const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.v3);
    }
  }

  WHEN("size 5")
  {
    using type = sum_storage<int, double, std::string_view, std::string, std::bitset<12>>;
    static_assert(type::size == 5);
    static_assert(std::is_same_v<type::template type<0>, int>);
    static_assert(std::is_same_v<type::template type<1>, double>);
    static_assert(std::is_same_v<type::template type<2>, std::string_view>);
    static_assert(std::is_same_v<type::template type<3>, std::string>);
    static_assert(std::is_same_v<type::template type<4>, std::bitset<12>>);

    WHEN("element v0 set")
    {
      constexpr auto element = std::in_place_type<int>;
      type a{element, 42};
      CHECK(a.v0 == 42);

      static_assert(decltype(a)::invocable<decltype(fn1), decltype(a)>);
      static_assert(decltype(a)::invocable<decltype(fn2), decltype(a)>);
      static_assert(not decltype(a)::invocable_typed<decltype(fn1), decltype(a)>);
      static_assert(decltype(a)::invocable<decltype(fn2), decltype(a)>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), int *>);
      CHECK(a.get_ptr(element) == &a.v0);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), int const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.v0);
    }

    WHEN("element v1 set")
    {
      constexpr auto element = std::in_place_type<double>;
      type a{element, 0.5};
      CHECK(a.v1 == 0.5);

      constexpr auto fn5 = [](auto &) {};
      constexpr auto fn6 = [](some_in_place_type auto, auto...) {};
      static_assert(decltype(a)::invocable<decltype(fn5), decltype(a) &>);
      static_assert(decltype(a)::invocable_typed<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), double *>);
      CHECK(a.get_ptr(element) == &a.v1);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), double const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.v1);
    }

    WHEN("element v2 set")
    {
      constexpr auto element = std::in_place_type<std::string_view>;
      type a{element, "baz"};
      CHECK(a.v2 == "baz");

      constexpr auto fn5 = [](auto &) {};
      constexpr auto fn6 = [](some_in_place_type auto, auto...) {};
      static_assert(decltype(a)::invocable<decltype(fn5), decltype(a) &>);
      static_assert(decltype(a)::invocable_typed<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), std::string_view *>);
      CHECK(a.get_ptr(element) == &a.v2);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), std::string_view const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.v2);
    }

    WHEN("element v3 set")
    {
      constexpr auto element = std::in_place_type<std::string>;
      type a{element, "bar"};
      CHECK(a.v3 == "bar");

      constexpr auto fn5 = [](auto &) {};
      constexpr auto fn6 = [](some_in_place_type auto, auto...) {};
      static_assert(decltype(a)::invocable<decltype(fn5), decltype(a) &>);
      static_assert(decltype(a)::invocable_typed<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), std::string *>);
      CHECK(a.get_ptr(element) == &a.v3);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), std::string const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.v3);
    }

    WHEN("more elements set")
    {
      constexpr auto element = std::in_place_type<std::bitset<12>>;
      type a{element, 12345678ul};
      CHECK(a.more.v0 == std::bitset<12>(12345678ul));

      constexpr auto fn5 = [](auto &) {};
      constexpr auto fn6 = [](some_in_place_type auto, auto...) {};
      static_assert(decltype(a)::invocable<decltype(fn5), decltype(a) &>);
      static_assert(decltype(a)::invocable_typed<decltype(fn6), decltype(a) &>);

      static_assert(std::is_same_v<decltype(a.get_ptr(element)), std::bitset<12> *>);
      CHECK(a.get_ptr(element) == &a.more.v0);
      static_assert(std::is_same_v<decltype(std::as_const(a).get_ptr(element)), std::bitset<12> const *>);
      CHECK(std::as_const(a).get_ptr(element) == &a.more.v0);
    }
  }
}

TEST_CASE("sum_storage functions", "[sum_storage][apply_sum_storage_ptr][invoke_sum_storage]")
{
  // NOTE We have 5 different specializations, need to test each.
  // This test is ridiculously long to exercise the value-category
  // preserving cast inside both invoke_sum_storage overloads

  using namespace fn::detail;
  constexpr auto fn1 = [](auto i) noexcept -> std::size_t { return sizeof(i); };
  constexpr auto fn2 = [](auto, auto *i) noexcept -> std::size_t { return sizeof(*i); };
  constexpr auto fn3 = [](auto, auto i) noexcept -> std::size_t { return sizeof(i); };

  WHEN("size 1")
  {
    sum_storage<int> a{std::in_place_type<int>, 42};
    CHECK(a.v0 == 42);
    CHECK(_apply_sum_storage_ptr(0, fn2, a) == 4);

    WHEN("value only")
    {
      CHECK(invoke_sum_storage(0, fn1, a) == 4);
      CHECK(invoke_sum_storage(0,
                               fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { return true; },
                                            [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                            [](int const &&) -> bool { throw 0; }),
                               a));
      CHECK(invoke_sum_storage(0,
                               fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                            [](int const &) -> bool { return true; }, [](int &&) -> bool { throw 0; },
                                            [](int const &&) -> bool { throw 0; }),
                               std::as_const(a)));
      CHECK(invoke_sum_storage(0,
                               fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                            [](int const &) -> bool { throw 0; }, [](int &&) -> bool { return true; },
                                            [](int const &&) -> bool { throw 0; }),
                               sum_storage<int>{std::in_place_type<int>, 42}));
      CHECK(invoke_sum_storage(0,
                               fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                            [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                            [](int const &&) -> bool { return true; }),
                               std::move(std::as_const(a))));
    }

    WHEN("tag and value")
    {
      CHECK(invoke_sum_storage(0, fn3, a) == 4);
      CHECK(invoke_sum_storage(0,
                               fn::overload([](auto, auto) -> bool { throw 1; },
                                            [](std::in_place_type_t<int>, int &) -> bool { return true; },
                                            [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                                            [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                            [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; }),
                               a));
      CHECK(invoke_sum_storage(0,
                               fn::overload([](auto, auto) -> bool { throw 1; },
                                            [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                                            [](std::in_place_type_t<int>, int const &) -> bool { return true; },
                                            [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                            [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; }),
                               std::as_const(a)));
      CHECK(invoke_sum_storage(0,
                               fn::overload([](auto, auto) -> bool { throw 1; },
                                            [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                                            [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                                            [](std::in_place_type_t<int>, int &&) -> bool { return true; },
                                            [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; }),
                               sum_storage<int>{std::in_place_type<int>, 42}));
      CHECK(invoke_sum_storage(0,
                               fn::overload([](auto, auto) -> bool { throw 1; },
                                            [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                                            [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                                            [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                            [](std::in_place_type_t<int>, int const &&) -> bool { return true; }),
                               std::move(std::as_const(a))));
    }
  }

  WHEN("size 2")
  {
    using type = sum_storage<int, double>;
    static_assert(type::size == 2);

    WHEN("element v0 set")
    {
      constexpr auto element = 0;
      type a{std::in_place_type<int>, 42};
      CHECK(a.v0 == 42);
      CHECK(_apply_sum_storage_ptr(element, fn2, a) == 4);

      WHEN("value only")
      {
        CHECK(invoke_sum_storage(element, fn1, a) == 4);
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { return true; },
                                              [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                              [](int const &&) -> bool { throw 0; }),
                                 a));
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                              [](int const &) -> bool { return true; }, [](int &&) -> bool { throw 0; },
                                              [](int const &&) -> bool { throw 0; }),
                                 std::as_const(a)));
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                              [](int const &) -> bool { throw 0; }, [](int &&) -> bool { return true; },
                                              [](int const &&) -> bool { throw 0; }),
                                 type{std::in_place_type<int>, 42}));
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                              [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                              [](int const &&) -> bool { return true; }),
                                 std::move(std::as_const(a))));
      }
      WHEN("tag and value")
      {
        CHECK(invoke_sum_storage(element, fn3, a) == 4);
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<int>, int &) -> bool { return true; },
                                              [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; }),
                                 a));
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int const &) -> bool { return true; },
                                              [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; }),
                                 std::as_const(a)));
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int &&) -> bool { return true; },
                                              [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; }),
                                 type{std::in_place_type<int>, 42}));
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int const &&) -> bool { return true; }),
                                 std::move(std::as_const(a))));
      }
    }

    WHEN("element v1 set")
    {
      constexpr auto element = 1;
      type a{std::in_place_type<double>, 0.5};
      CHECK(a.v1 == 0.5);
      CHECK(_apply_sum_storage_ptr(element, fn2, a) == 8);
      WHEN("value only")
      {
        CHECK(invoke_sum_storage(element, fn1, a) == 8);
        CHECK(
            invoke_sum_storage(element,
                               fn::overload([](auto) -> bool { throw 1; }, [](double &) -> bool { return true; },
                                            [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                                            [](double const &&) -> bool { throw 0; }),
                               a));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                                              [](double const &) -> bool { return true; },
                                              [](double &&) -> bool { throw 0; },
                                              [](double const &&) -> bool { throw 0; }),
                                 std::as_const(a)));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                                              [](double const &) -> bool { throw 0; },
                                              [](double &&) -> bool { return true; },
                                              [](double const &&) -> bool { throw 0; }),
                                 type{std::in_place_type<double>, 0.5}));
        CHECK(
            invoke_sum_storage(element,
                               fn::overload([](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                                            [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                                            [](double const &&) -> bool { return true; }),
                               std::move(std::as_const(a))));
      }
      WHEN("tag and value")
      {
        CHECK(invoke_sum_storage(element, fn3, a) == 8);
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<double>, double &) -> bool { return true; },
                                              [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                                              [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                                              [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; }),
                                 a));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                                              [](std::in_place_type_t<double>, double const &) -> bool { return true; },
                                              [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                                              [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; }),
                                 std::as_const(a)));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                                              [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                                              [](std::in_place_type_t<double>, double &&) -> bool { return true; },
                                              [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; }),
                                 type{std::in_place_type<double>, 0.5}));
        CHECK(
            invoke_sum_storage(element,
                               fn::overload([](auto, auto) -> bool { throw 1; },
                                            [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                                            [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                                            [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                                            [](std::in_place_type_t<double>, double const &&) -> bool { return true; }),
                               std::move(std::as_const(a))));
      }
    }
  }

  WHEN("size 3")
  {
    using type = sum_storage<int, double, std::string_view>;
    static_assert(type::size == 3);

    WHEN("element v0 set")
    {
      constexpr auto element = 0;
      type a{std::in_place_type<int>, 42};
      CHECK(a.v0 == 42);
      CHECK(_apply_sum_storage_ptr(element, fn2, a) == 4);
      WHEN("value only")
      {
        CHECK(invoke_sum_storage(element, fn1, a) == 4);
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { return true; },
                                              [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                              [](int const &&) -> bool { throw 0; }),
                                 a));
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                              [](int const &) -> bool { return true; }, [](int &&) -> bool { throw 0; },
                                              [](int const &&) -> bool { throw 0; }),
                                 std::as_const(a)));
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                              [](int const &) -> bool { throw 0; }, [](int &&) -> bool { return true; },
                                              [](int const &&) -> bool { throw 0; }),
                                 type{std::in_place_type<int>, 42}));
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                              [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                              [](int const &&) -> bool { return true; }),
                                 std::move(std::as_const(a))));
      }
      WHEN("tag and value")
      {
        CHECK(invoke_sum_storage(element, fn3, a) == 4);
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<int>, int &) -> bool { return true; },
                                              [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; }),
                                 a));
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int const &) -> bool { return true; },
                                              [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; }),
                                 std::as_const(a)));
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int &&) -> bool { return true; },
                                              [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; }),
                                 type{std::in_place_type<int>, 42}));
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int const &&) -> bool { return true; }),
                                 std::move(std::as_const(a))));
      }
    }

    WHEN("element v1 set")
    {
      constexpr auto element = 1;
      type a{std::in_place_type<double>, 0.5};
      CHECK(a.v1 == 0.5);
      CHECK(_apply_sum_storage_ptr(element, fn2, a) == 8);
      WHEN("value only")
      {
        CHECK(invoke_sum_storage(element, fn1, a) == 8);
        CHECK(
            invoke_sum_storage(element,
                               fn::overload([](auto) -> bool { throw 1; }, [](double &) -> bool { return true; },
                                            [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                                            [](double const &&) -> bool { throw 0; }),
                               a));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                                              [](double const &) -> bool { return true; },
                                              [](double &&) -> bool { throw 0; },
                                              [](double const &&) -> bool { throw 0; }),
                                 std::as_const(a)));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                                              [](double const &) -> bool { throw 0; },
                                              [](double &&) -> bool { return true; },
                                              [](double const &&) -> bool { throw 0; }),
                                 type{std::in_place_type<double>, 0.5}));
        CHECK(
            invoke_sum_storage(element,
                               fn::overload([](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                                            [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                                            [](double const &&) -> bool { return true; }),
                               std::move(std::as_const(a))));
      }
      WHEN("tag and value")
      {
        CHECK(invoke_sum_storage(element, fn3, a) == 8);
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<double>, double &) -> bool { return true; },
                                              [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                                              [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                                              [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; }),
                                 a));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                                              [](std::in_place_type_t<double>, double const &) -> bool { return true; },
                                              [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                                              [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; }),
                                 std::as_const(a)));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                                              [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                                              [](std::in_place_type_t<double>, double &&) -> bool { return true; },
                                              [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; }),
                                 type{std::in_place_type<double>, 0.5}));
        CHECK(
            invoke_sum_storage(element,
                               fn::overload([](auto, auto) -> bool { throw 1; },
                                            [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                                            [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                                            [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                                            [](std::in_place_type_t<double>, double const &&) -> bool { return true; }),
                               std::move(std::as_const(a))));
      }
    }

    WHEN("element v2 set")
    {
      constexpr auto element = 2;
      type a{std::in_place_type<std::string_view>, "baz"};
      CHECK(a.v2 == "baz");
      CHECK(_apply_sum_storage_ptr(element, fn2, a) == 16);
      WHEN("value only")
      {
        CHECK(invoke_sum_storage(element, fn1, a) == 16);
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto) -> bool { throw 1; },
                                              [](std::string_view &) -> bool { return true; },
                                              [](std::string_view const &) -> bool { throw 0; },
                                              [](std::string_view &&) -> bool { throw 0; },
                                              [](std::string_view const &&) -> bool { throw 0; }),
                                 a));
        CHECK(
            invoke_sum_storage(element,
                               fn::overload([](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                                            [](std::string_view const &) -> bool { return true; },
                                            [](std::string_view &&) -> bool { throw 0; },
                                            [](std::string_view const &&) -> bool { throw 0; }),
                               std::as_const(a)));
        CHECK(
            invoke_sum_storage(element,
                               fn::overload([](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                                            [](std::string_view const &) -> bool { throw 0; },
                                            [](std::string_view &&) -> bool { return true; },
                                            [](std::string_view const &&) -> bool { throw 0; }),
                               type{std::in_place_type<std::string_view>, "baz"}));
        CHECK(
            invoke_sum_storage(element,
                               fn::overload([](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                                            [](std::string_view const &) -> bool { throw 0; },
                                            [](std::string_view &&) -> bool { throw 0; },
                                            [](std::string_view const &&) -> bool { return true; }),
                               std::move(std::as_const(a))));
      }
      WHEN("tag and value")
      {
        CHECK(invoke_sum_storage(element, fn3, a) == 16);
        CHECK(invoke_sum_storage(
            element,
            fn::overload([](auto, auto) -> bool { throw 1; },
                         [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { return true; },
                         [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; }),
            a));
        CHECK(invoke_sum_storage(
            element,
            fn::overload([](auto, auto) -> bool { throw 1; },
                         [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { return true; },
                         [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; }),
            std::as_const(a)));
        CHECK(invoke_sum_storage(
            element,
            fn::overload([](auto, auto) -> bool { throw 1; },
                         [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { return true; },
                         [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; }),
            type{std::in_place_type<std::string_view>, "baz"}));
        CHECK(invoke_sum_storage(
            element,
            fn::overload(
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { return true; }),
            std::move(std::as_const(a))));
      }
    }
  }

  WHEN("size 4")
  {
    using type = sum_storage<int, double, std::string_view, std::string>;
    static_assert(type::size == 4);

    WHEN("element v0 set")
    {
      constexpr auto element = 0;
      type a{std::in_place_type<int>, 42};
      CHECK(a.v0 == 42);
      CHECK(_apply_sum_storage_ptr(element, fn2, a) == 4);
      WHEN("value only")
      {
        CHECK(invoke_sum_storage(element, fn1, a) == 4);
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { return true; },
                                              [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                              [](int const &&) -> bool { throw 0; }),
                                 a));
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                              [](int const &) -> bool { return true; }, [](int &&) -> bool { throw 0; },
                                              [](int const &&) -> bool { throw 0; }),
                                 std::as_const(a)));
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                              [](int const &) -> bool { throw 0; }, [](int &&) -> bool { return true; },
                                              [](int const &&) -> bool { throw 0; }),
                                 type{std::in_place_type<int>, 42}));
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                              [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                              [](int const &&) -> bool { return true; }),
                                 std::move(std::as_const(a))));
      }
      WHEN("tag and value")
      {
        CHECK(invoke_sum_storage(element, fn3, a) == 4);
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<int>, int &) -> bool { return true; },
                                              [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; }),
                                 a));
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int const &) -> bool { return true; },
                                              [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; }),
                                 std::as_const(a)));
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int &&) -> bool { return true; },
                                              [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; }),
                                 type{std::in_place_type<int>, 42}));
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int const &&) -> bool { return true; }),
                                 std::move(std::as_const(a))));
      }
    }

    WHEN("element v1 set")
    {
      constexpr auto element = 1;
      type a{std::in_place_type<double>, 0.5};
      CHECK(a.v1 == 0.5);
      CHECK(_apply_sum_storage_ptr(element, fn2, a) == 8);
      WHEN("value only")
      {
        CHECK(invoke_sum_storage(element, fn1, a) == 8);
        CHECK(
            invoke_sum_storage(element,
                               fn::overload([](auto) -> bool { throw 1; }, [](double &) -> bool { return true; },
                                            [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                                            [](double const &&) -> bool { throw 0; }),
                               a));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                                              [](double const &) -> bool { return true; },
                                              [](double &&) -> bool { throw 0; },
                                              [](double const &&) -> bool { throw 0; }),
                                 std::as_const(a)));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                                              [](double const &) -> bool { throw 0; },
                                              [](double &&) -> bool { return true; },
                                              [](double const &&) -> bool { throw 0; }),
                                 type{std::in_place_type<double>, 0.5}));
        CHECK(
            invoke_sum_storage(element,
                               fn::overload([](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                                            [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                                            [](double const &&) -> bool { return true; }),
                               std::move(std::as_const(a))));
      }
      WHEN("tag and value")
      {
        CHECK(invoke_sum_storage(element, fn3, a) == 8);
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<double>, double &) -> bool { return true; },
                                              [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                                              [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                                              [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; }),
                                 a));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                                              [](std::in_place_type_t<double>, double const &) -> bool { return true; },
                                              [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                                              [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; }),
                                 std::as_const(a)));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                                              [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                                              [](std::in_place_type_t<double>, double &&) -> bool { return true; },
                                              [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; }),
                                 type{std::in_place_type<double>, 0.5}));
        CHECK(
            invoke_sum_storage(element,
                               fn::overload([](auto, auto) -> bool { throw 1; },
                                            [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                                            [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                                            [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                                            [](std::in_place_type_t<double>, double const &&) -> bool { return true; }),
                               std::move(std::as_const(a))));
      }
    }

    WHEN("element v2 set")
    {
      constexpr auto element = 2;
      type a{std::in_place_type<std::string_view>, "baz"};
      CHECK(a.v2 == "baz");
      CHECK(_apply_sum_storage_ptr(element, fn2, a) == 16);
      WHEN("value only")
      {
        CHECK(invoke_sum_storage(element, fn1, a) == 16);
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto) -> bool { throw 1; },
                                              [](std::string_view &) -> bool { return true; },
                                              [](std::string_view const &) -> bool { throw 0; },
                                              [](std::string_view &&) -> bool { throw 0; },
                                              [](std::string_view const &&) -> bool { throw 0; }),
                                 a));
        CHECK(
            invoke_sum_storage(element,
                               fn::overload([](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                                            [](std::string_view const &) -> bool { return true; },
                                            [](std::string_view &&) -> bool { throw 0; },
                                            [](std::string_view const &&) -> bool { throw 0; }),
                               std::as_const(a)));
        CHECK(
            invoke_sum_storage(element,
                               fn::overload([](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                                            [](std::string_view const &) -> bool { throw 0; },
                                            [](std::string_view &&) -> bool { return true; },
                                            [](std::string_view const &&) -> bool { throw 0; }),
                               type{std::in_place_type<std::string_view>, "baz"}));
        CHECK(
            invoke_sum_storage(element,
                               fn::overload([](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                                            [](std::string_view const &) -> bool { throw 0; },
                                            [](std::string_view &&) -> bool { throw 0; },
                                            [](std::string_view const &&) -> bool { return true; }),
                               std::move(std::as_const(a))));
      }
      WHEN("tag and value")
      {
        CHECK(invoke_sum_storage(element, fn3, a) == 16);
        CHECK(invoke_sum_storage(
            element,
            fn::overload([](auto, auto) -> bool { throw 1; },
                         [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { return true; },
                         [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; }),
            a));
        CHECK(invoke_sum_storage(
            element,
            fn::overload([](auto, auto) -> bool { throw 1; },
                         [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { return true; },
                         [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; }),
            std::as_const(a)));
        CHECK(invoke_sum_storage(
            element,
            fn::overload([](auto, auto) -> bool { throw 1; },
                         [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { return true; },
                         [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; }),
            type{std::in_place_type<std::string_view>, "baz"}));
        CHECK(invoke_sum_storage(
            element,
            fn::overload(
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { return true; }),
            std::move(std::as_const(a))));
      }
    }

    WHEN("element v3 set")
    {
      constexpr auto element = 3;
      type a{std::in_place_type<std::string>, "bar"};
      CHECK(a.v3 == "bar");
      CHECK(_apply_sum_storage_ptr(element, fn2, a) == 32);
      WHEN("value only")
      {
        CHECK(invoke_sum_storage(element, fn1, a) == 32);
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto) -> bool { throw 1; }, [](std::string &) -> bool { return true; },
                                              [](std::string const &) -> bool { throw 0; },
                                              [](std::string &&) -> bool { throw 0; },
                                              [](std::string const &&) -> bool { throw 0; }),
                                 a));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto) -> bool { throw 1; }, [](std::string &) -> bool { throw 0; },
                                              [](std::string const &) -> bool { return true; },
                                              [](std::string &&) -> bool { throw 0; },
                                              [](std::string const &&) -> bool { throw 0; }),
                                 std::as_const(a)));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto) -> bool { throw 1; }, [](std::string &) -> bool { throw 0; },
                                              [](std::string const &) -> bool { throw 0; },
                                              [](std::string &&) -> bool { return true; },
                                              [](std::string const &&) -> bool { throw 0; }),
                                 type{std::in_place_type<std::string>, "baz"}));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto) -> bool { throw 1; }, [](std::string &) -> bool { throw 0; },
                                              [](std::string const &) -> bool { throw 0; },
                                              [](std::string &&) -> bool { throw 0; },
                                              [](std::string const &&) -> bool { return true; }),
                                 std::move(std::as_const(a))));
      }
      WHEN("tag and value")
      {
        CHECK(invoke_sum_storage(element, fn3, a) == 32);
        CHECK(invoke_sum_storage(
            element,
            fn::overload([](auto, auto) -> bool { throw 1; },
                         [](std::in_place_type_t<std::string>, std::string &) -> bool { return true; },
                         [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; }),
            a));
        CHECK(invoke_sum_storage(
            element,
            fn::overload([](auto, auto) -> bool { throw 1; },
                         [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string>, std::string const &) -> bool { return true; },
                         [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; }),
            std::as_const(a)));
        CHECK(invoke_sum_storage(
            element,
            fn::overload([](auto, auto) -> bool { throw 1; },
                         [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string>, std::string &&) -> bool { return true; },
                         [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; }),
            type{std::in_place_type<std::string>, "baz"}));
        CHECK(invoke_sum_storage(
            element,
            fn::overload([](auto, auto) -> bool { throw 1; },
                         [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string>, std::string const &&) -> bool { return true; }),
            std::move(std::as_const(a))));
      }
    }
  }

  WHEN("size 5")
  {
    using type = sum_storage<int, double, std::string_view, std::string, std::bitset<12>>;
    static_assert(type::size == 5);

    WHEN("element v0 set")
    {
      constexpr auto element = 0;
      type a{std::in_place_type<int>, 42};
      CHECK(a.v0 == 42);
      CHECK(_apply_sum_storage_ptr(element, fn2, a) == 4);
      WHEN("value only")
      {
        CHECK(invoke_sum_storage(element, fn1, a) == 4);
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { return true; },
                                              [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                              [](int const &&) -> bool { throw 0; }),
                                 a));
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                              [](int const &) -> bool { return true; }, [](int &&) -> bool { throw 0; },
                                              [](int const &&) -> bool { throw 0; }),
                                 std::as_const(a)));
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                              [](int const &) -> bool { throw 0; }, [](int &&) -> bool { return true; },
                                              [](int const &&) -> bool { throw 0; }),
                                 type{std::in_place_type<int>, 42}));
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto) -> bool { throw 1; }, [](int &) -> bool { throw 0; },
                                              [](int const &) -> bool { throw 0; }, [](int &&) -> bool { throw 0; },
                                              [](int const &&) -> bool { return true; }),
                                 std::move(std::as_const(a))));
      }
      WHEN("tag and value")
      {
        CHECK(invoke_sum_storage(element, fn3, a) == 4);
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<int>, int &) -> bool { return true; },
                                              [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; }),
                                 a));
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int const &) -> bool { return true; },
                                              [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; }),
                                 std::as_const(a)));
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int &&) -> bool { return true; },
                                              [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; }),
                                 type{std::in_place_type<int>, 42}));
        CHECK(invoke_sum_storage(0,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                                              [](std::in_place_type_t<int>, int const &&) -> bool { return true; }),
                                 std::move(std::as_const(a))));
      }
    }

    WHEN("element v1 set")
    {
      constexpr auto element = 1;
      type a{std::in_place_type<double>, 0.5};
      CHECK(a.v1 == 0.5);
      CHECK(_apply_sum_storage_ptr(element, fn2, a) == 8);
      WHEN("value only")
      {
        CHECK(invoke_sum_storage(element, fn1, a) == 8);
        CHECK(
            invoke_sum_storage(element,
                               fn::overload([](auto) -> bool { throw 1; }, [](double &) -> bool { return true; },
                                            [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                                            [](double const &&) -> bool { throw 0; }),
                               a));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                                              [](double const &) -> bool { return true; },
                                              [](double &&) -> bool { throw 0; },
                                              [](double const &&) -> bool { throw 0; }),
                                 std::as_const(a)));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                                              [](double const &) -> bool { throw 0; },
                                              [](double &&) -> bool { return true; },
                                              [](double const &&) -> bool { throw 0; }),
                                 type{std::in_place_type<double>, 0.5}));
        CHECK(
            invoke_sum_storage(element,
                               fn::overload([](auto) -> bool { throw 1; }, [](double &) -> bool { throw 0; },
                                            [](double const &) -> bool { throw 0; }, [](double &&) -> bool { throw 0; },
                                            [](double const &&) -> bool { return true; }),
                               std::move(std::as_const(a))));
      }
      WHEN("tag and value")
      {
        CHECK(invoke_sum_storage(element, fn3, a) == 8);
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<double>, double &) -> bool { return true; },
                                              [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                                              [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                                              [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; }),
                                 a));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                                              [](std::in_place_type_t<double>, double const &) -> bool { return true; },
                                              [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                                              [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; }),
                                 std::as_const(a)));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto, auto) -> bool { throw 1; },
                                              [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                                              [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                                              [](std::in_place_type_t<double>, double &&) -> bool { return true; },
                                              [](std::in_place_type_t<double>, double const &&) -> bool { throw 0; }),
                                 type{std::in_place_type<double>, 0.5}));
        CHECK(
            invoke_sum_storage(element,
                               fn::overload([](auto, auto) -> bool { throw 1; },
                                            [](std::in_place_type_t<double>, double &) -> bool { throw 0; },
                                            [](std::in_place_type_t<double>, double const &) -> bool { throw 0; },
                                            [](std::in_place_type_t<double>, double &&) -> bool { throw 0; },
                                            [](std::in_place_type_t<double>, double const &&) -> bool { return true; }),
                               std::move(std::as_const(a))));
      }
    }

    WHEN("element v2 set")
    {
      constexpr auto element = 2;
      type a{std::in_place_type<std::string_view>, "baz"};
      CHECK(a.v2 == "baz");
      CHECK(_apply_sum_storage_ptr(element, fn2, a) == 16);
      WHEN("value only")
      {
        CHECK(invoke_sum_storage(element, fn1, a) == 16);
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto) -> bool { throw 1; },
                                              [](std::string_view &) -> bool { return true; },
                                              [](std::string_view const &) -> bool { throw 0; },
                                              [](std::string_view &&) -> bool { throw 0; },
                                              [](std::string_view const &&) -> bool { throw 0; }),
                                 a));
        CHECK(
            invoke_sum_storage(element,
                               fn::overload([](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                                            [](std::string_view const &) -> bool { return true; },
                                            [](std::string_view &&) -> bool { throw 0; },
                                            [](std::string_view const &&) -> bool { throw 0; }),
                               std::as_const(a)));
        CHECK(
            invoke_sum_storage(element,
                               fn::overload([](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                                            [](std::string_view const &) -> bool { throw 0; },
                                            [](std::string_view &&) -> bool { return true; },
                                            [](std::string_view const &&) -> bool { throw 0; }),
                               type{std::in_place_type<std::string_view>, "baz"}));
        CHECK(
            invoke_sum_storage(element,
                               fn::overload([](auto) -> bool { throw 1; }, [](std::string_view &) -> bool { throw 0; },
                                            [](std::string_view const &) -> bool { throw 0; },
                                            [](std::string_view &&) -> bool { throw 0; },
                                            [](std::string_view const &&) -> bool { return true; }),
                               std::move(std::as_const(a))));
      }
      WHEN("tag and value")
      {
        CHECK(invoke_sum_storage(element, fn3, a) == 16);
        CHECK(invoke_sum_storage(
            element,
            fn::overload([](auto, auto) -> bool { throw 1; },
                         [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { return true; },
                         [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; }),
            a));
        CHECK(invoke_sum_storage(
            element,
            fn::overload([](auto, auto) -> bool { throw 1; },
                         [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { return true; },
                         [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; }),
            std::as_const(a)));
        CHECK(invoke_sum_storage(
            element,
            fn::overload([](auto, auto) -> bool { throw 1; },
                         [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { return true; },
                         [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { throw 0; }),
            type{std::in_place_type<std::string_view>, "baz"}));
        CHECK(invoke_sum_storage(
            element,
            fn::overload(
                [](auto, auto) -> bool { throw 1; },
                [](std::in_place_type_t<std::string_view>, std::string_view &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view &&) -> bool { throw 0; },
                [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> bool { return true; }),
            std::move(std::as_const(a))));
      }
    }

    WHEN("element v3 set")
    {
      constexpr auto element = 3;
      type a{std::in_place_type<std::string>, "bar"};
      CHECK(a.v3 == "bar");
      CHECK(_apply_sum_storage_ptr(element, fn2, a) == 32);
      WHEN("value only")
      {
        CHECK(invoke_sum_storage(element, fn1, a) == 32);
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto) -> bool { throw 1; }, [](std::string &) -> bool { return true; },
                                              [](std::string const &) -> bool { throw 0; },
                                              [](std::string &&) -> bool { throw 0; },
                                              [](std::string const &&) -> bool { throw 0; }),
                                 a));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto) -> bool { throw 1; }, [](std::string &) -> bool { throw 0; },
                                              [](std::string const &) -> bool { return true; },
                                              [](std::string &&) -> bool { throw 0; },
                                              [](std::string const &&) -> bool { throw 0; }),
                                 std::as_const(a)));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto) -> bool { throw 1; }, [](std::string &) -> bool { throw 0; },
                                              [](std::string const &) -> bool { throw 0; },
                                              [](std::string &&) -> bool { return true; },
                                              [](std::string const &&) -> bool { throw 0; }),
                                 type{std::in_place_type<std::string>, "baz"}));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto) -> bool { throw 1; }, [](std::string &) -> bool { throw 0; },
                                              [](std::string const &) -> bool { throw 0; },
                                              [](std::string &&) -> bool { throw 0; },
                                              [](std::string const &&) -> bool { return true; }),
                                 std::move(std::as_const(a))));
      }
      WHEN("tag and value")
      {
        CHECK(invoke_sum_storage(element, fn3, a) == 32);
        CHECK(invoke_sum_storage(
            element,
            fn::overload([](auto, auto) -> bool { throw 1; },
                         [](std::in_place_type_t<std::string>, std::string &) -> bool { return true; },
                         [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; }),
            a));
        CHECK(invoke_sum_storage(
            element,
            fn::overload([](auto, auto) -> bool { throw 1; },
                         [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string>, std::string const &) -> bool { return true; },
                         [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; }),
            std::as_const(a)));
        CHECK(invoke_sum_storage(
            element,
            fn::overload([](auto, auto) -> bool { throw 1; },
                         [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string>, std::string &&) -> bool { return true; },
                         [](std::in_place_type_t<std::string>, std::string const &&) -> bool { throw 0; }),
            type{std::in_place_type<std::string>, "baz"}));
        CHECK(invoke_sum_storage(
            element,
            fn::overload([](auto, auto) -> bool { throw 1; },
                         [](std::in_place_type_t<std::string>, std::string &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string>, std::string const &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string>, std::string &&) -> bool { throw 0; },
                         [](std::in_place_type_t<std::string>, std::string const &&) -> bool { return true; }),
            std::move(std::as_const(a))));
      }
    }

    WHEN("more elements set")
    {
      constexpr auto element = 4;
      type a{std::in_place_type<std::bitset<12>>, 12345678ul};
      CHECK(a.more.v0 == std::bitset<12>(12345678ul));
      CHECK(_apply_sum_storage_ptr(element, fn2, a) == 8);
      WHEN("value only")
      {
        CHECK(invoke_sum_storage(element, fn1, a) == 8);
        CHECK(invoke_sum_storage(
            element,
            fn::overload([](auto) -> bool { throw 1; }, [](std::bitset<12> &) -> bool { return true; },
                         [](std::bitset<12> const &) -> bool { throw 0; }, [](std::bitset<12> &&) -> bool { throw 0; },
                         [](std::bitset<12> const &&) -> bool { throw 0; }),
            a));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto) -> bool { throw 1; }, [](std::bitset<12> &) -> bool { throw 0; },
                                              [](std::bitset<12> const &) -> bool { return true; },
                                              [](std::bitset<12> &&) -> bool { throw 0; },
                                              [](std::bitset<12> const &&) -> bool { throw 0; }),
                                 std::as_const(a)));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto) -> bool { throw 1; }, [](std::bitset<12> &) -> bool { throw 0; },
                                              [](std::bitset<12> const &) -> bool { throw 0; },
                                              [](std::bitset<12> &&) -> bool { return true; },
                                              [](std::bitset<12> const &&) -> bool { throw 0; }),
                                 type{std::in_place_type<std::bitset<12>>, 12345678ul}));
        CHECK(invoke_sum_storage(element,
                                 fn::overload([](auto) -> bool { throw 1; }, [](std::bitset<12> &) -> bool { throw 0; },
                                              [](std::bitset<12> const &) -> bool { throw 0; },
                                              [](std::bitset<12> &&) -> bool { throw 0; },
                                              [](std::bitset<12> const &&) -> bool { return true; }),
                                 std::move(std::as_const(a))));
      }
      WHEN("tag and value")
      {
        CHECK(invoke_sum_storage(element, fn3, a) == 8);
        CHECK(invoke_sum_storage(
            element,
            fn::overload([](auto, auto) -> bool { throw 1; },
                         [](std::in_place_type_t<std::bitset<12>>, std::bitset<12> &) -> bool { return true; },
                         [](std::in_place_type_t<std::bitset<12>>, std::bitset<12> const &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::bitset<12>>, std::bitset<12> &&) -> bool { throw 0; },
                         [](std::in_place_type_t<std::bitset<12>>, std::bitset<12> const &&) -> bool { throw 0; }),
            a));
        CHECK(invoke_sum_storage(
            element,
            fn::overload([](auto, auto) -> bool { throw 1; },
                         [](std::in_place_type_t<std::bitset<12>>, std::bitset<12> &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::bitset<12>>, std::bitset<12> const &) -> bool { return true; },
                         [](std::in_place_type_t<std::bitset<12>>, std::bitset<12> &&) -> bool { throw 0; },
                         [](std::in_place_type_t<std::bitset<12>>, std::bitset<12> const &&) -> bool { throw 0; }),
            std::as_const(a)));
        CHECK(invoke_sum_storage(
            element,
            fn::overload([](auto, auto) -> bool { throw 1; },
                         [](std::in_place_type_t<std::bitset<12>>, std::bitset<12> &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::bitset<12>>, std::bitset<12> const &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::bitset<12>>, std::bitset<12> &&) -> bool { return true; },
                         [](std::in_place_type_t<std::bitset<12>>, std::bitset<12> const &&) -> bool { throw 0; }),
            type{std::in_place_type<std::bitset<12>>, 12345678ul}));
        CHECK(invoke_sum_storage(
            element,
            fn::overload([](auto, auto) -> bool { throw 1; },
                         [](std::in_place_type_t<std::bitset<12>>, std::bitset<12> &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::bitset<12>>, std::bitset<12> const &) -> bool { throw 0; },
                         [](std::in_place_type_t<std::bitset<12>>, std::bitset<12> &&) -> bool { throw 0; },
                         [](std::in_place_type_t<std::bitset<12>>, std::bitset<12> const &&) -> bool { return true; }),
            std::move(std::as_const(a))));
      }
    }
  }
}
