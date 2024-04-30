// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_PACK
#define INCLUDE_FUNCTIONAL_PACK

#include "functional/detail/pack_impl.hpp"
#include "functional/sum.hpp"

#include <type_traits>

namespace fn {

template <typename T>
concept some_pack = detail::_some_pack<T>;

template <typename... Ts> struct pack : detail::pack_impl<std::index_sequence_for<Ts...>, Ts...> {
  using _impl = detail::pack_impl<std::index_sequence_for<Ts...>, Ts...>;
  static_assert((... && (not some_sum<Ts>)));

  template <typename Arg> using append_type = pack<Ts..., Arg>;

  template <typename T>
  [[nodiscard]] constexpr auto append(std::in_place_type_t<T>, auto &&...args) & noexcept -> append_type<T>
    requires std::is_constructible_v<T, decltype(args)...>
             && requires { static_cast<append_type<T>>(_impl::template _append<T>(*this, FWD(args)...)); }
  {
    return {_impl::template _append<T>(*this, FWD(args)...)};
  }

  template <typename T>
  [[nodiscard]] constexpr auto append(std::in_place_type_t<T>, auto &&...args) const & noexcept -> append_type<T>
    requires std::is_constructible_v<T, decltype(args)...>
             && requires { static_cast<append_type<T>>(_impl::template _append<T>(*this, FWD(args)...)); }
  {
    return {_impl::template _append<T>(*this, FWD(args)...)};
  }

  template <typename T>
  [[nodiscard]] constexpr auto append(std::in_place_type_t<T>, auto &&...args) && noexcept -> append_type<T>
    requires std::is_constructible_v<T, decltype(args)...>
             && requires { static_cast<append_type<T>>(_impl::template _append<T>(std::move(*this), FWD(args)...)); }
  {
    return {_impl::template _append<T>(std::move(*this), FWD(args)...)};
  }

  template <typename T>
  [[nodiscard]] constexpr auto append(std::in_place_type_t<T>, auto &&...args) const && noexcept -> append_type<T>
    requires std::is_constructible_v<T, decltype(args)...>
             && requires { static_cast<append_type<T>>(_impl::template _append<T>(std::move(*this), FWD(args)...)); }
  {
    return {_impl::template _append<T>(std::move(*this), FWD(args)...)};
  }

  template <typename Arg>
  [[nodiscard]] constexpr auto append(Arg &&arg) & noexcept -> append_type<Arg>
    requires requires { static_cast<append_type<Arg>>(_impl::template _append<Arg>(*this, FWD(arg))); }
  {
    return {_impl::template _append<Arg>(*this, FWD(arg))};
  }

  template <typename Arg>
  [[nodiscard]] constexpr auto append(Arg &&arg) const & noexcept -> append_type<Arg>
    requires requires { static_cast<append_type<Arg>>(_impl::template _append<Arg>(*this, FWD(arg))); }
  {
    return {_impl::template _append<Arg>(*this, FWD(arg))};
  }

  template <typename Arg>
  [[nodiscard]] constexpr auto append(Arg &&arg) && noexcept -> append_type<Arg>
    requires requires { static_cast<append_type<Arg>>(_impl::template _append<Arg>(std::move(*this), FWD(arg))); }
  {
    return {_impl::template _append<Arg>(std::move(*this), FWD(arg))};
  }

  template <typename Arg>
  [[nodiscard]] constexpr auto append(Arg &&arg) const && noexcept -> append_type<Arg>
    requires requires { static_cast<append_type<Arg>>(_impl::template _append<Arg>(std::move(*this), FWD(arg))); }
  {
    return {_impl::template _append<Arg>(std::move(*this), FWD(arg))};
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn, auto &&...args) & noexcept -> decltype(auto)
    requires requires { _impl::_invoke(*this, FWD(fn), FWD(args)...); }
  {
    return _impl::_invoke(*this, FWD(fn), FWD(args)...);
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn, auto &&...args) const & noexcept -> decltype(auto)
    requires requires { _impl::_invoke(*this, FWD(fn), FWD(args)...); }
  {
    return _impl::_invoke(*this, FWD(fn), FWD(args)...);
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn, auto &&...args) && noexcept -> decltype(auto)
    requires requires { _impl::_invoke(std::move(*this), FWD(fn), FWD(args)...); }
  {
    return _impl::_invoke(std::move(*this), FWD(fn), FWD(args)...);
  }

  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn, auto &&...args) const && noexcept -> decltype(auto)
    requires requires { _impl::_invoke(std::move(*this), FWD(fn), FWD(args)...); }
  {
    return _impl::_invoke(std::move(*this), FWD(fn), FWD(args)...);
  }

  template <typename Ret, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn, auto &&...args) & noexcept -> Ret
    requires requires { _impl::_invoke(*this, FWD(fn), FWD(args)...); }
             && std::is_convertible_v<decltype(_impl::_invoke(*this, FWD(fn), FWD(args)...)), Ret>
  {
    return _impl::_invoke(*this, FWD(fn), FWD(args)...);
  }

  template <typename Ret, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn, auto &&...args) const & noexcept -> Ret
    requires requires { _impl::_invoke(*this, FWD(fn), FWD(args)...); }
             && std::is_convertible_v<decltype(_impl::_invoke(*this, FWD(fn), FWD(args)...)), Ret>
  {
    return _impl::_invoke(*this, FWD(fn), FWD(args)...);
  }

  template <typename Ret, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn, auto &&...args) && noexcept -> Ret
    requires requires { _impl::_invoke(std::move(*this), FWD(fn), FWD(args)...); }
             && std::is_convertible_v<decltype(_impl::_invoke(std::move(*this), FWD(fn), FWD(args)...)), Ret>
  {
    return _impl::_invoke(std::move(*this), FWD(fn), FWD(args)...);
  }

  template <typename Ret, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn, auto &&...args) const && noexcept -> Ret
    requires requires { _impl::_invoke(std::move(*this), FWD(fn), FWD(args)...); }
             && std::is_convertible_v<decltype(_impl::_invoke(std::move(*this), FWD(fn), FWD(args)...)), Ret>
  {
    return _impl::_invoke(std::move(*this), FWD(fn), FWD(args)...);
  }
};

template <typename... Args> pack(Args &&...args) -> pack<Args...>;

// Lifts
[[nodiscard]] constexpr auto as_pack() -> pack<> { return {}; }
template <typename T, typename... Args>
  requires(not some_in_place_type<T>)
[[nodiscard]] constexpr auto as_pack(T &&src, Args &&...args) -> decltype(auto)
{
  return pack<T, Args...>{FWD(src), FWD(args)...};
}

namespace detail {
namespace _join_fold {
template <typename Lh, typename Rh>
  requires some_sum<Lh> && some_sum<Rh>
[[nodiscard]] constexpr auto fold(auto &&lv, auto &&rv)
{
  return FWD(lv)._transform([&rv]<typename L>(std::in_place_type_t<L>, auto &&l) {
    return FWD(rv)._transform([&l]<typename R>(std::in_place_type_t<R>, auto &&r) {
      static_assert(not some_pack<R>, "Not implemented");
      if constexpr (some_pack<L>) {
        using value_type = typename L::template append_type<R>;
        return value_type(FWD(l).append(std::in_place_type_t<R>{}, FWD(r)));
      } else {
        using value_type = ::fn::pack<L, R>;
        return value_type{FWD(l), FWD(r)};
      }
    });
  });
}

template <typename Lh, typename Rh>
  requires some_sum<Lh> && (not some_sum<Rh>)
[[nodiscard]] constexpr auto fold(auto &&lv, auto &&rv)
{
  return FWD(lv)._transform([&rv]<typename L>(std::in_place_type_t<L>, auto &&l) {
    if constexpr (some_pack<L>) {
      using value_type = typename L::template append_type<Rh>;
      return value_type(FWD(l).append(std::in_place_type_t<Rh>{}, FWD(rv)));
    } else {
      using value_type = ::fn::pack<L, Rh>;
      return value_type{FWD(l), FWD(rv)};
    }
  });
}

template <typename Lh, typename Rh>
  requires(not some_sum<Lh>) && some_sum<Rh>
[[nodiscard]] constexpr auto fold(auto &&lv, auto &&rv)
{
  return FWD(rv)._transform([&lv]<typename R>(std::in_place_type_t<R>, auto &&r) {
    static_assert(not some_pack<R>, "Not implemented");
    if constexpr (some_pack<Lh>) {
      using value_type = typename Lh::template append_type<R>;
      return value_type(FWD(lv).append(std::in_place_type_t<R>{}, FWD(r)));
    } else {
      using value_type = ::fn::pack<Lh, R>;
      return value_type{FWD(lv), FWD(r)};
    }
  });
}

template <typename Lh, typename Rh>
  requires(not some_sum<Lh>) && (not some_sum<Rh>)
[[nodiscard]] constexpr auto fold(auto &&lv, auto &&rv)
{
  if constexpr (some_pack<Lh>) {
    using value_type = typename Lh::template append_type<Rh>;
    return value_type(FWD(lv).append(std::in_place_type_t<Rh>{}, FWD(rv)));
  } else {
    using value_type = ::fn::pack<Lh, Rh>;
    return value_type{FWD(lv), FWD(rv)};
  }
}
} // namespace _join_fold

template <template <typename> typename Tpl> [[nodiscard]] constexpr auto _join(auto &&lh, auto &&rh, auto &&efn)
{
  using Lh = std::remove_cvref_t<decltype(lh)>::value_type;
  using Rh = std::remove_cvref_t<decltype(rh)>::value_type;
  using value_type = decltype(::fn::detail::_join_fold::fold<Lh, Rh>(FWD(lh).value(), FWD(rh).value()));
  using type = Tpl<value_type>;
  if (lh.has_value() && rh.has_value())
    return type{std::in_place, ::fn::detail::_join_fold::fold<Lh, Rh>(FWD(lh).value(), FWD(rh).value())};
  else if (not lh.has_value())
    return type{efn(FWD(lh))};
  else
    return type{efn(FWD(rh))};
}

} // namespace detail

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_PACK
