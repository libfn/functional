fn::optional<int> opt{42};

auto value = opt 
    | filter([](auto &&i) { return i >= 42; });

REQUIRE(value.value() == 42);
