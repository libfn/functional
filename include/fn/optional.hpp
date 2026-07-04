// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_OPTIONAL
#define INCLUDE_FN_OPTIONAL

#include <pfn/optional.hpp>

#include <fn/detail/functional.hpp>
#include <fn/pack.hpp>
#include <fn/sum.hpp>

#include <compare>
#include <initializer_list>
#include <type_traits>
#include <utility>

namespace fn {

template <typename T>
concept some_optional = detail::_some_optional<T>;

namespace detail {

struct optional_policy {
  template <class U> using type = ::fn::optional<U>;
  template <class X> static constexpr bool is_specialization = _is_some_optional<X &>;
};

// Storage layer for ::fn::optional. Inherits the standard-conformant base from
// pfn, then hides the three monadic static helpers with sum-aware variants that
// materialise their result via `optional_policy::template type<U>`.
// The transform helpers hand pfn's _optional_from_invoke constructor a zero-argument
// thunk, so the result's contained value is direct-non-list-initialized from fn's own
// _invoke (or sum::transform) result: no extra move, and immovable result types work.
template <typename T> struct _optional_base : ::pfn::detail::_optional_base<T, optional_policy> {
  using _pfn_base = ::pfn::detail::_optional_base<T, optional_policy>;
  using _pfn_base::_pfn_base;

  // and_then
  template <typename Self, typename Fn>
  static constexpr auto _and_then(Self &&self, Fn &&fn)
    requires ::fn::detail::_is_invocable<Fn, decltype(*FWD(self))>::value
  {
    using type = typename ::fn::detail::_invoke_result<Fn, decltype(*FWD(self))>::type;
    static_assert(_is_some_optional<type &>);
    if (self.has_value())
      return ::fn::detail::_invoke(FWD(fn), *FWD(self));
    else {
#if defined(__clang__) && __clang_major__ <= 18
      // clang 15-18 miscompile the prvalue return below for three of the four Self ref-qualifier
      // instantiations (&, const &, const &&) at -O1/-O2: the disengaged result is observed with
      // garbage in set_ (storage-poison). The workaround dodges the buggy mandatory copy-elision.
      return ::std::move(type(::std::nullopt));
#else
      return type(::std::nullopt);
#endif
    }
  }

  // or_else (with value-widening into a sum)
  template <typename Self, typename Fn>
  static constexpr auto _or_else(Self &&self, Fn &&fn)
    requires ::fn::detail::_is_invocable<Fn>::value && ::std::is_constructible_v<T, decltype(*FWD(self))>
  {
    using type = typename ::fn::detail::_invoke_result<Fn>::type;
    static_assert(_is_some_optional<type &>);
    static_assert(::std::is_same_v<typename type::value_type, T> || some_sum<T>);
    if constexpr (::std::is_same_v<typename type::value_type, T>) {
      if (self.has_value())
        return type(::std::in_place, *FWD(self));
      else
        return ::fn::detail::_invoke(FWD(fn));
    } else {
      using new_value_type = sum_for<T, typename type::value_type>;
      using new_type = ::fn::optional<new_value_type>;
      if (self.has_value())
        return new_type{::std::in_place, *FWD(self)};
      else {
        auto t = ::fn::detail::_invoke(FWD(fn));
        if (t.has_value())
          return new_type{::std::in_place, ::std::move(t).value()};
        else
          return new_type{::std::nullopt};
      }
    }
  }

  // transform, value type is not a sum
  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&fn)
    requires(not some_sum<T>) && ::fn::detail::_is_invocable_if<not some_sum<T>, Fn, decltype(*FWD(self))>::value
  {
    using new_value_type = typename ::fn::detail::_invoke_result<Fn, decltype(*FWD(self))>::type;
    using type = ::fn::optional<new_value_type>;
    if (self.has_value())
      return type(::pfn::detail::_optional_from_invoke,
                  [&]() -> decltype(auto) { return ::fn::detail::_invoke(FWD(fn), *FWD(self)); });
    else
      return type(::std::nullopt);
  }

  // transform, value type is a sum (delegates to sum::transform)
  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&fn)
    requires some_sum<T>
  {
    using new_value_type = decltype((*FWD(self)).transform(FWD(fn)));
    using type = ::fn::optional<new_value_type>;
    if (self.has_value())
      return type(::pfn::detail::_optional_from_invoke,
                  [&]() -> decltype(auto) { return (*FWD(self)).transform(FWD(fn)); });
    else
      return type(::std::nullopt);
  }
};

} // namespace detail

template <typename T> class optional : private detail::_optional_base<T> {
  static_assert(not ::std::is_same_v<T, ::fn::sum<>>);
  static_assert(not ::std::is_reference_v<T>); // unlike pfn::optional, no support for optional<T&>
  using _base = detail::_optional_base<T>;

  // Allow sibling _optional_base instantiations to downcast into the private base.
  template <class, class> friend struct ::pfn::detail::_optional_base;
  template <class> friend struct ::fn::detail::_optional_base;

public:
  using value_type = T;

  // Constructors. Explicit forwarders to the base mirror pfn::optional.
  constexpr optional() noexcept : _base(::std::nullopt) {}
  constexpr optional(::std::nullopt_t) noexcept : _base(::std::nullopt) {}

  template <class U>
  constexpr explicit(not ::std::is_convertible_v<U const &, T>) optional(optional<U> const &s) //
      noexcept(::std::is_nothrow_constructible_v<T, U const &>)                                // extension
    requires(_base::template _can_copy_convert<U>::value)
      : _base(s)
  {
  }
  template <class U>
  constexpr explicit(not ::std::is_convertible_v<U, T>) optional(optional<U> &&s) //
      noexcept(::std::is_nothrow_constructible_v<T, U>)                           // extension
    requires(_base::template _can_move_convert<U>::value)
      : _base(::std::move(s))
  {
  }
  template <class U = ::std::remove_cv_t<T>>
  constexpr explicit(not ::std::is_convertible_v<U, T>) optional(U &&v) //
      noexcept(::std::is_nothrow_constructible_v<T, U>)                 // extension
    requires(_base::template _can_convert<U>::value)
      : _base(::std::in_place, FWD(v))
  {
  }

  template <class... Args>
  constexpr explicit optional(::std::in_place_t, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<T, Args...>) // extension
    requires ::std::is_constructible_v<T, Args...>
      : _base(::std::in_place, FWD(a)...)
  {
  }
  template <class U, class... Args>
  constexpr explicit optional(::std::in_place_t, ::std::initializer_list<U> il, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<T, ::std::initializer_list<U> &, Args...>)  // extension
    requires ::std::is_constructible_v<T, ::std::initializer_list<U> &, Args...>
      : _base(::std::in_place, il, FWD(a)...)
  {
  }

  constexpr optional(optional const &) = delete;
  constexpr optional(optional const &s)                   //
      noexcept(::std::is_nothrow_copy_constructible_v<T>) // extension
    requires(::std::is_copy_constructible_v<T> && ::std::is_trivially_copy_constructible_v<T>)
  = default;
  constexpr optional(optional const &s)                   //
      noexcept(::std::is_nothrow_copy_constructible_v<T>) // extension
    requires(::std::is_copy_constructible_v<T> && not ::std::is_trivially_copy_constructible_v<T>)
      : _base(s.set_, FWD(s).storage_)
  {
  }
  constexpr optional(optional &&) noexcept
    requires(::std::is_move_constructible_v<T> && ::std::is_trivially_move_constructible_v<T>)
  = default;
  constexpr optional(optional &&s) //
      noexcept(::std::is_nothrow_move_constructible_v<T>)
    requires(::std::is_move_constructible_v<T> && not ::std::is_trivially_move_constructible_v<T>)
      : _base(s.set_, FWD(s).storage_)
  {
  }

  constexpr ~optional() = default;

  // Assignment. Explicit forwarders to the base mirror pfn::optional.
  constexpr optional &operator=(::std::nullopt_t) noexcept
  {
    this->reset();
    return *this;
  }
  constexpr optional &operator=(optional const &) = delete;
  constexpr optional &operator=(optional const &s)                                                  //
      noexcept(::std::is_nothrow_copy_assignable_v<T> && ::std::is_nothrow_copy_constructible_v<T>) // extension
    requires(::std::is_copy_constructible_v<T> && ::std::is_copy_assignable_v<T>)
  {
    this->_assign(static_cast<_base const &>(s));
    return *this;
  }
  constexpr optional &operator=(optional &&s) //
      noexcept(::std::is_nothrow_move_assignable_v<T> && ::std::is_nothrow_move_constructible_v<T>)
    requires(::std::is_move_constructible_v<T> && ::std::is_move_assignable_v<T>)
  {
    this->_assign(static_cast<_base &&>(s));
    return *this;
  }

  template <class U = ::std::remove_cv_t<T>>
  constexpr optional &operator=(U &&v)                                                            //
      noexcept(::std::is_nothrow_assignable_v<T &, U> && ::std::is_nothrow_constructible_v<T, U>) // extension
    requires(_base::template _can_assign<U>::value)
  {
    this->_assign_value(FWD(v));
    return *this;
  }
  template <class U>
  constexpr optional &operator=(optional<U> const &s) //
      noexcept(::std::is_nothrow_assignable_v<T &, U const &>
               && ::std::is_nothrow_constructible_v<T, U const &>) // extension
    requires(_base::template _can_copy_assign<U>::value)
  {
    this->_assign_from(s);
    return *this;
  }
  template <class U>
  constexpr optional &operator=(optional<U> &&s)                                                  //
      noexcept(::std::is_nothrow_assignable_v<T &, U> && ::std::is_nothrow_constructible_v<T, U>) // extension
    requires(_base::template _can_move_assign<U>::value)
  {
    this->_assign_from(::std::move(s));
    return *this;
  }

  // Emplace and reset inherited from _optional_base
  using _base::emplace;
  using _base::reset;

  // Swap; body delegates to _optional_base helper
  constexpr void swap(optional &rhs) //
      noexcept(::std::is_nothrow_move_constructible_v<T> && ::std::is_nothrow_swappable_v<T>)
    requires(::std::is_swappable_v<T> && ::std::is_move_constructible_v<T>)
  {
    this->_swap_with(rhs);
  }

  // Observers inherited from _optional_base
  using _base::has_value;
  using _base::operator bool;
  using _base::operator*;
  using _base::operator->;
  using _base::value;
  using _base::value_or;

  // Monadic operations. Bodies delegate to _optional_base static helpers, which perform sum-widening.
  template <class F>
  constexpr auto and_then(F &&f) &                        //
      noexcept(noexcept(_base::_and_then(*this, FWD(f)))) // extension
      -> decltype(_base::_and_then(*this, FWD(f)))
  {
    return _base::_and_then(*this, FWD(f));
  }
  template <class F>
  constexpr auto and_then(F &&f) &&                                    //
      noexcept(noexcept(_base::_and_then(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_and_then(::std::move(*this), FWD(f)))
  {
    return _base::_and_then(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto and_then(F &&f) const &                  //
      noexcept(noexcept(_base::_and_then(*this, FWD(f)))) // extension
      -> decltype(_base::_and_then(*this, FWD(f)))
  {
    return _base::_and_then(*this, FWD(f));
  }
  template <class F>
  constexpr auto and_then(F &&f) const &&                              //
      noexcept(noexcept(_base::_and_then(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_and_then(::std::move(*this), FWD(f)))
  {
    return _base::_and_then(::std::move(*this), FWD(f));
  }

  template <class F>
  constexpr auto or_else(F &&f) &                        //
      noexcept(noexcept(_base::_or_else(*this, FWD(f)))) // extension
      -> decltype(_base::_or_else(*this, FWD(f)))
  {
    return _base::_or_else(*this, FWD(f));
  }
  template <class F>
  constexpr auto or_else(F &&f) &&                                    //
      noexcept(noexcept(_base::_or_else(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_or_else(::std::move(*this), FWD(f)))
  {
    return _base::_or_else(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto or_else(F &&f) const &                  //
      noexcept(noexcept(_base::_or_else(*this, FWD(f)))) // extension
      -> decltype(_base::_or_else(*this, FWD(f)))
  {
    return _base::_or_else(*this, FWD(f));
  }
  template <class F>
  constexpr auto or_else(F &&f) const &&                              //
      noexcept(noexcept(_base::_or_else(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_or_else(::std::move(*this), FWD(f)))
  {
    return _base::_or_else(::std::move(*this), FWD(f));
  }

  template <class F>
  constexpr auto transform(F &&f) &                        //
      noexcept(noexcept(_base::_transform(*this, FWD(f)))) // extension
      -> decltype(_base::_transform(*this, FWD(f)))
  {
    return _base::_transform(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform(F &&f) &&                                    //
      noexcept(noexcept(_base::_transform(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_transform(::std::move(*this), FWD(f)))
  {
    return _base::_transform(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto transform(F &&f) const &                  //
      noexcept(noexcept(_base::_transform(*this, FWD(f)))) // extension
      -> decltype(_base::_transform(*this, FWD(f)))
  {
    return _base::_transform(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform(F &&f) const &&                              //
      noexcept(noexcept(_base::_transform(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_transform(::std::move(*this), FWD(f)))
  {
    return _base::_transform(::std::move(*this), FWD(f));
  }

  // Convert to graded monad
  auto sum_value() const & -> optional<sum<value_type>>
    requires(not some_sum<value_type>)
  {
    using type = optional<sum<value_type>>;
    if (this->has_value())
      return type{::std::in_place, sum<value_type>(this->value())};
    else
      return type{::std::nullopt};
  }
  auto sum_value() && -> optional<sum<value_type>>
    requires(not some_sum<value_type>)
  {
    using type = optional<sum<value_type>>;
    if (this->has_value())
      return type{::std::in_place, sum<value_type>(::std::move(*this).value())};
    else
      return type{::std::nullopt};
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
    return ::std::move(*this);
  }
  auto sum_value() const && -> decltype(auto)
    requires(some_sum<value_type>)
  {
    return ::std::move(*this);
  }

private:
  // Direct-non-list-initializes the contained value from the result of a callable; used by
  // the monadic functions implemented in _optional_base.
  template <class Fn, class... Args>
  constexpr explicit optional(::pfn::detail::_optional_from_invoke_t tag, Fn &&fn, Args &&...args) //
      noexcept(::std::is_nothrow_constructible_v<_base, ::pfn::detail::_optional_from_invoke_t, Fn, Args...>)
      : _base(tag, FWD(fn), FWD(args)...)
  {
  }
};

template <class T> optional(T) -> optional<T>;

// Comparison operators. fn::optional does not derive from pfn::optional, so pfn's
// namespace-scope comparison templates do not apply here; unlike pfn (which follows the
// draft synopsis) only `==` and `<=>` are spelled, the rest come from C++20 rewriting.
template <class T, class U>
constexpr bool operator==(optional<T> const &x, optional<U> const &y) //
    noexcept(::pfn::detail::_eq_bool_noexcept<T, U>)                  // extension
  requires ::pfn::detail::_eq_bool<T, U>
{
  if (x.has_value() != y.has_value())
    return false;
  if (not x.has_value())
    return true;
  return *x == *y;
}
template <class T, ::std::three_way_comparable_with<T> U>
constexpr ::std::compare_three_way_result_t<T, U> operator<=>(optional<T> const &x, optional<U> const &y)
{
  return x.has_value() && y.has_value() ? *x <=> *y : x.has_value() <=> y.has_value();
}

// Comparison with nullopt
template <class T> constexpr bool operator==(optional<T> const &x, ::std::nullopt_t) noexcept
{
  return not x.has_value();
}
template <class T> constexpr ::std::strong_ordering operator<=>(optional<T> const &x, ::std::nullopt_t) noexcept
{
  return x.has_value() <=> false;
}

// Comparison with a value
template <class T, class U>
constexpr bool operator==(optional<T> const &x, U const &v) //
    noexcept(::pfn::detail::_eq_bool_noexcept<T, U>)        // extension
  requires(not some_optional<U>) && ::pfn::detail::_eq_bool<T, U>
{
  return x.has_value() && *x == v;
}
template <class T, class U>
  requires(not some_optional<U>) && ::std::three_way_comparable_with<T, U>
constexpr ::std::compare_three_way_result_t<T, U> operator<=>(optional<T> const &x, U const &v)
{
  return x.has_value() ? *x <=> v : ::std::strong_ordering::less;
}

// Lifts for sum transformation functions
[[nodiscard]] constexpr auto sum_value(some_optional auto &&src) -> decltype(auto) { return FWD(src).sum_value(); }

template <some_optional Lh, some_optional Rh> [[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  constexpr auto efn = [](auto const &) { return ::std::nullopt; };
  return ::fn::detail::_join<fn::optional>(FWD(lh), FWD(rh), efn);
}

} // namespace fn

#endif // INCLUDE_FN_OPTIONAL
