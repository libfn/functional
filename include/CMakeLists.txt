cmake_minimum_required(VERSION 3.25)
project(include)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pls keep the filenames sorted
set(SOURCE_FILES
    functional/and_then.hpp
    functional/fwd.hpp
    functional/operator.hpp
    functional/or_else.hpp
    functional/recover.hpp
    functional/transform_error.hpp
    functional/transform.hpp
)

add_library(${PROJECT_NAME} INTERFACE ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
