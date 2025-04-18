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

#include <concepts>
#include <type_traits>

namespace fn {
/**
 * @brief TODO
 *
 * @tparam Functor TODO
 * @tparam V TODO
 * @tparam Args TODO
 */
template <typename Functor, typename V, typename... Args>
concept monadic_invocable //
    = some_monadic_type<V> && ::std::invocable<typename Functor::apply, V, Args...>;

/**
 * @brief TODO
 *
 * @tparam Functor TODO
 * @tparam Args TODO
 */
template <typename Functor, typename... Args> struct functor final {
  using functor_type = Functor;
  using functor_apply = typename functor_type::apply;
  static constexpr unsigned size = sizeof...(Args);
  using data_t = pack<as_value_t<Args>...>;
  data_t data;

  static_assert(std::is_empty_v<functor_type> && std::is_empty_v<functor_apply>
                && std::is_default_constructible_v<functor_type> && std::is_default_constructible_v<functor_apply>);

  /**
   * @brief TODO
   *
   * @param v TODO
   * @param self TODO
   * @return TODO
   */
  [[nodiscard]] constexpr friend auto operator|(some_monadic_type auto &&v, auto &&self) noexcept -> decltype(auto)
    requires std::same_as<std::remove_cvref_t<decltype(self)>, functor>
             && monadic_invocable<functor_type, decltype(v), Args...>
  {
    return data_t::_swap_invoke(FWD(self).data, functor_apply{}, FWD(v));
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_FUNCTOR
