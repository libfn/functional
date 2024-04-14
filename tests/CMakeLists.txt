cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(main)
add_subdirectory(tests)
add_subdirectory(examples)
add_subdirectory(util)
