// Copyright (c) 2024 Gašper Ažman, Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_DETAIL_META
#define INCLUDE_FUNCTIONAL_DETAIL_META

#include <algorithm>
#include <array>
#include <concepts>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace fn::detail {

// TODO Remove `select_nth` when our compilers start supporting pack indexing https://wg21.link/P2662
template <std::size_t, typename...> struct select_nth;
template <std::size_t N> struct select_nth<N>; // Intentionally incomplete type

#if __has_builtin(__type_pack_element)
template <std::size_t N, typename... Ts>
  requires(sizeof...(Ts) > 0)
struct select_nth<N, Ts...> {
  static_assert(N < (sizeof...(Ts)));
  using type = __type_pack_element<N, Ts...>;
};
#else
template <std::size_t N, typename... Ts>
  requires(sizeof...(Ts) > 0)
struct select_nth<N, Ts...> {
  static_assert(N < (sizeof...(Ts)));
  using type = std::tuple_element_t<N, std::tuple<Ts...>>;
};
#endif

template <std::size_t N, typename... Ts> using select_nth_t = select_nth<N, Ts...>::type;

// NOTE Reverse to above, i.e. mapping of a type to position in a list of types.
template <typename T> struct _indexed_type {
  std::size_t index;
};
template <typename... Ts> struct _indexed_type_list : _indexed_type<Ts>... {
  constexpr _indexed_type_list(std::size_t i = 0) : _indexed_type<Ts>{i++}... {}
};
template <typename T, typename... Ts>
  requires(... || __is_same_as(Ts, T))
constexpr inline std::size_t type_index = static_cast<_indexed_type<T> const &>(_indexed_type_list<Ts...>()).index;

template <typename T, typename... Ts> constexpr inline bool type_one_of = (... || __is_same_as(Ts, T));

#ifdef __clang__
static constexpr std::string_view _normalized_name_anon{"(anonymous namespace)"};
static constexpr std::string_view _normalized_name_prefix{"sortkey() [T = "};
#elifdef __GNUC__
static constexpr std::string_view _normalized_name_anon{"{anonymous}"};
static constexpr std::string_view _normalized_name_prefix{"sortkey() [with T = "};
#endif
static constexpr std::size_t _normalized_name_TU_name_bound = 30;

template <auto TU_name, auto Input> struct _normalized_name final {
  template <std::size_t N> static constexpr auto apply() noexcept
  {
    std::string_view const sv{Input.data(), Input.size()};
    std::size_t s = sv.find(_normalized_name_prefix);
    std::string_view file{TU_name.size() <= _normalized_name_TU_name_bound
                              ? TU_name.data()
                              : TU_name.data() + (TU_name.size() - _normalized_name_TU_name_bound - 1),
                          std::min(TU_name.size() - 1, _normalized_name_TU_name_bound)};

    std::string result;
    s += _normalized_name_prefix.size();
    while (true) {
      std::size_t i = sv.find(_normalized_name_anon, s);
      if (i != std::string_view::npos) {
        result.append(sv.substr(s, i - s));
        result.append(std::string_view("(anonymous namespace in "));
        result.append(file);
        result.append(std::string_view(")"));
      } else {
        result.append(sv.substr(s, sv.size() - s - 2));
        break;
      }
      s = i + _normalized_name_anon.size();
    };

    if constexpr (N == 0)
      return result.size();
    else {
      std::array<char, N> a;
      for (std::size_t i = 0; i < result.size(); ++i) {
        a[i] = result[i];
      }
      return a;
    }
  }

  static constexpr auto _slice = apply<apply<0>()>();
  static constexpr std::string_view value{_slice.data(), _slice.size()};
};

namespace sortkey {
template <typename T> [[nodiscard]] static constexpr auto _make_sortkey()
{
  return _normalized_name<std::to_array(__BASE_FILE__), std::to_array(__PRETTY_FUNCTION__)>::value;
}
} // namespace sortkey
template <typename T> constexpr inline std::string_view type_sortkey_v = sortkey::_make_sortkey<T>();

// NOTE Normalized order of types - order based on type_sortkey_v
template <typename... Ts> struct normalized final {
  static constexpr std::size_t N = sizeof...(Ts);

  struct _uniqued final {
    std::array<std::size_t, N> indices;
    std::size_t size;
  };

  [[nodiscard]] static constexpr auto _indices() noexcept
  {
    std::array<std::string_view, sizeof...(Ts)> names{type_sortkey_v<Ts>...};
    std::array<std::size_t, sizeof...(Ts)> indices;
    std::generate(indices.begin(), indices.end(), [n = 0]() mutable -> std::size_t { return n++; });
    auto const less = [v = &names](std::size_t i, std::size_t j) constexpr { return (*v)[i] < (*v)[j]; };
    std::sort(indices.begin(), indices.end(), less);
    auto const equal = [v = &names](std::size_t i, std::size_t j) constexpr { return (*v)[i] == (*v)[j]; };
    auto const end = std::unique(indices.begin(), indices.end(), equal);
    return _uniqued{indices, static_cast<std::size_t>(end - indices.begin())};
  }

  static constexpr _uniqued _indices_v = _indices();

  template <template <typename...> typename F, std::size_t... Is>
  static constexpr auto _normalized_f(std::index_sequence<Is...> const &)
      -> F<select_nth_t<_indices_v.indices[Is], Ts...>...>;

  // How many unique types
  static constexpr std::size_t size = _indices_v.size;

  // Apply a given template on a normalized list of types
  template <template <typename...> typename F>
  using apply = decltype(_normalized_f<F>(std::make_index_sequence<size>()));
};

template <typename... T> static constexpr bool is_superset_of = false;
template <template <typename...> typename F, typename... Ts, typename... Tu>
static constexpr bool is_superset_of<F<Ts...>, F<Tu...>> = (... && type_one_of<Tu, Ts...>);

template <typename... Tx> struct _ts final {};

template <typename... Ts> struct is_normal final {
  static constexpr auto value = std::is_same<typename normalized<Ts...>::template apply<_ts>, _ts<Ts...>>::value;
};

template <typename... Ts> static constexpr bool is_normal_v = is_normal<Ts...>::value;

} // namespace fn::detail

#endif // INCLUDE_FUNCTIONAL_DETAIL_META
