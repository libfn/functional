fn::optional<double> op{std::nullopt};

auto value = op
    | and_then([](auto &&v) -> unsigned { return static_cast<unsigned>(v + 0.5); });

REQUIRE(not value.has_value());
