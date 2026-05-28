
FIND_PACKAGE(CURL REQUIRED)
INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRS})

# More common libs (libs that are required by both the game and the shared library)
# should be added here
SET(COMMON_LIBS ${CURL_LIBRARIES})
ADD_SUBDIRECTORY(shared_lib)
IF(BUILD_MEGAGLEST)
	MESSAGE(STATUS "Will try to build MegaGlest game")
	ADD_SUBDIRECTORY(glest_game)
ENDIF()
ADD_SUBDIRECTORY( ${PROJECT_SOURCE_DIR}/source/tests )
