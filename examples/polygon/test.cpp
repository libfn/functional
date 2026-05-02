// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "polygon.hpp"

#include <catch2/catch_all.hpp>

#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <istream>
#include <sstream>
#include <streambuf>
#include <string>
#include <string_view>

#ifdef _WIN32
#include <cstdint>
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
    if (ec) {
      FAIL_CHECK("failed to remove temporary directory " << path << ": " << ec.message());
    }
  }
};

struct temp_file : temp_dir {
  std::filesystem::path file;

  explicit temp_file(std::string_view content) : file(path / "content.txt") { std::ofstream(file) << content; }
  ~temp_file()
  {
    // If the file was never created (silent ofstream failure), remove() is a no-op via the ec overload.
    std::error_code ec;
    std::filesystem::remove(file, ec);
    if (ec) {
      FAIL_CHECK("failed to remove temporary file " << file << ": " << ec.message());
    }
  }
};

struct temp_symlink {
  std::filesystem::path link;
  std::error_code ec;

  temp_symlink(std::filesystem::path const &target, std::filesystem::path new_symlink) : link(std::move(new_symlink))
  {
    std::filesystem::create_symlink(target, link, ec);
  }
  temp_symlink(temp_symlink const &) = delete;
  temp_symlink &operator=(temp_symlink const &) = delete;
  ~temp_symlink()
  {
    // If create_symlink failed in the ctor, remove() is a no-op via the ec overload.
    std::error_code ec;
    std::filesystem::remove(link, ec);
    if (ec) {
      FAIL_CHECK("failed to remove temporary symlink " << link << ": " << ec.message());
    }
  }
};

// Helper producing a std::istream whose first read marks the stream as bad() without throwing.
// This allows testing of the algorithm's read_error path in a portable way.
struct failing_stream {
  struct buf_t : std::streambuf {
    std::istream *bound = nullptr;

  protected:
    int_type underflow() override
    {
      if (bound) {
        bound->setstate(std::ios_base::badbit);
      }
      return traits_type::eof();
    }
  } buf = {};
  std::istream stream;

  failing_stream() : stream(&buf) { buf.bound = &stream; }
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
  auto const result = parameters::make({"prog", "abcde", "f1.txt", "f2.txt"});
  REQUIRE(result.has_value());
  CHECK(result->characters == "abcde");
  REQUIRE(result->files.size() == 2);
  auto it = result->files.begin();
  CHECK(*it == "f1.txt");
  CHECK(*++it == "f2.txt");
}

TEST_CASE("parameters::make zero parameters", "[polygon][parameters]")
{
  auto const result = parameters::make({});
  REQUIRE(not result.has_value());
  CHECK(result.error() == parameters::error{parameters::too_few_parameters{"<program>"}});
}

TEST_CASE("parameters::make empty program name", "[polygon][parameters]")
{
  auto const result = parameters::make({""});
  REQUIRE(not result.has_value());
  CHECK(result.error() == parameters::error{parameters::too_few_parameters{"<program>"}});
}

TEST_CASE("parameters::make too few parameters", "[polygon][parameters]")
{
  auto const result = parameters::make({"polygon"});
  REQUIRE(not result.has_value());
  CHECK(result.error() == parameters::error{parameters::too_few_parameters{"polygon"}});
}

TEST_CASE("parameters::make rejects an invalid characters argument", "[polygon][parameters]")
{
  SECTION("too few characters")
  {
    auto const result = parameters::make({"prog", "ab"});
    REQUIRE(not result.has_value());
    CHECK(result.error() == parameters::error{parameters::too_few_characters{}});
  }

  SECTION("non-ascii character")
  {
    auto const result = parameters::make({"prog", "łąki"});
    REQUIRE(not result.has_value());
    CHECK(result.error() == parameters::error{parameters::non_ascii_characters{}});
  }
}

TEST_CASE("inputs::make", "[polygon][inputs]")
{
  SECTION("empty file list falls back to stdin")
  {
    parameters const p{.characters = "abc", .required = static_cast<unsigned char>('a'), .files = {}};
    auto const result = inputs::make(p);
    REQUIRE(result.has_value());
    REQUIRE(result->streams.size() == 1);
    CHECK(result->streams.front().path == "<stdin>");
    CHECK(result->streams.front().in == &std::cin);
    CHECK(result->required == static_cast<unsigned char>('a'));
  }

  SECTION("nonexistent path yields file_not_found")
  {
    temp_dir const td;
    auto const missing = td.path / "nonexistent_file.txt";
    parameters const p{.characters = "abc", .required = static_cast<unsigned char>('a'), .files = {missing.string()}};
    auto const result = inputs::make(p);
    REQUIRE(not result.has_value());
    REQUIRE(result.error().has_value<file_not_found>());
    auto const &err = *result.error().get_ptr<file_not_found>();
    CHECK(err.path == missing);
    CHECK(err.ec == std::errc::no_such_file_or_directory);
  }

  SECTION("path with non-directory component yields file_not_found")
  {
    temp_file const tf("");
    auto const subpath = tf.file / "nonexistent_subdirectory";
    parameters const p{.characters = "abc", .required = static_cast<unsigned char>('a'), .files = {subpath.string()}};
    auto const result = inputs::make(p);
    REQUIRE(not result.has_value());
    REQUIRE(result.error().has_value<file_not_found>());
    auto const &err = *result.error().get_ptr<file_not_found>();
    CHECK(err.path == subpath);
    CHECK(err.ec == std::errc::not_a_directory);
  }

  SECTION("symlink loop yields io_error")
  {
    // A self-referential symlink pair causes status() to fail with an ec that is neither
    // no_such_file_or_directory nor permission_denied, so the error is routed to io_error.
    temp_dir const td;
    auto const a = td.path / "a";
    auto const b = td.path / "b";

    // Two symlinks pointing at each other. RAII removes them on any exit path (including a failed
    // REQUIRE). If the first creation fails, the second is attempted as a no-op via the same path.
    temp_symlink const link_a{b, a};
    temp_symlink const link_b{a, b};
    if (link_a.ec || link_b.ec) {
      SKIP("symlinks not supported in this environment: " << (link_a.ec ? link_a.ec : link_b.ec).message());
    }
    parameters const p{.characters = "abc", .required = static_cast<unsigned char>('a'), .files = {a.string()}};
    auto const result = inputs::make(p);
    REQUIRE(not result.has_value());
    REQUIRE(result.error().has_value<io_error>());
    auto const &err = *result.error().get_ptr<io_error>();
    CHECK(err.path == a);
    CHECK(err.ec);
  }

  SECTION("opens a real file")
  {
    temp_file const tf("hello\nworld\n");
    parameters const p{.characters = "abc", .required = static_cast<unsigned char>('a'), .files = {tf.file.string()}};
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
      .required = static_cast<unsigned char>('a'),
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
  // Force the algorithm down its read_error path with a stream that fails on first read.
  failing_stream bad;

  inputs in{
      .characters = count_characters("abc"),
      .required = static_cast<unsigned char>('a'),
      .files = {},
      .streams = {{.path = "<bad>", .in = &bad.stream}},
  };

  cout_capture capture; // suppress incidental output
  auto const result = algorithm(in);
  REQUIRE(not result.has_value());
  CHECK(result.error() == read_error{{.path = "<bad>", .ec = std::make_error_code(std::io_errc::stream)}});
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
      .required = static_cast<unsigned char>('a'),
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
  // The "bad" stream's first read marks the stream as bad() without throwing
  failing_stream bad;

  std::istringstream good("abc\n");
  cout_capture capture;

  SECTION("error in the first stream skips the rest")
  {
    inputs in{
        .characters = count_characters("abc"),
        .required = static_cast<unsigned char>('a'),
        .files = {},
        .streams = {{.path = "<bad>", .in = &bad.stream}, {.path = "<good>", .in = &good}},
    };
    auto const result = algorithm(in);
    REQUIRE(not result.has_value());
    CHECK(result.error() == read_error{{.path = "<bad>", .ec = std::make_error_code(std::io_errc::stream)}});
    CHECK(good.tellg() == std::streampos(0));
    CHECK(capture.out.str().empty());
  }

  SECTION("error in the second stream preserves output from earlier streams")
  {
    inputs in{
        .characters = count_characters("abc"),
        .required = static_cast<unsigned char>('a'),
        .files = {},
        .streams = {{.path = "<good>", .in = &good}, {.path = "<bad>", .in = &bad.stream}},
    };
    auto const result = algorithm(in);
    REQUIRE(not result.has_value());
    CHECK(result.error() == read_error{{.path = "<bad>", .ec = std::make_error_code(std::io_errc::stream)}});
    CHECK(capture.out.str() == "* abc\n");
  }
}
