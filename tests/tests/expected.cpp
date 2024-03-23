// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/expected.hpp"
#include "functional/utility.hpp"

#include <catch2/catch_all.hpp>

#include <utility>

namespace {
enum Error { Unknown, FileNotFound };
}

TEST_CASE("expected pack support", "[expected][pack][and_then][transform][operator_and]")
{
  WHEN("and_then")
  {
    WHEN("value")
    {
      fn::expected<fn::pack<int, std::string_view>, Error> s{
          fn::pack<int>{12}.append(std::in_place_type<std::string_view>, "bar")};

      CHECK(s.and_then( //
                 fn::overload([](int &i, auto &&...) -> fn::expected<bool, Error> { return i == 12; },
                              [](int const &, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                              [](int &&, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                              [](int const &&, auto &&...) -> fn::expected<bool, Error> { throw 0; })) //
                .value());
      CHECK(std::as_const(s)
                .and_then( //
                    fn::overload([](int &, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                                 [](int const &i, auto &&...) -> fn::expected<bool, Error> { return i == 12; },
                                 [](int &&, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                                 [](int const &&, auto &&...) -> fn::expected<bool, Error> { throw 0; })) //
                .value());
      CHECK(fn::expected<fn::pack<int, std::string_view>, Error>{
          fn::pack<int>{12}.append(std::in_place_type<std::string_view>, "bar")}
                .and_then( //
                    fn::overload([](int &, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                                 [](int const &, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                                 [](int &&i, auto &&...) -> fn::expected<bool, Error> { return i == 12; },
                                 [](int const &&, auto &&...) -> fn::expected<bool, Error> { throw 0; })) //
                .value());
      CHECK(std::move(std::as_const(s))
                .and_then( //
                    fn::overload([](int &, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                                 [](int const &, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                                 [](int &&, auto &&...) -> fn::expected<bool, Error> { throw 0; },
                                 [](int const &&i, auto &&...) -> fn::expected<bool, Error> { return i == 12; })) //
                .value());
    }

    WHEN("error")
    {
      fn::expected<fn::pack<int, std::string_view>, Error> s{std::unexpect, FileNotFound};
      CHECK(s.and_then( //
                 [](auto...) -> fn::expected<bool, Error> { throw 0; })
                .error()
            == FileNotFound);
      CHECK(std::as_const(s)
                .and_then( //
                    [](auto...) -> fn::expected<bool, Error> { throw 0; })
                .error()
            == FileNotFound);
      CHECK(fn::expected<fn::pack<int, std::string_view>, Error>{std::unexpect, FileNotFound}
                .and_then( //
                    [](auto...) -> fn::expected<bool, Error> { throw 0; })
                .error()
            == FileNotFound);
      CHECK(std::move(std::as_const(s))
                .and_then( //
                    [](auto...) -> fn::expected<bool, Error> { throw 0; })
                .error()
            == FileNotFound);
    }
  }

  WHEN("transform")
  {
    WHEN("value")
    {
      fn::expected<fn::pack<int, std::string_view>, Error> s{
          fn::pack<int>{12}.append(std::in_place_type<std::string_view>, "bar")};

      CHECK(s.transform( //
                 fn::overload([](int &i, auto &&...) -> bool { return i == 12; },
                              [](int const &, auto &&...) -> bool { throw 0; },
                              [](int &&, auto &&...) -> bool { throw 0; },
                              [](int const &&, auto &&...) -> bool { throw 0; })) //
                .value());
      CHECK(std::as_const(s)
                .transform( //
                    fn::overload([](int &, auto &&...) -> bool { throw 0; },
                                 [](int const &i, auto &&...) -> bool { return i == 12; },
                                 [](int &&, auto &&...) -> bool { throw 0; },
                                 [](int const &&, auto &&...) -> bool { throw 0; })) //
                .value());
      CHECK(fn::expected<fn::pack<int, std::string_view>, Error>{
          fn::pack<int>{12}.append(std::in_place_type<std::string_view>, "bar")}
                .transform( //
                    fn::overload([](int &, auto &&...) -> bool { throw 0; },
                                 [](int const &, auto &&...) -> bool { throw 0; },
                                 [](int &&i, auto &&...) -> bool { return i == 12; },
                                 [](int const &&, auto &&...) -> bool { throw 0; })) //
                .value());
      CHECK(std::move(std::as_const(s))
                .transform( //
                    fn::overload([](int &, auto &&...) -> bool { throw 0; },
                                 [](int const &, auto &&...) -> bool { throw 0; },
                                 [](int &&, auto &&...) -> bool { throw 0; },
                                 [](int const &&i, auto &&...) -> bool { return i == 12; })) //
                .value());
    }

    WHEN("void result")
    {
      fn::expected<fn::pack<int, std::string_view>, Error> s{
          fn::pack<int>{12}.append(std::in_place_type<std::string_view>, "bar")};

      CHECK(s.transform( //
                 fn::overload([](int &, auto &&...) -> void {}, [](int const &, auto &&...) -> void { throw 0; },
                              [](int &&, auto &&...) -> void { throw 0; },
                              [](int const &&, auto &&...) -> void { throw 0; })) //
                .has_value());
      CHECK(std::as_const(s)
                .transform( //
                    fn::overload([](int &, auto &&...) -> void { throw 0; }, [](int const &, auto &&...) -> void {},
                                 [](int &&, auto &&...) -> void { throw 0; },
                                 [](int const &&, auto &&...) -> void { throw 0; })) //
                .has_value());
      CHECK(fn::expected<fn::pack<int, std::string_view>, Error>{
          fn::pack<int>{12}.append(std::in_place_type<std::string_view>, "bar")}
                .transform( //
                    fn::overload([](int &, auto &&...) -> void { throw 0; },
                                 [](int const &, auto &&...) -> void { throw 0; }, [](int &&, auto &&...) -> void {},
                                 [](int const &&, auto &&...) -> void { throw 0; })) //
                .has_value());
      CHECK(std::move(std::as_const(s))
                .transform( //
                    fn::overload(
                        [](int &, auto &&...) -> void { throw 0; }, [](int const &, auto &&...) -> void { throw 0; },
                        [](int &&, auto &&...) -> void { throw 0; }, [](int const &&, auto &&...) -> void {})) //
                .has_value());
    }

    WHEN("error")
    {
      fn::expected<fn::pack<int, std::string_view>, Error> s{std::unexpect, FileNotFound};
      CHECK(s.transform([](auto...) -> bool { throw 0; }).error() == FileNotFound);
      CHECK(std::as_const(s).transform([](auto...) -> bool { throw 0; }).error() == FileNotFound);
      CHECK(fn::expected<fn::pack<int, std::string_view>, Error>{std::unexpect, FileNotFound}
                .transform([](auto...) -> bool { throw 0; })
                .error()
            == FileNotFound);
      CHECK(std::move(std::as_const(s)).transform([](auto...) -> bool { throw 0; }).error() == FileNotFound);
    }
  }

  WHEN("operator &")
  {
    WHEN("value & void yield value")
    {
      static_assert(
          std::same_as<decltype(std::declval<fn::expected<int, Error>>() & std::declval<fn::expected<void, Error>>()),
                       fn::expected<int, Error>>);

      CHECK((fn::expected<int, Error>{42} //
             & fn::expected<void, Error>{})
                .value()
            == 42);
      CHECK((fn::expected<int, Error>{std::unexpect, FileNotFound} //
             & fn::expected<void, Error>{})
                .error()
            == FileNotFound);
      CHECK((fn::expected<int, Error>{42} //
             & fn::expected<void, Error>{std::unexpect, Unknown})
                .error()
            == Unknown);
      CHECK((fn::expected<int, Error>{std::unexpect, FileNotFound} //
             & fn::expected<void, Error>{std::unexpect, Unknown})
                .error()
            == FileNotFound);
    }

    WHEN("void & value yield value")
    {
      static_assert(
          std::same_as<decltype(std::declval<fn::expected<void, Error>>() & std::declval<fn::expected<int, Error>>()),
                       fn::expected<int, Error>>);

      CHECK((fn::expected<void, Error>{} //
             & fn::expected<int, Error>{12})
                .value()
            == 12);
      CHECK((fn::expected<void, Error>{std::unexpect, FileNotFound} //
             & fn::expected<int, Error>{12})
                .error()
            == FileNotFound);
      CHECK((fn::expected<void, Error>{} //
             & fn::expected<int, Error>{std::unexpect, Unknown})
                .error()
            == Unknown);
      CHECK((fn::expected<void, Error>{std::unexpect, FileNotFound} //
             & fn::expected<int, Error>{std::unexpect, Unknown})
                .error()
            == FileNotFound);
    }

    WHEN("void & void yield void")
    {
      static_assert(
          std::same_as<decltype(std::declval<fn::expected<void, Error>>() & std::declval<fn::expected<void, Error>>()),
                       fn::expected<void, Error>>);

      CHECK((fn::expected<void, Error>{} //
             & fn::expected<void, Error>{})
                .has_value());
      CHECK((fn::expected<void, Error>{std::unexpect, FileNotFound} //
             & fn::expected<void, Error>{})
                .error()
            == FileNotFound);
      CHECK((fn::expected<void, Error>{} //
             & fn::expected<void, Error>{std::unexpect, Unknown})
                .error()
            == Unknown);
      CHECK((fn::expected<void, Error>{std::unexpect, FileNotFound} //
             & fn::expected<void, Error>{std::unexpect, Unknown})
                .error()
            == FileNotFound);
    }

    WHEN("value & value yield pack")
    {
      static_assert(
          std::same_as<decltype(std::declval<fn::expected<int, Error>>() & std::declval<fn::expected<double, Error>>()),
                       fn::expected<fn::pack<int, double>, Error>>);

      CHECK((fn::expected<double, Error>{0.5} //
             & fn::expected<int, Error>{12})
                .transform([](double d, int i) constexpr -> bool { return d == 0.5 && i == 12; })
                .value());
      CHECK((fn::expected<double, Error>{std::unexpect, FileNotFound} //
             & fn::expected<int, Error>{12})
                .error()
            == FileNotFound);
      CHECK((fn::expected<double, Error>{} //
             & fn::expected<int, Error>{std::unexpect, Unknown})
                .error()
            == Unknown);
      CHECK((fn::expected<double, Error>{std::unexpect, FileNotFound} //
             & fn::expected<int, Error>{std::unexpect, Unknown})
                .error()
            == FileNotFound);
    }

    WHEN("pack & value yield pack")
    {
      static_assert(std::same_as<decltype(std::declval<fn::expected<fn::pack<double, bool>, Error>>()
                                          & std::declval<fn::expected<int, Error>>()),
                                 fn::expected<fn::pack<double, bool, int>, Error>>);

      CHECK((fn::expected<fn::pack<double, bool>, Error>{std::in_place, fn::pack<double, bool>{0.5, true}} //
             & fn::expected<int, Error>{12})
                .transform([](double d, bool b, int i) constexpr -> bool { return d == 0.5 && b && i == 12; })
                .value());
      CHECK((fn::expected<fn::pack<double, bool>, Error>{std::unexpect, FileNotFound} //
             & fn::expected<int, Error>{12})
                .error()
            == FileNotFound);
      CHECK((fn::expected<fn::pack<double, bool>, Error>{std::in_place, fn::pack<double, bool>{0.5, true}} //
             & fn::expected<int, Error>{std::unexpect, Unknown})
                .error()
            == Unknown);
      CHECK((fn::expected<fn::pack<double, bool>, Error>{std::unexpect, FileNotFound} //
             & fn::expected<int, Error>{std::unexpect, Unknown})
                .error()
            == FileNotFound);
    }

    WHEN("pack & void yield pack")
    {
      static_assert(std::same_as<decltype(std::declval<fn::expected<fn::pack<double, bool>, Error>>()
                                          & std::declval<fn::expected<void, Error>>()),
                                 fn::expected<fn::pack<double, bool>, Error>>);

      CHECK((fn::expected<fn::pack<double, bool>, Error>{std::in_place, fn::pack<double, bool>{0.5, true}} //
             & fn::expected<void, Error>{})
                .transform([](double d, bool b) constexpr -> bool { return d == 0.5 && b; })
                .value());
      CHECK((fn::expected<fn::pack<double, bool>, Error>{std::unexpect, FileNotFound} //
             & fn::expected<void, Error>{})
                .error()
            == FileNotFound);
      CHECK((fn::expected<fn::pack<double, bool>, Error>{std::in_place, fn::pack<double, bool>{0.5, true}} //
             & fn::expected<void, Error>{std::unexpect, Unknown})
                .error()
            == Unknown);
      CHECK((fn::expected<fn::pack<double, bool>, Error>{std::unexpect, FileNotFound} //
             & fn::expected<void, Error>{std::unexpect, Unknown})
                .error()
            == FileNotFound);
    }

    WHEN("void & pack yield pack")
    {
      static_assert(std::same_as<decltype(std::declval<fn::expected<void, Error>>()
                                          & std::declval<fn::expected<fn::pack<double, bool>, Error>>()),
                                 fn::expected<fn::pack<double, bool>, Error>>);

      CHECK((fn::expected<void, Error>{} //
             & fn::expected<fn::pack<double, bool>, Error>{std::in_place, fn::pack<double, bool>{0.5, true}})
                .transform([](double d, bool b) constexpr -> bool { return d == 0.5 && b; })
                .value());
      CHECK((fn::expected<void, Error>{std::unexpect, FileNotFound} //
             & fn::expected<fn::pack<double, bool>, Error>{std::in_place, fn::pack<double, bool>{0.5, true}})
                .error()
            == FileNotFound);
      CHECK((fn::expected<void, Error>{} //
             & fn::expected<fn::pack<double, bool>, Error>{std::unexpect, Unknown})
                .error()
            == Unknown);
      CHECK((fn::expected<void, Error>{std::unexpect, FileNotFound} //
             & fn::expected<fn::pack<double, bool>, Error>{std::unexpect, Unknown})
                .error()
            == FileNotFound);
    }

    WHEN("value & pack is unsupported")
    {
      static_assert(
          [](auto &&lh,                                                          //
             auto &&rh) constexpr -> bool { return not requires { lh & rh; }; }( //
                                      fn::expected<int, Error>{12},
                                      fn::expected<fn::pack<double, bool>, Error>{fn::pack<double, bool>{0.5, true}}));
    }
    WHEN("pack & pack is unsupported")
    {
      static_assert(
          [](auto &&lh,
             auto &&rh) constexpr -> bool { return not requires { lh & rh; }; }( //
                                      fn::expected<fn::pack<double, bool>, Error>{fn::pack<double, bool>{0.5, true}},
                                      fn::expected<fn::pack<double, bool>, Error>{fn::pack<double, bool>{0.5, true}}));
    }
  }
}

TEST_CASE("expected sum support and_then", "[expected][sum][and_then]")
{
  WHEN("value")
  {
    fn::expected<fn::sum<int, std::string_view>, Error> s{fn::sum{12}};

    CHECK(s.and_then( //
               fn::overload([](int &i) -> fn::expected<bool, Error> { return i == 12; },
                            [](int const &) -> fn::expected<bool, Error> { throw 0; },
                            [](int &&) -> fn::expected<bool, Error> { throw 0; },
                            [](int const &&) -> fn::expected<bool, Error> { throw 0; },
                            [](std::string_view &) -> fn::expected<bool, Error> { throw 0; },
                            [](std::string_view const &) -> fn::expected<bool, Error> { throw 0; },
                            [](std::string_view &&) -> fn::expected<bool, Error> { throw 0; },
                            [](std::string_view const &&) -> fn::expected<bool, Error> { throw 0; }))
              .value());

    CHECK(s.and_then( //
               fn::overload([](std::in_place_type_t<int>, int &i) -> fn::expected<bool, Error> { return i == 12; },
                            [](std::in_place_type_t<int>, int const &) -> fn::expected<bool, Error> { throw 0; },
                            [](std::in_place_type_t<int>, int &&) -> fn::expected<bool, Error> { throw 0; },
                            [](std::in_place_type_t<int>, int const &&) -> fn::expected<bool, Error> { throw 0; },
                            [](std::in_place_type_t<std::string_view>,
                               std::string_view &) -> fn::expected<bool, Error> { throw 0; },
                            [](std::in_place_type_t<std::string_view>,
                               std::string_view const &) -> fn::expected<bool, Error> { throw 0; },
                            [](std::in_place_type_t<std::string_view>,
                               std::string_view &&) -> fn::expected<bool, Error> { throw 0; },
                            [](std::in_place_type_t<std::string_view>,
                               std::string_view const &&) -> fn::expected<bool, Error> { throw 0; }))
              .value());

    CHECK(std::as_const(s)
              .and_then( //
                  fn::overload([](int &) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &i) -> fn::expected<bool, Error> { return i == 12; },
                               [](int &&) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &&) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view &) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view const &) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view &&) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view const &&) -> fn::expected<bool, Error> { throw 0; }))
              .value());

    CHECK(std::as_const(s)
              .and_then( //
                  fn::overload(
                      [](std::in_place_type_t<int>, int &) -> fn::expected<bool, Error> { throw 0; },
                      [](std::in_place_type_t<int>, int const &i) -> fn::expected<bool, Error> { return i == 12; },
                      [](std::in_place_type_t<int>, int &&) -> fn::expected<bool, Error> { throw 0; },
                      [](std::in_place_type_t<int>, int const &&) -> fn::expected<bool, Error> { throw 0; },
                      [](std::in_place_type_t<std::string_view>, std::string_view &) -> fn::expected<bool, Error> {
                        throw 0;
                      },
                      [](std::in_place_type_t<std::string_view>,
                         std::string_view const &) -> fn::expected<bool, Error> { throw 0; },
                      [](std::in_place_type_t<std::string_view>, std::string_view &&) -> fn::expected<bool, Error> {
                        throw 0;
                      },
                      [](std::in_place_type_t<std::string_view>,
                         std::string_view const &&) -> fn::expected<bool, Error> { throw 0; }))
              .value());

    CHECK(fn::expected<fn::sum<int, std::string_view>, Error>{fn::sum{12}}
              .and_then( //
                  fn::overload([](int &) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &) -> fn::expected<bool, Error> { throw 0; },
                               [](int &&i) -> fn::expected<bool, Error> { return i == 12; },
                               [](int const &&) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view &) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view const &) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view &&) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view const &&) -> fn::expected<bool, Error> { throw 0; }))
              .value());

    CHECK(fn::expected<fn::sum<int, std::string_view>, Error>{fn::sum{12}}
              .and_then( //
                  fn::overload([](std::in_place_type_t<int>, int &) -> fn::expected<bool, Error> { throw 0; },
                               [](std::in_place_type_t<int>, int const &) -> fn::expected<bool, Error> { throw 0; },
                               [](std::in_place_type_t<int>, int &&i) -> fn::expected<bool, Error> { return i == 12; },
                               [](std::in_place_type_t<int>, int const &&) -> fn::expected<bool, Error> { throw 0; },
                               [](std::in_place_type_t<std::string_view>,
                                  std::string_view &) -> fn::expected<bool, Error> { throw 0; },
                               [](std::in_place_type_t<std::string_view>,
                                  std::string_view const &) -> fn::expected<bool, Error> { throw 0; },
                               [](std::in_place_type_t<std::string_view>,
                                  std::string_view &&) -> fn::expected<bool, Error> { throw 0; },
                               [](std::in_place_type_t<std::string_view>,
                                  std::string_view const &&) -> fn::expected<bool, Error> { throw 0; }))
              .value());

    CHECK(std::move(std::as_const(s))
              .and_then( //
                  fn::overload([](int &) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &) -> fn::expected<bool, Error> { throw 0; },
                               [](int &&) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &&i) -> fn::expected<bool, Error> { return i == 12; },
                               [](std::string_view &) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view const &) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view &&) -> fn::expected<bool, Error> { throw 0; },
                               [](std::string_view const &&) -> fn::expected<bool, Error> { throw 0; }))
              .value());

    CHECK(std::move(std::as_const(s))
              .and_then( //
                  fn::overload(
                      [](std::in_place_type_t<int>, int &) -> fn::expected<bool, Error> { throw 0; },
                      [](std::in_place_type_t<int>, int const &) -> fn::expected<bool, Error> { throw 0; },
                      [](std::in_place_type_t<int>, int &&) -> fn::expected<bool, Error> { throw 0; },
                      [](std::in_place_type_t<int>, int const &&i) -> fn::expected<bool, Error> { return i == 12; },
                      [](std::in_place_type_t<std::string_view>, std::string_view &) -> fn::expected<bool, Error> {
                        throw 0;
                      },
                      [](std::in_place_type_t<std::string_view>,
                         std::string_view const &) -> fn::expected<bool, Error> { throw 0; },
                      [](std::in_place_type_t<std::string_view>, std::string_view &&) -> fn::expected<bool, Error> {
                        throw 0;
                      },
                      [](std::in_place_type_t<std::string_view>,
                         std::string_view const &&) -> fn::expected<bool, Error> { throw 0; }))
              .value());
  }

  WHEN("error")
  {
    fn::expected<fn::sum<int, std::string_view>, Error> s{std::unexpect, FileNotFound};
    CHECK(s.and_then( //
               [](auto...) -> fn::expected<bool, Error> { return {true}; })
              .error()
          == FileNotFound);
    CHECK(std::as_const(s)
              .and_then( //
                  [](auto...) -> fn::expected<bool, Error> { return {true}; })
              .error()
          == FileNotFound);
    CHECK(fn::expected<fn::sum<int, std::string_view>, Error>{std::unexpect, FileNotFound}
              .and_then( //
                  [](auto...) -> fn::expected<bool, Error> { return {true}; })
              .error()
          == FileNotFound);
    CHECK(std::move(std::as_const(s))
              .and_then( //
                  [](auto...) -> fn::expected<bool, Error> { return {true}; })
              .error()
          == FileNotFound);
  }

  WHEN("constexpr")
  {
    constexpr auto fn = fn::overload([](int &) -> fn::expected<bool, Error> { throw 0; },
                                     [](int const &i) -> fn::expected<bool, Error> { return i == 42; },
                                     [](int &&) -> fn::expected<bool, Error> { throw 0; },
                                     [](int const &&) -> fn::expected<bool, Error> { throw 0; },
                                     [](std::string_view &) -> fn::expected<bool, Error> { throw 0; },
                                     [](std::string_view const &) -> fn::expected<bool, Error> { throw 0; },
                                     [](std::string_view &&) -> fn::expected<bool, Error> { throw 0; },
                                     [](std::string_view const &&) -> fn::expected<bool, Error> { throw 0; });
    constexpr fn::expected<fn::sum<int, std::string_view>, Error> a{fn::sum{42}};
    static_assert(std::is_same_v<decltype(a.and_then(fn)), fn::expected<bool, Error>>);
    static_assert(a.and_then(fn).value());
  }
}

TEST_CASE("expected sum support or_else", "[expected][sum][or_else]")
{
  WHEN("value")
  {
    fn::expected<double, fn::sum<int, std::string_view>> s{std::unexpect, fn::sum{12}};

    CHECK(s.or_else( //
               fn::overload([](int &i) -> fn::expected<double, Error> { return {i}; },
                            [](int const &) -> fn::expected<double, Error> { throw 0; },
                            [](int &&) -> fn::expected<double, Error> { throw 0; },
                            [](int const &&) -> fn::expected<double, Error> { throw 0; },
                            [](std::string_view &) -> fn::expected<double, Error> { throw 0; },
                            [](std::string_view const &) -> fn::expected<double, Error> { throw 0; },
                            [](std::string_view &&) -> fn::expected<double, Error> { throw 0; },
                            [](std::string_view const &&) -> fn::expected<double, Error> { throw 0; }))
              .value()
          == 12);

    CHECK(s.or_else( //
               fn::overload([](std::in_place_type_t<int>, int &i) -> fn::expected<double, Error> { return {i}; },
                            [](std::in_place_type_t<int>, int const &) -> fn::expected<double, Error> { throw 0; },
                            [](std::in_place_type_t<int>, int &&) -> fn::expected<double, Error> { throw 0; },
                            [](std::in_place_type_t<int>, int const &&) -> fn::expected<double, Error> { throw 0; },
                            [](std::in_place_type_t<std::string_view>,
                               std::string_view &) -> fn::expected<double, Error> { throw 0; },
                            [](std::in_place_type_t<std::string_view>,
                               std::string_view const &) -> fn::expected<double, Error> { throw 0; },
                            [](std::in_place_type_t<std::string_view>,
                               std::string_view &&) -> fn::expected<double, Error> { throw 0; },
                            [](std::in_place_type_t<std::string_view>,
                               std::string_view const &&) -> fn::expected<double, Error> { throw 0; }))
              .value()
          == 12);

    CHECK(std::as_const(s)
              .or_else( //
                  fn::overload([](int &) -> fn::expected<double, Error> { throw 0; },
                               [](int const &i) -> fn::expected<double, Error> { return {i}; },
                               [](int &&) -> fn::expected<double, Error> { throw 0; },
                               [](int const &&) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view &) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view const &) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view &&) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view const &&) -> fn::expected<double, Error> { throw 0; }))
              .value()
          == 12);

    CHECK(
        std::as_const(s)
            .or_else( //
                fn::overload([](std::in_place_type_t<int>, int &) -> fn::expected<double, Error> { throw 0; },
                             [](std::in_place_type_t<int>, int const &i) -> fn::expected<double, Error> { return {i}; },
                             [](std::in_place_type_t<int>, int &&) -> fn::expected<double, Error> { throw 0; },
                             [](std::in_place_type_t<int>, int const &&) -> fn::expected<double, Error> { throw 0; },
                             [](std::in_place_type_t<std::string_view>,
                                std::string_view &) -> fn::expected<double, Error> { throw 0; },
                             [](std::in_place_type_t<std::string_view>,
                                std::string_view const &) -> fn::expected<double, Error> { throw 0; },
                             [](std::in_place_type_t<std::string_view>,
                                std::string_view &&) -> fn::expected<double, Error> { throw 0; },
                             [](std::in_place_type_t<std::string_view>,
                                std::string_view const &&) -> fn::expected<double, Error> { throw 0; }))
            .value()
        == 12);

    CHECK(fn::expected<double, fn::sum<int, std::string_view>>{std::unexpect, fn::sum{12}}
              .or_else( //
                  fn::overload([](int &) -> fn::expected<double, Error> { throw 0; },
                               [](int const &) -> fn::expected<double, Error> { throw 0; },
                               [](int &&i) -> fn::expected<double, Error> { return {i}; },
                               [](int const &&) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view &) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view const &) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view &&) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view const &&) -> fn::expected<double, Error> { throw 0; }))
              .value()
          == 12);

    CHECK(fn::expected<double, fn::sum<int, std::string_view>>{std::unexpect, fn::sum{12}}
              .or_else( //
                  fn::overload([](std::in_place_type_t<int>, int &) -> fn::expected<double, Error> { throw 0; },
                               [](std::in_place_type_t<int>, int const &) -> fn::expected<double, Error> { throw 0; },
                               [](std::in_place_type_t<int>, int &&i) -> fn::expected<double, Error> { return {i}; },
                               [](std::in_place_type_t<int>, int const &&) -> fn::expected<double, Error> { throw 0; },
                               [](std::in_place_type_t<std::string_view>,
                                  std::string_view &) -> fn::expected<double, Error> { throw 0; },
                               [](std::in_place_type_t<std::string_view>,
                                  std::string_view const &) -> fn::expected<double, Error> { throw 0; },
                               [](std::in_place_type_t<std::string_view>,
                                  std::string_view &&) -> fn::expected<double, Error> { throw 0; },
                               [](std::in_place_type_t<std::string_view>,
                                  std::string_view const &&) -> fn::expected<double, Error> { throw 0; }))
              .value()
          == 12);

    CHECK(std::move(std::as_const(s))
              .or_else( //
                  fn::overload([](int &) -> fn::expected<double, Error> { throw 0; },
                               [](int const &) -> fn::expected<double, Error> { throw 0; },
                               [](int &&) -> fn::expected<double, Error> { throw 0; },
                               [](int const &&i) -> fn::expected<double, Error> { return {i}; },
                               [](std::string_view &) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view const &) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view &&) -> fn::expected<double, Error> { throw 0; },
                               [](std::string_view const &&) -> fn::expected<double, Error> { throw 0; }))
              .value()
          == 12);

    CHECK(std::move(std::as_const(s))
              .or_else( //
                  fn::overload(
                      [](std::in_place_type_t<int>, int &) -> fn::expected<double, Error> { throw 0; },
                      [](std::in_place_type_t<int>, int const &) -> fn::expected<double, Error> { throw 0; },
                      [](std::in_place_type_t<int>, int &&) -> fn::expected<double, Error> { throw 0; },
                      [](std::in_place_type_t<int>, int const &&i) -> fn::expected<double, Error> { return {i}; },
                      [](std::in_place_type_t<std::string_view>, std::string_view &) -> fn::expected<double, Error> {
                        throw 0;
                      },
                      [](std::in_place_type_t<std::string_view>,
                         std::string_view const &) -> fn::expected<double, Error> { throw 0; },
                      [](std::in_place_type_t<std::string_view>, std::string_view &&) -> fn::expected<double, Error> {
                        throw 0;
                      },
                      [](std::in_place_type_t<std::string_view>,
                         std::string_view const &&) -> fn::expected<double, Error> { throw 0; }))
              .value()
          == 12);

    WHEN("value")
    {
      fn::expected<double, fn::sum<int, std::string_view>> s{1.5};
      CHECK(s.or_else( //
                 [](auto...) -> fn::expected<double, Error> { throw 0; })
                .value()
            == 1.5);
      CHECK(std::as_const(s)
                .or_else( //
                    [](auto...) -> fn::expected<double, Error> { throw 0; })
                .value()
            == 1.5);
      CHECK(fn::expected<double, fn::sum<int, std::string_view>>{1.5}
                .or_else( //
                    [](auto...) -> fn::expected<double, Error> { throw 0; })
                .value()
            == 1.5);
      CHECK(std::move(std::as_const(s))
                .or_else( //
                    [](auto...) -> fn::expected<double, Error> { throw 0; })
                .value()
            == 1.5);
    }

    WHEN("constexpr")
    {
      constexpr auto fn = fn::overload([](int &) -> fn::expected<double, Error> { throw 0; },
                                       [](int const &i) -> fn::expected<double, Error> { return {i}; },
                                       [](int &&) -> fn::expected<double, Error> { throw 0; },
                                       [](int const &&) -> fn::expected<double, Error> { throw 0; },
                                       [](std::string_view &) -> fn::expected<double, Error> { throw 0; },
                                       [](std::string_view const &) -> fn::expected<double, Error> { throw 0; },
                                       [](std::string_view &&) -> fn::expected<double, Error> { throw 0; },
                                       [](std::string_view const &&) -> fn::expected<double, Error> { throw 0; });
      constexpr fn::expected<double, fn::sum<int, std::string_view>> a{std::unexpect, fn::sum{42}};
      static_assert(std::is_same_v<decltype(a.or_else(fn)), fn::expected<double, Error>>);
      static_assert(a.or_else(fn).value() == 42);
    }
  }

  WHEN("void")
  {
    fn::expected<void, fn::sum<int, std::string_view>> s{std::unexpect, fn::sum{12}};

    CHECK(s.or_else( //
               fn::overload([](int &) -> fn::expected<void, Error> { return std::unexpected<Error>{FileNotFound}; },
                            [](int const &) -> fn::expected<void, Error> { throw 0; },
                            [](int &&) -> fn::expected<void, Error> { throw 0; },
                            [](int const &&) -> fn::expected<void, Error> { throw 0; },
                            [](std::string_view &) -> fn::expected<void, Error> { throw 0; },
                            [](std::string_view const &) -> fn::expected<void, Error> { throw 0; },
                            [](std::string_view &&) -> fn::expected<void, Error> { throw 0; },
                            [](std::string_view const &&) -> fn::expected<void, Error> { throw 0; }))
              .error()
          == FileNotFound);

    CHECK(s.or_else( //
               fn::overload([](std::in_place_type_t<int>,
                               int &) -> fn::expected<void, Error> { return std::unexpected<Error>{FileNotFound}; },
                            [](std::in_place_type_t<int>, int const &) -> fn::expected<void, Error> { throw 0; },
                            [](std::in_place_type_t<int>, int &&) -> fn::expected<void, Error> { throw 0; },
                            [](std::in_place_type_t<int>, int const &&) -> fn::expected<void, Error> { throw 0; },
                            [](std::in_place_type_t<std::string_view>,
                               std::string_view &) -> fn::expected<void, Error> { throw 0; },
                            [](std::in_place_type_t<std::string_view>,
                               std::string_view const &) -> fn::expected<void, Error> { throw 0; },
                            [](std::in_place_type_t<std::string_view>,
                               std::string_view &&) -> fn::expected<void, Error> { throw 0; },
                            [](std::in_place_type_t<std::string_view>,
                               std::string_view const &&) -> fn::expected<void, Error> { throw 0; }))
              .error()
          == FileNotFound);

    CHECK(std::as_const(s)
              .or_else( //
                  fn::overload(
                      [](int &) -> fn::expected<void, Error> { throw 0; },
                      [](int const &) -> fn::expected<void, Error> { return std::unexpected<Error>{FileNotFound}; },
                      [](int &&) -> fn::expected<void, Error> { throw 0; },
                      [](int const &&) -> fn::expected<void, Error> { throw 0; },
                      [](std::string_view &) -> fn::expected<void, Error> { throw 0; },
                      [](std::string_view const &) -> fn::expected<void, Error> { throw 0; },
                      [](std::string_view &&) -> fn::expected<void, Error> { throw 0; },
                      [](std::string_view const &&) -> fn::expected<void, Error> { throw 0; }))
              .error()
          == FileNotFound);

    CHECK(std::as_const(s)
              .or_else( //
                  fn::overload([](std::in_place_type_t<int>, int &) -> fn::expected<void, Error> { throw 0; },
                               [](std::in_place_type_t<int>, int const &) -> fn::expected<void, Error> {
                                 return std::unexpected<Error>{FileNotFound};
                               },
                               [](std::in_place_type_t<int>, int &&) -> fn::expected<void, Error> { throw 0; },
                               [](std::in_place_type_t<int>, int const &&) -> fn::expected<void, Error> { throw 0; },
                               [](std::in_place_type_t<std::string_view>,
                                  std::string_view &) -> fn::expected<void, Error> { throw 0; },
                               [](std::in_place_type_t<std::string_view>,
                                  std::string_view const &) -> fn::expected<void, Error> { throw 0; },
                               [](std::in_place_type_t<std::string_view>,
                                  std::string_view &&) -> fn::expected<void, Error> { throw 0; },
                               [](std::in_place_type_t<std::string_view>,
                                  std::string_view const &&) -> fn::expected<void, Error> { throw 0; }))
              .error()
          == FileNotFound);

    CHECK(fn::expected<void, fn::sum<int, std::string_view>>{std::unexpect, fn::sum{12}}
              .or_else( //
                  fn::overload([](int &) -> fn::expected<void, Error> { throw 0; },
                               [](int const &) -> fn::expected<void, Error> { throw 0; },
                               [](int &&) -> fn::expected<void, Error> { return std::unexpected<Error>{FileNotFound}; },
                               [](int const &&) -> fn::expected<void, Error> { throw 0; },
                               [](std::string_view &) -> fn::expected<void, Error> { throw 0; },
                               [](std::string_view const &) -> fn::expected<void, Error> { throw 0; },
                               [](std::string_view &&) -> fn::expected<void, Error> { throw 0; },
                               [](std::string_view const &&) -> fn::expected<void, Error> { throw 0; }))
              .error()
          == FileNotFound);

    CHECK(fn::expected<void, fn::sum<int, std::string_view>>{std::unexpect, fn::sum{12}}
              .or_else( //
                  fn::overload([](std::in_place_type_t<int>, int &) -> fn::expected<void, Error> { throw 0; },
                               [](std::in_place_type_t<int>, int const &) -> fn::expected<void, Error> { throw 0; },
                               [](std::in_place_type_t<int>, int &&) -> fn::expected<void, Error> {
                                 return std::unexpected<Error>{FileNotFound};
                               },
                               [](std::in_place_type_t<int>, int const &&) -> fn::expected<void, Error> { throw 0; },
                               [](std::in_place_type_t<std::string_view>,
                                  std::string_view &) -> fn::expected<void, Error> { throw 0; },
                               [](std::in_place_type_t<std::string_view>,
                                  std::string_view const &) -> fn::expected<void, Error> { throw 0; },
                               [](std::in_place_type_t<std::string_view>,
                                  std::string_view &&) -> fn::expected<void, Error> { throw 0; },
                               [](std::in_place_type_t<std::string_view>,
                                  std::string_view const &&) -> fn::expected<void, Error> { throw 0; }))
              .error()
          == FileNotFound);

    CHECK(std::move(std::as_const(s))
              .or_else( //
                  fn::overload(
                      [](int &) -> fn::expected<void, Error> { throw 0; },
                      [](int const &) -> fn::expected<void, Error> { throw 0; },
                      [](int &&) -> fn::expected<void, Error> { throw 0; },
                      [](int const &&) -> fn::expected<void, Error> { return std::unexpected<Error>{FileNotFound}; },
                      [](std::string_view &) -> fn::expected<void, Error> { throw 0; },
                      [](std::string_view const &) -> fn::expected<void, Error> { throw 0; },
                      [](std::string_view &&) -> fn::expected<void, Error> { throw 0; },
                      [](std::string_view const &&) -> fn::expected<void, Error> { throw 0; }))
              .error()
          == FileNotFound);

    CHECK(std::move(std::as_const(s))
              .or_else( //
                  fn::overload([](std::in_place_type_t<int>, int &) -> fn::expected<void, Error> { throw 0; },
                               [](std::in_place_type_t<int>, int const &) -> fn::expected<void, Error> { throw 0; },
                               [](std::in_place_type_t<int>, int &&) -> fn::expected<void, Error> { throw 0; },
                               [](std::in_place_type_t<int>, int const &&) -> fn::expected<void, Error> {
                                 return std::unexpected<Error>{FileNotFound};
                               },
                               [](std::in_place_type_t<std::string_view>,
                                  std::string_view &) -> fn::expected<void, Error> { throw 0; },
                               [](std::in_place_type_t<std::string_view>,
                                  std::string_view const &) -> fn::expected<void, Error> { throw 0; },
                               [](std::in_place_type_t<std::string_view>,
                                  std::string_view &&) -> fn::expected<void, Error> { throw 0; },
                               [](std::in_place_type_t<std::string_view>,
                                  std::string_view const &&) -> fn::expected<void, Error> { throw 0; }))
              .error()
          == FileNotFound);

    WHEN("value")
    {
      fn::expected<void, fn::sum<int, std::string_view>> s{};
      CHECK(s.or_else( //
                 [](auto...) -> fn::expected<void, Error> { throw 0; })
                .has_value());
      CHECK(std::as_const(s)
                .or_else( //
                    [](auto...) -> fn::expected<void, Error> { throw 0; })
                .has_value());
      CHECK(fn::expected<void, fn::sum<int, std::string_view>>{}
                .or_else( //
                    [](auto...) -> fn::expected<void, Error> { throw 0; })
                .has_value());
      CHECK(std::move(std::as_const(s))
                .or_else( //
                    [](auto...) -> fn::expected<void, Error> { throw 0; })
                .has_value());
    }

    WHEN("constexpr")
    {
      constexpr auto fn
          = fn::overload([](int &) -> fn::expected<void, Error> { throw 0; },
                         [](int const &) -> fn::expected<void, Error> { return std::unexpected<Error>{FileNotFound}; },
                         [](int &&) -> fn::expected<void, Error> { throw 0; },
                         [](int const &&) -> fn::expected<void, Error> { throw 0; },
                         [](std::string_view &) -> fn::expected<void, Error> { throw 0; },
                         [](std::string_view const &) -> fn::expected<void, Error> { throw 0; },
                         [](std::string_view &&) -> fn::expected<void, Error> { throw 0; },
                         [](std::string_view const &&) -> fn::expected<void, Error> { throw 0; });
      constexpr fn::expected<void, fn::sum<int, std::string_view>> a{std::unexpect, fn::sum{42}};
      static_assert(std::is_same_v<decltype(a.or_else(fn)), fn::expected<void, Error>>);
      static_assert(a.or_else(fn).error() == FileNotFound);
    }
  }
}

TEST_CASE("expected sum support transform", "[expected][sum][transform]")
{
  WHEN("value")
  {
    fn::expected<fn::sum<int, std::string_view>, Error> s{fn::sum{12}};

    CHECK(s.transform( //
               fn::overload(
                   [](int &) -> std::monostate { return {}; }, [](int const &) -> std::monostate { throw 0; },
                   [](int &&) -> std::monostate { throw 0; }, [](int const &&) -> std::monostate { throw 0; },
                   [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                   [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }))
              .value()
              .has_value<std::monostate>());

    CHECK(s.transform( //
               fn::overload([](std::in_place_type_t<int>, int &) -> std::monostate { return {}; },
                            [](std::in_place_type_t<int>, int const &) -> std::monostate { throw 0; },
                            [](std::in_place_type_t<int>, int &&) -> std::monostate { throw 0; },
                            [](std::in_place_type_t<int>, int const &&) -> std::monostate { throw 0; },
                            [](std::in_place_type_t<std::string_view>, std::string_view &) -> int { throw 0; },
                            [](std::in_place_type_t<std::string_view>, std::string_view const &) -> int { throw 0; },
                            [](std::in_place_type_t<std::string_view>, std::string_view &&) -> int { throw 0; },
                            [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> int { throw 0; }))
              .value()
              .has_value<std::monostate>());

    CHECK(std::as_const(s)
              .transform( //
                  fn::overload(
                      [](int &) -> std::monostate { throw 0; }, [](int const &) -> std::monostate { return {}; },
                      [](int &&) -> std::monostate { throw 0; }, [](int const &&) -> std::monostate { throw 0; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }))
              .value()
              .has_value<std::monostate>());

    CHECK(
        std::as_const(s)
            .transform( //
                fn::overload([](std::in_place_type_t<int>, int &) -> std::monostate { throw 0; },
                             [](std::in_place_type_t<int>, int const &) -> std::monostate { return {}; },
                             [](std::in_place_type_t<int>, int &&) -> std::monostate { throw 0; },
                             [](std::in_place_type_t<int>, int const &&) -> std::monostate { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view &) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view const &) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view &&) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> int { throw 0; }))
            .value()
            .has_value<std::monostate>());

    CHECK(fn::expected<fn::sum<int, std::string_view>, Error>{fn::sum{12}}
              .transform( //
                  fn::overload(
                      [](int &) -> std::monostate { throw 0; }, [](int const &) -> std::monostate { throw 0; },
                      [](int &&) -> std::monostate { return {}; }, [](int const &&) -> std::monostate { throw 0; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }))
              .value()
              .has_value<std::monostate>());

    CHECK(
        fn::expected<fn::sum<int, std::string_view>, Error>{fn::sum{12}}
            .transform( //
                fn::overload([](std::in_place_type_t<int>, int &) -> std::monostate { throw 0; },
                             [](std::in_place_type_t<int>, int const &) -> std::monostate { throw 0; },
                             [](std::in_place_type_t<int>, int &&) -> std::monostate { return {}; },
                             [](std::in_place_type_t<int>, int const &&) -> std::monostate { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view &) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view const &) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view &&) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> int { throw 0; }))
            .value()
            .has_value<std::monostate>());

    CHECK(std::move(std::as_const(s))
              .transform( //
                  fn::overload(
                      [](int &) -> std::monostate { throw 0; }, [](int const &) -> std::monostate { throw 0; },
                      [](int &&) -> std::monostate { throw 0; }, [](int const &&) -> std::monostate { return {}; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }))
              .value()
              .has_value<std::monostate>());

    CHECK(
        std::move(std::as_const(s))
            .transform( //
                fn::overload([](std::in_place_type_t<int>, int &) -> std::monostate { throw 0; },
                             [](std::in_place_type_t<int>, int const &) -> std::monostate { throw 0; },
                             [](std::in_place_type_t<int>, int &&) -> std::monostate { throw 0; },
                             [](std::in_place_type_t<int>, int const &&) -> std::monostate { return {}; },
                             [](std::in_place_type_t<std::string_view>, std::string_view &) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view const &) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view &&) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> int { throw 0; }))
            .value()
            .has_value<std::monostate>());
  }

  WHEN("error")
  {
    fn::expected<fn::sum<int, std::string_view>, Error> s{std::unexpect, FileNotFound};
    CHECK(s.transform( //
               [](auto...) -> std::monostate { throw 0; })
              .error()
          == FileNotFound);
    CHECK(std::as_const(s)
              .transform( //
                  [](auto...) -> std::monostate { throw 0; })
              .error()
          == FileNotFound);
    CHECK(fn::expected<fn::sum<int, std::string_view>, Error>{std::unexpect, FileNotFound}
              .transform( //
                  [](auto...) -> std::monostate { throw 0; })
              .error()
          == FileNotFound);
    CHECK(std::move(std::as_const(s))
              .transform( //
                  [](auto...) -> std::monostate { throw 0; })
              .error()
          == FileNotFound);
  }

  WHEN("constexpr")
  {
    constexpr auto fn
        = fn::overload([](int &) -> bool { throw 0; }, [](int const &) -> bool { return true; },
                       [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; },
                       [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                       [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; });
    constexpr fn::expected<fn::sum<int, std::string_view>, Error> a{fn::sum{42}};
    static_assert(std::is_same_v<decltype(a.transform(fn)), fn::expected<fn::sum<bool, int>, Error>>);
    // TODO Switch bool to std::monostate or similar user-defined type
    static_assert(a.transform(fn).value().has_value<bool>());
  }
}

TEST_CASE("expected sum support transform_error", "[expected][sum][transform_error]")
{
  WHEN("value")
  {
    fn::expected<double, fn::sum<int, std::string_view>> s{std::unexpect, fn::sum{12}};

    CHECK(s.transform_error( //
               fn::overload(
                   [](int &i) -> bool { return i == 12; }, [](int const &) -> bool { throw 0; },
                   [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; },
                   [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                   [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }))
              .error()
          == fn::sum{true});

    CHECK(s.transform_error( //
               fn::overload([](std::in_place_type_t<int>, int &i) -> bool { return i == 12; },
                            [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                            [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                            [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; },
                            [](std::in_place_type_t<std::string_view>, std::string_view &) -> int { throw 0; },
                            [](std::in_place_type_t<std::string_view>, std::string_view const &) -> int { throw 0; },
                            [](std::in_place_type_t<std::string_view>, std::string_view &&) -> int { throw 0; },
                            [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> int { throw 0; }))
              .error()
          == fn::sum{true});

    CHECK(std::as_const(s)
              .transform_error( //
                  fn::overload(
                      [](int &) -> bool { throw 0; }, [](int const &i) -> bool { return i == 12; },
                      [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }))
              .error()
          == fn::sum{true});

    CHECK(
        std::as_const(s)
            .transform_error( //
                fn::overload([](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                             [](std::in_place_type_t<int>, int const &i) -> bool { return i == 12; },
                             [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                             [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view &) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view const &) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view &&) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> int { throw 0; }))
            .error()
        == fn::sum{true});

    CHECK(fn::expected<double, fn::sum<int, std::string_view>>{std::unexpect, fn::sum{12}}
              .transform_error( //
                  fn::overload(
                      [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                      [](int &&i) -> bool { return i == 12; }, [](int const &&) -> bool { throw 0; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }))
              .error()
          == fn::sum{true});

    CHECK(
        fn::expected<double, fn::sum<int, std::string_view>>{std::unexpect, fn::sum{12}}
            .transform_error( //
                fn::overload([](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                             [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                             [](std::in_place_type_t<int>, int &&i) -> bool { return i == 12; },
                             [](std::in_place_type_t<int>, int const &&) -> bool { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view &) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view const &) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view &&) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> int { throw 0; }))
            .error()
        == fn::sum{true});

    CHECK(std::move(std::as_const(s))
              .transform_error( //
                  fn::overload(
                      [](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                      [](int &&) -> bool { throw 0; }, [](int const &&i) -> bool { return i == 12; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }))
              .error()
          == fn::sum{true});

    CHECK(
        std::move(std::as_const(s))
            .transform_error( //
                fn::overload([](std::in_place_type_t<int>, int &) -> bool { throw 0; },
                             [](std::in_place_type_t<int>, int const &) -> bool { throw 0; },
                             [](std::in_place_type_t<int>, int &&) -> bool { throw 0; },
                             [](std::in_place_type_t<int>, int const &&i) -> bool { return i == 12; },
                             [](std::in_place_type_t<std::string_view>, std::string_view &) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view const &) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view &&) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> int { throw 0; }))
            .error()
        == fn::sum{true});

    WHEN("value")
    {
      fn::expected<double, fn::sum<int, std::string_view>> s{1.5};
      CHECK(s.transform_error( //
                 [](auto...) -> bool { throw 0; })
                .value()
            == 1.5);
      CHECK(std::as_const(s)
                .transform_error( //
                    [](auto...) -> bool { throw 0; })
                .value()
            == 1.5);
      CHECK(fn::expected<double, fn::sum<int, std::string_view>>{1.5}
                .transform_error( //
                    [](auto...) -> bool { throw 0; })
                .value()
            == 1.5);
      CHECK(std::move(std::as_const(s))
                .transform_error( //
                    [](auto...) -> bool { throw 0; })
                .value()
            == 1.5);
    }

    WHEN("constexpr")
    {
      constexpr auto fn = fn::overload(
          [](int &) -> bool { throw 0; }, [](int const &i) -> bool { return i == 42; }, [](int &&) -> bool { throw 0; },
          [](int const &&) -> bool { throw 0; }, [](std::string_view &) -> int { throw 0; },
          [](std::string_view const &) -> int { throw 0; }, [](std::string_view &&) -> int { throw 0; },
          [](std::string_view const &&) -> int { throw 0; });
      constexpr fn::expected<double, fn::sum<int, std::string_view>> a{std::unexpect, fn::sum{42}};
      static_assert(std::is_same_v<decltype(a.transform_error(fn)), fn::expected<double, fn::sum<bool, int>>>);
      static_assert(a.transform_error(fn).error() == fn::sum{true});
    }
  }

  WHEN("void")
  {
    fn::expected<void, fn::sum<int, std::string_view>> s{std::unexpect, fn::sum{12}};

    CHECK(s.transform_error( //
               fn::overload(
                   [](int &i) -> int { return i; }, [](int const &) -> int { throw 0; }, [](int &&) -> int { throw 0; },
                   [](int const &&) -> int { throw 0; }, [](std::string_view &) -> int { throw 0; },
                   [](std::string_view const &) -> int { throw 0; }, [](std::string_view &&) -> int { throw 0; },
                   [](std::string_view const &&) -> int { throw 0; }))
              .error()
          == fn::sum{12});

    CHECK(s.transform_error( //
               fn::overload([](std::in_place_type_t<int>, int &i) -> int { return i; },
                            [](std::in_place_type_t<int>, int const &) -> int { throw 0; },
                            [](std::in_place_type_t<int>, int &&) -> int { throw 0; },
                            [](std::in_place_type_t<int>, int const &&) -> int { throw 0; },
                            [](std::in_place_type_t<std::string_view>, std::string_view &) -> int { throw 0; },
                            [](std::in_place_type_t<std::string_view>, std::string_view const &) -> int { throw 0; },
                            [](std::in_place_type_t<std::string_view>, std::string_view &&) -> int { throw 0; },
                            [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> int { throw 0; }))
              .error()
          == fn::sum{12});

    CHECK(std::as_const(s)
              .transform_error( //
                  fn::overload(
                      [](int &) -> int { throw 0; }, [](int const &i) -> int { return i; },
                      [](int &&) -> int { throw 0; }, [](int const &&) -> int { throw 0; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }))
              .error()
          == fn::sum{12});

    CHECK(
        std::as_const(s)
            .transform_error( //
                fn::overload([](std::in_place_type_t<int>, int &) -> int { throw 0; },
                             [](std::in_place_type_t<int>, int const &i) -> int { return i; },
                             [](std::in_place_type_t<int>, int &&) -> int { throw 0; },
                             [](std::in_place_type_t<int>, int const &&) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view &) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view const &) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view &&) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> int { throw 0; }))
            .error()
        == fn::sum{12});

    CHECK(fn::expected<void, fn::sum<int, std::string_view>>{std::unexpect, fn::sum{12}}
              .transform_error( //
                  fn::overload(
                      [](int &) -> int { throw 0; }, [](int const &) -> int { throw 0; },
                      [](int &&i) -> int { return i; }, [](int const &&) -> int { throw 0; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }))
              .error()
          == fn::sum{12});

    CHECK(
        fn::expected<void, fn::sum<int, std::string_view>>{std::unexpect, fn::sum{12}}
            .transform_error( //
                fn::overload([](std::in_place_type_t<int>, int &) -> int { throw 0; },
                             [](std::in_place_type_t<int>, int const &) -> int { throw 0; },
                             [](std::in_place_type_t<int>, int &&i) -> int { return i; },
                             [](std::in_place_type_t<int>, int const &&) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view &) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view const &) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view &&) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> int { throw 0; }))
            .error()
        == fn::sum{12});

    CHECK(std::move(std::as_const(s))
              .transform_error( //
                  fn::overload(
                      [](int &) -> int { throw 0; }, [](int const &) -> int { throw 0; },
                      [](int &&) -> int { throw 0; }, [](int const &&i) -> int { return i; },
                      [](std::string_view &) -> int { throw 0; }, [](std::string_view const &) -> int { throw 0; },
                      [](std::string_view &&) -> int { throw 0; }, [](std::string_view const &&) -> int { throw 0; }))
              .error()
          == fn::sum{12});

    CHECK(
        std::move(std::as_const(s))
            .transform_error( //
                fn::overload([](std::in_place_type_t<int>, int &) -> int { throw 0; },
                             [](std::in_place_type_t<int>, int const &) -> int { throw 0; },
                             [](std::in_place_type_t<int>, int &&) -> int { throw 0; },
                             [](std::in_place_type_t<int>, int const &&i) -> int { return i; },
                             [](std::in_place_type_t<std::string_view>, std::string_view &) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view const &) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view &&) -> int { throw 0; },
                             [](std::in_place_type_t<std::string_view>, std::string_view const &&) -> int { throw 0; }))
            .error()
        == fn::sum{12});

    WHEN("value")
    {
      fn::expected<void, fn::sum<int, std::string_view>> s{};
      CHECK(s.transform_error( //
                 [](auto...) -> int { throw 0; })
                .has_value());
      CHECK(std::as_const(s)
                .transform_error( //
                    [](auto...) -> int { throw 0; })
                .has_value());
      CHECK(fn::expected<void, fn::sum<int, std::string_view>>{}
                .transform_error( //
                    [](auto...) -> int { throw 0; })
                .has_value());
      CHECK(std::move(std::as_const(s))
                .transform_error( //
                    [](auto...) -> int { throw 0; })
                .has_value());
    }

    WHEN("constexpr")
    {
      constexpr auto fn = fn::overload(
          [](int &) -> int { throw 0; }, [](int const &i) -> int { return i; }, [](int &&) -> int { throw 0; },
          [](int const &&) -> int { throw 0; }, [](std::string_view &) -> int { throw 0; },
          [](std::string_view const &) -> int { throw 0; }, [](std::string_view &&) -> int { throw 0; },
          [](std::string_view const &&) -> int { throw 0; });
      constexpr fn::expected<void, fn::sum<int, std::string_view>> a{std::unexpect, fn::sum{42}};
      static_assert(std::is_same_v<decltype(a.transform_error(fn)), fn::expected<void, fn::sum<int>>>);
      static_assert(a.transform_error(fn).error() == fn::sum{42});
    }
  }
}

TEST_CASE("expected polyfills and_then", "[expected][polyfill][and_then]")
{
  WHEN("value")
  {
    fn::expected<int, Error> s{12};
    CHECK(s.and_then( //
               fn::overload([](int &i) -> fn::expected<bool, Error> { return i == 12; },
                            [](int const &) -> fn::expected<bool, Error> { throw 0; },
                            [](int &&) -> fn::expected<bool, Error> { throw 0; },
                            [](int const &&) -> fn::expected<bool, Error> { throw 0; })) //
              .value());
    CHECK(std::as_const(s)
              .and_then( //
                  fn::overload([](int &) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &i) -> fn::expected<bool, Error> { return i == 12; },
                               [](int &&) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &&) -> fn::expected<bool, Error> { throw 0; })) //
              .value());
    CHECK(fn::expected<int, Error>{12}
              .and_then( //
                  fn::overload([](int &) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &) -> fn::expected<bool, Error> { throw 0; },
                               [](int &&i) -> fn::expected<bool, Error> { return i == 12; },
                               [](int const &&) -> fn::expected<bool, Error> { throw 0; })) //
              .value());
    CHECK(std::move(std::as_const(s))
              .and_then( //
                  fn::overload([](int &) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &) -> fn::expected<bool, Error> { throw 0; },
                               [](int &&) -> fn::expected<bool, Error> { throw 0; },
                               [](int const &&i) -> fn::expected<bool, Error> { return i == 12; })) //
              .value());

    WHEN("error")
    {
      fn::expected<int, Error> s{std::unexpect, Unknown};
      CHECK(s.and_then( //
                 [](auto) -> fn::expected<bool, Error> { throw 0; })
                .error()
            == Unknown);
      CHECK(std::as_const(s)
                .and_then( //
                    [](auto) -> fn::expected<bool, Error> { throw 0; })
                .error()
            == Unknown);
      CHECK(fn::expected<int, Error>{std::unexpect, Unknown}
                .and_then( //
                    [](auto) -> fn::expected<bool, Error> { throw 0; })
                .error()
            == Unknown);
      CHECK(std::move(std::as_const(s))
                .and_then( //
                    [](auto) -> fn::expected<bool, Error> { throw 0; })
                .error()
            == Unknown);
    }
  }

  WHEN("void")
  {
    fn::expected<void, Error> s{};
    CHECK(s.and_then([]() -> fn::expected<bool, Error> { return true; }).value());
    CHECK(std::as_const(s).and_then([]() -> fn::expected<bool, Error> { return true; }).value());
    CHECK(fn::expected<void, Error>{}.and_then([]() -> fn::expected<bool, Error> { return true; }).value());
    CHECK(std::move(std::as_const(s)).and_then([]() -> fn::expected<bool, Error> { return true; }).value());

    WHEN("error")
    {
      fn::expected<void, Error> s{std::unexpect, Unknown};
      CHECK(s.and_then([]() -> fn::expected<bool, Error> { throw 0; }).error() == Unknown);
      CHECK(std::as_const(s).and_then([]() -> fn::expected<bool, Error> { throw 0; }).error() == Unknown);
      CHECK(fn::expected<void, Error>{std::unexpect, Unknown}
                .and_then([]() -> fn::expected<bool, Error> { throw 0; })
                .error()
            == Unknown);
      CHECK(std::move(std::as_const(s)).and_then([]() -> fn::expected<bool, Error> { throw 0; }).error() == Unknown);
    }
  }
}

TEST_CASE("expected polyfills or_else", "[expected][polyfill][or_else][pack]")
{
  WHEN("value")
  {
    fn::expected<int, Error> s{1};
    CHECK(s.or_else([](auto) -> fn::expected<int, Error> { throw 0; }).value());
    CHECK(std::as_const(s).or_else([](auto) -> fn::expected<int, Error> { throw 0; }).value());
    CHECK(fn::expected<int, Error>{1}.or_else([](auto) -> fn::expected<int, Error> { throw 0; }).value());
    CHECK(std::move(std::as_const(s)).or_else([](auto) -> fn::expected<int, Error> { throw 0; }).value());

    WHEN("error")
    {
      fn::expected<int, Error> s{std::unexpect, FileNotFound};
      CHECK(s.or_else( //
                 fn::overload([](Error &e) -> fn::expected<int, Error> { return e == FileNotFound; },
                              [](Error const &) -> fn::expected<int, Error> { throw 0; },
                              [](Error &&) -> fn::expected<int, Error> { throw 0; },
                              [](Error const &&) -> fn::expected<int, Error> { throw 0; })) //
                .value());
      CHECK(std::as_const(s)
                .or_else( //
                    fn::overload([](Error &) -> fn::expected<int, Error> { throw 0; },
                                 [](Error const &e) -> fn::expected<int, Error> { return e == FileNotFound; },
                                 [](Error &&) -> fn::expected<int, Error> { throw 0; },
                                 [](Error const &&) -> fn::expected<int, Error> { throw 0; })) //
                .value());
      CHECK(fn::expected<int, Error>{std::unexpect, FileNotFound}
                .or_else( //
                    fn::overload([](Error &) -> fn::expected<int, Error> { throw 0; },
                                 [](Error const &) -> fn::expected<int, Error> { throw 0; },
                                 [](Error &&e) -> fn::expected<int, Error> { return e == FileNotFound; },
                                 [](Error const &&) -> fn::expected<int, Error> { throw 0; })) //
                .value());
      CHECK(std::move(std::as_const(s))
                .or_else( //
                    fn::overload([](Error &) -> fn::expected<int, Error> { throw 0; },
                                 [](Error const &) -> fn::expected<int, Error> { throw 0; },
                                 [](Error &&) -> fn::expected<int, Error> { throw 0; },
                                 [](Error const &&e) -> fn::expected<int, Error> { return e == FileNotFound; })) //
                .value());
    }

    WHEN("pack error")
    {
      fn::expected<int, fn::pack<int, Error>> s{std::unexpect, fn::pack{12, FileNotFound}};
      CHECK(s.or_else( //
                 fn::overload([](int, Error &e) -> fn::expected<int, Error> { return e == FileNotFound; },
                              [](int, Error const &) -> fn::expected<int, Error> { throw 0; },
                              [](int, Error &&) -> fn::expected<int, Error> { throw 0; },
                              [](int, Error const &&) -> fn::expected<int, Error> { throw 0; })) //
                .value());
      CHECK(std::as_const(s)
                .or_else( //
                    fn::overload([](int, Error &) -> fn::expected<int, Error> { throw 0; },
                                 [](int, Error const &e) -> fn::expected<int, Error> { return e == FileNotFound; },
                                 [](int, Error &&) -> fn::expected<int, Error> { throw 0; },
                                 [](int, Error const &&) -> fn::expected<int, Error> { throw 0; })) //
                .value());
      CHECK(std::move(std::as_const(s))
                .or_else( //
                    fn::overload([](int, Error &) -> fn::expected<int, Error> { throw 0; },
                                 [](int, Error const &) -> fn::expected<int, Error> { throw 0; },
                                 [](int, Error &&) -> fn::expected<int, Error> { throw 0; },
                                 [](int, Error const &&e) -> fn::expected<int, Error> { return e == FileNotFound; })) //
                .value());
      CHECK(std::move(s)
                .or_else( //
                    fn::overload([](int, Error &) -> fn::expected<int, Error> { throw 0; },
                                 [](int, Error const &) -> fn::expected<int, Error> { throw 0; },
                                 [](int, Error &&e) -> fn::expected<int, Error> { return e == FileNotFound; },
                                 [](int, Error const &&) -> fn::expected<int, Error> { throw 0; })) //
                .value());
    }
  }

  WHEN("void")
  {
    fn::expected<void, Error> s{};
    CHECK(s.or_else([](auto) -> fn::expected<void, Error> { throw 0; }).has_value());
    CHECK(std::as_const(s).or_else([](auto) -> fn::expected<void, Error> { throw 0; }).has_value());
    CHECK(fn::expected<void, Error>{}.or_else([](auto) -> fn::expected<void, Error> { throw 0; }).has_value());
    CHECK(std::move(std::as_const(s)).or_else([](auto) -> fn::expected<void, Error> { throw 0; }).has_value());

    WHEN("error")
    {
      fn::expected<void, Error> s{std::unexpect, FileNotFound};
      CHECK(s.or_else( //
                 fn::overload([](Error &) -> fn::expected<void, Error> { return {}; },
                              [](Error const &) -> fn::expected<void, Error> { throw 0; },
                              [](Error &&) -> fn::expected<void, Error> { throw 0; },
                              [](Error const &&) -> fn::expected<void, Error> { throw 0; }))
                .has_value());
      CHECK(std::as_const(s)
                .or_else( //
                    fn::overload([](Error &) -> fn::expected<void, Error> { throw 0; },
                                 [](Error const &) -> fn::expected<void, Error> { return {}; },
                                 [](Error &&) -> fn::expected<void, Error> { throw 0; },
                                 [](Error const &&) -> fn::expected<void, Error> { throw 0; }))
                .has_value());
      CHECK(fn::expected<void, Error>{std::unexpect, FileNotFound}
                .or_else( //
                    fn::overload([](Error &) -> fn::expected<void, Error> { throw 0; },
                                 [](Error const &) -> fn::expected<void, Error> { throw 0; },
                                 [](Error &&) -> fn::expected<void, Error> { return {}; },
                                 [](Error const &&) -> fn::expected<void, Error> { throw 0; }))
                .has_value());
      CHECK(std::move(std::as_const(s))
                .or_else( //
                    fn::overload([](Error &) -> fn::expected<void, Error> { throw 0; },
                                 [](Error const &) -> fn::expected<void, Error> { throw 0; },
                                 [](Error &&) -> fn::expected<void, Error> { throw 0; },
                                 [](Error const &&) -> fn::expected<void, Error> { return {}; }))
                .has_value());
    }

    WHEN("pack error")
    {
      fn::expected<void, fn::pack<int, Error>> s{std::unexpect, fn::pack{12, FileNotFound}};
      CHECK(s.or_else( //
                 fn::overload([](int, Error &) -> fn::expected<void, Error> { return {}; },
                              [](int, Error const &) -> fn::expected<void, Error> { throw 0; },
                              [](int, Error &&) -> fn::expected<void, Error> { throw 0; },
                              [](int, Error const &&) -> fn::expected<void, Error> { throw 0; }))
                .has_value());
      CHECK(std::as_const(s)
                .or_else( //
                    fn::overload([](int, Error &) -> fn::expected<void, Error> { throw 0; },
                                 [](int, Error const &) -> fn::expected<void, Error> { return {}; },
                                 [](int, Error &&) -> fn::expected<void, Error> { throw 0; },
                                 [](int, Error const &&) -> fn::expected<void, Error> { throw 0; }))
                .has_value());
      CHECK(std::move(std::as_const(s))
                .or_else( //
                    fn::overload([](int, Error &) -> fn::expected<void, Error> { throw 0; },
                                 [](int, Error const &) -> fn::expected<void, Error> { throw 0; },
                                 [](int, Error &&) -> fn::expected<void, Error> { throw 0; },
                                 [](int, Error const &&) -> fn::expected<void, Error> { return {}; }))
                .has_value());
      CHECK(std::move(s)
                .or_else( //
                    fn::overload([](int, Error &) -> fn::expected<void, Error> { throw 0; },
                                 [](int, Error const &) -> fn::expected<void, Error> { throw 0; },
                                 [](int, Error &&) -> fn::expected<void, Error> { return {}; },
                                 [](int, Error const &&) -> fn::expected<void, Error> { throw 0; }))
                .has_value());
    }
  }
}

TEST_CASE("expected polyfills transform", "[expected][polyfill][transform]")
{
  WHEN("value")
  {
    fn::expected<int, Error> s{12};
    CHECK(s.transform( //
               fn::overload([](int &i) -> bool { return i == 12; }, [](int const &) -> bool { throw 0; },
                            [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; })) //
              .value());
    CHECK(std::as_const(s)
              .transform( //
                  fn::overload([](int &) -> bool { throw 0; }, [](int const &i) -> bool { return i == 12; },
                               [](int &&) -> bool { throw 0; }, [](int const &&) -> bool { throw 0; })) //
              .value());
    CHECK(fn::expected<int, Error>{12}
              .transform( //
                  fn::overload([](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                               [](int &&i) -> bool { return i == 12; }, [](int const &&) -> bool { throw 0; })) //
              .value());
    CHECK(std::move(std::as_const(s))
              .transform( //
                  fn::overload([](int &) -> bool { throw 0; }, [](int const &) -> bool { throw 0; },
                               [](int &&) -> bool { throw 0; }, [](int const &&i) -> bool { return i == 12; })) //
              .value());

    WHEN("void result")
    {
      fn::expected<int, Error> s{12};
      CHECK(s.transform( //
                 fn::overload([](int &) -> void {}, [](int const &) -> void { throw 0; },
                              [](int &&) -> void { throw 0; }, [](int const &&) -> void { throw 0; })) //
                .has_value());
      CHECK(std::as_const(s)
                .transform( //
                    fn::overload([](int &) -> void { throw 0; }, [](int const &) -> void {},
                                 [](int &&) -> void { throw 0; }, [](int const &&) -> void { throw 0; })) //
                .has_value());
      CHECK(fn::expected<int, Error>{12}
                .transform( //
                    fn::overload([](int &) -> void { throw 0; }, [](int const &) -> void { throw 0; },
                                 [](int &&) -> void {}, [](int const &&) -> void { throw 0; })) //
                .has_value());
      CHECK(std::move(std::as_const(s))
                .transform( //
                    fn::overload([](int &) -> void { throw 0; }, [](int const &) -> void { throw 0; },
                                 [](int &&) -> void { throw 0; }, [](int const &&) -> void {})) //
                .has_value());
    }

    WHEN("error")
    {
      fn::expected<int, Error> s{std::unexpect, Unknown};
      CHECK(s.transform([](auto) -> bool { throw 0; }).error() == Unknown);
      CHECK(std::as_const(s).transform([](auto) -> bool { throw 0; }).error() == Unknown);
      CHECK(fn::expected<int, Error>{std::unexpect, Unknown}.transform([](auto) -> bool { throw 0; }).error()
            == Unknown);
      CHECK(std::move(std::as_const(s)).transform([](auto) -> bool { throw 0; }).error() == Unknown);
    }
  }

  WHEN("void")
  {
    fn::expected<void, Error> s{};
    CHECK(s.transform([]() -> bool { return true; }).value());
    CHECK(std::as_const(s).transform([]() -> bool { return true; }).value());
    CHECK(fn::expected<void, Error>{}.transform([]() -> bool { return true; }).value());
    CHECK(std::move(std::as_const(s)).transform([]() -> bool { return true; }).value());

    WHEN("void result")
    {
      fn::expected<void, Error> s{};
      CHECK(s.transform([]() -> void {}).has_value());
      CHECK(std::as_const(s).transform([]() -> void {}).has_value());
      CHECK(fn::expected<void, Error>{}.transform([]() -> void {}).has_value());
      CHECK(std::move(std::as_const(s)).transform([]() -> void {}).has_value());
    }

    WHEN("error")
    {
      fn::expected<void, Error> s{std::unexpect, Unknown};
      CHECK(s.transform([]() -> bool { throw 0; }).error() == Unknown);
      CHECK(std::as_const(s).transform([]() -> bool { throw 0; }).error() == Unknown);
      CHECK(fn::expected<void, Error>{std::unexpect, Unknown}.transform([]() -> bool { throw 0; }).error() == Unknown);
      CHECK(std::move(std::as_const(s)).transform([]() -> bool { throw 0; }).error() == Unknown);
    }
  }
}

TEST_CASE("expected polyfills transform_error", "[expected][polyfill][transform_error][pack]")
{
  WHEN("value")
  {
    fn::expected<int, Error> s{12};
    CHECK(s.transform_error([](Error) -> bool { throw 0; }).value() == 12);
    CHECK(std::as_const(s).transform_error([](Error) -> bool { throw 0; }).value() == 12);
    CHECK(fn::expected<int, Error>{12}.transform_error([](Error) -> bool { throw 0; }).value() == 12);
    CHECK(std::move(std::as_const(s)).transform_error([](Error) -> bool { throw 0; }).value() == 12);

    WHEN("error")
    {
      fn::expected<int, Error> s{std::unexpect, FileNotFound};
      CHECK(
          s.transform_error( //
               fn::overload([](Error &e) -> bool { return e == FileNotFound; }, [](Error const &) -> bool { throw 0; },
                            [](Error &&) -> bool { throw 0; }, [](Error const &&) -> bool { throw 0; })) //
              .error());
      CHECK(std::as_const(s)
                .transform_error( //
                    fn::overload([](Error &) -> bool { throw 0; },
                                 [](Error const &e) -> bool { return e == FileNotFound; },
                                 [](Error &&) -> bool { throw 0; }, [](Error const &&) -> bool { throw 0; })) //
                .error());
      CHECK(fn::expected<int, Error>{std::unexpect, FileNotFound}
                .transform_error( //
                    fn::overload([](Error &) -> bool { throw 0; }, [](Error const &) -> bool { throw 0; },
                                 [](Error &&e) -> bool { return e == FileNotFound; },
                                 [](Error const &&) -> bool { throw 0; })) //
                .error());
      CHECK(std::move(std::as_const(s))
                .transform_error( //
                    fn::overload([](Error &) -> bool { throw 0; }, [](Error const &) -> bool { throw 0; },
                                 [](Error &&) -> bool { throw 0; },
                                 [](Error const &&e) -> bool { return e == FileNotFound; })) //
                .error());
    }

    WHEN("pack error")
    {
      fn::expected<int, fn::pack<int, Error>> s{std::unexpect, fn::pack{12, FileNotFound}};
      CHECK(s.transform_error( //
                 fn::overload([](int, Error &e) -> bool { return e == FileNotFound; },
                              [](int, Error const &) -> bool { throw 0; }, [](int, Error &&) -> bool { throw 0; },
                              [](int, Error const &&) -> bool { throw 0; })) //
                .error());
      CHECK(
          std::as_const(s)
              .transform_error( //
                  fn::overload([](int, Error &) -> bool { throw 0; },
                               [](int, Error const &e) -> bool { return e == FileNotFound; },
                               [](int, Error &&) -> bool { throw 0; }, [](int, Error const &&) -> bool { throw 0; })) //
              .error());
      CHECK(std::move(std::as_const(s))
                .transform_error( //
                    fn::overload([](int, Error &) -> bool { throw 0; }, [](int, Error const &) -> bool { throw 0; },
                                 [](int, Error &&) -> bool { throw 0; },
                                 [](int, Error const &&e) -> bool { return e == FileNotFound; })) //
                .error());
      CHECK(std::move(s)
                .transform_error( //
                    fn::overload([](int, Error &) -> bool { throw 0; }, [](int, Error const &) -> bool { throw 0; },
                                 [](int, Error &&e) -> bool { return e == FileNotFound; },
                                 [](int, Error const &&) -> bool { throw 0; })) //
                .error());
    }
  }

  WHEN("void")
  {
    fn::expected<void, Error> s{};
    CHECK(s.transform_error([](auto) -> bool { throw 0; }).has_value());
    CHECK(std::as_const(s).transform_error([](auto) -> bool { throw 0; }).has_value());
    CHECK(fn::expected<void, Error>{}.transform_error([](auto) -> bool { throw 0; }).has_value());
    CHECK(std::move(std::as_const(s)).transform_error([](auto) -> bool { throw 0; }).has_value());

    WHEN("error")
    {
      fn::expected<void, Error> s{std::unexpect, FileNotFound};
      CHECK(s.transform_error( //
                 fn::overload([](Error &) -> bool { return true; }, [](Error const &) -> bool { throw 0; },
                              [](Error &&) -> bool { throw 0; }, [](Error const &&) -> bool { throw 0; }))
                .error());
      CHECK(std::as_const(s)
                .transform_error( //
                    fn::overload([](Error &) -> bool { throw 0; }, [](Error const &) -> bool { return true; },
                                 [](Error &&) -> bool { throw 0; }, [](Error const &&) -> bool { throw 0; }))
                .error());
      CHECK(fn::expected<void, Error>{std::unexpect, FileNotFound}
                .transform_error( //
                    fn::overload([](Error &) -> bool { throw 0; }, [](Error const &) -> bool { throw 0; },
                                 [](Error &&) -> bool { return true; }, [](Error const &&) -> bool { throw 0; }))
                .error());
      CHECK(std::move(std::as_const(s))
                .transform_error( //
                    fn::overload([](Error &) -> bool { throw 0; }, [](Error const &) -> bool { throw 0; },
                                 [](Error &&) -> bool { throw 0; }, [](Error const &&) -> bool { return true; }))
                .error());
    }

    WHEN("pack error")
    {
      fn::expected<void, fn::pack<int, Error>> s{std::unexpect, fn::pack{12, FileNotFound}};
      CHECK(s.transform_error( //
                 fn::overload([](int, Error &) -> bool { return true; }, [](int, Error const &) -> bool { throw 0; },
                              [](int, Error &&) -> bool { throw 0; }, [](int, Error const &&) -> bool { throw 0; }))
                .error());
      CHECK(std::as_const(s)
                .transform_error( //
                    fn::overload([](int, Error &) -> bool { throw 0; }, [](int, Error const &) -> bool { return true; },
                                 [](int, Error &&) -> bool { throw 0; }, [](int, Error const &&) -> bool { throw 0; }))
                .error());
      CHECK(std::move(std::as_const(s))
                .transform_error( //
                    fn::overload([](int, Error &) -> bool { throw 0; }, [](int, Error const &) -> bool { throw 0; },
                                 [](int, Error &&) -> bool { throw 0; },
                                 [](int, Error const &&) -> bool { return true; }))
                .error());
      CHECK(std::move(s)
                .transform_error( //
                    fn::overload([](int, Error &) -> bool { throw 0; }, [](int, Error const &) -> bool { throw 0; },
                                 [](int, Error &&) -> bool { return true; },
                                 [](int, Error const &&) -> bool { throw 0; }))
                .error());
    }
  }
}
