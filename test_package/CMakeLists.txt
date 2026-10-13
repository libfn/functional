cmake_minimum_required(VERSION 3.25)
project(libfn_test_package LANGUAGES CXX)

find_package(libfn CONFIG REQUIRED)

add_executable(main src/main.cpp)
target_link_libraries(main PRIVATE libfn::fn libfn::pfn)
target_compile_features(main PRIVATE cxx_std_20)

# fn::pack initialization elides braces by design; warnings-as-errors asserts the
# package carries the compile options consumers need (cmake/CompilationOptions.cmake).
target_compile_options(main PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Werror>
    $<$<CXX_COMPILER_ID:MSVC>:/W4;/WX>)

# Off by default: the C++26 mode needs std::type_order, so the workflows select the
# lanes that build it — this must never become a compiler-capability probe.
option(LIBFN_TEST_CXX26 "Build a second executable consuming libfn::fn_cxx26" OFF)
if(LIBFN_TEST_CXX26)
    add_executable(main_cxx26 src/main.cpp)
    # Let the CMake target supply C++26; Conan supplies it through the profile.
    target_link_libraries(main_cxx26 PRIVATE libfn::fn_cxx26)
    target_compile_options(main_cxx26 PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Werror>
        $<$<CXX_COMPILER_ID:MSVC>:/W4;/WX>)
endif()

# Conan's libfn::libfn aggregate links every component. Check that its
# LIBFN_CXX26 definition matches the package's cxx26 option.
option(LIBFN_TEST_AGGREGATE "Compile a mode check against the libfn::libfn aggregate" OFF)
if(LIBFN_TEST_AGGREGATE)
    add_library(aggregate OBJECT src/aggregate.cpp)
    target_link_libraries(aggregate PRIVATE libfn::libfn)
    target_compile_definitions(aggregate PRIVATE LIBFN_TEST_EXPECT_CXX26=$<BOOL:${LIBFN_TEST_CXX26}>)
endif()
