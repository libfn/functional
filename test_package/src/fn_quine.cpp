#include <fn/expected.hpp>
#include <fn/transform.hpp>

#include <cstdio>
#include <string_view>

int main()
{
  std::string_view q = R"(#include <fn/expected.hpp>
#include <fn/transform.hpp>

#include <cstdio>
#include <string_view>

int main()
{
  std::string_view q = R"(%s%c";
  return (fn::expected<std::string_view, int>{q} //
          | fn::transform([](std::string_view s) {
              std::printf(s.data(), s.data(), ')');
              return 0;
            }))
      .value();
}
)";
  return (fn::expected<std::string_view, int>{q} //
          | fn::transform([](std::string_view s) {
              std::printf(s.data(), s.data(), ')');
              return 0;
            }))
      .value();
}
