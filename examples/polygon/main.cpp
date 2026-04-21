#include "fn/and_then.hpp"
#include "fn/expected.hpp"
#include "fn/or_else.hpp"
#include "fn/transform.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <istream>
#include <list>
#include <set>
#include <string>
#include <type_traits>
#include <utility>

struct parameters {
  std::string characters;
  std::list<std::string> files;

  struct too_few_parameters {
    std::string const message;
    static constexpr int code = 1;
  };

  struct too_few_characters {
    static constexpr char const message[] = "Error: at least 3 characters must be provided";
    static constexpr int code = 2;
  };

  using error = fn::sum<too_few_characters, too_few_parameters>;

  static auto make(int argc, char const *argv[]) -> fn::expected<parameters, error>
  {
    if (argc < 2) {
      return std::unexpected(
          too_few_parameters{.message = "Usage: " + std::string(argv[0]) + " <characters> [<file>...]"});
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
};

template <typename T>
concept parameters_error = parameters::error::has_type<std::remove_cvref_t<T>>;

void print_error(parameters_error auto &&err) { std::cerr << err.message << "\n"; }

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
    static constexpr char const message[] = "Error: file not found";
    static constexpr int code = 3;
  };
  struct permission_denied : file_error {
    static constexpr char const message[] = "Error: permission denied";
    static constexpr int code = 4;
  };

  using error = fn::sum<file_not_found, permission_denied>;

  counter characters;
  unsigned char required_character;
  std::list<std::unique_ptr<std::ifstream>> files;
  std::list<std::istream *> inputs; // Yes, this name is legal here and I will not change it

  static auto make(parameters &&p) -> fn::expected<struct inputs, error>
  {
    struct inputs result {
      .characters = count_characters(p.characters), .required_character = static_cast<unsigned char>(p.characters[0]),
      .files = {}, .inputs = {},
    };

    if (p.files.empty()) {
      result.inputs.push_back(&std::cin);
      return result;
    }

    static constexpr auto open_file
        = [](std::string const &filename) -> fn::expected<std::unique_ptr<std::ifstream>, error> {
      std::filesystem::path path(filename);
      if (!std::filesystem::exists(path)) {
        return std::unexpected(file_not_found{{.path = std::move(path)}});
      }

      auto file = std::make_unique<std::ifstream>(path);
      if (!file->is_open()) {
        return std::unexpected(permission_denied{{.path = std::move(path)}});
      }
      return std::move(file);
    };

    for (auto const &filename : p.files) {
      auto file = open_file(filename);

      // Early return on error
      if (!file.has_value())
        return std::unexpected(std::move(file).error());

      result.files.push_back(std::move(file).value());
      result.inputs.push_back(result.files.back().get());
    }

    return result;
  }
};

template <typename T>
concept inputs_error = inputs::error::has_type<std::remove_cvref_t<T>>;

void print_error(inputs_error auto &&err) { std::cerr << err.message << ": " << err.path << "\n"; }

constexpr inline struct algorithm_t {
  auto operator()(inputs const &inputs) const -> int
  {
    std::set<std::string> words;

    constexpr auto match = [](counter const &lh, counter const &rh) -> bool {
      for (std::size_t i = 0; i < rh.size(); ++i) {
        if (lh[i] < rh[i]) {
          return false;
        }
      }
      return true;
    };

    for (std::istream *input : inputs.inputs) {
      std::string line;
      while (std::getline(*input, line)) {
        auto const counts = count_characters(line);

        if (line.size() >= 3 && counts[inputs.required_character] > 0 && match(inputs.characters, counts)) {
          if (words.insert(line).second) {
            if (inputs.characters == counts)
              std::cout << "* ";
            std::cout << line << "\n";
          }
        }
      }
    }

    return 0;
  }
} algorithm;

auto main(int argc, char const **argv) -> int
try {
  static constexpr auto wrap = [](auto &&v) -> fn::expected<std::remove_cvref_t<decltype(v)>, fn::sum<>> { //
    return FWD(v);
  };

  return ((wrap(argc) & wrap(argv))        //
          | fn::and_then(parameters::make) //
          | fn::and_then(inputs::make)     //
          | fn::transform(algorithm)       //
          | fn::or_else(                   //
              [](auto &&p) -> fn::expected<int, fn::sum<>> {
                print_error(p);
                return p.code;
              }))
      .value();
} catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
  return 13;
}
