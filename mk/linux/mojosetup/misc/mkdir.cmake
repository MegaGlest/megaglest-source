# CMake 2.4.3 lacks a "CMake -E mkdir" command.
# It does have a MAKE_DIRECTORY command, but we cannot run it from inside
# a custom rule or target.  So, we wrap it in a script, which we can then
# call from a custom command or target.
#
# INPUT:
#
#   DIR - absolute pathname of directory to be created
#
# TYPICAL USAGE, from inside a custom target or rule:
#
#   COMMAND ${CMAKE_COMMAND}
#     -D DIR=${mydirectory}
#     -P ${CMAKE_HOME_DIRECTORY}/mkdir.cmake

MESSAGE(STATUS "Creating directory ${DIR}")
FILE(MAKE_DIRECTORY "${DIR}")

