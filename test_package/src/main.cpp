#include <fn/expected.hpp>
#include <fn/pack.hpp>
#include <fn/sum.hpp>
#include <fn/transform.hpp>

#include <cstdio>

int main()
{
  constexpr fn::expected<fn::pack<char, char const *>, fn::sum<>> quine
      = fn::pack<char, char const *>{')', R"(#include <fn/expected.hpp>
#include <fn/pack.hpp>
#include <fn/sum.hpp>
#include <fn/transform.hpp>

#include <cstdio>

int main()
{
  constexpr fn::expected<fn::pack<char, char const *>, fn::sum<>> quine
      = fn::pack<char, char const *>{'%c', R"(%s%c"};
  return (quine //
          | fn::transform([](char c, char const *s) {
              std::printf(s, c, s, c);
              return 0;
            }))
      .value();
}
)"};
  return (quine //
          | fn::transform([](char c, char const *s) {
              std::printf(s, c, s, c);
              return 0;
            }))
      .value();
}
