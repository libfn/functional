// Copyright (c) 2024 Gašper Ažman, Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_DETAIL_META
#define INCLUDE_FN_DETAIL_META

#include <algorithm>
#include <array>
#include <libfn_version.hpp>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#ifdef LIBFN_CXX26
#include <compare>
#ifndef __cpp_lib_type_order
#error "LIBFN_CXX26 requires std::type_order (C++26, feature-test macro __cpp_lib_type_order); see CONTRIBUTING.md"
#endif
#endif

namespace fn::inline LIBFN_VERSION::detail {

// TODO Remove `select_nth` when our compilers start supporting pack indexing https://wg21.link/P2662
template <::std::size_t, typename...> struct select_nth;
template <::std::size_t N> struct select_nth<N>; // Intentionally incomplete type

// MSVC has no __type_pack_element and its preprocessor rejects __has_builtin, so gate _MSC_VER first.
#if defined(_MSC_VER)
#define FN_DETAIL_HAS_TYPE_PACK_ELEMENT 0
#elif __has_builtin(__type_pack_element)
#define FN_DETAIL_HAS_TYPE_PACK_ELEMENT 1
#else
#define FN_DETAIL_HAS_TYPE_PACK_ELEMENT 0
#endif
#if FN_DETAIL_HAS_TYPE_PACK_ELEMENT
template <::std::size_t N, typename... Ts>
  requires(sizeof...(Ts) > 0)
struct select_nth<N, Ts...> {
  static_assert(N < (sizeof...(Ts)));
  using type = __type_pack_element<N, Ts...>;
};
#else
template <::std::size_t N, typename... Ts>
  requires(sizeof...(Ts) > 0)
struct select_nth<N, Ts...> {
  static_assert(N < (sizeof...(Ts)));
  using type = ::std::tuple_element_t<N, ::std::tuple<Ts...>>;
};
#endif
#undef FN_DETAIL_HAS_TYPE_PACK_ELEMENT

template <::std::size_t N, typename... Ts> using select_nth_t = select_nth<N, Ts...>::type;

// NOTE Reverse to above, i.e. mapping of a type to position in a list of types.
template <typename T> struct _indexed_type {
  ::std::size_t index;
};
template <typename... Ts> struct _indexed_type_list : _indexed_type<Ts>... {
  constexpr explicit _indexed_type_list(::std::size_t i = 0) : _indexed_type<Ts>{i++}... {}
};
template <typename T, typename... Ts>
  requires(... || ::std::is_same_v<Ts, T>)
constexpr inline ::std::size_t type_index = static_cast<_indexed_type<T> const &>(_indexed_type_list<Ts...>()).index;

template <typename T, typename... Ts> constexpr inline bool type_one_of = (... || ::std::is_same_v<Ts, T>);

#if defined(__clang__) || defined(__GNUC__)

#ifdef __clang__
static constexpr ::std::string_view _normalized_name_anon{"(anonymous namespace)"};
static constexpr ::std::string_view _normalized_name_prefix{"sortkey() [T = "};
#else
static constexpr ::std::string_view _normalized_name_anon{"{anonymous}"};
static constexpr ::std::string_view _normalized_name_prefix{"sortkey() [with T = "};
#endif
static constexpr ::std::size_t _normalized_name_TU_name_bound = 30;

template <auto TU_name, auto Input> struct _normalized_name final {
  template <::std::size_t N> static constexpr auto apply() noexcept
  {
    ::std::string_view const sv{Input.data(), Input.size()};
    ::std::size_t s = sv.find(_normalized_name_prefix);
    ::std::string_view file{TU_name.size() <= _normalized_name_TU_name_bound
                                ? TU_name.data()
                                : TU_name.data() + (TU_name.size() - _normalized_name_TU_name_bound - 1),
                            ::std::min(TU_name.size() - 1, _normalized_name_TU_name_bound)};

    ::std::string result;
    s += _normalized_name_prefix.size();
    while (true) {
      ::std::size_t i = sv.find(_normalized_name_anon, s);
      if (i != ::std::string_view::npos) {
        result.append(sv.substr(s, i - s));
        result.append(::std::string_view("(anonymous namespace in "));
        result.append(file);
        result.append(::std::string_view(")"));
      } else {
        result.append(sv.substr(s, sv.size() - s - 2));
        break;
      }
      s = i + _normalized_name_anon.size();
    };

    if constexpr (N == 0)
      return result.size();
    else {
      ::std::array<char, N> a;
      for (::std::size_t i = 0; i < result.size(); ++i) {
        a[i] = result[i];
      }
      return a;
    }
  }

  static constexpr auto _slice = apply<apply<0>()>();
  static constexpr ::std::string_view value{_slice.data(), _slice.size()};
};

namespace sortkey {
template <typename T> [[nodiscard]] static constexpr auto _make_sortkey()
{
  return _normalized_name<::std::to_array(__BASE_FILE__), ::std::to_array(__PRETTY_FUNCTION__)>::value;
}
} // namespace sortkey

#elif defined(_MSC_VER)

namespace sortkey {
// MSVC __FUNCSIG__ is "auto __cdecl fn::detail::sortkey::_make_sortkey<TYPE>(void)"; the type is the
// balanced substring between "_make_sortkey<" and the trailing ">(void)". Only used as a stable per-type
// ordering key (no anon/TU rewriting like the GCC/Clang path), so a view into __FUNCSIG__ suffices.
template <typename T> [[nodiscard]] static constexpr auto _make_sortkey() -> ::std::string_view
{
  ::std::string_view const sv{__FUNCSIG__};
  constexpr ::std::string_view prefix{"_make_sortkey<"};
  ::std::size_t const b = sv.find(prefix) + prefix.size();
  ::std::size_t const e = sv.rfind(">(void)");
  return sv.substr(b, e - b);
}
} // namespace sortkey

#else
#error "fn/detail/meta.hpp: type ordering needs __PRETTY_FUNCTION__ (GCC/Clang) or __FUNCSIG__ (MSVC)"
#endif

template <typename T> constexpr inline ::std::string_view type_sortkey_v = sortkey::_make_sortkey<T>();

// How many distinct types (by identity, not by key): a type counts unless it recurs later.
template <typename... Ts> constexpr inline ::std::size_t _distinct_types = 0;
template <typename T, typename... Ts>
constexpr inline ::std::size_t _distinct_types<T, Ts...> = _distinct_types<Ts...> + (type_one_of<T, Ts...> ? 0 : 1);

#ifdef LIBFN_CXX26
// T's rank within Ts: how many pack members order strictly below it under std::type_order.
// Ranks of distinct types differ (the order is strong and total), identical types share theirs —
// an ordering key injective by identity, where the sortkey scrape is injective only by spelling.
template <typename T, typename... Ts>
constexpr inline ::std::size_t _type_order_rank
    = (::std::size_t{0} + ... + (::std::type_order_v<Ts, T> == ::std::strong_ordering::less ? 1 : 0));
#endif

// NOTE Normalized order of types - order based on type_sortkey_v, or std::type_order in the
// LIBFN_CXX26 mode (the orders may differ, which is why the mode is a distinct ABI namespace)
template <typename... Ts> struct normalized final {
  static constexpr ::std::size_t N = sizeof...(Ts);

  struct _uniqued final {
    ::std::array<::std::size_t, N> indices;
    ::std::size_t count;
  };

  [[nodiscard]] static constexpr auto _indices() noexcept
  {
#ifdef LIBFN_CXX26
    ::std::array<::std::size_t, sizeof...(Ts)> keys{_type_order_rank<Ts, Ts...>...};
#else
    ::std::array<::std::string_view, sizeof...(Ts)> keys{type_sortkey_v<Ts>...};
#endif
    ::std::array<::std::size_t, sizeof...(Ts)> indices{};
    ::std::ranges::generate(indices, [n = 0]() mutable -> ::std::size_t { return n++; });
    auto const less = [v = &keys](::std::size_t i, ::std::size_t j) constexpr { return (*v)[i] < (*v)[j]; };
    ::std::ranges::sort(indices, less);
    auto const equal = [v = &keys](::std::size_t i, ::std::size_t j) constexpr { return (*v)[i] == (*v)[j]; };
    auto const end = ::std::ranges::unique(indices, equal).begin();
    return _uniqued{indices, static_cast<::std::size_t>(end - indices.begin())};
  }

  static constexpr _uniqued _indices_v = _indices();

  // The collision floor (#326): dedup and canonical order both rest on key injectivity, so
  // count mismatch is exactly two distinct types sharing one. Known colliders: same-scope lambdas
  // (no positional disambiguator in gcc), same-named function-local types (clang prints no scope).
  // If this assert fires, the user project should rename one of the colliding types to a unique
  // name (or move it to a different scope); or enable the LIBFN_CXX26 mode, whose rank keys
  // cannot collide
  static_assert(_distinct_types<Ts...> == _indices_v.count, "distinct types must not share a sort key");

  template <template <typename...> typename F, ::std::size_t... Is>
  static constexpr auto _normalized_f(::std::index_sequence<Is...> const &)
      -> F<select_nth_t<_indices_v.indices[Is], Ts...>...>;

  // How many unique types
  static constexpr ::std::size_t size = _indices_v.count;

  // Apply a given template on a normalized list of types
  template <template <typename...> typename F>
  using apply = decltype(_normalized_f<F>(::std::make_index_sequence<size>()));
};

template <typename... T> static constexpr bool is_superset_of = false;
template <template <typename...> typename F, typename... Ts, typename... Tu>
static constexpr bool is_superset_of<F<Ts...>, F<Tu...>> = (... && type_one_of<Tu, Ts...>);

template <typename... Tx> struct _ts final {};

template <typename... Ts> struct is_normal final {
  static constexpr auto value = ::std::is_same_v<typename normalized<Ts...>::template apply<_ts>, _ts<Ts...>>;
};

template <typename... Ts> static constexpr bool is_normal_v = is_normal<Ts...>::value;

} // namespace fn::inline LIBFN_VERSION::detail

#endif // INCLUDE_FN_DETAIL_META
