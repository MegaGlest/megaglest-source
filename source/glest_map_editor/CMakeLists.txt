# common libraries

SET(EXTERNAL_LIBS "")

FIND_PACKAGE(SDL REQUIRED)
INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${SDL_LIBRARY})

FIND_PACKAGE(OpenGL REQUIRED)
INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})
SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${OPENGL_LIBRARY})

IF(NOT WIN32)
	# It was noticed that when using MinGW gcc it is essential that 'core' is mentioned before 'base'.
	FIND_PACKAGE(wxWidgets COMPONENTS core base gl REQUIRED)

	# wxWidgets include (this will do all the magic to configure everything)
	include( ${wxWidgets_USE_FILE} )
	#INCLUDE_DIRECTORIES(${WX_INCLUDE_DIR})
	SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${wxWidgets_LIBRARIES})
ELSE()
	SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${PROJECT_SOURCE_DIR}/source/win32_deps/wxWidgets-2.8.10/lib/libwx_mswu-2.8-i586-mingw32msvc.dll.a)
	SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${PROJECT_SOURCE_DIR}/source/win32_deps/wxWidgets-2.8.10/lib/libwx_mswu_gl-2.8-i586-mingw32msvc.dll.a)
	SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${PROJECT_SOURCE_DIR}/source/win32_deps/wxWidgets-2.8.10/lib/libwxpng-2.8-i586-mingw32msvc.a)
	SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${PROJECT_SOURCE_DIR}/source/win32_deps/wxWidgets-2.8.10/lib/libwxjpeg-2.8-i586-mingw32msvc.a)
	SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${PROJECT_SOURCE_DIR}/source/win32_deps/wxWidgets-2.8.10/lib/libwxtiff-2.8-i586-mingw32msvc.a)
	SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${PROJECT_SOURCE_DIR}/source/win32_deps/wxWidgets-2.8.10/lib/libwxzlib-2.8-i586-mingw32msvc.a)

	INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/wxWidgets-2.8.10/include/)
	INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/wxWidgets-2.8.10/lib/wx/include/i586-mingw32msvc-msw-unicode-release-static-2.8)

	SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH}
			       ${PROJECT_SOURCE_DIR}/source/win32_deps/lib
			       ${PROJECT_SOURCE_DIR}/source/win32_deps/xerces-c-src_2_8_0/lib)

	INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/include)
	INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/openal-soft-1.12.854/include)
	INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/openal-soft-1.12.854)
	INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/xerces-c-src_2_8_0/include)
	INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/libogg-1.2.1/include)
	INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/lua-5.1/src)
	INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/jpeg-8b)
	INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/lpng141)
	INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/zlib-1.2.5)
	INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/curl-7.20.1/include)
	INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/SDL-1.2.x/include)
			       
	link_directories(${PROJECT_SOURCE_DIR}/source/win32_deps/lib)
	link_directories(${PROJECT_SOURCE_DIR}/source/win32_deps/xerces-c-src_2_8_0/lib)
ENDIF()

# stupid hack because cmake WILL NOT find libircclient for some reason
if(WIN32)

	if(EXISTS ${PROJECT_SOURCE_DIR}/source/win32_deps/libircclient/include/libircclient.h)
		set(LIBIRCCLIENT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/source/win32_deps/libircclient/include)
	  	link_directories(${PROJECT_SOURCE_DIR}/source/win32_deps/libircclient/build/lib)
	endif()

else()
	#CHECK_INCLUDE_FILES(libircclient.h LIBIRCCLIENT_INCLUDE_DIR)
	FIND_PATH(LIBIRCCLIENT_INCLUDE_DIR NAME libircclient.h)
endif()

#check_include_files("libircclient/libircclient.h;libircclient/include/libircclient.h" LIBIRCCLIENT_H_)
if(NOT LIBIRCCLIENT_INCLUDE_DIR)
       message(FATAL_ERROR "missing libircclient : please install it [${LIBIRCCLIENT_INCLUDE_DIR}] [${CMAKE_FIND_ROOT_PATH}]")
endif()
INCLUDE_DIRECTORIES(${LIBIRCCLIENT_INCLUDE_DIR})
#SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ircclient)


#########################################################################################
# glest map editor

SET(DIRS_WITH_SRC
    . )

SET(GLEST_LIB_INCLUDE_ROOT "../shared_lib/include/")
SET(GLEST_LIB_INCLUDE_DIRS
    ${GLEST_LIB_INCLUDE_ROOT}platform/common
	${GLEST_LIB_INCLUDE_ROOT}platform/sdl
	${GLEST_LIB_INCLUDE_ROOT}platform/posix
	${GLEST_LIB_INCLUDE_ROOT}util
	${GLEST_LIB_INCLUDE_ROOT}graphics
	${GLEST_LIB_INCLUDE_ROOT}graphics/gl
	${GLEST_LIB_INCLUDE_ROOT}map)
INCLUDE_DIRECTORIES( ${GLEST_LIB_INCLUDE_DIRS} )

SET(MG_SOURCE_FILES "")
	
FOREACH(DIR IN LISTS DIRS_WITH_SRC)
	INCLUDE_DIRECTORIES( ${DIR} )

	set(SRC_DIR_TO_GLOB ${DIR})
	FILE(GLOB SRC_FILES_FROM_THIS_DIR ${SRC_DIR_TO_GLOB}/*.cpp)
	SET(MG_SOURCE_FILES ${MG_SOURCE_FILES} ${SRC_FILES_FROM_THIS_DIR})
        FILE(GLOB INCLUDE_FILES_FROM_THIS_DIR ${SRC_DIR_TO_GLOB}/*.h)
	SET(MG_INCLUDE_FILES ${MG_INCLUDE_FILES} ${INCLUDE_FILES_FROM_THIS_DIR})	
	IF(APPLE)
		FILE(GLOB SRC_FILES_FROM_THIS_DIR ${SRC_DIR_TO_GLOB}/*.mm)
		#MESSAGE(STATUS "SRC_FILES_FROM_THIS_DIR: ${SRC_FILES_FROM_THIS_DIR}")
		SET(MG_SOURCE_FILES ${MG_SOURCE_FILES} ${SRC_FILES_FROM_THIS_DIR})
		FILE(GLOB SRC_FILES_FROM_THIS_DIR ${SRC_DIR_TO_GLOB}/*.m)
		#MESSAGE(STATUS "SRC_FILES_FROM_THIS_DIR: ${SRC_FILES_FROM_THIS_DIR}")
		SET(MG_SOURCE_FILES ${MG_SOURCE_FILES} ${SRC_FILES_FROM_THIS_DIR})	
		ENDIF(APPLE)
ENDFOREACH(DIR)

# [RCL] FIXME: make it compiler-agnostic
SET(CMAKE_CXX_FLAGS "-O3 -fno-strict-aliasing -frounding-math  -fsignaling-nans -mfpmath=sse -msse -DSTREFLOP_SSE")

IF(WIN32)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WINDOWS -D_WIN32 -D_STDCALL_SUPPORTED -D_M_IX86 -DXML_LIBRARY -DMWIN -D__WXMSW__ -DCURL_STATICLIB")
ENDIF()

SET(TARGET_NAME "glest_editor")

IF(UNIX AND NOT APPLE)
	SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/mk/linux)
ELSE()
	SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/data/glest_game)
ENDIF()

SET_SOURCE_FILES_PROPERTIES(${MG_INCLUDE_FILES} PROPERTIES HEADER_FILE_ONLY 1)

ADD_EXECUTABLE(${TARGET_NAME} ${MG_SOURCE_FILES} ${MG_INCLUDE_FILES})
IF(WIN32)
	TARGET_LINK_LIBRARIES(${TARGET_NAME} stdc++ gcc odbc32 wsock32 winspool winmm shell32 comctl32 ctl3d32  advapi32 wsock32 opengl32 glu32 ole32 oleaut32 uuid mingw32 ddraw dsound dxguid ws2_32 iphlpapi wsock32 libogg libvorbis libvorbisfile zlib jpeg libpng xerces-c2_8_0 OpenAL32 libcurl winmm gdi32 opengl32 glu32 SDL SDLmain lua5.1 streflop libglest stdc++ moldname mingwex msvcrt user32 kernel32)
ENDIF()

TARGET_LINK_LIBRARIES(${TARGET_NAME} streflop)
TARGET_LINK_LIBRARIES(${TARGET_NAME} libglest)
TARGET_LINK_LIBRARIES(${TARGET_NAME} ${EXTERNAL_LIBS})
