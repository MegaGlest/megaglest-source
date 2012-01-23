#
#
# Try to find the FTGL libraries
# Once done this will define
#
# FTGL_FOUND          - system has ftgl
# FTGL_INCLUDE_DIR    - path to FTGL/FTGL.h
# FTGL_LIBRARY      - the library that must be included
#
#

OPTION(WANT_STATIC_LIBS "builds as many static libs as possible" OFF)
IF(WANT_STATIC_LIBS)
	OPTION(FTGL_STATIC "Set to ON to link your project with static library (instead of DLL)." ON)
ENDIF()

IF (FTGL_LIBRARY AND FTGL_INCLUDE_DIR)
  SET(FTGL_FOUND "YES")
ELSE (FTGL_LIBRARY AND FTGL_INCLUDE_DIR)

  FIND_PATH(FTGL_INCLUDE_DIR FTGL/ftgl.h PATHS /usr/local/include /usr/include)

IF (FTGL_STATIC AND NOT FTGL_LIBRARY)
  FIND_LIBRARY(FTGL_LIBRARY NAMES libftgl.a ftgl PATHS /usr/local/lib /usr/lib)
ELSE()
  FIND_LIBRARY(FTGL_LIBRARY NAMES ftgl PATHS /usr/local/lib /usr/lib)
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

