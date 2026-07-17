// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_OPTIONAL
#define INCLUDE_FN_OPTIONAL

#include <pfn/optional.hpp>
#include <pfn/utility.hpp>

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

// `or_else` has two arms - the callback's own optional is returned, or its value type and T are
// widened into a sum - and `if constexpr` picks between them. A noexcept-specifier is an ordinary
// constant expression, so it cannot pick: the untaken arm's spelling would have to be well-formed
// too, and it is not. Hence a trait, whose constrained specializations mirror the body's arms. The
// unconstrained primary answers for a callback returning something that is not an optional at all -
// the body's static_assert is the diagnostic there, and it must not be pre-empted by a hard error
// in the specification.
template <typename T, typename Fn, typename ValArg> struct _nothrow_optional_or_else : ::std::false_type {};

template <typename T, typename Fn, typename ValArg>
  requires ::std::is_same_v<::std::remove_cvref_t<typename _apply_result<Fn>::type>, ::fn::optional<T>>
struct _nothrow_optional_or_else<T, Fn, ValArg>
    : ::std::bool_constant<_is_nothrow_applicable<Fn>::value
                               && ::std::is_nothrow_constructible_v<::fn::optional<T>, ::std::in_place_t, ValArg>> {};

template <typename T, typename Fn, typename ValArg>
  requires _is_some_optional<::std::remove_cvref_t<typename _apply_result<Fn>::type> &>
           && (not ::std::is_same_v<::std::remove_cvref_t<typename _apply_result<Fn>::type>, ::fn::optional<T>>)
struct _nothrow_optional_or_else<T, Fn, ValArg> {
  using type = ::std::remove_cvref_t<typename _apply_result<Fn>::type>;
  using new_type = ::fn::optional<sum_for<T, typename type::value_type>>;

  // an empty-sum value can never exist to be relocated: that arm is unreachable, and cannot throw
  static constexpr bool value                                          //
      = _is_nothrow_applicable<Fn>::value                              // the callback
        && _nothrow_initializable<new_type, ::std::in_place_t, ValArg> // self's value
        && (empty_sum<typename type::value_type>
            || _nothrow_initializable<new_type, ::std::in_place_t,
                                      decltype(::std::declval<type>().value())>); // its value
};

// Twin of the trait above for the empty-sum or_else arm, whose body has no engaged branch: the
// self's-value conjunct is dropped, and the callback (with the widening of its value) answers alone.
template <typename T, typename Fn> struct _nothrow_optional_or_else_empty : ::std::false_type {};

template <typename T, typename Fn>
  requires ::std::is_same_v<::std::remove_cvref_t<typename _apply_result<Fn>::type>, ::fn::optional<T>>
struct _nothrow_optional_or_else_empty<T, Fn> : ::std::bool_constant<_is_nothrow_applicable<Fn>::value> {};

template <typename T, typename Fn>
  requires _is_some_optional<::std::remove_cvref_t<typename _apply_result<Fn>::type> &>
           && (not ::std::is_same_v<::std::remove_cvref_t<typename _apply_result<Fn>::type>, ::fn::optional<T>>)
struct _nothrow_optional_or_else_empty<T, Fn> {
  using type = ::std::remove_cvref_t<typename _apply_result<Fn>::type>;
  using new_type = ::fn::optional<sum_for<T, typename type::value_type>>;

  static constexpr bool value             //
      = _is_nothrow_applicable<Fn>::value // the callback
        && _nothrow_initializable<new_type, ::std::in_place_t, decltype(::std::declval<type>().value())>; // its value
};

// Storage layer for ::fn::optional. Inherits the standard-conformant base from
// pfn, then hides the three monadic static helpers with sum-aware variants that
// materialise their result via `optional_policy::template type<U>`.
// The transform helpers hand pfn's _optional_from_invoke constructor a zero-argument
// thunk, so the result's contained value is direct-non-list-initialized from fn's own
// _apply (or sum::transform) result: no extra move, and immovable result types work.
// The statics carry the same extension noexcept as pfn's, computed through fn's own machinery:
// the callback of a sum/pack dispatch is invoked through `_apply`, not called directly, so it is
// `_is_nothrow_applicable` - not the std trait, which is false for a callable that is not directly
// applicable on a sum or a pack - that answers for it.
template <typename T> struct _optional_base : ::pfn::detail::_optional_base<T, optional_policy> {
  using _pfn_base = ::pfn::detail::_optional_base<T, optional_policy>;
  using _pfn_base::_pfn_base;

  // and_then
  template <typename Self, typename Fn>
  static constexpr auto _and_then(Self &&self, Fn &&fn)                               //
      noexcept(::fn::detail::_is_nothrow_applicable<Fn, decltype(*FWD(self))>::value) // extension
    requires ::fn::detail::_is_applicable<Fn, decltype(*FWD(self))>::value
  {
    using type = ::std::remove_cvref_t<typename ::fn::detail::_apply_result<Fn, decltype(*FWD(self))>::type>;
    static_assert(_is_some_optional<type &>);
    if (self.has_value())
      return ::fn::detail::_apply(FWD(fn), *FWD(self));
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

  // and_then, value type is the empty sum: a value can never be constructed, so the callback can
  // never be presented one - it is left alone, not invoked and not even instantiated, and the
  // result is *this unchanged.
  template <typename Self, typename Fn>
  static constexpr auto _and_then(Self &&self, Fn &&)                      //
      noexcept(::std::is_nothrow_constructible_v<::fn::optional<T>, Self>) // extension
      -> ::fn::optional<T>
    requires empty_sum<T> && ::std::is_constructible_v<::fn::optional<T>, Self>
  {
    return FWD(self);
  }

  // or_else (with value-widening into a sum)
  template <typename Self, typename Fn>
  static constexpr auto _or_else(Self &&self, Fn &&fn)                                      //
      noexcept(::fn::detail::_nothrow_optional_or_else<T, Fn, decltype(*FWD(self))>::value) // extension
    requires(not empty_sum<T>) && ::fn::detail::_is_applicable<Fn>::value
            && ::std::is_constructible_v<T, decltype(*FWD(self))>
  {
    using type = ::std::remove_cvref_t<typename ::fn::detail::_apply_result<Fn>::type>;
    static_assert(_is_some_optional<type &>);
    // compare whole optional types (not value_type) so optional<T&> instantiations, whose
    // value_type is the unqualified referent, take the same-type arm
    static_assert(::std::is_same_v<type, ::fn::optional<T>> || some_sum<T>);
    if constexpr (::std::is_same_v<type, ::fn::optional<T>>) {
      if (self.has_value())
        return type(::std::in_place, *FWD(self));
      else
        return ::fn::detail::_apply(FWD(fn));
    } else {
      using new_value_type = sum_for<T, typename type::value_type>;
      using new_type = ::fn::optional<new_value_type>;
      if (self.has_value())
        return new_type{::std::in_place, *FWD(self)};
      else {
        auto t = ::fn::detail::_apply(FWD(fn));
        if (t.has_value()) {
          if constexpr (not empty_sum<typename type::value_type>)
            return new_type{::std::in_place, ::std::move(t).value()};
          else
            ::pfn::unreachable(); // LCOV_EXCL_LINE
        } else
          return new_type{::std::nullopt};
      }
    }
  }

  // or_else, value type is the empty sum: never engaged, so the callback's optional is the whole
  // result - the general overload's engaged arms would have no value to copy. The widening
  // contract is unchanged: sum_for<sum<>, U> is U's own normal form.
  template <typename Self, typename Fn>
  static constexpr auto _or_else(Self &&, Fn &&fn)                          //
      noexcept(::fn::detail::_nothrow_optional_or_else_empty<T, Fn>::value) // extension
    requires empty_sum<T> && ::fn::detail::_is_applicable<Fn>::value
  {
    using type = ::std::remove_cvref_t<typename ::fn::detail::_apply_result<Fn>::type>;
    static_assert(_is_some_optional<type &>);
    if constexpr (::std::is_same_v<type, ::fn::optional<T>>)
      return ::fn::detail::_apply(FWD(fn));
    else {
      using new_value_type = sum_for<T, typename type::value_type>;
      using new_type = ::fn::optional<new_value_type>;
      auto t = ::fn::detail::_apply(FWD(fn));
      if (t.has_value())
        return new_type{::std::in_place, ::std::move(t).value()};
      else
        return new_type{::std::nullopt};
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

  // transform, value type is not a sum. In the noexcept spec, only the apply can throw: the
  // result is direct-non-list-initialized from the thunk's result (guaranteed elision).
  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&fn)                              //
      noexcept(::fn::detail::_is_nothrow_applicable<Fn, decltype(*FWD(self))>::value) // extension
    requires(not some_sum<T>) && ::fn::detail::_is_applicable_if<not some_sum<T>, Fn, decltype(*FWD(self))>::value
  {
    using new_value_type = ::std::remove_cv_t<typename ::fn::detail::_apply_result<Fn, decltype(*FWD(self))>::type>;
    using type = ::fn::optional<new_value_type>;
    if (self.has_value())
      return type(::pfn::detail::_optional_from_invoke,
                  [&fn, &self]() -> decltype(auto) { return ::fn::detail::_apply(FWD(fn), *FWD(self)); });
    else
      return type(::std::nullopt);
  }

  // transform, value type is a sum (delegates to sum::transform). The callback is constrained here,
  // in the immediate context: the deduced return type instantiates the body, so leaving it to
  // sum::transform's own constraint would make a bad callback a hard error instead of dropping the
  // candidate - and would poison overload resolution, since the losing candidates form their
  // signatures too.
  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&fn)  //
      noexcept(noexcept((*FWD(self)).transform(FWD(fn)))) // extension
    requires some_sum<T> && (not empty_sum<T>) && ::fn::detail::_typelist_applicable<Fn, decltype(*FWD(self))>
  {
    using new_value_type = decltype((*FWD(self)).transform(FWD(fn)));
    using type = ::fn::optional<new_value_type>;
    if (self.has_value())
      return type(::pfn::detail::_optional_from_invoke,
                  [&fn, &self]() -> decltype(auto) { return (*FWD(self)).transform(FWD(fn)); });
    else
      return type(::std::nullopt);
  }

  // apply: elimination over both states, both arms required outright - the engaged arm eliminates
  // the value through fn's own _apply (a pack or tuple-like payload by elements, a sum by
  // dispatch), the empty arm is invoked without it. Over an empty-sum value this overload set
  // needs no gate: sum<> has no apply, so _is_applicable and _apply_tagged answer false for every
  // Fn and the general overloads drop out.
  template <typename Self, typename Fn, typename... Args>
  static constexpr auto _apply(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(::fn::detail::_is_nothrow_applicable<Fn, decltype(*FWD(self)), Args...>::value
               && ::fn::detail::_is_nothrow_applicable<Fn, Args...>::value) // extension
      -> decltype(auto)
    requires ::fn::detail::_is_applicable<Fn, decltype(*FWD(self)), Args...>::value
             && ::fn::detail::_is_applicable<Fn, Args...>::value
  {
    // Both arms are viable here, so they must yield the same result type.
    static_assert(::std::is_same_v<typename ::fn::detail::_apply_result<Fn, decltype(*FWD(self)), Args...>::type,
                                   typename ::fn::detail::_apply_result<Fn, Args...>::type>);
    if (self.has_value())
      return ::fn::detail::_apply(FWD(fn), *FWD(self), FWD(args)...);
    else
      return ::fn::detail::_apply(FWD(fn), FWD(args)...);
  }

  template <typename Ret, typename Self, typename Fn, typename... Args>
  static constexpr auto _apply_r(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(::fn::detail::_is_nothrow_applicable_r<Ret, Fn, decltype(*FWD(self)), Args...>::value
               && ::fn::detail::_is_nothrow_applicable_r<Ret, Fn, Args...>::value) // extension
      -> Ret
    requires ::fn::detail::_is_applicable_r<Ret, Fn, decltype(*FWD(self)), Args...>::value
             && ::fn::detail::_is_applicable_r<Ret, Fn, Args...>::value
  {
    if (self.has_value())
      return ::fn::detail::_apply_r<Ret>(FWD(fn), *FWD(self), FWD(args)...);
    else
      return ::fn::detail::_apply_r<Ret>(FWD(fn), FWD(args)...);
  }

  // apply_type: the tagged form - the engaged arm receives ::std::in_place followed by the value
  // as _apply_tagged hands it over (a tuple-like value's elements form is the row's one
  // signature), the empty arm ::std::nullopt alone, passed as a prvalue like every tag; trailing
  // arguments follow either arm's content.
  template <typename Self, typename Fn, typename... Args>
  static constexpr auto _apply_type(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(noexcept(::fn::detail::_apply_tagged<::std::in_place_t>(FWD(fn), *FWD(self), FWD(args)...))
               && ::fn::detail::_is_nothrow_applicable<Fn, ::std::nullopt_t, Args &&...>::value) // extension
      -> decltype(auto)
    requires requires { ::fn::detail::_apply_tagged<::std::in_place_t>(FWD(fn), *FWD(self), FWD(args)...); }
             && ::fn::detail::_is_applicable<Fn, ::std::nullopt_t, Args &&...>::value
  {
    // Both arms are viable here, so they must yield the same result type.
    static_assert(
        ::std::is_same_v<decltype(::fn::detail::_apply_tagged<::std::in_place_t>(FWD(fn), *FWD(self), FWD(args)...)),
                         typename ::fn::detail::_apply_result<Fn, ::std::nullopt_t, Args &&...>::type>);
    if (self.has_value())
      return ::fn::detail::_apply_tagged<::std::in_place_t>(FWD(fn), *FWD(self), FWD(args)...);
    else
      return ::fn::detail::_apply(FWD(fn), ::std::nullopt_t{::std::nullopt}, FWD(args)...);
  }

  template <typename Ret, typename Self, typename Fn, typename... Args>
  static constexpr auto _apply_type_r(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(noexcept(::fn::detail::_apply_tagged_r<Ret, ::std::in_place_t>(FWD(fn), *FWD(self), FWD(args)...))
               && ::fn::detail::_is_nothrow_applicable_r<Ret, Fn, ::std::nullopt_t, Args &&...>::value) // extension
      -> Ret
    requires requires { ::fn::detail::_apply_tagged_r<Ret, ::std::in_place_t>(FWD(fn), *FWD(self), FWD(args)...); }
             && ::fn::detail::_is_applicable_r<Ret, Fn, ::std::nullopt_t, Args &&...>::value
  {
    if (self.has_value())
      return ::fn::detail::_apply_tagged_r<Ret, ::std::in_place_t>(FWD(fn), *FWD(self), FWD(args)...);
    else
      return ::fn::detail::_apply_r<Ret>(FWD(fn), ::std::nullopt_t{::std::nullopt}, FWD(args)...);
  }

  // apply, value type is the empty sum: never engaged, so the empty arm alone is exhaustive and
  // dispatch needs no branch; nothing names the engaged row, so an arm set carrying an arm for it
  // never instantiates it.
  template <typename Self, typename Fn, typename... Args>
  static constexpr auto _apply(Self &&, Fn &&fn, Args &&...args)         //
      noexcept(::fn::detail::_is_nothrow_applicable<Fn, Args...>::value) // extension
      -> decltype(auto)
    requires empty_sum<T> && ::fn::detail::_is_applicable<Fn, Args...>::value
  {
    return ::fn::detail::_apply(FWD(fn), FWD(args)...);
  }

  template <typename Ret, typename Self, typename Fn, typename... Args>
  static constexpr auto _apply_r(Self &&, Fn &&fn, Args &&...args)              //
      noexcept(::fn::detail::_is_nothrow_applicable_r<Ret, Fn, Args...>::value) // extension
      -> Ret
    requires empty_sum<T> && ::fn::detail::_is_applicable_r<Ret, Fn, Args...>::value
  {
    return ::fn::detail::_apply_r<Ret>(FWD(fn), FWD(args)...);
  }

  // apply_type, value type is the empty sum: the nullopt arm alone is exhaustive
  template <typename Self, typename Fn, typename... Args>
  static constexpr auto _apply_type(Self &&, Fn &&fn, Args &&...args)                         //
      noexcept(::fn::detail::_is_nothrow_applicable<Fn, ::std::nullopt_t, Args &&...>::value) // extension
      -> decltype(auto)
    requires empty_sum<T> && ::fn::detail::_is_applicable<Fn, ::std::nullopt_t, Args &&...>::value
  {
    return ::fn::detail::_apply(FWD(fn), ::std::nullopt_t{::std::nullopt}, FWD(args)...);
  }

  template <typename Ret, typename Self, typename Fn, typename... Args>
  static constexpr auto _apply_type_r(Self &&, Fn &&fn, Args &&...args)                              //
      noexcept(::fn::detail::_is_nothrow_applicable_r<Ret, Fn, ::std::nullopt_t, Args &&...>::value) // extension
      -> Ret
    requires empty_sum<T> && ::fn::detail::_is_applicable_r<Ret, Fn, ::std::nullopt_t, Args &&...>::value
  {
    return ::fn::detail::_apply_r<Ret>(FWD(fn), ::std::nullopt_t{::std::nullopt}, FWD(args)...);
  }

  // transform, value type is the empty sum: a value can never be constructed, so the callback can
  // never be presented one - it is left alone, not invoked and not even instantiated, the mapping
  // is the identity and the result is *this unchanged.
  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&)                     //
      noexcept(::std::is_nothrow_constructible_v<::fn::optional<T>, Self>) // extension
      -> ::fn::optional<T>
    requires empty_sum<T> && ::std::is_constructible_v<::fn::optional<T>, Self>
  {
    return FWD(self);
  }
};

} // namespace detail

template <typename T> class optional : private detail::_optional_base<T> { // NOSONAR cpp:S3624 base manages storage
  static_assert(::pfn::detail::_is_valid_optional<T>);
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
  constexpr optional &operator=(optional const &)                                                   //
      noexcept(::std::is_nothrow_copy_assignable_v<T> && ::std::is_nothrow_copy_constructible_v<T>) // extension
    requires(::std::is_copy_constructible_v<T> && ::std::is_copy_assignable_v<T>
             && ::std::is_trivially_copy_constructible_v<T> && ::std::is_trivially_copy_assignable_v<T>
             && ::std::is_trivially_destructible_v<T>)
  = default;
  constexpr optional &operator=(optional const &s)                                                  //
      noexcept(::std::is_nothrow_copy_assignable_v<T> && ::std::is_nothrow_copy_constructible_v<T>) // extension
    requires(::std::is_copy_constructible_v<T> && ::std::is_copy_assignable_v<T>
             && (not ::std::is_trivially_copy_constructible_v<T> || not ::std::is_trivially_copy_assignable_v<T>
                 || not ::std::is_trivially_destructible_v<T>))
  {
    this->_assign(static_cast<_base const &>(s));
    return *this;
  }
  constexpr optional &operator=(optional &&) //
      noexcept(::std::is_nothrow_move_assignable_v<T> && ::std::is_nothrow_move_constructible_v<T>)
    requires(::std::is_move_constructible_v<T> && ::std::is_move_assignable_v<T>
             && ::std::is_trivially_move_constructible_v<T> && ::std::is_trivially_move_assignable_v<T>
             && ::std::is_trivially_destructible_v<T>)
  = default;
  constexpr optional &operator=(optional &&s) //
      noexcept(::std::is_nothrow_move_assignable_v<T> && ::std::is_nothrow_move_constructible_v<T>)
    requires(::std::is_move_constructible_v<T> && ::std::is_move_assignable_v<T>
             && (not ::std::is_trivially_move_constructible_v<T> || not ::std::is_trivially_move_assignable_v<T>
                 || not ::std::is_trivially_destructible_v<T>))
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

  // Elimination over both states, mirroring sum's apply family: the engaged arm takes the value
  // unpacked as fn::apply would hand it over, the empty arm takes no value (apply) or the nullopt
  // tag alone (apply_type). Bodies delegate to _optional_base static helpers.
  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply(F &&f, Args &&...args) &        //
      noexcept(noexcept(_base::_apply(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply(*this, FWD(f), FWD(args)...))
  {
    return _base::_apply(*this, FWD(f), FWD(args)...);
  }
  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply(F &&f, Args &&...args) &&                    //
      noexcept(noexcept(_base::_apply(::std::move(*this), FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply(::std::move(*this), FWD(f), FWD(args)...))
  {
    return _base::_apply(::std::move(*this), FWD(f), FWD(args)...);
  }
  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply(F &&f, Args &&...args) const &  //
      noexcept(noexcept(_base::_apply(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply(*this, FWD(f), FWD(args)...))
  {
    return _base::_apply(*this, FWD(f), FWD(args)...);
  }
  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply(F &&f, Args &&...args) const &&              //
      noexcept(noexcept(_base::_apply(::std::move(*this), FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply(::std::move(*this), FWD(f), FWD(args)...))
  {
    return _base::_apply(::std::move(*this), FWD(f), FWD(args)...);
  }

  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_r(F &&f, Args &&...args) &                      //
      noexcept(noexcept(_base::template _apply_r<Ret>(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_r<Ret>(*this, FWD(f), FWD(args)...))
  {
    return _base::template _apply_r<Ret>(*this, FWD(f), FWD(args)...);
  }
  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_r(F &&f, Args &&...args) &&                                  //
      noexcept(noexcept(_base::template _apply_r<Ret>(::std::move(*this), FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_r<Ret>(::std::move(*this), FWD(f), FWD(args)...))
  {
    return _base::template _apply_r<Ret>(::std::move(*this), FWD(f), FWD(args)...);
  }
  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_r(F &&f, Args &&...args) const &                //
      noexcept(noexcept(_base::template _apply_r<Ret>(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_r<Ret>(*this, FWD(f), FWD(args)...))
  {
    return _base::template _apply_r<Ret>(*this, FWD(f), FWD(args)...);
  }
  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_r(F &&f, Args &&...args) const &&                            //
      noexcept(noexcept(_base::template _apply_r<Ret>(::std::move(*this), FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_r<Ret>(::std::move(*this), FWD(f), FWD(args)...))
  {
    return _base::template _apply_r<Ret>(::std::move(*this), FWD(f), FWD(args)...);
  }

  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply_type(F &&f, Args &&...args) &        //
      noexcept(noexcept(_base::_apply_type(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply_type(*this, FWD(f), FWD(args)...))
  {
    return _base::_apply_type(*this, FWD(f), FWD(args)...);
  }
  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply_type(F &&f, Args &&...args) &&                    //
      noexcept(noexcept(_base::_apply_type(::std::move(*this), FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply_type(::std::move(*this), FWD(f), FWD(args)...))
  {
    return _base::_apply_type(::std::move(*this), FWD(f), FWD(args)...);
  }
  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply_type(F &&f, Args &&...args) const &  //
      noexcept(noexcept(_base::_apply_type(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply_type(*this, FWD(f), FWD(args)...))
  {
    return _base::_apply_type(*this, FWD(f), FWD(args)...);
  }
  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply_type(F &&f, Args &&...args) const &&              //
      noexcept(noexcept(_base::_apply_type(::std::move(*this), FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply_type(::std::move(*this), FWD(f), FWD(args)...))
  {
    return _base::_apply_type(::std::move(*this), FWD(f), FWD(args)...);
  }

  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_type_r(F &&f, Args &&...args) &                      //
      noexcept(noexcept(_base::template _apply_type_r<Ret>(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_type_r<Ret>(*this, FWD(f), FWD(args)...))
  {
    return _base::template _apply_type_r<Ret>(*this, FWD(f), FWD(args)...);
  }
  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_type_r(F &&f, Args &&...args) &&                                  //
      noexcept(noexcept(_base::template _apply_type_r<Ret>(::std::move(*this), FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_type_r<Ret>(::std::move(*this), FWD(f), FWD(args)...))
  {
    return _base::template _apply_type_r<Ret>(::std::move(*this), FWD(f), FWD(args)...);
  }
  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_type_r(F &&f, Args &&...args) const &                //
      noexcept(noexcept(_base::template _apply_type_r<Ret>(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_type_r<Ret>(*this, FWD(f), FWD(args)...))
  {
    return _base::template _apply_type_r<Ret>(*this, FWD(f), FWD(args)...);
  }
  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_type_r(F &&f, Args &&...args) const &&                            //
      noexcept(noexcept(_base::template _apply_type_r<Ret>(::std::move(*this), FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_type_r<Ret>(::std::move(*this), FWD(f), FWD(args)...))
  {
    return _base::template _apply_type_r<Ret>(::std::move(*this), FWD(f), FWD(args)...);
  }

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

  // Elimination over both states; a reference optional always hands the referent over as T&.
  // Bodies delegate to _optional_base static helpers.
  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply(F &&f, Args &&...args) const    //
      noexcept(noexcept(_base::_apply(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply(*this, FWD(f), FWD(args)...))
  {
    return _base::_apply(*this, FWD(f), FWD(args)...);
  }
  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_r(F &&f, Args &&...args) const                  //
      noexcept(noexcept(_base::template _apply_r<Ret>(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_r<Ret>(*this, FWD(f), FWD(args)...))
  {
    return _base::template _apply_r<Ret>(*this, FWD(f), FWD(args)...);
  }
  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply_type(F &&f, Args &&...args) const    //
      noexcept(noexcept(_base::_apply_type(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply_type(*this, FWD(f), FWD(args)...))
  {
    return _base::_apply_type(*this, FWD(f), FWD(args)...);
  }
  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_type_r(F &&f, Args &&...args) const                  //
      noexcept(noexcept(_base::template _apply_type_r<Ret>(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_type_r<Ret>(*this, FWD(f), FWD(args)...))
  {
    return _base::template _apply_type_r<Ret>(*this, FWD(f), FWD(args)...);
  }

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
