// Copyright (c) 2025 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_PFN_EXPECTED
#define INCLUDE_PFN_EXPECTED

#include <exception>
#include <type_traits>
#include <utility>

namespace fn::detail {

template <class E> class bad_expected_access;

template <> class bad_expected_access<void> : public std::exception {
protected:
  bad_expected_access() noexcept;
  bad_expected_access(bad_expected_access const &) noexcept;
  bad_expected_access(bad_expected_access &&) noexcept;
  bad_expected_access &operator=(bad_expected_access const &) noexcept;
  bad_expected_access &operator=(bad_expected_access &&) noexcept;
  ~bad_expected_access();

public:
  [[nodiscard]] char const *what() const noexcept override;
};

template <class E> class bad_expected_access : public bad_expected_access<void> {
public:
  explicit bad_expected_access(E);
  [[nodiscard]] char const *what() const noexcept override;
  E &error() & noexcept;
  E const &error() const & noexcept;
  E &&error() && noexcept;
  E const &&error() const && noexcept;

private:
  E e_;
};

struct unexpect_t {
  explicit unexpect_t() = default;
};
constexpr inline unexpect_t unexpect{};

template <class E> class unexpected {
public:
  constexpr unexpected(unexpected const &) = default;
  constexpr unexpected(unexpected &&) = default;
  template <class Err = E> constexpr explicit unexpected(Err &&);
  template <class... Args> constexpr explicit unexpected(std::in_place_t, Args &&...);
  template <class U, class... Args>
  constexpr explicit unexpected(std::in_place_t, std::initializer_list<U>, Args &&...);

  constexpr unexpected &operator=(unexpected const &) = default;
  constexpr unexpected &operator=(unexpected &&) = default;

  constexpr E const &error() const & noexcept;
  constexpr E &error() & noexcept;
  constexpr E const &&error() const && noexcept;
  constexpr E &&error() && noexcept;

  constexpr void swap(unexpected &other) noexcept(/* TODO */ false);

  template <class E2> constexpr friend bool operator==(unexpected const &, unexpected<E2> const &);

  constexpr friend void swap(unexpected &x, unexpected &y) noexcept(noexcept(x.swap(y)));

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

} // namespace fn::detail

#endif // INCLUDE_PFN_EXPECTED
