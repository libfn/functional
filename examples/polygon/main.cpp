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
#include <type_traits>
#include <utility>
#include <vector>

struct parameters {
  std::string characters;
  std::vector<std::string> files;

  struct too_few_parameters {};
  struct too_few_characters {
    static constexpr char const *message = "Error: at least 3 characters must be provided";
  };

  using error = fn::sum<too_few_characters, too_few_parameters>;

  static auto make(int argc, char *argv[]) -> fn::expected<parameters, error>
  {
    if (argc < 3) {
      return std::unexpected(too_few_parameters{});
    }

    parameters params;
    params.characters = argv[1];
    if (params.characters.size() < 3) {
      return std::unexpected(too_few_characters{});
    }

    for (int i = 2; i < argc; ++i) {
      params.files.emplace_back(argv[i]);
    }
    return params;
  }

  static void print_error(auto &&error, char const *const program_name)
  {
    if constexpr (std::same_as<std::remove_cvref_t<decltype(error)>, too_few_parameters>) {
      std::cerr << "Usage: " << program_name << " <characters> <file>...\n";
    } else
      std::cerr << error.message << "\n";
  }
};

template <typename T>
concept parameters_error = parameters::error::has_type<std::remove_cvref_t<T>>;

using counter = std::array<std::size_t, 256>;

auto count_characters(std::string const &in) -> counter
{
  counter result = {};
  for (char c : in) {
    ++result[static_cast<unsigned char>(c)];
  }
  return result;
}

struct inputs {
  struct file_error {
    std::filesystem::path path;
  };

  struct file_not_found : file_error {
    static constexpr char const *message = "Error: file not found";
  };
  struct permission_denied : file_error {
    static constexpr char const *message = "Error: permission denied";
  };

  using error = fn::sum<file_not_found, permission_denied>;

  counter characters;
  unsigned char required_character;
  std::vector<std::ifstream> files;

  static auto make(parameters &&p) -> fn::expected<inputs, error>
  {
    static constexpr auto open_file = [](std::string const &filename) -> fn::expected<std::ifstream, error> {
      std::filesystem::path path(filename);
      if (!std::filesystem::exists(path)) {
        return std::unexpected(file_not_found{{.path = std::move(path)}});
      }

      std::ifstream file(path);
      if (!file.is_open()) {
        return std::unexpected(permission_denied{{.path = std::move(path)}});
      }
      return std::move(file);
    };

    inputs result{.characters = count_characters(p.characters),
                  .required_character = static_cast<unsigned char>(p.characters[0]),
                  .files = {}};

    // Cannot use accumulate here because ifstream disables copy assignment
    for (auto const &filename : p.files) {
      auto file = open_file(filename);

      // Early return on error
      if (!file.has_value())
        return std::unexpected(std::move(file).error());

      result.files.push_back(std::move(file).value());
    }

    return result;
  }

  static void print_error(auto &&error) { std::cerr << error.message << ": " << error.path << "\n"; }
};

template <typename T>
concept inputs_error = inputs::error::has_type<std::remove_cvref_t<T>>;

int main(int argc, char *argv[])
{
  static constexpr auto wrap = [](auto &&v) -> fn::expected<std::remove_cvref_t<decltype(v)>, fn::sum<>> { //
    return FWD(v);
  };

  static constexpr auto match = [](counter const &lh, counter const &rh) -> bool {
    for (std::size_t i = 0; i < rh.size(); ++i) {
      if (lh[i] < rh[i]) {
        return false;
      }
    }
    return true;
  };

  return ((wrap(argc) & wrap(argv))        //
          | fn::and_then(parameters::make) //
          | fn::and_then(inputs::make)     //
          | fn::transform([](inputs &&i) -> int {
              std::set<std::string> words;

              return std::accumulate( //
                  i.files.begin(), i.files.end(),
                  0, //
                  [&i, &words](int, std::ifstream &file) -> int {
                    std::string line;
                    while (std::getline(file, line)) {
                      auto counts = count_characters(line);
                      if (line.size() >= 3 && counts[i.required_character] > 0 && match(i.characters, counts)) {
                        if (words.insert(line).second) {
                          if (i.characters == counts)
                            std::cout << "* ";
                          std::cout << line << "\n";
                        }
                      }
                    }
                    return 0;
                  });
            })
          | fn::or_else( //
              fn::overload{[argv](parameters_error auto &&p) -> fn::expected<int, fn::sum<>> {
                             parameters::print_error(p, argv[0]);
                             return 1;
                           },
                           [](inputs_error auto &&e) -> fn::expected<int, fn::sum<>> {
                             inputs::print_error(e);
                             return 2;
                           }}))
      .value();
}
