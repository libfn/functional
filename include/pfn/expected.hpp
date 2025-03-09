// Copyright (c) 2025 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_PFN_EXPECTED
#define INCLUDE_PFN_EXPECTED

#include <cassert>
#include <exception>
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

#define ASSERT(...) assert((__VA_ARGS__) == true);

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
} // namespace detail

// declare void specialization
template <class T, class E>
  requires ::std::is_void_v<T>
class expected<T, E>;

template <class T, class E> class expected {
private:
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

  template <class U>
  using _can_convert = ::std::bool_constant<                            //
      not ::std::is_same_v<::std::remove_cvref_t<U>, ::std::in_place_t> //
      && not ::std::is_same_v<::std::remove_cvref_t<U>, unexpect_t>     //
      && not ::std::is_same_v<expected, ::std::remove_cvref_t<U>>       //
      && not detail::_is_some_unexpected<::std::remove_cvref_t<U>>      //
      && ::std::is_constructible_v<T, U>                                //
      && (not ::std::is_same_v<bool, ::std::remove_cv_t<T>>             //
          || not detail::_is_some_expected<::std::remove_cvref_t<U>>)>;

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
  constexpr expected(expected const &s) //
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
      ::std::construct_at(&v_, s.v_);
    else
      ::std::construct_at(&e_, s.e_);
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
      ::std::construct_at(&v_, ::std::move(s.v_));
    else
      ::std::construct_at(&e_, ::std::move(s.e_));
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
      ::std::construct_at(&v_, s.v_);
    else
      ::std::construct_at(&e_, s.e_);
  }

  template <class U, class G>
  constexpr explicit(not ::std::is_convertible_v<U, T> || not ::std::is_convertible_v<G, E>)
      expected(expected<U, G> &&s)                                                                 //
      noexcept(::std::is_nothrow_constructible_v<T, U> && ::std::is_nothrow_constructible_v<E, G>) // extension
    requires(_can_move_convert<U, G>::value)
      : set_(s.set_)
  {
    if (set_)
      ::std::construct_at(&v_, ::std::move(s.v_));
    else
      ::std::construct_at(&e_, ::std::move(s.e_));
  }

  template <class U = ::std::remove_cv_t<T>>
  constexpr explicit(not ::std::is_convertible_v<U, T>) expected(U &&v) //
      noexcept(::std::is_nothrow_constructible_v<T, U &&>)              // extension
    requires(_can_convert<U>::value)
      : v_(FWD(v)), set_(true)
  {
  }

  template <class G> constexpr explicit(/* TODO */ false) expected(unexpected<G> const &);
  template <class G> constexpr explicit(/* TODO */ false) expected(unexpected<G> &&);

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
  constexpr ~expected()                             //
      noexcept(::std::is_nothrow_destructible_v<E>) // extension
    requires(::std::is_trivially_destructible_v<T> && not ::std::is_trivially_destructible_v<E>)
  {
    if (not set_)
      ::std::destroy_at(&e_);
    // else T is trivially destructible, no need to do anything
  }
  constexpr ~expected()                             //
      noexcept(::std::is_nothrow_destructible_v<T>) // extension
    requires(not ::std::is_trivially_destructible_v<T> && ::std::is_trivially_destructible_v<E>)
  {
    if (set_)
      ::std::destroy_at(&v_);
    // else E is trivially destructible, no need to do anything
  }
  constexpr ~expected()                                                                    //
      noexcept(::std::is_nothrow_destructible_v<T> && ::std::is_nothrow_destructible_v<E>) // extension
    requires(not ::std::is_trivially_destructible_v<T> && not ::std::is_trivially_destructible_v<E>)
  {
    if (set_)
      ::std::destroy_at(&v_);
    else
      ::std::destroy_at(&e_);
  }

  // [expected.object.assign], assignment
  constexpr expected &operator=(expected const &);
  constexpr expected &operator=(expected &&) noexcept(/* TODO */ false);
  template <class U = T> constexpr expected &operator=(U &&);
  template <class G> constexpr expected &operator=(unexpected<G> const &);
  template <class G> constexpr expected &operator=(unexpected<G> &&);

  template <class... Args> constexpr T &emplace(Args &&...) noexcept;
  template <class U, class... Args> constexpr T &emplace(std::initializer_list<U>, Args &&...) noexcept;

  // [expected.object.swap], swap
  constexpr void swap(expected &) noexcept(/* TODO */ false);
  constexpr friend void swap(expected &x, expected &y) noexcept(noexcept(x.swap(y)));

  // [expected.object.obs], observers
  constexpr T const *operator->() const noexcept
  {
    ASSERT(set_);
    return addressof(v_);
  }
  constexpr T *operator->() noexcept
  {
    ASSERT(set_);
    return addressof(v_);
  }
  constexpr T const &operator*() const & noexcept { return *(this->operator->()); }
  constexpr T &operator*() & noexcept { return *(this->operator->()); }
  constexpr T const &&operator*() const && noexcept { return ::std::move(*(this->operator->())); }
  constexpr T &&operator*() && noexcept { return ::std::move(*(this->operator->())); }
  constexpr explicit operator bool() const noexcept { return set_; }
  constexpr bool has_value() const noexcept { return set_; }
  constexpr T const &value() const &
  {
    if (!set_)
      throw bad_expected_access<E>(::std::as_const(e_));
    return v_;
  }
  constexpr T &value() &
  {
    if (!set_)
      throw bad_expected_access<E>(::std::as_const(e_));
    return v_;
  }
  constexpr T const &&value() const &&
  {
    if (!set_)
      throw bad_expected_access<E>(::std::as_const(e_));
    return ::std::move(v_);
  }
  constexpr T &&value() &&
  {
    if (!set_)
      throw bad_expected_access<E>(::std::as_const(e_));
    return ::std::move(v_);
  }
  constexpr E const &error() const & noexcept
  {
    ASSERT(!set_);
    return e_;
  }
  constexpr E &error() & noexcept
  {
    ASSERT(!set_);
    return e_;
  }
  constexpr E const &&error() const && noexcept
  {
    ASSERT(!set_);
    return ::std::move(e_);
  }
  constexpr E &&error() && noexcept
  {
    ASSERT(!set_);
    return ::std::move(e_);
  }

  template <class U> constexpr T value_or(U &&) const &;
  template <class U> constexpr T value_or(U &&) &&;
  template <class G = E> constexpr E error_or(G &&) const &;
  template <class G = E> constexpr E error_or(G &&) &&;

  // [expected.object.monadic], monadic operations
  template <class F> constexpr auto and_then(F &&f) &;
  template <class F> constexpr auto and_then(F &&f) &&;
  template <class F> constexpr auto and_then(F &&f) const &;
  template <class F> constexpr auto and_then(F &&f) const &&;
  template <class F> constexpr auto or_else(F &&f) &;
  template <class F> constexpr auto or_else(F &&f) &&;
  template <class F> constexpr auto or_else(F &&f) const &;
  template <class F> constexpr auto or_else(F &&f) const &&;
  template <class F> constexpr auto transform(F &&f) &;
  template <class F> constexpr auto transform(F &&f) &&;
  template <class F> constexpr auto transform(F &&f) const &;
  template <class F> constexpr auto transform(F &&f) const &&;
  template <class F> constexpr auto transform_error(F &&f) &;
  template <class F> constexpr auto transform_error(F &&f) &&;
  template <class F> constexpr auto transform_error(F &&f) const &;
  template <class F> constexpr auto transform_error(F &&f) const &&;

  // [expected.object.eq], equality operators
  template <class T2, class E2>
    requires(not ::std::is_void_v<T2>)
  constexpr friend bool operator==(expected const &x, expected<T2, E2> const &y);
  template <class T2> constexpr friend bool operator==(expected const &, const T2 &);
  template <class E2> constexpr friend bool operator==(expected const &, unexpected<E2> const &);

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
public:
  using value_type = T;
  using error_type = E;
  using unexpected_type = unexpected<E>;

  template <class U> using rebind = expected<U, error_type>;

  // [expected.void.cons], constructors
  constexpr expected() noexcept;
  constexpr expected(expected const &);
  constexpr expected(expected &&) noexcept(/* TODO */ false);
  template <class U, class G> constexpr explicit(/* TODO */ false) expected(expected<U, G> const &);
  template <class U, class G> constexpr explicit(/* TODO */ false) expected(expected<U, G> &&);

  template <class G> constexpr explicit(/* TODO */ false) expected(unexpected<G> const &);
  template <class G> constexpr explicit(/* TODO */ false) expected(unexpected<G> &&);

  constexpr explicit expected(::std::in_place_t) noexcept;
  template <class... Args> constexpr explicit expected(unexpect_t, Args &&...);
  template <class U, class... Args> constexpr explicit expected(unexpect_t, ::std::initializer_list<U>, Args &&...);

  // [expected.void.dtor], destructor
  constexpr ~expected();

  // [expected.void.assign], assignment
  constexpr expected &operator=(expected const &);
  constexpr expected &operator=(expected &&) noexcept(/* TODO */ false);
  template <class G> constexpr expected &operator=(unexpected<G> const &);
  template <class G> constexpr expected &operator=(unexpected<G> &&);
  constexpr void emplace() noexcept;

  // [expected.void.swap], swap
  constexpr void swap(expected &) noexcept(/* TODO */ false);
  constexpr friend void swap(expected &x, expected &y) noexcept(noexcept(x.swap(y)));

  // [expected.void.obs], observers
  constexpr explicit operator bool() const noexcept;
  constexpr bool has_value() const noexcept;
  constexpr void operator*() const noexcept;
  constexpr void value() const &; // freestanding-deleted
  constexpr void value() &&;      // freestanding-deleted
  constexpr E const &error() const & noexcept;
  constexpr E &error() & noexcept;
  constexpr E const &&error() const && noexcept;
  constexpr E &&error() && noexcept;
  template <class G = E> constexpr E error_or(G &&) const &;
  template <class G = E> constexpr E error_or(G &&) &&;

  // [expected.void.monadic], monadic operations
  template <class F> constexpr auto and_then(F &&f) &;
  template <class F> constexpr auto and_then(F &&f) &&;
  template <class F> constexpr auto and_then(F &&f) const &;
  template <class F> constexpr auto and_then(F &&f) const &&;
  template <class F> constexpr auto or_else(F &&f) &;
  template <class F> constexpr auto or_else(F &&f) &&;
  template <class F> constexpr auto or_else(F &&f) const &;
  template <class F> constexpr auto or_else(F &&f) const &&;
  template <class F> constexpr auto transform(F &&f) &;
  template <class F> constexpr auto transform(F &&f) &&;
  template <class F> constexpr auto transform(F &&f) const &;
  template <class F> constexpr auto transform(F &&f) const &&;
  template <class F> constexpr auto transform_error(F &&f) &;
  template <class F> constexpr auto transform_error(F &&f) &&;
  template <class F> constexpr auto transform_error(F &&f) const &;
  template <class F> constexpr auto transform_error(F &&f) const &&;

  // [expected.void.eq], equality operators
  template <class T2, class E2>
    requires ::std::is_void_v<T2>
  constexpr friend bool operator==(expected const &x, expected<T2, E2> const &y);
  template <class E2> constexpr friend bool operator==(expected const &, unexpected<E2> const &);

private:
  union {
    unsigned char dummy_;
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
