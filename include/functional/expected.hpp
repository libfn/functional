// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_EXPECTED
#define INCLUDE_FUNCTIONAL_EXPECTED

#include "functional/detail/meta.hpp"
#include "functional/functional.hpp"
#include "functional/fwd.hpp"
#include "functional/pack.hpp"
#include "functional/sum.hpp"
#include "functional/utility.hpp"

#include <concepts>
#include <expected>
#include <functional>
#include <type_traits>
#include <utility>

namespace fn {

template <typename T>
concept some_expected = detail::_some_expected<T>;

template <typename T>
concept some_expected_non_void = //
    some_expected<T>             //
    && !std::is_same_v<void, typename std::remove_cvref_t<T>::value_type>;

template <typename T>
concept some_expected_void = //
    some_expected<T>         //
    && std::is_same_v<void, typename std::remove_cvref_t<T>::value_type>;

template <typename T, typename Err> struct expected final : std::expected<T, Err> {
  using value_type = std::expected<T, Err>::value_type;
  using error_type = std::expected<T, Err>::error_type;
  using unexpected_type = std::unexpected<Err>;

  using std::expected<T, Err>::expected;

  // convert to graded monad
  auto sum_error() const & -> expected<value_type, sum<error_type>>
    requires(not some_sum<error_type>)
  {
    using type = expected<value_type, sum<error_type>>;
    if (this->has_value())
      return type{std::in_place, this->value()};
    else
      return type{std::unexpect, sum<error_type>(this->error())};
  }

  auto sum_error() && -> expected<value_type, sum<error_type>>
    requires(not some_sum<error_type>)
  {
    using type = expected<value_type, sum<error_type>>;
    if (this->has_value())
      return type{std::in_place, std::move(*this).value()};
    else
      return type{std::unexpect, sum<error_type>(std::move(*this).error())};
  }

  auto sum_error() & -> decltype(auto)
    requires(some_sum<error_type>)
  {
    return *this;
  }

  auto sum_error() const & -> decltype(auto)
    requires(some_sum<error_type>)
  {
    return *this;
  }

  auto sum_error() && -> decltype(auto)
    requires(some_sum<error_type>)
  {
    return std::move(*this);
  }

  auto sum_error() const && -> decltype(auto)
    requires(some_sum<error_type>)
  {
    return std::move(*this);
  }

  auto sum_value() const & -> expected<sum<value_type>, error_type>
    requires(not std::is_same_v<value_type, void>) && (not some_sum<value_type>)
  {
    using type = expected<sum<value_type>, error_type>;
    if (this->has_value())
      return type{std::in_place, sum<value_type>(this->value())};
    else
      return type{std::unexpect, this->error()};
  }

  auto sum_value() && -> expected<sum<value_type>, error_type>
    requires(not std::is_same_v<value_type, void>) && (not some_sum<value_type>)
  {
    using type = expected<sum<value_type>, error_type>;
    if (this->has_value())
      return type{std::in_place, sum<value_type>(std::move(*this).value())};
    else
      return type{std::unexpect, std::move(*this).error()};
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

  // and_then non void
  template <typename Fn>
  constexpr auto and_then(Fn &&fn) &
    requires std::is_constructible_v<Err, Err &> && (not std::is_same_v<value_type, void>)
  {
    using type = ::fn::detail::_invoke_result<Fn, value_type &>::type;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type> || some_sum<error_type>);
    if constexpr (std::is_same_v<typename type::error_type, error_type>) {
      if (this->has_value())
        return ::fn::detail::_invoke(FWD(fn), this->value());
      else
        return type(std::unexpect, this->error());
    } else {
      using new_error_type = ::fn::detail::_collapsing_sum::normalized<
          ::fn::sum, ::fn::detail::_collapsing_sum::flattened<error_type, typename type::error_type>>::type;
      using new_type = ::fn::expected<typename type::value_type, new_error_type>;
      if (this->has_value()) {
        auto t = ::fn::detail::_invoke(FWD(fn), this->value());
        if (t.has_value())
          if constexpr (not std::is_same_v<typename new_type::value_type, void>)
            return new_type{std::in_place, std::move(t).value()};
          else
            return new_type{std::in_place};
        else
          return new_type{std::unexpect, std::move(t).error()};
      } else
        return new_type(std::unexpect, this->error());
    }
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const &
    requires std::is_constructible_v<Err, Err const &> && (not std::is_same_v<value_type, void>)
  {
    using type = ::fn::detail::_invoke_result<Fn, value_type const &>::type;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type> || some_sum<error_type>);
    if constexpr (std::is_same_v<typename type::error_type, error_type>) {
      if (this->has_value())
        return ::fn::detail::_invoke(FWD(fn), this->value());
      else
        return type(std::unexpect, this->error());
    } else {
      using new_error_type = ::fn::detail::_collapsing_sum::normalized<
          ::fn::sum, ::fn::detail::_collapsing_sum::flattened<error_type, typename type::error_type>>::type;
      using new_type = ::fn::expected<typename type::value_type, new_error_type>;
      if (this->has_value()) {
        auto t = ::fn::detail::_invoke(FWD(fn), this->value());
        if (t.has_value())
          if constexpr (not std::is_same_v<typename new_type::value_type, void>)
            return new_type{std::in_place, std::move(t).value()};
          else
            return new_type{std::in_place};
        else
          return new_type{std::unexpect, std::move(t).error()};
      } else
        return new_type(std::unexpect, this->error());
    }
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) &&
    requires std::is_constructible_v<Err, Err> && (not std::is_same_v<value_type, void>)
  {
    using type = ::fn::detail::_invoke_result<Fn, value_type &&>::type;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type> || some_sum<error_type>);
    if constexpr (std::is_same_v<typename type::error_type, error_type>) {
      if (this->has_value())
        return ::fn::detail::_invoke(FWD(fn), std::move(*this).value());
      else
        return type(std::unexpect, std::move(*this).error());
    } else {
      using new_error_type = ::fn::detail::_collapsing_sum::normalized<
          ::fn::sum, ::fn::detail::_collapsing_sum::flattened<error_type, typename type::error_type>>::type;
      using new_type = ::fn::expected<typename type::value_type, new_error_type>;
      if (this->has_value()) {
        auto t = ::fn::detail::_invoke(FWD(fn), std::move(*this).value());
        if (t.has_value())
          if constexpr (not std::is_same_v<typename new_type::value_type, void>)
            return new_type{std::in_place, std::move(t).value()};
          else
            return new_type{std::in_place};
        else
          return new_type{std::unexpect, std::move(t).error()};
      } else
        return new_type(std::unexpect, std::move(*this).error());
    }
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const &&
    requires std::is_constructible_v<Err, Err const> && (not std::is_same_v<value_type, void>)
  {
    using type = ::fn::detail::_invoke_result<Fn, value_type const &&>::type;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type> || some_sum<error_type>);
    if constexpr (std::is_same_v<typename type::error_type, error_type>) {
      if (this->has_value())
        return ::fn::detail::_invoke(FWD(fn), std::move(*this).value());
      else
        return type(std::unexpect, std::move(*this).error());
    } else {
      using new_error_type = ::fn::detail::_collapsing_sum::normalized<
          ::fn::sum, ::fn::detail::_collapsing_sum::flattened<error_type, typename type::error_type>>::type;
      using new_type = ::fn::expected<typename type::value_type, new_error_type>;
      if (this->has_value()) {
        auto t = ::fn::detail::_invoke(FWD(fn), std::move(*this).value());
        if (t.has_value())
          if constexpr (not std::is_same_v<typename new_type::value_type, void>)
            return new_type{std::in_place, std::move(t).value()};
          else
            return new_type{std::in_place};
        else
          return new_type{std::unexpect, std::move(t).error()};
      } else
        return new_type(std::unexpect, std::move(*this).error());
    }
  }

  // and_then void
  template <typename Fn>
  constexpr auto and_then(Fn &&fn) &
    requires std::is_constructible_v<Err, Err &> && std::is_same_v<value_type, void>
  {
    using type = ::fn::detail::_invoke_result<Fn>::type;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type> || some_sum<error_type>);
    if constexpr (std::is_same_v<typename type::error_type, error_type>) {
      if (this->has_value())
        return ::fn::detail::_invoke(FWD(fn));
      else
        return type(std::unexpect, this->error());
    } else {
      using new_error_type = ::fn::detail::_collapsing_sum::normalized<
          ::fn::sum, ::fn::detail::_collapsing_sum::flattened<error_type, typename type::error_type>>::type;
      using new_type = ::fn::expected<typename type::value_type, new_error_type>;
      if (this->has_value()) {
        auto t = ::fn::detail::_invoke(FWD(fn));
        if (t.has_value())
          if constexpr (not std::is_same_v<typename new_type::value_type, void>)
            return new_type{std::in_place, std::move(t).value()};
          else
            return new_type{std::in_place};
        else
          return new_type{std::unexpect, std::move(t).error()};
      } else
        return new_type(std::unexpect, this->error());
    }
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const &
    requires std::is_constructible_v<Err, Err const &> && std::is_same_v<value_type, void>
  {
    using type = ::fn::detail::_invoke_result<Fn>::type;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type> || some_sum<error_type>);
    if constexpr (std::is_same_v<typename type::error_type, error_type>) {
      if (this->has_value())
        return ::fn::detail::_invoke(FWD(fn));
      else
        return type(std::unexpect, this->error());
    } else {
      using new_error_type = ::fn::detail::_collapsing_sum::normalized<
          ::fn::sum, ::fn::detail::_collapsing_sum::flattened<error_type, typename type::error_type>>::type;
      using new_type = ::fn::expected<typename type::value_type, new_error_type>;
      if (this->has_value()) {
        auto t = ::fn::detail::_invoke(FWD(fn));
        if (t.has_value())
          if constexpr (not std::is_same_v<typename new_type::value_type, void>)
            return new_type{std::in_place, std::move(t).value()};
          else
            return new_type{std::in_place};
        else
          return new_type{std::unexpect, std::move(t).error()};
      } else
        return new_type(std::unexpect, this->error());
    }
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) &&
    requires std::is_constructible_v<Err, Err> && std::is_same_v<value_type, void>
  {
    using type = ::fn::detail::_invoke_result<Fn>::type;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type> || some_sum<error_type>);
    if constexpr (std::is_same_v<typename type::error_type, error_type>) {
      if (this->has_value())
        return ::fn::detail::_invoke(FWD(fn));
      else
        return type(std::unexpect, std::move(*this).error());
    } else {
      using new_error_type = ::fn::detail::_collapsing_sum::normalized<
          ::fn::sum, ::fn::detail::_collapsing_sum::flattened<error_type, typename type::error_type>>::type;
      using new_type = ::fn::expected<typename type::value_type, new_error_type>;
      if (this->has_value()) {
        auto t = ::fn::detail::_invoke(FWD(fn));
        if (t.has_value())
          if constexpr (not std::is_same_v<typename new_type::value_type, void>)
            return new_type{std::in_place, std::move(t).value()};
          else
            return new_type{std::in_place};
        else
          return new_type{std::unexpect, std::move(t).error()};
      } else
        return new_type(std::unexpect, std::move(*this).error());
    }
  }

  template <typename Fn>
  constexpr auto and_then(Fn &&fn) const &&
    requires std::is_constructible_v<Err, Err const> && std::is_same_v<value_type, void>
  {
    using type = ::fn::detail::_invoke_result<Fn>::type;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::error_type, error_type> || some_sum<error_type>);
    if constexpr (std::is_same_v<typename type::error_type, error_type>) {
      if (this->has_value())
        return ::fn::detail::_invoke(FWD(fn));
      else
        return type(std::unexpect, std::move(*this).error());
    } else {
      using new_error_type = ::fn::detail::_collapsing_sum::normalized<
          ::fn::sum, ::fn::detail::_collapsing_sum::flattened<error_type, typename type::error_type>>::type;
      using new_type = ::fn::expected<typename type::value_type, new_error_type>;
      if (this->has_value()) {
        auto t = ::fn::detail::_invoke(FWD(fn));
        if (t.has_value())
          if constexpr (not std::is_same_v<typename new_type::value_type, void>)
            return new_type{std::in_place, std::move(t).value()};
          else
            return new_type{std::in_place};
        else
          return new_type{std::unexpect, std::move(t).error()};
      } else
        return new_type(std::unexpect, std::move(*this).error());
    }
  }

  // or_else
  template <typename Fn>
  constexpr auto or_else(Fn &&fn) &
    requires (std::is_constructible_v<T, T &> || std::same_as<T, void>)
  {
    using type = ::fn::detail::_invoke_result<Fn, error_type &>::type;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::value_type, value_type> || some_sum<value_type>);
    if constexpr (std::is_same_v<typename type::value_type, value_type>) {
      if (this->has_value())
        if constexpr (not std::is_same_v<value_type, void>)
          return type(std::in_place, this->value());
        else
          return type();
      else
        return ::fn::detail::_invoke(FWD(fn), this->error());
    } else {
      static_assert(not std::is_same_v<typename type::value_type, void>);
      using new_value_type = ::fn::detail::_collapsing_sum::normalized<
          ::fn::sum, ::fn::detail::_collapsing_sum::flattened<value_type, typename type::value_type>>::type;
      using new_type = ::fn::expected<new_value_type, typename type::error_type>;
      if (this->has_value())
        return new_type{std::in_place, this->value()};
      else {
        auto t = ::fn::detail::_invoke(FWD(fn), this->error());
        if (t.has_value())
          return new_type{std::in_place, std::move(t).value()};
        else
          return new_type{std::unexpect, std::move(t).error()};
      }
    }
  }

  template <typename Fn>
  constexpr auto or_else(Fn &&fn) const &
    requires(std::is_constructible_v<T, T const &> || std::same_as<T, void>)
  {
    using type = ::fn::detail::_invoke_result<Fn, error_type const &>::type;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::value_type, value_type> || some_sum<value_type>);
    if constexpr (std::is_same_v<typename type::value_type, value_type>) {
      if (this->has_value())
        if constexpr (not std::is_same_v<value_type, void>)
          return type(std::in_place, this->value());
        else
          return type();
      else
        return ::fn::detail::_invoke(FWD(fn), this->error());
    } else {
      static_assert(not std::is_same_v<typename type::value_type, void>);
      using new_value_type = ::fn::detail::_collapsing_sum::normalized<
          ::fn::sum, ::fn::detail::_collapsing_sum::flattened<value_type, typename type::value_type>>::type;
      using new_type = ::fn::expected<new_value_type, typename type::error_type>;
      if (this->has_value())
        return new_type{std::in_place, this->value()};
      else {
        auto t = ::fn::detail::_invoke(FWD(fn), this->error());
        if (t.has_value())
          return new_type{std::in_place, std::move(t).value()};
        else
          return new_type{std::unexpect, std::move(t).error()};
      }
    }
  }

  template <typename Fn>
  constexpr auto or_else(Fn &&fn) &&
    requires(std::is_constructible_v<T, T> || std::same_as<T, void>)
  {
    using type = ::fn::detail::_invoke_result<Fn, error_type &&>::type;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::value_type, value_type> || some_sum<value_type>);
    if constexpr (std::is_same_v<typename type::value_type, value_type>) {
      if (this->has_value())
        if constexpr (not std::is_same_v<value_type, void>)
          return type(std::in_place, std::move(*this).value());
        else
          return type();
      else
        return ::fn::detail::_invoke(FWD(fn), std::move(*this).error());
    } else {
      static_assert(not std::is_same_v<typename type::value_type, void>);
      using new_value_type = ::fn::detail::_collapsing_sum::normalized<
          ::fn::sum, ::fn::detail::_collapsing_sum::flattened<value_type, typename type::value_type>>::type;
      using new_type = ::fn::expected<new_value_type, typename type::error_type>;
      if (this->has_value())
        return new_type{std::in_place, std::move(*this).value()};
      else {
        auto t = ::fn::detail::_invoke(FWD(fn), std::move(*this).error());
        if (t.has_value())
          return new_type{std::in_place, std::move(t).value()};
        else
          return new_type{std::unexpect, std::move(t).error()};
      }
    }
  }

  template <typename Fn>
  constexpr auto or_else(Fn &&fn) const &&
    requires(std::is_constructible_v<T, T const> || std::same_as<T, void>)
  {
    using type = ::fn::detail::_invoke_result<Fn, error_type const &&>::type;
    static_assert(some_expected<type>);
    static_assert(std::is_same_v<typename type::value_type, value_type> || some_sum<value_type>);
    if constexpr (std::is_same_v<typename type::value_type, value_type>) {
      if (this->has_value())
        if constexpr (not std::is_same_v<value_type, void>)
          return type(std::in_place, std::move(*this).value());
        else
          return type();
      else
        return ::fn::detail::_invoke(FWD(fn), std::move(*this).error());
    } else {
      static_assert(not std::is_same_v<typename type::value_type, void>);
      using new_value_type = ::fn::detail::_collapsing_sum::normalized<
          ::fn::sum, ::fn::detail::_collapsing_sum::flattened<value_type, typename type::value_type>>::type;
      using new_type = ::fn::expected<new_value_type, typename type::error_type>;
      if (this->has_value())
        return new_type{std::in_place, std::move(*this).value()};
      else {
        auto t = ::fn::detail::_invoke(FWD(fn), std::move(*this).error());
        if (t.has_value())
          return new_type{std::in_place, std::move(t).value()};
        else
          return new_type{std::unexpect, std::move(t).error()};
      }
    }
  }

  // transform not void, not sum
  template <typename Fn>
          constexpr auto transform(Fn &&fn) & requires(not std::is_same_v<value_type, void>)
      && (not some_sum<value_type>)
  {
    using new_value_type = detail::_invoke_result<Fn, value_type &>::type;
    using type = expected<new_value_type, error_type>;
    if (this->has_value())
      if constexpr (std::is_same_v<new_value_type, void>) {
        ::fn::detail::_invoke(FWD(fn), this->value());
        return type();
      } else
        return type(std::in_place, ::fn::detail::_invoke(FWD(fn), this->value()));
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const &
    requires(not std::is_same_v<value_type, void>) && (not some_sum<value_type>)
  {
    using new_value_type = detail::_invoke_result<Fn, value_type const &>::type;
    using type = expected<new_value_type, error_type>;
    if (this->has_value())
      if constexpr (std::is_same_v<new_value_type, void>) {
        ::fn::detail::_invoke(FWD(fn), this->value());
        return type();
      } else
        return type(std::in_place, ::fn::detail::_invoke(FWD(fn), this->value()));
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn>
      constexpr auto transform(Fn &&fn) && requires(not std::is_same_v<value_type, void>) && (not some_sum<value_type>)
  {
    using new_value_type = detail::_invoke_result<Fn, value_type &&>::type;
    using type = expected<new_value_type, error_type>;
    if (this->has_value())
      if constexpr (std::is_same_v<new_value_type, void>) {
        ::fn::detail::_invoke(FWD(fn), std::move(*this).value());
        return type();
      } else
        return type(std::in_place, ::fn::detail::_invoke(FWD(fn), std::move(*this).value()));
    else
      return type(std::unexpect, std::move(*this).error());
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const &&
    requires(not std::is_same_v<value_type, void>) && (not some_sum<value_type>)
  {
    using new_value_type = detail::_invoke_result<Fn, value_type const &&>::type;
    using type = expected<new_value_type, error_type>;
    if (this->has_value())
      if constexpr (std::is_same_v<new_value_type, void>) {
        ::fn::detail::_invoke(FWD(fn), std::move(*this).value());
        return type();
      } else
        return type(std::in_place, ::fn::detail::_invoke(FWD(fn), std::move(*this).value()));
    else
      return type(std::unexpect, std::move(*this).error());
  }

  // transform sum
  template <typename Fn>
  constexpr auto transform(Fn &&fn) &
    requires some_sum<value_type>
  {
    using new_value_type = decltype(this->value().transform(FWD(fn)));
    using type = expected<new_value_type, error_type>;
    if (this->has_value())
      if constexpr (std::is_same_v<new_value_type, void>) {
        this->value().transform(FWD(fn));
        return type();
      } else
        return type(std::in_place, this->value().transform(FWD(fn)));
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const &
    requires some_sum<value_type>
  {
    using new_value_type = decltype(this->value().transform(FWD(fn)));
    using type = expected<new_value_type, error_type>;
    if (this->has_value())
      if constexpr (std::is_same_v<new_value_type, void>) {
        this->value().transform(FWD(fn));
        return type();
      } else
        return type(std::in_place, this->value().transform(FWD(fn)));
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) &&
    requires some_sum<value_type>
  {
    using new_value_type = decltype(std::move(*this).value().transform(FWD(fn)));
    using type = expected<new_value_type, error_type>;
    if (this->has_value())
      if constexpr (std::is_same_v<new_value_type, void>) {
        std::move(*this).value().transform(FWD(fn));
        return type();
      } else
        return type(std::in_place, std::move(*this).value().transform(FWD(fn)));
    else
      return type(std::unexpect, std::move(*this).error());
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const &&
    requires some_sum<value_type>
  {
    using new_value_type = decltype(std::move(*this).value().transform(FWD(fn)));
    using type = expected<new_value_type, error_type>;
    if (this->has_value())
      if constexpr (std::is_same_v<new_value_type, void>) {
        std::move(*this).value().transform(FWD(fn));
        return type();
      } else
        return type(std::in_place, std::move(*this).value().transform(FWD(fn)));
    else
      return type(std::unexpect, std::move(*this).error());
  }

  // transform void
  template <typename Fn>
  constexpr auto transform(Fn &&fn) &
    requires std::is_same_v<value_type, void>
  {
    using new_value_type = detail::_invoke_result<Fn>::type;
    using type = expected<new_value_type, error_type>;
    if (this->has_value())
      if constexpr (std::is_same_v<new_value_type, void>) {
        ::fn::detail::_invoke(FWD(fn));
        return type();
      } else
        return type(std::in_place, ::fn::detail::_invoke(FWD(fn)));
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const &
    requires std::is_same_v<value_type, void>
  {
    using new_value_type = detail::_invoke_result<Fn>::type;
    using type = expected<new_value_type, error_type>;
    if (this->has_value())
      if constexpr (std::is_same_v<new_value_type, void>) {
        ::fn::detail::_invoke(FWD(fn));
        return type();
      } else
        return type(std::in_place, ::fn::detail::_invoke(FWD(fn)));
    else
      return type(std::unexpect, this->error());
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) &&
    requires std::is_same_v<value_type, void>
  {
    using new_value_type = detail::_invoke_result<Fn>::type;
    using type = expected<new_value_type, error_type>;
    if (this->has_value())
      if constexpr (std::is_same_v<new_value_type, void>) {
        ::fn::detail::_invoke(FWD(fn));
        return type();
      } else
        return type(std::in_place, ::fn::detail::_invoke(FWD(fn)));
    else
      return type(std::unexpect, std::move(*this).error());
  }

  template <typename Fn>
  constexpr auto transform(Fn &&fn) const &&
    requires std::is_same_v<value_type, void>
  {
    using new_value_type = detail::_invoke_result<Fn>::type;
    using type = expected<new_value_type, error_type>;
    if (this->has_value())
      if constexpr (std::is_same_v<new_value_type, void>) {
        ::fn::detail::_invoke(FWD(fn));
        return type();
      } else
        return type(std::in_place, ::fn::detail::_invoke(FWD(fn)));
    else
      return type(std::unexpect, std::move(*this).error());
  }

  // transform_error not sum
  template <typename Fn>
  constexpr auto transform_error(Fn &&fn) &
    requires(not some_sum<error_type>)
  {
    using new_error_type = detail::_invoke_result<Fn, error_type &>::type;
    using type = expected<value_type, new_error_type>;
    if (this->has_value())
      if constexpr (not std::is_same_v<value_type, void>)
        return type(std::in_place, this->value());
      else
        return type();
    else
      return type(std::unexpect, ::fn::detail::_invoke(FWD(fn), this->error()));
  }

  template <typename Fn>
  constexpr auto transform_error(Fn &&fn) const &
    requires(not some_sum<error_type>)
  {
    using new_error_type = detail::_invoke_result<Fn, error_type const &>::type;
    using type = expected<value_type, new_error_type>;
    if (this->has_value())
      if constexpr (not std::is_same_v<value_type, void>)
        return type(std::in_place, this->value());
      else
        return type();
    else
      return type(std::unexpect, ::fn::detail::_invoke(FWD(fn), this->error()));
  }

  template <typename Fn>
  constexpr auto transform_error(Fn &&fn) &&
    requires(not some_sum<error_type>)
  {
    using new_error_type = detail::_invoke_result<Fn, error_type &&>::type;
    using type = expected<value_type, new_error_type>;
    if (this->has_value())
      if constexpr (not std::is_same_v<value_type, void>)
        return type(std::in_place, std::move(*this).value());
      else
        return type();
    else
      return type(std::unexpect, ::fn::detail::_invoke(FWD(fn), std::move(*this).error()));
  }

  template <typename Fn>
  constexpr auto transform_error(Fn &&fn) const &&
    requires(not some_sum<error_type>)
  {
    using new_error_type = detail::_invoke_result<Fn, error_type const &&>::type;
    using type = expected<value_type, new_error_type>;
    if (this->has_value())
      if constexpr (not std::is_same_v<value_type, void>)
        return type(std::in_place, std::move(*this).value());
      else
        return type();
    else
      return type(std::unexpect, ::fn::detail::_invoke(FWD(fn), std::move(*this).error()));
  }

  // transform_error sum
  template <typename Fn>
  constexpr auto transform_error(Fn &&fn) &
    requires some_sum<error_type>
  {
    using new_error_type = decltype(this->error().transform(FWD(fn)));
    using type = expected<value_type, new_error_type>;
    if (this->has_value())
      if constexpr (not std::is_same_v<value_type, void>)
        return type(std::in_place, this->value());
      else
        return type();
    else
      return type(std::unexpect, this->error().transform(FWD(fn)));
  }

  template <typename Fn>
  constexpr auto transform_error(Fn &&fn) const &
    requires some_sum<error_type>
  {
    using new_error_type = decltype(this->error().transform(FWD(fn)));
    using type = expected<value_type, new_error_type>;
    if (this->has_value())
      if constexpr (not std::is_same_v<value_type, void>)
        return type(std::in_place, this->value());
      else
        return type();
    else
      return type(std::unexpect, this->error().transform(FWD(fn)));
  }

  template <typename Fn>
  constexpr auto transform_error(Fn &&fn) &&
    requires some_sum<error_type>
  {
    using new_error_type = decltype(std::move(*this).error().transform(FWD(fn)));
    using type = expected<value_type, new_error_type>;
    if (this->has_value())
      if constexpr (not std::is_same_v<value_type, void>)
        return type(std::in_place, std::move(*this).value());
      else
        return type();
    else
      return type(std::unexpect, std::move(*this).error().transform(FWD(fn)));
  }

  template <typename Fn>
  constexpr auto transform_error(Fn &&fn) const &&
    requires some_sum<error_type>
  {
    using new_error_type = decltype(std::move(*this).error().transform(FWD(fn)));
    using type = expected<value_type, new_error_type>;
    if (this->has_value())
      if constexpr (not std::is_same_v<value_type, void>)
        return type(std::in_place, std::move(*this).value());
      else
        return type();
    else
      return type(std::unexpect, std::move(*this).error().transform(FWD(fn)));
  }
};

// When any of the sides is expected<void, ...>, we do not produce expected<pack<...>, ...>
// Instead just elide void and carry non-void (or elide both voids if that's what we get)
template <some_expected_non_void Lh, some_expected_void Rh>
  requires std::is_same_v<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using value_type = std::remove_cvref_t<Lh>::value_type;
  using error_type = std::remove_cvref_t<Lh>::error_type;
  using type = expected<value_type, error_type>;
  if (lh.has_value() && rh.has_value())
    return type{std::in_place, FWD(lh).value()};
  else if (not lh.has_value())
    return type{std::unexpect, FWD(lh).error()};
  else
    return type{std::unexpect, FWD(rh).error()};
}

template <some_expected_non_void Lh, some_expected_void Rh>
  requires(not std::is_same_v<typename std::remove_cvref_t<Lh>::error_type,
                              typename std::remove_cvref_t<Rh>::error_type>)
          && (some_sum<typename std::remove_cvref_t<Lh>::error_type>
              || some_sum<typename std::remove_cvref_t<Rh>::error_type>)
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using value_type = std::remove_cvref_t<Lh>::value_type;
  using new_error_type = ::fn::detail::_collapsing_sum::normalized<
      ::fn::sum, ::fn::detail::_collapsing_sum::flattened<typename std::remove_cvref_t<Lh>::error_type,
                                                          typename std::remove_cvref_t<Rh>::error_type>>::type;
  using type = expected<value_type, new_error_type>;
  if (lh.has_value() && rh.has_value())
    return type{std::in_place, FWD(lh).value()};
  else if (not lh.has_value())
    return type{std::unexpect, new_error_type{FWD(lh).error()}};
  else
    return type{std::unexpect, new_error_type{FWD(rh).error()}};
}

template <some_expected_void Lh, some_expected_non_void Rh>
  requires std::is_same_v<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using value_type = std::remove_cvref_t<Rh>::value_type;
  using error_type = std::remove_cvref_t<Rh>::error_type;
  using type = expected<value_type, error_type>;
  if (lh.has_value() && rh.has_value())
    return type{std::in_place, FWD(rh).value()};
  else if (not lh.has_value())
    return type{std::unexpect, FWD(lh).error()};
  else
    return type{std::unexpect, FWD(rh).error()};
}

template <some_expected_void Lh, some_expected_non_void Rh>
  requires(not std::is_same_v<typename std::remove_cvref_t<Lh>::error_type,
                              typename std::remove_cvref_t<Rh>::error_type>)
          && (some_sum<typename std::remove_cvref_t<Lh>::error_type>
              || some_sum<typename std::remove_cvref_t<Rh>::error_type>)
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using value_type = std::remove_cvref_t<Rh>::value_type;
  using new_error_type = ::fn::detail::_collapsing_sum::normalized<
      ::fn::sum, ::fn::detail::_collapsing_sum::flattened<typename std::remove_cvref_t<Lh>::error_type,
                                                          typename std::remove_cvref_t<Rh>::error_type>>::type;
  using type = expected<value_type, new_error_type>;
  if (lh.has_value() && rh.has_value())
    return type{std::in_place, FWD(rh).value()};
  else if (not lh.has_value())
    return type{std::unexpect, new_error_type{FWD(lh).error()}};
  else
    return type{std::unexpect, new_error_type{FWD(rh).error()}};
}

template <some_expected_void Lh, some_expected_void Rh>
  requires std::is_same_v<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using error_type = std::remove_cvref_t<Rh>::error_type;
  using type = expected<void, error_type>;
  if (lh.has_value() && rh.has_value())
    return type();
  else if (not lh.has_value())
    return type{std::unexpect, FWD(lh).error()};
  else
    return type{std::unexpect, FWD(rh).error()};
}

template <some_expected_void Lh, some_expected_void Rh>
  requires(not std::is_same_v<typename std::remove_cvref_t<Lh>::error_type,
                              typename std::remove_cvref_t<Rh>::error_type>)
          && (some_sum<typename std::remove_cvref_t<Lh>::error_type>
              || some_sum<typename std::remove_cvref_t<Rh>::error_type>)
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using new_error_type = ::fn::detail::_collapsing_sum::normalized<
      ::fn::sum, ::fn::detail::_collapsing_sum::flattened<typename std::remove_cvref_t<Lh>::error_type,
                                                          typename std::remove_cvref_t<Rh>::error_type>>::type;
  using type = expected<void, new_error_type>;
  if (lh.has_value() && rh.has_value())
    return type();
  else if (not lh.has_value())
    return type{std::unexpect, new_error_type{FWD(lh).error()}};
  else
    return type{std::unexpect, new_error_type{FWD(rh).error()}};
}

// Overloads when both sides are non-void, producing expected<pack<...>, ...>
template <some_expected_non_void Lh, some_expected_non_void Rh>
  requires std::is_same_v<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>
           && (not some_pack<typename std::remove_cvref_t<Lh>::value_type>)
           && (not some_pack<typename std::remove_cvref_t<Rh>::value_type>)
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using lh_type = std::remove_cvref_t<Lh>::value_type;
  using rh_type = std::remove_cvref_t<Rh>::value_type;
  using value_type = pack<lh_type, rh_type>;
  using error_type = std::remove_cvref_t<Rh>::error_type;
  using type = expected<value_type, error_type>;
  if (lh.has_value() && rh.has_value())
    return type{std::in_place, pack<lh_type>{FWD(lh).value()}.append(std::in_place_type_t<rh_type>{}, FWD(rh).value())};
  else if (not lh.has_value())
    return type{std::unexpect, FWD(lh).error()};
  else
    return type{std::unexpect, FWD(rh).error()};
}

template <some_expected_non_void Lh, some_expected_non_void Rh>
  requires(not std::is_same_v<typename std::remove_cvref_t<Lh>::error_type,
                              typename std::remove_cvref_t<Rh>::error_type>)
          && (some_sum<typename std::remove_cvref_t<Lh>::error_type>
              || some_sum<typename std::remove_cvref_t<Rh>::error_type>)
          && (not some_pack<typename std::remove_cvref_t<Lh>::value_type>)
          && (not some_pack<typename std::remove_cvref_t<Rh>::value_type>)
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using lh_type = std::remove_cvref_t<Lh>::value_type;
  using rh_type = std::remove_cvref_t<Rh>::value_type;
  using value_type = pack<lh_type, rh_type>;
  using new_error_type = ::fn::detail::_collapsing_sum::normalized<
      ::fn::sum, ::fn::detail::_collapsing_sum::flattened<typename std::remove_cvref_t<Lh>::error_type,
                                                          typename std::remove_cvref_t<Rh>::error_type>>::type;
  using type = expected<value_type, new_error_type>;
  if (lh.has_value() && rh.has_value())
    return type{std::in_place, pack<lh_type>{FWD(lh).value()}.append(std::in_place_type_t<rh_type>{}, FWD(rh).value())};
  else if (not lh.has_value())
    return type{std::unexpect, new_error_type{FWD(lh).error()}};
  else
    return type{std::unexpect, new_error_type{FWD(rh).error()}};
}

template <some_expected_non_void Lh, some_expected_non_void Rh>
  requires std::is_same_v<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>
           && some_pack<typename std::remove_cvref_t<Lh>::value_type>
           && (not some_pack<typename std::remove_cvref_t<Rh>::value_type>)
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using lh_type = std::remove_cvref_t<Lh>::value_type;
  using rh_type = std::remove_cvref_t<Rh>::value_type;
  using value_type = typename lh_type::template append_type<rh_type>;
  using error_type = std::remove_cvref_t<Rh>::error_type;
  using type = expected<value_type, error_type>;
  if (lh.has_value() && rh.has_value())
    return type{std::in_place, FWD(lh).value().append(std::in_place_type_t<rh_type>{}, FWD(rh).value())};
  else if (not lh.has_value())
    return type{std::unexpect, FWD(lh).error()};
  else
    return type{std::unexpect, FWD(rh).error()};
}

template <some_expected_non_void Lh, some_expected_non_void Rh>
  requires(not std::is_same_v<typename std::remove_cvref_t<Lh>::error_type,
                              typename std::remove_cvref_t<Rh>::error_type>)
          && (some_sum<typename std::remove_cvref_t<Lh>::error_type>
              || some_sum<typename std::remove_cvref_t<Rh>::error_type>)
          && some_pack<typename std::remove_cvref_t<Lh>::value_type>
          && (not some_pack<typename std::remove_cvref_t<Rh>::value_type>)
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using lh_type = std::remove_cvref_t<Lh>::value_type;
  using rh_type = std::remove_cvref_t<Rh>::value_type;
  using value_type = typename lh_type::template append_type<rh_type>;
  using new_error_type = ::fn::detail::_collapsing_sum::normalized<
      ::fn::sum, ::fn::detail::_collapsing_sum::flattened<typename std::remove_cvref_t<Lh>::error_type,
                                                          typename std::remove_cvref_t<Rh>::error_type>>::type;
  using type = expected<value_type, new_error_type>;
  if (lh.has_value() && rh.has_value())
    return type{std::in_place, FWD(lh).value().append(std::in_place_type_t<rh_type>{}, FWD(rh).value())};
  else if (not lh.has_value())
    return type{std::unexpect, new_error_type{FWD(lh).error()}};
  else
    return type{std::unexpect, new_error_type{FWD(rh).error()}};
}

template <some_expected_non_void Lh, some_expected_non_void Rh>
  requires(not some_pack<typename std::remove_cvref_t<Lh>::value_type>)
              && some_pack<typename std::remove_cvref_t<Rh>::value_type>
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept = delete;

template <some_expected_non_void Lh, some_expected_non_void Rh>
  requires some_pack<typename std::remove_cvref_t<Lh>::value_type>
               && (some_pack<typename std::remove_cvref_t<Rh>::value_type>)
constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept = delete;

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_EXPECTED
