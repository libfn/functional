fn::optional<int> opt{12};

auto value = opt
    | filter([](auto&& i) { return i >= 42; });

REQUIRE(not value.has_value());
