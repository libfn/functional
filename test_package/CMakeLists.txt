cmake_minimum_required(VERSION 3.25)
project(libfn_test_package LANGUAGES CXX)

find_package(libfn CONFIG REQUIRED)

# pfn is always exposed (C++20-compatible polyfills).
add_executable(pfn_quine src/pfn_quine.cpp)
target_link_libraries(pfn_quine PRIVATE libfn::pfn)
target_compile_features(pfn_quine PRIVATE cxx_std_20)

# fn is only exposed when the recipe was built without disable_cxx23.
if(NOT TEST_DISABLE_CXX23)
    add_executable(fn_quine src/fn_quine.cpp)
    target_link_libraries(fn_quine PRIVATE libfn::fn)
    target_compile_features(fn_quine PRIVATE cxx_std_23)
endif()
