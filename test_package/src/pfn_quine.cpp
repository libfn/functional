#include <pfn/expected.hpp>

#include <cstdio>
#include <string_view>

int main()
{
  std::string_view q = R"(#include <pfn/expected.hpp>

#include <cstdio>
#include <string_view>

int main()
{
  std::string_view q = R"(%s%c";
  return pfn::expected<std::string_view, int>{q}
      .transform([](std::string_view s) {
        std::printf(s.data(), s.data(), ')');
        return 0;
      })
      .value();
}
)";
  return pfn::expected<std::string_view, int>{q}
      .transform([](std::string_view s) {
        std::printf(s.data(), s.data(), ')');
        return 0;
      })
      .value();
}
