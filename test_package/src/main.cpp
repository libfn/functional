#include <fn/choice.hpp>
#include <fn/pack.hpp>
#include <fn/transform.hpp>

#include <iostream>
#include <numeric>
#include <string>

static constexpr char const *src[] = {
    R"(#include <fn/choice.hpp>
#include <fn/pack.hpp>
#include <fn/transform.hpp>

#include <iostream>
#include <numeric>
#include <string>

static constexpr char const *src[] = {
)",
    R"(
};

int main())",
    R"(
{
  static constexpr auto quote = [](char const *s) { return std::string("    R\"(") + s + ")\",\n"; };
  using quine_t
      = fn::choice_for<fn::pack<>, fn::pack<std::string, std::string>, fn::pack<std::string, std::string, std::string>>;)",
    R"(
  return std::accumulate(std::begin(src),        //
                         std::end(src),          //
                         quine_t(fn::as_pack()), //
                         [](quine_t acc, char const *s) {
                           return std::move(acc)
                                  | fn::transform(fn::overload{
                                      [s]() { return fn::as_pack<std::string, std::string>(s, quote(s)); },
                                      [s](std::string prefix, std::string quoted, auto &&...suffix) {
                                        return fn::as_pack<std::string, std::string, std::string>(
                                            prefix, quoted + quote(s), (suffix + ... + s));
                                      },
                                  });
                         }))",
    R"(
      .apply([](auto &&...strings) {
        ((std::cout << strings), ...);
        return 0;
      });
}
)",

};

int main()
{
  static constexpr auto quote = [](char const *s) { return std::string("    R\"(") + s + ")\",\n"; };
  using quine_t
      = fn::choice_for<fn::pack<>, fn::pack<std::string, std::string>, fn::pack<std::string, std::string, std::string>>;
  return std::accumulate(std::begin(src),        //
                         std::end(src),          //
                         quine_t(fn::as_pack()), //
                         [](quine_t acc, char const *s) {
                           return std::move(acc)
                                  | fn::transform(fn::overload{
                                      [s]() { return fn::as_pack<std::string, std::string>(s, quote(s)); },
                                      [s](std::string prefix, std::string quoted, auto &&...suffix) {
                                        return fn::as_pack<std::string, std::string, std::string>(
                                            prefix, quoted + quote(s), (suffix + ... + s));
                                      },
                                  });
                         })
      .apply([](auto &&...strings) {
        ((std::cout << strings), ...);
        return 0;
      });
}
