// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef EXAMPLES_POLYGON_POLYGON
#define EXAMPLES_POLYGON_POLYGON

#include "fn/and_then.hpp"
#include "fn/expected.hpp"

#include <cerrno>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <istream>
#include <list>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

struct parameters {
  std::string characters;
  std::list<std::string> files;

  struct too_few_parameters {
    too_few_parameters(too_few_parameters const &) = default;
    too_few_parameters(too_few_parameters &&) noexcept = default;
    explicit too_few_parameters(std::string_view program_name)
        : message("Usage: " + std::string(program_name) + " <characters> [<file>...]")
    {
    }

    constexpr bool operator==(too_few_parameters const &) const noexcept = default;

    std::string const message;
    static constexpr int code = 1;
  };

  struct too_few_characters {
    constexpr bool operator==(too_few_characters const &) const noexcept = default;
    static constexpr char const message[] = "Error: at least 3 characters must be provided";
    static constexpr int code = 2;
  };

  struct non_ascii_characters {
    constexpr bool operator==(non_ascii_characters const &) const noexcept = default;
    static constexpr char const message[] = "Error: only 7-bit ASCII characters are supported";
    static constexpr int code = 6;
  };

  using error = fn::sum<non_ascii_characters, too_few_characters, too_few_parameters>;

  static auto make(int argc, char const **argv) -> fn::expected<parameters, error>
  {
    if (argc < 2) {
      // Per the C standard argv[0] may be a null pointer (when argc == 0); on Linux the kernel
      // also synthesises an empty argv[0] when execve is called with an empty argv
      std::string_view const program_name = (argc >= 1 && argv[0] != nullptr && argv[0][0] != '\0') //
                                                ? std::string_view{argv[0]}
                                                : std::string_view{"<program>"};
      return std::unexpected(too_few_parameters{program_name});
    }

    parameters params;
    params.characters = argv[1];
    if (params.characters.size() < 3) {
      return std::unexpected(too_few_characters{});
    }

    for (char c : params.characters) {
      if (static_cast<unsigned char>(c) > 0x7F) {
        return std::unexpected(non_ascii_characters{});
      }
    }

    for (int i = 2; i < argc; ++i) {
      params.files.emplace_back(argv[i]);
    }
    return params;
  }
};

template <typename T>
concept parameters_error = parameters::error::has_type<std::remove_cvref_t<T>>;

inline void print_error(parameters_error auto &&err) { std::cerr << FWD(err).message << "\n"; }

using counter = std::array<std::size_t, 256>;

inline auto count_characters(std::string const &in) -> counter
{
  counter result = {};
  for (char c : in) {
    ++result[static_cast<unsigned char>(c)];
  }
  return result;
}

// File-scope hierarchy of errors that refer to a particular file path; produced by both the
// inputs stage (open failures) and the algorithm stage (read failures)
struct file_error {
  std::filesystem::path path;
  std::error_code ec;

  bool operator==(file_error const &) const noexcept = default;
};

struct file_not_found : file_error {
  static constexpr char const message[] = "Error: file not found";
  static constexpr int code = 3;
};
struct permission_denied : file_error {
  static constexpr char const message[] = "Error: permission denied";
  static constexpr int code = 4;
};
struct io_error : file_error {
  static constexpr char const message[] = "Error: I/O error";
  static constexpr int code = 5;
};
struct read_error : file_error {
  static constexpr char const message[] = "Error: read failed";
  static constexpr int code = 7;
};

template <typename T>
concept file_path_error = std::derived_from<std::remove_cvref_t<T>, file_error>;

inline void print_error(file_path_error auto &&err)
{
  std::cerr << FWD(err).message << ": " << FWD(err).path;
  if (FWD(err).ec) {
    std::cerr << " (" << FWD(err).ec.message() << ")";
  }
  std::cerr << "\n";
}

struct inputs {
  using error = fn::sum<file_not_found, io_error, permission_denied>;

  struct stream {
    std::filesystem::path path; // "<stdin>" when source is std::cin
    std::istream *in;
  };

  counter characters;
  unsigned char required_character;
  std::list<std::unique_ptr<std::ifstream>> files;
  std::list<stream> streams;

  static auto make(parameters const &p) -> fn::expected<struct inputs, error>
  {
    struct inputs init {
      .characters = count_characters(p.characters), .required_character = static_cast<unsigned char>(p.characters[0]),
      .files = {}, .streams = {},
    };

    if (p.files.empty()) {
      init.streams.push_back({.path = "<stdin>", .in = &std::cin});
      return init;
    }

    static constexpr auto open_file
        = [](std::string const &filename) -> fn::expected<std::unique_ptr<std::ifstream>, error> {
      std::filesystem::path path(filename);

      auto file = std::make_unique<std::ifstream>(path);
      if (file->is_open()) {
        return file;
      }

      // Open failed; classify the cause via std::filesystem::status as a best-effort diagnostic.
      std::error_code ec;
      auto const type = std::filesystem::status(path, ec).type();
      if (ec == std::errc::no_such_file_or_directory || type == std::filesystem::file_type::not_found) {
        return std::unexpected(file_not_found{{.path = std::move(path), .ec = ec}});
      }
      if (ec == std::errc::permission_denied) {
        return std::unexpected(permission_denied{{.path = std::move(path), .ec = ec}});
      }
      return std::unexpected(io_error{{.path = std::move(path), .ec = ec}});
    };

    static constexpr auto append =
        [](struct inputs r, std::filesystem::path path, std::unique_ptr<std::ifstream> f) -> struct inputs {
      r.files.push_back(std::move(f));
      r.streams.push_back({.path = std::move(path), .in = r.files.back().get()});
      return r;
    };

    // Recursive monadic fold over the file list (Y-combinator: self is passed explicitly)
    using iterator = std::list<std::string>::const_iterator;
    static constexpr auto fold =
        [](auto self, struct inputs r, iterator it, iterator end) -> fn::expected<struct inputs, error>
    {
      if (it == end) {
        return r;
      }

      return open_file(*it)                                      //
             | fn::and_then([ r = std::move(r), self, it, end ]( //
                                std::unique_ptr<std::ifstream> f) mutable -> fn::expected<struct inputs, error> {
                 return self(self, append(std::move(r), *it, std::move(f)), std::next(it), end);
               });
    };

    return fold(fold, std::move(init), p.files.begin(), p.files.end());
  }
};

constexpr inline struct algorithm_t {
  auto operator()(inputs const &inputs) const -> fn::expected<int, read_error>
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

    for (auto const &source : inputs.streams) {
      std::string line;
      while (std::getline(*source.in, line)) {
        auto const counts = count_characters(line);

        if (not(line.size() >= 3                         //
                && counts[inputs.required_character] > 0 //
                && match(inputs.characters, counts)      //
                && words.insert(line).second)) {
          continue;
        }

        if (inputs.characters == counts) {
          std::cout << "* ";
        }
        std::cout << line << "\n";
      }

      // getline stops on EOF (clean) or on an actual stream failure; only the former is acceptable
      if (source.in->eof() && !source.in->bad()) {
        continue;
      }
      return std::unexpected(read_error{{.path = source.path, .ec = std::error_code(errno, std::generic_category())}});
    }

    return 0;
  }
} algorithm;

#endif // EXAMPLES_POLYGON_POLYGON
