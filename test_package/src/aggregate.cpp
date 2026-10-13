#include <libfn_version.hpp>

#if defined(LIBFN_CXX26) != LIBFN_TEST_EXPECT_CXX26
#error "libfn::libfn carries a mode other than the one selected"
#endif
