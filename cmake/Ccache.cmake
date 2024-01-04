# Simple ccache detection

if((DEFINED ENV{DISABLE_CCACHE_DETECTION}) OR (DEFINED DISABLE_CCACHE_DETECTION))
  message(VERBOSE "Suppressing ccache detection: ${DISABLE_CCACHE_DETECTION}")
elseif(DEFINED CMAKE_CXX_COMPILER_LAUNCHER)
  message(VERBOSE "Explicitly set compiler launcher: ${CMAKE_CXX_COMPILER_LAUNCHER}")
else()
  find_program(CCACHE_FOUND "ccache")
  if(CCACHE_FOUND)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_FOUND}")
    message(VERBOSE "Found ccache at: ${CCACHE_FOUND}")
  endif(CCACHE_FOUND)
endif()
