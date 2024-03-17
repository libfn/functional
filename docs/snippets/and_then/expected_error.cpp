struct Error { std::string what; };
fn::expected<double, Error> ex = std::unexpected<Error>{"Not good"};

auto value = ex 
    | and_then([](auto &&v) -> unsigned { return static_cast<unsigned>(v + 0.5); }); // Not called because ex contains an Error

REQUIRE(value.error().what == "Not good");
