// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_SUM
#define INCLUDE_FN_SUM

#include <fn/detail/functional.hpp>
#include <fn/detail/meta.hpp>
#include <fn/detail/traits.hpp>
#include <fn/detail/variadic_union.hpp>
#include <fn/functional.hpp>

#include <memory>
#include <type_traits>
#include <utility>

namespace fn {

/**
 * @brief TODO
 *
 * @tparam T TODO
 */
template <typename T>
concept some_sum = detail::_some_sum<T>;

/**
 * @brief TODO
 *
 * @tparam T TODO
 */
template <typename T>
concept some_in_place_type = detail::_some_in_place_type<T>;

namespace detail {
template <typename T>
static constexpr bool _is_valid_sum_subtype //
    = (not ::std::is_same_v<void, T>)       //
    &&(not ::std::is_reference_v<T>)        //
    &&(not some_sum<T>)                     //
    &&(not some_in_place_type<T>)           //
    &&::std::is_same_v<T, ::std::remove_cv_t<T>>;

struct _invoke_autodetect_tag final {};

// Whether comparing an alternative can throw. An alternative the other side does not have is never
// compared, so it cannot throw - and need not even be comparable, which is why this is a guarded
// specialization rather than a disjunction: unlike a requires-clause, a noexcept-specifier is an
// ordinary constant expression, and every operand of its `||` has to be well-formed.
template <typename T, typename... Tx> constexpr inline bool _nothrow_eq_with = true;
template <typename T, typename... Tx>
  requires type_one_of<T, Tx...>
constexpr inline bool _nothrow_eq_with<T, Tx...> = noexcept(::std::declval<T const &>() == ::std::declval<T const &>());

template <typename Fn, typename Self, typename T, typename... Args> struct _typelist_select_invoke_result;
template <typename Fn, typename Self, template <typename...> typename Tpl, typename... Ts, typename... Args>
struct _typelist_select_invoke_result<Fn, Self, Tpl<Ts...>, Args...> {
  using T0 = select_nth_t<0, Ts...>;
  using R0 = ::fn::detail::_invoke_result<Fn, apply_const_lvalue_t<Self, T0>, Args...>::type;
  static constexpr bool type_found
      = (...
         && ::std::is_same_v<R0,
                             typename ::fn::detail::_invoke_result<Fn, apply_const_lvalue_t<Self, Ts>, Args...>::type>);
  // If every alternative is invocable here, they must all yield the same result type.
  static_assert(not(... && ::fn::detail::_is_invocable<Fn, apply_const_lvalue_t<Self, Ts>, Args...>::value)
                || type_found);
  using type = ::std::conditional_t<type_found, R0, void>;
};

struct _collapsing_sum_tag final {};

namespace _collapsing_sum {
template <typename... Ts> struct typelist;
template <typename... Ts> extern typelist<Ts...> const &typelist_v;

template <typename... Ts, typename T>
auto operator^(typelist<Ts...> const &, typelist<T> const &) -> typelist<Ts..., T> const &;
template <typename... Ts, typename... Us>
auto operator^(typelist<Ts...> const &, typelist<::fn::sum<Us...>> const &) -> typelist<Ts..., Us...> const &;

template <typename... Ts> using flattened = ::std::remove_cvref_t<decltype((typelist_v<> ^ ... ^ typelist_v<Ts>))>;

template <template <typename...> typename Tpl, typename T> struct normalized;
template <template <typename...> typename Tpl, typename... Ts> struct normalized<Tpl, typelist<Ts...>> {
  using type = typename ::fn::detail::normalized<Ts...>::template apply<Tpl>;
};
} // namespace _collapsing_sum

// A result no sum can hold - void above all - must drop the caller's candidate, not explode:
// everything behind the gate (flattening, normalization, the sum they name) hard-errors OUTSIDE
// any immediate context. The false gate has no `type`, and the collapsing traits pass that absence
// through by inheritance, so it surfaces where a return type names `::type` - in the immediate
// context, as a clean substitution failure.
template <bool, template <typename...> typename Tpl, typename... Rs> struct _collapsing_sum_gate {};
template <template <typename...> typename Tpl, typename... Rs> struct _collapsing_sum_gate<true, Tpl, Rs...> {
  using type = _collapsing_sum::normalized<Tpl, _collapsing_sum::flattened<Rs...>>::type;
};

template <typename R> constexpr inline bool _collapsible_result = some_sum<R> || _is_valid_sum_subtype<R>;

template <typename Fn, typename Self, typename T, typename... Args> struct _typelist_collapsing_sum;
template <typename Fn, typename Self, template <typename...> typename Tpl, typename... Ts, typename... Args>
struct _typelist_collapsing_sum<Fn, Self, Tpl<Ts...>, Args...>
    : _collapsing_sum_gate<
          (...
           && _collapsible_result<::std::remove_cvref_t<
               typename ::fn::detail::_invoke_result<Fn, apply_const_lvalue_t<Self, Ts>, Args...>::type>>),
          Tpl,
          ::std::remove_cvref_t<
              typename ::fn::detail::_invoke_result<Fn, apply_const_lvalue_t<Self, Ts>, Args...>::type>...> {};

template <typename T, typename Fn, typename Self, typename... Args> struct _sum_invoke_result final {
  using type = T;
};
template <typename Fn, typename Self, typename... Args>
struct _sum_invoke_result<_invoke_autodetect_tag, Fn, Self, Args...> final {
  using type = _typelist_select_invoke_result<Fn, Self, ::std::remove_cvref_t<Self>, Args...>::type;
};
template <typename Fn, typename Self, typename... Args>
struct _sum_invoke_result<_collapsing_sum_tag, Fn, Self, Args...> final
    : _typelist_collapsing_sum<Fn, Self, ::std::remove_cvref_t<Self>, Args...> {};

template <typename Fn, typename Self, typename T> struct _typelist_type_select_invoke_result;
template <typename Fn, typename Self, template <typename...> typename Tpl, typename... Ts>
struct _typelist_type_select_invoke_result<Fn, Self, Tpl<Ts...>> {
  using T0 = select_nth_t<0, Ts...>;
  using R0 = ::fn::detail::_invoke_type_result<T0, Fn, apply_const_lvalue_t<Self, T0>>::type;
  static_assert((...
                 && ::std::is_same_v<
                     R0, typename ::fn::detail::_invoke_type_result<Ts, Fn, apply_const_lvalue_t<Self, Ts>>::type>));
  using type = R0;
};

template <typename Fn, typename Self, typename T> struct _typelist_type_collapsing_sum;
template <typename Fn, typename Self, template <typename...> typename Tpl, typename... Ts>
struct _typelist_type_collapsing_sum<Fn, Self, Tpl<Ts...>>
    : _collapsing_sum_gate<
          (...
           && _collapsible_result<::std::remove_cvref_t<
               typename ::fn::detail::_invoke_type_result<Ts, Fn, apply_const_lvalue_t<Self, Ts>>::type>>),
          Tpl,
          ::std::remove_cvref_t<
              typename ::fn::detail::_invoke_type_result<Ts, Fn, apply_const_lvalue_t<Self, Ts>>::type>...> {};

template <typename T, typename Fn, typename Self> struct _sum_invoke_type_result final {
  using type = T;
};
template <typename Fn, typename Self> struct _sum_invoke_type_result<_invoke_autodetect_tag, Fn, Self> final {
  using type = _typelist_type_select_invoke_result<Fn, Self, ::std::remove_cvref_t<Self>>::type;
};
template <typename Fn, typename Self>
struct _sum_invoke_type_result<_collapsing_sum_tag, Fn, Self> final
    : _typelist_type_collapsing_sum<Fn, Self, ::std::remove_cvref_t<Self>> {};

} // namespace detail

/**
 * @brief TODO
 *
 * @tparam Ts TODO
 */
template <typename... Ts> struct sum;

/**
 * @brief A unit of sum<> monoid, cannot be created but offers useful static functions and can be put in an union
 */
template <> struct sum<> final {
  constexpr sum() noexcept = delete; // NOTE, `= delete` here is the whole point
  constexpr ~sum() noexcept = default;
  constexpr sum(sum const &) noexcept = default;
  constexpr sum(sum &&) noexcept = default;

  static constexpr ::std::size_t size = 0;
  template <typename T> static constexpr bool has_type = false;
};

/**
 * @brief TODO
 *
 * @tparam Ts TODO
 */
template <typename... Ts>
  requires(sizeof...(Ts) > 0)
struct sum<Ts...> {
  static_assert((... && detail::_is_valid_sum_subtype<Ts>));
  static_assert(::std::same_as<typename detail::normalized<Ts...>::template apply<::fn::sum>, sum>);

  using data_t = detail::variadic_union<Ts...>;
  data_t data;
  ::std::size_t index;

  static constexpr ::std::size_t size = sizeof...(Ts);

  // What copying and moving a sum cost, asked of the storage that performs them - see the concepts
  // in detail/variadic_union.hpp. Everything below is specified and constrained in these terms, so
  // that no declaration has to restate what the storage does, and none can drift from it.
  static constexpr bool _copyable = (... && detail::_makeable<data_t, Ts, Ts const &>);
  static constexpr bool _movable = (... && detail::_makeable<data_t, Ts, Ts>);
  static constexpr bool _nothrow_copyable = (... && detail::_nothrow_makeable<data_t, Ts, Ts const &>);
  static constexpr bool _nothrow_movable = (... && detail::_nothrow_makeable<data_t, Ts, Ts>);

  // Assignment requires of every alternative its own `operator=`, and `_reassign` uses it whenever
  // the incoming alternative is the one already held; a different alternative is replaced by
  // construction, through `_reinit`. Each helper chooses its arm per alternative - so the choice
  // belongs INSIDE the fold: hoisting it out would demand that one arm serve them all, and would
  // reject a sum whose alternatives simply need different ones. The nothrow disjunct is what keeps
  // the strong guarantee without a valueless state - some arm must be unable to lose the value in
  // hand.
  static constexpr bool _copy_assignable
      = (...
         && (::std::is_copy_assignable_v<Ts> && detail::_makeable<data_t, Ts, Ts const &>
             && (detail::_nothrow_makeable<data_t, Ts, Ts const &> || detail::_nothrow_makeable<data_t, Ts, Ts>)));
  static constexpr bool _nothrow_copy_assignable
      = (... && (::std::is_nothrow_copy_assignable_v<Ts> && detail::_nothrow_makeable<data_t, Ts, Ts const &>));
  static constexpr bool _move_assignable
      = (... && (::std::is_move_assignable_v<Ts> && detail::_nothrow_makeable<data_t, Ts, Ts>));
  static constexpr bool _nothrow_move_assignable = (... && ::std::is_nothrow_move_assignable_v<Ts>);

  // Each special member is trivial exactly when every alternative permits it, with the gates
  // `std::variant` uses. The trivial arm IS the compiler's defaulted member - for the assignments,
  // member-wise copying of the union's object representation - which for types passing these gates
  // is observationally identical to the general arms: a pure optimization, not different semantics.
  static constexpr bool _trivially_destructible = (... && ::std::is_trivially_destructible_v<Ts>);
  static constexpr bool _trivially_copy_constructible = (... && ::std::is_trivially_copy_constructible_v<Ts>);
  static constexpr bool _trivially_move_constructible = (... && ::std::is_trivially_move_constructible_v<Ts>);
  static constexpr bool _trivially_copy_assignable
      = (...
         && (::std::is_trivially_copy_constructible_v<Ts> && ::std::is_trivially_copy_assignable_v<Ts>
             && ::std::is_trivially_destructible_v<Ts>));
  static constexpr bool _trivially_move_assignable
      = (...
         && (::std::is_trivially_move_constructible_v<Ts> && ::std::is_trivially_move_assignable_v<Ts>
             && ::std::is_trivially_destructible_v<Ts>));

  /**
   * @brief TODO
   *
   * @tparam Ts I
   */
  template <::std::size_t I> using select_nth = detail::select_nth_t<I, Ts...>;

  /**
   * @brief TODO
   *
   * @tparam T TODO
   */
  template <typename T> static constexpr bool has_type = data_t::template has_type<T>;

  // Every dispatch below is nothrow exactly when the call on each alternative is: which one runs is
  // a run-time choice, so one throwing alternative is enough to make the whole dispatch throwing.
  template <typename Ret>
  [[nodiscard]] constexpr auto
  _invoke(auto &&fn) const & noexcept(detail::_is_nothrow_rtst_invocable<Ret, decltype(fn), sum const &>)
  {
    return detail::invoke_type_variadic_union<Ret, data_t>(this->data, this->index, FWD(fn));
  }

  template <typename Ret>
  [[nodiscard]] constexpr auto
  _invoke(auto &&fn) && noexcept(detail::_is_nothrow_rtst_invocable<Ret, decltype(fn), sum &&>)
  {
    return detail::invoke_type_variadic_union<Ret, data_t>(::std::move(*this).data, ::std::move(*this).index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto _transform(Fn &&fn) const & noexcept(
      detail::_is_nothrow_rtst_invocable<
          typename detail::_sum_invoke_type_result<detail::_collapsing_sum_tag, Fn &&, sum const &>::type, Fn &&,
          sum const &>) ->
      typename detail::_sum_invoke_type_result<detail::_collapsing_sum_tag, Fn &&, sum const &>::type
  {
    using type = detail::_sum_invoke_type_result<detail::_collapsing_sum_tag, Fn &&, sum const &>::type;
    return detail::invoke_type_variadic_union<type, data_t>(this->data, index, FWD(fn));
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto _transform(Fn &&fn) && noexcept(
      detail::_is_nothrow_rtst_invocable<
          typename detail::_sum_invoke_type_result<detail::_collapsing_sum_tag, Fn &&, sum &&>::type, Fn &&, sum &&>) ->
      typename detail::_sum_invoke_type_result<detail::_collapsing_sum_tag, Fn &&, sum &&>::type
  {
    using type = detail::_sum_invoke_type_result<detail::_collapsing_sum_tag, Fn &&, sum &&>::type;
    return detail::invoke_type_variadic_union<type, data_t>(::std::move(*this).data, index, FWD(fn));
  }

  /**
   * @brief TODO
   *
   * @tparam T TODO
   * @param v TODO
   */
  template <typename T>
  constexpr sum(T &&v) // NOSONAR cpp:S1709,S6458 implicit arm of the explicit pair; has_type excludes self
      noexcept(detail::_nothrow_makeable<data_t, ::std::remove_cvref_t<T>, decltype(v)>)
    requires has_type<::std::remove_cvref_t<T>> && (detail::_makeable<data_t, ::std::remove_cvref_t<T>, decltype(v)>)
                 && (::std::is_convertible_v<decltype(v), ::std::remove_cvref_t<T>>)
      : data(detail::make_variadic_union<::std::remove_cvref_t<T>, data_t>(FWD(v))),
        index(detail::type_index<::std::remove_cvref_t<T>, Ts...>)
  {
  }

  /**
   * @brief TODO
   *
   * @tparam T TODO
   * @param v TODO
   */
  template <typename T>
  constexpr explicit sum(T &&v) // NOSONAR cpp:S6458 has_type excludes self
      noexcept(detail::_nothrow_makeable<data_t, ::std::remove_cvref_t<T>, decltype(v)>)
    requires has_type<::std::remove_cvref_t<T>> && (detail::_makeable<data_t, ::std::remove_cvref_t<T>, decltype(v)>)
                 && (not ::std::is_convertible_v<decltype(v), ::std::remove_cvref_t<T>>)
      : data(detail::make_variadic_union<::std::remove_cvref_t<T>, data_t>(FWD(v))),
        index(detail::type_index<::std::remove_cvref_t<T>, Ts...>)
  {
  }

  /**
   * @brief TODO
   *
   * @tparam T TODO
   * @param args TODO
   */
  template <typename T>
  constexpr explicit sum(::std::in_place_type_t<T>,
                         auto &&...args) noexcept(detail::_nothrow_makeable<data_t, T, decltype(args)...>)
    requires has_type<T> && detail::_makeable<data_t, T, decltype(args)...>
      : data(detail::make_variadic_union<T, data_t>(FWD(args)...)), index(detail::type_index<T, Ts...>)
  {
  }

  /**
   * @brief TODO
   *
   * @tparam Tx TODO
   * @param arg TODO
   */
  template <typename... Tx>
  constexpr sum(sum<Tx...> const &arg) // NOSONAR cpp:S1709 implicit widening by design
      noexcept((... && detail::_nothrow_makeable<data_t, Tx, Tx const &>))
    requires detail::is_superset_of<sum, sum<Tx...>> && (not ::std::is_same_v<sum, sum<Tx...>>)
                 && (... && detail::_makeable<data_t, Tx, Tx const &>) && (sizeof...(Tx) > 0)
      : data(FWD(arg).template _invoke<data_t>([]<typename T>(::std::in_place_type_t<T>, auto &&v) {
          return detail::make_variadic_union<T, data_t>(FWD(v));
        })),
        index(FWD(arg).template _invoke<::std::size_t>([]<typename T>(::std::in_place_type_t<T>, auto &&) { //
          return detail::type_index<T, Ts...>;
        }))
  {
  }

  /**
   * @brief TODO
   *
   * @tparam Tx TODO
   * @param arg TODO
   */
  template <typename... Tx>
  constexpr sum(sum<Tx...> &&arg) // NOSONAR cpp:S1709 implicit widening by design
      noexcept((... && detail::_nothrow_makeable<data_t, Tx, Tx>))
    requires detail::is_superset_of<sum, sum<Tx...>> && (not ::std::is_same_v<sum, sum<Tx...>>)
                 && (... && detail::_makeable<data_t, Tx, Tx>) && (sizeof...(Tx) > 0)
      : data(FWD(arg).template _invoke<data_t>([]<typename T>(::std::in_place_type_t<T>, auto &&v) {
          return detail::make_variadic_union<T, data_t>(FWD(v));
        })),
        index(FWD(arg).template _invoke<::std::size_t>([]<typename T>(::std::in_place_type_t<T>, auto &&) { //
          return detail::type_index<T, Ts...>;
        }))
  {
  }

  /**
   * @brief TODO
   *
   * @tparam Tx TODO
   * @param arg TODO
   */
  template <typename... Tx>
  constexpr sum(::std::in_place_type_t<sum<Tx...>>, some_sum auto &&arg) //
      noexcept((... && detail::_nothrow_makeable<data_t, Tx, apply_const_lvalue_t<decltype(arg), Tx &&>>))
    requires ::std::is_same_v<::std::remove_cvref_t<decltype(arg)>, sum<Tx...>>
                 && detail::is_superset_of<sum, sum<Tx...>> && (sizeof...(Tx) > 0)
      : data(FWD(arg).template _invoke<data_t>([]<typename T>(::std::in_place_type_t<T>, auto &&v) {
          return detail::make_variadic_union<T, data_t>(FWD(v));
        })),
        index(FWD(arg).template _invoke<::std::size_t>([]<typename T>(::std::in_place_type_t<T>, auto &&) { //
          return detail::type_index<T, Ts...>;
        }))
  {
  }

  /**
   * @brief TODO
   *
   * @param other TODO
   */
  constexpr sum(sum const &other)
    requires _trivially_copy_constructible
  = default;
  constexpr sum(sum const &other) noexcept(_nothrow_copyable)
    requires(not _trivially_copy_constructible) && _copyable
      : data(detail::invoke_type_variadic_union<data_t, data_t>(                 //
            other.data, other.index,                                             //
            []<typename T>(::std::in_place_type_t<T>, auto const &v) -> data_t { //
              return detail::make_variadic_union<T, data_t>(v);
            })),
        index(other.index)
  {
  }

  /**
   * @brief TODO
   *
   * @param other TODO
   */
  constexpr sum(sum &&other)
    requires _trivially_move_constructible
  = default;
  constexpr sum(sum &&other) noexcept(_nothrow_movable)
    requires(not _trivially_move_constructible) && _movable
      : data(detail::invoke_type_variadic_union<data_t, data_t>(            //
            ::std::move(other).data, other.index,                           //
            []<typename T>(::std::in_place_type_t<T>, auto &&v) -> data_t { //
              return detail::make_variadic_union<T, data_t>(FWD(v));
            })),
        index(other.index)
  {
  }

  constexpr ~sum()
    requires _trivially_destructible
  = default;
  constexpr ~sum() noexcept
    requires(not _trivially_destructible)
  {
    detail::invoke_type_variadic_union<void, data_t>( //
        this->data, index, [this]<typename T>(::std::in_place_type_t<T>, auto &&) {
          ::std::destroy_at(detail::ptr_variadic_union<T, data_t>(this->data));
        });
  }

  // Replaces the alternative in hand with a `T` built from `args`, mirroring reinit-expected
  // ([expected.object.assign]): a sum always holds one of its alternatives - there is no valueless
  // state to fall back on - so the one it holds is destroyed only once its replacement is certain.
  // Which arm applies is decided per alternative, not for the sum as a whole: a nothrow-copyable one
  // is built straight over the old, and one whose copy can throw is copied into a temporary first,
  // where only its (nothrow) move goes near the storage.
  //
  // The standard's third arm - snapshot the old, roll back on throw - belongs to assignment of the
  // SAME alternative, which is `_reassign`'s job; here the old alternative is a different type, and
  // admitting a snapshot arm would gate the incoming alternative on the outgoing one's movability.
  // An alternative that can be neither built nor moved without throwing is constrained away instead.
  //
  // The arm is chosen by asking the storage what it can do, never by a trait that restates it:
  // `std::is_nothrow_move_constructible_v` asks about `T(T&&)`, the storage performs `T{...}`, and
  // the two can disagree - braced initialization considers initializer-list constructors first, so a
  // type can promise a nothrow move and still throw from `T{std::move(t)}`. Believing that promise
  // here would destroy the old alternative and then fail to replace it.
  template <typename T, typename... Args>
  constexpr void _reinit(Args &&...args) noexcept(detail::_nothrow_makeable<data_t, T, Args...>)
    requires has_type<T> && detail::_makeable<data_t, T, Args...>
             && (detail::_nothrow_makeable<data_t, T, Args...> || detail::_nothrow_makeable<data_t, T, T>)
  {
    if constexpr (detail::_nothrow_makeable<data_t, T, Args...>) {
      ::std::destroy_at(this);
      ::std::construct_at(this, ::std::in_place_type<T>, FWD(args)...);
    } else {
      T tmp{FWD(args)...}; // may throw, and the storage is untouched until it cannot
      ::std::destroy_at(this);
      ::std::construct_at(this, ::std::in_place_type<T>, ::std::move(tmp)); // cannot throw: see above
    }
  }

  // Assigns the incoming alternative with its own `operator=` when it is the one already held: the
  // arms mirror `_reinit`'s shape, and the third is the standard's snapshot-and-restore - live here,
  // where old and new are the same type, so the constraint that admits the alternative also
  // guarantees its snapshot.
  template <typename T, typename V> constexpr void _reassign(V &&v) noexcept(::std::is_nothrow_assignable_v<T &, V>)
  {
    T *held = detail::ptr_variadic_union<T, data_t>(this->data);
    if constexpr (::std::is_nothrow_assignable_v<T &, V>) {
      *held = FWD(v);
    } else if constexpr (::std::is_nothrow_assignable_v<T &, T>) {
      T tmp{FWD(v)}; // may throw, and the value in hand is untouched until it cannot
      *held = ::std::move(tmp);
    } else if constexpr (detail::_nothrow_makeable<data_t, T, T>) {
      T snap{::std::move(*held)};
      try {
        *held = FWD(v);
      } catch (...) {
        ::std::destroy_at(this);
        ::std::construct_at(this, ::std::in_place_type<T>, ::std::move(snap)); // cannot throw: see above
        throw;
      }
    } else {
      T snap{*held}; // the copy that cannot throw, where the move is the one that can
      try {
        *held = FWD(v);
      } catch (...) {
        ::std::destroy_at(this);
        ::std::construct_at(this, ::std::in_place_type<T>, snap); // restored by that same nothrow copy
        throw;
      }
    }
  }

  /**
   * @brief Copy assignment, with the strong exception guarantee
   *
   * @param other TODO
   * @return TODO
   */
  // Assignment requires of every alternative its own `operator=`, and uses it when the incoming
  // alternative is the one already held; a different alternative is replaced by construction, as
  // [variant.assign] does. A sum therefore no longer offers an operation its alternative refuses -
  // a pack holding a reference deletes assignment precisely because C++ cannot rebind a reference,
  // and destroy-and-reconstruct would synthesize exactly that.
  constexpr sum &operator=(sum const &other)
    requires _trivially_copy_assignable
  = default;
  constexpr sum &operator=(sum const &other) noexcept(_nothrow_copy_assignable)
    requires(not _trivially_copy_assignable) && _copy_assignable
  {
    if (this != &other) {
      if (index == other.index) {
        detail::invoke_type_variadic_union<void, data_t>( //
            other.data, other.index,
            [this]<typename T>(::std::in_place_type_t<T>, auto const &v) { this->template _reassign<T>(v); });
      } else {
        detail::invoke_type_variadic_union<void, data_t>( //
            other.data, other.index,
            [this]<typename T>(::std::in_place_type_t<T>, auto const &v) { this->template _reinit<T>(v); });
      }
    }
    return *this;
  }

  /**
   * @brief Move assignment, with the strong exception guarantee
   *
   * @param other TODO
   * @return TODO
   */
  // The nothrow move construction is still demanded - `_reinit`'s replacement arm and `_reassign`'s
  // snapshot both rest on it - but the operator itself is only as nothrow as the alternatives' own
  // move assignment. Where moving can throw, a nothrow-copy-assignable sum is still assignable from
  // an rvalue, by copy.
  constexpr sum &operator=(sum &&other)
    requires _trivially_move_assignable
  = default;
  constexpr sum &operator=(sum &&other) noexcept(_nothrow_move_assignable)
    requires(not _trivially_move_assignable) && _move_assignable
  {
    if (this != &other) {
      if (index == other.index) {
        detail::invoke_type_variadic_union<void, data_t>( //
            ::std::move(other).data, other.index,
            [this]<typename T>(::std::in_place_type_t<T>, auto &&v) { this->template _reassign<T>(FWD(v)); });
      } else {
        detail::invoke_type_variadic_union<void, data_t>( //
            ::std::move(other).data, other.index,
            [this]<typename T>(::std::in_place_type_t<T>, auto &&v) { this->template _reinit<T>(FWD(v)); });
      }
    }
    return *this;
  }

  /**
   * @brief TODO
   *
   * @tparam T TODO
   * @return TODO
   */
  template <typename T>
    requires has_type<T>
  [[nodiscard]] constexpr bool has_value(::std::in_place_type_t<T> = ::std::in_place_type<T>) const noexcept
  {
    return detail::invoke_type_variadic_union<bool, data_t>( //
        this->data, index,
        []<typename U>(::std::in_place_type_t<U>, auto &&) constexpr -> bool { return ::std::is_same_v<T, U>; });
  }

  /**
   * @brief TODO
   *
   * @tparam T TODO
   * @return TODO
   */
  template <typename T>
    requires has_type<T>
  [[nodiscard]] constexpr T *get_ptr(::std::in_place_type_t<T> = ::std::in_place_type<T>) noexcept
  {
    return has_value(::std::in_place_type<T>) ? detail::ptr_variadic_union<T, data_t>(data) : nullptr;
  }

  /**
   * @brief TODO
   *
   * @tparam T TODO
   * @return TODO
   */
  template <typename T>
    requires has_type<T>
  [[nodiscard]] constexpr T const *get_ptr(::std::in_place_type_t<T> = ::std::in_place_type<T>) const noexcept
  {
    return has_value(::std::in_place_type<T>) ? detail::ptr_variadic_union<T, data_t>(data) : nullptr;
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto invoke(Fn &&fn, Args &&...args) & noexcept(
      detail::_is_nothrow_rts_invocable<
          typename detail::_sum_invoke_result<detail::_invoke_autodetect_tag, Fn &&, sum &, Args &&...>::type, Fn &&,
          sum &, Args &&...>) ->
      typename detail::_sum_invoke_result<detail::_invoke_autodetect_tag, Fn &&, sum &, Args &&...>::type
    requires typelist_invocable<Fn, sum &, Args &&...>
  {
    using type = detail::_sum_invoke_result<detail::_invoke_autodetect_tag, Fn &&, sum &, Args &&...>::type;
    return detail::invoke_variadic_union<type, data_t>(this->data, index, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto invoke(Fn &&fn, Args &&...args) const & noexcept(
      detail::_is_nothrow_rts_invocable<
          typename detail::_sum_invoke_result<detail::_invoke_autodetect_tag, Fn &&, sum const &, Args &&...>::type,
          Fn &&, sum const &, Args &&...>) ->
      typename detail::_sum_invoke_result<detail::_invoke_autodetect_tag, Fn &&, sum const &, Args &&...>::type
    requires typelist_invocable<Fn, sum const &, Args &&...>
  {
    using type = detail::_sum_invoke_result<detail::_invoke_autodetect_tag, Fn &&, sum const &, Args &&...>::type;
    return detail::invoke_variadic_union<type, data_t>(this->data, index, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto invoke(Fn &&fn, Args &&...args) && noexcept(
      detail::_is_nothrow_rts_invocable<
          typename detail::_sum_invoke_result<detail::_invoke_autodetect_tag, Fn &&, sum &&, Args &&...>::type, Fn &&,
          sum &&, Args &&...>) ->
      typename detail::_sum_invoke_result<detail::_invoke_autodetect_tag, Fn &&, sum &&, Args &&...>::type
    requires typelist_invocable<Fn, sum &&, Args &&...>
  {
    using type = detail::_sum_invoke_result<detail::_invoke_autodetect_tag, Fn &&, sum &&, Args &&...>::type;
    return detail::invoke_variadic_union<type, data_t>(::std::move(*this).data, index, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto invoke(Fn &&fn, Args &&...args) const && noexcept(
      detail::_is_nothrow_rts_invocable<
          typename detail::_sum_invoke_result<detail::_invoke_autodetect_tag, Fn &&, sum const &&, Args &&...>::type,
          Fn &&, sum const &&, Args &&...>) ->
      typename detail::_sum_invoke_result<detail::_invoke_autodetect_tag, Fn &&, sum const &&, Args &&...>::type
    requires typelist_invocable<Fn, sum const &&, Args &&...>
  {
    using type = detail::_sum_invoke_result<detail::_invoke_autodetect_tag, Fn &&, sum const &&, Args &&...>::type;
    return detail::invoke_variadic_union<type, data_t>(::std::move(*this).data, index, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Ret TODO
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto
  invoke_r(Fn &&fn, Args &&...args) & noexcept(detail::_is_nothrow_rts_invocable<Ret, Fn &&, sum &, Args &&...>) -> Ret
    requires typelist_invocable_r<Ret, Fn, sum &, Args &&...>
  {
    using type = detail::_sum_invoke_result<Ret, Fn &&, sum &, Args &&...>::type;
    return detail::invoke_variadic_union<type, data_t>(this->data, index, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Ret TODO
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn, Args &&...args) const & noexcept(
      detail::_is_nothrow_rts_invocable<Ret, Fn &&, sum const &, Args &&...>) -> Ret
    requires typelist_invocable_r<Ret, Fn, sum const &, Args &&...>
  {
    using type = detail::_sum_invoke_result<Ret, Fn &&, sum const &, Args &&...>::type;
    return detail::invoke_variadic_union<type, data_t>(this->data, index, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Ret TODO
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto
  invoke_r(Fn &&fn, Args &&...args) && noexcept(detail::_is_nothrow_rts_invocable<Ret, Fn &&, sum &&, Args &&...>)
      -> Ret
    requires typelist_invocable_r<Ret, Fn, sum &&, Args &&...>
  {
    using type = detail::_sum_invoke_result<Ret, Fn &&, sum &&, Args &&...>::type;
    return detail::invoke_variadic_union<type, data_t>(::std::move(*this).data, index, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Ret TODO
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Ret, typename Fn, typename... Args>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn, Args &&...args) const && noexcept(
      detail::_is_nothrow_rts_invocable<Ret, Fn &&, sum const &&, Args &&...>) -> Ret
    requires typelist_invocable_r<Ret, Fn, sum const &&, Args &&...>
  {
    using type = detail::_sum_invoke_result<Ret, Fn &&, sum const &&, Args &&...>::type;
    return detail::invoke_variadic_union<type, data_t>(::std::move(*this).data, index, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto transform(Fn &&fn, Args &&...args) & noexcept(
      detail::_is_nothrow_rts_invocable<
          typename detail::_sum_invoke_result<detail::_collapsing_sum_tag, Fn &&, sum &, Args &&...>::type, Fn &&,
          sum &, Args &&...>) ->
      typename detail::_sum_invoke_result<detail::_collapsing_sum_tag, Fn &&, sum &, Args &&...>::type
    requires typelist_invocable<Fn, sum &, Args &&...>
  {
    using type = detail::_sum_invoke_result<detail::_collapsing_sum_tag, Fn &&, sum &, Args &&...>::type;
    return detail::invoke_variadic_union<type, data_t>(this->data, index, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto transform(Fn &&fn, Args &&...args) const & noexcept(
      detail::_is_nothrow_rts_invocable<
          typename detail::_sum_invoke_result<detail::_collapsing_sum_tag, Fn &&, sum const &, Args &&...>::type, Fn &&,
          sum const &, Args &&...>) ->
      typename detail::_sum_invoke_result<detail::_collapsing_sum_tag, Fn &&, sum const &, Args &&...>::type
    requires typelist_invocable<Fn, sum const &, Args &&...>
  {
    using type = detail::_sum_invoke_result<detail::_collapsing_sum_tag, Fn &&, sum const &, Args &&...>::type;
    return detail::invoke_variadic_union<type, data_t>(this->data, index, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto transform(Fn &&fn, Args &&...args) && noexcept(
      detail::_is_nothrow_rts_invocable<
          typename detail::_sum_invoke_result<detail::_collapsing_sum_tag, Fn &&, sum &&, Args &&...>::type, Fn &&,
          sum &&, Args &&...>) ->
      typename detail::_sum_invoke_result<detail::_collapsing_sum_tag, Fn &&, sum &&, Args &&...>::type
    requires typelist_invocable<Fn, sum &&, Args &&...>
  {
    using type = detail::_sum_invoke_result<detail::_collapsing_sum_tag, Fn &&, sum &&, Args &&...>::type;
    return detail::invoke_variadic_union<type, data_t>(::std::move(*this).data, index, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @tparam Args TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn, typename... Args>
  [[nodiscard]] constexpr auto transform(Fn &&fn, Args &&...args) const && noexcept(
      detail::_is_nothrow_rts_invocable<
          typename detail::_sum_invoke_result<detail::_collapsing_sum_tag, Fn &&, sum const &&, Args &&...>::type,
          Fn &&, sum const &&, Args &&...>) ->
      typename detail::_sum_invoke_result<detail::_collapsing_sum_tag, Fn &&, sum const &&, Args &&...>::type
    requires typelist_invocable<Fn, sum const &&, Args &&...>
  {
    using type = detail::_sum_invoke_result<detail::_collapsing_sum_tag, Fn &&, sum const &&, Args &&...>::type;
    return detail::invoke_variadic_union<type, data_t>(::std::move(*this).data, index, FWD(fn), FWD(args)...);
  }
};

// CTAD for single-element sum
template <typename T> explicit sum(::std::in_place_type_t<T>, auto &&...) -> sum<T>;
template <typename T> explicit sum(T) -> sum<::std::remove_cvref_t<T>>;

namespace detail {
// The value lift builds `sum<remove_cvref_t<Src>>` - a class whose body refuses an in_place tag as
// an alternative. MSVC (C++20 mode) compiles a candidate's noexcept-specifier once deduction
// succeeds, BEFORE the constraint rejects the tag, so the specifier must not name that sum unless
// the guard holds: a guarded specialization, as `_nothrow_eq_with` is, and for the same reason.
template <typename Src> constexpr inline bool _nothrow_sum_lift = false;
template <typename Src>
  requires(not some_in_place_type<Src>)
constexpr inline bool _nothrow_sum_lift<Src> = ::std::is_nothrow_constructible_v<sum<::std::remove_cvref_t<Src>>, Src>;
} // namespace detail

// Lifts
/**
 * @brief TODO
 *
 * @param src TODO
 * @return TODO
 */
[[nodiscard]] constexpr auto as_sum(auto &&src) //
    noexcept(detail::_nothrow_sum_lift<decltype(src)>) -> decltype(auto)
  requires(not some_in_place_type<decltype(src)>)
{
  return sum<::std::remove_cvref_t<decltype(src)>>(FWD(src));
}

/**
 * @brief TODO
 *
 * @param src TODO
 * @return TODO
 */
template <typename T>
[[nodiscard]] constexpr auto as_sum(::std::in_place_type_t<T>, auto &&...args) //
    noexcept(::std::is_nothrow_constructible_v<sum<T>, ::std::in_place_type_t<T>, decltype(args)...>) -> decltype(auto)
  requires ::std::is_constructible_v<sum<T>, ::std::in_place_type_t<T>, decltype(args)...>
{
  return sum<T>(::std::in_place_type<T>, FWD(args)...);
}

/**
 * @brief TODO
 *
 * @tparam Ts TODO
 * @tparam Tx TODO
 * @param lh TODO
 * @param rh TODO
 * @return TODO
 */
template <typename... Ts, typename... Tx>
[[nodiscard]] constexpr bool operator==(sum<Ts...> const &lh, sum<Tx...> const &rh) //
    noexcept((... && detail::_nothrow_eq_with<Ts, Tx...>))
  requires(... && (::std::equality_comparable<Ts> || not detail::type_one_of<Ts, Tx...>)) //
          && ((sizeof...(Ts)) > 0) && ((sizeof...(Tx)) > 0)
{
  return lh.template _invoke<bool>([&rh]<typename T>(::std::in_place_type_t<T> d, auto const &lh) {
    if constexpr (::std::remove_cvref_t<decltype(rh)>::template has_type<T>) {
      return rh.has_value(d) && lh == *rh.get_ptr(d);
    } else {
      return false;
    }
  });
}

// No operator!=: C++20 rewrites `a != b` as `!(a == b)`, so the synthesized candidate inherits
// operator=='s constraints and the two cannot drift apart.

/**
 * @brief TODO
 *
 * @tparam Ts TODO
 */
template <typename... Ts>
using sum_for = detail::_collapsing_sum::normalized<::fn::sum, detail::_collapsing_sum::flattened<Ts...>>::type;

} // namespace fn

#endif // INCLUDE_FN_SUM
