# CMake 2.4.3 lacks a "CMake -E copy" command that handles wildcards.
#
# INPUT:
#
#   FROM - absolute pathname with wildcards to copy
#   TO - absolute pathname of directory to copy to
#
# TYPICAL USAGE, from inside a custom target or rule:
#
#   COMMAND ${CMAKE_COMMAND}
#     -D FROM=${mydirectory}/*.dll
#     -D TO=${yourdirectory}
#     -P ${CMAKE_HOME_DIRECTORY}/cp.cmake

FILE(GLOB FILELIST "${FROM}")

FOREACH(LOOPER ${FILELIST})
    MESSAGE(STATUS "Copying ${LOOPER} to ${TO}")
    EXEC_PROGRAM("${CMAKE_COMMAND}" ARGS "-E copy '${LOOPER}' '${TO}'"
                 OUTPUT_VARIABLE EXECOUT
                 RETURN_VALUE RC
    )
    # !!! FIXME: how do you do NOT EQUALS?
    IF(NOT RC EQUAL 0)
        MESSAGE(STATUS "${EXECOUT}")
        MESSAGE(FATAL_ERROR "Copy of '${LOOPER}' failed!")
    ENDIF(NOT RC EQUAL 0)
ENDFOREACH(LOOPER)

