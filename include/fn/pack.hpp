// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_PACK
#define INCLUDE_FN_PACK

#include <fn/copack.hpp>
#include <fn/detail/meta.hpp>
#include <fn/detail/pack_impl.hpp>
#include <fn/monadic.hpp>
#include <libfn_version.hpp>

#include <type_traits>

#include <fn/detail/macro_begin.hpp>

namespace fn {
inline namespace LIBFN_VERSION {

/**
 * @brief Checks if a type is a `pack` (with any elements)
 *
 * @tparam T Type to check, possibly cv-ref qualified
 */
template <typename T>
concept some_pack = detail::_some_pack<T>;

/**
 * @brief The product payload: all fields present, strictly flat
 *
 * A tuple-like structure supporting the tuple protocol - `get`, `tuple_size`, `tuple_element`,
 * structured bindings - and an `append` mechanism. Unlike `std::tuple`, a `pack` is not a valid
 * element of a `pack`: appending one splices its fields in rather than nesting it. Elements may
 * be lvalue references - `pack<T&>` is how the other carriers propagate references - and a const
 * pack propagates its const onto reference elements, handing out read-only views of the
 * referenced data. `pack<>` is the nullary product, the algebra's unit: a value that exists and
 * holds nothing, where the uninhabited `copack<>` is the zero. A structural type when its
 * elements are, so a constexpr pack can be used as a template parameter.
 *
 * @tparam Ts Element types; values or lvalue references, never rvalue references
 */
template <typename... Ts> struct pack : detail::pack_impl<::std::index_sequence_for<Ts...>, Ts...> {
  using _impl = detail::pack_impl<::std::index_sequence_for<Ts...>, Ts...>;
  static_assert((... && detail::_is_valid_pack_element<Ts>));

  template <typename T> using append_type = _impl::template append_type<T>;

  /**
   * @brief Compares two packs of the same type element by element, `<=>` lexicographically
   *
   * Every element decides: one that cannot be compared leaves the operator non-viable, which asking
   * answers rather than erroring on, and a reference element compares its referent, as
   * `fn::optional<T&>` and `std::tuple` do. `<=>` asks each element for its own, synthesizing no
   * ordering from `<`. `!=`, `<`, `>`, `<=` and `>=` follow from these two by rewriting.
   */
  [[nodiscard]] constexpr auto operator==(pack const &other) const //
      noexcept(noexcept(_impl::_equal(*this, other))) -> bool
    requires requires(pack const &a, pack const &b) { _impl::_equal(a, b); }
  {
    return _impl::_equal(*this, other);
  }

  [[nodiscard]] constexpr auto operator<=>(pack const &other) const //
      noexcept(noexcept(_impl::_compare(*this, other)))
    requires requires(pack const &a, pack const &b) { _impl::_compare(a, b); }
  {
    return _impl::_compare(*this, other);
  }

  /**
   * @brief Appends an element of type `T`, constructed in place from the arguments
   *
   * A `pack` for `T` splices its fields in - packs stay strictly flat - and a `copack` is not a
   * valid element at all. The result is a new pack; the existing elements are copied or moved in
   * `*this`'s value category, so a const pack with a reference element is not appendable: const
   * propagates through the reference, and the new pack's element cannot bind it.
   *
   * @tparam T Type of the new element
   * @param args Arguments to construct the new element from
   * @return A new pack, with the new element at the end
   */
  template <typename T>
  [[nodiscard]] constexpr auto append(::std::in_place_type_t<T>, auto &&...args) & //
      noexcept(noexcept(_impl::template _append<T>(::std::declval<pack &>(), FWD(args)...))) -> append_type<T>
    requires requires { append_type<T>{_impl::template _append<T>(*this, FWD(args)...)}; }
  {
    return {_impl::template _append<T>(*this, FWD(args)...)};
  }

  template <typename T>
  [[nodiscard]] constexpr auto append(::std::in_place_type_t<T>, auto &&...args) const & //
      noexcept(noexcept(_impl::template _append<T>(::std::declval<pack const &>(), FWD(args)...))) -> append_type<T>
    requires requires { append_type<T>{_impl::template _append<T>(*this, FWD(args)...)}; }
  {
    return {_impl::template _append<T>(*this, FWD(args)...)};
  }

  template <typename T>
  [[nodiscard]] constexpr auto append(::std::in_place_type_t<T>, auto &&...args) && //
      noexcept(noexcept(_impl::template _append<T>(::std::declval<pack &&>(), FWD(args)...))) -> append_type<T>
    requires requires { append_type<T>{_impl::template _append<T>(::std::move(*this), FWD(args)...)}; }
  {
    return {_impl::template _append<T>(::std::move(*this), FWD(args)...)};
  }

  template <typename T>
  [[nodiscard]] constexpr auto append(::std::in_place_type_t<T>, auto &&...args) const && //
      noexcept(noexcept(_impl::template _append<T>(::std::declval<pack const &&>(), FWD(args)...))) -> append_type<T>
    requires requires { append_type<T>{_impl::template _append<T>(::std::move(*this), FWD(args)...)}; }
  {
    return {_impl::template _append<T>(::std::move(*this), FWD(args)...)};
  }

  /**
   * @brief Appends a value; a `pack` argument splices its fields in
   *
   * Packs stay strictly flat: appending a pack appends its fields, never a nested pack. The
   * result is a new pack; the existing elements are copied or moved in `*this`'s value category.
   *
   * @param arg Value to append, or a pack whose fields to append
   * @return A new pack, with the addition at the end
   */
  template <typename Arg>
  [[nodiscard]] constexpr auto append(Arg &&arg) & //
      noexcept(noexcept(_impl::template _append<Arg>(::std::declval<pack &>(), FWD(arg)))) -> append_type<Arg>
    requires(not some_in_place_type<Arg>)
            && requires { append_type<Arg>{_impl::template _append<Arg>(*this, FWD(arg))}; }
  {
    return {_impl::template _append<Arg>(*this, FWD(arg))};
  }

  template <typename Arg>
  [[nodiscard]] constexpr auto append(Arg &&arg) const & //
      noexcept(noexcept(_impl::template _append<Arg>(::std::declval<pack const &>(), FWD(arg)))) -> append_type<Arg>
    requires(not some_in_place_type<Arg>)
            && requires { append_type<Arg>{_impl::template _append<Arg>(*this, FWD(arg))}; }
  {
    return {_impl::template _append<Arg>(*this, FWD(arg))};
  }

  template <typename Arg>
  [[nodiscard]] constexpr auto append(Arg &&arg) && //
      noexcept(noexcept(_impl::template _append<Arg>(::std::declval<pack &&>(), FWD(arg)))) -> append_type<Arg>
    requires(not some_in_place_type<Arg>)
            && requires { append_type<Arg>{_impl::template _append<Arg>(::std::move(*this), FWD(arg))}; }
  {
    return {_impl::template _append<Arg>(::std::move(*this), FWD(arg))};
  }

  template <typename Arg>
  [[nodiscard]] constexpr auto append(Arg &&arg) const && //
      noexcept(noexcept(_impl::template _append<Arg>(::std::declval<pack const &&>(), FWD(arg)))) -> append_type<Arg>
    requires(not some_in_place_type<Arg>)
            && requires { append_type<Arg>{_impl::template _append<Arg>(::std::move(*this), FWD(arg))}; }
  {
    return {_impl::template _append<Arg>(::std::move(*this), FWD(arg))};
  }

  /**
   * @brief Eliminates the pack: the elements spread into the callable as separate arguments
   *
   * The bridge between the product as stored and the product as an argument list: `pack<A, B>`
   * invokes `f(a, b)`, and `pack<>` invokes `f()`. The elements are handed over in `*this`'s
   * cv-qualification and value category, and trailing arguments follow them.
   *
   * @param fn Callable applied on the elements
   * @param args Additional arguments, appended after the elements
   * @return The callable's result
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto apply(Fn &&fn, auto &&...args) & //
      noexcept(noexcept(_impl::_apply(::std::declval<pack &>(), FWD(fn), FWD(args)...)))
          -> DEDUCED_RETURN(_impl::_apply(*this, FWD(fn), FWD(args)...))
    requires requires { _impl::_apply(*this, FWD(fn), FWD(args)...); }
  {
    return _impl::_apply(*this, FWD(fn), FWD(args)...);
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto apply(Fn &&fn, auto &&...args) const & //
      noexcept(noexcept(_impl::_apply(::std::declval<pack const &>(), FWD(fn), FWD(args)...)))
          -> DEDUCED_RETURN(_impl::_apply(*this, FWD(fn), FWD(args)...))
    requires requires { _impl::_apply(*this, FWD(fn), FWD(args)...); }
  {
    return _impl::_apply(*this, FWD(fn), FWD(args)...);
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto apply(Fn &&fn, auto &&...args) && //
      noexcept(noexcept(_impl::_apply(::std::declval<pack &&>(), FWD(fn), FWD(args)...)))
          -> DEDUCED_RETURN(_impl::_apply(::std::move(*this), FWD(fn), FWD(args)...))
    requires requires { _impl::_apply(::std::move(*this), FWD(fn), FWD(args)...); }
  {
    return _impl::_apply(::std::move(*this), FWD(fn), FWD(args)...);
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto apply(Fn &&fn, auto &&...args) const && //
      noexcept(noexcept(_impl::_apply(::std::declval<pack const &&>(), FWD(fn), FWD(args)...)))
          -> DEDUCED_RETURN(_impl::_apply(::std::move(*this), FWD(fn), FWD(args)...))
    requires requires { _impl::_apply(::std::move(*this), FWD(fn), FWD(args)...); }
  {
    return _impl::_apply(::std::move(*this), FWD(fn), FWD(args)...);
  }

  /**
   * @brief Eliminates the pack, converting the result to `Ret`
   *
   * @tparam Ret Type the result converts to
   * @param fn Callable applied on the elements
   * @param args Additional arguments, appended after the elements
   * @return The callable's result, converted to `Ret`
   */
  template <typename Ret, typename Fn>
  [[nodiscard]] constexpr auto apply_r(Fn &&fn, auto &&...args) & //
      noexcept(noexcept(_impl::template _apply_r<Ret>(::std::declval<pack &>(), FWD(fn), FWD(args)...))) -> Ret
    requires requires { _impl::template _apply_r<Ret>(*this, FWD(fn), FWD(args)...); }
  {
    return _impl::template _apply_r<Ret>(*this, FWD(fn), FWD(args)...);
  }

  template <typename Ret, typename Fn>
  [[nodiscard]] constexpr auto apply_r(Fn &&fn, auto &&...args) const & //
      noexcept(noexcept(_impl::template _apply_r<Ret>(::std::declval<pack const &>(), FWD(fn), FWD(args)...))) -> Ret
    requires requires { _impl::template _apply_r<Ret>(*this, FWD(fn), FWD(args)...); }
  {
    return _impl::template _apply_r<Ret>(*this, FWD(fn), FWD(args)...);
  }

  template <typename Ret, typename Fn>
  [[nodiscard]] constexpr auto apply_r(Fn &&fn, auto &&...args) && //
      noexcept(noexcept(_impl::template _apply_r<Ret>(::std::declval<pack &&>(), FWD(fn), FWD(args)...))) -> Ret
    requires requires { _impl::template _apply_r<Ret>(::std::move(*this), FWD(fn), FWD(args)...); }
  {
    return _impl::template _apply_r<Ret>(::std::move(*this), FWD(fn), FWD(args)...);
  }

  template <typename Ret, typename Fn>
  [[nodiscard]] constexpr auto apply_r(Fn &&fn, auto &&...args) const && //
      noexcept(noexcept(_impl::template _apply_r<Ret>(::std::declval<pack const &&>(), FWD(fn), FWD(args)...))) -> Ret
    requires requires { _impl::template _apply_r<Ret>(::std::move(*this), FWD(fn), FWD(args)...); }
  {
    return _impl::template _apply_r<Ret>(::std::move(*this), FWD(fn), FWD(args)...);
  }
};

template <typename... Args> pack(Args &&...args) -> pack<Args...>;

/**
 * @brief Tuple-protocol element access
 *
 * Returns the `I`-th element carrying the pack's cv-qualification and value
 * category, exactly as `apply` would pass it. Found by ADL, so it also serves
 * structured bindings and the generic `using ::std::get; get<I>(p)` idiom.
 *
 * @tparam I element index
 * @param p the pack
 * @return reference to the `I`-th element
 */
template <::std::size_t I, some_pack P>
[[nodiscard]] constexpr decltype(auto) get(P &&p) noexcept
  requires(I < ::std::remove_cvref_t<P>::size)
{
  return ::std::remove_cvref_t<P>::template _get<I>(FWD(p));
}

// Lifts
/**
 * @brief Lifts no values into the empty `pack` - the unit
 * @return `pack<>`
 */
[[nodiscard]] constexpr auto as_pack() noexcept -> pack<> { return {}; }

/**
 * @brief Lifts values into a `pack`, deduction preserving each argument's value category
 *
 * `as_pack(42)` yields `pack<int>`; an lvalue `x` yields `pack<int &>` - a reference rather than
 * a copy.
 *
 * @param src First value to lift
 * @param args Further values to lift
 * @return The new pack
 */
// The unused leading pack absorbs explicit template arguments and the constraint rejects them:
// this overload is deduction-only (value-category preserving), the overload below serves spelled types
template <typename... Explicit, typename T, typename... Args>
  requires(sizeof...(Explicit) == 0) && (not some_in_place_type<T>)
          && detail::_initializable<pack<T, Args...>, T, Args...>
[[nodiscard]] constexpr auto as_pack(T &&src, Args &&...args) //
    noexcept(detail::_nothrow_initializable<pack<T, Args...>, T, Args...>) -> pack<T, Args...>
{
  return pack<T, Args...>{FWD(src), FWD(args)...};
}

/**
 * @brief Lifts values into a `pack` of exactly the spelled element types
 *
 * `as_pack<bool, int>(x, d)` converts each argument at the call boundary; every element type must
 * be spelled - a partial spelling is not viable - and a reference element is what you ask for, as
 * in `as_pack<int const &>(x)`.
 *
 * @tparam T First element type, as spelled
 * @tparam Args Further element types, as spelled
 * @param src First value, converted to `T`
 * @param args Further values, converted to `Args...`
 * @return The new pack
 */
// No element type is deduced: the explicit form names ALL of pack<T, Args...> or is not viable
// (a partial spelling fails on arity); by-value parameters admit conversion at the call boundary
// (narrowing included) while still relocating rvalue arguments
template <typename T, typename... Args>
  requires(not some_in_place_type<T>) && detail::_initializable<pack<T, Args...>, T, Args...>
[[nodiscard]] constexpr auto as_pack(::std::type_identity_t<T> src, ::std::type_identity_t<Args>... args) //
    noexcept(detail::_nothrow_initializable<pack<T, Args...>, T, Args...>) -> pack<T, Args...>
{
  return pack<T, Args...>{FWD(src), FWD(args)...};
}

namespace detail {

// `value()` throws when the monad holds no value, but the join only reaches it once `has_value()`
// has answered - so the specification below asks what folding and lifting the value promise, with
// the accessor spelled as a type rather than as a call which would drag its own throw in.
template <typename Monad> using _value_of_t = decltype(::std::declval<Monad>().value());

// A product with an uninhabited factor is itself uninhabited, and copack<> has no value fold - the
// join over such a side must not name the fold, in the declared type, the noexcept specification
// or the body, and always resolves through `efn`.
template <typename Lh, typename Rh>
constexpr inline bool _uninhabited_join = empty_copack<typename ::std::remove_cvref_t<Lh>::value_type>
                                          || empty_copack<typename ::std::remove_cvref_t<Rh>::value_type>;

template <bool Uninhabited, typename Lh, typename Rh> struct _joined {
  using type = copack<>;
};
template <typename Lh, typename Rh> struct _joined<false, Lh, Rh> {
  using type = decltype(::fn::detail::_fold_detail::fold<typename ::std::remove_cvref_t<Lh>::value_type,
                                                         typename ::std::remove_cvref_t<Rh>::value_type>(
      ::std::declval<_value_of_t<Lh>>(), ::std::declval<_value_of_t<Rh>>()));
};
template <typename Lh, typename Rh> using _joined_t = typename _joined<_uninhabited_join<Lh, Rh>, Lh, Rh>::type;

// `_join` invokes `efn` as an lvalue - a named parameter - so the `Efn &` questions ask about the
// call the body performs; the reference collapses to it whatever category the callable arrived in.
template <bool Uninhabited, template <typename> typename Tpl, typename Lh, typename Rh, typename Efn>
struct _nothrow_join_arm {
  static constexpr bool value = ::std::is_nothrow_invocable_v<Efn &, Lh> && ::std::is_nothrow_invocable_v<Efn &, Rh>
                                && _nothrow_initializable<Tpl<copack<>>, ::std::invoke_result_t<Efn &, Lh>>
                                && _nothrow_initializable<Tpl<copack<>>, ::std::invoke_result_t<Efn &, Rh>>;
};
template <template <typename> typename Tpl, typename Lh, typename Rh, typename Efn>
struct _nothrow_join_arm<false, Tpl, Lh, Rh, Efn> {
  static constexpr bool value
      = noexcept(::fn::detail::_fold_detail::fold<typename ::std::remove_cvref_t<Lh>::value_type,
                                                  typename ::std::remove_cvref_t<Rh>::value_type>(
            ::std::declval<_value_of_t<Lh>>(), ::std::declval<_value_of_t<Rh>>()))
        && _nothrow_initializable<Tpl<_joined_t<Lh, Rh>>, ::std::in_place_t, _joined_t<Lh, Rh>>
        && ::std::is_nothrow_invocable_v<Efn &, Lh> && ::std::is_nothrow_invocable_v<Efn &, Rh>
        && _nothrow_initializable<Tpl<_joined_t<Lh, Rh>>, ::std::invoke_result_t<Efn &, Lh>>
        && _nothrow_initializable<Tpl<_joined_t<Lh, Rh>>, ::std::invoke_result_t<Efn &, Rh>>;
};
template <template <typename> typename Tpl, typename Lh, typename Rh, typename Efn>
constexpr inline bool _nothrow_join = _nothrow_join_arm<_uninhabited_join<Lh, Rh>, Tpl, Lh, Rh, Efn>::value;

// The disjunction's value channel: the sum of the two value types, with void spelled pack<> -
// both are the unit, and a copack cannot hold void.
template <typename V> using _sum_element_t = ::std::conditional_t<::std::is_void_v<V>, pack<>, V>;
template <typename Lh, typename Rh>
using _disjoined_t = copack_for<_sum_element_t<typename ::std::remove_cvref_t<Lh>::value_type>,
                                _sum_element_t<typename ::std::remove_cvref_t<Rh>::value_type>>;

template <typename T> constexpr inline bool _dead_value = empty_copack<typename ::std::remove_cvref_t<T>::value_type>;

// A dead side (uninhabited value) never relocates into the result - its inject arm is if
// constexpr'd out of the body, and weighs nothing here.
template <bool Dead, typename Type, typename Side> struct _nothrow_disj_inject {
  static constexpr bool value = true;
};
template <typename Type, typename Side> struct _nothrow_disj_inject<false, Type, Side> {
  static constexpr bool value
      = _nothrow_initializable<Type, ::std::in_place_t, decltype(::std::declval<Side>().value())>;
};

template <template <typename> typename Tpl>
[[nodiscard]] constexpr auto _join(auto &&lh, auto &&rh, auto &&efn) //
    noexcept(_nothrow_join<Tpl, decltype(lh), decltype(rh), decltype(efn)>)
        -> Tpl<_joined_t<decltype(lh), decltype(rh)>>
{
  using type = Tpl<_joined_t<decltype(lh), decltype(rh)>>;
  if constexpr (_uninhabited_join<decltype(lh), decltype(rh)>) {
    if (not lh.has_value())
      return type{efn(FWD(lh))};
    else
      return type{efn(FWD(rh))};
  } else {
    using Lh = ::std::remove_cvref_t<decltype(lh)>::value_type;
    using Rh = ::std::remove_cvref_t<decltype(rh)>::value_type;
    if (lh.has_value() && rh.has_value())
      return type{::std::in_place, ::fn::detail::_fold_detail::fold<Lh, Rh>(FWD(lh).value(), FWD(rh).value())};
    else if (not lh.has_value())
      return type{efn(FWD(lh))};
    else
      return type{efn(FWD(rh))};
  }
}

} // namespace detail

/**
 * @brief The data conjunction: concatenates into a `pack`, distributing over `copack` alternatives
 *
 * With plain data on both sides the fields concatenate into one flat `pack`. When either operand
 * is a `copack`, the product distributes over its alternatives - two copacks yield the full
 * cartesian product - producing a normalized `copack` of `pack`s. Dispatches on its left operand:
 * a bare `scalar & scalar` is not part of the algebra, so lift one side first, as in
 * `fn::as_pack(a) & b`.
 *
 * @param lh A `pack` or a `copack`
 * @param rh The data to conjoin: a scalar, a `pack` or a `copack`
 * @return A `pack`, or a `copack` of `pack`s where alternatives distribute
 */
[[nodiscard]] constexpr auto operator&(auto &&lh, auto &&rh) //
    noexcept(noexcept(::fn::detail::_fold_detail::fold<::std::remove_cvref_t<decltype(lh)>,
                                                       ::std::remove_cvref_t<decltype(rh)>>(FWD(lh), FWD(rh))))
  requires(some_copack<decltype(lh)> || some_pack<decltype(lh)>)
{
  using Lh = ::std::remove_cvref_t<decltype(lh)>;
  using Rh = ::std::remove_cvref_t<decltype(rh)>;
  return ::fn::detail::_fold_detail::fold<Lh, Rh>(FWD(lh), FWD(rh));
}

namespace detail {
// The data fold takes data. A monadic carrier among the arguments would become a pack element,
// silently answering a question the caller did not ask: `&` over carriers conjoins the carriers
// themselves, and their values cannot be reached without `value()`, which throws.
template <typename... Ts>
concept _no_carrier = (... && (not some_monadic_type<Ts>));

// ... and the carrier folds take carriers, all of them: a mixed argument list belongs to neither
// world and is refused, rather than resolved by the leading argument.
template <typename... Ts>
concept _all_carriers = (... && some_monadic_type<Ts>);
} // namespace detail

/**
 * @brief The n-ary fold of `operator &` above; a single argument is forwarded unchanged
 *
 * Two modes, never mixed in one call: with every argument a computation carrier the fold is the
 * monadic conjunction, exactly what cascading `operator &` produces, and with none of them a
 * carrier it is the data-level product, a leading scalar lifted into a `pack` first. A mixed
 * argument list is refused rather than resolved by the leading argument.
 */
constexpr inline struct conjoin_t {
  /**
   * @brief Forwards a single argument unchanged
   * @param arg The argument
   * @return The argument, forwarded
   */
  template <typename Arg> [[nodiscard]] constexpr auto operator()(Arg &&arg) const -> decltype(arg) { return FWD(arg); }

  /**
   * @brief Folds data into a product, or carriers into their conjunction
   *
   * With no carrier among the arguments the fold is the data-level product: a leading scalar is
   * lifted into a `pack` first, and a leading `pack` or `copack` dispatches `operator &` itself.
   * With every argument a carrier the same fold is their monadic conjunction.
   *
   * @param arg The leading argument
   * @param args Further arguments - all data, or all carriers, never the two mixed
   * @return The folded product, or the folded conjunction
   */
  template <typename Arg, typename... Args>
    requires(not some_copack<Arg>) && (not some_pack<Arg>) && detail::_no_carrier<Arg, Args...>
  [[nodiscard]] constexpr auto operator()(Arg &&arg, Args &&...args) const
  {
    return (::fn::pack{FWD(arg)} & ... & FWD(args));
  }

  template <typename Arg, typename... Args>
    requires(some_copack<Arg> || some_pack<Arg>) && detail::_no_carrier<Args...>
  [[nodiscard]] constexpr auto operator()(Arg &&arg, Args &&...args) const
  {
    return (FWD(arg) & ... & FWD(args));
  }

  template <typename Arg, typename... Args>
    requires(sizeof...(Args) > 0)
            && detail::_all_carriers<Arg, Args...> && requires(Arg &&a, Args &&...as) { (FWD(a) & ... & FWD(as)); }
  [[nodiscard]] constexpr auto operator()(Arg &&arg, Args &&...args) const //
      noexcept(noexcept((FWD(arg) & ... & FWD(args))))
  {
    return (FWD(arg) & ... & FWD(args));
  }
} conjoin;

/**
 * @brief The n-ary fold of the disjunction `operator |` over the monadic carriers; a single
 *        argument is forwarded unchanged
 *
 * Carriers only, in every arity - disjunction has no data-level form. An identity-cluster operand
 * makes the whole disjunction total, folding the result into `just` or `choice`.
 */
constexpr inline struct disjoin_t {
  // Carriers only, in every arity: `|` over anything else is the built-in operator, and folding
  // integers into 3 is not what this asks for
  template <some_monadic_type Arg> [[nodiscard]] constexpr auto operator()(Arg &&arg) const -> decltype(arg)
  {
    return FWD(arg);
  }

  template <typename Arg, typename... Args>
    requires(sizeof...(Args) > 0)
            && detail::_all_carriers<Arg, Args...> && requires(Arg &&a, Args &&...as) { (FWD(a) | ... | FWD(as)); }
  [[nodiscard]] constexpr auto operator()(Arg &&arg, Args &&...args) const //
      noexcept(noexcept((FWD(arg) | ... | FWD(args))))
  {
    return (FWD(arg) | ... | FWD(args));
  }
} disjoin;

} // namespace LIBFN_VERSION
} // namespace fn

namespace std {
template <typename... Ts>
struct tuple_size<::fn::pack<Ts...>> : ::std::integral_constant<::std::size_t, sizeof...(Ts)> {};

template <::std::size_t I, typename... Ts> struct tuple_element<I, ::fn::pack<Ts...>> {
  using type = ::fn::detail::select_nth_t<I, Ts...>;
};

// A const pack propagates const onto reference elements, so `tuple_element<I, pack const>`
// must match `get` and cannot defer to the generic `tuple_element<I, const T>`.
template <::std::size_t I, typename... Ts> struct tuple_element<I, ::fn::pack<Ts...> const> {
  using type = decltype(::fn::detail::_apply_const<::fn::pack<Ts...> const &, ::fn::detail::select_nth_t<I, Ts...>>);
};
} // namespace std

#include <fn/detail/macro_end.hpp>

#endif // INCLUDE_FN_PACK
