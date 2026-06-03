// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_EXPECTED
#define INCLUDE_FN_EXPECTED

#include <pfn/expected.hpp>

#include <fn/fwd.hpp>
#include <fn/pack.hpp>
#include <fn/sum.hpp>

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

namespace detail {

struct expected_policy {
  template <class U, class G> using type = ::fn::expected<U, G>;
  template <class X> static constexpr bool is_specialization = _is_some_expected<X &>;
};

// Storage layer for ::fn::expected. Inherits the standard-conformant base from
// pfn, then hides the four monadic static helpers with sum-widening variants
// that materialise their result via `expected_policy::template type<U, G>`.
template <typename T, typename E> struct _storage : ::pfn::detail::_storage<T, E, expected_policy> {
  using _pfn_base = ::pfn::detail::_storage<T, E, expected_policy>;
  using _pfn_base::_pfn_base;

  // and_then, non-void value type
  template <typename Self, typename Fn>
  static constexpr auto _and_then(Self &&self, Fn &&fn)
    requires(not ::std::is_void_v<T>) && ::fn::detail::_is_invocable<Fn, decltype(_pfn_base::_value(FWD(self)))>::value
            && ::std::is_constructible_v<E, decltype(_pfn_base::_error(FWD(self)))>
  {
    using type = typename ::fn::detail::_invoke_result<Fn, decltype(_pfn_base::_value(FWD(self)))>::type;
    static_assert(_is_some_expected<type &>);
    static_assert(::std::is_same_v<typename type::error_type, E> || some_sum<E>);
    if constexpr (::std::is_same_v<typename type::error_type, E>) {
      if (self.has_value())
        return ::fn::detail::_invoke(FWD(fn), _pfn_base::_value(FWD(self)));
      else
        return type(::pfn::unexpect, _pfn_base::_error(FWD(self)));
    } else {
      using new_error_type = sum_for<E, typename type::error_type>;
      using new_type = ::fn::expected<typename type::value_type, new_error_type>;
      if (self.has_value()) {
        auto t = ::fn::detail::_invoke(FWD(fn), _pfn_base::_value(FWD(self)));
        if (t.has_value())
          if constexpr (not ::std::is_void_v<typename new_type::value_type>)
            return new_type{::std::in_place, ::std::move(t).value()};
          else
            return new_type{::std::in_place};
        else
          return new_type{::pfn::unexpect, ::std::move(t).error()};
      } else {
        if constexpr (not ::std::is_same_v<E, sum<>>)
          return new_type(::pfn::unexpect, _pfn_base::_error(FWD(self)));
        else
          ::std::unreachable();
      }
    }
  }

  // and_then, void value type
  template <typename Self, typename Fn>
  static constexpr auto _and_then(Self &&self, Fn &&fn)
    requires(::std::is_void_v<T>) && ::fn::detail::_is_invocable<Fn>::value
            && ::std::is_constructible_v<E, decltype(_pfn_base::_error(FWD(self)))>
  {
    using type = typename ::fn::detail::_invoke_result<Fn>::type;
    static_assert(_is_some_expected<type &>);
    static_assert(::std::is_same_v<typename type::error_type, E> || some_sum<E>);
    if constexpr (::std::is_same_v<typename type::error_type, E>) {
      if (self.has_value())
        return ::fn::detail::_invoke(FWD(fn));
      else
        return type(::pfn::unexpect, _pfn_base::_error(FWD(self)));
    } else {
      using new_error_type = sum_for<E, typename type::error_type>;
      using new_type = ::fn::expected<typename type::value_type, new_error_type>;
      if (self.has_value()) {
        auto t = ::fn::detail::_invoke(FWD(fn));
        if (t.has_value())
          if constexpr (not ::std::is_void_v<typename new_type::value_type>)
            return new_type{::std::in_place, ::std::move(t).value()};
          else
            return new_type{::std::in_place};
        else
          return new_type{::pfn::unexpect, ::std::move(t).error()};
      } else {
        if constexpr (not ::std::is_same_v<E, sum<>>)
          return new_type(::pfn::unexpect, _pfn_base::_error(FWD(self)));
        else
          ::std::unreachable();
      }
    }
  }

  // or_else, covers both void and non-void value type
  template <typename Self, typename Fn>
  static constexpr auto _or_else(Self &&self, Fn &&fn)
    requires ::fn::detail::_is_invocable<Fn, decltype(_pfn_base::_error(FWD(self)))>::value
             && (::std::is_void_v<T> || ::std::is_constructible_v<T, decltype(_pfn_base::_value(FWD(self)))>)
  {
    using type = typename ::fn::detail::_invoke_result<Fn, decltype(_pfn_base::_error(FWD(self)))>::type;
    static_assert(_is_some_expected<type &>);
    static_assert(::std::is_same_v<typename type::value_type, T> || some_sum<T>);
    if constexpr (::std::is_same_v<typename type::value_type, T>) {
      if (self.has_value())
        if constexpr (not ::std::is_void_v<T>)
          return type(::std::in_place, _pfn_base::_value(FWD(self)));
        else {
          static_assert(::std::is_void_v<typename type::value_type>);
#if defined(__clang__) && __clang_major__ <= 18
          // clang 15-18 miscompile the prvalue return below for three of the four Self ref-qualifier
          // instantiations (&, const &, const &&) at -O1/-O2: the value-state result is observed with
          // set_ == false (storage-poison). Naming the local and returning it by name dodges the buggy
          // mandatory copy-elision lowering. Fixed in clang-19; gcc unaffected — both take the prvalue
          // path, which keeps guaranteed elision and so also supports a non-movable error type (whereas
          // this workaround requires `type` be move-constructible).
          type result{::std::in_place};
          return result;
#else
          return type{::std::in_place};
#endif
        }
      else
        return ::fn::detail::_invoke(FWD(fn), _pfn_base::_error(FWD(self)));
    } else {
      static_assert(not ::std::is_void_v<typename type::value_type>);
      using new_value_type = sum_for<T, typename type::value_type>;
      using new_type = ::fn::expected<new_value_type, typename type::error_type>;
      if (self.has_value())
        return new_type{::std::in_place, _pfn_base::_value(FWD(self))};
      else {
        auto t = ::fn::detail::_invoke(FWD(fn), _pfn_base::_error(FWD(self)));
        if (t.has_value())
          return new_type{::std::in_place, ::std::move(t).value()};
        else
          return new_type{::pfn::unexpect, ::std::move(t).error()};
      }
    }
  }

  // transform, non-void value type, not a sum
  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&fn)
    requires(not ::std::is_void_v<T>) && (not some_sum<T>)
            && ::fn::detail::_is_invocable<Fn, decltype(_pfn_base::_value(FWD(self)))>::value
            && ::std::is_constructible_v<E, decltype(_pfn_base::_error(FWD(self)))>
  {
    using new_value_type = typename ::fn::detail::_invoke_result<Fn, decltype(_pfn_base::_value(FWD(self)))>::type;
    using type = ::fn::expected<new_value_type, E>;
    if (self.has_value())
      if constexpr (::std::is_void_v<new_value_type>) {
        ::fn::detail::_invoke(FWD(fn), _pfn_base::_value(FWD(self)));
        return type();
      } else
        return type(::std::in_place, ::fn::detail::_invoke(FWD(fn), _pfn_base::_value(FWD(self))));
    else
      return type(::pfn::unexpect, _pfn_base::_error(FWD(self)));
  }

  // transform, value type is a sum (delegates to sum::transform)
  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&fn)
    requires some_sum<T>
  {
    using new_value_type = decltype(_pfn_base::_value(FWD(self)).transform(FWD(fn)));
    using type = ::fn::expected<new_value_type, E>;
    if (self.has_value())
      if constexpr (::std::is_void_v<new_value_type>) {
        _pfn_base::_value(FWD(self)).transform(FWD(fn));
        return type();
      } else
        return type(::std::in_place, _pfn_base::_value(FWD(self)).transform(FWD(fn)));
    else
      return type(::pfn::unexpect, _pfn_base::_error(FWD(self)));
  }

  // transform, void value type
  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&fn)
    requires(::std::is_void_v<T>) && ::fn::detail::_is_invocable<Fn>::value
  {
    using new_value_type = typename ::fn::detail::_invoke_result<Fn>::type;
    using type = ::fn::expected<new_value_type, E>;
    if (self.has_value())
      if constexpr (::std::is_void_v<new_value_type>) {
        ::fn::detail::_invoke(FWD(fn));
        return type();
      } else
        return type(::std::in_place, ::fn::detail::_invoke(FWD(fn)));
    else
      return type(::pfn::unexpect, _pfn_base::_error(FWD(self)));
  }

  // transform_error, error type is not a sum
  template <typename Self, typename Fn>
  static constexpr auto _transform_error(Self &&self, Fn &&fn)
    requires(not some_sum<E>) && ::fn::detail::_is_invocable<Fn, decltype(_pfn_base::_error(FWD(self)))>::value
  {
    using new_error_type = typename ::fn::detail::_invoke_result<Fn, decltype(_pfn_base::_error(FWD(self)))>::type;
    using type = ::fn::expected<T, new_error_type>;
    if (self.has_value())
      if constexpr (not ::std::is_void_v<T>)
        return type(::std::in_place, _pfn_base::_value(FWD(self)));
      else
        return type();
    else
      return type(::pfn::unexpect, ::fn::detail::_invoke(FWD(fn), _pfn_base::_error(FWD(self))));
  }

  // transform_error, error type is a sum (delegates to sum::transform)
  template <typename Self, typename Fn>
  static constexpr auto _transform_error(Self &&self, Fn &&fn)
    requires some_sum<E>
  {
    using new_error_type = decltype(_pfn_base::_error(FWD(self)).transform(FWD(fn)));
    using type = ::fn::expected<T, new_error_type>;
    if (self.has_value())
      if constexpr (not ::std::is_void_v<T>)
        return type(::std::in_place, _pfn_base::_value(FWD(self)));
      else
        return type();
    else
      return type(::pfn::unexpect, _pfn_base::_error(FWD(self)).transform(FWD(fn)));
  }
};

} // namespace detail

// Primary template - non-void value type
template <typename T, typename Err> struct expected : private detail::_storage<T, Err> {
  using _base = detail::_storage<T, Err>;
  using value_type = T;
  using error_type = Err;
  using unexpected_type = ::pfn::unexpected<Err>;
  static_assert(not ::std::is_same_v<value_type, ::fn::sum<>>);

  template <class U> using rebind = expected<U, error_type>;

  // Allow sibling _storage instantiations to downcast into the private base.
  template <class, class, class> friend struct ::pfn::detail::_storage;
  template <class, class> friend struct ::fn::detail::_storage;

  // Constructors. Explicit forwarders to the base mirror pfn::expected.
  constexpr expected() noexcept(::std::is_nothrow_default_constructible_v<T>)
    requires ::std::is_default_constructible_v<T>
      : _base(::std::in_place)
  {
  }

  template <class U, class G>
  constexpr explicit(not ::std::is_convertible_v<U const &, T> || not ::std::is_convertible_v<G const &, Err>)
      expected(expected<U, G> const &s) //
      noexcept(::std::is_nothrow_constructible_v<T, U const &> && ::std::is_nothrow_constructible_v<Err, G const &>)
    requires(_base::template _can_copy_convert<U, G>::value)
      : _base(s)
  {
  }
  template <class U, class G>
  constexpr explicit(not ::std::is_convertible_v<U, T> || not ::std::is_convertible_v<G, Err>)
      expected(expected<U, G> &&s) //
      noexcept(::std::is_nothrow_constructible_v<T, U> && ::std::is_nothrow_constructible_v<Err, G>)
    requires(_base::template _can_move_convert<U, G>::value)
      : _base(::std::move(s))
  {
  }
  template <class U = ::std::remove_cv_t<T>>
  constexpr explicit(not ::std::is_convertible_v<U, T>) expected(U &&v) //
      noexcept(::std::is_nothrow_constructible_v<T, U>)
    requires(_base::template _can_convert<U>::value)
      : _base(::std::in_place, FWD(v))
  {
  }

  template <class G>
  constexpr explicit(not ::std::is_convertible_v<G const &, Err>) expected(::pfn::unexpected<G> const &g) //
      noexcept(::std::is_nothrow_constructible_v<Err, G const &>)
    requires(::std::is_constructible_v<Err, G const &>)
      : _base(::pfn::unexpect, ::std::forward<G const &>(g.error()))
  {
  }
  template <class G>
  constexpr explicit(not ::std::is_convertible_v<G, Err>) expected(::pfn::unexpected<G> &&g) //
      noexcept(::std::is_nothrow_constructible_v<Err, G>)
    requires(::std::is_constructible_v<Err, G>)
      : _base(::pfn::unexpect, ::std::forward<G>(g.error()))
  {
  }

  template <class... Args>
  constexpr explicit expected(::std::in_place_t, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<T, Args...>)
    requires ::std::is_constructible_v<T, Args...>
      : _base(::std::in_place, FWD(a)...)
  {
  }
  template <class U, class... Args>
  constexpr explicit expected(::std::in_place_t, ::std::initializer_list<U> il, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<T, ::std::initializer_list<U> &, Args...>)
    requires ::std::is_constructible_v<T, ::std::initializer_list<U> &, Args...>
      : _base(::std::in_place, il, FWD(a)...)
  {
  }
  template <class... Args>
  constexpr explicit expected(::pfn::unexpect_t, Args &&...a)   //
      noexcept(::std::is_nothrow_constructible_v<Err, Args...>) //
    requires ::std::is_constructible_v<Err, Args...>
      : _base(::pfn::unexpect, FWD(a)...)
  {
  }
  template <class U, class... Args>
  constexpr explicit expected(::pfn::unexpect_t, ::std::initializer_list<U> il, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<Err, ::std::initializer_list<U> &, Args...>)
    requires ::std::is_constructible_v<Err, ::std::initializer_list<U> &, Args...>
      : _base(::pfn::unexpect, il, FWD(a)...)
  {
  }

  constexpr expected(expected const &) = delete;
  constexpr expected(expected const &s) //
      noexcept(::std::is_nothrow_copy_constructible_v<T> && ::std::is_nothrow_copy_constructible_v<Err>)
    requires(::std::is_copy_constructible_v<T> && ::std::is_copy_constructible_v<Err>
             && ::std::is_trivially_copy_constructible_v<T> && ::std::is_trivially_copy_constructible_v<Err>)
  = default;
  constexpr expected(expected const &s) //
      noexcept(::std::is_nothrow_copy_constructible_v<T> && ::std::is_nothrow_copy_constructible_v<Err>)
    requires(::std::is_copy_constructible_v<T> && ::std::is_copy_constructible_v<Err>
             && (not ::std::is_trivially_copy_constructible_v<T> || not ::std::is_trivially_copy_constructible_v<Err>))
      : _base(s.set_, FWD(s).storage_)
  {
  }
  constexpr expected(expected &&s)
    requires(::std::is_move_constructible_v<T> && ::std::is_move_constructible_v<Err>
             && ::std::is_trivially_move_constructible_v<T> && ::std::is_trivially_move_constructible_v<Err>)
  = default;
  constexpr expected(expected &&s) //
      noexcept(::std::is_nothrow_move_constructible_v<T> && ::std::is_nothrow_move_constructible_v<Err>)
    requires(::std::is_move_constructible_v<T> && ::std::is_move_constructible_v<Err>
             && (not ::std::is_trivially_move_constructible_v<T> || not ::std::is_trivially_move_constructible_v<Err>))
      : _base(s.set_, FWD(s).storage_)
  {
  }

  constexpr ~expected() = default;

  // Assignment. Explicit forwarders mirror pfn::expected to avoid an MSVC bug.
  template <class U = T>
  constexpr expected &operator=(U &&s) //
      noexcept(::std::is_nothrow_assignable_v<T &, U> && ::std::is_nothrow_constructible_v<T, U>)
    requires(_base::template _can_convert_assign<U>::value)
  {
    this->_assign_value(FWD(s));
    return *this;
  }
  template <class G>
  constexpr expected &operator=(::pfn::unexpected<G> const &s) //
      noexcept(::std::is_nothrow_assignable_v<Err &, G const &> && ::std::is_nothrow_constructible_v<Err, G const &>)
    requires(::std::is_constructible_v<Err, G const &> && ::std::is_assignable_v<Err &, G const &>
             && (::std::is_nothrow_constructible_v<Err, G const &> || ::std::is_nothrow_move_constructible_v<T>
                 || ::std::is_nothrow_move_constructible_v<Err>))
  {
    this->_assign_unexpected(s);
    return *this;
  }
  template <class G>
  constexpr expected &operator=(::pfn::unexpected<G> &&s) //
      noexcept(::std::is_nothrow_assignable_v<Err &, G> && ::std::is_nothrow_constructible_v<Err, G>)
    requires(::std::is_constructible_v<Err, G> && ::std::is_assignable_v<Err &, G>
             && (::std::is_nothrow_constructible_v<Err, G> || ::std::is_nothrow_move_constructible_v<T>
                 || ::std::is_nothrow_move_constructible_v<Err>))
  {
    this->_assign_unexpected(::std::move(s));
    return *this;
  }
  constexpr expected &operator=(expected const &) = delete;
  constexpr expected &operator=(expected const &s) //
      noexcept(::std::is_nothrow_copy_assignable_v<T> && ::std::is_nothrow_copy_constructible_v<T>
               && ::std::is_nothrow_copy_assignable_v<Err> && ::std::is_nothrow_copy_constructible_v<Err>)
    requires(::std::is_copy_assignable_v<T> && ::std::is_copy_constructible_v<T> && ::std::is_copy_assignable_v<Err>
             && ::std::is_copy_constructible_v<Err>
             && (::std::is_nothrow_move_constructible_v<T> || ::std::is_nothrow_move_constructible_v<Err>))
  {
    this->_assign(static_cast<_base const &>(s));
    return *this;
  }
  constexpr expected &operator=(expected &&s) //
      noexcept(::std::is_nothrow_move_assignable_v<T> && ::std::is_nothrow_move_constructible_v<T>
               && ::std::is_nothrow_move_assignable_v<Err> && ::std::is_nothrow_move_constructible_v<Err>)
    requires(::std::is_move_constructible_v<T> && ::std::is_move_assignable_v<T> && ::std::is_move_constructible_v<Err>
             && ::std::is_move_assignable_v<Err>
             && (::std::is_nothrow_move_constructible_v<T> || ::std::is_nothrow_move_constructible_v<Err>))
  {
    this->_assign(static_cast<_base &&>(s));
    return *this;
  }

  // Observers inherited from _storage
  using _base::operator*;
  using _base::operator->;
  using _base::operator bool;
  using _base::error;
  using _base::error_or;
  using _base::has_error;
  using _base::has_value;
  using _base::value;
  using _base::value_or;

  // Emplace inherited from _storage
  using _base::emplace;

  // Swap; body delegates to _storage helper
  constexpr void
  swap(expected &rhs) noexcept(::std::is_nothrow_move_constructible_v<T> && ::std::is_nothrow_swappable_v<T>
                               && ::std::is_nothrow_move_constructible_v<Err> && ::std::is_nothrow_swappable_v<Err>)
    requires(::std::is_swappable_v<T> && ::std::is_swappable_v<Err> && ::std::is_move_constructible_v<T>
             && ::std::is_move_constructible_v<Err>
             && (::std::is_nothrow_move_constructible_v<T> || ::std::is_nothrow_move_constructible_v<Err>))
  {
    this->_swap_with(rhs);
  }

  // Monadic operations. Bodies delegate to _storage static helpers, which perform sum-widening.
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

  template <class F>
  constexpr auto transform_error(F &&f) &                        //
      noexcept(noexcept(_base::_transform_error(*this, FWD(f)))) // extension
      -> decltype(_base::_transform_error(*this, FWD(f)))
  {
    return _base::_transform_error(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform_error(F &&f) &&                                    //
      noexcept(noexcept(_base::_transform_error(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_transform_error(::std::move(*this), FWD(f)))
  {
    return _base::_transform_error(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto transform_error(F &&f) const &                  //
      noexcept(noexcept(_base::_transform_error(*this, FWD(f)))) // extension
      -> decltype(_base::_transform_error(*this, FWD(f)))
  {
    return _base::_transform_error(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform_error(F &&f) const &&                              //
      noexcept(noexcept(_base::_transform_error(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_transform_error(::std::move(*this), FWD(f)))
  {
    return _base::_transform_error(::std::move(*this), FWD(f));
  }

  // Convert to graded monad
  auto sum_error() const & -> expected<value_type, sum<error_type>>
    requires(not some_sum<error_type>)
  {
    using type = expected<value_type, sum<error_type>>;
    if (this->has_value())
      return type{std::in_place, this->value()};
    else
      return type{::pfn::unexpect, sum<error_type>(this->error())};
  }
  auto sum_error() && -> expected<value_type, sum<error_type>>
    requires(not some_sum<error_type>)
  {
    using type = expected<value_type, sum<error_type>>;
    if (this->has_value())
      return type{std::in_place, std::move(*this).value()};
    else
      return type{::pfn::unexpect, sum<error_type>(std::move(*this).error())};
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
    requires(not some_sum<value_type>)
  {
    using type = expected<sum<value_type>, error_type>;
    if (this->has_value())
      return type{std::in_place, sum<value_type>(this->value())};
    else
      return type{::pfn::unexpect, this->error()};
  }
  auto sum_value() && -> expected<sum<value_type>, error_type>
    requires(not some_sum<value_type>)
  {
    using type = expected<sum<value_type>, error_type>;
    if (this->has_value())
      return type{std::in_place, sum<value_type>(std::move(*this).value())};
    else
      return type{::pfn::unexpect, std::move(*this).error()};
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
};

template <typename Err> struct expected<void, Err> : private detail::_storage<void, Err> {
  using _base = detail::_storage<void, Err>;
  using value_type = void;
  using error_type = Err;
  using unexpected_type = ::pfn::unexpected<Err>;

  template <class U> using rebind = expected<U, error_type>;

  template <class, class, class> friend struct ::pfn::detail::_storage;
  template <class, class> friend struct ::fn::detail::_storage;

  constexpr expected() noexcept : _base(::std::in_place) {}

  template <class U, class G>
  constexpr explicit(not ::std::is_convertible_v<G const &, Err>) expected(expected<U, G> const &s) //
      noexcept(::std::is_nothrow_constructible_v<Err, G const &>)
    requires(_base::template _can_copy_convert<U, G>::value)
      : _base(s)
  {
  }
  template <class U, class G>
  constexpr explicit(not ::std::is_convertible_v<G, Err>) expected(expected<U, G> &&s) //
      noexcept(::std::is_nothrow_constructible_v<Err, G>)
    requires(_base::template _can_move_convert<U, G>::value)
      : _base(::std::move(s))
  {
  }
  template <class G>
  constexpr explicit(not ::std::is_convertible_v<G const &, Err>) expected(::pfn::unexpected<G> const &g) //
      noexcept(::std::is_nothrow_constructible_v<Err, G const &>)
    requires(::std::is_constructible_v<Err, G const &>)
      : _base(::pfn::unexpect, ::std::forward<G const &>(g.error()))
  {
  }
  template <class G>
  constexpr explicit(not ::std::is_convertible_v<G, Err>) expected(::pfn::unexpected<G> &&g) //
      noexcept(::std::is_nothrow_constructible_v<Err, G>)
    requires(::std::is_constructible_v<Err, G>)
      : _base(::pfn::unexpect, ::std::forward<G>(g.error()))
  {
  }

  constexpr explicit expected(::std::in_place_t) noexcept : _base(::std::in_place) {}

  template <class... Args>
  constexpr explicit expected(::pfn::unexpect_t, Args &&...a)   //
      noexcept(::std::is_nothrow_constructible_v<Err, Args...>) //
    requires ::std::is_constructible_v<Err, Args...>
      : _base(::pfn::unexpect, FWD(a)...)
  {
  }
  template <class U, class... Args>
  constexpr explicit expected(::pfn::unexpect_t, ::std::initializer_list<U> il, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<Err, ::std::initializer_list<U> &, Args...>)
    requires ::std::is_constructible_v<Err, ::std::initializer_list<U> &, Args...>
      : _base(::pfn::unexpect, il, FWD(a)...)
  {
  }

  constexpr expected(expected const &) = delete;
  constexpr expected(expected const &)
    requires(::std::is_copy_constructible_v<Err> && ::std::is_trivially_copy_constructible_v<Err>)
  = default;
  constexpr expected(expected const &s) //
      noexcept(::std::is_nothrow_copy_constructible_v<Err>)
    requires(::std::is_copy_constructible_v<Err> && not ::std::is_trivially_copy_constructible_v<Err>)
      : _base(s.set_, FWD(s).storage_)
  {
  }
  constexpr expected(expected &&s)
    requires(::std::is_move_constructible_v<Err> && ::std::is_trivially_move_constructible_v<Err>)
  = default;
  constexpr expected(expected &&s) //
      noexcept(::std::is_nothrow_move_constructible_v<Err>)
    requires(::std::is_move_constructible_v<Err> && not ::std::is_trivially_move_constructible_v<Err>)
      : _base(s.set_, FWD(s).storage_)
  {
  }

  constexpr ~expected() = default;

  template <class G>
  constexpr expected &operator=(::pfn::unexpected<G> const &s) //
      noexcept(::std::is_nothrow_assignable_v<Err &, G const &> && ::std::is_nothrow_constructible_v<Err, G const &>)
    requires(::std::is_constructible_v<Err, G const &> && ::std::is_assignable_v<Err &, G const &>)
  {
    this->_assign_unexpected(s);
    return *this;
  }
  template <class G>
  constexpr expected &operator=(::pfn::unexpected<G> &&s) //
      noexcept(::std::is_nothrow_assignable_v<Err &, G> && ::std::is_nothrow_constructible_v<Err, G>)
    requires(::std::is_constructible_v<Err, G> && ::std::is_assignable_v<Err &, G>)
  {
    this->_assign_unexpected(::std::move(s));
    return *this;
  }
  constexpr expected &operator=(expected const &) = delete;
  constexpr expected &operator=(expected const &s) //
      noexcept(::std::is_nothrow_copy_assignable_v<Err> && ::std::is_nothrow_copy_constructible_v<Err>)
    requires(::std::is_copy_assignable_v<Err> && ::std::is_copy_constructible_v<Err>)
  {
    this->_assign(static_cast<_base const &>(s));
    return *this;
  }
  constexpr expected &operator=(expected &&s) //
      noexcept(::std::is_nothrow_move_assignable_v<Err> && ::std::is_nothrow_move_constructible_v<Err>)
    requires(::std::is_move_constructible_v<Err> && ::std::is_move_assignable_v<Err>)
  {
    this->_assign(static_cast<_base &&>(s));
    return *this;
  }

  using _base::emplace;

  constexpr void swap(expected &rhs) //
      noexcept(::std::is_nothrow_move_constructible_v<Err> && ::std::is_nothrow_swappable_v<Err>)
    requires(::std::is_swappable_v<Err> && ::std::is_move_constructible_v<Err>)
  {
    this->_swap_with(rhs);
  }

  using _base::operator*;
  using _base::operator bool;
  using _base::error;
  using _base::error_or;
  using _base::has_error;
  using _base::has_value;
  using _base::value;

  // Monadic operations. Bodies delegate to _storage static helpers, which perform sum-widening.
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

  template <class F>
  constexpr auto transform_error(F &&f) &                        //
      noexcept(noexcept(_base::_transform_error(*this, FWD(f)))) // extension
      -> decltype(_base::_transform_error(*this, FWD(f)))
  {
    return _base::_transform_error(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform_error(F &&f) &&                                    //
      noexcept(noexcept(_base::_transform_error(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_transform_error(::std::move(*this), FWD(f)))
  {
    return _base::_transform_error(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto transform_error(F &&f) const &                  //
      noexcept(noexcept(_base::_transform_error(*this, FWD(f)))) // extension
      -> decltype(_base::_transform_error(*this, FWD(f)))
  {
    return _base::_transform_error(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform_error(F &&f) const &&                              //
      noexcept(noexcept(_base::_transform_error(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_transform_error(::std::move(*this), FWD(f)))
  {
    return _base::_transform_error(::std::move(*this), FWD(f));
  }
};
// Lifts for sum transformation functions
[[nodiscard]] constexpr auto sum_value(some_expected auto &&src) -> decltype(auto) { return FWD(src).sum_value(); }
[[nodiscard]] constexpr auto sum_error(some_expected auto &&src) -> decltype(auto) { return FWD(src).sum_error(); }

// When any of the sides is expected<void, ...>, we do not produce expected<pack<...>, ...>
// Instead just elide void and carry non-void (or elide both voids if that's what we get)
template <typename Lh, typename Rh>
  requires some_expected_void<Lh> && (not some_expected_void<Rh>)
           && std::is_same_v<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using error_type = std::remove_cvref_t<Lh>::error_type;
  using value_type = std::remove_cvref_t<Rh>::value_type;
  using type = expected<value_type, error_type>;
  if (lh.has_value() && rh.has_value())
    return type{std::in_place, FWD(rh).value()};
  else if (not lh.has_value())
    return type{::pfn::unexpect, FWD(lh).error()};
  else
    return type{::pfn::unexpect, FWD(rh).error()};
}

template <typename Lh, typename Rh>
  requires some_expected_void<Lh> && (not some_expected_void<Rh>)
           && (not std::is_same_v<typename std::remove_cvref_t<Lh>::error_type,
                                  typename std::remove_cvref_t<Rh>::error_type>)
           && (some_sum<typename std::remove_cvref_t<Lh>::error_type>
               || some_sum<typename std::remove_cvref_t<Rh>::error_type>)
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using new_error_type
      = sum_for<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>;
  using value_type = std::remove_cvref_t<Rh>::value_type;
  using type = expected<value_type, new_error_type>;
  if (lh.has_value() && rh.has_value())
    return type{std::in_place, FWD(rh).value()};
  else if (not lh.has_value()) {
    if constexpr (not std::is_same_v<typename std::remove_cvref_t<Lh>::error_type, sum<>>)
      return type{::pfn::unexpect, new_error_type{FWD(lh).error()}};
    else
      std::unreachable();
  } else {
    if constexpr (not std::is_same_v<typename std::remove_cvref_t<Rh>::error_type, sum<>>)
      return type{::pfn::unexpect, new_error_type{FWD(rh).error()}};
    else
      std::unreachable();
  }
}

template <typename Lh, typename Rh>
  requires(not some_expected_void<Lh>) && some_expected_void<Rh>
          && std::is_same_v<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using error_type = std::remove_cvref_t<Lh>::error_type;
  using value_type = std::remove_cvref_t<Lh>::value_type;
  using type = expected<value_type, error_type>;
  if (lh.has_value() && rh.has_value())
    return type{std::in_place, FWD(lh).value()};
  else if (not lh.has_value())
    return type{::pfn::unexpect, FWD(lh).error()};
  else
    return type{::pfn::unexpect, FWD(rh).error()};
}

template <typename Lh, typename Rh>
  requires(not some_expected_void<Lh>) && some_expected_void<Rh>
          && (not std::is_same_v<typename std::remove_cvref_t<Lh>::error_type,
                                 typename std::remove_cvref_t<Rh>::error_type>)
          && (some_sum<typename std::remove_cvref_t<Lh>::error_type>
              || some_sum<typename std::remove_cvref_t<Rh>::error_type>)
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using new_error_type
      = sum_for<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>;
  using value_type = std::remove_cvref_t<Lh>::value_type;
  using type = expected<value_type, new_error_type>;
  if (lh.has_value() && rh.has_value())
    return type{std::in_place, FWD(lh).value()};
  else if (not lh.has_value()) {
    if constexpr (not std::is_same_v<typename std::remove_cvref_t<Lh>::error_type, sum<>>)
      return type{::pfn::unexpect, new_error_type{FWD(lh).error()}};
    else
      std::unreachable();
  } else {
    if constexpr (not std::is_same_v<typename std::remove_cvref_t<Rh>::error_type, sum<>>)
      return type{::pfn::unexpect, new_error_type{FWD(rh).error()}};
    else
      std::unreachable();
  }
}

template <typename Lh, typename Rh>
  requires some_expected_void<Lh> && some_expected_void<Rh>
           && std::is_same_v<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using error_type = std::remove_cvref_t<Lh>::error_type;
  using type = expected<void, error_type>;
  if (lh.has_value() && rh.has_value())
    return type{std::in_place};
  else if (not lh.has_value())
    return type{::pfn::unexpect, FWD(lh).error()};
  else
    return type{::pfn::unexpect, FWD(rh).error()};
}

template <typename Lh, typename Rh>
  requires some_expected_void<Lh> && some_expected_void<Rh>
           && (not std::is_same_v<typename std::remove_cvref_t<Lh>::error_type,
                                  typename std::remove_cvref_t<Rh>::error_type>)
           && (some_sum<typename std::remove_cvref_t<Lh>::error_type>
               || some_sum<typename std::remove_cvref_t<Rh>::error_type>)
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using new_error_type
      = sum_for<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>;
  using type = expected<void, new_error_type>;
  if (lh.has_value() && rh.has_value())
    return type{std::in_place};
  else if (not lh.has_value()) {
    if constexpr (not std::is_same_v<typename std::remove_cvref_t<Lh>::error_type, sum<>>)
      return type{::pfn::unexpect, new_error_type{FWD(lh).error()}};
    else
      std::unreachable();
  } else {
    if constexpr (not std::is_same_v<typename std::remove_cvref_t<Rh>::error_type, sum<>>)
      return type{::pfn::unexpect, new_error_type{FWD(rh).error()}};
    else
      std::unreachable();
  }
}

// Overloads when both sides are non-void, producing either of
// expected<pack<...>, ...> or expected<sum<pack<...>, pack...>, ...>
namespace detail {
template <typename E> struct _expected_type {
  template <typename T> using type = ::fn::expected<T, E>;
};
} // namespace detail

template <typename Lh, typename Rh>
  requires(not some_expected_void<Lh>) && (not some_expected_void<Rh>)
          && std::is_same_v<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using error_type = std::remove_cvref_t<Lh>::error_type;
  static constexpr auto efn = [](auto &&v) { return ::pfn::unexpected<error_type>(FWD(v).error()); };
  return ::fn::detail::_join<detail::template _expected_type<error_type>::template type>(FWD(lh), FWD(rh), efn);
}

template <typename Lh, typename Rh>
  requires(not some_expected_void<Lh>) && (not some_expected_void<Rh>)
          && (not std::is_same_v<typename std::remove_cvref_t<Lh>::error_type,
                                 typename std::remove_cvref_t<Rh>::error_type>)
          && (some_sum<typename std::remove_cvref_t<Lh>::error_type>
              || some_sum<typename std::remove_cvref_t<Rh>::error_type>)
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) noexcept
{
  using new_error_type
      = sum_for<typename std::remove_cvref_t<Lh>::error_type, typename std::remove_cvref_t<Rh>::error_type>;
  static constexpr auto efn = [](auto &&v) {
    if constexpr (not std::is_same_v<typename std::remove_cvref_t<decltype(v)>::error_type, sum<>>) {
      return ::pfn::unexpected<new_error_type>(FWD(v).error());
    } else {
      std::unreachable();
    }
  };
  return ::fn::detail::_join<detail::template _expected_type<new_error_type>::template type>(FWD(lh), FWD(rh), efn);
}

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_EXPECTED
