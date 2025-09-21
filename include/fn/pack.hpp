// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_PACK
#define INCLUDE_FN_PACK

#include <fn/detail/pack_impl.hpp>
#include <fn/sum.hpp>

#include <type_traits>

namespace fn {

/**
 * @brief TODO
 *
 * @tparam T TODO
 */
template <typename T>
concept some_pack = detail::_some_pack<T>;

/**
 * @brief TODO
 *
 * Some more text here.
 *
 * @tparam Ts TODO
 */
template <typename... Ts> struct pack : detail::pack_impl<std::index_sequence_for<Ts...>, Ts...> {
  using _impl = detail::pack_impl<std::index_sequence_for<Ts...>, Ts...>;
  static_assert((... && (not some_sum<Ts>)));

  template <typename T> using append_type = _impl::template append_type<T>;

  /**
   * @brief appends a thing
   *
   * @tparam T TODO
   * @param args TODO
   * @return TODO
   */
  template <typename T>
  [[nodiscard]] constexpr auto append(std::in_place_type_t<T>, auto &&...args) & noexcept -> append_type<T>
    requires std::is_constructible_v<T, decltype(args)...>
             && requires { static_cast<append_type<T>>(_impl::template _append<T>(*this, FWD(args)...)); }
  {
    return {_impl::template _append<T>(*this, FWD(args)...)};
  }

  /**
   * @brief appends a thing
   *
   * @tparam T TODO
   * @param args TODO
   * @return TODO
   */
  template <typename T>
  [[nodiscard]] constexpr auto append(std::in_place_type_t<T>, auto &&...args) const & noexcept -> append_type<T>
    requires std::is_constructible_v<T, decltype(args)...>
             && requires { static_cast<append_type<T>>(_impl::template _append<T>(*this, FWD(args)...)); }
  {
    return {_impl::template _append<T>(*this, FWD(args)...)};
  }

  /**
   * @brief appends a thing
   *
   * @tparam T TODO
   * @param args TODO
   * @return TODO
   */
  template <typename T>
  [[nodiscard]] constexpr auto append(std::in_place_type_t<T>, auto &&...args) && noexcept -> append_type<T>
    requires std::is_constructible_v<T, decltype(args)...>
             && requires { static_cast<append_type<T>>(_impl::template _append<T>(std::move(*this), FWD(args)...)); }
  {
    return {_impl::template _append<T>(std::move(*this), FWD(args)...)};
  }

  /**
   * @brief appends a thing
   *
   * @tparam T TODO
   * @param args TODO
   * @return TODO
   */
  template <typename T>
  [[nodiscard]] constexpr auto append(std::in_place_type_t<T>, auto &&...args) const && noexcept -> append_type<T>
    requires std::is_constructible_v<T, decltype(args)...>
             && requires { static_cast<append_type<T>>(_impl::template _append<T>(std::move(*this), FWD(args)...)); }
  {
    return {_impl::template _append<T>(std::move(*this), FWD(args)...)};
  }

  /**
   * @brief appends a thing
   *
   * @tparam Arg TODO
   * @param arg TODO
   * @return TODO
   */
  template <typename Arg>
  [[nodiscard]] constexpr auto append(Arg &&arg) & noexcept -> append_type<Arg>
    requires requires { static_cast<append_type<Arg>>(_impl::template _append<Arg>(*this, FWD(arg))); }
  {
    return {_impl::template _append<Arg>(*this, FWD(arg))};
  }

  /**
   * @brief appends a thing
   *
   * @tparam Arg TODO
   * @param arg TODO
   * @return TODO
   */
  template <typename Arg>
  [[nodiscard]] constexpr auto append(Arg &&arg) const & noexcept -> append_type<Arg>
    requires requires { static_cast<append_type<Arg>>(_impl::template _append<Arg>(*this, FWD(arg))); }
  {
    return {_impl::template _append<Arg>(*this, FWD(arg))};
  }

  /**
   * @brief appends a thing
   *
   * @tparam Arg TODO
   * @param arg TODO
   * @return TODO
   */
  template <typename Arg>
  [[nodiscard]] constexpr auto append(Arg &&arg) && noexcept -> append_type<Arg>
    requires requires { static_cast<append_type<Arg>>(_impl::template _append<Arg>(std::move(*this), FWD(arg))); }
  {
    return {_impl::template _append<Arg>(std::move(*this), FWD(arg))};
  }

  /**
   * @brief appends a thing
   *
   * @tparam Arg TODO
   * @param arg TODO
   * @return TODO
   */
  template <typename Arg>
  [[nodiscard]] constexpr auto append(Arg &&arg) const && noexcept -> append_type<Arg>
    requires requires { static_cast<append_type<Arg>>(_impl::template _append<Arg>(std::move(*this), FWD(arg))); }
  {
    return {_impl::template _append<Arg>(std::move(*this), FWD(arg))};
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn, auto &&...args) & noexcept -> decltype(auto)
    requires requires { _impl::_invoke(*this, FWD(fn), FWD(args)...); }
  {
    return _impl::_invoke(*this, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn, auto &&...args) const & noexcept -> decltype(auto)
    requires requires { _impl::_invoke(*this, FWD(fn), FWD(args)...); }
  {
    return _impl::_invoke(*this, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn, auto &&...args) && noexcept -> decltype(auto)
    requires requires { _impl::_invoke(std::move(*this), FWD(fn), FWD(args)...); }
  {
    return _impl::_invoke(std::move(*this), FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Fn TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Fn>
  [[nodiscard]] constexpr auto invoke(Fn &&fn, auto &&...args) const && noexcept -> decltype(auto)
    requires requires { _impl::_invoke(std::move(*this), FWD(fn), FWD(args)...); }
  {
    return _impl::_invoke(std::move(*this), FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Ret TODO
   * @tparam Fn TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Ret, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn, auto &&...args) & noexcept -> Ret
    requires requires { _impl::_invoke(*this, FWD(fn), FWD(args)...); }
             && std::is_convertible_v<decltype(_impl::_invoke(*this, FWD(fn), FWD(args)...)), Ret>
  {
    return _impl::_invoke(*this, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Ret TODO
   * @tparam Fn TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Ret, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn, auto &&...args) const & noexcept -> Ret
    requires requires { _impl::_invoke(*this, FWD(fn), FWD(args)...); }
             && std::is_convertible_v<decltype(_impl::_invoke(*this, FWD(fn), FWD(args)...)), Ret>
  {
    return _impl::_invoke(*this, FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Ret TODO
   * @tparam Fn TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Ret, typename Fn>
  [[nodiscard]] constexpr auto invoke_r(Fn &&fn, auto &&...args) && noexcept -> Ret
    requires requires { _impl::_invoke(std::move(*this), FWD(fn), FWD(args)...); }
             && std::is_convertible_v<decltype(_impl::_invoke(std::move(*this), FWD(fn), FWD(args)...)), Ret>
  {
    return _impl::_invoke(std::move(*this), FWD(fn), FWD(args)...);
  }

  /**
   * @brief TODO
   *
   * @tparam Ret TODO
   * @tparam Fn TODO
   * @param fn TODO
   * @param args TODO
   * @return TODO
   */
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
/**
 * @brief TODO
 *
 * @tparam T TODO
 * @tparam Args TODO
 * @param src TODO
 * @param args TODO
 * @return TODO
 */
[[nodiscard]] constexpr auto as_pack() -> pack<> { return {}; }
template <typename T, typename... Args>
  requires(not some_in_place_type<T>)
[[nodiscard]] constexpr auto as_pack(T &&src, Args &&...args) -> decltype(auto)
{
  return pack<T, Args...>{FWD(src), FWD(args)...};
}

namespace detail {

template <template <typename> typename Tpl> [[nodiscard]] constexpr auto _join(auto &&lh, auto &&rh, auto &&efn)
{
  using Lh = std::remove_cvref_t<decltype(lh)>::value_type;
  using Rh = std::remove_cvref_t<decltype(rh)>::value_type;
  using value_type = decltype(::fn::detail::_fold_detail::fold<Lh, Rh>(FWD(lh).value(), FWD(rh).value()));
  using type = Tpl<value_type>;
  if (lh.has_value() && rh.has_value())
    return type{std::in_place, ::fn::detail::_fold_detail::fold<Lh, Rh>(FWD(lh).value(), FWD(rh).value())};
  else if (not lh.has_value())
    return type{efn(FWD(lh))};
  else
    return type{efn(FWD(rh))};
}

} // namespace detail

/**
 * @brief Data concantenation operator - creates a pack or a sum of packs
 *
 * @param lh TODO
 * @param rh TODO
 * @return TODO
 */
[[nodiscard]] constexpr auto operator&(auto &&lh, auto &&rh)
  requires(some_sum<decltype(lh)> || some_pack<decltype(lh)>)
{
  using Lh = std::remove_cvref_t<decltype(lh)>;
  using Rh = std::remove_cvref_t<decltype(rh)>;
  return ::fn::detail::_fold_detail::fold<Lh, Rh>(FWD(lh), FWD(rh));
}

/**
 * @brief Identity, which is basically lift for operator & above
 */
constexpr inline struct identity_t {
  /**
   * @brief TODO
   *
   * @tparam Arg TODO
   * @param arg TODO
   * @return TODO
   */
  template <typename Arg> [[nodiscard]] constexpr static auto operator()(Arg &&arg) -> decltype(arg)
  {
    return FWD(arg);
  }

  /**
   * @brief TODO
   *
   * @tparam Arg TODO
   * @tparam Args TODO
   * @param arg TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Arg, typename... Args>
    requires(not some_sum<Arg>) && (not some_pack<Arg>)
  [[nodiscard]] constexpr static auto operator()(Arg &&arg, Args &&...args)
  {
    return (::fn::pack{FWD(arg)} & ... & FWD(args));
  }

  /**
   * @brief TODO
   *
   * @tparam Arg TODO
   * @tparam Args TODO
   * @param arg TODO
   * @param args TODO
   * @return TODO
   */
  template <typename Arg, typename... Args>
    requires some_sum<Arg> || some_pack<Arg>
  [[nodiscard]] constexpr static auto operator()(Arg &&arg, Args &&...args)
  {
    return (FWD(arg) & ... & FWD(args));
  }
} identity;

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_PACK
