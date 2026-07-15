// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_DETAIL_PACK_IMPL
#define INCLUDE_FN_DETAIL_PACK_IMPL

#include <fn/detail/functional.hpp>
#include <fn/detail/fwd.hpp>
#include <fn/detail/macro_fwd.hpp>
#include <fn/detail/meta.hpp>
#include <fn/detail/traits.hpp>

#include <type_traits>
#include <utility>

namespace fn::detail {
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

template <typename, typename... Ts> struct pack_impl;

template <typename... Ts> struct _pack_append;
// A pack never holds a sum. The specialization is defined but has no `type`, so naming
// `append_type<sum>` is a substitution failure rather than a use of an incomplete type; and the
// two specializations must exclude each other, or both match a sum and the choice is ambiguous.
template <typename T, typename... Ts>
  requires _some_sum<T>
struct _pack_append<T, Ts...> {};
template <typename T, typename... Ts>
  requires(not _some_pack<T>) && (not _some_sum<T>)
struct _pack_append<T, Ts...> {
  using impl = pack_impl<::std::index_sequence_for<Ts..., T>, Ts..., T>;
  using type = ::fn::pack<Ts..., T>;
};

template <typename T, typename... Ts> struct _pack_append_pack;
template <typename... Tx, typename... Ts> struct _pack_append_pack<::fn::pack<Tx...>, Ts...> {
  using impl = pack_impl<::std::index_sequence_for<Ts..., Tx...>, Ts..., Tx...>;
  using type = ::fn::pack<Ts..., Tx...>;
};
template <typename T, typename... Ts>
  requires _some_pack<T>
struct _pack_append<T, Ts...> {
  using impl = _pack_append_pack<::std::remove_cvref_t<T>, Ts...>::impl;
  using type = _pack_append_pack<::std::remove_cvref_t<T>, Ts...>::type;
};

template <::std::size_t... Is, typename... Ts>
struct pack_impl<::std::index_sequence<Is...>, Ts...> : _element<Is, Ts>... {
  static constexpr ::std::size_t size = sizeof...(Is);

  template <typename Self, typename Fn, typename... Args>
    requires(not(... || (_some_pack<Args> || _some_sum<Args>)))
  static constexpr auto _swap_invoke(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(::std::is_nothrow_invocable_v<Fn &&, Args &&..., apply_const_lvalue_t<Self, Ts &&>...>)
          -> ::std::invoke_result<Fn &&, Args &&..., apply_const_lvalue_t<Self, Ts &&>...>::type
    requires(::std::is_invocable<Fn &&, Args && ..., apply_const_lvalue_t<Self, Ts &&>...>::value)
  {
    return ::std::invoke(FWD(fn), FWD(args)...,
                         static_cast<apply_const_lvalue_t<Self, Ts &&>>(FWD(self)._element<Is, Ts>::v)...);
  }

  template <typename Self, typename Fn, typename... Args>
    requires(not(... || (_some_pack<Args> || _some_sum<Args>)))
  static constexpr auto _invoke(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(_is_nothrow_applicable<Fn &&, apply_const_lvalue_t<Self, Ts &&>..., Args &&...>::value)
          -> _apply_result<Fn &&, apply_const_lvalue_t<Self, Ts &&>..., Args &&...>::type
    requires(_is_applicable<Fn &&, apply_const_lvalue_t<Self, Ts &&>..., Args && ...>::value)
  {
    return ::fn::detail::_apply(
        FWD(fn), static_cast<apply_const_lvalue_t<Self, Ts &&>>(FWD(self)._element<Is, Ts>::v)..., FWD(args)...);
  }

  template <typename Ret, typename Self, typename Fn, typename... Args>
    requires(not(... || (_some_pack<Args> || _some_sum<Args>)))
  static constexpr auto _apply_r(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(_is_nothrow_applicable_r<Ret, Fn &&, apply_const_lvalue_t<Self, Ts &&>..., Args &&...>::value) -> Ret
    requires(_is_applicable_r<Ret, Fn &&, apply_const_lvalue_t<Self, Ts &&>..., Args && ...>::value)
  {
    return ::fn::detail::_apply_r<Ret>(
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
    requires(not _some_sum<T>) && (not _some_pack<T>) && _relocatable<Self> && _makeable_element<T, decltype(args)...>
  {
    return {static_cast<apply_const_lvalue_t<Self, Ts &&>>(FWD(self)._element<Is, Ts>::v)...,
            _make_element<T>(FWD(args)...)};
  }

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
    return FWD(other)._invoke(FWD(other), [&self](auto &&...args) {
      return type{static_cast<apply_const_lvalue_t<Self, Ts &&>>(FWD(self)._element<Is, Ts>::v)..., FWD(args)...};
    });
  }
};

} // namespace fn::detail

#endif // INCLUDE_FN_DETAIL_PACK_IMPL
