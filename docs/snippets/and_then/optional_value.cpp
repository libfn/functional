fn::optional<double> op{12.1};

auto value = op
    | and_then([](auto &&v) -> unsigned { return static_cast<unsigned>(v + 0.5); });

REQUIRE(value.value() == 13u);
