# filter

This section is testing znai integration with Doxygen.
See below for some possible uses.

> Seems like there is not much in terms of doc we can get for concepts?
-----
:include-doxygen-compound: fn::invocable_filter

-----
:include-doxygen-doc: fn::filter_t

-----
> Can't document niebloids (or even global variable) with this tool? The XML output of doxygen definitely contains this variable.
<!-- :include-doxygen-compound: fn::filter -->
<!-- :include-doxygen-doc: fn::filter -->

-----
:include-doxygen-compound: fn::filter_t

:include-doxygen-compound: fn::filter_t::apply

-----
## All the operator() overloads

> What i'd really like to be able to do is to grab all members of some entity and then use them as input to some template. So that we can generate similarly looking blocks of doc for each function or whatever.
> I could not find a way to do it yet in Znai.

:include-doxygen-member: fn::filter_t::apply::operator() { signatureOnly: true, includeAllMatches: true }

## Code example

```cpp
std::optional<int> opt1{42};
auto v1 = opt1 
    | filter([](auto&& i) { return i >= 42; });
REQUIRE(v1.value() == 42);

std::optional<int> opt2{12};
auto v2 = opt2 
    | filter([](auto&& i) { return i >= 42; });
REQUIRE(not v2.has_value());
```
