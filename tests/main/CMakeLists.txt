cmake_minimum_required(VERSION 3.25)
project(catch2_main)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CATCH2_MAIN_SOURCE_FILES main.cpp)
add_library(${PROJECT_NAME} ${CATCH2_MAIN_SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} PUBLIC Catch2::Catch2)
