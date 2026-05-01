// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "polygon.hpp"

#include <catch2/catch_all.hpp>

#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <istream>
#include <list>
#include <sstream>
#include <streambuf>
#include <string>
#include <string_view>
#include <vector>

#ifdef _WIN32
#include <random>
#include <stdexcept>
#else
#include <stdlib.h>
#include <system_error>
#include <unistd.h>
#endif

namespace {

// RAII helper that redirects std::cout to a captured stringstream
struct cout_capture {
  std::ostringstream out;
  std::streambuf *prev;

  cout_capture() : prev(std::cout.rdbuf(out.rdbuf())) {}
  cout_capture(cout_capture const &) = delete;
  cout_capture &operator=(cout_capture const &) = delete;
  ~cout_capture() { std::cout.rdbuf(prev); }
};

// RAII helper for a temporary directory. The unique path is composed from
// std::filesystem::temp_directory_path() and a per-process random salt plus an
// atomic per-call counter — fully portable (POSIX & Windows), no deprecated facilities.
struct temp_dir {
  std::filesystem::path path;

  static auto make_temp_dir(std::string const &prefix) -> std::filesystem::path
  {
    // Note: temp_directory_path() is only used to discover the platform's temp root;
    // the actual directory is created atomically via mkdtemp (POSIX) or O_EXCL-equivalent
    // create_directory in a strong-PRNG retry loop (Windows %TEMP% is already per-user).
    auto const tmp_dir = std::filesystem::temp_directory_path(); // NOSONAR - see above

#ifdef _WIN32
    // Windows: %TEMP% is inherently per-user (e.g., AppData\Local\Temp).
    // Because it is not a shared global directory, the cross-user race condition risk is mitigated.
    // We use a strong PRNG to generate a unique name and create_directory, which acts like O_EXCL.
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<std::uint64_t> dis;

    for (int i = 0; i < 1000; ++i) { // Retry limit to prevent infinite loops
      std::filesystem::path candidate = tmp_dir / (prefix + std::to_string(dis(gen)));
      std::error_code ec;

      // Fails safely if the directory already exists
      if (std::filesystem::create_directory(candidate, ec)) {
        return candidate;
      }
    }
    throw std::runtime_error("Failed to create secure temporary directory after 1000 attempts");

#else
    // POSIX (Linux/macOS): /tmp is shared globally. We MUST use mkdtemp.
    // mkdtemp requires a template string ending in exactly 6 'X's.
    std::string template_name = (tmp_dir / (prefix + "XXXXXX")).string();

    // mkdtemp modifies the template string in-place and creates the dir with strict 0700 permissions atomically.
    // C++17's .data() returns a non-const char* which is safe for mkdtemp to modify.
    if (::mkdtemp(template_name.data()) == nullptr) {
      throw std::system_error(errno, std::generic_category(), "Failed to create secure temp directory via mkdtemp");
    }

    return std::filesystem::path(template_name);
#endif
  }

  temp_dir() : path(make_temp_dir("polygon_test_")) {}
  temp_dir(temp_dir const &) = delete;
  temp_dir &operator=(temp_dir const &) = delete;
  ~temp_dir()
  {
    std::error_code ec;
    std::filesystem::remove(path, ec);
  }
};

// RAII helper for a temporary file populated with content. The file lives inside a
// per-instance temp_dir; the file is removed first, then the now-empty directory is
// removed by the base destructor. Both steps use std::filesystem::remove (non-recursive).
struct temp_file : temp_dir {
  std::filesystem::path file;

  explicit temp_file(std::string_view content) : file(path / "content.txt") { std::ofstream(file) << content; }
  ~temp_file()
  {
    std::error_code ec;
    std::filesystem::remove(file, ec);
  }
};

// Helper producing a std::istream whose first read sets errno and marks the stream as
// bad() without throwing. This allows testing of the algorithm's read_error in a portable way.
// Setting badbit directly via a back-pointer to the bound istream keeps errno intact.
struct failing_stream {
  struct buf_t : std::streambuf {
    int err;
    std::istream *bound = nullptr;
    explicit buf_t(int e) : err(e) {}

  protected:
    int_type underflow() override
    {
      errno = err;
      if (bound) {
        bound->setstate(std::ios_base::badbit);
      }
      return traits_type::eof();
    }
  } buf;
  std::istream stream;

  explicit failing_stream(int e) : buf(e), stream(&buf) { buf.bound = &stream; }
  failing_stream(failing_stream const &) = delete;
  failing_stream &operator=(failing_stream const &) = delete;
};

} // namespace

TEST_CASE("count_characters", "[polygon][count_characters]")
{
  auto const c = count_characters("aabbc");
  CHECK(c[static_cast<unsigned char>('a')] == 2);
  CHECK(c[static_cast<unsigned char>('b')] == 2);
  CHECK(c[static_cast<unsigned char>('c')] == 1);
  CHECK(c[static_cast<unsigned char>('z')] == 0);
}

TEST_CASE("parameters::make happy path", "[polygon][parameters]")
{
  std::vector<char const *> argv{"prog", "abcde", "f1.txt", "f2.txt"};
  auto const result = parameters::make(static_cast<int>(argv.size()), argv.data());
  REQUIRE(result.has_value());
  CHECK(result->characters == "abcde");
  REQUIRE(result->files.size() == 2);
  auto it = result->files.begin();
  CHECK(*it == "f1.txt");
  CHECK(*++it == "f2.txt");
}

TEST_CASE("parameters::make too few parameters", "[polygon][parameters]")
{
  SECTION("argc == 0 with null argv")
  {
    std::vector<char const *> argv{nullptr};
    auto const result = parameters::make(0, argv.data());
    REQUIRE(not result.has_value());
    CHECK(result.error() == parameters::error{parameters::too_few_parameters{"<program>"}});
  }

  SECTION("argc == 1 with program name")
  {
    std::vector<char const *> argv{"polygon"};
    auto const result = parameters::make(static_cast<int>(argv.size()), argv.data());
    REQUIRE(not result.has_value());
    CHECK(result.error() == parameters::error{parameters::too_few_parameters{"polygon"}});
  }
}

TEST_CASE("parameters::make rejects an invalid characters argument", "[polygon][parameters]")
{
  SECTION("too few characters")
  {
    std::vector<char const *> argv{"prog", "ab"};
    auto const result = parameters::make(static_cast<int>(argv.size()), argv.data());
    REQUIRE(not result.has_value());
    CHECK(result.error() == parameters::error{parameters::too_few_characters{}});
  }

  SECTION("non-ascii character")
  {
    std::vector<char const *> argv{"prog", "łąki"};
    auto const result = parameters::make(static_cast<int>(argv.size()), argv.data());
    REQUIRE(not result.has_value());
    CHECK(result.error() == parameters::error{parameters::non_ascii_characters{}});
  }
}

TEST_CASE("inputs::make", "[polygon][inputs]")
{
  SECTION("empty file list falls back to stdin")
  {
    parameters const p{.characters = "abc", .files = {}};
    auto const result = inputs::make(p);
    REQUIRE(result.has_value());
    REQUIRE(result->streams.size() == 1);
    CHECK(result->streams.front().path == "<stdin>");
    CHECK(result->streams.front().in == &std::cin);
    CHECK(result->required_character == static_cast<unsigned char>('a'));
  }

  SECTION("nonexistent path yields file_not_found")
  {
    std::filesystem::path const missing = "/nonexistent/path/that/does/not/exist";
    parameters const p{.characters = "abc", .files = {missing.string()}};
    auto const result = inputs::make(p);
    REQUIRE(not result.has_value());
    CHECK(result.error()
          == inputs::error{
              file_not_found{{.path = missing, .ec = std::make_error_code(std::errc::no_such_file_or_directory)}}});
  }

  SECTION("nonexistent path yields io_error")
  {
    temp_file const tf("");
    auto const subpath = tf.file / "nonexistent_subdirectory";
    parameters const p{.characters = "abc", .files = {subpath}};
    auto const result = inputs::make(p);
    REQUIRE(not result.has_value());
    CHECK(result.error()
          == inputs::error{io_error{{.path = subpath, .ec = std::make_error_code(std::errc::not_a_directory)}}});
  }

  SECTION("opens a real file")
  {
    temp_file const tf("hello\nworld\n");
    parameters const p{.characters = "abc", .files = {tf.file.string()}};
    auto const result = inputs::make(p);
    REQUIRE(result.has_value());
    REQUIRE(result->streams.size() == 1);
    CHECK(result->streams.front().path == tf.file);
    CHECK(result->files.size() == 1);
  }
}

TEST_CASE("algorithm filters and prints anagrams", "[polygon][algorithm]")
{
  // "abcd" has every required character but also a 'd' not in the requirements -> filtered
  // "ab"   is shorter than 3                                                   -> filtered
  // "xyz"  lacks 'a' (the required character)                                  -> filtered
  // "abc", "bca", "bac" are anagrams of "abc"                                  -> printed with leading "* "
  // "bca" appears twice but the deduplication set drops the second occurrence
  std::istringstream src("abc\nbca\nxyz\nbac\nabcd\nab\nbca\n");
  inputs in{
      .characters = count_characters("abc"),
      .required_character = static_cast<unsigned char>('a'),
      .files = {},
      .streams = {{.path = "<test>", .in = &src}},
  };

  cout_capture capture;
  auto const result = algorithm(in);
  REQUIRE(result.has_value());
  CHECK(*result == 0);
  CHECK(capture.out.str() == "* abc\n* bca\n* bac\n");
}

TEST_CASE("algorithm returns read_error on bad stream", "[polygon][algorithm]")
{
  // Force the algorithm down its read_error path with a stream that fails on first read
  failing_stream bad(EISDIR);

  inputs in{
      .characters = count_characters("abc"),
      .required_character = static_cast<unsigned char>('a'),
      .files = {},
      .streams = {{.path = "<bad>", .in = &bad.stream}},
  };

  cout_capture capture; // suppress incidental output
  auto const result = algorithm(in);
  REQUIRE(not result.has_value());
  CHECK(result.error() == read_error{{.path = "<bad>", .ec = std::make_error_code(std::errc::is_a_directory)}});
}

TEST_CASE("algorithm processes multiple streams with cross-stream dedup", "[polygon][algorithm]")
{
  // First stream prints "* abc" (exact match) and "* bca"; "abcd" is filtered (extra 'd').
  // Second stream's leading "abc" is dropped by the cross-stream dedup set; "bac" is new and
  // printed; "xyz" lacks 'a' and is filtered. Both streams must be drained in order.
  std::istringstream src1("abc\nbca\nabcd\n");
  std::istringstream src2("abc\nbac\nxyz\n");
  inputs in{
      .characters = count_characters("abc"),
      .required_character = static_cast<unsigned char>('a'),
      .files = {},
      .streams = {{.path = "<s1>", .in = &src1}, {.path = "<s2>", .in = &src2}},
  };

  cout_capture capture;
  auto const result = algorithm(in);
  REQUIRE(result.has_value());
  CHECK(*result == 0);
  CHECK(capture.out.str() == "* abc\n* bca\n* bac\n");
}

TEST_CASE("algorithm aborts on read_error within a multi-stream input", "[polygon][algorithm]")
{
  // The "bad" stream's first read sets badbit and errno=EISDIR (see failing_stream).
  // The "good" stream contains an exact match that would print "* abc\n" when drained.
  failing_stream bad(EISDIR);

  std::istringstream good("abc\n");
  cout_capture capture;

  SECTION("error in the first stream skips the rest")
  {
    inputs in{
        .characters = count_characters("abc"),
        .required_character = static_cast<unsigned char>('a'),
        .files = {},
        .streams = {{.path = "<bad>", .in = &bad.stream}, {.path = "<good>", .in = &good}},
    };
    auto const result = algorithm(in);
    REQUIRE(not result.has_value());
    CHECK(result.error() == read_error{{.path = "<bad>", .ec = std::make_error_code(std::errc::is_a_directory)}});
    // The second stream must not have been read: its read position remains at the start
    CHECK(good.tellg() == std::streampos(0));
    // No output should have been produced
    CHECK(capture.out.str().empty());
  }

  SECTION("error in the second stream preserves output from earlier streams")
  {
    inputs in{
        .characters = count_characters("abc"),
        .required_character = static_cast<unsigned char>('a'),
        .files = {},
        .streams = {{.path = "<good>", .in = &good}, {.path = "<bad>", .in = &bad.stream}},
    };
    auto const result = algorithm(in);
    REQUIRE(not result.has_value());
    // The error is tagged with the failing (second) stream's path, not the first
    CHECK(result.error() == read_error{{.path = "<bad>", .ec = std::make_error_code(std::errc::is_a_directory)}});
    // Output produced from the first stream before the failure must be preserved
    CHECK(capture.out.str() == "* abc\n");
  }
}
