CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( MegaGlest )

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/mk/cmake/Modules)

ADD_SUBDIRECTORY( source/shared_lib )
ADD_SUBDIRECTORY( source/glest_game )

IF(APPLE)
	include(mk/macosx/CMakeLists.txt)
ENDIF(APPLE)
