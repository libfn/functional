// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

// readme-example
#include <fn/and_then.hpp>
#include <fn/or_else.hpp>
#include <fn/utility.hpp>

#include <charconv>
#include <string_view>
#include <type_traits>

enum class HostError { Empty };
enum class PortError { NotANumber };
enum class NetError { PortOutOfRange };

struct Endpoint {
  std::string_view host;
  int port;
};

auto parse_host(std::string_view s) -> fn::expected<std::string_view, HostError>
{
  if (s.empty())
    return pfn::unexpected<HostError>{HostError::Empty};
  return {s};
}

auto parse_port(std::string_view s) -> fn::expected<int, PortError>
{
  int p = {};
  char const *end = s.data() + s.size();
  if (std::from_chars(s.data(), end, p).ptr != end)
    return pfn::unexpected<PortError>{PortError::NotANumber};
  return {p};
}

auto endpoint(std::string_view host, std::string_view port)
{
  return (fn::expected<void, fn::sum<>>{} & parse_host(host) & parse_port(port))
         | fn::and_then([](std::string_view h, int p) -> fn::expected<Endpoint, NetError> {
             if (p < 1 || p > 0xffff)
               return pfn::unexpected<NetError>{NetError::PortOutOfRange};
             return Endpoint{h, p};
           });
}

// The pipeline type is composed by the library, including the sum of everything that can go wrong:
static_assert(
    std::is_same_v<decltype(endpoint("", "")), fn::expected<Endpoint, fn::sum<HostError, NetError, PortError>>>);

struct BadConfiguration {};
using configured = fn::expected<Endpoint, BadConfiguration>;

// Recover from some errors, translate others; compilation error if `fn::overload` cannot match *all* errors
auto with_default(std::string_view host, std::string_view port)
{
  return endpoint(host, port)
         | fn::or_else(fn::overload{[](HostError) -> configured { return pfn::unexpected{BadConfiguration{}}; },
                                    [](PortError) -> configured { return pfn::unexpected{BadConfiguration{}}; },
                                    [](NetError) -> configured { return Endpoint{"localhost", 8080}; }});
}

// ... and the sum of errors collapses at the API boundary:
static_assert(std::is_same_v<decltype(with_default("", "")), configured>);
// readme-example

int main()
{
  return (endpoint("test.example.com", "8080").value().port == 8080    //
          && endpoint("", "8080").error() == fn::sum{HostError::Empty} //
          && endpoint("test.example.com", "0").error() == fn::sum{NetError::PortOutOfRange}
          && with_default("test.example.com", "0").value().port == 8080 //
          && not with_default("", "8080").has_value())
             ? 0
             : 1;
}
