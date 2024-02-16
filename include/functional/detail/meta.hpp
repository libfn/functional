// Copyright (c) 2024 Gašper Ažman, Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_DETAIL_META
#define INCLUDE_FUNCTIONAL_DETAIL_META

#include <algorithm>
#include <array>
#include <concepts>
#include <cstdint>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace fn::detail {

// TODO Remove `select_nth` when our compilers start supporting pack indexing https://wg21.link/P2662
template <std::size_t, typename...> struct select_nth;
template <std::size_t N> struct select_nth<N>; // Intentionally incomplete type

template <std::size_t N, typename T1, typename... Ts> struct select_nth<N, T1, Ts...> {
  static_assert(N < (1 + sizeof...(Ts)));
  using type = std::tuple_element_t<N, std::tuple<T1, Ts...>>;
};

template <std::size_t N, typename... Ts> using select_nth_t = select_nth<N, Ts...>::type;

// NOTE Reverse to above, i.e. mapping of a type to position in a list of types.
template <typename T> struct _indexed_type {
  std::size_t index;
};
template <typename... Ts> struct _indexed_type_list : _indexed_type<Ts>... {
  constexpr _indexed_type_list(std::size_t i = 0) : _indexed_type<Ts>{i++}... {}
};
template <typename T, typename... Ts>
  requires(... || std::same_as<Ts, T>)
constexpr inline std::size_t type_index = static_cast<_indexed_type<T> const &>(_indexed_type_list<Ts...>()).index;

template <typename T, typename... Ts> constexpr inline bool type_one_of = (... || std::same_as<Ts, T>);

template <auto TU_name, auto Input> struct _normalized_name final {
  static constexpr std::size_t bound = 4096;
  static constexpr std::size_t TU_name_bound = 30;
  struct _slice_t final {
    std::array<char, bound> data = {};
    std::size_t end = 0;
  };

  static constexpr auto apply() noexcept -> _slice_t
  {
    std::string_view const sv{Input.data(), Input.size()};
#ifdef __clang__
    static constexpr std::string_view anon{"(anonymous namespace)"};
    static constexpr std::string_view prefix{"sortkey() [T = "};
#else
#ifdef __GNUC__
    static constexpr std::string_view anon{"{anonymous}"};
    static constexpr std::string_view prefix{"sortkey() [with T = "};
#endif
#endif
    _slice_t result = {};
    constexpr auto append = [](_slice_t &to, std::string_view from) {
      for (char c : from) {
        if (to.end == bound) {
          return;
        }
        to.data[to.end++] = c;
      }
    };

    std::size_t s = sv.find(prefix);
    if (s == std::string_view::npos || sv[sv.size() - 2] != ']' || sv[sv.size() - 1] != '\0') {
      std::unreachable();
    }
    if (TU_name.size() < 1 || TU_name[TU_name.size() - 1] != '\0') {
      std::unreachable();
    }
    std::string_view file{TU_name.size() <= TU_name_bound ? TU_name.data()
                                                          : TU_name.data() + (TU_name.size() - TU_name_bound - 1),
                          std::min(TU_name.size() - 1, TU_name_bound)};

    s += prefix.size();
    while (true) {
      std::size_t i = sv.find(anon, s);
      if (i != std::string_view::npos) {
        append(result, sv.substr(s, i - s));
        append(result, std::string_view("(anonymous namespace in "));
        append(result, file);
        append(result, std::string_view(")"));
      } else {
        append(result, sv.substr(s, sv.size() - s - 2));
        break;
      }
      s = i + anon.size();
    };

    return result;
  }

  static constexpr auto _slice = apply();
  static constexpr std::string_view value{_slice.data.data(), _slice.end};
};

template <typename T> [[nodiscard]] static constexpr auto _make_sortkey()
{
  return _normalized_name<std::to_array(__BASE_FILE__), std::to_array(__PRETTY_FUNCTION__)>::value;
}
template <typename T> constexpr inline std::string_view type_sortkey_v = _make_sortkey<T>();

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

template <typename... Ts> struct is_normal final {
  template <typename... Tx> struct _ts final {};
  static constexpr auto value = std::is_same<typename normalized<Ts...>::template apply<_ts>, _ts<Ts...>>::value;
};

template <typename... Ts> static constexpr bool is_normal_v = is_normal<Ts...>::value;

} // namespace fn::detail

#endif // INCLUDE_FUNCTIONAL_DETAIL_META
