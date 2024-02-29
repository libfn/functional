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

  // NOTE Monadic operations, only `and_then` and `transform` are supported
  template <typename Fn> constexpr auto and_then(Fn &&fn) & noexcept -> decltype(this->invoke_to(FWD(fn)))
  {
    using type = decltype(this->invoke_to(FWD(fn)));
    static_assert(some_choice<type>);
    return this->invoke_to(FWD(fn));
  }

  template <typename Fn> constexpr auto and_then(Fn &&fn) const & noexcept -> decltype(this->invoke_to(FWD(fn)))
  {
    using type = decltype(this->invoke_to(FWD(fn)));
    static_assert(some_choice<type>);
    return this->invoke_to(FWD(fn));
  }

  template <typename Fn> constexpr auto and_then(Fn &&fn) && noexcept -> decltype(std::move(*this).invoke_to(FWD(fn)))
  {
    using type = decltype(std::move(*this).invoke_to(FWD(fn)));
    static_assert(some_choice<type>);
    return std::move(*this).invoke_to(FWD(fn));
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const && noexcept -> decltype(std::move(*this).invoke_to(FWD(fn)))
  {
    using type = decltype(std::move(*this).invoke_to(FWD(fn)));
    static_assert(some_choice<type>);
    return std::move(*this).invoke_to(FWD(fn));
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) & noexcept
      -> detail::normalized<Ts..., decltype(this->invoke_to(FWD(fn)))>::template apply<choice>
  {
    using type = decltype(this->invoke_to(FWD(fn)));
    static_assert(detail::_is_valid_choice_subtype<type>);
    using result_t = detail::normalized<Ts..., type>::template apply<choice>;
    return result_t{std::in_place_type<type>, this->invoke_to(FWD(fn))};
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const & noexcept
      -> detail::normalized<Ts..., decltype(this->invoke_to(FWD(fn)))>::template apply<choice>
  {
    using type = decltype(this->invoke_to(FWD(fn)));
    static_assert(detail::_is_valid_choice_subtype<type>);
    using result_t = detail::normalized<Ts..., type>::template apply<choice>;
    return result_t{std::in_place_type<type>, this->invoke_to(FWD(fn))};
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) && noexcept
      -> detail::normalized<Ts..., decltype(std::move(*this).invoke_to(FWD(fn)))>::template apply<choice>
  {
    using type = decltype(std::move(*this).invoke_to(FWD(fn)));
    static_assert(detail::_is_valid_choice_subtype<type>);
    using result_t = detail::normalized<Ts..., type>::template apply<choice>;
    return result_t{std::in_place_type<type>, std::move(*this).invoke_to(FWD(fn))};
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const && noexcept
      -> detail::normalized<Ts..., decltype(std::move(*this).invoke_to(FWD(fn)))>::template apply<choice>
  {
    using type = decltype(std::move(*this).invoke_to(FWD(fn)));
    static_assert(detail::_is_valid_choice_subtype<type>);
    using result_t = detail::normalized<Ts..., type>::template apply<choice>;
    return result_t{std::in_place_type<type>, std::move(*this).invoke_to(FWD(fn))};
  }
};

// CTAD for single-element choice
template <typename T> choice(std::in_place_type_t<T>, auto &&...) -> choice<T>;
template <typename T> choice(T) -> choice<T>;

template <typename... Ts> using choice_for = detail::normalized<Ts...>::template apply<choice>;

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_CHOICE
