# Pull or find Catch2 dependency for unit tests

if((DEFINED ENV{CMAKE_FIND_CATCH2}) OR DISABLE_FETCH_CONTENT)
    find_package(Catch2 3)
else()
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_SHALLOW    TRUE
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.7.1)
    FetchContent_MakeAvailable(Catch2)

    list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
endif()
