// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_VALUE_OR
#define INCLUDE_FN_VALUE_OR

#include <fn/concepts.hpp>
#include <fn/functor.hpp>
#include <libfn_version.hpp>

#include <type_traits>

#include <fn/detail/macro_begin.hpp>

namespace fn {
inline namespace LIBFN_VERSION {
/**
 * @brief Checks if the monadic type can be used with the pipeline `value_or` operation
 *
 * @tparam V The monadic type
 * @tparam Args Arguments the fallback value is built from
 */
// The fallback builds the RESULT, not merely its value type: for an `optional<T&>` those differ -
// the value type is the referent, which a prvalue can construct, but the result binds a reference to
// it, which a prvalue cannot. And the existing value is carried over when there is one, so it must
// be able to survive that: an immovable value type would otherwise satisfy this and then fail inside
// the body. A void value is the degenerate case of both: `value_or()` substitutes the empty value,
// so the arguments must be none at all, and there is nothing to carry over.
template <typename V, typename... Args>
concept applicable_value_or                                                                                         //
    = (some_expected_non_void<V> && ::std::is_constructible_v<::std::remove_cvref_t<V>, ::std::in_place_t, Args...> //
       && detail::_relocatable_value<V>)
      || (some_expected_void<V> && ::std::is_constructible_v<::std::remove_cvref_t<V>, ::std::in_place_t, Args...>)
      || (some_optional<V> && ::std::is_constructible_v<::std::remove_cvref_t<V>, ::std::in_place_t, Args...>
          && detail::_relocatable_value<V>);

/**
 * @brief Supply a fallback for the failure state, keeping the carrier
 *
 * Unlike the member `value_or`, which eliminates the carrier and yields a value, the pipeline form
 * returns the carrier engaged with either its own value or a fallback built in place from the
 * arguments. Where the value type is `void`, `value_or()` takes no arguments at all. Rejected on
 * `choice` and `just`, which can never lack a value; on the identity `expected` the fallback stays
 * constrained yet dead - generic code compiles, the branch is never taken.
 *
 * Use through the `fn::value_or` nielbloid.
 */
constexpr inline struct value_or_t final {
  /**
   * @brief Supply a fallback for the failure state, keeping the carrier
   * @param args Arguments the fallback value is built from, in place
   * @return A functor that will substitute the fallback where the value is missing
   */
  template <typename... Args>
  [[nodiscard]] constexpr auto operator()(Args &&...args) const
      noexcept(noexcept(functor<value_or_t, Args &&...>{FWD(args)...})) -> functor<value_or_t, Args &&...> //
  {
    return {FWD(args)...};
  }

  struct apply;
} value_or = {};

struct value_or_t::apply final {
  /**
   * @brief Returns the operand engaged with its own value, or the fallback built from the arguments
   *
   * @param v The monad
   * @param args Arguments the fallback value is built from, in place
   * @return A carrier of the same type, holding a value
   */
  // The fallback is built inside, so its construction is weighed here rather than propagated: the
  // callable or_else receives is a lambda, which cannot be named in this specification.
  // An identity expected is deliberately not excluded with choice and just: its fallback stays
  // constrained yet dead, the delegated member or_else being vacuous.
  template <some_monadic_type V, typename... Args>
  [[nodiscard]] constexpr auto operator()(V &&v, Args &&...args) const //
      noexcept(::std::is_nothrow_constructible_v<::std::remove_cvref_t<V>, ::std::in_place_t, Args...>
               && detail::_nothrow_carry_value<::std::remove_cvref_t<V>, V>) -> ::std::remove_cvref_t<V>
    requires(not some_choice<V>) && (not some_just<V>) && applicable_value_or<V &&, Args...>
  {
    using type = ::std::remove_cvref_t<V>;
    return FWD(v).or_else([&args...](auto &&...) -> type { return type{::std::in_place, FWD(args)...}; });
  }
};

} // namespace LIBFN_VERSION
} // namespace fn

#include <fn/detail/macro_end.hpp>

#endif // INCLUDE_FN_VALUE_OR
