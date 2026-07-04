// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_PFN_OPTIONAL
#define INCLUDE_PFN_OPTIONAL

#include <cassert>
#include <compare>
#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional> // For anything that is not std::optional or std::make_optional
#include <type_traits>
#include <utility>

#ifdef FWD
#pragma push_macro("FWD")
#define INCLUDE_PFN_OPTIONAL__POP_FWD
#undef FWD
#endif

// Also defined in fn/detail/fwd_macro.hpp but pfn/* headers are standalone
#define FWD(...) static_cast<decltype(__VA_ARGS__) &&>(__VA_ARGS__)

#ifdef ASSERT
#pragma push_macro("ASSERT")
#define INCLUDE_PFN_OPTIONAL__POP_ASSERT
#undef ASSERT
#endif

// LIBFN_ASSERT is a customization point for the user
#ifdef LIBFN_ASSERT
#define ASSERT(...) LIBFN_ASSERT(__VA_ARGS__)
#else
#define ASSERT(...) assert((__VA_ARGS__) == true)
#endif

namespace pfn {

// [optional.optional], class template optional
template <class T> class optional; // partially freestanding

// [optional.optional.ref], partial specialization of optional for lvalue reference types
template <class T> class optional<T &>; // partially freestanding

namespace detail {
// Deduction probe: deliberately declared without a definition (it is only named in an unevaluated
// context), and called qualified so that ADL cannot pull unrelated overloads into the probe.
template <class U> void _derived_from_optional(optional<U> const &);

template <class T>
concept _is_derived_from_optional = requires(T const &t) { // exposition only
  detail::_derived_from_optional(t);
};

// Helper used as noexcept(...) operand where we want to evaluate both:
// * noexcept of an expression itself (e.g. operator==) AND
// * noexcept of the expression's implicit conversion to bool
// May only be used in unevaluated contexts; any ODR-use will trigger a link error.
// Also declared in pfn/expected.hpp (a benign redeclaration; pfn/* headers are standalone).
constexpr bool _implicit_to_bool(bool) noexcept;

template <typename> constexpr bool _is_some_optional = false;
template <typename T> constexpr bool _is_some_optional<::pfn::optional<T>> = true;

// [optional.relops] and [optional.comp.with.t] Constraints: the comparison expression is
// well-formed and its result is convertible to bool. Operands are probed as const lvalues,
// matching both *x on a const optional (L const& collapses to plain V& when L is V&, i.e.
// for optional<V&>) and the value operand of [optional.comp.with.t].
template <class L, class R>
concept _eq_bool = requires(L const &l, R const &r) {
  { l == r } -> ::std::convertible_to<bool>;
};
template <class L, class R>
concept _ne_bool = requires(L const &l, R const &r) {
  { l != r } -> ::std::convertible_to<bool>;
};
template <class L, class R>
concept _lt_bool = requires(L const &l, R const &r) {
  { l < r } -> ::std::convertible_to<bool>;
};
template <class L, class R>
concept _gt_bool = requires(L const &l, R const &r) {
  { l > r } -> ::std::convertible_to<bool>;
};
template <class L, class R>
concept _le_bool = requires(L const &l, R const &r) {
  { l <= r } -> ::std::convertible_to<bool>;
};
template <class L, class R>
concept _ge_bool = requires(L const &l, R const &r) {
  { l >= r } -> ::std::convertible_to<bool>;
};

// Conditional noexcept of the same comparison expressions, for the comparison operators'
// extension noexcept clauses (probing the implicit conversion too, hence _implicit_to_bool).
template <class L, class R>
constexpr bool _eq_bool_noexcept
    = noexcept(_implicit_to_bool(::std::declval<L const &>() == ::std::declval<R const &>()));
template <class L, class R>
constexpr bool _ne_bool_noexcept
    = noexcept(_implicit_to_bool(::std::declval<L const &>() != ::std::declval<R const &>()));
template <class L, class R>
constexpr bool _lt_bool_noexcept
    = noexcept(_implicit_to_bool(::std::declval<L const &>() < ::std::declval<R const &>()));
template <class L, class R>
constexpr bool _gt_bool_noexcept
    = noexcept(_implicit_to_bool(::std::declval<L const &>() > ::std::declval<R const &>()));
template <class L, class R>
constexpr bool _le_bool_noexcept
    = noexcept(_implicit_to_bool(::std::declval<L const &>() <= ::std::declval<R const &>()));
template <class L, class R>
constexpr bool _ge_bool_noexcept
    = noexcept(_implicit_to_bool(::std::declval<L const &>() >= ::std::declval<R const &>()));

} // namespace detail

// [optional.relops], relational operators
template <class T, class U>
constexpr bool operator==(optional<T> const &, optional<U> const &) //
    noexcept(detail::_eq_bool_noexcept<T, U>)                       // extension
  requires detail::_eq_bool<T, U>;
template <class T, class U>
constexpr bool operator!=(optional<T> const &, optional<U> const &) //
    noexcept(detail::_ne_bool_noexcept<T, U>)                       // extension
  requires detail::_ne_bool<T, U>;
template <class T, class U>
constexpr bool operator<(optional<T> const &, optional<U> const &) //
    noexcept(detail::_lt_bool_noexcept<T, U>)                      // extension
  requires detail::_lt_bool<T, U>;
template <class T, class U>
constexpr bool operator>(optional<T> const &, optional<U> const &) //
    noexcept(detail::_gt_bool_noexcept<T, U>)                      // extension
  requires detail::_gt_bool<T, U>;
template <class T, class U>
constexpr bool operator<=(optional<T> const &, optional<U> const &) //
    noexcept(detail::_le_bool_noexcept<T, U>)                       // extension
  requires detail::_le_bool<T, U>;
template <class T, class U>
constexpr bool operator>=(optional<T> const &, optional<U> const &) //
    noexcept(detail::_ge_bool_noexcept<T, U>)                       // extension
  requires detail::_ge_bool<T, U>;
template <class T, ::std::three_way_comparable_with<T> U>
constexpr ::std::compare_three_way_result_t<T, U> operator<=>(optional<T> const &, optional<U> const &);

// [optional.nullops], comparison with nullopt
template <class T> constexpr bool operator==(optional<T> const &, ::std::nullopt_t) noexcept;
template <class T> constexpr ::std::strong_ordering operator<=>(optional<T> const &, ::std::nullopt_t) noexcept;

// [optional.comp.with.t], comparison with T
template <class T, class U>
constexpr bool operator==(optional<T> const &, U const &) //
    noexcept(detail::_eq_bool_noexcept<T, U>)             // extension
  requires(not detail::_is_some_optional<U>) && detail::_eq_bool<T, U>;
template <class T, class U>
constexpr bool operator==(T const &, optional<U> const &) //
    noexcept(detail::_eq_bool_noexcept<T, U>)             // extension
  requires(not detail::_is_some_optional<T>) && detail::_eq_bool<T, U>;
template <class T, class U>
constexpr bool operator!=(optional<T> const &, U const &) //
    noexcept(detail::_ne_bool_noexcept<T, U>)             // extension
  requires(not detail::_is_some_optional<U>) && detail::_ne_bool<T, U>;
template <class T, class U>
constexpr bool operator!=(T const &, optional<U> const &) //
    noexcept(detail::_ne_bool_noexcept<T, U>)             // extension
  requires(not detail::_is_some_optional<T>) && detail::_ne_bool<T, U>;
template <class T, class U>
constexpr bool operator<(optional<T> const &, U const &) //
    noexcept(detail::_lt_bool_noexcept<T, U>)            // extension
  requires(not detail::_is_some_optional<U>) && detail::_lt_bool<T, U>;
template <class T, class U>
constexpr bool operator<(T const &, optional<U> const &) //
    noexcept(detail::_lt_bool_noexcept<T, U>)            // extension
  requires(not detail::_is_some_optional<T>) && detail::_lt_bool<T, U>;
template <class T, class U>
constexpr bool operator>(optional<T> const &, U const &) //
    noexcept(detail::_gt_bool_noexcept<T, U>)            // extension
  requires(not detail::_is_some_optional<U>) && detail::_gt_bool<T, U>;
template <class T, class U>
constexpr bool operator>(T const &, optional<U> const &) //
    noexcept(detail::_gt_bool_noexcept<T, U>)            // extension
  requires(not detail::_is_some_optional<T>) && detail::_gt_bool<T, U>;
template <class T, class U>
constexpr bool operator<=(optional<T> const &, U const &) //
    noexcept(detail::_le_bool_noexcept<T, U>)             // extension
  requires(not detail::_is_some_optional<U>) && detail::_le_bool<T, U>;
template <class T, class U>
constexpr bool operator<=(T const &, optional<U> const &) //
    noexcept(detail::_le_bool_noexcept<T, U>)             // extension
  requires(not detail::_is_some_optional<T>) && detail::_le_bool<T, U>;
template <class T, class U>
constexpr bool operator>=(optional<T> const &, U const &) //
    noexcept(detail::_ge_bool_noexcept<T, U>)             // extension
  requires(not detail::_is_some_optional<U>) && detail::_ge_bool<T, U>;
template <class T, class U>
constexpr bool operator>=(T const &, optional<U> const &) //
    noexcept(detail::_ge_bool_noexcept<T, U>)             // extension
  requires(not detail::_is_some_optional<T>) && detail::_ge_bool<T, U>;
template <class T, class U>
  requires(not detail::_is_derived_from_optional<U>) && ::std::three_way_comparable_with<T, U>
constexpr ::std::compare_three_way_result_t<T, U> operator<=>(optional<T> const &, U const &);

// [optional.specalg], specialized algorithms
template <class T>
constexpr void swap(optional<T> &x, optional<T> &y) noexcept(noexcept(x.swap(y)))
  requires(::std::is_reference_v<T> || (::std::is_move_constructible_v<T> && ::std::is_swappable_v<T>));

// The leading defaulted non-type parameter implements this overload's [optional.specalg]
// Constraints: "the call to make_optional does not use an explicit template-argument-list
// that begins with a type template-argument" -- so make_optional<U>(...) always selects an
// in_place overload below (for U = X& there is nothing this overload could do: decay_t
// strips the reference and would silently copy the referent).
template <int = 0, class T>
constexpr optional<::std::decay_t<T>> make_optional(T &&v)                       //
    noexcept(::std::is_nothrow_constructible_v<optional<::std::decay_t<T>>, T>); // extension
template <class T, class... Args>
constexpr optional<T> make_optional(Args &&...args)          //
    noexcept(::std::is_nothrow_constructible_v<T, Args...>); // extension
template <class T, class U, class... Args>
constexpr optional<T> make_optional(::std::initializer_list<U> il, Args &&...args)         //
    noexcept(::std::is_nothrow_constructible_v<T, ::std::initializer_list<U> &, Args...>); // extension

namespace detail {

// Internal union wrapper for optional's value / no-value storage. This mirrors the
// strategy of `pfn::detail::_expected_union_t<void, E>` with the value and "empty"
// roles reversed: the engaged value lives in `v_`, and `e_` is a trivial placeholder
// for the disengaged state, so the union always has an active member (required for
// constexpr use). Copy/move ctors are defaulted iff `T` is trivially copy/move
// constructible; the destructor is defaulted iff `T` is trivially destructible.
// Tag selecting _optional_union_t's/_optional_base's "construct from any source exposing
// has_value()/operator*()" constructor, disambiguating it from the (bool, S&&) one below
// (which instead reads another union's raw v_ member directly).
constexpr inline struct _from_optional_t {
  explicit _from_optional_t() = default;
} _from_optional{};

// Tag selecting the "direct-non-list-initialize the contained value from the result of
// std::invoke" constructors, required by transform ([optional.monadic]/[optional.ref.monadic]):
// the specified initialization is exactly `U u(invoke(...))` -- guaranteed elision, so no
// extra move and an immovable U works -- which means the invoke expression itself must reach
// the contained value's initializer.
constexpr inline struct _optional_from_invoke_t {
  explicit _optional_from_invoke_t() = default;
} _optional_from_invoke{};

template <class T> union _optional_union_t {
  // _dummy_t placeholder, so the union always has an active member
  struct _dummy_t final {
    constexpr _dummy_t() noexcept = default;
  };

  using _value_t = T;
  T v_;
  _dummy_t e_; // disengaged-state placeholder

  template <typename S>
  constexpr explicit _optional_union_t(bool s, S &&src) //
      noexcept(::std::is_nothrow_constructible_v<T, decltype((FWD(src).v_))>)
  {
    if (s)
      ::std::construct_at(::std::addressof(v_), FWD(src).v_);
    else
      ::std::construct_at(::std::addressof(e_));
  }

  // Constructs from any source exposing has_value()/operator*() (e.g. a differently-typed
  // optional, value or reference) -- mirrors the (bool, S&&) ctor above but reads engagement
  // and the value through that public API instead of another union's raw v_ member, so it
  // works uniformly for optional<U&> sources too (a completely different internal
  // representation: a bare pointer, no union or set_ at all).
  template <typename S>
  constexpr explicit _optional_union_t(_from_optional_t /*ignored*/, S &&src) //
      noexcept(::std::is_nothrow_constructible_v<T, decltype(*FWD(src))>)
  {
    if (src.has_value())
      ::std::construct_at(::std::addressof(v_), *FWD(src));
    else
      ::std::construct_at(::std::addressof(e_));
  }

  template <typename Fn, typename... Args>
  constexpr explicit _optional_union_t(_optional_from_invoke_t /*ignored*/, Fn &&fn, Args &&...args) //
      noexcept(::std::is_nothrow_invocable_v<Fn, Args...>
               && ::std::is_nothrow_constructible_v<T, ::std::invoke_result_t<Fn, Args...>>)
      : v_(::std::invoke(FWD(fn), FWD(args)...))
  {
  }

  constexpr _optional_union_t(_optional_union_t const &) = delete;
  constexpr _optional_union_t(_optional_union_t const &) noexcept //
    requires(::std::is_trivially_copy_constructible_v<T>)
  = default;
  constexpr _optional_union_t(_optional_union_t &&) = delete;
  constexpr _optional_union_t(_optional_union_t &&) noexcept //
    requires(::std::is_trivially_move_constructible_v<T>)
  = default;
  constexpr _optional_union_t &operator=(_optional_union_t const &) = delete;
  constexpr _optional_union_t &operator=(_optional_union_t &&) = delete;

  template <class... Args>
  constexpr explicit _optional_union_t(::std::in_place_t /*ignored*/, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<T, Args...>)
    requires ::std::is_constructible_v<T, Args...>
      : v_(FWD(a)...)
  {
  }
  constexpr explicit _optional_union_t(::std::nullopt_t /*ignored*/) noexcept : e_{} {}

  constexpr ~_optional_union_t() noexcept
    requires(::std::is_trivially_destructible_v<T>)
  = default;
  constexpr ~_optional_union_t() noexcept {}

  // reinit, mirroring [expected.void.assign]'s direct construction: one of New/Old is
  // always the trivial `_dummy_t` (value <-> empty transition), so no strong-exception
  // snapshot of the old member is needed.
  template <typename New, typename Old, typename... Args>
  static constexpr void _reinit(New *newp, Old *oldp, Args &&...args) //
      noexcept(::std::is_nothrow_constructible_v<New, Args...>)
  {
    if constexpr (::std::is_same_v<New, _dummy_t>) {
      ::std::destroy_at(oldp);
      ::std::construct_at(newp); // Never throws, since New is _dummy_t
    } else if constexpr (::std::is_nothrow_constructible_v<New, Args...>) {
      ::std::destroy_at(oldp);
      ::std::construct_at(newp, ::std::forward<Args>(args)...); // Never throws
    } else {
      ::std::destroy_at(oldp);
      try {
        ::std::construct_at(newp, ::std::forward<Args>(args)...);
      } catch (...) {
        ::std::construct_at(oldp); // Never throws, since Old is _dummy_t
        throw;
      }
    }
  }
};

template <typename> constexpr bool _is_optional_union = false;
template <typename T> constexpr bool _is_optional_union<_optional_union_t<T>> = true;

// [optional.optional.general]: only lvalue references and complete non-array object types are
// valid contained types; also the Mandates of transform's result type ([optional.monadic]).
template <typename T>
constexpr bool _is_valid_optional =                                                         //
    (::std::is_lvalue_reference_v<T>                                                        //
     || (::std::is_object_v<T> && ::std::is_destructible_v<T> && not ::std::is_array_v<T>)) //
    && not ::std::is_same_v<::std::remove_cv_t<T>, ::std::in_place_t>                       //
    && not ::std::is_same_v<::std::remove_cv_t<T>, ::std::nullopt_t>;

// Shared implementation base class for ::pfn::optional. Members are public since inheritance is
// private. Not used by ::pfn::optional<T&> which models a pointer and does not need `bool set_`.
template <class T, class Policy> struct _optional_base {
  using _storage_t = _optional_union_t<T>;
  using _value_t = _storage_t::_value_t; // == T
  _storage_t storage_;
  bool set_;

  template <class U>
  using _can_convert = ::std::bool_constant<                                               //
      ::std::is_constructible_v<T, U>                                                      //
      && not ::std::is_same_v<::std::remove_cvref_t<U>, ::std::in_place_t>                 //
      && not ::std::is_same_v<::std::remove_cvref_t<U>, typename Policy::template type<T>> //
      && (not ::std::is_same_v<bool, ::std::remove_cv_t<T>>                                //
          || not Policy::template is_specialization<::std::remove_cvref_t<U>>)>;

  template <class U>
  using _can_assign = ::std::bool_constant<                                                  //
      ::std::is_constructible_v<T, U>                                                        //
          && ::std::is_assignable_v<T &, U>                                                  //
      && not ::std::conjunction_v<::std::is_scalar<T>, ::std::is_same<T, ::std::decay_t<U>>> //
      && not ::std::is_same_v<::std::remove_cvref_t<U>, typename Policy::template type<T>>>;

  // [optional.ctor], implementation of converts-from-any-cvref
  template <class W>
  using _converts_from_an_cvrev = ::std::bool_constant<::std::disjunction_v<      //
      ::std::is_constructible<T, W &>, ::std::is_convertible<W &, T>,             //
      ::std::is_constructible<T, W>, ::std::is_convertible<W, T>,                 //
      ::std::is_constructible<T, W const &>, ::std::is_convertible<W const &, T>, //
      ::std::is_constructible<T, W const>, ::std::is_convertible<W const, T>>>;

  template <typename U>
  using _can_copy_convert = ::std::bool_constant<       //
      ::std::is_constructible_v<T, U const &>           //
      && (::std::is_same_v<bool, ::std::remove_cv_t<T>> //
          || not _converts_from_an_cvrev<typename Policy::template type<U>>::value)>;

  template <typename U>
  using _can_move_convert = ::std::bool_constant<       //
      ::std::is_constructible_v<T, U>                   //
      && (::std::is_same_v<bool, ::std::remove_cv_t<T>> //
          || not _converts_from_an_cvrev<typename Policy::template type<U>>::value)>;

  template <typename U, typename UF>
  using _can_copy_assign_detail = ::std::bool_constant< //
      ::std::is_constructible_v<T, UF>                  //
          && ::std::is_assignable_v<T &, UF>
      && not _converts_from_an_cvrev<typename Policy::template type<U>>::value      //
      && not ::std::is_assignable_v<T &, typename Policy::template type<U> &>       //
      && not ::std::is_assignable_v<T &, typename Policy::template type<U> &&>      //
      && not ::std::is_assignable_v<T &, typename Policy::template type<U> const &> //
      && not ::std::is_assignable_v<T &, typename Policy::template type<U> const &&>>;

  template <typename U> using _can_copy_assign = _can_copy_assign_detail<U, U const &>;
  template <typename U> using _can_move_assign = _can_copy_assign_detail<U, U>;

  // Shared construction helper: selects the union's active member from `s`. Used by
  // the wrapper's copy/move and (later) the converting constructors.
  template <typename S>
  constexpr explicit _optional_base(bool s, S &&src)
    requires(_is_optional_union<::std::remove_cvref_t<S>>)
      : storage_(s, FWD(src)), set_(s)
  {
  }

  template <class... Args>
  constexpr explicit _optional_base(::std::in_place_t /*ignored*/, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<_storage_t, ::std::in_place_t, Args...>)
    requires ::std::is_constructible_v<_storage_t, ::std::in_place_t, Args...>
      : storage_(::std::in_place, FWD(a)...), set_(true)
  {
  }
  template <class U, class... Args>
  constexpr explicit _optional_base(::std::in_place_t /*ignored*/, ::std::initializer_list<U> il, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<_storage_t, ::std::in_place_t, ::std::initializer_list<U> &, Args...>)
    requires ::std::is_constructible_v<_storage_t, ::std::in_place_t, ::std::initializer_list<U> &, Args...>
      : storage_(::std::in_place, il, FWD(a)...), set_(true)
  {
  }
  template <typename Fn, typename... Args>
  constexpr explicit _optional_base(_optional_from_invoke_t tag, Fn &&fn, Args &&...args) //
      noexcept(::std::is_nothrow_constructible_v<_storage_t, _optional_from_invoke_t, Fn, Args...>)
      : storage_(tag, FWD(fn), FWD(args)...), set_(true)
  {
  }
  constexpr explicit _optional_base(::std::nullopt_t /*ignored*/) noexcept //
      : storage_(::std::nullopt), set_(false)
  {
  }

  // Shared construction helper for any source exposing has_value()/operator*() -- i.e.
  // another optional, same or differently typed, value or reference. Delegates to the
  // union's own _from_optional_t ctor, which reads that public observer API directly rather
  // than reaching into the source's storage, so it works uniformly for both optional<U> (a
  // discriminated union) and optional<U&> (a bare pointer, no union or `set_` at all)
  // sources alike. Used by the converting ctors below; _assign_from applies the same
  // public-API reads in its own body (via _reinit), without going through this ctor.
  template <typename S>
  constexpr explicit _optional_base(_from_optional_t tag, S &&s) //
      noexcept(::std::is_nothrow_constructible_v<_storage_t, _from_optional_t, S>)
      : storage_(tag, FWD(s)), set_(s.has_value())
  {
  }

  // [optional.ctor], converting constructors from a differently-typed optional<U>.
  template <class U>
  constexpr explicit(not ::std::is_convertible_v<U const &, T>)
      _optional_base(typename Policy::template type<U> const &s) //
      noexcept(::std::is_nothrow_constructible_v<T, U const &>)  // extension
    requires(_can_copy_convert<U>::value)
      : _optional_base(_from_optional, s)
  {
  }
  template <class U>
  constexpr explicit(not ::std::is_convertible_v<U, T>) _optional_base(typename Policy::template type<U> &&s) //
      noexcept(::std::is_nothrow_constructible_v<T, U>) // extension
    requires(_can_move_convert<U>::value)
      : _optional_base(_from_optional, ::std::move(s))
  {
  }

  constexpr _optional_base(_optional_base const &) noexcept = default;
  constexpr _optional_base(_optional_base &&) noexcept = default;
  constexpr _optional_base &operator=(_optional_base const &) = delete;
  constexpr _optional_base &operator=(_optional_base &&) = delete;

  constexpr ~_optional_base() //
    requires(::std::is_trivially_destructible_v<_value_t>)
  = default;
  constexpr ~_optional_base() //
    requires(not ::std::is_trivially_destructible_v<_value_t>)
  {
    if (set_)
      ::std::destroy_at(::std::addressof(storage_.v_));
  }

  // [optional.mod] reset; also the disengage step of operator=(nullopt_t) and emplace
  constexpr void reset() noexcept
  {
    if (set_) {
      _storage_t::_reinit(::std::addressof(storage_.e_), ::std::addressof(storage_.v_));
      set_ = false;
    }
  }

  // Assignment body shared by the public optional operator= overloads, which keep their
  // constraints/noexcept clauses and forward `s` here as an lvalue or rvalue.
  constexpr void _assign(auto &&s)
  {
    if (set_ && s.set_) {
      storage_.v_ = FWD(s).storage_.v_;
    } else if (set_) {
      _storage_t::_reinit(::std::addressof(storage_.e_), ::std::addressof(storage_.v_), FWD(s).storage_.e_);
      set_ = false;
    } else if (s.set_) {
      _storage_t::_reinit(::std::addressof(storage_.v_), ::std::addressof(storage_.e_), FWD(s).storage_.v_);
      set_ = true;
    } else {
      // no effect: both are disengaged, so the trivial `_dummy_t` is already active
    }
  }

  template <class U> constexpr void _assign_value(U &&s)
  {
    if (set_) {
      storage_.v_ = FWD(s);
    } else {
      _storage_t::_reinit(::std::addressof(storage_.v_), ::std::addressof(storage_.e_), FWD(s));
      set_ = true;
    }
  }

  // Converting-assignment body shared by the public optional operator= overloads. Reads `s`
  // through its public has_value()/operator*() (same reasoning as the converting ctors'
  // shared helper above: works uniformly for optional<U> and optional<U&> sources alike),
  // and reuses _reinit for *this's own storage exactly like _assign above.
  template <typename S> constexpr void _assign_from(S &&s)
  {
    if (set_ && s.has_value()) {
      storage_.v_ = *FWD(s);
    } else if (set_) {
      _storage_t::_reinit(::std::addressof(storage_.e_), ::std::addressof(storage_.v_));
      set_ = false;
    } else if (s.has_value()) {
      _storage_t::_reinit(::std::addressof(storage_.v_), ::std::addressof(storage_.e_), *FWD(s));
      set_ = true;
    }
    // else: both disengaged, no effect
  }

  // Swap body shared by the public optional swap. Same-state mirrors _expected_base::_swap_with;
  // cross-state reuses _reinit: the empty side gains the value first (and _reinit restores its
  // dummy if that move throws), the donor is destroyed only afterwards -- [optional.swap]'s
  // construct-then-destroy order and its unchanged-engagement-on-exception guarantee.
  constexpr void _swap_with(_optional_base &rhs)
  {
    if (set_ == rhs.set_) {
      if (set_) {
        using ::std::swap;
        swap(storage_.v_, rhs.storage_.v_);
      }
      // both disengaged: no effect
    } else if (set_) {
      _storage_t::_reinit(::std::addressof(rhs.storage_.v_), ::std::addressof(rhs.storage_.e_),
                          ::std::move(storage_.v_));
      rhs.set_ = true;
      _storage_t::_reinit(::std::addressof(storage_.e_), ::std::addressof(storage_.v_));
      set_ = false;
    } else {
      rhs._swap_with(*this);
    }
  }

  template <class... Args>
  constexpr T &emplace(Args &&...args) //
    requires ::std::is_constructible_v<T, Args...>
  {
    reset();
    _storage_t::_reinit(::std::addressof(storage_.v_), ::std::addressof(storage_.e_), FWD(args)...);
    set_ = true;
    return storage_.v_;
  }
  template <class U, class... Args>
  constexpr T &emplace(::std::initializer_list<U> il, Args &&...args) //
    requires ::std::is_constructible_v<T, ::std::initializer_list<U> &, Args...>
  {
    reset();
    _storage_t::_reinit(::std::addressof(storage_.v_), ::std::addressof(storage_.e_), il, FWD(args)...);
    set_ = true;
    return storage_.v_;
  }

  // [optional.observe], observers
  constexpr T const *operator->() const noexcept
  {
    ASSERT(set_); // LCOV_EXCL_LINE
    return ::std::addressof(storage_.v_);
  }
  constexpr T *operator->() noexcept
  {
    ASSERT(set_); // LCOV_EXCL_LINE
    return ::std::addressof(storage_.v_);
  }
  constexpr T const &operator*() const & noexcept { return *(this->operator->()); }
  constexpr T &operator*() & noexcept { return *(this->operator->()); }
  constexpr T const &&operator*() const && noexcept { return ::std::move(*(this->operator->())); }
  constexpr T &&operator*() && noexcept { return ::std::move(*(this->operator->())); }
  constexpr explicit operator bool() const noexcept { return set_; }
  constexpr bool has_value() const noexcept { return set_; }

  constexpr T const &value() const &
  {
    if (not set_)
      throw ::std::bad_optional_access();
    return storage_.v_;
  }
  constexpr T &value() &
  {
    if (not set_)
      throw ::std::bad_optional_access();
    return storage_.v_;
  }
  constexpr T const &&value() const &&
  {
    if (not set_)
      throw ::std::bad_optional_access();
    return ::std::move(storage_.v_);
  }
  constexpr T &&value() &&
  {
    if (not set_)
      throw ::std::bad_optional_access();
    return ::std::move(storage_.v_);
  }

  template <class U = ::std::remove_cv_t<T>> constexpr T value_or(U &&v) const &
  {
    static_assert(::std::is_copy_constructible_v<T> && ::std::is_convertible_v<U &&, T>);
    return set_ ? storage_.v_ : static_cast<T>(::std::forward<U>(v));
  }
  template <class U = ::std::remove_cv_t<T>> constexpr T value_or(U &&v) &&
  {
    static_assert(::std::is_move_constructible_v<T> && ::std::is_convertible_v<U &&, T>);
    return set_ ? ::std::move(storage_.v_) : static_cast<T>(::std::forward<U>(v));
  }

  // [optional.monadic] bodies, shared by the public optional's ref-qualified overload sets.
  // Self is the public optional (not this base), so `*FWD(self)` spells the draft's exact
  // decltype((val)) / decltype(std::move(val)) value category and everything is reached
  // through public API -- same reasoning as the _from_optional_t constructors above.
  template <typename Self, typename Fn>
  static constexpr auto _and_then(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn, decltype(*FWD(self))>)
    requires ::std::is_invocable_v<Fn, decltype(*FWD(self))>
  {
    using result_t = ::std::remove_cvref_t<::std::invoke_result_t<Fn, decltype(*FWD(self))>>;
    static_assert(Policy::template is_specialization<result_t>); // [optional.monadic] Mandates
    if (self.has_value()) {
      return ::std::invoke(FWD(fn), *FWD(self));
    }
    return result_t();
  }

  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&fn) //
      noexcept(
          ::std::is_nothrow_invocable_v<Fn, decltype(*FWD(self))>
          && ::std::is_nothrow_constructible_v<::std::remove_cv_t<::std::invoke_result_t<Fn, decltype(*FWD(self))>>,
                                               ::std::invoke_result_t<Fn, decltype(*FWD(self))>>)
    requires ::std::is_invocable_v<Fn, decltype(*FWD(self))>
  {
    using value_t = ::std::remove_cv_t<::std::invoke_result_t<Fn, decltype(*FWD(self))>>;
    static_assert(_is_valid_optional<value_t>); // [optional.monadic] Mandates
    using result_t = typename Policy::template type<value_t>;
    if (self.has_value()) {
      return result_t(_optional_from_invoke, FWD(fn), *FWD(self));
    }
    return result_t();
  }

  template <typename Self, typename Fn>
  static constexpr auto _or_else(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn>
               && ::std::is_nothrow_constructible_v<typename Policy::template type<T>, Self>)
    requires(::std::is_invocable_v<Fn> && ::std::is_constructible_v<typename Policy::template type<T>, Self>)
  {
    using result_t = typename Policy::template type<T>;
    static_assert(::std::is_same_v<::std::remove_cvref_t<::std::invoke_result_t<Fn>>, result_t>); // Mandates
    if (self.has_value()) {
      return result_t(FWD(self)); // the draft's `return *this;` / `return std::move(*this);`
    }
    return ::std::invoke(FWD(fn));
  }
};

// optional<T&> needs its own base: the referent is held directly as a pointer (nullptr
// encodes the disengaged state), so there is no value union, no discriminant flag, and
// every special member is implicitly trivial.
//
// TODO reference_constructs_from_temporary_v is a C++23 trait with no portable C++20
// fallback, so the dangling-reference guards of [optional.ref.ctor] (constraints and
// deleted overloads for binding a temporary) and of [optional.ref.assign]'s emplace are
// deferred; until then such constructions compile and dangle.
template <class T, class Policy> struct _optional_base<T &, Policy> {
  using _value_t = T &;
  T *v_ = nullptr;

  // [optional.ref.expos], exposition only helper. Binds a reference to `u` (through whatever conversion T&
  // requires -- possibly a throwing user conversion operator) and stores its address.
  template <class U> constexpr void _convert_ref_init_val(U &&u) noexcept(::std::is_nothrow_constructible_v<T &, U>)
  {
    // Workaround for MSVC error C2440, same semantics as the specified `T &r(FWD(u));` ([expr.static.cast]/4)
    v_ = ::std::addressof(static_cast<T &>(FWD(u)));
  }

  template <class U>
  using _can_convert = ::std::bool_constant<                                              //
      not ::std::is_same_v<::std::remove_cvref_t<U>, typename Policy::template type<T &>> //
      && not ::std::is_same_v<::std::remove_cvref_t<U>, ::std::in_place_t>                //
      && ::std::is_constructible_v<T &, U>>;

  // [optional.ref.ctor], shared constraint of the converting constructors from optional<U>;
  // UF is the U flavor the overload binds from (U&, U const&, U, U const).
  template <class U, class UF>
  using _can_convert_from = ::std::bool_constant<                                    //
      not ::std::is_same_v<::std::remove_cv_t<T>, typename Policy::template type<U>> //
      && not ::std::is_same_v<T &, U>                                                //
      && ::std::is_constructible_v<T &, UF>>;

  constexpr _optional_base() noexcept = default;
  constexpr explicit _optional_base(::std::nullopt_t /*ignored*/) noexcept {}

  template <class Arg>
  constexpr explicit _optional_base(::std::in_place_t /*ignored*/, Arg &&arg) //
      noexcept(::std::is_nothrow_constructible_v<T &, Arg>)                   // extension
    requires ::std::is_constructible_v<T &, Arg>
  {
    _convert_ref_init_val(FWD(arg));
  }

  template <typename Fn, typename... Args>
  constexpr explicit _optional_base(_optional_from_invoke_t /*ignored*/, Fn &&fn, Args &&...args) //
      noexcept(::std::is_nothrow_invocable_v<Fn, Args...>
               && ::std::is_nothrow_constructible_v<T &, ::std::invoke_result_t<Fn, Args...>>)
  {
    _convert_ref_init_val(::std::invoke(FWD(fn), FWD(args)...));
  }

  // [optional.ref.ctor], converting constructors from a differently-typed optional<U>: four
  // overloads (unlike optional<T>'s two), since both the constness and the value category of
  // the source select which U flavor T& must bind.
  template <class U>
  constexpr explicit(not ::std::is_convertible_v<U &, T &>) _optional_base(typename Policy::template type<U> &rhs) //
      noexcept(::std::is_nothrow_constructible_v<T &, U &>)
    requires(_can_convert_from<U, U &>::value)
  {
    if (rhs.has_value())
      _convert_ref_init_val(*rhs);
  }
  template <class U>
  constexpr explicit(not ::std::is_convertible_v<U const &, T &>)
      _optional_base(typename Policy::template type<U> const &rhs) //
      noexcept(::std::is_nothrow_constructible_v<T &, U const &>)
    requires(_can_convert_from<U, U const &>::value)
  {
    if (rhs.has_value())
      _convert_ref_init_val(*rhs);
  }
  template <class U>
  constexpr explicit(not ::std::is_convertible_v<U, T &>) _optional_base(typename Policy::template type<U> &&rhs) //
      noexcept(::std::is_nothrow_constructible_v<T &, U>)
    requires(_can_convert_from<U, U>::value)
  {
    if (rhs.has_value())
      _convert_ref_init_val(*::std::move(rhs));
  }
  template <class U>
  constexpr explicit(not ::std::is_convertible_v<U const, T &>)
      _optional_base(typename Policy::template type<U> const &&rhs) //
      noexcept(::std::is_nothrow_constructible_v<T &, U const>)
    requires(_can_convert_from<U, U const>::value)
  {
    if (rhs.has_value())
      _convert_ref_init_val(*::std::move(rhs));
  }

  // [optional.ref.assign]
  template <class U>
  constexpr T &emplace(U &&u) noexcept(::std::is_nothrow_constructible_v<T &, U>)
    requires ::std::is_constructible_v<T &, U>
  {
    _convert_ref_init_val(FWD(u));
    return *v_;
  }

  // [optional.ref.swap] and [optional.ref.mod]: the pointer representation makes both trivial
  constexpr void _swap_with(_optional_base &rhs) noexcept { ::std::swap(v_, rhs.v_); }
  constexpr void reset() noexcept { v_ = nullptr; }

  // [optional.ref.observe], observers. Unlike optional<T>, const does not propagate to the
  // referent: *this being const only means the stored pointer can't be rebound, not that T
  // becomes T const -- so these all return plain T&/T*, and there is only ever one overload
  // (no ref-qualifier/const overload set like optional<T>'s).
  constexpr T *operator->() const noexcept
  {
    ASSERT(v_ != nullptr); // LCOV_EXCL_LINE
    return v_;
  }
  constexpr T &operator*() const noexcept { return *(this->operator->()); }
  constexpr explicit operator bool() const noexcept { return v_ != nullptr; }
  constexpr bool has_value() const noexcept { return v_ != nullptr; }

  constexpr T &value() const
  {
    if (v_ == nullptr)
      throw ::std::bad_optional_access();
    return *v_;
  }

  template <class U = ::std::remove_cv_t<T>> constexpr ::std::remove_cv_t<T> value_or(U &&u) const
  {
    static_assert(::std::is_constructible_v<::std::remove_cv_t<T>, T &> //
                  && ::std::is_convertible_v<U, ::std::remove_cv_t<T>>);
    return v_ != nullptr ? *v_ : static_cast<::std::remove_cv_t<T>>(::std::forward<U>(u));
  }

  // [optional.ref.monadic] bodies; unlike optional<T>'s, the callable always receives plain
  // T& (const on the optional does not propagate to the referent), so Self only says which
  // public overload delegated here.
  template <typename Self, typename Fn>
  static constexpr auto _and_then(Self &&self, Fn &&fn) noexcept(::std::is_nothrow_invocable_v<Fn, T &>)
    requires ::std::is_invocable_v<Fn, T &>
  {
    using result_t = ::std::remove_cvref_t<::std::invoke_result_t<Fn, T &>>;
    static_assert(Policy::template is_specialization<result_t>); // [optional.ref.monadic] Mandates
    if (self.has_value()) {
      return ::std::invoke(FWD(fn), *self);
    }
    return result_t();
  }

  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn, T &>
               && ::std::is_nothrow_constructible_v<::std::remove_cv_t<::std::invoke_result_t<Fn, T &>>,
                                                    ::std::invoke_result_t<Fn, T &>>)
    requires ::std::is_invocable_v<Fn, T &>
  {
    using value_t = ::std::remove_cv_t<::std::invoke_result_t<Fn, T &>>;
    static_assert(_is_valid_optional<value_t>); // [optional.ref.monadic] Mandates
    using result_t = typename Policy::template type<value_t>;
    if (self.has_value()) {
      return result_t(_optional_from_invoke, FWD(fn), *self);
    }
    return result_t();
  }

  template <typename Self, typename Fn>
  static constexpr auto _or_else(Self &&self, Fn &&fn) noexcept(::std::is_nothrow_invocable_v<Fn>)
    requires ::std::is_invocable_v<Fn>
  {
    using result_t = typename Policy::template type<T &>;
    static_assert(::std::is_same_v<::std::remove_cvref_t<::std::invoke_result_t<Fn>>, result_t>); // Mandates
    if (self.has_value()) {
      return result_t(*self); // the draft's `return *val;` -- rebinds the same referent
    }
    return ::std::invoke(FWD(fn));
  }
};

struct optional_policy {
  template <class T> using type = ::pfn::optional<T>;
  template <class X> static constexpr bool is_specialization = _is_some_optional<X>;
};

// [optional.hash]: hash<optional<T>> is enabled iff hash<remove_const_t<T>> is enabled --
// detected through the disabled-specialization criteria of [unord.hash] (an unspecialized
// std::hash is a complete but disabled type on all mainstream implementations). Never true
// for T = U& (no std::hash specialization for reference types is ever enabled), so
// hash<optional<U&>> is disabled.
template <class X>
concept _hash_enabled_for = ::std::is_default_constructible_v<::std::hash<X>> //
                            && requires(::std::hash<X> const &h, X const &x) {
                                 { h(x) } -> ::std::same_as<::std::size_t>;
                               };

// Opt is the optional type whose std::hash specialization derives this base; a parameter so
// that a derived library's optional (e.g. fn::optional) can reuse the same machinery.
template <class Opt, class T, bool = _hash_enabled_for<::std::remove_const_t<T>>> struct _optional_hash_base {
  ::std::size_t operator()(Opt const &o) const                        //
      noexcept(noexcept(::std::hash<::std::remove_const_t<T>>{}(*o))) // extension
  {
    if (o.has_value())
      return ::std::hash<::std::remove_const_t<T>>{}(*o);
    return static_cast<::std::size_t>(-7919); // [optional.hash] disengaged: an unspecified value
  }
};
// Disabled case: per [unord.hash] a disabled hash specialization is not a function object
// (no call operator) and is neither constructible nor assignable.
template <class Opt, class T> struct _optional_hash_base<Opt, T, false> {
  _optional_hash_base() = delete;
  _optional_hash_base(_optional_hash_base const &) = delete;
  _optional_hash_base(_optional_hash_base &&) = delete;
  _optional_hash_base &operator=(_optional_hash_base const &) = delete;
  _optional_hash_base &operator=(_optional_hash_base &&) = delete;
};

} // namespace detail

template <class T> class optional : private detail::_optional_base<T, detail::optional_policy> {
  static_assert(detail::_is_valid_optional<T>);
  using _base = detail::_optional_base<T, detail::optional_policy>;

  // Allow sibling `_optional_base` instantiations to call the private from-invoke ctor
  // below (its caller is another optional's base).
  template <class, class> friend struct detail::_optional_base;

public:
  using value_type = T;

  // [optional.ctor], constructors
  constexpr optional() noexcept : _base(::std::nullopt) {}
  constexpr optional(::std::nullopt_t) noexcept : _base(::std::nullopt) {}

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

  template <class U = ::std::remove_cv_t<T>>
  constexpr explicit(not ::std::is_convertible_v<U, T>) optional(U &&v) // NOSONAR cpp:S6458 _can_convert excludes self
      noexcept(::std::is_nothrow_constructible_v<T, U>)                 // extension
    requires(_base::template _can_convert<U>::value)
      : _base(::std::in_place, FWD(v))
  {
  }
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

  // [optional.dtor], destructor
  constexpr ~optional() = default;

  // [optional.assign], assignment
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
  constexpr optional &operator=(optional &&s) // NOSONAR cpp:S5018 standard mandated `noexcept` spec.
      noexcept(::std::is_nothrow_move_assignable_v<T> && ::std::is_nothrow_move_constructible_v<T>) // required
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

  using _base::emplace;

  // [optional.swap], swap; body delegates to _optional_base helper
  constexpr void swap(optional &rhs) // NOSONAR cpp:S5018 standard mandated `noexcept` spec.
      noexcept(::std::is_nothrow_move_constructible_v<T> && ::std::is_nothrow_swappable_v<T>) // required
  {
    static_assert(::std::is_move_constructible_v<T>);
    this->_swap_with(rhs);
  }

  // [optional.observe], observers
  using _base::has_value;
  using _base::operator bool;
  using _base::operator*;
  using _base::operator->;
  using _base::value; // freestanding-deleted
  using _base::value_or;

  // [optional.monadic], monadic operations; bodies delegate to _optional_base helpers
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
  constexpr auto or_else(F &&f) &&                                    //
      noexcept(noexcept(_base::_or_else(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_or_else(::std::move(*this), FWD(f)))
    requires(::std::invocable<F> && ::std::move_constructible<T>)
  {
    return _base::_or_else(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto or_else(F &&f) const &                  //
      noexcept(noexcept(_base::_or_else(*this, FWD(f)))) // extension
      -> decltype(_base::_or_else(*this, FWD(f)))
    requires(::std::invocable<F> && ::std::copy_constructible<T>)
  {
    return _base::_or_else(*this, FWD(f));
  }

  // [optional.mod], modifiers
  using _base::reset;

private:
  // Direct-non-list-initializes the contained value from the result of std::invoke; used by
  // transform implemented in _optional_base.
  template <class Fn, class... Args>
  constexpr explicit optional(detail::_optional_from_invoke_t tag, Fn &&fn, Args &&...args) //
      noexcept(::std::is_nothrow_constructible_v<_base, detail::_optional_from_invoke_t, Fn, Args...>)
      : _base(tag, FWD(fn), FWD(args)...)
  {
  }
};

template <class T> optional(T) -> optional<T>;

template <class T> class optional<T &> : private detail::_optional_base<T &, detail::optional_policy> {
  static_assert(detail::_is_valid_optional<T>);
  using _base = detail::_optional_base<T &, detail::optional_policy>;

  // Allow sibling `_optional_base` instantiations to call the private from-invoke ctor
  // below (its caller is another optional's base).
  template <class, class> friend struct detail::_optional_base;

public:
  using value_type = T;

  // [optional.ref.ctor], constructors
  constexpr optional() noexcept = default;
  constexpr optional(::std::nullopt_t) noexcept : optional() {}
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

  // [optional.ref.assign], assignment
  constexpr optional &operator=(::std::nullopt_t) noexcept
  {
    this->reset();
    return *this;
  }
  constexpr optional &operator=(optional const &rhs) noexcept = default;

  using _base::emplace;

  // [optional.ref.swap], swap
  constexpr void swap(optional &rhs) noexcept { this->_swap_with(rhs); }

  // [optional.ref.observe], observers
  using _base::has_value;
  using _base::operator bool;
  using _base::operator*;
  using _base::operator->;
  using _base::value; // freestanding-deleted
  using _base::value_or;

  // [optional.ref.monadic], monadic operations; bodies delegate to _optional_base helpers
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
  constexpr auto or_else(F &&f) const                    //
      noexcept(noexcept(_base::_or_else(*this, FWD(f)))) // extension
      -> decltype(_base::_or_else(*this, FWD(f)))
    requires ::std::invocable<F>
  {
    return _base::_or_else(*this, FWD(f));
  }

  // [optional.ref.mod], modifiers
  using _base::reset;

private:
  // Direct-non-list-initializes the bound reference from the result of std::invoke; used by
  // transform implemented in _optional_base.
  template <class Fn, class... Args>
  constexpr explicit optional(detail::_optional_from_invoke_t tag, Fn &&fn, Args &&...args) //
      noexcept(::std::is_nothrow_constructible_v<_base, detail::_optional_from_invoke_t, Fn, Args...>)
      : _base(tag, FWD(fn), FWD(args)...)
  {
  }
};

// [optional.relops], relational operators
template <class T, class U>
constexpr bool operator==(optional<T> const &x, optional<U> const &y) //
    noexcept(detail::_eq_bool_noexcept<T, U>)                         // extension
  requires detail::_eq_bool<T, U>
{
  if (x.has_value() != y.has_value())
    return false;
  if (not x.has_value())
    return true;
  return *x == *y;
}
template <class T, class U>
constexpr bool operator!=(optional<T> const &x, optional<U> const &y) //
    noexcept(detail::_ne_bool_noexcept<T, U>)                         // extension
  requires detail::_ne_bool<T, U>
{
  if (x.has_value() != y.has_value())
    return true;
  if (not x.has_value())
    return false;
  return *x != *y;
}
template <class T, class U>
constexpr bool operator<(optional<T> const &x, optional<U> const &y) //
    noexcept(detail::_lt_bool_noexcept<T, U>)                        // extension
  requires detail::_lt_bool<T, U>
{
  if (not y.has_value())
    return false;
  if (not x.has_value())
    return true;
  return *x < *y;
}
template <class T, class U>
constexpr bool operator>(optional<T> const &x, optional<U> const &y) //
    noexcept(detail::_gt_bool_noexcept<T, U>)                        // extension
  requires detail::_gt_bool<T, U>
{
  if (not x.has_value())
    return false;
  if (not y.has_value())
    return true;
  return *x > *y;
}
template <class T, class U>
constexpr bool operator<=(optional<T> const &x, optional<U> const &y) //
    noexcept(detail::_le_bool_noexcept<T, U>)                         // extension
  requires detail::_le_bool<T, U>
{
  if (not x.has_value())
    return true;
  if (not y.has_value())
    return false;
  return *x <= *y;
}
template <class T, class U>
constexpr bool operator>=(optional<T> const &x, optional<U> const &y) //
    noexcept(detail::_ge_bool_noexcept<T, U>)                         // extension
  requires detail::_ge_bool<T, U>
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

// [optional.nullops], comparison with nullopt
template <class T> constexpr bool operator==(optional<T> const &x, ::std::nullopt_t) noexcept
{
  return not x.has_value();
}
template <class T> constexpr ::std::strong_ordering operator<=>(optional<T> const &x, ::std::nullopt_t) noexcept
{
  return x.has_value() <=> false;
}

// [optional.comp.with.t], comparison with T
template <class T, class U>
constexpr bool operator==(optional<T> const &x, U const &v) //
    noexcept(detail::_eq_bool_noexcept<T, U>)               // extension
  requires(not detail::_is_some_optional<U>) && detail::_eq_bool<T, U>
{
  return x.has_value() ? *x == v : false;
}
template <class T, class U>
constexpr bool operator==(T const &v, optional<U> const &x) //
    noexcept(detail::_eq_bool_noexcept<T, U>)               // extension
  requires(not detail::_is_some_optional<T>) && detail::_eq_bool<T, U>
{
  return x.has_value() ? v == *x : false;
}
template <class T, class U>
constexpr bool operator!=(optional<T> const &x, U const &v) //
    noexcept(detail::_ne_bool_noexcept<T, U>)               // extension
  requires(not detail::_is_some_optional<U>) && detail::_ne_bool<T, U>
{
  return x.has_value() ? *x != v : true;
}
template <class T, class U>
constexpr bool operator!=(T const &v, optional<U> const &x) //
    noexcept(detail::_ne_bool_noexcept<T, U>)               // extension
  requires(not detail::_is_some_optional<T>) && detail::_ne_bool<T, U>
{
  return x.has_value() ? v != *x : true;
}
template <class T, class U>
constexpr bool operator<(optional<T> const &x, U const &v) //
    noexcept(detail::_lt_bool_noexcept<T, U>)              // extension
  requires(not detail::_is_some_optional<U>) && detail::_lt_bool<T, U>
{
  return x.has_value() ? *x < v : true;
}
template <class T, class U>
constexpr bool operator<(T const &v, optional<U> const &x) //
    noexcept(detail::_lt_bool_noexcept<T, U>)              // extension
  requires(not detail::_is_some_optional<T>) && detail::_lt_bool<T, U>
{
  return x.has_value() ? v < *x : false;
}
template <class T, class U>
constexpr bool operator>(optional<T> const &x, U const &v) //
    noexcept(detail::_gt_bool_noexcept<T, U>)              // extension
  requires(not detail::_is_some_optional<U>) && detail::_gt_bool<T, U>
{
  return x.has_value() ? *x > v : false;
}
template <class T, class U>
constexpr bool operator>(T const &v, optional<U> const &x) //
    noexcept(detail::_gt_bool_noexcept<T, U>)              // extension
  requires(not detail::_is_some_optional<T>) && detail::_gt_bool<T, U>
{
  return x.has_value() ? v > *x : true;
}
template <class T, class U>
constexpr bool operator<=(optional<T> const &x, U const &v) //
    noexcept(detail::_le_bool_noexcept<T, U>)               // extension
  requires(not detail::_is_some_optional<U>) && detail::_le_bool<T, U>
{
  return x.has_value() ? *x <= v : true;
}
template <class T, class U>
constexpr bool operator<=(T const &v, optional<U> const &x) //
    noexcept(detail::_le_bool_noexcept<T, U>)               // extension
  requires(not detail::_is_some_optional<T>) && detail::_le_bool<T, U>
{
  return x.has_value() ? v <= *x : false;
}
template <class T, class U>
constexpr bool operator>=(optional<T> const &x, U const &v) //
    noexcept(detail::_ge_bool_noexcept<T, U>)               // extension
  requires(not detail::_is_some_optional<U>) && detail::_ge_bool<T, U>
{
  return x.has_value() ? *x >= v : false;
}
template <class T, class U>
constexpr bool operator>=(T const &v, optional<U> const &x) //
    noexcept(detail::_ge_bool_noexcept<T, U>)               // extension
  requires(not detail::_is_some_optional<T>) && detail::_ge_bool<T, U>
{
  return x.has_value() ? v >= *x : true;
}
template <class T, class U>
  requires(not detail::_is_derived_from_optional<U>) && ::std::three_way_comparable_with<T, U>
constexpr ::std::compare_three_way_result_t<T, U> operator<=>(optional<T> const &x, U const &v)
{
  return x.has_value() ? *x <=> v : ::std::strong_ordering::less;
}

// [optional.specalg], specialized algorithms
template <class T>
constexpr void swap(optional<T> &x, optional<T> &y) noexcept(noexcept(x.swap(y)))
  requires(::std::is_reference_v<T> || (::std::is_move_constructible_v<T> && ::std::is_swappable_v<T>))
{
  x.swap(y);
}

// The int parameter's default lives on the declaration above, along with the explanation of
// the [optional.specalg] constraint it implements.
template <int, class T>
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

} // namespace pfn

namespace std {
// [optional.hash], hash support
template <class T> struct hash<::pfn::optional<T>> : ::pfn::detail::_optional_hash_base<::pfn::optional<T>, T> {};
} // namespace std

#undef ASSERT

#ifdef INCLUDE_PFN_OPTIONAL__POP_ASSERT
#pragma pop_macro("ASSERT")
#endif

#undef FWD

#ifdef INCLUDE_PFN_OPTIONAL__POP_FWD
#pragma pop_macro("FWD")
#endif

#endif // INCLUDE_PFN_OPTIONAL
