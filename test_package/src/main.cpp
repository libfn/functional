#include <fn/and_then.hpp>
#include <fn/choice.hpp>
#include <fn/pack.hpp>
#include <fn/utility.hpp>

#include <cstdio>
#include <numeric>

static constexpr char const *src[] = {")", R"(#include <fn/and_then.hpp>
#include <fn/choice.hpp>
#include <fn/pack.hpp>
#include <fn/utility.hpp>

#include <cstdio>
#include <numeric>

static constexpr char const *src[] = {"%c", R"(%s%c",
                                      ""};

int main()
{
  using quine_t = fn::choice_for<fn::pack<>, fn::pack<char>, fn::pack<char, char const *>>;
  return std::accumulate(
             std::begin(src), std::end(src), quine_t{fn::as_pack()}, //
             [](quine_t &&acc, char const *s) -> quine_t {
               return acc
                      | fn::and_then(fn::overload{[s]() -> quine_t { return fn::pack<char>{*s}; },
                                                  [s](char c) -> quine_t { return fn::pack<char, char const *>{c, s}; },
                                                  [](char c, char const *fmt) -> quine_t {
                                                    std::printf(fmt, c, fmt, c);
                                                    return fn::as_pack();
                                                  }});
             })
      .apply([](auto &&...args) -> int { return sizeof...(args); });
}
)",
                                      ""};

int main()
{
  using quine_t = fn::choice_for<fn::pack<>, fn::pack<char>, fn::pack<char, char const *>>;
  return std::accumulate(
             std::begin(src), std::end(src), quine_t{fn::as_pack()}, //
             [](quine_t &&acc, char const *s) -> quine_t {
               return acc
                      | fn::and_then(fn::overload{[s]() -> quine_t { return fn::pack<char>{*s}; },
                                                  [s](char c) -> quine_t { return fn::pack<char, char const *>{c, s}; },
                                                  [](char c, char const *fmt) -> quine_t {
                                                    std::printf(fmt, c, fmt, c);
                                                    return fn::as_pack();
                                                  }});
             })
      .apply([](auto &&...args) -> int { return sizeof...(args); });
}
