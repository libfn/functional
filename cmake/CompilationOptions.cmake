# Add compilation options appropriate for the current compiler

function(append_compilation_options)
    set(options WARNINGS OPTIMIZATION)
    set(oneValueArgs NAME)
    cmake_parse_arguments(Options "${options}" "${oneValueArgs}" "" ${ARGN})

    if(NOT DEFINED Options_NAME)
      message(FATAL_ERROR "NAME must be set")
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
      if(Options_WARNINGS)
          target_compile_options(${Options_NAME} PRIVATE /W4)
      endif()

      if(Options_OPTIMIZATION)
          target_compile_options(${Options_NAME} PRIVATE $<IF:$<CONFIG:Debug>,/Od,/Ox>)
      endif()
    else()
      if(Options_WARNINGS)
          target_compile_options(${Options_NAME} PRIVATE -Wall -Wextra -Wpedantic)
      endif()

      if(Options_OPTIMIZATION)
          target_compile_options(${Options_NAME} PRIVATE $<IF:$<CONFIG:Debug>,-O0,-O2>)
      endif()
    endif()
endfunction()
