// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_FUNCTOR
#define INCLUDE_FUNCTIONAL_FUNCTOR

#include <tuple>
#include <utility>

namespace fn {

template <typename Functor, typename... Args> struct functor final {
  using functor_type = Functor;
  constexpr static unsigned size = sizeof...(Args);
  std::tuple<Args...> args;

  friend auto operator|(auto &&v, functor const &self) noexcept -> auto
    requires requires {
      monadic_apply(std::forward<decltype(v)>(v), functor_type{}, self.args);
    }
  {
    return monadic_apply(std::forward<decltype(v)>(v), functor_type{},
                         self.args);
  }

  friend auto operator|(auto &&v, functor &&self) noexcept -> auto
    requires requires {
      monadic_apply(std::forward<decltype(v)>(v), functor_type{},
                    std::move(self.args));
    }
  {
    return monadic_apply(std::forward<decltype(v)>(v), functor_type{},
                         std::move(self.args));
  }
};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_FUNCTOR
