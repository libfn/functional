#include <fn/and_then.hpp>
#include <fn/choice.hpp>
#include <fn/pack.hpp>
#include <fn/utility.hpp>

#include <cstdio>

static constexpr char const *src[] = {")", R"(#include <fn/and_then.hpp>
#include <fn/choice.hpp>
#include <fn/pack.hpp>
#include <fn/utility.hpp>

#include <cstdio>

static constexpr char const *src[] = {"%c", R"(%s%c",
                                      ""};

int main()
{
  fn::choice_for<fn::pack<>, char, fn::pack<char, char const *>> quine = fn::pack<>{};
  for (char const *s : src) {
    quine = quine
            | fn::and_then(fn::overload{//
                                        [s]() -> decltype(quine) { return *s; },
                                        [s](char c) -> decltype(quine) { return fn::pack<char, char const *>{c, s}; },
                                        [](char c, char const *s) -> decltype(quine) {
                                          std::printf(s, c, s, c);
                                          return fn::pack<>{};
                                        }});
  }
}
)",
                                      ""};

int main()
{
  fn::choice_for<fn::pack<>, char, fn::pack<char, char const *>> quine = fn::pack<>{};
  for (char const *s : src) {
    quine = quine
            | fn::and_then(fn::overload{//
                                        [s]() -> decltype(quine) { return *s; },
                                        [s](char c) -> decltype(quine) { return fn::pack<char, char const *>{c, s}; },
                                        [](char c, char const *s) -> decltype(quine) {
                                          std::printf(s, c, s, c);
                                          return fn::pack<>{};
                                        }});
  }
}
