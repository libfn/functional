// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <util/refusing.hpp>

#include <fn/expected.hpp>

#include <utility>

// A guard named <subject>_to_<fallback> holds a call on a <subject> expected whose callback refuses
// the value or error in that category: the call must not compile, rather than run the <fallback>
// overload.

namespace {
struct Error {};

// Copyable but not movable: the rvalue overloads, constrained on constructing the unchanged side
// from an rvalue, drop out as in the draft, and const && takes the call
struct CopyOnly {
  CopyOnly() = default;
  CopyOnly(CopyOnly const &) = default;
  CopyOnly(CopyOnly &&) = delete;
};
} // namespace

void transform()
{
  fn::expected<int, Error> o{1};
  fn::expected<int, Error> const c{1};

  (void)o.transform(refusing<int>{});
#ifdef LIBFN_REJECT_transform_lvalue_to_clvalue
  (void)o.transform(refusing<int, int &>{});
#endif

  (void)std::move(o).transform(refusing<int>{});
#ifdef LIBFN_REJECT_transform_rvalue_to_crvalue
  (void)std::move(o).transform(refusing<int, int &&>{});
#endif
#ifdef LIBFN_REJECT_transform_rvalue_to_clvalue
  (void)std::move(o).transform(refusing<int, int &&, int const &&>{});
#endif

  (void)std::move(c).transform(refusing<int>{});
#ifdef LIBFN_REJECT_transform_crvalue_to_clvalue
  (void)std::move(c).transform(refusing<int, int const &&>{});
#endif

  fn::expected<int, CopyOnly> m{1};
  (void)std::move(m).transform(refusing<int, int &&>{});
#ifdef LIBFN_REJECT_transform_copy_only_error_to_clvalue
  (void)std::move(m).transform(refusing<int, int &&, int const &&>{});
#endif
}

void transform_copack()
{
  fn::expected<fn::copack<int>, Error> o{fn::copack<int>{1}};

  (void)std::move(o).transform(refusing<int>{});
#ifdef LIBFN_REJECT_transform_copack_rvalue_to_clvalue
  (void)std::move(o).transform(refusing<int, int &&, int const &&>{});
#endif
}

void transform_error()
{
  fn::expected<int, Error> o{fn::unexpect};
  fn::expected<int, Error> const c{fn::unexpect};

  (void)o.transform_error(refusing<int>{});
#ifdef LIBFN_REJECT_transform_error_lvalue_to_clvalue
  (void)o.transform_error(refusing<int, Error &>{});
#endif

  (void)std::move(o).transform_error(refusing<int>{});
#ifdef LIBFN_REJECT_transform_error_rvalue_to_crvalue
  (void)std::move(o).transform_error(refusing<int, Error &&>{});
#endif
#ifdef LIBFN_REJECT_transform_error_rvalue_to_clvalue
  (void)std::move(o).transform_error(refusing<int, Error &&, Error const &&>{});
#endif

  (void)std::move(c).transform_error(refusing<int>{});
#ifdef LIBFN_REJECT_transform_error_crvalue_to_clvalue
  (void)std::move(c).transform_error(refusing<int, Error const &&>{});
#endif
}

void transform_error_copack()
{
  fn::expected<int, fn::copack<Error>> o{fn::unexpect, fn::copack<Error>{Error{}}};

  (void)std::move(o).transform_error(refusing<int>{});
#ifdef LIBFN_REJECT_transform_error_copack_rvalue_to_clvalue
  (void)std::move(o).transform_error(refusing<int, Error &&, Error const &&>{});
#endif
}

void void_transform_error()
{
  fn::expected<void, Error> o{fn::unexpect};

  (void)std::move(o).transform_error(refusing<int>{});
#ifdef LIBFN_REJECT_void_transform_error_rvalue_to_clvalue
  (void)std::move(o).transform_error(refusing<int, Error &&, Error const &&>{});
#endif
}

template <class S, class F>
concept can_transform = requires(S s, F f) { s.transform(f); };

// The member asserts what it requires: a non-invocable callback is a hard error even in a dependent
// requires-expression, and fn::applicable_transform answers instead.
void transform_not_invocable()
{
  static_assert(can_transform<fn::expected<int, Error>, refusing<int>>);
#ifdef LIBFN_REJECT_transform_not_invocable
  (void)can_transform<fn::expected<int, Error>, int>;
#endif
}
