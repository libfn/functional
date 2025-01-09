find_program(ZNAI_FOUND NAMES znai)

if(ZNAI_FOUND)
    message(STATUS "Znai found at ${ZNAI_FOUND}")
    set(Znai ${ZNAI_FOUND})
endif()
