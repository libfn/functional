// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_PACK
#define INCLUDE_FN_PACK

#include <fn/detail/meta.hpp>
#include <fn/detail/pack_impl.hpp>
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

  /**
   * @brief The pack type that appending a `T` yields
   */
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

  /**
   * @brief Orders two packs lexicographically, element by element
   */
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
      noexcept(noexcept(_impl::template _append<T, append_type<T>>(::std::declval<pack &>(), FWD(args)...)))
          -> append_type<T>
    requires requires { _impl::template _append<T, append_type<T>>(*this, FWD(args)...); }
  {
    return _impl::template _append<T, append_type<T>>(*this, FWD(args)...);
  }

  template <typename T>
  [[nodiscard]] constexpr auto append(::std::in_place_type_t<T>, auto &&...args) const & //
      noexcept(noexcept(_impl::template _append<T, append_type<T>>(::std::declval<pack const &>(), FWD(args)...)))
          -> append_type<T>
    requires requires { _impl::template _append<T, append_type<T>>(*this, FWD(args)...); }
  {
    return _impl::template _append<T, append_type<T>>(*this, FWD(args)...);
  }

  template <typename T>
  [[nodiscard]] constexpr auto append(::std::in_place_type_t<T>, auto &&...args) && //
      noexcept(noexcept(_impl::template _append<T, append_type<T>>(::std::declval<pack &&>(), FWD(args)...)))
          -> append_type<T>
    requires requires { _impl::template _append<T, append_type<T>>(::std::move(*this), FWD(args)...); }
  {
    return _impl::template _append<T, append_type<T>>(::std::move(*this), FWD(args)...);
  }

  template <typename T>
  [[nodiscard]] constexpr auto append(::std::in_place_type_t<T>, auto &&...args) const && //
      noexcept(noexcept(_impl::template _append<T, append_type<T>>(::std::declval<pack const &&>(), FWD(args)...)))
          -> append_type<T>
    requires requires { _impl::template _append<T, append_type<T>>(::std::move(*this), FWD(args)...); }
  {
    return _impl::template _append<T, append_type<T>>(::std::move(*this), FWD(args)...);
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
      noexcept(noexcept(_impl::template _append<Arg, append_type<Arg>>(::std::declval<pack &>(), FWD(arg))))
          -> append_type<Arg>
    requires(not detail::_some_in_place_type<Arg>)
            && requires { _impl::template _append<Arg, append_type<Arg>>(*this, FWD(arg)); }
  {
    return _impl::template _append<Arg, append_type<Arg>>(*this, FWD(arg));
  }

  template <typename Arg>
  [[nodiscard]] constexpr auto append(Arg &&arg) const & //
      noexcept(noexcept(_impl::template _append<Arg, append_type<Arg>>(::std::declval<pack const &>(), FWD(arg))))
          -> append_type<Arg>
    requires(not detail::_some_in_place_type<Arg>)
            && requires { _impl::template _append<Arg, append_type<Arg>>(*this, FWD(arg)); }
  {
    return _impl::template _append<Arg, append_type<Arg>>(*this, FWD(arg));
  }

  template <typename Arg>
  [[nodiscard]] constexpr auto append(Arg &&arg) && //
      noexcept(noexcept(_impl::template _append<Arg, append_type<Arg>>(::std::declval<pack &&>(), FWD(arg))))
          -> append_type<Arg>
    requires(not detail::_some_in_place_type<Arg>)
            && requires { _impl::template _append<Arg, append_type<Arg>>(::std::move(*this), FWD(arg)); }
  {
    return _impl::template _append<Arg, append_type<Arg>>(::std::move(*this), FWD(arg));
  }

  template <typename Arg>
  [[nodiscard]] constexpr auto append(Arg &&arg) const && //
      noexcept(noexcept(_impl::template _append<Arg, append_type<Arg>>(::std::declval<pack const &&>(), FWD(arg))))
          -> append_type<Arg>
    requires(not detail::_some_in_place_type<Arg>)
            && requires { _impl::template _append<Arg, append_type<Arg>>(::std::move(*this), FWD(arg)); }
  {
    return _impl::template _append<Arg, append_type<Arg>>(::std::move(*this), FWD(arg));
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
  requires(sizeof...(Explicit) == 0) && (not detail::_some_in_place_type<T>)
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
  requires(not detail::_some_in_place_type<T>) && detail::_initializable<pack<T, Args...>, T, Args...>
[[nodiscard]] constexpr auto as_pack(::std::type_identity_t<T> src, ::std::type_identity_t<Args>... args) //
    noexcept(detail::_nothrow_initializable<pack<T, Args...>, T, Args...>) -> pack<T, Args...>
{
  return pack<T, Args...>{FWD(src), FWD(args)...};
}

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
