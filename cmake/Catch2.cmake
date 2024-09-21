if(DISABLE_FETCH_CONTENT)
  find_package(Catch2 3)
else()  
  include(FetchContent)
  FetchContent_Declare(
    Catch2
    GIT_SHALLOW    TRUE
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.4)
  FetchContent_MakeAvailable(Catch2)
  
  list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
endif()
