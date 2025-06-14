// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_OPTIONAL
#define INCLUDE_FN_OPTIONAL

#include <fn/detail/functional.hpp>
#include <fn/pack.hpp>
#include <fn/sum.hpp>

#include <optional>
#include <type_traits>
#include <utility>

namespace fn {

template <typename T>
concept some_optional = detail::_some_optional<T>;

template <typename T> struct optional final : std::optional<T> {
  using value_type = std::optional<T>::value_type;
  static_assert(not std::is_same_v<value_type, ::fn::sum<>>);

  using std::optional<T>::optional;

  auto sum_value() const & -> optional<sum<value_type>>
    requires(not std::is_same_v<value_type, void>) && (not some_sum<value_type>)
  {
    using type = optional<sum<value_type>>;
    if (this->has_value())
      return type{std::in_place, sum<value_type>(this->value())};
    else
      return type{std::nullopt};
  }

  auto sum_value() && -> optional<sum<value_type>>
    requires(not std::is_same_v<value_type, void>) && (not some_sum<value_type>)
  {
    using type = optional<sum<value_type>>;
    if (this->has_value())
      return type{std::in_place, sum<value_type>(std::move(*this).value())};
    else
      return type{std::nullopt};
  }

  auto sum_value() & -> decltype(auto)
    requires(some_sum<value_type>)
  {
    return *this;
  }

  auto sum_value() const & -> decltype(auto)
    requires(some_sum<value_type>)
  {
    return *this;
  }

  auto sum_value() && -> decltype(auto)
    requires(some_sum<value_type>)
  {
    return std::move(*this);
  }

  auto sum_value() const && -> decltype(auto)
    requires(some_sum<value_type>)
  {
    return std::move(*this);
  }

  template <typename Fn> constexpr auto and_then(Fn &&fn) &
  {
    using type = ::fn::detail::_invoke_result<Fn, value_type &>::type;
    static_assert(some_optional<type>);
    if (this->has_value())
      return ::fn::detail::_invoke(FWD(fn), this->value());
    else
      return type(std::nullopt);
  }

  template <typename Fn> constexpr auto and_then(Fn &&fn) const &
  {
    using type = ::fn::detail::_invoke_result<Fn, value_type const &>::type;
    static_assert(some_optional<type>);
    if (this->has_value())
      return ::fn::detail::_invoke(FWD(fn), this->value());
    else
      return type(std::nullopt);
  }

  template <typename Fn> constexpr auto and_then(Fn &&fn) &&
  {
    using type = ::fn::detail::_invoke_result<Fn, value_type &&>::type;
    static_assert(some_optional<type>);
    if (this->has_value())
      return ::fn::detail::_invoke(FWD(fn), std::move(*this).value());
    else
      return type(std::nullopt);
  }

  template <typename Fn> constexpr auto and_then(Fn &&fn) const &&
  {
    using type = ::fn::detail::_invoke_result<Fn, value_type const &&>::type;
    static_assert(some_optional<type>);
    if (this->has_value())
      return ::fn::detail::_invoke(FWD(fn), std::move(*this).value());
    else
      return type(std::nullopt);
  }

  template <typename Fn>
    requires std::is_constructible_v<T, T &>
  constexpr auto or_else(Fn &&fn) &
  {
    using type = ::fn::detail::_invoke_result<Fn>::type;
    static_assert(some_optional<type>);
    static_assert(std::is_same_v<typename type::value_type, value_type> || some_sum<value_type>);
    if constexpr (std::is_same_v<typename type::value_type, value_type>) {
      if (this->has_value())
        return type(std::in_place, this->value());
      else
        return ::fn::detail::_invoke(FWD(fn));
    } else {
      using new_value_type = sum_for<value_type, typename type::value_type>;
      using new_type = ::fn::optional<new_value_type>;
      if (this->has_value())
        return new_type{std::in_place, this->value()};
      else {
        auto t = ::fn::detail::_invoke(FWD(fn));
        if (t.has_value())
          return new_type{std::in_place, std::move(t).value()};
        else
          return new_type{std::nullopt};
      }
    }
  }

  template <typename Fn>
    requires std::is_constructible_v<T, T const &>
  constexpr auto or_else(Fn &&fn) const &
  {
    using type = ::fn::detail::_invoke_result<Fn>::type;
    static_assert(some_optional<type>);
    static_assert(std::is_same_v<typename type::value_type, value_type> || some_sum<value_type>);
    if constexpr (std::is_same_v<typename type::value_type, value_type>) {
      if (this->has_value())
        return type(std::in_place, this->value());
      else
        return ::fn::detail::_invoke(FWD(fn));
    } else {
      using new_value_type = sum_for<value_type, typename type::value_type>;
      using new_type = ::fn::optional<new_value_type>;
      if (this->has_value())
        return new_type{std::in_place, this->value()};
      else {
        auto t = ::fn::detail::_invoke(FWD(fn));
        if (t.has_value())
          return new_type{std::in_place, std::move(t).value()};
        else
          return new_type{std::nullopt};
      }
    }
  }

  template <typename Fn>
    requires std::is_constructible_v<T, T>
  constexpr auto or_else(Fn &&fn) &&
  {
    using type = ::fn::detail::_invoke_result<Fn>::type;
    static_assert(some_optional<type>);
    static_assert(std::is_same_v<typename type::value_type, value_type> || some_sum<value_type>);
    if constexpr (std::is_same_v<typename type::value_type, value_type>) {
      if (this->has_value())
        return type(std::in_place, std::move(*this).value());
      else
        return ::fn::detail::_invoke(FWD(fn));
    } else {
      using new_value_type = sum_for<value_type, typename type::value_type>;
      using new_type = ::fn::optional<new_value_type>;
      if (this->has_value())
        return new_type{std::in_place, std::move(*this).value()};
      else {
        auto t = ::fn::detail::_invoke(FWD(fn));
        if (t.has_value())
          return new_type{std::in_place, std::move(t).value()};
        else
          return new_type{std::nullopt};
      }
    }
  }

  template <typename Fn>
    requires std::is_constructible_v<T, T const>
  constexpr auto or_else(Fn &&fn) const &&
  {
    using type = ::fn::detail::_invoke_result<Fn>::type;
    static_assert(some_optional<type>);
    static_assert(std::is_same_v<typename type::value_type, value_type> || some_sum<value_type>);
    if constexpr (std::is_same_v<typename type::value_type, value_type>) {
      if (this->has_value())
        return type(std::in_place, std::move(*this).value());
      else
        return ::fn::detail::_invoke(FWD(fn));
    } else {
      using new_value_type = sum_for<value_type, typename type::value_type>;
      using new_type = ::fn::optional<new_value_type>;
      if (this->has_value())
        return new_type{std::in_place, std::move(*this).value()};
      else {
        auto t = ::fn::detail::_invoke(FWD(fn));
        if (t.has_value())
          return new_type{std::in_place, std::move(t).value()};
        else
          return new_type{std::nullopt};
      }
    }
  }

  // transform not sum
  template <typename Fn>
  constexpr auto transform(Fn &&fn) &
    requires(not some_sum<value_type>)
  {
    using new_value_type = ::fn::detail::_invoke_result<Fn, value_type &>::type;
    using type = optional<new_value_type>;
    if (this->has_value())
      return type(std::in_place, ::fn::detail::_invoke(FWD(fn), this->value()));
    else
      return type(std::nullopt);
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const &
    requires(not some_sum<value_type>)
  {
    using new_value_type = ::fn::detail::_invoke_result<Fn, value_type const &>::type;
    using type = optional<new_value_type>;
    if (this->has_value())
      return type(std::in_place, ::fn::detail::_invoke(FWD(fn), this->value()));
    else
      return type(std::nullopt);
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) &&
    requires(not some_sum<value_type>)
  {
    using new_value_type = ::fn::detail::_invoke_result<Fn, value_type &&>::type;
    using type = optional<new_value_type>;
    if (this->has_value())
      return type(std::in_place, ::fn::detail::_invoke(FWD(fn), std::move(*this).value()));
    else
      return type(std::nullopt);
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const &&
    requires(not some_sum<value_type>)
  {
    using new_value_type = ::fn::detail::_invoke_result<Fn, value_type const &&>::type;
    using type = optional<new_value_type>;
    if (this->has_value())
      return type(std::in_place, ::fn::detail::_invoke(FWD(fn), std::move(*this).value()));
    else
      return type(std::nullopt);
  }

  // transform sum
  template <typename Fn>
  constexpr auto transform(Fn &&fn) &
    requires some_sum<value_type>
  {
    using new_value_type = decltype(this->value().transform(FWD(fn)));
    using type = optional<new_value_type>;
    if (this->has_value())
      return type(std::in_place, this->value().transform(FWD(fn)));
    else
      return type(std::nullopt);
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const &
    requires some_sum<value_type>
  {
    using new_value_type = decltype(this->value().transform(FWD(fn)));
    using type = optional<new_value_type>;
    if (this->has_value())
      return type(std::in_place, this->value().transform(FWD(fn)));
    else
      return type(std::nullopt);
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) &&
    requires some_sum<value_type>
  {
    using new_value_type = decltype(std::move(*this).value().transform(FWD(fn)));
    using type = optional<new_value_type>;
    if (this->has_value())
      return type(std::in_place, std::move(*this).value().transform(FWD(fn)));
    else
      return type(std::nullopt);
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const &&
    requires some_sum<value_type>
  {
    using new_value_type = decltype(std::move(*this).value().transform(FWD(fn)));
    using type = optional<new_value_type>;
    if (this->has_value())
      return type(std::in_place, std::move(*this).value().transform(FWD(fn)));
    else
      return type(std::nullopt);
  }
};

template <class T> optional(T) -> optional<T>;

// Lifts for sum transformation functions
[[nodiscard]] constexpr auto sum_value(some_optional auto &&src) -> decltype(auto) { return FWD(src).sum_value(); }

template <some_optional Lh, some_optional Rh> [[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  static constexpr auto efn = [](auto const &) { return std::nullopt; };
  return ::fn::detail::_join<fn::optional>(FWD(lh), FWD(rh), efn);
}

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_OPTIONAL
