// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_CHOICE
#define INCLUDE_FUNCTIONAL_CHOICE

#include "functional/detail/fwd_macro.hpp"
#include "functional/detail/meta.hpp"
#include "functional/sum.hpp"
#include "functional/utility.hpp"

#include <type_traits>
#include <utility>

namespace fn {

template <typename... Ts> struct choice;

template <> struct choice<>; // Intentionally incomplete

namespace detail {
template <typename... Ts> constexpr bool _is_some_choice = false;
template <typename... Ts> constexpr bool _is_some_choice<::fn::choice<Ts...> &> = true;
template <typename... Ts> constexpr bool _is_some_choice<::fn::choice<Ts...> const &> = true;
} // namespace detail

template <typename T>
concept some_choice = detail::_is_some_choice<T &>;

namespace detail {
template <typename T>
static constexpr bool _is_valid_choice_subtype //
    = (not std::is_same_v<void, T>)&&(not std::is_reference_v<T>)&&(not some_sum<T>)&&(not some_in_place_type<T>);
}

template <typename... Ts>
  requires(sizeof...(Ts) > 0)
struct choice<Ts...> : sum<Ts...> {
  static_assert((... && detail::_is_valid_choice_subtype<Ts>));
  static_assert(detail::is_normal_v<Ts...>);
  using _impl = sum<Ts...>;

  static constexpr std::size_t size = sizeof...(Ts);
  template <std::size_t I> using select_nth = detail::select_nth_t<I, Ts...>;
  template <typename T> static constexpr bool has_type = _impl::template has_type<T>;

  template <typename T>
  constexpr choice(T &&v)
    requires has_type<std::remove_reference_t<T>> && (std::is_constructible_v<std::remove_reference_t<T>, decltype(v)>)
             && (std::is_convertible_v<decltype(v), std::remove_reference_t<T>>)
      : _impl(std::in_place_type<std::remove_reference_t<T>>, FWD(v))
  {
  }

  template <typename T>
  constexpr explicit choice(T &&v)
    requires has_type<std::remove_reference_t<T>> && (std::is_constructible_v<std::remove_reference_t<T>, decltype(v)>)
             && (not std::is_convertible_v<decltype(v), std::remove_reference_t<T>>)
      : _impl(std::in_place_type<std::remove_reference_t<T>>, FWD(v))
  {
  }

  template <typename T>
  constexpr choice(std::in_place_type_t<T> d, auto &&...args) noexcept
    requires has_type<T>
      : _impl(d, FWD(args)...)
  {
  }

  template <typename... Tx>
  explicit constexpr choice(sum<Tx...> const &v) noexcept
    requires detail::is_superset_of<choice, choice<Tx...>> && (... && std::is_copy_constructible_v<Tx>)
      : _impl(std::in_place_type<sum<Tx...>>, FWD(v))
  {
  }

  template <typename... Tx>
  explicit constexpr choice(sum<Tx...> &&v) noexcept
    requires detail::is_superset_of<choice, choice<Tx...>> && (... && std::is_move_constructible_v<Tx>)
      : _impl(std::in_place_type<sum<Tx...>>, FWD(v))
  {
  }

  template <typename... Tx>
  constexpr choice(std::in_place_type_t<sum<Tx...>>, some_sum auto &&v) noexcept
    requires std::is_same_v<std::remove_cvref_t<decltype(v)>, sum<Tx...>>
             && detail::is_superset_of<choice, choice<Tx...>>
      : _impl(std::in_place_type<sum<Tx...>>, FWD(v))
  {
  }

  constexpr choice(choice const &other) noexcept = default;
  constexpr choice(choice &&other) noexcept = default;
  constexpr ~choice() = default;

  [[nodiscard]] constexpr bool operator==(choice const &rh) const noexcept
    requires(... && std::equality_comparable<Ts>)
  {
    return _impl::operator==(rh);
  }

  [[nodiscard]] constexpr bool operator!=(choice const &rh) const noexcept
    requires(... && std::equality_comparable<Ts>)
  {
    return not(*this == rh);
  }

  // NOTE Not strictly needed, but put here for sake of documentation
  using _impl::transform_to;

  // NOTE Monadic operations, only `and_then` and `transform` are supported
  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) & noexcept
    requires detail::typelist_invocable<Fn, choice &> && (not detail::typelist_type_invocable<Fn, choice &>)
  {
    using type = detail::_invoke_result<detail::_collapsing_sum_tag, decltype(fn), choice &>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) & noexcept
    requires detail::typelist_type_invocable<Fn, choice &>
  {
    using type = detail::_invoke_type_result<detail::_collapsing_sum_tag, decltype(fn), choice &>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) const & noexcept
    requires detail::typelist_invocable<Fn, choice const &> && (not detail::typelist_type_invocable<Fn, choice const &>)
  {
    using type = detail::_invoke_result<detail::_collapsing_sum_tag, decltype(fn), choice const &>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) const & noexcept
    requires detail::typelist_type_invocable<Fn, choice const &>
  {
    using type = detail::_invoke_type_result<detail::_collapsing_sum_tag, decltype(fn), choice const &>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) && noexcept
    requires detail::typelist_invocable<Fn, choice &&> && (not detail::typelist_type_invocable<Fn, choice &&>)
  {
    using type = detail::_invoke_result<detail::_collapsing_sum_tag, decltype(fn), choice &&>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(std::move(_impl::data), _impl::index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) && noexcept
    requires detail::typelist_type_invocable<Fn, choice &&>
  {
    using type = detail::_invoke_type_result<detail::_collapsing_sum_tag, decltype(fn), choice &&>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(std::move(_impl::data), _impl::index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) const && noexcept
    requires detail::typelist_invocable<Fn, choice const &&>
             && (not detail::typelist_type_invocable<Fn, choice const &&>)
  {
    using type = detail::_invoke_result<detail::_collapsing_sum_tag, decltype(fn), choice const &&>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(std::move(_impl::data), _impl::index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) const && noexcept
    requires detail::typelist_type_invocable<Fn, choice const &&>
  {
    using type = detail::_invoke_type_result<detail::_collapsing_sum_tag, decltype(fn), choice const &&>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(std::move(_impl::data), _impl::index, FWD(fn));
  }

  template <typename Fn> constexpr auto and_then(Fn &&fn) & noexcept -> decltype(this->transform_to(FWD(fn)))
  {
    static_assert(some_choice<decltype(this->transform_to(FWD(fn)))>);
    return this->transform_to(FWD(fn));
  }

  template <typename Fn> constexpr auto and_then(Fn &&fn) const & noexcept -> decltype(this->transform_to(FWD(fn)))
  {
    static_assert(some_choice<decltype(this->transform_to(FWD(fn)))>);
    return this->transform_to(FWD(fn));
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) && noexcept -> decltype(std::move(*this).transform_to(FWD(fn)))
  {
    static_assert(some_choice<decltype(std::move(*this).transform_to(FWD(fn)))>);
    return std::move(*this).transform_to(FWD(fn));
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const && noexcept -> decltype(std::move(*this).transform_to(FWD(fn)))
  {
    static_assert(some_choice<decltype(std::move(*this).transform_to(FWD(fn)))>);
    return std::move(*this).transform_to(FWD(fn));
  }
};

// CTAD for single-element choice
template <typename T> choice(std::in_place_type_t<T>, auto &&...) -> choice<T>;
template <typename T> choice(T) -> choice<T>;

template <typename... Ts> using choice_for = detail::normalized<Ts...>::template apply<choice>;

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_CHOICE
