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
