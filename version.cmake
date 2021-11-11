
find_program(GIT_EXECUTABLE git)

if(GIT_EXECUTABLE)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} log -1 --format=%h
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_DESCRIBE_VERSION
        RESULT_VARIABLE GIT_DESCRIBE_ERR_CODE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT GIT_DESCRIBE_ERR_CODE)
        set(GIT_VERSION ${GIT_DESCRIBE_VERSION})
    endif()
endif()

if(NOT DEFINED GIT_VERSION)
    set(GIT_VERSION 0)
    message(WARNING "Failed to get GIT_VERSION from Git log, using default version :: \"${GIT_VERSION}\".")
endif()

configure_file(${SRC} ${DST} @ONLY)
