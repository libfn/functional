struct Error { std::string what; };
fn::expected<int, Error> ex{12};

auto value = ex 
    | filter([](auto &&i) { return i >= 42; }, 
             [](auto) { return Error{"Less than 42"}; });

REQUIRE(value.error().what == "Less than 42");
