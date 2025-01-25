// Copyright (c) 2025 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_PFN_EXPECTED
#define INCLUDE_PFN_EXPECTED

#include <exception>
#include <initializer_list>
#include <type_traits>
#include <utility>

#ifdef FWD
#pragma push_macro("FWD")
#define INCLUDE_PFN_EXPECTED__POP_FWD
#undef FWD
#endif

// Also defined in fn/detail/fwd_macro.hpp but pfn/* headers are standalone
#define FWD(...) static_cast<decltype(__VA_ARGS__) &&>(__VA_ARGS__)

namespace pfn {

template <class E> class bad_expected_access;

template <> class bad_expected_access<void> : public std::exception {
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
  E &&error() && noexcept { return std::move(e_); }
  E const &&error() const && noexcept { return std::move(e_); }

private:
  E e_;
};

struct unexpect_t {
  explicit unexpect_t() = default;
};
constexpr inline unexpect_t unexpect{};

template <class E> class unexpected;

namespace detail {
template <typename T> constexpr bool _is_some_unexpected = false;
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
  constexpr explicit unexpected(std::in_place_t, Args &&...a) noexcept(::std::is_nothrow_constructible_v<E, Args...>)
    requires ::std::is_constructible_v<E, Args...>
      : e_(FWD(a)...)
  {
  }

  template <class U, class... Args>
  constexpr explicit unexpected(std::in_place_t, std::initializer_list<U> i, Args &&...a) noexcept(
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

// declare void specialization
template <class T, class E>
  requires std::is_void_v<T>
class expected<T, E>;

template <class T, class E> class expected {
public:
  using value_type = T;
  using error_type = E;
  using unexpected_type = unexpected<E>;

  template <class U> using rebind = expected<U, error_type>;

  // [expected.object.cons], constructors
  constexpr expected();
  constexpr expected(expected const &);
  constexpr expected(expected &&) noexcept(/* TODO */ false);
  template <class U, class G> constexpr explicit(/* TODO */ false) expected(expected<U, G> const &);
  template <class U, class G> constexpr explicit(/* TODO */ false) expected(expected<U, G> &&);

  template <class U = T> constexpr explicit(/* TODO */ false) expected(U &&v);

  template <class G> constexpr explicit(/* TODO */ false) expected(unexpected<G> const &);
  template <class G> constexpr explicit(/* TODO */ false) expected(unexpected<G> &&);

  template <class... Args> constexpr explicit expected(std::in_place_t, Args &&...);
  template <class U, class... Args> constexpr explicit expected(std::in_place_t, std::initializer_list<U>, Args &&...);
  template <class... Args> constexpr explicit expected(unexpect_t, Args &&...);
  template <class U, class... Args> constexpr explicit expected(unexpect_t, std::initializer_list<U>, Args &&...);

  // [expected.object.dtor], destructor
  constexpr ~expected();

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
  constexpr T const *operator->() const noexcept;
  constexpr T *operator->() noexcept;
  constexpr T const &operator*() const & noexcept;
  constexpr T &operator*() & noexcept;
  constexpr T const &&operator*() const && noexcept;
  constexpr T &&operator*() && noexcept;
  constexpr explicit operator bool() const noexcept;
  constexpr bool has_value() const noexcept;
  constexpr T const &value() const &;   // freestanding-deleted
  constexpr T &value() &;               // freestanding-deleted
  constexpr T const &&value() const &&; // freestanding-deleted
  constexpr T &&value() &&;             // freestanding-deleted
  constexpr E const &error() const & noexcept;
  constexpr E &error() & noexcept;
  constexpr E const &&error() const && noexcept;
  constexpr E &&error() && noexcept;
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
    requires(!std::is_void_v<T2>)
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
  requires std::is_void_v<T>
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

  constexpr explicit expected(std::in_place_t) noexcept;
  template <class... Args> constexpr explicit expected(unexpect_t, Args &&...);
  template <class U, class... Args> constexpr explicit expected(unexpect_t, std::initializer_list<U>, Args &&...);

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
    requires std::is_void_v<T2>
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

#undef FWD

#ifdef INCLUDE_PFN_EXPECTED__POP_FWD
#pragma pop_macro("FWD")
#endif

#endif // INCLUDE_PFN_EXPECTED
