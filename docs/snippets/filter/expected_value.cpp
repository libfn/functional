struct Error {
  std::string what;
};
fn::expected<int, Error> ex{42};

auto value = ex
             | filter([](auto &&i) { return i >= 42; },            // Filter out values less than 42
                      [](auto) { return Error{"Less than 42"}; }); // Return error if predicate fails

REQUIRE(value.value() == 42);
