// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_FUNCTOR
#define INCLUDE_FN_FUNCTOR

#include <fn/concepts.hpp>
#include <fn/functional.hpp>
#include <fn/pack.hpp>
#include <fn/utility.hpp>
#include <libfn_version.hpp>

#include <concepts>
#include <type_traits>

#include <fn/detail/macro_begin.hpp>

namespace fn {
inline namespace LIBFN_VERSION {
/**
 * @brief A pipeline step waiting for a carrier: the verb and its arguments as one value
 *
 * What a verb call such as `fn::and_then(f)` returns - a description of the operation, executed
 * only when a carrier is piped into it with `operator|`. The arguments are stored as `as_value_t`
 * prescribes: rvalues by value, non-empty lvalues - an immovable callable among them - by
 * reference.
 *
 * @tparam Functor The verb, such as `fn::and_then_t`
 * @tparam Args The verb's arguments as deduced, typically the callback
 */
template <typename Functor, typename... Args> struct functor final {
  using functor_type = Functor;
  using functor_apply = typename functor_type::apply;
  static constexpr unsigned size = sizeof...(Args);
  using data_t = pack<as_value_t<Args>...>;
  data_t data;

  static_assert(::std::is_empty_v<functor_type> && ::std::is_empty_v<functor_apply>
                && ::std::is_default_constructible_v<functor_type> && ::std::is_default_constructible_v<functor_apply>);

  /**
   * @brief Feeds a carrier into the pipeline step
   *
   * @param v The carrier, in any value category
   * @param self The step - this `functor`
   * @return Whatever the verb's `apply` returns for this carrier and the stored arguments
   */
  // The pipeline propagates what the operation itself promises: the verb's `apply` knows, and
  // `_swap_invoke` carries that answer up. Promising `noexcept` here regardless would turn an
  // exception the monad's own member would propagate into a call to std::terminate.
  [[nodiscard]] constexpr friend auto operator|(some_monadic_type auto &&v, auto &&self) //
      noexcept(noexcept(data_t::_swap_invoke(FWD(self).data, functor_apply{}, FWD(v)))) -> decltype(auto)
    requires ::std::same_as<::std::remove_cvref_t<decltype(self)>, functor>
             && monadic_invocable<functor_type, decltype(v), Args...>
  {
    return data_t::_swap_invoke(FWD(self).data, functor_apply{}, FWD(v));
  }
};

} // namespace LIBFN_VERSION
} // namespace fn

#include <fn/detail/macro_end.hpp>

#endif // INCLUDE_FN_FUNCTOR
