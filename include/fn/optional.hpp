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
#include <functional>
#include <initializer_list>
#include <type_traits>
#include <utility>

namespace fn {

template <typename T>
concept some_optional = detail::_some_optional<T>;

namespace detail {

// [optional.iterators]: the implementation-defined iterator types for fn::optional. A
// minimal wrapper over T* whose job is to keep pointer-ness out of optional interface.
template <class T> class _optional_iterator {
  static_assert(::std::is_object_v<T>);

  T *p_ = nullptr;

  // Only an optional's base mints iterators from storage pointers; the sibling friendship
  // lets the iterator -> const_iterator converting constructor read p_.
  template <class, class> friend struct ::pfn::detail::_optional_base;
  template <class> friend class _optional_iterator;

  constexpr explicit _optional_iterator(T *p) noexcept : p_(p) {}

public:
  using iterator_concept = ::std::contiguous_iterator_tag;
  using iterator_category = ::std::random_access_iterator_tag;
  using value_type = ::std::remove_cv_t<T>;
  using difference_type = ::std::ptrdiff_t;
  using pointer = T *;
  using reference = T &;

  constexpr _optional_iterator() noexcept = default;

  // iterator -> const_iterator, required by the container iterator requirements
  template <class U>
    requires ::std::is_same_v<U const, T>
  constexpr _optional_iterator(_optional_iterator<U> const &other) noexcept // NOSONAR cpp:S1709 implicit per spec
      : p_(other.p_)
  {
  }

  [[nodiscard]] constexpr T &operator*() const noexcept { return *p_; }
  [[nodiscard]] constexpr T *operator->() const noexcept { return p_; } // std::to_address requires this
  [[nodiscard]] constexpr T &operator[](difference_type n) const noexcept { return p_[n]; }

  constexpr _optional_iterator &operator++() noexcept
  {
    ++p_;
    return *this;
  }
  constexpr _optional_iterator operator++(int) noexcept
  {
    auto r = *this;
    ++p_;
    return r;
  }
  constexpr _optional_iterator &operator--() noexcept
  {
    --p_;
    return *this;
  }
  constexpr _optional_iterator operator--(int) noexcept
  {
    auto r = *this;
    --p_;
    return r;
  }
  constexpr _optional_iterator &operator+=(difference_type n) noexcept
  {
    p_ += n;
    return *this;
  }
  constexpr _optional_iterator &operator-=(difference_type n) noexcept
  {
    p_ -= n;
    return *this;
  }

  [[nodiscard]] constexpr friend _optional_iterator operator+(_optional_iterator i, difference_type n) noexcept
  {
    return i += n;
  }
  [[nodiscard]] constexpr friend _optional_iterator operator+(difference_type n, _optional_iterator i) noexcept
  {
    return i += n;
  }
  [[nodiscard]] constexpr friend _optional_iterator operator-(_optional_iterator i, difference_type n) noexcept
  {
    return i -= n;
  }
  [[nodiscard]] constexpr friend difference_type operator-(_optional_iterator const &x,
                                                           _optional_iterator const &y) noexcept
  {
    return x.p_ - y.p_;
  }

  [[nodiscard]] constexpr friend bool operator==(_optional_iterator const &, _optional_iterator const &) noexcept
      = default;
  [[nodiscard]] constexpr friend ::std::strong_ordering operator<=>(_optional_iterator const &x,
                                                                    _optional_iterator const &y) noexcept
  {
    return x.p_ <=> y.p_;
  }
};

struct optional_policy {
  template <class U> using type = ::fn::optional<U>;
  template <class U> using iterator = _optional_iterator<U>;
  template <class X> static constexpr bool is_specialization = _is_some_optional<X &>;
};

// Storage layer for ::fn::optional. Inherits the standard-conformant base from
// pfn, then hides the three monadic static helpers with sum-aware variants that
// materialise their result via `optional_policy::template type<U>`.
// The transform helpers hand pfn's _optional_from_invoke constructor a zero-argument
// thunk, so the result's contained value is direct-non-list-initialized from fn's own
// _invoke (or sum::transform) result: no extra move, and immovable result types work.
// The statics carry the same extension noexcept as pfn's, spelled with the std traits: for
// the sum/pack dispatch extensions std::is_nothrow_invocable_v is false (the callable is not
// directly invocable on a sum or pack), so those are conservatively noexcept(false).
template <typename T> struct _optional_base : ::pfn::detail::_optional_base<T, optional_policy> {
  using _pfn_base = ::pfn::detail::_optional_base<T, optional_policy>;
  using _pfn_base::_pfn_base;

  // and_then
  template <typename Self, typename Fn>
  static constexpr auto _and_then(Self &&self, Fn &&fn)                 //
      noexcept(::std::is_nothrow_invocable_v<Fn, decltype(*FWD(self))>) // extension
    requires ::fn::detail::_is_invocable<Fn, decltype(*FWD(self))>::value
  {
    using type = ::std::remove_cvref_t<typename ::fn::detail::_invoke_result<Fn, decltype(*FWD(self))>::type>;
    static_assert(_is_some_optional<type &>);
    if (self.has_value())
      return ::fn::detail::_invoke(FWD(fn), *FWD(self));
    else {
#if defined(__clang__) && __clang_major__ <= 18
      // clang 15-18 miscompile the prvalue return below for three of the four Self ref-qualifier
      // instantiations (&, const &, const &&) at -O1/-O2: the disengaged result is observed with
      // garbage in set_ (storage-poison). The workaround dodges the buggy mandatory copy-elision,
      // at the cost of a move -- an immovable result type must keep the prvalue (the workaround
      // would not compile; the miscompile is not observed in that shape).
      if constexpr (::std::is_move_constructible_v<type>)
        return ::std::move(type(::std::nullopt));
      else
        return type(::std::nullopt);
#else
      return type(::std::nullopt);
#endif
    }
  }

  // or_else (with value-widening into a sum)
  template <typename Self, typename Fn>
  static constexpr auto _or_else(Self &&self, Fn &&fn) //
      noexcept(
          ::std::is_same_v<::std::remove_cvref_t<typename ::fn::detail::_invoke_result<Fn>::type>, ::fn::optional<T>>
          && ::std::is_nothrow_invocable_v<Fn>
          && ::std::is_nothrow_constructible_v<::fn::optional<T>, Self>) // extension
    requires ::fn::detail::_is_invocable<Fn>::value && ::std::is_constructible_v<T, decltype(*FWD(self))>
  {
    using type = ::std::remove_cvref_t<typename ::fn::detail::_invoke_result<Fn>::type>;
    static_assert(_is_some_optional<type &>);
    // compare whole optional types (not value_type) so optional<T&> instantiations, whose
    // value_type is the unqualified referent, take the same-type arm
    static_assert(::std::is_same_v<type, ::fn::optional<T>> || some_sum<T>);
    if constexpr (::std::is_same_v<type, ::fn::optional<T>>) {
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

  // or_else for the optional<T&> wrapper: forwards to pfn's reference _or_else (hidden by the
  // _or_else above), propagating its constraints and noexcept -- a reference optional has
  // nothing to sum-widen, so pfn's semantics apply exactly
  template <typename Self, typename Fn>
  static constexpr auto _or_else_ref(Self &&self, Fn &&fn)        //
      noexcept(noexcept(_pfn_base::_or_else(FWD(self), FWD(fn)))) //
      -> decltype(_pfn_base::_or_else(FWD(self), FWD(fn)))
  {
    return _pfn_base::_or_else(FWD(self), FWD(fn));
  }

  // transform, value type is not a sum. In the noexcept spec, only the invoke can throw: the
  // result is direct-non-list-initialized from the thunk's result (guaranteed elision).
  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&fn)                //
      noexcept(::std::is_nothrow_invocable_v<Fn, decltype(*FWD(self))>) // extension
    requires(not some_sum<T>) && ::fn::detail::_is_invocable_if<not some_sum<T>, Fn, decltype(*FWD(self))>::value
  {
    using new_value_type = ::std::remove_cv_t<typename ::fn::detail::_invoke_result<Fn, decltype(*FWD(self))>::type>;
    using type = ::fn::optional<new_value_type>;
    if (self.has_value())
      return type(::pfn::detail::_optional_from_invoke,
                  [&fn, &self]() -> decltype(auto) { return ::fn::detail::_invoke(FWD(fn), *FWD(self)); });
    else
      return type(::std::nullopt);
  }

  // transform, value type is a sum (delegates to sum::transform). The callback is constrained here,
  // in the immediate context: the deduced return type instantiates the body, so leaving it to
  // sum::transform's own constraint would make a bad callback a hard error instead of dropping the
  // candidate - and would poison overload resolution, since the losing candidates form their
  // signatures too.
  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&fn)
    requires some_sum<T> && ::fn::detail::_typelist_invocable<Fn, decltype(*FWD(self))>
  {
    using new_value_type = decltype((*FWD(self)).transform(FWD(fn)));
    using type = ::fn::optional<new_value_type>;
    if (self.has_value())
      return type(::pfn::detail::_optional_from_invoke,
                  [&fn, &self]() -> decltype(auto) { return (*FWD(self)).transform(FWD(fn)); });
    else
      return type(::std::nullopt);
  }
};

} // namespace detail

template <typename T> class optional : private detail::_optional_base<T> { // NOSONAR cpp:S3624 base manages storage
  static_assert(::pfn::detail::_is_valid_optional<T>);
  static_assert(not ::std::is_same_v<T, ::fn::sum<>>);
  using _base = detail::_optional_base<T>;

  // Allow sibling _optional_base instantiations to downcast into the private base.
  template <class, class> friend struct ::pfn::detail::_optional_base;
  template <class> friend struct ::fn::detail::_optional_base;

public:
  using value_type = T;
  // [optional.iterators]: mirrors pfn::optional, with fn's own iterator type
  using iterator = detail::_optional_iterator<T>;
  using const_iterator = detail::_optional_iterator<T const>;

  // Constructors. Explicit forwarders to the base mirror pfn::optional.
  constexpr optional() noexcept : _base(::std::nullopt) {}
  constexpr optional(::std::nullopt_t) noexcept : _base(::std::nullopt) {} // NOSONAR cpp:S1709 implicit per spec

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
  constexpr explicit(not ::std::is_convertible_v<U, T>) optional(U &&v) // NOSONAR cpp:S6458 _can_convert excludes self
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
  {
    static_assert(::std::is_move_constructible_v<T>);
    this->_swap_with(rhs);
  }

  // Iterator support inherited from _optional_base, mirrors pfn::optional
  using _base::begin;
  using _base::end;

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

  // or_else has only const& and && overloads (mirroring pfn::optional): an extra & or const&&
  // overload would silently change which reference category an engaged value is copied through
  // in a program switched over from pfn::optional.
  template <class F>
  constexpr auto or_else(F &&f) const &                  //
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

  // Convert to graded monad. The lifting overloads wrap the value in a sum and that sum in the
  // result, so they weigh both; the ones whose value type already is a sum only return *this.
  constexpr auto sum_value() const & noexcept(::std::is_nothrow_constructible_v<sum<value_type>, value_type const &>
                                              && ::std::is_nothrow_move_constructible_v<sum<value_type>>) // extension
      -> optional<sum<value_type>>
    requires(not some_sum<value_type>)
  {
    using type = optional<sum<value_type>>;
    if (this->has_value())
      return type{::std::in_place, sum<value_type>(this->value())};
    else
      return type{::std::nullopt};
  }
  constexpr auto sum_value() && noexcept(::std::is_nothrow_constructible_v<sum<value_type>, value_type>
                                         && ::std::is_nothrow_move_constructible_v<sum<value_type>>) // extension
      -> optional<sum<value_type>>
    requires(not some_sum<value_type>)
  {
    using type = optional<sum<value_type>>;
    if (this->has_value())
      return type{::std::in_place, sum<value_type>(::std::move(*this).value())};
    else
      return type{::std::nullopt};
  }
  constexpr auto sum_value() & noexcept -> decltype(auto)
    requires(some_sum<value_type>)
  {
    return *this;
  }
  constexpr auto sum_value() const & noexcept -> decltype(auto)
    requires(some_sum<value_type>)
  {
    return *this;
  }
  constexpr auto sum_value() && noexcept -> decltype(auto)
    requires(some_sum<value_type>)
  {
    return ::std::move(*this);
  }
  constexpr auto sum_value() const && noexcept -> decltype(auto)
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

// Partial specialization for lvalue reference types, mirroring pfn::optional<T&>. The monadic
// operations delegate to the same fn::detail::_optional_base statics as optional<T>'s, which
// the underlying pfn reference base makes shallow: the callable always receives plain T&, and
// there is only ever one overload of each (no ref-qualifier/const overload set).
template <class T> class optional<T &> : private detail::_optional_base<T &> {
  static_assert(::pfn::detail::_is_valid_optional<T>);
  using _base = detail::_optional_base<T &>;

  // Allow sibling _optional_base instantiations to downcast into the private base.
  template <class, class> friend struct ::pfn::detail::_optional_base;
  template <class> friend struct ::fn::detail::_optional_base;

public:
  using value_type = T;
  // [optional.ref.iterators]: mirrors pfn::optional, with fn's own iterator type
  using iterator = detail::_optional_iterator<T>;

  // Constructors. Explicit forwarders to the base mirror pfn::optional.
  constexpr optional() noexcept = default;
  constexpr optional(::std::nullopt_t) noexcept : optional() {} // NOSONAR cpp:S1709 implicit per spec
  constexpr optional(optional const &rhs) noexcept = default;

  template <class Arg>
  constexpr explicit optional(::std::in_place_t, Arg &&arg) //
      noexcept(::std::is_nothrow_constructible_v<T &, Arg>) // extension
    requires ::std::is_constructible_v<T &, Arg>
      : _base(::std::in_place, FWD(arg))
  {
  }

  template <class U>
  constexpr explicit(not ::std::is_convertible_v<U, T &>)
      optional(U &&u) // NOSONAR cpp:S6458 _can_convert excludes self
      noexcept(::std::is_nothrow_constructible_v<T &, U>)
    requires(_base::template _can_convert<U>::value)
      : _base(::std::in_place, FWD(u))
  {
  }
  template <class U>
  constexpr explicit(not ::std::is_convertible_v<U &, T &>) optional(optional<U> &rhs) //
      noexcept(::std::is_nothrow_constructible_v<T &, U &>)
    requires(_base::template _can_convert_from<U, U &>::value)
      : _base(rhs)
  {
  }
  template <class U>
  constexpr explicit(not ::std::is_convertible_v<U const &, T &>) optional(optional<U> const &rhs) //
      noexcept(::std::is_nothrow_constructible_v<T &, U const &>)
    requires(_base::template _can_convert_from<U, U const &>::value)
      : _base(rhs)
  {
  }
  template <class U>
  constexpr explicit(not ::std::is_convertible_v<U, T &>) optional(optional<U> &&rhs) //
      noexcept(::std::is_nothrow_constructible_v<T &, U>)
    requires(_base::template _can_convert_from<U, U>::value)
      : _base(::std::move(rhs))
  {
  }
  template <class U>
  constexpr explicit(not ::std::is_convertible_v<U const, T &>) optional(optional<U> const &&rhs) //
      noexcept(::std::is_nothrow_constructible_v<T &, U const>)
    requires(_base::template _can_convert_from<U, U const>::value)
      : _base(::std::move(rhs))
  {
  }

  constexpr ~optional() = default;

  // Assignment
  constexpr optional &operator=(::std::nullopt_t) noexcept
  {
    this->reset();
    return *this;
  }
  constexpr optional &operator=(optional const &rhs) noexcept = default;

  // Emplace and reset inherited from _optional_base
  using _base::emplace;
  using _base::reset;

  // Swap; body delegates to _optional_base helper
  constexpr void swap(optional &rhs) noexcept { this->_swap_with(rhs); }

  // Iterator support inherited from _optional_base, mirrors pfn::optional
  using _base::begin;
  using _base::end;

  // Observers inherited from _optional_base
  using _base::has_value;
  using _base::operator bool;
  using _base::operator*;
  using _base::operator->;
  using _base::value;
  using _base::value_or;

  // Monadic operations. Bodies delegate to _optional_base static helpers.
  template <class F>
  constexpr auto and_then(F &&f) const                    //
      noexcept(noexcept(_base::_and_then(*this, FWD(f)))) // extension
      -> decltype(_base::_and_then(*this, FWD(f)))
  {
    return _base::_and_then(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform(F &&f) const                    //
      noexcept(noexcept(_base::_transform(*this, FWD(f)))) // extension
      -> decltype(_base::_transform(*this, FWD(f)))
  {
    return _base::_transform(*this, FWD(f));
  }
  template <class F>
  constexpr auto or_else(F &&f) const                        //
      noexcept(noexcept(_base::_or_else_ref(*this, FWD(f)))) // extension
      -> decltype(_base::_or_else_ref(*this, FWD(f)))
    requires ::std::invocable<F>
  {
    return _base::_or_else_ref(*this, FWD(f));
  }

private:
  // Direct-non-list-initializes the bound reference from the result of a callable; used by
  // the monadic functions implemented in _optional_base.
  template <class Fn, class... Args>
  constexpr explicit optional(::pfn::detail::_optional_from_invoke_t tag, Fn &&fn, Args &&...args) //
      noexcept(::std::is_nothrow_constructible_v<_base, ::pfn::detail::_optional_from_invoke_t, Fn, Args...>)
      : _base(tag, FWD(fn), FWD(args)...)
  {
  }
};

namespace detail {
// Deduction probe: deliberately declared without a definition (it is only named in an
// unevaluated context), and called qualified so that ADL cannot pull unrelated overloads.
template <class U> void _derived_from_optional(::fn::optional<U> const &);

template <class T>
concept _is_derived_from_optional = requires(T const &t) { ::fn::detail::_derived_from_optional(t); };
} // namespace detail

// Comparison operators, the same full set as pfn::optional's (whose namespace-scope templates
// do not apply here, since fn::optional does not derive from pfn::optional).

// Relational operators
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
template <class T, class U>
constexpr bool operator!=(optional<T> const &x, optional<U> const &y) //
    noexcept(::pfn::detail::_ne_bool_noexcept<T, U>)                  // extension
  requires ::pfn::detail::_ne_bool<T, U>
{
  if (x.has_value() != y.has_value())
    return true;
  if (not x.has_value())
    return false;
  return *x != *y;
}
template <class T, class U>
constexpr bool operator<(optional<T> const &x, optional<U> const &y) //
    noexcept(::pfn::detail::_lt_bool_noexcept<T, U>)                 // extension
  requires ::pfn::detail::_lt_bool<T, U>
{
  if (not y.has_value())
    return false;
  if (not x.has_value())
    return true;
  return *x < *y;
}
template <class T, class U>
constexpr bool operator>(optional<T> const &x, optional<U> const &y) //
    noexcept(::pfn::detail::_gt_bool_noexcept<T, U>)                 // extension
  requires ::pfn::detail::_gt_bool<T, U>
{
  if (not x.has_value())
    return false;
  if (not y.has_value())
    return true;
  return *x > *y;
}
template <class T, class U>
constexpr bool operator<=(optional<T> const &x, optional<U> const &y) //
    noexcept(::pfn::detail::_le_bool_noexcept<T, U>)                  // extension
  requires ::pfn::detail::_le_bool<T, U>
{
  if (not x.has_value())
    return true;
  if (not y.has_value())
    return false;
  return *x <= *y;
}
template <class T, class U>
constexpr bool operator>=(optional<T> const &x, optional<U> const &y) //
    noexcept(::pfn::detail::_ge_bool_noexcept<T, U>)                  // extension
  requires ::pfn::detail::_ge_bool<T, U>
{
  if (not y.has_value())
    return true;
  if (not x.has_value())
    return false;
  return *x >= *y;
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
  requires(not detail::_is_some_optional<U &>) && ::pfn::detail::_eq_bool<T, U>
{
  return x.has_value() ? *x == v : false;
}
template <class T, class U>
constexpr bool operator==(T const &v, optional<U> const &x) //
    noexcept(::pfn::detail::_eq_bool_noexcept<T, U>)        // extension
  requires(not detail::_is_some_optional<T &>) && ::pfn::detail::_eq_bool<T, U>
{
  return x.has_value() ? v == *x : false;
}
template <class T, class U>
constexpr bool operator!=(optional<T> const &x, U const &v) //
    noexcept(::pfn::detail::_ne_bool_noexcept<T, U>)        // extension
  requires(not detail::_is_some_optional<U &>) && ::pfn::detail::_ne_bool<T, U>
{
  return x.has_value() ? *x != v : true;
}
template <class T, class U>
constexpr bool operator!=(T const &v, optional<U> const &x) //
    noexcept(::pfn::detail::_ne_bool_noexcept<T, U>)        // extension
  requires(not detail::_is_some_optional<T &>) && ::pfn::detail::_ne_bool<T, U>
{
  return x.has_value() ? v != *x : true;
}
template <class T, class U>
constexpr bool operator<(optional<T> const &x, U const &v) //
    noexcept(::pfn::detail::_lt_bool_noexcept<T, U>)       // extension
  requires(not detail::_is_some_optional<U &>) && ::pfn::detail::_lt_bool<T, U>
{
  return x.has_value() ? *x < v : true;
}
template <class T, class U>
constexpr bool operator<(T const &v, optional<U> const &x) //
    noexcept(::pfn::detail::_lt_bool_noexcept<T, U>)       // extension
  requires(not detail::_is_some_optional<T &>) && ::pfn::detail::_lt_bool<T, U>
{
  return x.has_value() ? v < *x : false;
}
template <class T, class U>
constexpr bool operator>(optional<T> const &x, U const &v) //
    noexcept(::pfn::detail::_gt_bool_noexcept<T, U>)       // extension
  requires(not detail::_is_some_optional<U &>) && ::pfn::detail::_gt_bool<T, U>
{
  return x.has_value() ? *x > v : false;
}
template <class T, class U>
constexpr bool operator>(T const &v, optional<U> const &x) //
    noexcept(::pfn::detail::_gt_bool_noexcept<T, U>)       // extension
  requires(not detail::_is_some_optional<T &>) && ::pfn::detail::_gt_bool<T, U>
{
  return x.has_value() ? v > *x : true;
}
template <class T, class U>
constexpr bool operator<=(optional<T> const &x, U const &v) //
    noexcept(::pfn::detail::_le_bool_noexcept<T, U>)        // extension
  requires(not detail::_is_some_optional<U &>) && ::pfn::detail::_le_bool<T, U>
{
  return x.has_value() ? *x <= v : true;
}
template <class T, class U>
constexpr bool operator<=(T const &v, optional<U> const &x) //
    noexcept(::pfn::detail::_le_bool_noexcept<T, U>)        // extension
  requires(not detail::_is_some_optional<T &>) && ::pfn::detail::_le_bool<T, U>
{
  return x.has_value() ? v <= *x : false;
}
template <class T, class U>
constexpr bool operator>=(optional<T> const &x, U const &v) //
    noexcept(::pfn::detail::_ge_bool_noexcept<T, U>)        // extension
  requires(not detail::_is_some_optional<U &>) && ::pfn::detail::_ge_bool<T, U>
{
  return x.has_value() ? *x >= v : false;
}
template <class T, class U>
constexpr bool operator>=(T const &v, optional<U> const &x) //
    noexcept(::pfn::detail::_ge_bool_noexcept<T, U>)        // extension
  requires(not detail::_is_some_optional<T &>) && ::pfn::detail::_ge_bool<T, U>
{
  return x.has_value() ? v >= *x : true;
}
template <class T, class U>
  requires(not detail::_is_derived_from_optional<U>) && ::std::three_way_comparable_with<T, U>
constexpr ::std::compare_three_way_result_t<T, U> operator<=>(optional<T> const &x, U const &v)
{
  return x.has_value() ? *x <=> v : ::std::strong_ordering::less;
}

// Specialized algorithms
template <class T>
constexpr void swap(optional<T> &x, optional<T> &y) noexcept(noexcept(x.swap(y)))
  requires(::std::is_reference_v<T> || (::std::is_move_constructible_v<T> && ::std::is_swappable_v<T>))
{
  x.swap(y);
}

// The leading defaulted non-type parameter mirrors pfn::make_optional: an explicit
// template-argument-list beginning with a type template-argument always selects an in_place
// overload below (for U = X& there is nothing this overload could do: decay_t strips the
// reference and would silently copy the referent).
template <int = 0, class T>
constexpr optional<::std::decay_t<T>> make_optional(T &&v)                      //
    noexcept(::std::is_nothrow_constructible_v<optional<::std::decay_t<T>>, T>) // extension
{
  return optional<::std::decay_t<T>>(FWD(v));
}
template <class T, class... Args>
constexpr optional<T> make_optional(Args &&...args)         //
    noexcept(::std::is_nothrow_constructible_v<T, Args...>) // extension
{
  return optional<T>(::std::in_place, FWD(args)...);
}
template <class T, class U, class... Args>
constexpr optional<T> make_optional(::std::initializer_list<U> il, Args &&...args)        //
    noexcept(::std::is_nothrow_constructible_v<T, ::std::initializer_list<U> &, Args...>) // extension
{
  return optional<T>(::std::in_place, il, FWD(args)...);
}

// Lifts for sum transformation functions
[[nodiscard]] constexpr auto sum_value(some_optional auto &&src) noexcept(noexcept(FWD(src).sum_value()))
    -> decltype(auto)
{
  return FWD(src).sum_value();
}

namespace detail {
// A named type, not a lambda: `operator&`'s specification has to name it, and a lambda cannot be
// spelled in an unevaluated operand before C++20's P0315, which our floor compilers predate.
struct _optional_efn final {
  [[nodiscard]] constexpr auto operator()(auto const &) const noexcept -> ::std::nullopt_t { return ::std::nullopt; }
};
} // namespace detail

template <some_optional Lh, some_optional Rh>
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) //
    noexcept(noexcept(::fn::detail::_join<fn::optional>(FWD(lh), FWD(rh), detail::_optional_efn{})))
{
  return ::fn::detail::_join<fn::optional>(FWD(lh), FWD(rh), detail::_optional_efn{});
}

} // namespace fn

namespace std {
// hash support, reusing pfn's [optional.hash] machinery (enabled iff hash<remove_const_t<T>>
// is enabled, hence never for reference types)
template <class T> struct hash<::fn::optional<T>> : ::pfn::detail::_optional_hash_base<::fn::optional<T>, T> {};

#if defined(__cpp_lib_format_ranges)
// range-format opt-out, mirroring pfn's [optional.syn] specialization
template <class T> constexpr range_format format_kind<::fn::optional<T>> = range_format::disabled;
#endif

// view opt-in, mirroring pfn's [optional.syn] specialization (nested-namespace block for
// MSVC, as in pfn/optional.hpp)
namespace ranges {
template <class T> constexpr bool enable_view<::fn::optional<T>> = true;
} // namespace ranges
} // namespace std

#endif // INCLUDE_FN_OPTIONAL
