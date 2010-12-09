CMAKE_MINIMUM_REQUIRED( VERSION 2.6.2 )
PROJECT( MegaGlest )

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/mk/cmake/Modules)

ADD_SUBDIRECTORY( source/shared_lib )
ADD_SUBDIRECTORY( source/glest_game )
ADD_SUBDIRECTORY( source/glest_map_editor )
ADD_SUBDIRECTORY( source/g3d_viewer )
ADD_SUBDIRECTORY( source/configurator )

IF(APPLE)
	include(mk/macosx/CMakeLists.txt)
ENDIF(APPLE)
