#include <concepts>
#include <fn/and_then.hpp>
#include <fn/choice.hpp>
#include <fn/concepts.hpp>
#include <fn/copack.hpp>
#include <fn/discard.hpp>
#include <fn/expected.hpp>
#include <fn/fail.hpp>
#include <fn/filter.hpp>
#include <fn/inspect.hpp>
#include <fn/inspect_error.hpp>
#include <fn/just.hpp>
#include <fn/or_else.hpp>
#include <fn/pack.hpp>
#include <fn/recover.hpp>
#include <fn/transform.hpp>
#include <fn/transform_error.hpp>
#include <fn/utility.hpp>
#include <fn/value_or.hpp>
#include <string_view>

// sync-example-types-def
struct Error {};
struct OtherError {};

struct A {};
struct B {};
struct C {};
struct D {};

struct UserId {};
struct User {};
struct FilePath {};
struct MaximumSize {};
struct BlockSize {};

struct NotANumber {};
struct OutOfRange {};
struct Missing {
  auto operator<=>(Missing const &) const = default;
};
struct IoError {};
struct BadSyntax {};
struct UnknownKey {};
// sync-example-types-def

// Dummy definitions for non-linked prototypes to prevent linker errors:
fn::expected<UserId, fn::copack<NotANumber>> parse_id(std::string_view)
{
  return fn::expected<UserId, fn::copack<NotANumber>>{::fn::unexpect, NotANumber{}};
}
fn::expected<UserId, fn::copack<OutOfRange>> validate(UserId)
{
  return fn::expected<UserId, fn::copack<OutOfRange>>{::fn::unexpect, OutOfRange{}};
}
fn::expected<User, fn::copack_for<IoError, Missing>> load(UserId)
{
  return fn::expected<User, fn::copack_for<IoError, Missing>>{::fn::unexpect, IoError{}};
}
fn::expected<UserId, fn::copack<NotANumber>> parse_numeric()
{
  return fn::expected<UserId, fn::copack<NotANumber>>{::fn::unexpect, NotANumber{}};
}
fn::expected<User, fn::copack<Missing>> load_user(UserId)
{
  return fn::expected<User, fn::copack<Missing>>{::fn::unexpect, Missing{}};
}
fn::expected<fn::copack_for<MaximumSize, FilePath, BlockSize>,
             fn::copack_for<BadSyntax, UnknownKey>>
read_config()
{
  return fn::expected<fn::copack_for<MaximumSize, FilePath, BlockSize>,
                      fn::copack_for<BadSyntax, UnknownKey>>{::fn::unexpect, BadSyntax{}};
}

// sync-example-graded-pipeline
auto parse_id(std::string_view) -> fn::expected<UserId, fn::copack<NotANumber>>;
auto validate(UserId) -> fn::expected<UserId, fn::copack<OutOfRange>>;
auto load(UserId) -> fn::expected<User, fn::copack_for<IoError, Missing>>;

auto graded_pipeline(std::string_view sv) -> void
{
  auto result = parse_id(sv) | fn::and_then(validate) | fn::and_then(load);

  // The exact derived error union of the pipeline is recorded in the type:
  static_assert(
      std::same_as<decltype(result),
                   fn::expected<User, fn::copack_for<IoError, Missing, NotANumber, OutOfRange>>>);
}
// sync-example-graded-pipeline

// sync-example-copack-set-semantics
auto test_copack_set_semantics() -> void
{
  using SetA = fn::copack_for<NotANumber, OutOfRange>;
  using SetB = fn::copack_for<Missing, OutOfRange>;

  // Flattening, deduplication, and reordering happen automatically:
  using Union = fn::copack_for<SetA, SetB, Missing, Missing, fn::copack<IoError>>;

  static_assert(std::same_as<Union, fn::copack_for<IoError, Missing, NotANumber, OutOfRange>>);
}
// sync-example-copack-set-semantics

// sync-example-test-pack
auto test_pack(int x = 12, double d = 3.14) -> void
{
  fn::pack p{UserId{}, User{}};         // CTAD
  [[maybe_unused]] auto [id, user] = p; // Structured bindings work naturally

  // Ordered, non-deduplicated fields
  using P = fn::pack<UserId, User, User>;
  static_assert(std::tuple_size_v<P> == 3);

  // Found via ADL (like std::get)
  using std::get;
  static_assert(std::same_as<decltype(get<0>(p)), UserId &>);

  // Splicing scalars or other packs via append:
  auto row = fn::pack{UserId{}}.append(FilePath{});
  auto wider = std::move(row).append(fn::pack{true, 3});

  static_assert(std::same_as<decltype(wider), fn::pack<UserId, FilePath, bool, int>>);

  // Explicitly lifting a single scalar value into a pack:
  auto lifted_lvalue = fn::as_pack(x);
  static_assert(std::same_as<decltype(lifted_lvalue), fn::pack<int &>>);

  auto lifted_rvalue = fn::as_pack(42);
  static_assert(std::same_as<decltype(lifted_rvalue), fn::pack<int>>);

  // Spelling the element type explicitly can be used to opt out of reference preservation:
  auto copied = fn::as_pack<int>(x);
  static_assert(std::same_as<decltype(copied), fn::pack<int>>);
  // ... or to force a specific reference type (subject to parameter binding rules):
  auto referenced = fn::as_pack<int const &>(x);
  static_assert(std::same_as<decltype(referenced), fn::pack<int const &>>);

  // The explicit form also coerces - the argument converts at the call boundary:
  auto coerced = fn::as_pack<bool, int>(x, d);
  static_assert(std::same_as<decltype(coerced), fn::pack<bool, int>>);
}
// sync-example-test-pack

// sync-example-test-copack
struct IntegerToken {};
struct StringToken {};

auto test_copack() -> void
{
  static constexpr fn::copack_for<IntegerToken, StringToken> token = IntegerToken{};

  // Member apply eliminates the copack by routing the active alternative to an overload set:
  static constexpr auto value
      = token.apply(fn::overload{[](IntegerToken) { return 1; }, [](StringToken) { return 2; }});
  static_assert(value == 1);

  // Storing a pack inside a copack is allowed, including a pack holding a reference:
  auto cpr = fn::as_copack(fn::as_pack<int const &>(value));
  static_assert(std::same_as<decltype(cpr), fn::copack<fn::pack<int const &>>>);

  // Singular lift and direct value extraction (only allowed for singular copacks):
  auto cp = fn::as_copack(42);
  using std::get;
  static_assert(std::same_as<decltype(get(cp)), int &>);
}
// sync-example-test-copack

// sync-example-mapping-values-and-errors
auto mapping_values_and_errors() -> void
{
  fn::expected<UserId, fn::copack_for<Missing, IoError>> ex{};

  auto mapped_val = ex | fn::transform([](UserId) { return User{}; });
  static_assert(
      std::same_as<decltype(mapped_val), fn::expected<User, fn::copack_for<IoError, Missing>>>);

  auto mapped_err = ex
                    | fn::transform_error(fn::overload{[](Missing) { return BadSyntax{}; },
                                                       [](IoError e) { return e; }});

  static_assert(
      std::same_as<decltype(mapped_err), fn::expected<UserId, fn::copack_for<BadSyntax, IoError>>>);
}
// sync-example-mapping-values-and-errors

// sync-example-operator-and-composition
auto operator_and_composition(fn::expected<int, Error> a, fn::expected<bool, OtherError> b) -> void
{
  auto result = a & b;
  static_assert(std::same_as<decltype(result),
                             fn::expected<fn::pack<int, bool>, fn::copack_for<Error, OtherError>>>);
}
// sync-example-operator-and-composition

// sync-example-cartesian-distribution
auto test_cartesian_distribution(fn::copack_for<A, B> ab, fn::copack_for<C, D> cd) -> void
{
  // Cartesian distribution of copacks: (A + B) x (C + D) = (A x C) + (A x D) + (B x C) + (B x D)
  auto result1 = ab & cd;
  static_assert(
      std::same_as<decltype(result1),
                   fn::copack_for<fn::pack<A, C>, fn::pack<A, D>, fn::pack<B, C>, fn::pack<B, D>>>);

  // Cartesian distribution of a pack and a copack: (A x B) x (C + D) = (A x B x C) + (A x B x D)
  constexpr fn::pack<A, B> Pab = {A{}, B{}};
  auto result2 = Pab & cd;
  static_assert(
      std::same_as<decltype(result2), fn::copack_for<fn::pack<A, B, C>, fn::pack<A, B, D>>>);
}
// sync-example-cartesian-distribution

// sync-example-conjunction-with-identity-cluster
auto test_conjunction_with_identity_cluster(fn::expected<int, Error> ex, fn::just<double> j) -> void
{
  // Conjoining an expected with a just
  auto res1 = ex & j;
  static_assert(std::same_as<decltype(res1), fn::expected<fn::pack<int, double>, Error>>);

  // Conjoining with a unit (just<void>) completely elides the unit
  auto res2 = ex & fn::just<void>{};
  static_assert(std::same_as<decltype(res2), decltype(ex)>);

  // Conjoining a choice causes distribution inside the carrier
  fn::choice<bool, double> ch = 1.5;
  auto res3 = ex & ch;
  static_assert(std::same_as<
                decltype(res3),
                fn::expected<fn::copack_for<fn::pack<int, double>, fn::pack<int, bool>>, Error>>);
}
// sync-example-conjunction-with-identity-cluster

// sync-example-operator-or-composition
auto operator_or_composition(fn::expected<int, Error> a, fn::expected<bool, OtherError> b) -> void
{
  auto result = a | b;
  static_assert(std::same_as<decltype(result),
                             fn::expected<fn::copack_for<int, bool>, fn::pack<Error, OtherError>>>);
}
// sync-example-operator-or-composition

// sync-example-test-disjoin
auto test_disjoin(fn::expected<int, Error> a, fn::expected<bool, OtherError> b) -> void
{
  // Multiple fallible operands compose cleanly
  auto res1 = fn::disjoin(a, b);
  static_assert(std::same_as<decltype(res1),
                             fn::expected<fn::copack_for<int, bool>, fn::pack<Error, OtherError>>>);

  // Because just<double> cannot fail, the entire disjunction with infallible operands becomes total
  auto res2 = fn::disjoin(a, b, fn::just<double>{1.5});
  static_assert(std::same_as<decltype(res2), fn::choice<bool, double, int>>);
}
// sync-example-test-disjoin

// sync-example-sequential-bind
auto parse_numeric() -> fn::expected<UserId, fn::copack<NotANumber>>;
auto load_user(UserId) -> fn::expected<User, fn::copack<Missing>>;

auto sequential_bind() -> void
{
  auto result = parse_numeric() | fn::and_then(load_user);
  static_assert(
      std::same_as<decltype(result), fn::expected<User, fn::copack_for<Missing, NotANumber>>>);
}
// sync-example-sequential-bind

// sync-example-test-same-kind
static_assert(fn::same_kind<fn::optional<int>, fn::optional<User>>);
static_assert(fn::same_kind<fn::expected<int, IoError>, fn::expected<User, IoError>>);
static_assert(!fn::same_kind<fn::expected<int, IoError>, fn::expected<User, Missing>>);
// sync-example-test-same-kind

// sync-example-config-pipeline
auto read_config() -> fn::expected<fn::copack_for<MaximumSize, FilePath, BlockSize>,
                                   fn::copack_for<BadSyntax, UnknownKey>>;

auto config_pipeline() -> void
{
  auto validated
      = read_config()
        | fn::and_then(fn::overload{
            [](MaximumSize v) { return fn::expected<MaximumSize, fn::copack<OutOfRange>>{v}; },
            [](FilePath v) { return fn::expected<FilePath, fn::copack<Missing>>{v}; },
            [](BlockSize v) { return fn::expected<BlockSize, fn::copack<OutOfRange>>{v}; }});

  // The result exactly bounds both the successful paths and the error paths
  static_assert(
      std::same_as<decltype(validated),
                   fn::expected<fn::copack_for<BlockSize, FilePath, MaximumSize>,
                                fn::copack_for<BadSyntax, Missing, OutOfRange, UnknownKey>>>);
}
// sync-example-config-pipeline

// sync-example-test-same-kind-graded
static_assert(
    fn::same_kind<fn::expected<int, fn::copack<IoError>>, fn::expected<User, fn::copack<Missing>>>);
// sync-example-test-same-kind-graded

// sync-example-test-explicit-lifting
auto test_explicit_lifting(fn::expected<User, IoError> result, fn::optional<User> opt) -> void
{
  // Explicitly lift the error side of expected:
  auto graded_err = std::move(result).copack_error();
  static_assert(std::same_as<decltype(graded_err), fn::expected<User, fn::copack<IoError>>>);

  // Explicitly lift the value side of expected:
  auto graded_val = std::move(result).copack_value();
  static_assert(std::same_as<decltype(graded_val), fn::expected<fn::copack<User>, IoError>>);

  // Explicitly lift the value side of optional:
  auto graded_opt = std::move(opt).copack_value();
  static_assert(std::same_as<decltype(graded_opt), fn::optional<fn::copack<User>>>);
}
// sync-example-test-explicit-lifting

template <typename T> using cannot_fail_t = fn::expected<T, fn::copack<>>;

// sync-example-test-identity-cross
auto test_identity_cross() -> void
{
  fn::just<UserId> j{UserId{}};

  // Cross-carrier pipeline bind to another identity carrier
  auto result = j | fn::and_then([](UserId u) { return fn::expected<UserId, fn::copack<>>{u}; });

  static_assert(std::same_as<decltype(result), fn::expected<UserId, fn::copack<>>>);
}
// sync-example-test-identity-cross

// sync-example-test-success-bridge
auto test_success_bridge(fn::just<int> j) -> void
{
  // An identity carrier can bridge to fallible carriers on the success path
  auto to_opt = j | fn::and_then([](int i) { return fn::optional<int>{i}; });
  static_assert(std::same_as<decltype(to_opt), fn::optional<int>>);

  auto to_exp = j | fn::and_then([](int i) { return fn::expected<int, IoError>{i}; });
  static_assert(std::same_as<decltype(to_exp), fn::expected<int, IoError>>);

  // Bridging a multi-alternative choice to fallible optional with heterogeneous success join:
  auto choice_to_opt
      = fn::choice_for<int, bool>{true}
        | fn::and_then(fn::overload{[](int) -> fn::optional<char> { return {'a'}; },
                                    [](bool) -> fn::optional<long> { return {2L}; }});
  static_assert(std::same_as<decltype(choice_to_opt), fn::optional<fn::copack_for<char, long>>>);
}
// sync-example-test-success-bridge

// sync-example-test-failure-bridge
auto test_failure_bridge(fn::expected<int, IoError> ex, fn::optional<int> opt) -> void
{
  // Fallible carriers can bridge to each other on the failure/empty recovery path
  auto expected_to_optional = ex | fn::or_else([](IoError) { return fn::optional<int>{}; });
  static_assert(std::same_as<decltype(expected_to_optional), fn::optional<int>>);

  auto optional_to_expected = opt | fn::or_else([]() { return fn::expected<int, IoError>{100}; });
  static_assert(std::same_as<decltype(optional_to_expected), fn::expected<int, IoError>>);
}
// sync-example-test-failure-bridge

// sync-example-vacuous-or-else
auto test_vacuous_or_else() -> void
{
  using type = decltype(fn::expected<void, fn::copack<>>{} | fn::or_else(std::declval<int>()));
  static_assert(std::same_as<type, fn::expected<void, fn::copack<>>>);
}
// sync-example-vacuous-or-else

// sync-example-test-identity-transformation
auto test_identity_transformation(fn::just<UserId> j) -> void
{
  // Transforming a just with a callable returning a copack produces a choice
  auto mapped
      = j | fn::transform([](UserId) { return fn::copack_for<Missing, FilePath>{Missing{}}; });

  static_assert(std::same_as<decltype(mapped), fn::choice_for<FilePath, Missing>>);
}
// sync-example-test-identity-transformation

// sync-example-test-choice-mapping
auto test_choice_mapping(fn::choice<User, UserId> ch) -> void
{
  constexpr auto mapper = fn::overload{[](UserId) { return fn::choice<Missing>{Missing{}}; },
                                       [](User) { return fn::choice<FilePath>{FilePath{}}; }};

  // transform nests the returned choice as a mapped value
  auto mapped = ch | fn::transform(mapper);
  static_assert(
      std::same_as<decltype(mapped), fn::choice_for<fn::choice<FilePath>, fn::choice<Missing>>>);

  // and_then joins and flattens them into a normalized superset choice
  auto bound = ch | fn::and_then(mapper);
  static_assert(std::same_as<decltype(bound), fn::choice_for<FilePath, Missing>>);
}
// sync-example-test-choice-mapping

// sync-example-test-elimination
auto test_elimination(fn::expected<UserId, fn::copack<Missing>> ex) -> int
{
  return ex.apply(fn::overload{[](UserId) { return 1; }, [](Missing) { return 0; }});
}
// sync-example-test-elimination

// sync-example-test-laws
constexpr auto test_laws() -> void
{
  constexpr fn::expected<int, fn::copack<Missing>> ex{42};

  // Functor Identity: mapping with identity yields the same value
  constexpr auto id = [](auto v) { return v; };
  static_assert((ex | fn::transform(id)) == ex);

  // Monad Left Identity: pure(x) >>= f is equivalent to f(x)
  constexpr auto pure = [](int v) { return fn::expected<int, fn::copack<Missing>>{v}; };
  constexpr auto f = [](int v) { return fn::expected<int, fn::copack<Missing>>{v * 2}; };
  static_assert((pure(42) | fn::and_then(f)) == f(42));
}
// sync-example-test-laws

// sync-example-test-references
auto test_references() -> void
{
  int x = 42;

  // optional supports references directly
  fn::optional<int &> opt{x};
  static_assert(std::same_as<decltype(opt.value()), int &>);

  // expected must wrap references inside a pack
  fn::expected<fn::pack<int &>, Error> ex{fn::as_pack(x)};
  static_assert(std::same_as<decltype(ex.value()), fn::pack<int &> &>);
}
// sync-example-test-references

int main()
{
  // Touch all functions to prove compilability and execution
  std::string_view sv = "";
  graded_pipeline(sv);
  operator_and_composition({}, {});
  test_copack_set_semantics();
  test_pack();
  test_copack();
  mapping_values_and_errors();
  test_cartesian_distribution(A{}, C{});
  test_conjunction_with_identity_cluster({42}, {1.5});
  operator_or_composition({}, {});
  test_disjoin({12}, {true});
  sequential_bind();
  config_pipeline();
  test_explicit_lifting(fn::expected<User, IoError>{User{}}, fn::optional<User>{User{}});
  test_identity_cross();
  test_success_bridge({1});
  test_failure_bridge(fn::expected<int, IoError>{}, fn::optional<int>{});
  test_vacuous_or_else();
  test_identity_transformation({UserId{}});
  test_choice_mapping({UserId{}});
  (void)test_elimination(fn::expected<UserId, fn::copack<Missing>>{UserId{}});
  test_laws();
  test_references();
  return 0;
}
