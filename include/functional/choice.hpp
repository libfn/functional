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

template <typename... Ts>
  requires(sizeof...(Ts) > 0)
struct choice<Ts...> final {
  template <typename T>
  static constexpr bool _is_valid_subtype //
      = (not std::same_as<void, T>)&&(not some_sum<T>)&&(not std::is_reference_v<T>)&&(not some_in_place_type<T>);
  static_assert((... && _is_valid_subtype<Ts>));
  static_assert(detail::is_normal_v<Ts...>);

  using _type = typename detail::normalized<Ts...>::template apply<sum>;
  static constexpr std::size_t size = _type::size;
  template <typename T> static constexpr bool has_type = (... || std::same_as<Ts, T>);
  _type _v;

  template <typename Fn, typename Self> static constexpr bool invocable = detail::typelist_invocable<Fn, Self>;
  template <typename Fn, typename Self>
  static constexpr bool type_invocable = detail::typelist_type_invocable<Fn, Self>;

  template <typename Fn, typename Self> struct invoke_result;
  template <typename Fn, typename Self>
    requires invocable<Fn, Self>
  struct invoke_result<Fn, Self> final {
    using _T0 = detail::select_nth_t<0, Ts...>;
    using _R0 = std::invoke_result_t<Fn, apply_const_lvalue_t<Self, _T0 &&>>;
    static_assert((... && std::same_as<_R0, std::invoke_result_t<Fn, apply_const_lvalue_t<Self, Ts &&>>>));
    using type = _R0;
  };
  template <typename Fn, typename Self>
    requires type_invocable<Fn, Self>
  struct invoke_result<Fn, Self> final {
    using _T0 = detail::select_nth_t<0, Ts...>;
    using _R0 = std::invoke_result_t<Fn, std::in_place_type_t<_T0>, apply_const_lvalue_t<Self, _T0 &&>>;
    static_assert(
        (...
         && std::same_as<_R0, std::invoke_result_t<Fn, std::in_place_type_t<_T0>, apply_const_lvalue_t<Self, Ts &&>>>));
    using type = _R0;
  };

  template <typename Fn, typename Self> using invoke_result_t = typename invoke_result<Fn, Self>::type;

  template <typename T>
  constexpr choice(T &&v)
    requires(size == 1) && (not std::same_as<std::remove_cvref_t<T>, choice<Ts...>>) && (not some_sum<T>)
            && (not some_in_place_type<T>) && (... || std::is_constructible_v<Ts, T>)
            && (... || std::is_convertible_v<T, Ts>)
      : _v(std::in_place_type<T>, FWD(v))
  {
  }

  template <typename T>
  constexpr explicit choice(T &&v)
    requires(size == 1) && (not std::same_as<std::remove_cvref_t<T>, choice<Ts...>>) && (not some_sum<T>)
            && (not some_in_place_type<T>) && (... || std::is_constructible_v<Ts, T>)
            && (not(... || std::is_convertible_v<T, Ts>))
      : _v(std::in_place_type<T>, FWD(v))
  {
  }

  template <typename T>
  constexpr choice(std::in_place_type_t<T> d, auto &&...args) noexcept
    requires(... || std::is_same_v<T, Ts>)
      : _v(d, FWD(args)...)
  {
  }

  constexpr ~choice() = default;

  template <typename T>
    requires has_type<T>
  [[nodiscard]] constexpr bool has_value() const noexcept
  {
    return this->_v.invoke(
        []<typename U>(std::in_place_type_t<U>, auto &&) constexpr -> bool { return std::same_as<T, U>; });
  }

  template <typename T>
    requires has_type<T>
  [[nodiscard]] constexpr bool has_value(std::in_place_type_t<T>) const noexcept
  {
    return this->_v.invoke(
        []<typename U>(std::in_place_type_t<U>, auto &&) constexpr -> bool { return std::same_as<T, U>; });
  }

  template <typename T>
  [[nodiscard]] constexpr bool operator==(T const &rh) const noexcept
    requires(has_type<T> || has_type<T const>) && std::equality_comparable<T>
  {
    return this->_v == rh;
  }

  template <typename T>
  [[nodiscard]] constexpr bool operator!=(T const &rh) const noexcept
    requires(has_type<T> || has_type<T const>) && std::equality_comparable<T>
  {
    return not(*this == rh);
  }

  [[nodiscard]] constexpr bool operator==(choice<Ts...> const &rh) const noexcept
    requires(... && std::equality_comparable<Ts>)
  {
    return this->_v == rh._v;
  }

  [[nodiscard]] constexpr bool operator!=(choice<Ts...> const &rh) const noexcept
    requires(... && std::equality_comparable<Ts>)
  {
    return not(*this == rh);
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) & noexcept
    requires(invocable<Fn &&, _type &>) || (type_invocable<Fn &&, _type &>)
  {
    return this->_v.invoke(FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) const & noexcept
    requires(invocable<Fn &&, _type const &>) || (type_invocable<Fn &&, _type const &>)
  {
    return this->_v.invoke(FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) && noexcept
    requires(invocable<Fn &&, _type &&>) || (type_invocable<Fn &&, _type &&>)
  {
    return std::move(*this)._v.invoke(FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn) const && noexcept
    requires(invocable<Fn &&, _type const &&>) || (type_invocable<Fn &&, _type const &&>)
  {
    return std::move(*this)._v.invoke(FWD(fn));
  }

  // NOTE Monadic operations, only `and_then` and `transform` are supported
  template <typename Fn> constexpr auto and_then(Fn &&fn) & noexcept -> decltype(this->invoke(FWD(fn)))
  {
    using type = decltype(this->invoke(FWD(fn)));
    static_assert(some_choice<type>);
    return this->invoke(FWD(fn));
  }

  template <typename Fn> constexpr auto and_then(Fn &&fn) const & noexcept -> decltype(this->invoke(FWD(fn)))
  {
    using type = decltype(this->invoke(FWD(fn)));
    static_assert(some_choice<type>);
    return this->invoke(FWD(fn));
  }

  template <typename Fn> constexpr auto and_then(Fn &&fn) && noexcept -> decltype(std::move(*this).invoke(FWD(fn)))
  {
    using type = decltype(std::move(*this).invoke(FWD(fn)));
    static_assert(some_choice<type>);
    return std::move(*this).invoke(FWD(fn));
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const && noexcept -> decltype(std::move(*this).invoke(FWD(fn)))
  {
    using type = decltype(std::move(*this).invoke(FWD(fn)));
    static_assert(some_choice<type>);
    return std::move(*this).invoke(FWD(fn));
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) & noexcept
      -> detail::normalized<Ts..., decltype(this->invoke(FWD(fn)))>::template apply<choice>
  {
    using type = decltype(this->invoke(FWD(fn)));
    static_assert(_is_valid_subtype<type>);
    using result_t = detail::normalized<Ts..., type>::template apply<choice>;
    return result_t{std::in_place_type<type>, this->invoke(FWD(fn))};
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const & noexcept
      -> detail::normalized<Ts..., decltype(this->invoke(FWD(fn)))>::template apply<choice>
  {
    using type = decltype(this->invoke(FWD(fn)));
    static_assert(_is_valid_subtype<type>);
    using result_t = detail::normalized<Ts..., type>::template apply<choice>;
    return result_t{std::in_place_type<type>, this->invoke(FWD(fn))};
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) && noexcept
      -> detail::normalized<Ts..., decltype(std::move(*this).invoke(FWD(fn)))>::template apply<choice>
  {
    using type = decltype(std::move(*this).invoke(FWD(fn)));
    static_assert(_is_valid_subtype<type>);
    using result_t = detail::normalized<Ts..., type>::template apply<choice>;
    return result_t{std::in_place_type<type>, std::move(*this).invoke(FWD(fn))};
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const && noexcept
      -> detail::normalized<Ts..., decltype(std::move(*this).invoke(FWD(fn)))>::template apply<choice>
  {
    using type = decltype(std::move(*this).invoke(FWD(fn)));
    static_assert(_is_valid_subtype<type>);
    using result_t = detail::normalized<Ts..., type>::template apply<choice>;
    return result_t{std::in_place_type<type>, std::move(*this).invoke(FWD(fn))};
  }
};

// CTAD for single-element choice
template <typename T> choice(std::in_place_type_t<T>, auto &&...) -> choice<T>;
template <typename T> choice(T) -> choice<T>;

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_CHOICE
