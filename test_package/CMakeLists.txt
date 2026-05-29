cmake_minimum_required(VERSION 3.25)
project(libfn_test_package LANGUAGES CXX)

find_package(libfn CONFIG REQUIRED)

# pfn is always exposed (C++20-compatible polyfills).
add_executable(pfn_quine src/pfn_quine.cpp)
target_link_libraries(pfn_quine PRIVATE libfn::pfn)
target_compile_features(pfn_quine PRIVATE cxx_std_20)

# fn_quine needs libfn::fn, not provided when libfn is built with disable_cxx23.
if(TARGET libfn::fn)
    add_executable(fn_quine src/fn_quine.cpp)
    target_link_libraries(fn_quine PRIVATE libfn::fn)
    target_compile_features(fn_quine PRIVATE cxx_std_23)
endif()
