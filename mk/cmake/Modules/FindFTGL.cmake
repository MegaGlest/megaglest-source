#
#
# Try to find the FTGL libraries
# Once done this will define
#
# FTGL_FOUND          - system has ftgl
# FTGL_INCLUDE_DIR    - path to FTGL/FTGL.h
# FTGL_LIBRARY        - the library that must be included
# FTGL_LIBRARY_PATH   - the library path
#
#

#message(STATUS "!!!!!!!!!!!!!!!!!!!!!!!!!!!!! #1 Searching for FTGL lib in custom path: [${FTGL_LIBRARY_PATH}]")

IF (FTGL_LIBRARY AND FTGL_INCLUDE_DIR)
  SET(FTGL_FOUND "YES")
  message(STATUS "** FTGL lib ALREADY FOUND in: [${FTGL_LIBRARY}]")
ELSE (FTGL_LIBRARY AND FTGL_INCLUDE_DIR)

  IF(FTGL_LIBRARY_PATH)
        message(STATUS "** Searching for FTGL lib in custom path: [${FTGL_LIBRARY_PATH}]")
  ENDIF()

  FIND_PATH(FTGL_INCLUDE_DIR FTGL/ftgl.h 
            PATHS /usr/local/include 
                  /usr/include)

  IF (STATIC_FTGL AND NOT FTGL_LIBRARY)
    FIND_LIBRARY(FTGL_LIBRARY 
                  NAMES libftgl.a ftgl.a ftgl libftgl libftgl.dll
                  PATHS /usr/local/lib 
                        /usr/lib
                        ${FTGL_LIBRARY_PATH})
  ELSE()
    FIND_LIBRARY(FTGL_LIBRARY 
                  NAMES ftgl libftgl libftgl.dll libftgl.a
                  PATHS /usr/local/lib 
                        /usr/lib
                        ${FTGL_LIBRARY_PATH})
  ENDIF()
  
  IF (FTGL_INCLUDE_DIR AND FTGL_LIBRARY)
    SET(FTGL_FOUND "YES")
  ELSE (FTGL_INCLUDE_DIR AND FTGL_LIBRARY)
    SET(FTGL_FOUND "NO")
  ENDIF (FTGL_INCLUDE_DIR AND FTGL_LIBRARY)
ENDIF (FTGL_LIBRARY AND FTGL_INCLUDE_DIR)

IF (FTGL_FOUND)
  MESSAGE(STATUS "Found FTGL libraries at ${FTGL_LIBRARY} and includes at ${FTGL_INCLUDE_DIR}")
ELSE (FTGL_FOUND)
  IF (FTGL_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find FTGL libraries")
  ENDIF (FTGL_FIND_REQUIRED)
ENDIF (FTGL_FOUND)

MARK_AS_ADVANCED(FTGL_INCLUDE_DIR FTGL_LIBRARY)
