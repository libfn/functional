// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_CHOICE
#define INCLUDE_FUNCTIONAL_CHOICE

#include "functional/detail/meta.hpp"
#include "functional/sum.hpp"
#include "functional/utility.hpp"

#include <type_traits>
#include <utility>

namespace fn {

template <typename T>
concept some_choice = detail::_some_choice<T>;

template <> struct choice<>; // Intentionally incomplete

namespace detail {
template <typename T>
static constexpr bool _is_valid_choice_subtype //
    = (not std::is_same_v<void, T>)&&(not std::is_reference_v<T>)&&(not some_sum<T>)&&(not some_in_place_type<T>);
}

template <typename... Ts>
  requires(sizeof...(Ts) > 0)
struct choice<Ts...> : sum<Ts...> {
  static_assert((... && detail::_is_valid_choice_subtype<Ts>));
  static_assert(std::same_as<typename detail::normalized<Ts...>::template apply<::fn::choice>, choice>);
  using _impl = sum<Ts...>;
  using value_type = _impl;

  static constexpr std::size_t size = sizeof...(Ts);
  template <std::size_t I> using select_nth = detail::select_nth_t<I, Ts...>;
  template <typename T> static constexpr bool has_type = _impl::template has_type<T>;

  template <typename T>
  constexpr choice(T &&v)
    requires has_type<std::remove_cvref_t<T>> && (std::is_constructible_v<std::remove_cvref_t<T>, decltype(v)>)
             && (std::is_convertible_v<decltype(v), std::remove_cvref_t<T>>)
      : _impl(std::in_place_type<std::remove_cvref_t<T>>, FWD(v))
  {
  }

  template <typename T>
  constexpr explicit choice(T &&v)
    requires has_type<std::remove_cvref_t<T>> && (std::is_constructible_v<std::remove_cvref_t<T>, decltype(v)>)
             && (not std::is_convertible_v<decltype(v), std::remove_cvref_t<T>>)
      : _impl(std::in_place_type<std::remove_cvref_t<T>>, FWD(v))
  {
  }

  template <typename T>
  constexpr choice(std::in_place_type_t<T> d, auto &&...args) noexcept
    requires has_type<T>
      : _impl(d, FWD(args)...)
  {
  }

  template <typename... Tx>
  constexpr choice(sum<Tx...> const &v) noexcept
    requires detail::is_superset_of<choice, choice<Tx...>> && (... && std::is_copy_constructible_v<Tx>)
      : _impl(std::in_place_type<sum<Tx...>>, FWD(v))
  {
  }

  template <typename... Tx>
  constexpr choice(sum<Tx...> &&v) noexcept
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

  [[nodiscard]] constexpr value_type &value() & noexcept { return *this; }
  [[nodiscard]] constexpr value_type const &value() const & noexcept { return *this; }
  [[nodiscard]] constexpr value_type &&value() && noexcept { return std::move(*this); }
  [[nodiscard]] constexpr value_type const &&value() const && noexcept { return std::move(*this); }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) & noexcept
    requires typelist_invocable<Fn, choice &> && (not typelist_type_invocable<Fn, choice &>)
  {
    using type = detail::_sum_invoke_result<detail::_invoke_autodetect_tag, decltype(fn), choice &>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) & noexcept
    requires typelist_type_invocable<Fn, choice &>
  {
    using type = detail::_sum_invoke_type_result<detail::_invoke_autodetect_tag, decltype(fn), choice &>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) const & noexcept
    requires typelist_invocable<Fn, choice const &> && (not typelist_type_invocable<Fn, choice const &>)
  {
    using type = detail::_sum_invoke_result<detail::_invoke_autodetect_tag, decltype(fn), choice const &>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) const & noexcept
    requires typelist_type_invocable<Fn, choice const &>
  {
    using type = detail::_sum_invoke_type_result<detail::_invoke_autodetect_tag, decltype(fn), choice const &>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) && noexcept
    requires typelist_invocable<Fn, choice &&> && (not typelist_type_invocable<Fn, choice &&>)
  {
    using type = detail::_sum_invoke_result<detail::_invoke_autodetect_tag, decltype(fn), choice &&>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(std::move(_impl::data), _impl::index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) && noexcept
    requires typelist_type_invocable<Fn, choice &&>
  {
    using type = detail::_sum_invoke_type_result<detail::_invoke_autodetect_tag, decltype(fn), choice &&>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(std::move(_impl::data), _impl::index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) const && noexcept
    requires typelist_invocable<Fn, choice const &&> && (not typelist_type_invocable<Fn, choice const &&>)
  {
    using type = detail::_sum_invoke_result<detail::_invoke_autodetect_tag, decltype(fn), choice const &&>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(std::move(_impl::data), _impl::index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) const && noexcept
    requires typelist_type_invocable<Fn, choice const &&>
  {
    using type = detail::_sum_invoke_type_result<detail::_invoke_autodetect_tag, decltype(fn), choice const &&>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(std::move(_impl::data), _impl::index, FWD(fn));
  }

  template <typename T, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn) & noexcept
    requires typelist_invocable<Fn, choice &> && (not typelist_type_invocable<Fn, choice &>)
  {
    using type = detail::_sum_invoke_result<T, decltype(fn), choice &>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
  }

  template <typename T, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn) & noexcept
    requires typelist_type_invocable<Fn, choice &>
  {
    using type = detail::_sum_invoke_type_result<T, decltype(fn), choice &>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
  }

  template <typename T, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn) const & noexcept
    requires typelist_invocable<Fn, choice const &> && (not typelist_type_invocable<Fn, choice const &>)
  {
    using type = detail::_sum_invoke_result<T, decltype(fn), choice const &>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
  }

  template <typename T, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn) const & noexcept
    requires typelist_type_invocable<Fn, choice const &>
  {
    using type = detail::_sum_invoke_type_result<T, decltype(fn), choice const &>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
  }

  template <typename T, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn) && noexcept
    requires typelist_invocable<Fn, choice &&> && (not typelist_type_invocable<Fn, choice &&>)
  {
    using type = detail::_sum_invoke_result<T, decltype(fn), choice &&>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(std::move(_impl::data), _impl::index, FWD(fn));
  }

  template <typename T, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn) && noexcept
    requires typelist_type_invocable<Fn, choice &&>
  {
    using type = detail::_sum_invoke_type_result<T, decltype(fn), choice &&>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(std::move(_impl::data), _impl::index, FWD(fn));
  }

  template <typename T, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn) const && noexcept
    requires typelist_invocable<Fn, choice const &&> && (not typelist_type_invocable<Fn, choice const &&>)
  {
    using type = detail::_sum_invoke_result<T, decltype(fn), choice const &&>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(std::move(_impl::data), _impl::index, FWD(fn));
  }

  template <typename T, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn) const && noexcept
    requires typelist_type_invocable<Fn, choice const &&>
  {
    using type = detail::_sum_invoke_type_result<T, decltype(fn), choice const &&>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(std::move(_impl::data), _impl::index, FWD(fn));
  }

  // NOTE Monadic operations, only `and_then` and `transform` are supported
  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) & noexcept
    requires typelist_invocable<Fn, choice &> && (not typelist_type_invocable<Fn, choice &>)
  {
    using type = detail::_sum_invoke_result<detail::_collapsing_sum_tag, decltype(fn), choice &>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) & noexcept
    requires typelist_type_invocable<Fn, choice &>
  {
    using type = detail::_sum_invoke_type_result<detail::_collapsing_sum_tag, decltype(fn), choice &>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) const & noexcept
    requires typelist_invocable<Fn, choice const &> && (not typelist_type_invocable<Fn, choice const &>)
  {
    using type = detail::_sum_invoke_result<detail::_collapsing_sum_tag, decltype(fn), choice const &>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) const & noexcept
    requires typelist_type_invocable<Fn, choice const &>
  {
    using type = detail::_sum_invoke_type_result<detail::_collapsing_sum_tag, decltype(fn), choice const &>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(_impl::data, _impl::index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) && noexcept
    requires typelist_invocable<Fn, choice &&> && (not typelist_type_invocable<Fn, choice &&>)
  {
    using type = detail::_sum_invoke_result<detail::_collapsing_sum_tag, decltype(fn), choice &&>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(std::move(_impl::data), _impl::index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) && noexcept
    requires typelist_type_invocable<Fn, choice &&>
  {
    using type = detail::_sum_invoke_type_result<detail::_collapsing_sum_tag, decltype(fn), choice &&>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(std::move(_impl::data), _impl::index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) const && noexcept
    requires typelist_invocable<Fn, choice const &&> && (not typelist_type_invocable<Fn, choice const &&>)
  {
    using type = detail::_sum_invoke_result<detail::_collapsing_sum_tag, decltype(fn), choice const &&>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(std::move(_impl::data), _impl::index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto transform(Fn &&fn) const && noexcept
    requires typelist_type_invocable<Fn, choice const &&>
  {
    using type = detail::_sum_invoke_type_result<detail::_collapsing_sum_tag, decltype(fn), choice const &&>::type;
    return detail::invoke_variadic_union<type, typename _impl::data_t>(std::move(_impl::data), _impl::index, FWD(fn));
  }

  template <typename Fn> constexpr auto and_then(Fn &&fn) & noexcept -> decltype(this->invoke(FWD(fn)))
  {
    static_assert(some_choice<decltype(this->invoke(FWD(fn)))>);
    return this->invoke(FWD(fn));
  }

  template <typename Fn> constexpr auto and_then(Fn &&fn) const & noexcept -> decltype(this->invoke(FWD(fn)))
  {
    static_assert(some_choice<decltype(this->invoke(FWD(fn)))>);
    return this->invoke(FWD(fn));
  }

  template <typename Fn> constexpr auto and_then(Fn &&fn) && noexcept -> decltype(std::move(*this).invoke(FWD(fn)))
  {
    static_assert(some_choice<decltype(std::move(*this).invoke(FWD(fn)))>);
    return std::move(*this).invoke(FWD(fn));
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const && noexcept -> decltype(std::move(*this).invoke(FWD(fn)))
  {
    static_assert(some_choice<decltype(std::move(*this).invoke(FWD(fn)))>);
    return std::move(*this).invoke(FWD(fn));
  }
};

// CTAD for single-element choice
template <typename T> explicit choice(std::in_place_type_t<T>, auto &&...) -> choice<T>;
template <typename T> explicit choice(T) -> choice<T>;

template <typename... Ts, typename... Tx>
[[nodiscard]] constexpr bool operator==(choice<Ts...> const &lh, choice<Tx...> const &rh) noexcept
  requires(... && (std::equality_comparable<Ts> || not detail::type_one_of<Ts, Tx...>))
          and (not std::is_same_v<choice<Ts...>, choice<Tx...>>)
{
  return lh.template invoke_r<bool>([&rh]<typename T>(std::in_place_type_t<T> d, auto const &lh) noexcept {
    if constexpr (std::remove_cvref_t<decltype(rh)>::template has_type<T>) {
      return rh.has_value(d) && lh == *rh.get_ptr(d); // GCOVR_EXCL_BR_LINE
    } else {
      return false;
    }
  });
}

template <typename... Ts, typename... Tx>
[[nodiscard]] constexpr bool operator!=(choice<Ts...> const &lh, choice<Tx...> const &rh) noexcept
  requires(... && (std::equality_comparable<Ts> || not detail::type_one_of<Ts, Tx...>))
{
  return not(lh == rh);
}

template <typename... Ts> using choice_for = detail::normalized<Ts...>::template apply<choice>;

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_CHOICE
