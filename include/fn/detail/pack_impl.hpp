// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_DETAIL_PACK_IMPL
#define INCLUDE_FN_DETAIL_PACK_IMPL

#include <fn/detail/fwd.hpp>
#include <fn/detail/meta.hpp>
#include <fn/detail/traits.hpp>
#include <libfn_version.hpp>

#include <pfn/functional.hpp>

#include <functional>
#include <type_traits>
#include <utility>

#include <fn/detail/macro_begin.hpp>

namespace fn::inline LIBFN_VERSION::detail {
template <::std::size_t I, typename T> struct _element {
  static_assert(not ::std::is_rvalue_reference_v<T>);
  T v; // NOSONAR cpp:S6226 MSVC ignores the attribute
};

// Initializes one element. A reference element is spelled as a cast rather than `T{arg}`: the two
// are the same thing ([expr.type.conv]/1.3 defines the latter as direct-initializing an invented
// variable of type T, and [dcl.init.list]/3.9 initializes a reference from a single element whose
// type is reference-related to it, i.e. binds), but gcc reads the braced form as materializing a
// temporary and rejects the bind - while accepting the equivalent declaration. The cast keeps both
// compilers on the binding path.
template <typename T>
[[nodiscard]] constexpr auto _make_element(auto &&...args) noexcept(_nothrow_initializable<T, decltype(args)...>) -> T
  requires _initializable<T, decltype(args)...>
{
  if constexpr (::std::is_reference_v<T>)
    return T(FWD(args)...); // a single argument, so this is a cast expression: it binds
  else
    return T{FWD(args)...};
}

// The two questions anyone above may ask about constructing an element, asked OF the function that
// constructs it rather than restated in terms of a trait - the same discipline as `_makeable` beside
// `make_variadic_union`, and for the same reason: a restatement can drift from the deed.
template <typename T, typename... Args>
concept _makeable_element = requires { _make_element<T>(::std::declval<Args>()...); };

template <typename T, typename... Args>
concept _nothrow_makeable_element = requires { requires noexcept(_make_element<T>(::std::declval<Args>()...)); };

// One element's relocation into a new pack: the copy-initialization its holder performs
// ([dcl.init.aggr]/4.3, reached through brace elision), asked of the holder one element at a time -
// `_element<I, T>{src}` elides into the same member copy-initialization. This excludes explicit
// constructors, where `is_[nothrow_]constructible_v` would admit them.
template <typename E, typename Src>
concept _relocatable_element = requires { E{::std::declval<Src>()}; };

template <typename E, typename Src>
concept _nothrow_relocatable_element = requires { requires noexcept(E{::std::declval<Src>()}); };

// The algebra's own constructors are not elements: a copack must distribute over the product, and a
// nested pack is a non-canonical spelling of the flat product with no consistent shape (apply
// flattens it, the tuple protocol preserves it). Foreign structured types stay opaque atoms.
template <typename T> static constexpr bool _is_valid_pack_element = (not _some_copack<T>) && (not _some_pack<T>);

template <typename T> constexpr inline bool _spliceable_pack = false;
template <typename... Tx>
constexpr inline bool _spliceable_pack<::fn::pack<Tx...>> = (... && _is_valid_pack_element<Tx>);

template <typename, typename... Ts> struct pack_impl;

template <typename... Ts> struct _pack_append;
// A pack never holds a copack. The specialization is defined but has no `type`, so naming
// `append_type<copack>` is a substitution failure rather than a use of an incomplete type; and the
// specializations must exclude each other, or more than one matches and the choice is ambiguous.
template <typename T, typename... Ts>
  requires _some_copack<T>
struct _pack_append<T, Ts...> {};
template <typename T, typename... Ts>
  requires(not _some_pack<T>) && (not _some_copack<T>)
struct _pack_append<T, Ts...> {
  using impl = pack_impl<::std::index_sequence_for<Ts..., T>, Ts..., T>;
  using type = ::fn::pack<Ts..., T>;
};

template <typename T, typename... Ts> struct _pack_append_pack;
template <typename... Tx, typename... Ts> struct _pack_append_pack<::fn::pack<Tx...>, Ts...> {
  using impl = pack_impl<::std::index_sequence_for<Ts..., Tx...>, Ts..., Tx...>;
  using type = ::fn::pack<Ts..., Tx...>;
};
// A tag can NAME a pack type whose own elements are invalid without instantiating it; splicing it
// would instantiate the ill-formed result. Same shape as the copack case: defined without `type`, so
// asking stays a substitution failure.
template <typename T, typename... Ts>
  requires _some_pack<T> && (not _spliceable_pack<::std::remove_cvref_t<T>>)
struct _pack_append<T, Ts...> {};
template <typename T, typename... Ts>
  requires _some_pack<T> && _spliceable_pack<::std::remove_cvref_t<T>>
struct _pack_append<T, Ts...> {
  using impl = _pack_append_pack<::std::remove_cvref_t<T>, Ts...>::impl;
  using type = _pack_append_pack<::std::remove_cvref_t<T>, Ts...>::type;
};

template <::std::size_t... Is, typename... Ts>
struct pack_impl<::std::index_sequence<Is...>, Ts...> : _element<Is, Ts>... {
  static constexpr ::std::size_t size = sizeof...(Is);

  // Comparison is element-wise rather than defaulted, because a defaulted comparison over a
  // reference member is deleted outright, where a reference element must compare its REFERENT - the
  // semantics of optional<T&> and of std::tuple. Asked of the elements as the fold reaches them, so
  // an element which cannot be compared leaves the operator non-viable rather than ill-formed; the
  // `&&` fold answers true for the empty pack, and the `||` fold stops at the first element that
  // orders. No ordering is synthesized from `<`: an element brings its own `<=>` or none.
  template <typename Self>
  static constexpr auto _equal(Self const &lh, Self const &rh) //
      noexcept((... && noexcept(static_cast<bool>(lh._element<Is, Ts>::v == rh._element<Is, Ts>::v)))) -> bool
    requires(... && requires {
      { lh._element<Is, Ts>::v == rh._element<Is, Ts>::v } -> ::std::convertible_to<bool>;
    })
  {
    return (... && static_cast<bool>(lh._element<Is, Ts>::v == rh._element<Is, Ts>::v));
  }

  // The common category is `void` - a valid return type, not a substitution failure - when an
  // element's `<=>` answers something that is not a comparison category, or when the elements'
  // categories have no common one. Left unsaid, that would make the operator viable to ask about and
  // ill-formed to use, so it is said.
  template <typename Self>
  static constexpr auto _compare(Self const &lh, Self const &rh) //
      noexcept((... && noexcept(lh._element<Is, Ts>::v <=> rh._element<Is, Ts>::v)))
          -> ::std::common_comparison_category_t<decltype(lh._element<Is, Ts>::v <=> rh._element<Is, Ts>::v)...>
    requires(not ::std::is_void_v<
             ::std::common_comparison_category_t<decltype(lh._element<Is, Ts>::v <=> rh._element<Is, Ts>::v)...>>)
  {
    using type = ::std::common_comparison_category_t<decltype(lh._element<Is, Ts>::v <=> rh._element<Is, Ts>::v)...>;
    type result = type::equivalent;
    [[maybe_unused]] bool const ordered
        = (((result = (lh._element<Is, Ts>::v <=> rh._element<Is, Ts>::v)) != 0) || ...);
    return result;
  }

  template <typename Self, typename Fn, typename... Args>
    requires(not(... || (_some_pack<Args> || _some_copack<Args>)))
  static constexpr auto _swap_invoke(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(::std::is_nothrow_invocable_v<Fn &&, Args &&..., apply_const_lvalue_t<Self, Ts &&>...>)
          -> ::std::invoke_result<Fn &&, Args &&..., apply_const_lvalue_t<Self, Ts &&>...>::type
    requires(::std::is_invocable<Fn &&, Args && ..., apply_const_lvalue_t<Self, Ts &&>...>::value)
  {
    return ::std::invoke(FWD(fn), FWD(args)...,
                         static_cast<apply_const_lvalue_t<Self, Ts &&>>(FWD(self)._element<Is, Ts>::v)...);
  }

  // The pack's elements are the argument list, and every argument is terminal: handed over whole
  // via INVOKE, never re-entering elimination. Routing through the engine instead would give a
  // lone tuple-like element std::apply's meaning, making its treatment depend on its siblings.
  template <typename Self, typename Fn, typename... Args>
    requires(not(... || (_some_pack<Args> || _some_copack<Args>)))
  static constexpr auto _apply(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(::std::is_nothrow_invocable_v<Fn &&, apply_const_lvalue_t<Self, Ts &&>..., Args &&...>)
          -> ::std::invoke_result<Fn &&, apply_const_lvalue_t<Self, Ts &&>..., Args &&...>::type
    requires(::std::is_invocable<Fn &&, apply_const_lvalue_t<Self, Ts &&>..., Args && ...>::value)
  {
    return ::std::invoke(FWD(fn), static_cast<apply_const_lvalue_t<Self, Ts &&>>(FWD(self)._element<Is, Ts>::v)...,
                         FWD(args)...);
  }

  template <typename Ret, typename Self, typename Fn, typename... Args>
    requires(not(... || (_some_pack<Args> || _some_copack<Args>)))
  static constexpr auto _apply_r(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(::std::is_nothrow_invocable_r_v<Ret, Fn &&, apply_const_lvalue_t<Self, Ts &&>..., Args &&...>) -> Ret
    requires(::std::is_invocable_r<Ret, Fn &&, apply_const_lvalue_t<Self, Ts &&>..., Args && ...>::value)
  {
    return ::pfn::invoke_r<Ret>(
        FWD(fn), static_cast<apply_const_lvalue_t<Self, Ts &&>>(FWD(self)._element<Is, Ts>::v)..., FWD(args)...);
  }

  template <::std::size_t I, typename Self>
  static constexpr decltype(auto) _get(Self &&self) noexcept
    requires(I < size)
  {
    return static_cast<apply_const_lvalue_t<Self, select_nth_t<I, Ts...> &&>>(
        FWD(self)._element<I, select_nth_t<I, Ts...>>::v);
  }

  template <typename T> using append_type = _pack_append<T, Ts...>::type;

  // Every existing element is relocated into the new pack, so appending weighs that as well as the
  // construction of the element being appended.
  template <typename Self>
  static constexpr bool _relocatable
      = (... && _relocatable_element<_element<Is, Ts>, apply_const_lvalue_t<Self, Ts &&>>);

  template <typename Self>
  static constexpr bool _nothrow_relocatable
      = (... && _nothrow_relocatable_element<_element<Is, Ts>, apply_const_lvalue_t<Self, Ts &&>>);

  template <typename T, typename Self>
  static constexpr auto _append(Self &&self, auto &&...args) //
      noexcept(_nothrow_relocatable<Self> && _nothrow_makeable_element<T, decltype(args)...>)
          -> pack_impl<::std::index_sequence<Is..., size>, Ts..., T>
    requires(not _some_copack<T>) && (not _some_pack<T>)
            && _relocatable<Self> && _makeable_element<T, decltype(args)...>
  {
    return {static_cast<apply_const_lvalue_t<Self, Ts &&>>(FWD(self)._element<Is, Ts>::v)...,
            _make_element<T>(FWD(args)...)};
  }

  // Splicing is normalization, not elimination: the appended pack's elements relocate through
  // INVOKE, so a lone tuple-like element arrives whole instead of taking std::apply's meaning
  // through the engine's tuple arm.
  template <typename T, typename Self>
  static constexpr auto _append(Self &&self, auto &&other) //
      noexcept(_nothrow_relocatable<Self>
               && ::std::remove_cvref_t<T>::_impl::template _nothrow_relocatable<decltype(other)>) -> //
      typename _pack_append<::std::remove_cvref_t<T>, Ts...>::impl
    requires _some_pack<T> && (::std::is_same_v<::std::remove_cvref_t<decltype(other)>, ::std::remove_cvref_t<T>>)
             && _relocatable<Self> && ::std::remove_cvref_t<T>::_impl::template
  _relocatable<decltype(other)>
  {
    using type = _pack_append<::std::remove_cvref_t<T>, Ts...>::impl;
    return FWD(other)._swap_invoke(FWD(other), [&self](auto &&...args) {
      return type{static_cast<apply_const_lvalue_t<Self, Ts &&>>(FWD(self)._element<Is, Ts>::v)..., FWD(args)...};
    });
  }

  // The tag form constructs the named pack from the arguments, then splices it: appending a pack
  // means concatenation in every spelling. One relocation per element more than appending the
  // elements directly - the spelling of an append is a performance knob, never a result change.
  template <typename T, typename Self>
  static constexpr auto _append(Self &&self, auto &&...args) //
      noexcept(_nothrow_relocatable<Self> && _nothrow_initializable<::std::remove_cvref_t<T>, decltype(args)...>
               && ::std::remove_cvref_t<T>::_impl::template _nothrow_relocatable<::std::remove_cvref_t<T>>) -> //
      typename _pack_append<::std::remove_cvref_t<T>, Ts...>::impl
    requires _some_pack<T>
             && (not(sizeof...(args) == 1
                     && (... && ::std::is_same_v<::std::remove_cvref_t<decltype(args)>, ::std::remove_cvref_t<T>>)))
             && _relocatable<Self> && _initializable<::std::remove_cvref_t<T>, decltype(args)...>
             && ::std::remove_cvref_t<T>::_impl::template
  _relocatable<::std::remove_cvref_t<T>>
  {
    using pack_t = ::std::remove_cvref_t<T>;
    using type = _pack_append<pack_t, Ts...>::impl;
    return pack_t::_impl::_swap_invoke(pack_t{FWD(args)...}, [&self](auto &&...elems) {
      return type{static_cast<apply_const_lvalue_t<Self, Ts &&>>(FWD(self)._element<Is, Ts>::v)..., FWD(elems)...};
    });
  }
};

} // namespace fn::inline LIBFN_VERSION::detail

#include <fn/detail/macro_end.hpp>

#endif // INCLUDE_FN_DETAIL_PACK_IMPL
