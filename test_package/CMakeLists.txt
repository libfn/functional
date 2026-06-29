cmake_minimum_required(VERSION 3.25)
project(libfn_test_package LANGUAGES CXX)

find_package(libfn CONFIG REQUIRED)

add_executable(main src/main.cpp)
target_link_libraries(main PRIVATE libfn::fn libfn::pfn)
target_compile_features(main PRIVATE cxx_std_20)
