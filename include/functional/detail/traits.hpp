#ifndef INCLUDE_FUNCTIONAL_DETAIL_TRAITS
#define INCLUDE_FUNCTIONAL_DETAIL_TRAITS

#include <optional>
#include <type_traits>

namespace fn::detail {

// Change any rvalue or empty value to prvalue, but leave lvalues unchanged.
// This is meant to find the type of data members which won't bind to rvalues.
template <typename T> extern T _as_value;

template <typename T> extern T _as_value<T &&>;
template <typename T>
  requires(std::is_empty_v<T>)
extern T _as_value<T &>;
template <typename T>
  requires(!std::is_empty_v<T>)
extern T &_as_value<T &>;

template <typename T> extern T const _as_value<T const &&>;
template <typename T>
  requires(std::is_empty_v<T>)
extern T const _as_value<T const &>;
template <typename T>
  requires(!std::is_empty_v<T>)
extern T const &_as_value<T const &>;

// Add const to second type, if first type is const
template <typename T, typename V> extern V _apply_const;
template <typename T, typename V> extern V const _apply_const<T const &, V>;
template <typename T, typename V> extern V const &_apply_const<T const &, V &>;
template <typename T, typename V>
extern V const &&_apply_const<T const &, V &&>;

template <typename T, typename V>
using apply_const_t = decltype(detail::_apply_const<T &, V>);

} // namespace fn::detail

#endif // INCLUDE_FUNCTIONAL_DETAIL_TRAITS
