static constexpr auto parse = [](std::string_view str) noexcept
      -> fn::choice_for<bool, double, long, std::string_view, std::nullptr_t, std::nullopt_t> {
    if (str.size() > 0) {
      if (str.size() > 1 && str[0] == '\'' && str[str.size() - 1] == '\'')
        return {str.substr(1, str.size() - 2)};
      else if (str.size() > 1 && str[0] == '\"' && str[str.size() - 1] == '\"')
        return {str.substr(1, str.size() - 2)};
      else if (str == "true")
        return {true};
      else if (str == "false")
        return {false};
      else if (str == "null")
        return {nullptr};
      else {
        if (str.find_first_not_of("01234567890") != std::string_view::npos) {
          double tmp = {};
          // TODO switch to std::from_chars when supported by libc++
          std::istringstream ss{std::string{str.data(), str.size()}};
          if ((ss >> tmp)) {
            return {tmp};
          }
        } else {
          long tmp = {};
          auto const end = str.data() + str.size();
          if (std::from_chars(str.data(), end, tmp).ptr == end) {
            return {tmp};
          }
        }
      }
      return {std::nullopt};
    }
    return {nullptr};
  };

  static_assert(std::is_same_v<decltype(parse("")),
                               fn::choice_for<bool, double, long, std::string_view, std::nullopt_t, std::nullptr_t>>);
  CHECK(parse("'abc'") == fn::choice{std::string_view{"abc"}});
  CHECK(parse(R"("def")") == fn::choice{std::string_view{"def"}});
  CHECK(parse("null") == fn::choice(nullptr));
  CHECK(parse("") == fn::choice(nullptr));
  CHECK(parse("true") == fn::choice(true));
  CHECK(parse("false") == fn::choice(false));
  CHECK(parse("1025") == fn::choice(1025l));
  CHECK(parse("10.25") == fn::choice(10.25));
  CHECK(parse("2e9") == fn::choice(2e9));
  CHECK(parse("5e9") == fn::choice(5e9));
  CHECK(parse("foo").has_value(std::in_place_type<std::nullopt_t>));

