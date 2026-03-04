#include "fn/and_then.hpp"
#include "fn/expected.hpp"
#include "fn/or_else.hpp"
#include "fn/transform.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <set>
#include <string>
#include <vector>

enum parameter_error {
  too_few_parameters,
  too_few_characters,
};

enum file_error {
  file_not_found,
  permission_denied,
};

auto open_file(std::string const &filename) -> fn::expected<std::ifstream, file_error>
{
  std::filesystem::path p(filename);
  if (!std::filesystem::exists(p)) {
    return std::unexpected(file_not_found);
  }

  std::ifstream file(p);
  if (!file.is_open()) {
    return std::unexpected(permission_denied);
  }
  return file;
}

auto count_characters(std::string const &in) -> std::array<std::size_t, 256>
{
  std::array<std::size_t, 256> result = {};
  for (char c : in) {
    ++result[static_cast<unsigned char>(c)];
  }
  return result;
}

auto match(std::array<std::size_t, 256> const &lh, std::array<std::size_t, 256> const &rh) -> bool
{
  for (std::size_t i = 0; i < 256; ++i) {
    if (lh[i] < rh[i]) {
      return false;
    }
  }
  return true;
}

int main(int argc, char *argv[])
{
  struct parameters {
    std::string characters;
    std::vector<std::string> files;
  };

  struct inputs {
    std::array<std::size_t, 256> characters;
    unsigned char required_character;
    std::vector<std::ifstream> files;
  };

  using expected_inputs = fn::expected<inputs, file_error>;

  return (fn::expected<void, fn::sum<>>{} //
          | fn::and_then([argc, argv]() -> fn::expected<parameters, parameter_error> {
              if (argc < 3) {
                return std::unexpected(too_few_parameters);
              }

              parameters params;
              params.characters = argv[1];
              if (params.characters.size() < 3) {
                return std::unexpected(too_few_characters);
              }

              for (int i = 2; i < argc; ++i) {
                params.files.emplace_back(argv[i]);
              }
              return params;
            })
          | fn::and_then([](parameters &&p) -> expected_inputs {
              return std::accumulate( //
                  p.files.begin(), p.files.end(),
                  expected_inputs(inputs{.characters = count_characters(p.characters),
                                         .required_character = static_cast<unsigned char>(p.characters[0]),
                                         .files = {}}),
                  [](expected_inputs acc, std::string const &filename) -> expected_inputs {
                    return std::move(acc) //
                           | fn::and_then([&filename](inputs &&in) -> expected_inputs {
                               return open_file(filename) //
                                      | fn::transform([&in](std::ifstream &&file) -> inputs {
                                          in.files.push_back(std::move(file));
                                          return std::move(in);
                                        });
                             });
                  });
            })
          | fn::transform([](inputs &&i) -> int {
              std::set<std::string> words;

              return std::accumulate(i.files.begin(), i.files.end(), 0, [&i, &words](int, std::ifstream &file) -> int {
                std::string line;
                while (std::getline(file, line)) {
                  auto counts = count_characters(line);
                  if (line.size() >= 3 && counts[i.required_character] > 0 && match(i.characters, counts)) {
                    if (words.insert(line).second) {
                      std::cout << line << "\n";
                    }
                  }
                }
                return 0;
              });
            })
          | fn::or_else(fn::overload{//
                                     [argv](parameter_error p) -> fn::expected<int, fn::sum<>> {
                                       switch (p) {
                                       case too_few_parameters:
                                         std::cerr << "Usage: " << argv[0] << " <characters> <file>...\n";
                                         break;
                                       case too_few_characters:
                                         std::cerr << "Error: at least 3 characters must be provided\n";
                                         break;
                                       }
                                       return 1;
                                     },
                                     [](file_error e) -> fn::expected<int, fn::sum<>> {
                                       switch (e) {
                                       case file_not_found:
                                         std::cerr << "Error: file not found\n";
                                         break;
                                       case permission_denied:
                                         std::cerr << "Error: permission denied\n";
                                         break;
                                       }
                                       return 2;
                                     }}))
      .value();
}
