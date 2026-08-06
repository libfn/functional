#include <fn/pack.hpp>
#include <iostream>

static constexpr auto src = fn::pack{
    R"(#include <fn/pack.hpp>
#include <iostream>

static constexpr auto src = fn::pack{
)",
    R"(};

)",
    R"(int main()
{
)",
    R"(  auto const quote = [](char const *s) { return std::string("    R\"(") + s + ")\",\n"; };
)",
    R"(  return src.apply([&](char const *head, auto const *...tail) -> int {
)",
    R"(    std::cout << head << (quote(head) + ... + quote(tail)) << (std::string() + ... + tail);
)",
    R"(    return sizeof...(tail) == 0;
)",
    R"(  });
)",
    R"(}
)",
};

int main()
{
  auto const quote = [](char const *s) { return std::string("    R\"(") + s + ")\",\n"; };
  return src.apply([&](char const *head, auto const *...tail) -> int {
    std::cout << head << (quote(head) + ... + quote(tail)) << (std::string() + ... + tail);
    return sizeof...(tail) == 0;
  });
}
