#include <fn/expected.hpp>
#include <fn/transform.hpp>

#include <cstdio>

int main()
{
  constexpr fn::expected<char const *, int> quine = {R"(#include <fn/expected.hpp>
#include <fn/transform.hpp>

#include <cstdio>

int main()
{
  constexpr fn::expected<char const *, int> quine = {R"(%s%c"};
  return (quine | fn::transform([](char const *s) {
            std::printf(s, s, ')');
            return 0;
          }))
      .value();
}
)"};
  return (quine | fn::transform([](char const *s) {
            std::printf(s, s, ')');
            return 0;
          }))
      .value();
}
