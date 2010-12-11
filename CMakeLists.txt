CMAKE_MINIMUM_REQUIRED( VERSION 2.6.2 )
PROJECT( MegaGlest )

SET(CMAKE_VERBOSE_MAKEFILE ON)
SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/mk/cmake/Modules)

IF(WIN32)
	SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH}
						   ${PROJECT_SOURCE_DIR}/source/win32_deps/lib
						   ${PROJECT_SOURCE_DIR}/source/win32_deps/xerces-c-src_2_8_0/lib)

	SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH}
						   ${PROJECT_SOURCE_DIR}/source/win32_deps/include
						   ${PROJECT_SOURCE_DIR}/source/win32_deps/openal-soft-1.12.854/include
						   ${PROJECT_SOURCE_DIR}/source/win32_deps/openal-soft-1.12.854
						   ${PROJECT_SOURCE_DIR}/source/win32_deps/xerces-c-src_2_8_0/include
						   ${PROJECT_SOURCE_DIR}/source/win32_deps/libogg-1.2.1/include
						   ${PROJECT_SOURCE_DIR}/source/win32_deps/lua-5.1/src
						   ${PROJECT_SOURCE_DIR}/source/win32_deps/jpeg-8b
						   ${PROJECT_SOURCE_DIR}/source/win32_deps/lpng141
						   ${PROJECT_SOURCE_DIR}/source/win32_deps/zlib-1.2.5
						   ${PROJECT_SOURCE_DIR}/source/win32_deps/curl-7.20.1/include
						   ${PROJECT_SOURCE_DIR}/source/win32_deps/SDL-1.2.x/include
						   )
	link_directories(${PROJECT_SOURCE_DIR}/source/win32_deps/lib)
	link_directories(${PROJECT_SOURCE_DIR}/source/win32_deps/xerces-c-src_2_8_0/lib)
	include_directories("${PROJECT_SOURCE_DIR}/source/win32_deps/Microsoft\ DirectX\ SDK \(November 2007\)/Include")
	include_directories(${PROJECT_SOURCE_DIR}/source/win32_deps/openal-soft-1.12.854/include)
ENDIF()
	   
ADD_SUBDIRECTORY( source/shared_lib )
ADD_SUBDIRECTORY( source/glest_game )
IF(UNIX)
	ADD_SUBDIRECTORY( source/glest_map_editor )
	ADD_SUBDIRECTORY( source/g3d_viewer )
	ADD_SUBDIRECTORY( source/configurator )
ENDIF()

IF(APPLE)
	include(mk/macosx/CMakeLists.txt)
ENDIF(APPLE)
