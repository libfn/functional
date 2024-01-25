cmake_minimum_required(VERSION 3.25)
project(util)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pls keep the filenames sorted
set(UTIL_SOURCE_FILES    
    static_check.hpp
)
add_library(${PROJECT_NAME} INTERFACE ${UTIL_SOURCE_FILES})
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} INTERFACE include)
