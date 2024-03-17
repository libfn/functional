struct Error { std::string what; };
fn::expected<double, Error> ex{12.1};

auto value = ex 
    | and_then([](auto &&v) -> unsigned { return static_cast<unsigned>(v + 0.5); });

REQUIRE(value.value() == 13u);
