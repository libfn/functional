// Copyright (c) 2025 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_PFN_EXPECTED
#define INCLUDE_PFN_EXPECTED

#include <cassert>
#include <concepts>
#include <exception>
#include <functional>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>

#ifdef FWD
#pragma push_macro("FWD")
#define INCLUDE_PFN_EXPECTED__POP_FWD
#undef FWD
#endif

// Also defined in fn/detail/fwd_macro.hpp but pfn/* headers are standalone
#define FWD(...) static_cast<decltype(__VA_ARGS__) &&>(__VA_ARGS__)

#ifdef ASSERT
#pragma push_macro("ASSERT")
#define INCLUDE_PFN_EXPECTED__POP_ASSERT
#undef ASSERT
#endif

// LIBFN_ASSERT is a customization point for the user
#ifdef LIBFN_ASSERT
#define ASSERT(...) LIBFN_ASSERT(__VA_ARGS__)
#else
#define ASSERT(...) assert((__VA_ARGS__) == true)
#endif

namespace pfn {

template <class E> class bad_expected_access;

template <> class bad_expected_access<void> : public ::std::exception {
protected:
  bad_expected_access() noexcept = default;
  bad_expected_access(bad_expected_access const &) noexcept = default;
  bad_expected_access(bad_expected_access &&) noexcept = default;
  bad_expected_access &operator=(bad_expected_access const &) noexcept = default;
  bad_expected_access &operator=(bad_expected_access &&) noexcept = default;
  ~bad_expected_access() noexcept = default;

public:
  [[nodiscard]] char const *what() const noexcept override
  {
    static char const msg_[] = "bad access to expected without expected value";
    return msg_;
  }
};

template <class E> class bad_expected_access : public bad_expected_access<void> {
public:
  explicit bad_expected_access(E e) : e_(std::move(e)) {}
  [[nodiscard]] char const *what() const noexcept override { return bad_expected_access<void>::what(); };
  E &error() & noexcept { return e_; }
  E const &error() const & noexcept { return e_; }
  E &&error() && noexcept { return ::std::move(e_); }
  E const &&error() const && noexcept { return ::std::move(e_); }

private:
  E e_;
};

struct unexpect_t {
  explicit unexpect_t() = default;
};
constexpr inline unexpect_t unexpect{};

template <class E> class unexpected;

namespace detail {
template <typename> constexpr bool _is_some_unexpected = false;
template <typename T> constexpr bool _is_some_unexpected<::pfn::unexpected<T>> = true;

template <typename T>
constexpr bool _is_valid_unexpected = //
    ::std::is_object_v<T>             // i.e. not a reference or void or function
    && not ::std::is_array_v<T>       //
    && not _is_some_unexpected<T>     //
    && not ::std::is_const_v<T>       //
    && not ::std::is_volatile_v<T>;
} // namespace detail

template <class E> class unexpected {
  static_assert(detail::_is_valid_unexpected<E>);

public:
  constexpr unexpected(unexpected const &) = default;
  constexpr unexpected(unexpected &&) = default;

  template <class Err = E>
  constexpr explicit unexpected(Err &&e) noexcept(::std::is_nothrow_constructible_v<E, Err>)
    requires(not ::std::is_same_v<::std::remove_cvref_t<Err>, unexpected> &&        //
             not ::std::is_same_v<::std::remove_cvref_t<Err>, ::std::in_place_t> && //
             ::std::is_constructible_v<E, Err>)
      : e_(FWD(e))
  {
  }

  template <class... Args>
  constexpr explicit unexpected(::std::in_place_t, Args &&...a) noexcept(::std::is_nothrow_constructible_v<E, Args...>)
    requires ::std::is_constructible_v<E, Args...>
      : e_(FWD(a)...)
  {
  }

  template <class U, class... Args>
  constexpr explicit unexpected(::std::in_place_t, ::std::initializer_list<U> i, Args &&...a) noexcept(
      ::std::is_nothrow_constructible_v<E, ::std::initializer_list<U> &, Args...>)
    requires ::std::is_constructible_v<E, ::std::initializer_list<U> &, Args...>
      : e_(i, FWD(a)...)
  {
  }

  constexpr unexpected &operator=(unexpected const &) = default;
  constexpr unexpected &operator=(unexpected &&) = default;

  constexpr E const &error() const & noexcept { return e_; };
  constexpr E &error() & noexcept { return e_; };
  constexpr E const &&error() const && noexcept { return ::std::move(e_); };
  constexpr E &&error() && noexcept { return ::std::move(e_); };

  constexpr void swap(unexpected &other) noexcept(::std::is_nothrow_swappable_v<E>)
  {
    static_assert(::std::is_swappable_v<E>);
    using ::std::swap;
    swap(e_, other.e_);
  }

  template <class E2> constexpr friend bool operator==(unexpected const &x, unexpected<E2> const &y)
  {
    return x.e_ == y.e_;
  }

  constexpr friend void swap(unexpected &x, unexpected &y) noexcept(noexcept(x.swap(y)))
    requires ::std::is_swappable_v<E>
  {
    x.swap(y);
  }

private:
  E e_;
};

template <class E> unexpected(E) -> unexpected<E>;

template <class T, class E> class expected;
namespace detail {
template <typename> constexpr bool _is_some_expected = false;
template <typename T, typename E> constexpr bool _is_some_expected<::pfn::expected<T, E>> = true;

template <typename T, typename E>
constexpr bool _is_valid_expected =                                   //
    not ::std::is_reference_v<T>                                      //
    && not ::std::is_function_v<T>                                    //
    && not ::std::is_same_v<::std::remove_cv_t<T>, ::std::in_place_t> //
    && not ::std::is_same_v<::std::remove_cv_t<T>, unexpect_t>        //
    && not _is_some_unexpected<::std::remove_cv_t<T>>                 //
    && detail::_is_valid_unexpected<E>;
} // namespace detail

// declare void specialization
template <class T, class E>
  requires ::std::is_void_v<T>
class expected<T, E>;

template <class T, class E> class expected {
  static_assert(detail::_is_valid_expected<T, E>);

  template <class U, class G, class UF, class GF>
  using _can_convert_detail = ::std::bool_constant<                           //
      not(::std::is_same_v<T, U> && ::std::is_same_v<E, G>)                   //
      && ::std::is_constructible_v<T, UF>                                     //
      && ::std::is_constructible_v<E, GF>                                     //
      && not ::std::is_constructible_v<unexpected<E>, expected<U, G> &>       //
      && not ::std::is_constructible_v<unexpected<E>, expected<U, G>>         //
      && not ::std::is_constructible_v<unexpected<E>, expected<U, G> const &> //
      && not ::std::is_constructible_v<unexpected<E>, expected<U, G> const>   //
      && (::std::is_same_v<bool, ::std::remove_cv_t<T>>                       //
          || (                                                                //
              not ::std::is_constructible_v<T, expected<U, G> &>              //
              && not ::std::is_constructible_v<T, expected<U, G>>             //
              && not ::std::is_constructible_v<T, expected<U, G> const &>     //
              && not ::std::is_constructible_v<T, expected<U, G> const>       //
              && not ::std::is_convertible_v<expected<U, G> &, T>             //
              && not ::std::is_convertible_v<expected<U, G> &&, T>            //
              && not ::std::is_convertible_v<expected<U, G> const &, T>       //
              && not ::std::is_convertible_v<expected<U, G> const &&, T>))>;

  template <class U, class G> using _can_copy_convert = _can_convert_detail<U, G, U const &, G const &>;
  template <class U, class G> using _can_move_convert = _can_convert_detail<U, G, U, G>;
  template <class U, class G> friend class expected;

  template <class U>
  using _can_convert = ::std::bool_constant<                            //
      not ::std::is_same_v<::std::remove_cvref_t<U>, ::std::in_place_t> //
      && not ::std::is_same_v<::std::remove_cvref_t<U>, unexpect_t>     // LWG4222
      && not ::std::is_same_v<expected, ::std::remove_cvref_t<U>>       //
      && not detail::_is_some_unexpected<::std::remove_cvref_t<U>>      //
      && ::std::is_constructible_v<T, U>                                //
      && (not ::std::is_same_v<bool, ::std::remove_cv_t<T>>             //
          || not detail::_is_some_expected<::std::remove_cvref_t<U>>)>;

  template <class U>
  using _can_convert_assign = ::std::bool_constant<                //
      not ::std::is_same_v<expected, ::std::remove_cvref_t<U>>     //
      && not detail::_is_some_unexpected<::std::remove_cvref_t<U>> //
      && ::std::is_constructible_v<T, U>                           //
      && ::std::is_assignable_v<T, U>                              //
      && (::std::is_nothrow_constructible_v<T, U>                  //
          || ::std::is_nothrow_move_constructible_v<T>             //
          || ::std::is_nothrow_move_constructible_v<E>)>;

  // [expected.object.assign]
  template <typename New, typename Old, typename... Args>
  static constexpr void _reinit(New &newval, Old &oldval, Args &&...args) //
      noexcept(::std::is_nothrow_constructible_v<New, Args...>)
  {
    if constexpr (::std::is_nothrow_constructible_v<New, Args...>) {
      ::std::destroy_at(::std::addressof(oldval));
      ::std::construct_at(::std::addressof(newval), ::std::forward<Args>(args)...);
    } else if constexpr (::std::is_nothrow_move_constructible_v<New>) {
      New tmp(::std::forward<Args>(args)...);
      ::std::destroy_at(::std::addressof(oldval));
      ::std::construct_at(::std::addressof(newval), std::move(tmp));
    } else {
      Old tmp(std::move(oldval));
      ::std::destroy_at(::std::addressof(oldval));
      try {
        ::std::construct_at(::std::addressof(newval), ::std::forward<Args>(args)...);
      } catch (...) {
        ::std::construct_at(::std::addressof(oldval), std::move(tmp));
        throw;
      }
    }
  }

  static constexpr void _swap(expected &lhs, expected &rhs) //
      noexcept(::std::is_nothrow_move_constructible_v<T> && ::std::is_nothrow_move_constructible_v<E>)
  {
    if constexpr (::std::is_nothrow_move_constructible_v<E>) {
      E tmp(::std::move(rhs.e_));
      ::std::destroy_at(::std::addressof(rhs.e_));
      try {
        ::std::construct_at(::std::addressof(rhs.v_), ::std::move(lhs.v_));
        ::std::destroy_at(::std::addressof(lhs.v_));
        ::std::construct_at(::std::addressof(lhs.e_), ::std::move(tmp));
      } catch (...) {
        ::std::construct_at(::std::addressof(rhs.e_), ::std::move(tmp));
        throw;
      }
    } else {
      T tmp(::std::move(lhs.v_));
      ::std::destroy_at(::std::addressof(lhs.v_));
      try {
        ::std::construct_at(::std::addressof(lhs.e_), ::std::move(rhs.e_));
        ::std::destroy_at(::std::addressof(rhs.e_));
        ::std::construct_at(::std::addressof(rhs.v_), ::std::move(tmp));
      } catch (...) {
        ::std::construct_at(::std::addressof(lhs.v_), ::std::move(tmp));
        throw;
      }
    }
    lhs.set_ = false;
    rhs.set_ = true;
  }

  constexpr T const &_value() const & noexcept
  {
    ASSERT(set_);
    return v_;
  }
  constexpr T &_value() & noexcept
  {
    ASSERT(set_);
    return v_;
  }
  constexpr T const &&_value() const && noexcept
  {
    ASSERT(set_);
    return ::std::move(v_);
  }
  constexpr T &&_value() && noexcept
  {
    ASSERT(set_);
    return ::std::move(v_);
  }

  template <typename Self, typename Fn>
  static constexpr auto _and_then(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn, decltype(FWD(self)._value())>
               && ::std::is_nothrow_constructible_v<E, decltype(FWD(self).error())>)
    requires(::std::is_invocable_v<Fn, decltype(FWD(self)._value())>
             && ::std::is_constructible_v<E, decltype(FWD(self).error())>)
  {
    using result_t = ::std::remove_cvref_t<::std::invoke_result_t<Fn, decltype(FWD(self)._value())>>;
    static_assert(detail::_is_some_expected<result_t>);
    static_assert(::std::is_same_v<typename result_t::error_type, typename ::std::remove_cvref_t<Self>::error_type>);
    if (self.has_value()) {
      return ::std::invoke(FWD(fn), FWD(self)._value());
    }
    return result_t(unexpect, FWD(self).error());
  }

  template <typename Self, typename Fn>
  static constexpr auto _or_else(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn, decltype(FWD(self).error())>
               && ::std::is_nothrow_constructible_v<T, decltype(FWD(self)._value())>)
    requires(::std::is_invocable_v<Fn, decltype(FWD(self).error())>
             && ::std::is_constructible_v<T, decltype(FWD(self)._value())>)
  {
    using result_t = ::std::remove_cvref_t<::std::invoke_result_t<Fn, decltype(FWD(self).error())>>;
    static_assert(detail::_is_some_expected<result_t>);
    static_assert(::std::is_same_v<typename result_t::value_type, typename ::std::remove_cvref_t<Self>::value_type>);
    if (self.has_value()) {
      return result_t(::std::in_place, FWD(self)._value());
    }
    return ::std::invoke(FWD(fn), FWD(self).error());
  }

  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn, decltype(FWD(self)._value())>
               && ::std::is_nothrow_constructible_v<E, decltype(FWD(self).error())>
               && (::std::is_void_v<::std::invoke_result_t<Fn, decltype(FWD(self)._value())>>
                   || ::std::is_nothrow_constructible_v<
                       ::std::remove_cv_t<::std::invoke_result_t<Fn, decltype(FWD(self)._value())>>,
                       ::std::invoke_result_t<Fn, decltype(FWD(self)._value())>>))
    requires(::std::is_invocable_v<Fn, decltype(FWD(self)._value())>
             && ::std::is_constructible_v<E, decltype(FWD(self).error())>)
  {
    using value_t = ::std::remove_cv_t<::std::invoke_result_t<Fn, decltype(FWD(self)._value())>>;
    static_assert(detail::_is_valid_expected<value_t, E>);
    using result_t = expected<value_t, E>;
    if (self.has_value()) {
      if constexpr (not ::std::is_void_v<value_t>) {
        static_assert(::std::is_constructible_v<value_t, ::std::invoke_result_t<Fn, decltype(FWD(self)._value())>>);
        return result_t(::std::in_place, ::std::invoke(FWD(fn), FWD(self)._value()));
      } else {
        ::std::invoke(FWD(fn), FWD(self)._value());
        return result_t(::std::in_place);
      }
    }
    return result_t(unexpect, FWD(self).error());
  }

  template <typename Self, typename Fn>
  static constexpr auto _transform_error(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn, decltype(FWD(self).error())>
               && ::std::is_nothrow_constructible_v<T, decltype(FWD(self)._value())>
               && ::std::is_nothrow_constructible_v<
                   ::std::remove_cv_t<::std::invoke_result_t<Fn, decltype(FWD(self).error())>>,
                   ::std::invoke_result_t<Fn, decltype(FWD(self).error())>>)
    requires(::std::is_invocable_v<Fn, decltype(FWD(self).error())>
             && ::std::is_constructible_v<T, decltype(FWD(self)._value())>)
  {
    using error_t = ::std::remove_cv_t<::std::invoke_result_t<Fn, decltype(FWD(self).error())>>;
    static_assert(detail::_is_valid_unexpected<error_t>);
    static_assert(::std::is_constructible_v<error_t, ::std::invoke_result_t<Fn, decltype(FWD(self).error())>>);
    using result_t = expected<T, error_t>;
    if (not self.has_value()) {
      return result_t(unexpect, ::std::invoke(FWD(fn), FWD(self).error()));
    }
    return result_t(::std::in_place, FWD(self)._value());
  }

public:
  using value_type = T;
  using error_type = E;
  using unexpected_type = unexpected<E>;

  template <class U> using rebind = expected<U, error_type>;

  // [expected.object.cons], constructors
  constexpr expected()                                       //
      noexcept(::std::is_nothrow_default_constructible_v<T>) // extension
    requires ::std::is_default_constructible_v<T>
      : v_(), set_(true)
  {
  }

  constexpr expected(expected const &) = delete;
  constexpr expected(expected const &s)                                                                //
      noexcept(::std::is_nothrow_copy_constructible_v<T> && ::std::is_nothrow_copy_constructible_v<E>) // extension
    requires(::std::is_copy_constructible_v<T> && ::std::is_copy_constructible_v<E>
             && ::std::is_trivially_copy_constructible_v<T> && ::std::is_trivially_copy_constructible_v<E>)
  = default;
  constexpr expected(expected const &s)                                                                //
      noexcept(::std::is_nothrow_copy_constructible_v<T> && ::std::is_nothrow_copy_constructible_v<E>) // extension
    requires(::std::is_copy_constructible_v<T> && ::std::is_copy_constructible_v<E>
             && (not ::std::is_trivially_copy_constructible_v<T> || not ::std::is_trivially_copy_constructible_v<E>))
      : set_(s.set_)
  {
    if (set_)
      ::std::construct_at(::std::addressof(v_), s.v_);
    else
      ::std::construct_at(::std::addressof(e_), s.e_);
  }

  constexpr expected(expected &&s)
    requires(::std::is_move_constructible_v<T> && ::std::is_move_constructible_v<E>
             && ::std::is_trivially_move_constructible_v<T> && ::std::is_trivially_move_constructible_v<E>)
  = default;
  constexpr expected(expected &&s)                                                                     //
      noexcept(::std::is_nothrow_move_constructible_v<T> && ::std::is_nothrow_move_constructible_v<E>) // required
    requires(::std::is_move_constructible_v<T> && ::std::is_move_constructible_v<E>
             && (not ::std::is_trivially_move_constructible_v<T> || not ::std::is_trivially_move_constructible_v<E>))
      : set_(s.set_)
  {
    if (set_)
      ::std::construct_at(::std::addressof(v_), ::std::move(s.v_));
    else
      ::std::construct_at(::std::addressof(e_), ::std::move(s.e_));
  }

  template <class U, class G>
  constexpr explicit(not ::std::is_convertible_v<U const &, T> || not ::std::is_convertible_v<G const &, E>)
      expected(expected<U, G> const &s) //
      noexcept(::std::is_nothrow_constructible_v<T, U const &>
               && ::std::is_nothrow_constructible_v<E, G const &>) // extension
    requires(_can_copy_convert<U, G>::value)
      : set_(s.set_)
  {
    if (set_)
      ::std::construct_at(::std::addressof(v_), s.v_);
    else
      ::std::construct_at(::std::addressof(e_), s.e_);
  }

  template <class U, class G>
  constexpr explicit(not ::std::is_convertible_v<U, T> || not ::std::is_convertible_v<G, E>)
      expected(expected<U, G> &&s)                                                                 //
      noexcept(::std::is_nothrow_constructible_v<T, U> && ::std::is_nothrow_constructible_v<E, G>) // extension
    requires(_can_move_convert<U, G>::value)
      : set_(s.set_)
  {
    if (set_)
      ::std::construct_at(::std::addressof(v_), ::std::move(s.v_));
    else
      ::std::construct_at(::std::addressof(e_), ::std::move(s.e_));
  }

  template <class U = ::std::remove_cv_t<T>>
  constexpr explicit(not ::std::is_convertible_v<U, T>) expected(U &&v) //
      noexcept(::std::is_nothrow_constructible_v<T, U>)                 // extension
    requires(_can_convert<U>::value)
      : v_(FWD(v)), set_(true)
  {
  }

  template <class G>
  constexpr explicit(!::std::is_convertible_v<G const &, E>) expected(unexpected<G> const &g) //
      noexcept(::std::is_nothrow_constructible_v<E, G const &>)                               // extension
    requires(::std::is_constructible_v<E, G const &>)
      : e_(::std::forward<G const &>(g.error())), set_(false)
  {
  }

  template <class G>
  constexpr explicit(!::std::is_convertible_v<G, E>) expected(unexpected<G> &&g) //
      noexcept(::std::is_nothrow_constructible_v<E, G>)                          // extension
    requires(::std::is_constructible_v<E, G>)
      : e_(::std::forward<G>(g.error())), set_(false)
  {
  }

  template <class... Args>
  constexpr explicit expected(::std::in_place_t, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<T, Args...>) // extension
    requires ::std::is_constructible_v<T, Args...>
      : v_(FWD(a)...), set_(true)
  {
  }
  template <class U, class... Args>
  constexpr explicit expected(::std::in_place_t, ::std::initializer_list<U> il, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<T, ::std::initializer_list<U> &, Args...>)  // extension
    requires ::std::is_constructible_v<T, ::std::initializer_list<U> &, Args...>
      : v_(il, FWD(a)...), set_(true)
  {
  }
  template <class... Args>
  constexpr explicit expected(unexpect_t, Args &&...a)        //
      noexcept(::std::is_nothrow_constructible_v<E, Args...>) // extension
    requires ::std::is_constructible_v<E, Args...>
      : e_(FWD(a)...), set_(false)
  {
  }
  template <class U, class... Args>
  constexpr explicit expected(unexpect_t, ::std::initializer_list<U> il, Args &&...a)       //
      noexcept(::std::is_nothrow_constructible_v<E, ::std::initializer_list<U> &, Args...>) // extension
    requires ::std::is_constructible_v<E, ::std::initializer_list<U> &, Args...>
      : e_(il, FWD(a)...), set_(false)
  {
  }

  // [expected.object.dtor], destructor
  constexpr ~expected() noexcept
    requires(::std::is_trivially_destructible_v<T> && ::std::is_trivially_destructible_v<E>)
  = default;
  constexpr ~expected() //
    requires(::std::is_trivially_destructible_v<T> && not ::std::is_trivially_destructible_v<E>)
  {
    if (not set_)
      ::std::destroy_at(::std::addressof(e_));
    // else T is trivially destructible, no need to do anything
  }
  constexpr ~expected() //
    requires(not ::std::is_trivially_destructible_v<T> && ::std::is_trivially_destructible_v<E>)
  {
    if (set_)
      ::std::destroy_at(::std::addressof(v_));
    // else E is trivially destructible, no need to do anything
  }
  constexpr ~expected() //
    requires(not ::std::is_trivially_destructible_v<T> && not ::std::is_trivially_destructible_v<E>)
  {
    if (set_)
      ::std::destroy_at(::std::addressof(v_));
    else
      ::std::destroy_at(::std::addressof(e_));
  }

  // [expected.object.assign], assignment
  constexpr expected &operator=(expected const &) = delete;
  constexpr expected &operator=(expected const &s) //
      noexcept(::std::is_nothrow_copy_assignable_v<T> && ::std::is_nothrow_copy_constructible_v<T>
               && ::std::is_nothrow_copy_assignable_v<E> && ::std::is_nothrow_copy_constructible_v<E>) // extension
    requires(::std::is_copy_assignable_v<T> && ::std::is_copy_constructible_v<T> && ::std::is_copy_assignable_v<E>
             && ::std::is_copy_constructible_v<E>
             && (::std::is_nothrow_move_constructible_v<T> || ::std::is_nothrow_move_constructible_v<E>))
  {
    if (set_ && s.set_) {
      v_ = s.v_;
    } else if (set_) {
      _reinit(e_, v_, s.e_);
      set_ = false;
    } else if (s.set_) {
      _reinit(v_, e_, s.v_);
      set_ = true;
    } else {
      e_ = s.e_;
    }
    return *this;
  }

  constexpr expected &operator=(expected &&s) //
      noexcept(::std::is_nothrow_move_assignable_v<T> && ::std::is_nothrow_move_constructible_v<T>
               && ::std::is_nothrow_move_assignable_v<E> && ::std::is_nothrow_move_constructible_v<E>) // required
    requires(::std::is_move_constructible_v<T> && ::std::is_move_assignable_v<T> && ::std::is_move_constructible_v<E>
             && ::std::is_move_assignable_v<E>
             && (::std::is_nothrow_move_constructible_v<T> || ::std::is_nothrow_move_constructible_v<E>))
  {
    if (set_ && s.set_) {
      v_ = ::std::move(s.v_);
    } else if (set_) {
      _reinit(e_, v_, ::std::move(s.e_));
      set_ = false;
    } else if (s.set_) {
      _reinit(v_, e_, ::std::move(s.v_));
      set_ = true;
    } else {
      e_ = ::std::move(s.e_);
    }
    return *this;
  }

  template <class U = T>
  constexpr expected &operator=(U &&s) //
      noexcept(::std::is_nothrow_assignable_v<T, U> && ::std::is_nothrow_constructible_v<T, U>
               && (::std::is_nothrow_move_constructible_v<T> || ::std::is_nothrow_move_constructible_v<E>)) // extension
    requires(_can_convert_assign<U>::value)
  {
    if (set_) {
      v_ = FWD(s);
    } else {
      _reinit(v_, e_, FWD(s));
      set_ = true;
    }
    return *this;
  }

  template <class G>
  constexpr expected &operator=(unexpected<G> const &s) //
      noexcept(::std::is_nothrow_assignable_v<E, G const &> && ::std::is_nothrow_constructible_v<E, G const &>
               && (::std::is_nothrow_move_constructible_v<E> || ::std::is_nothrow_move_constructible_v<T>)) // extension
    requires(::std::is_constructible_v<E, G const &> && ::std::is_assignable_v<E &, G const &>
             && (::std::is_nothrow_constructible_v<E, G const &> || ::std::is_nothrow_move_constructible_v<T>
                 || ::std::is_nothrow_move_constructible_v<E>))
  {
    if (not set_) {
      e_ = ::std::forward<G const &>(s.error());
    } else {
      _reinit(e_, v_, ::std::forward<G const &>(s.error()));
      set_ = false;
    }
    return *this;
  }

  template <class G>
  constexpr expected &operator=(unexpected<G> &&s) //
      noexcept(::std::is_nothrow_assignable_v<E, G> && ::std::is_nothrow_constructible_v<E, G>
               && (::std::is_nothrow_move_constructible_v<E> || ::std::is_nothrow_move_constructible_v<T>)) // extension
    requires(::std::is_constructible_v<E, G> && ::std::is_assignable_v<E &, G>
             && (::std::is_nothrow_constructible_v<E, G> || ::std::is_nothrow_move_constructible_v<T>
                 || ::std::is_nothrow_move_constructible_v<E>))
  {
    if (not set_) {
      e_ = ::std::forward<G>(s.error());
    } else {
      _reinit(e_, v_, ::std::forward<G>(s.error()));
      set_ = false;
    }
    return *this;
  }

  template <class... Args>
  constexpr T &emplace(Args &&...args) noexcept
    requires(::std::is_nothrow_constructible_v<T, Args...>)
  {
    if (set_) {
      ::std::destroy_at(::std::addressof(v_));
    } else {
      ::std::destroy_at(::std::addressof(e_));
      set_ = true;
    }
    return *::std::construct_at(::std::addressof(v_), std::forward<Args>(args)...);
  }

  template <class U, class... Args>
  constexpr T &emplace(::std::initializer_list<U> il, Args &&...args) noexcept
    requires(::std::is_nothrow_constructible_v<T, ::std::initializer_list<U> &, Args...>)
  {
    if (set_) {
      ::std::destroy_at(::std::addressof(v_));
    } else {
      ::std::destroy_at(::std::addressof(e_));
      set_ = true;
    }
    return *::std::construct_at(::std::addressof(v_), il, std::forward<Args>(args)...);
  }

  // [expected.object.swap], swap
  constexpr void
  swap(expected &rhs) noexcept(::std::is_nothrow_move_constructible_v<T> && ::std::is_nothrow_swappable_v<T>
                               && ::std::is_nothrow_move_constructible_v<E> && ::std::is_nothrow_swappable_v<E>)
    requires(::std::is_swappable_v<T> && ::std::is_swappable_v<E> && ::std::is_move_constructible_v<T>
             && ::std::is_move_constructible_v<E>
             && (::std::is_nothrow_move_constructible_v<T> || ::std::is_nothrow_move_constructible_v<E>))
  {
    bool const lhset = has_value();
    bool const rhset = rhs.has_value();
    if (lhset == rhset) {
      if (lhset) {
        using ::std::swap;
        swap(v_, rhs.v_);
      } else {
        using ::std::swap;
        swap(e_, rhs.e_);
      }
    } else {
      if (lhset) {
        _swap(*this, rhs);
      } else {
        _swap(rhs, *this);
      }
    }
  }

  constexpr friend void swap(expected &x, expected &y) noexcept(noexcept(x.swap(y)))
    requires requires { x.swap(y); }
  {
    x.swap(y);
  }

  // [expected.object.obs], observers
  constexpr T const *operator->() const noexcept
  {
    ASSERT(set_);
    return ::std::addressof(v_);
  }
  constexpr T *operator->() noexcept
  {
    ASSERT(set_);
    return ::std::addressof(v_);
  }
  constexpr T const &operator*() const & noexcept { return *(this->operator->()); }
  constexpr T &operator*() & noexcept { return *(this->operator->()); }
  constexpr T const &&operator*() const && noexcept { return ::std::move(*(this->operator->())); }
  constexpr T &&operator*() && noexcept { return ::std::move(*(this->operator->())); }
  constexpr explicit operator bool() const noexcept { return set_; }
  constexpr bool has_value() const noexcept { return set_; }
  constexpr bool has_error() const noexcept { return !set_; }
  constexpr T const &value() const &
  {
    static_assert(::std::is_copy_constructible_v<E>);
    if (not set_)
      throw bad_expected_access<E>(e_);
    return v_;
  }
  constexpr T &value() &
  {
    static_assert(::std::is_copy_constructible_v<E>);
    if (not set_)
      throw bad_expected_access<E>(::std::as_const(e_));
    return v_;
  }
  constexpr T const &&value() const &&
  {
    static_assert(::std::is_copy_constructible_v<E>);
    static_assert(::std::is_constructible_v<E, E const &&>);
    if (not set_)
      throw bad_expected_access<E>(::std::move(e_));
    return ::std::move(v_);
  }
  constexpr T &&value() &&
  {
    static_assert(::std::is_copy_constructible_v<E>);
    static_assert(::std::is_constructible_v<E, E &&>);
    if (not set_)
      throw bad_expected_access<E>(::std::move(e_));
    return ::std::move(v_);
  }
  constexpr E const &error() const & noexcept
  {
    ASSERT(not set_);
    return e_;
  }
  constexpr E &error() & noexcept
  {
    ASSERT(not set_);
    return e_;
  }
  constexpr E const &&error() const && noexcept
  {
    ASSERT(not set_);
    return ::std::move(e_);
  }
  constexpr E &&error() && noexcept
  {
    ASSERT(not set_);
    return ::std::move(e_);
  }

  template <class U>
  constexpr T value_or(U &&v) const &                                                              //
      noexcept(::std::is_nothrow_copy_constructible_v<T> && ::std::is_nothrow_convertible_v<U, T>) // extension
  {
    static_assert(::std::is_copy_constructible_v<T>);
    static_assert(::std::is_convertible_v<U, T>);
    return set_ ? v_ : static_cast<T>(FWD(v));
  }
  template <class U>
  constexpr T value_or(U &&v) &&                                                                   //
      noexcept(::std::is_nothrow_move_constructible_v<T> && ::std::is_nothrow_convertible_v<U, T>) // extension
  {
    static_assert(::std::is_move_constructible_v<T>);
    static_assert(::std::is_convertible_v<U, T>);
    return set_ ? ::std::move(v_) : static_cast<T>(FWD(v));
  }

  template <class G = E>
  constexpr E error_or(G &&e) const &                                                              //
      noexcept(::std::is_nothrow_copy_constructible_v<E> && ::std::is_nothrow_convertible_v<G, E>) // extension
  {
    static_assert(::std::is_copy_constructible_v<E>);
    static_assert(::std::is_convertible_v<G, E>);
    if (set_) {
      return FWD(e);
    }
    return e_;
  }
  template <class G = E>
  constexpr E error_or(G &&e) &&                                                                   //
      noexcept(::std::is_nothrow_move_constructible_v<E> && ::std::is_nothrow_convertible_v<G, E>) // extension
  {
    static_assert(::std::is_move_constructible_v<E>);
    static_assert(::std::is_convertible_v<G, E>);
    if (set_) {
      return FWD(e);
    }
    return ::std::move(e_);
  }

  // [expected.object.monadic], monadic operations
  template <class F>
  constexpr auto and_then(F &&f) &                 //
      noexcept(noexcept(_and_then(*this, FWD(f)))) // extension
      -> decltype(_and_then(*this, FWD(f)))
  {
    return _and_then(*this, FWD(f));
  }
  template <class F>
  constexpr auto and_then(F &&f) &&                             //
      noexcept(noexcept(_and_then(::std::move(*this), FWD(f)))) // extension
      -> decltype(_and_then(::std::move(*this), FWD(f)))
  {
    return _and_then(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto and_then(F &&f) const &           //
      noexcept(noexcept(_and_then(*this, FWD(f)))) // extension
      -> decltype(_and_then(*this, FWD(f)))
  {
    return _and_then(*this, FWD(f));
  }
  template <class F>
  constexpr auto and_then(F &&f) const &&                       //
      noexcept(noexcept(_and_then(::std::move(*this), FWD(f)))) // extension
      -> decltype(_and_then(::std::move(*this), FWD(f)))
  {
    return _and_then(::std::move(*this), FWD(f));
  }

  template <class F>
  constexpr auto or_else(F &&f) &                 //
      noexcept(noexcept(_or_else(*this, FWD(f)))) // extension
      -> decltype(_or_else(*this, FWD(f)))
  {
    return _or_else(*this, FWD(f));
  }
  template <class F>
  constexpr auto or_else(F &&f) &&                             //
      noexcept(noexcept(_or_else(::std::move(*this), FWD(f)))) // extension
      -> decltype(_or_else(::std::move(*this), FWD(f)))
  {
    return _or_else(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto or_else(F &&f) const &           //
      noexcept(noexcept(_or_else(*this, FWD(f)))) // extension
      -> decltype(_or_else(*this, FWD(f)))
  {
    return _or_else(*this, FWD(f));
  }
  template <class F>
  constexpr auto or_else(F &&f) const &&                       //
      noexcept(noexcept(_or_else(::std::move(*this), FWD(f)))) // extension
      -> decltype(_or_else(::std::move(*this), FWD(f)))
  {
    return _or_else(::std::move(*this), FWD(f));
  }

  template <class F>
  constexpr auto transform(F &&f) &                 //
      noexcept(noexcept(_transform(*this, FWD(f)))) // extension
      -> decltype(_transform(*this, FWD(f)))
  {
    return _transform(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform(F &&f) &&                             //
      noexcept(noexcept(_transform(::std::move(*this), FWD(f)))) // extension
      -> decltype(_transform(::std::move(*this), FWD(f)))
  {
    return _transform(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto transform(F &&f) const &           //
      noexcept(noexcept(_transform(*this, FWD(f)))) // extension
      -> decltype(_transform(*this, FWD(f)))
  {
    return _transform(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform(F &&f) const &&                       //
      noexcept(noexcept(_transform(::std::move(*this), FWD(f)))) // extension
      -> decltype(_transform(::std::move(*this), FWD(f)))
  {
    return _transform(::std::move(*this), FWD(f));
  }

  template <class F>
  constexpr auto transform_error(F &&f) &                 //
      noexcept(noexcept(_transform_error(*this, FWD(f)))) // extension
      -> decltype(_transform_error(*this, FWD(f)))
  {
    return _transform_error(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform_error(F &&f) &&                             //
      noexcept(noexcept(_transform_error(::std::move(*this), FWD(f)))) // extension
      -> decltype(_transform_error(::std::move(*this), FWD(f)))
  {
    return _transform_error(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto transform_error(F &&f) const &           //
      noexcept(noexcept(_transform_error(*this, FWD(f)))) // extension
      -> decltype(_transform_error(*this, FWD(f)))
  {
    return _transform_error(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform_error(F &&f) const &&                       //
      noexcept(noexcept(_transform_error(::std::move(*this), FWD(f)))) // extension
      -> decltype(_transform_error(::std::move(*this), FWD(f)))
  {
    return _transform_error(::std::move(*this), FWD(f));
  }

  // [expected.object.eq], equality operators
  template <class T2, class E2>
    requires(not ::std::is_void_v<T2>)
  constexpr friend bool operator==(expected const &x, expected<T2, E2> const &y) //
      noexcept(noexcept(static_cast<bool>(*x == *y)) && noexcept(static_cast<bool>(x.error() == y.error())))           // extension
    requires ( //
requires {
      { *x == *y } -> std::convertible_to<bool>;
    } &&
requires {
    { x.error() == y.error() } -> std::convertible_to<bool>;
    })
  {
    if (x.has_value() != y.has_value()) {
      return false;
    } else if (x.has_value()) {
      return *x == *y;
    } else {
      return x.error() == y.error();
    }
  }
  template <class T2>
    requires(not detail::_is_some_expected<T2>)
  constexpr friend bool operator==(expected const &x, const T2 &v) //
      noexcept(noexcept(static_cast<bool>(*x == v)))               // extension
    requires requires {
      { *x == v } -> std::convertible_to<bool>;
    }
  {
    return x.has_value() && static_cast<bool>(*x == v);
  }
  template <class E2>
  constexpr friend bool operator==(expected const &x, unexpected<E2> const &e) //
      noexcept(noexcept(static_cast<bool>(x.error() == e.error())))            // extension
    requires requires {
      { x.error() == e.error() } -> std::convertible_to<bool>;
    }
  {
    return (not x.has_value()) && static_cast<bool>(x.error() == e.error());
  }

private:
  union {
    T v_;
    E e_;
  };
  bool set_;
};

template <class T, class E>
  requires ::std::is_void_v<T>
class expected<T, E> {
  static_assert(detail::_is_valid_unexpected<E>);

  template <class U, class G, class UF, class GF>
  using _can_convert_detail = ::std::bool_constant<                           //
      ::std::is_void_v<U> && ::std::is_constructible_v<E, GF>                 //
      && not ::std::is_constructible_v<unexpected<E>, expected<U, G> &>       //
      && not ::std::is_constructible_v<unexpected<E>, expected<U, G>>         //
      && not ::std::is_constructible_v<unexpected<E>, expected<U, G> const &> //
      && not ::std::is_constructible_v<unexpected<E>, expected<U, G> const>>;

  template <class U, class G> using _can_copy_convert = _can_convert_detail<U, G, U const &, G const &>;
  template <class U, class G> using _can_move_convert = _can_convert_detail<U, G, U, G>;
  template <class U, class G> friend class expected;

  template <class U>
  using _can_convert = ::std::bool_constant<                            //
      not ::std::is_same_v<::std::remove_cvref_t<U>, ::std::in_place_t> //
      && not ::std::is_same_v<::std::remove_cvref_t<U>, unexpect_t>     // LWG4222
      && not ::std::is_same_v<expected, ::std::remove_cvref_t<U>>       //
      && not detail::_is_some_unexpected<::std::remove_cvref_t<U>>      //
      && ::std::is_constructible_v<T, U>                                //
      && (not ::std::is_same_v<bool, ::std::remove_cv_t<T>>             //
          || not detail::_is_some_expected<::std::remove_cvref_t<U>>)>;

  template <typename Self, typename Fn>
  static constexpr auto _and_then(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn> && ::std::is_nothrow_constructible_v<E, decltype(FWD(self).error())>)
    requires(::std::is_invocable_v<Fn> && ::std::is_constructible_v<E, decltype(FWD(self).error())>)
  {
    using result_t = ::std::remove_cvref_t<::std::invoke_result_t<Fn>>;
    static_assert(detail::_is_some_expected<result_t>);
    static_assert(::std::is_same_v<typename result_t::error_type, typename ::std::remove_cvref_t<Self>::error_type>);
    if (self.has_value()) {
      return ::std::invoke(FWD(fn));
    }
    return result_t(unexpect, FWD(self).error());
  }

  template <typename Self, typename Fn>
  static constexpr auto _or_else(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn, decltype(FWD(self).error())>)
    requires(::std::is_invocable_v<Fn, decltype(FWD(self).error())>)
  {
    using result_t = ::std::remove_cvref_t<::std::invoke_result_t<Fn, decltype(FWD(self).error())>>;
    static_assert(detail::_is_some_expected<result_t>);
    static_assert(::std::is_same_v<typename result_t::value_type, typename ::std::remove_cvref_t<Self>::value_type>);
    if (self.has_value()) {
      return result_t(::std::in_place);
    }
    return ::std::invoke(FWD(fn), FWD(self).error());
  }

  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn> && ::std::is_nothrow_constructible_v<E, decltype(FWD(self).error())>
               && (::std::is_void_v<::std::invoke_result_t<Fn>>
                   || ::std::is_nothrow_constructible_v<::std::remove_cv_t<::std::invoke_result_t<Fn>>,
                                                        ::std::invoke_result_t<Fn>>))
    requires(::std::is_invocable_v<Fn> && ::std::is_constructible_v<E, decltype(FWD(self).error())>)
  {
    using value_t = ::std::remove_cv_t<::std::invoke_result_t<Fn>>;
    static_assert(detail::_is_valid_expected<value_t, E>);
    using result_t = expected<value_t, E>;
    if (self.has_value()) {
      if constexpr (not ::std::is_void_v<value_t>) {
        static_assert(::std::is_constructible_v<value_t, ::std::invoke_result_t<Fn>>);
        return result_t(::std::in_place, ::std::invoke(FWD(fn)));
      } else {
        ::std::invoke(FWD(fn));
        return result_t(::std::in_place);
      }
    }
    return result_t(unexpect, FWD(self).error());
  }

  template <typename Self, typename Fn>
  static constexpr auto _transform_error(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn, decltype(FWD(self).error())>
               && ::std::is_nothrow_constructible_v<
                   ::std::remove_cv_t<::std::invoke_result_t<Fn, decltype(FWD(self).error())>>,
                   ::std::invoke_result_t<Fn, decltype(FWD(self).error())>>)
    requires(::std::is_invocable_v<Fn, decltype(FWD(self).error())>)
  {
    using error_t = ::std::remove_cv_t<::std::invoke_result_t<Fn, decltype(FWD(self).error())>>;
    static_assert(detail::_is_valid_unexpected<error_t>);
    static_assert(::std::is_constructible_v<error_t, ::std::invoke_result_t<Fn, decltype(FWD(self).error())>>);
    using result_t = expected<T, error_t>;
    if (not self.has_value()) {
      return result_t(unexpect, ::std::invoke(FWD(fn), FWD(self).error()));
    }
    return result_t(::std::in_place);
  }

public:
  using value_type = T;
  using error_type = E;
  using unexpected_type = unexpected<E>;

  template <class U> using rebind = expected<U, error_type>;

  // [expected.void.cons], constructors
  constexpr expected() noexcept : d_(), set_(true) {}
  constexpr expected(expected const &) = delete;
  constexpr expected(expected const &)                                                         //
    requires(::std::is_copy_constructible_v<E> && ::std::is_trivially_copy_constructible_v<E>) //
  = default;
  constexpr expected(expected const &s)                                                            //
      noexcept(::std::is_nothrow_copy_constructible_v<E>)                                          // extension
    requires(::std::is_copy_constructible_v<E> && not ::std::is_trivially_copy_constructible_v<E>) //
      : d_(), set_(s.set_)
  {
    if (not set_) {
      ::std::destroy_at(::std::addressof(d_));
      ::std::construct_at(::std::addressof(e_), s.e_);
    }
  }

  constexpr expected(expected &&s)                                                             //
    requires(::std::is_move_constructible_v<E> && ::std::is_trivially_move_constructible_v<E>) //
  = default;
  constexpr expected(expected &&s)                        //
      noexcept(::std::is_nothrow_move_constructible_v<E>) // required
    requires(::std::is_move_constructible_v<E> && not ::std::is_trivially_move_constructible_v<E>)
      : d_(), set_(s.set_)
  {
    if (not set_) {
      ::std::destroy_at(::std::addressof(d_));
      ::std::construct_at(::std::addressof(e_), ::std::move(s.e_));
    }
  }

  template <class U, class G>
  constexpr explicit(not ::std::is_convertible_v<G const &, E>) expected(expected<U, G> const &s) //
      noexcept(::std::is_nothrow_constructible_v<E, G const &>)                                   // extension
    requires(_can_copy_convert<U, G>::value)
      : d_(), set_(s.set_)
  {
    if (not set_) {
      ::std::destroy_at(::std::addressof(d_));
      ::std::construct_at(::std::addressof(e_), s.e_);
    }
  }
  template <class U, class G>
  constexpr explicit(not ::std::is_convertible_v<G, E>) expected(expected<U, G> &&s) //
      noexcept(::std::is_nothrow_constructible_v<E, G>)                              // extension
    requires(_can_move_convert<U, G>::value)
      : d_(), set_(s.set_)
  {
    if (not set_) {
      ::std::destroy_at(::std::addressof(d_));
      ::std::construct_at(::std::addressof(e_), ::std::move(s.e_));
    }
  }

  template <class G>
  constexpr explicit(!::std::is_convertible_v<G const &, E>) expected(unexpected<G> const &g) //
      noexcept(::std::is_nothrow_constructible_v<E, G const &>)                               // extension
    requires(::std::is_constructible_v<E, G const &>)
      : e_(::std::forward<G const &>(g.error())), set_(false)
  {
  }
  template <class G>
  constexpr explicit(!::std::is_convertible_v<G, E>) expected(unexpected<G> &&g) //
      noexcept(::std::is_nothrow_constructible_v<E, G>)                          // extension
    requires(::std::is_constructible_v<E, G>)
      : e_(::std::forward<G>(g.error())), set_(false)
  {
  }

  constexpr explicit expected(::std::in_place_t) noexcept : d_(), set_(true) {}

  template <class... Args>
  constexpr explicit expected(unexpect_t, Args &&...a)        //
      noexcept(::std::is_nothrow_constructible_v<E, Args...>) // extension
    requires ::std::is_constructible_v<E, Args...>
      : e_(FWD(a)...), set_(false)
  {
  }
  template <class U, class... Args>
  constexpr explicit expected(unexpect_t, ::std::initializer_list<U> il, Args &&...a)       //
      noexcept(::std::is_nothrow_constructible_v<E, ::std::initializer_list<U> &, Args...>) // extension
    requires ::std::is_constructible_v<E, ::std::initializer_list<U> &, Args...>
      : e_(il, FWD(a)...), set_(false)
  {
  }

  // [expected.void.dtor], destructor
  constexpr ~expected() noexcept
    requires(::std::is_trivially_destructible_v<E>)
  = default;
  constexpr ~expected() //
    requires(not ::std::is_trivially_destructible_v<E>)
  {
    if (not set_)
      ::std::destroy_at(::std::addressof(e_));
    else
      ::std::destroy_at(::std::addressof(d_));
  }

  // [expected.void.assign], assignment
  constexpr expected &operator=(expected const &) = delete;
  constexpr expected &operator=(expected const &s)                                                  //
      noexcept(::std::is_nothrow_copy_assignable_v<E> && ::std::is_nothrow_copy_constructible_v<E>) // extension
    requires(::std::is_copy_assignable_v<E> && ::std::is_copy_constructible_v<E>)
  {
    if (set_ && s.set_) {
      ;
    } else if (set_) {
      ::std::destroy_at(::std::addressof(d_));
      ::std::construct_at(::std::addressof(e_), s.e_);
      set_ = false;
    } else if (s.set_) {
      ::std::destroy_at(::std::addressof(e_));
      ::std::construct_at(::std::addressof(d_));
      set_ = true;
    } else {
      e_ = s.e_;
    }
    return *this;
  }

  constexpr expected &operator=(expected &&s)                                                       //
      noexcept(::std::is_nothrow_move_assignable_v<E> && ::std::is_nothrow_move_constructible_v<E>) // required
    requires(::std::is_move_constructible_v<E> && ::std::is_move_assignable_v<E>)
  {
    if (set_ && s.set_) {
      ;
    } else if (set_) {
      ::std::destroy_at(::std::addressof(d_));
      ::std::construct_at(::std::addressof(e_), ::std::move(s.e_));
      set_ = false;
    } else if (s.set_) {
      ::std::destroy_at(::std::addressof(e_));
      ::std::construct_at(::std::addressof(d_));
      set_ = true;
    } else {
      e_ = ::std::move(s.e_);
    }
    return *this;
  }

  template <class G>
  constexpr expected &operator=(unexpected<G> const &s) //
      noexcept(::std::is_nothrow_assignable_v<E, G const &>
               && ::std::is_nothrow_constructible_v<E, G const &>) // extension
    requires(::std::is_constructible_v<E, G const &> && ::std::is_assignable_v<E &, G const &>)
  {
    if (set_) {
      ::std::destroy_at(::std::addressof(d_));
      ::std::construct_at(::std::addressof(e_), ::std::forward<G const &>(s.error()));
      set_ = false;
    } else {
      e_ = ::std::forward<G const &>(s.error());
    }
    return *this;
  }

  template <class G>
  constexpr expected &operator=(unexpected<G> &&s)                                              //
      noexcept(::std::is_nothrow_assignable_v<E, G> && ::std::is_nothrow_constructible_v<E, G>) // extension
    requires(::std::is_constructible_v<E, G> && ::std::is_assignable_v<E &, G>)
  {
    if (set_) {
      ::std::destroy_at(::std::addressof(d_));
      ::std::construct_at(::std::addressof(e_), ::std::forward<G>(s.error()));
      set_ = false;
    } else {
      e_ = ::std::forward<G>(s.error());
    }
    return *this;
  }

  constexpr void emplace() noexcept
  {
    if (not set_) {
      ::std::destroy_at(::std::addressof(e_));
      ::std::construct_at(::std::addressof(d_));
      set_ = true;
    }
  }

  // [expected.void.swap], swap
  constexpr void swap(expected &rhs) //
      noexcept(::std::is_nothrow_move_constructible_v<E> && ::std::is_nothrow_swappable_v<E>)
    requires(::std::is_swappable_v<E> && ::std::is_move_constructible_v<E>)
  {
    bool const lhset = has_value();
    bool const rhset = rhs.has_value();
    if (lhset == rhset) {
      if (not lhset) {
        using ::std::swap;
        swap(e_, rhs.e_);
      }
    } else {
      if (lhset) {
        ::std::destroy_at(::std::addressof(d_));
        ::std::construct_at(::std::addressof(e_), ::std::move(rhs.e_));
        ::std::destroy_at(::std::addressof(rhs.e_));
        ::std::construct_at(::std::addressof(rhs.d_));
        set_ = false;
        rhs.set_ = true;
      } else {
        rhs.swap(*this);
      }
    }
  }

  constexpr friend void swap(expected &x, expected &y) noexcept(noexcept(x.swap(y)))
    requires requires { x.swap(y); }
  {
    x.swap(y);
  }

  // [expected.void.obs], observers
  constexpr explicit operator bool() const noexcept { return set_; }
  constexpr bool has_value() const noexcept { return set_; }
  constexpr bool has_error() const noexcept { return !set_; }
  constexpr void operator*() const noexcept { ASSERT(set_); }
  constexpr void value() const &
  {
    static_assert(::std::is_copy_constructible_v<E>);
    if (not set_)
      throw bad_expected_access<E>(e_);
  }
  constexpr void value() &&
  {
    static_assert(::std::is_copy_constructible_v<E>);
    static_assert(::std::is_move_constructible_v<E>);
    if (not set_)
      throw bad_expected_access<E>(::std::move(e_));
  }
  constexpr E const &error() const & noexcept
  {
    ASSERT(not set_);
    return e_;
  }
  constexpr E &error() & noexcept
  {
    ASSERT(not set_);
    return e_;
  }
  constexpr E const &&error() const && noexcept
  {
    ASSERT(not set_);
    return ::std::move(e_);
  }
  constexpr E &&error() && noexcept
  {
    ASSERT(not set_);
    return ::std::move(e_);
  }

  template <class G = E>
  constexpr E error_or(G &&e) const &                                                              //
      noexcept(::std::is_nothrow_copy_constructible_v<E> && ::std::is_nothrow_convertible_v<G, E>) // extension
  {
    static_assert(::std::is_copy_constructible_v<E>);
    static_assert(::std::is_convertible_v<G, E>);
    if (set_) {
      return FWD(e);
    }
    return e_;
  }
  template <class G = E>
  constexpr E error_or(G &&e) &&                                                                   //
      noexcept(::std::is_nothrow_move_constructible_v<E> && ::std::is_nothrow_convertible_v<G, E>) // extension
  {
    static_assert(::std::is_move_constructible_v<E>);
    static_assert(::std::is_convertible_v<G, E>);
    if (set_) {
      return FWD(e);
    }
    return ::std::move(e_);
  }

  // [expected.void.monadic], monadic operations
  template <class F>
  constexpr auto and_then(F &&f) &                 //
      noexcept(noexcept(_and_then(*this, FWD(f)))) // extension
      -> decltype(_and_then(*this, FWD(f)))
  {
    return _and_then(*this, FWD(f));
  }
  template <class F>
  constexpr auto and_then(F &&f) &&                             //
      noexcept(noexcept(_and_then(::std::move(*this), FWD(f)))) // extension
      -> decltype(_and_then(::std::move(*this), FWD(f)))
  {
    return _and_then(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto and_then(F &&f) const &           //
      noexcept(noexcept(_and_then(*this, FWD(f)))) // extension
      -> decltype(_and_then(*this, FWD(f)))
  {
    return _and_then(*this, FWD(f));
  }
  template <class F>
  constexpr auto and_then(F &&f) const &&                       //
      noexcept(noexcept(_and_then(::std::move(*this), FWD(f)))) // extension
      -> decltype(_and_then(::std::move(*this), FWD(f)))
  {
    return _and_then(::std::move(*this), FWD(f));
  }

  template <class F>
  constexpr auto or_else(F &&f) &                 //
      noexcept(noexcept(_or_else(*this, FWD(f)))) // extension
      -> decltype(_or_else(*this, FWD(f)))
  {
    return _or_else(*this, FWD(f));
  }
  template <class F>
  constexpr auto or_else(F &&f) &&                             //
      noexcept(noexcept(_or_else(::std::move(*this), FWD(f)))) // extension
      -> decltype(_or_else(::std::move(*this), FWD(f)))
  {
    return _or_else(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto or_else(F &&f) const &           //
      noexcept(noexcept(_or_else(*this, FWD(f)))) // extension
      -> decltype(_or_else(*this, FWD(f)))
  {
    return _or_else(*this, FWD(f));
  }
  template <class F>
  constexpr auto or_else(F &&f) const &&                       //
      noexcept(noexcept(_or_else(::std::move(*this), FWD(f)))) // extension
      -> decltype(_or_else(::std::move(*this), FWD(f)))
  {
    return _or_else(::std::move(*this), FWD(f));
  }

  template <class F>
  constexpr auto transform(F &&f) &                 //
      noexcept(noexcept(_transform(*this, FWD(f)))) // extension
      -> decltype(_transform(*this, FWD(f)))
  {
    return _transform(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform(F &&f) &&                             //
      noexcept(noexcept(_transform(::std::move(*this), FWD(f)))) // extension
      -> decltype(_transform(::std::move(*this), FWD(f)))
  {
    return _transform(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto transform(F &&f) const &           //
      noexcept(noexcept(_transform(*this, FWD(f)))) // extension
      -> decltype(_transform(*this, FWD(f)))
  {
    return _transform(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform(F &&f) const &&                       //
      noexcept(noexcept(_transform(::std::move(*this), FWD(f)))) // extension
      -> decltype(_transform(::std::move(*this), FWD(f)))
  {
    return _transform(::std::move(*this), FWD(f));
  }

  template <class F>
  constexpr auto transform_error(F &&f) &                 //
      noexcept(noexcept(_transform_error(*this, FWD(f)))) // extension
      -> decltype(_transform_error(*this, FWD(f)))
  {
    return _transform_error(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform_error(F &&f) &&                             //
      noexcept(noexcept(_transform_error(::std::move(*this), FWD(f)))) // extension
      -> decltype(_transform_error(::std::move(*this), FWD(f)))
  {
    return _transform_error(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto transform_error(F &&f) const &           //
      noexcept(noexcept(_transform_error(*this, FWD(f)))) // extension
      -> decltype(_transform_error(*this, FWD(f)))
  {
    return _transform_error(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform_error(F &&f) const &&                       //
      noexcept(noexcept(_transform_error(::std::move(*this), FWD(f)))) // extension
      -> decltype(_transform_error(::std::move(*this), FWD(f)))
  {
    return _transform_error(::std::move(*this), FWD(f));
  }

  // [expected.void.eq], equality operators
  template <class T2, class E2>
    requires(::std::is_void_v<T2>)
  constexpr friend bool operator==(expected const &x, expected<T2, E2> const &y) //
      noexcept(noexcept(static_cast<bool>((x.error() == y.error()))))            // extension
    requires requires {
      { x.error() == y.error() } -> std::convertible_to<bool>;
    }
  {
    if (x.has_value() != y.has_value()) {
      return false;
    } else if (x.has_value()) {
      return true;
    } else {
      return x.error() == y.error();
    }
  }
  template <class E2>
  constexpr friend bool operator==(expected const &x, unexpected<E2> const &e) //
      noexcept(noexcept(static_cast<bool>(x.error() == e.error())))            // extension
    requires requires {
      { x.error() == e.error() } -> std::convertible_to<bool>;
    }
  {
    return (not x.has_value()) && static_cast<bool>(x.error() == e.error());
  }

private:
  struct dummy final {
    constexpr dummy() noexcept = default;
    constexpr ~dummy() noexcept = default;
  };

  union {
    [[no_unique_address]] dummy d_;
    E e_;
  };
  bool set_;
};

} // namespace pfn

#undef ASSERT

#ifdef INCLUDE_PFN_EXPECTED__POP_ASSERT
#pragma pop_macro("ASSERT")
#endif

#undef FWD

#ifdef INCLUDE_PFN_EXPECTED__POP_FWD
#pragma pop_macro("FWD")
#endif

#endif // INCLUDE_PFN_EXPECTED
