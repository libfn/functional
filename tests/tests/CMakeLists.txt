cmake_minimum_required(VERSION 3.25)
project(tests)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pls keep the filenames sorted
set(SOURCE_FILES
    dummy.cpp
)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} include catch2_main Catch2::Catch2)

add_test(
    NAME ${PROJECT_NAME}
    COMMAND ${PROJECT_NAME} -r console
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
