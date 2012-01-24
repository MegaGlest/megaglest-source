#########################################################################################
# streflop

# IMPORTANT: should come BEFORE finding packages
IF(WANT_STREFLOP)
	ADD_SUBDIRECTORY(sources/streflop)
ENDIF()

#########################################################################################
# common libraries

SET(EXTERNAL_LIBS "")
SET(TARGET_NAME "libmegaglest")

IF(BUILD_MEGAGLEST_MODEL_VIEWER OR BUILD_MEGAGLEST_MAP_EDITOR OR BUILD_MEGAGLEST_CONFIGURATOR OR BUILD_MEGAGLEST)
	MESSAGE(STATUS "Will try to build MegaGlest shared library")

	INCLUDE (CheckIncludeFiles)

	IF(WIN32)
		SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH}
					${PROJECT_SOURCE_DIR}/source/win32_deps/lib
					${PROJECT_SOURCE_DIR}/source/win32_deps/xerces-c-src_2_8_0/lib
					${PROJECT_SOURCE_DIR}/source/win32_deps/wxWidgets-2.8.10/lib)

		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/include)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/openal-soft-1.12.854/include)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/openal-soft-1.12.854)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/xerces-c-src_2_8_0/include)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/libogg-1.2.1/include)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/lua-5.1/src)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/jpeg-8b)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/lpng141)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/zlib-1.2.5)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/curl-7.21.3/include)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/SDL-1.2.x/include)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/wxWidgets-2.8.10/include)
				       
		link_directories(${PROJECT_SOURCE_DIR}/source/win32_deps/lib)
		link_directories(${PROJECT_SOURCE_DIR}/source/win32_deps/xerces-c-src_2_8_0/lib)
		link_directories(${PROJECT_SOURCE_DIR}/source/win32_deps/wxWidgets-2.8.10/lib)

		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/include)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/shared_lib/include/platform/posix)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/shared_lib/include/platform/win32)
	ENDIF()

	FIND_PACKAGE(CURL REQUIRED)
	INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRS})
	IF(UNIX)
		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${CURL_LIBRARIES})
	ENDIF()
	# check libcurl version #
	IF(WIN32)
		file (READ ${PROJECT_SOURCE_DIR}${CURL_INCLUDE_DIR}/curl/curlver.h CURL_VERSION_H_CONTENTS)
	ELSE()
		file (READ ${CURL_INCLUDE_DIR}/curl/curlver.h CURL_VERSION_H_CONTENTS)
	ENDIF()

	string (REGEX MATCH "LIBCURL_VERSION_MAJOR[ \t]+([0-9]+)" LIBCURL_VERSION_MJ ${CURL_VERSION_H_CONTENTS})
	string (REGEX MATCH "([0-9]+)" LIBCURL_VERSION_MJ ${LIBCURL_VERSION_MJ})
	string (REGEX MATCH "LIBCURL_VERSION_MINOR[ \t]+([0-9]+)" LIBCURL_VERSION_MI ${CURL_VERSION_H_CONTENTS})
	string (REGEX MATCH "([0-9]+)" LIBCURL_VERSION_MI ${LIBCURL_VERSION_MI})
	string (REGEX MATCH "LIBCURL_VERSION_PATCH[ \t]+([0-9]+)" LIBCURL_VERSION_PT ${CURL_VERSION_H_CONTENTS})
	string (REGEX MATCH "([0-9]+)" LIBCURL_VERSION_PT ${LIBCURL_VERSION_PT})
	set (CURL_MAJOR_VERSION ${LIBCURL_VERSION_MJ})
	set (CURL_MINOR_VERSION ${LIBCURL_VERSION_MI})
	set (CURL_PATCH_VERSION ${LIBCURL_VERSION_PT})
	include_regular_expression("^.*$")
	# Setup package meta-data
	set(CURL_VERSION ${CURL_MAJOR_VERSION}.${CURL_MINOR_VERSION}.${CURL_PATCH_VERSION})
	message(STATUS "curl version=[${CURL_VERSION}]")
	#IF( ${CURL_VERSION} VERSION_LESS 7.21.0)
	IF( ${CURL_VERSION} VERSION_LESS 7.16.4)
		message(STATUS "(please visit http://curl.haxx.se/libcurl/ to find a newer version)")	
		#message(FATAL_ERROR " CURL version = [${CURL_VERSION}] we require AT LEAST [7.21.0]")
		message(FATAL_ERROR " CURL version = [${CURL_VERSION}] we require AT LEAST [7.16.4]")
	ENDIF()

	FIND_PACKAGE(SDL REQUIRED)
	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
	IF(UNIX)
		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${SDL_LIBRARY})
	ENDIF()

	FIND_PACKAGE(OpenGL REQUIRED)
	INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})
	IF(UNIX)
		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${OPENGL_LIBRARY})
	ENDIF()

	FIND_PACKAGE(XercesC REQUIRED)
	INCLUDE_DIRECTORIES(${XERCESC_INCLUDE_DIR})
	IF(UNIX)
		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${XERCESC_LIBRARY})
	ENDIF()

	FIND_PACKAGE(OpenAL REQUIRED)
	INCLUDE_DIRECTORIES(${OPENAL_INCLUDE_DIR})
	IF(UNIX)
		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${OPENAL_LIBRARY})
	ENDIF()

	# FIND_PACKAGE(OGG REQUIRED)
	# INCLUDE_DIRECTORIES(${OGG_INCLUDE_DIR})
	# IF(UNIX)
		# SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${OGG_LIBRARY})
		# SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${VORBIS_LIBRARY})
		# SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${VORBIS_FILE_LIBRARY})
	# ENDIF()

	FIND_PACKAGE(LUA REQUIRED)
	INCLUDE_DIRECTORIES(${LUA_INCLUDE_DIR})
	IF(UNIX)
		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${LUA_LIBRARIES})
	ENDIF()

	OPTION(WANT_STATIC_LIBS "builds as many static libs as possible" OFF)

	IF(WANT_STATIC_LIBS)
		SET(JPEG_NAMES libjpeg.a ${JPEG_NAMES}) 
	ENDIF()

	FIND_PACKAGE(JPEG REQUIRED)
	INCLUDE_DIRECTORIES(${JPEG_INCLUDE_DIR})
	IF(UNIX)
		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${JPEG_LIBRARY})
	ENDIF()
	
	IF(WANT_STATIC_LIBS)
		SET(PNG_NAMES libpng.a ${PNG_NAMES}) 
	ENDIF()
	FIND_PACKAGE(PNG REQUIRED)
	INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIR})
	IF(UNIX)
		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${PNG_LIBRARY})
	ENDIF()

	FIND_PACKAGE(FontConfig)
	IF(FONTCONFIG_FOUND)
            MESSAGE(STATUS "**NOTE: FontConfig support was detected and enabled.")
	    SET(HAVE_FONTCONFIG 1)
            ADD_DEFINITIONS(-DHAVE_FONTCONFIG)

	    INCLUDE_DIRECTORIES( ${FONTCONFIG_INCLUDE_DIR} ) 
	    SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${FONTCONFIG_LIBRARIES} )
	ENDIF(FONTCONFIG_FOUND)

	OPTION(USE_FTGL "Use FTGL for on-screen fonts (found on your system)" ON)

	if(USE_FTGL)
		ADD_DEFINITIONS(-DUSE_FTGL)

		find_package( Freetype REQUIRED)
		include_directories( ${FREETYPE_INCLUDE_DIRS} )
	
	endif(USE_FTGL)

	#INCLUDE(FindPkgConfig.cmake)
	#pkg_search_module (FTGL ftgl)
	FIND_PACKAGE(FTGL)


	IF(USE_FTGL)
		if (FTGL_FOUND)
		        MESSAGE(STATUS "**NOTE: FTGL font support was detected and enabled.")
		else()
		        MESSAGE(STATUS 
			"**NOTE: Warning! FTGL has been enabled but not detected. Your compilation will probably break. Turn off FTGL support by setting USE_FTGL to false, or visit http://ftgl.wiki.sourceforge.net/ for help on installing FTGL.")
		endif()
	
		ADD_DEFINITIONS(-DUSE_FTGL)
	
		#set(FTGL_INCUDE_DIR ${FTGL_INCLUDE_DIR} ${CMAKE_INCLUDE_PATH} ${OPENGL_INCLUDE_DIR})
		#set(FTGL_LINK_DIRS ${FTGL_LIBRARY_DIRS} )
		#set(FTGL_LINK_TARGETS ftgl freetype)
	
		INCLUDE_DIRECTORIES(${FTGL_INCLUDE_DIR})
		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${FTGL_LIBRARY})

	ELSE()
		IF(FTGL_FOUND)
		        MESSAGE(STATUS "**NOTE:  Warning: FTGL font support was detected but NOT enabled. You can enable it by setting USE_FTGL=true")
		ELSE()
		        MESSAGE(STATUS "**NOTE:  Warning: FTGL font support was not detected. Visit http://ftgl.wiki.sourceforge.net/ for help on installing FTGL.")
		ENDIF()
	
		# todo: change this because its obscure and probably not being used
		#set (FTGL_INCLUDE_DIRS )
		#set (FTGL_LINK_DIRS )
		#set (FTGL_LINK_TARGETS )
	ENDIF(USE_FTGL)

	IF(MINIUPNPC_VERSION_PRE1_5)
		ADD_DEFINITIONS(-DMINIUPNPC_VERSION_PRE1_5)
	ENDIF()
	IF(MINIUPNPC_VERSION_PRE1_6)
		ADD_DEFINITIONS(-DMINIUPNPC_VERSION_PRE1_6)
	ENDIF()

	OPTION(USE_FREETYPEGL "Use Freetype-GL for on-screen fonts" OFF)
	if(USE_FREETYPEGL)
		ADD_DEFINITIONS(-DUSE_FREETYPEGL)
	endif(USE_FREETYPEGL)

	FIND_PACKAGE(Freetype REQUIRED)
	INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_PATH})
	IF(UNIX)
		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${FREETYPE_LIBRARY})
	ENDIF()

	FIND_PACKAGE(GLEW REQUIRED)
	INCLUDE_DIRECTORIES(${GLEW_INCLUDE_PATH})
	IF(UNIX)
		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${GLEW_LIBRARY})
	ENDIF()

	#########################################################################################
	# megaglest lib

	SET(DIRS_WITH_SRC
		feathery_ftp	
		glew
		graphics
		graphics/freetype-gl
		graphics/gl
		graphics/md5
		lua
		map	
		platform/common
		sound
		sound/openal
		util
		xml)

	IF(WANT_STREFLOP)
		SET(DIRS_WITH_SRC
			${DIRS_WITH_SRC}
			streflop
			streflop/libm_flt32_source)
	ENDIF()

	IF(NOT WANT_STATIC_LIBS)
		MESSAGE(STATUS "*** Searching for miniupnpc since WANT_STATIC_LIBS is off ...")
		FIND_PACKAGE(Miniupnpc)
	ENDIF()

	if(NOT MINIUPNP_FOUND)
		MESSAGE(STATUS "*** Using EMBEDDED miniupnpc since dev system does not have it... MINIUPNP_FOUND [${MINIUPNP_FOUND}]")

		SET(DIRS_WITH_SRC ${DIRS_WITH_SRC} platform/miniupnpc)
	else()
		MESSAGE(STATUS "*** Using SHARED miniupnpc found in [${MINIUPNP_INCLUDE_DIR}] MINIUPNP_FOUND [${MINIUPNP_FOUND}] MINIUPNP_LIBRARY [${MINIUPNP_LIBRARY}]")

		INCLUDE_DIRECTORIES(${MINIUPNP_INCLUDE_DIR})
		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${MINIUPNP_LIBRARY})
	endif()

	IF(APPLE)
		SET(DIRS_WITH_SRC ${DIRS_WITH_SRC} platform/sdl)
		SET(DIRS_WITH_SRC ${DIRS_WITH_SRC} platform/posix)
		SET(DIRS_WITH_SRC ${DIRS_WITH_SRC} platform/macosx)
	ELSEIF(WIN32)
		SET(DIRS_WITH_SRC ${DIRS_WITH_SRC} platform/win32)
		SET(DIRS_WITH_SRC ${DIRS_WITH_SRC} sound/ds8)
		SET(DIRS_WITH_SRC ${DIRS_WITH_SRC} ${PROJECT_SOURCE_DIR}/source/win32_deps/src)
	ELSE()
		SET(DIRS_WITH_SRC ${DIRS_WITH_SRC} platform/sdl)
		SET(DIRS_WITH_SRC ${DIRS_WITH_SRC} platform/posix)
		SET(DIRS_WITH_SRC ${DIRS_WITH_SRC} platform/unix)
	ENDIF()
	
	SET(MG_INCLUDES_ROOT "include/")
	SET(MG_SOURCES_ROOT "sources/")
	SET(MG_INCLUDE_FILES "")
	SET(MG_SOURCE_FILES "")
	INCLUDE_DIRECTORIES( ${MG_INCLUDES_ROOT}platform/sdl )

	IF(NOT WANT_STATIC_LIBS)
		MESSAGE(STATUS "*** Searching for libircclient since WANT_STATIC_LIBS is off ...")
		FIND_PACKAGE(Ircclient)
	ENDIF()

	if(NOT IRCCLIENT_FOUND)
		MESSAGE(STATUS "*** Using EMBEDDED libircclient since dev system does not have it... IRCCLIENT_FOUND [${IRCCLIENT_FOUND}]")

		SET(MG_SOURCE_FILES ${MG_SOURCE_FILES} ${PROJECT_SOURCE_DIR}/source/shared_lib/sources/libircclient/src/libircclient.c)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/shared_lib/include/libircclient/include)
	else()
		MESSAGE(STATUS "*** Using SHARED libircclient found in [${IRCCLIENT_INCLUDE_DIR}] IRCCLIENT_FOUND [${IRCCLIENT_FOUND}] IRCCLIENT_LIBRARY [${IRCCLIENT_LIBRARY}]")

		INCLUDE_DIRECTORIES(${IRCCLIENT_INCLUDE_DIR})
		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${IRCCLIENT_LIBRARY})
	endif()

        set(CMAKE_REQUIRED_INCLUDES "${IRCCLIENT_INCLUDE_DIR}")
        set(CMAKE_REQUIRED_LIBRARIES "${IRCCLIENT_LIBRARY}")
        check_cxx_source_runs("
        #include <stdio.h>
        #include <stdlib.h>
        #include <libircclient.h>
        int main()
        {
        unsigned int high = 0;
        unsigned int low = 0;
        irc_get_version(&high,&low);
        if( (high == 1 && low >= 6) || (high > 1)) {
          return 0;
        }
        throw 1;
        }"
        HAS_LIBIRCCLIENT_1_6)

        IF(HAS_LIBIRCCLIENT_1_6)
                message(STATUS "Found HAS_LIBIRCCLIENT_1_6: ${HAS_LIBIRCCLIENT_1_6}")
        ELSE()
                ADD_DEFINITIONS(-DLIBIRCCLIENT_PRE1_6)
        ENDIF()

	IF(WIN32)
		SET(MG_SOURCE_FILES ${MG_SOURCE_FILES} ${PROJECT_SOURCE_DIR}/source/shared_lib/sources/platform/posix/socket.cpp)
		SET(MG_SOURCE_FILES ${MG_SOURCE_FILES} ${PROJECT_SOURCE_DIR}/source/shared_lib/sources/platform/posix/ircclient.cpp)
		SET(MG_SOURCE_FILES ${MG_SOURCE_FILES} ${PROJECT_SOURCE_DIR}/source/shared_lib/sources/platform/posix/miniftpserver.cpp)
		SET(MG_SOURCE_FILES ${MG_SOURCE_FILES} ${PROJECT_SOURCE_DIR}/source/shared_lib/sources/platform/posix/miniftpclient.cpp)
		SET(MG_SOURCE_FILES ${MG_SOURCE_FILES} ${PROJECT_SOURCE_DIR}/source/shared_lib/sources/platform/sdl/gl_wrap.cpp)
		SET(MG_SOURCE_FILES ${MG_SOURCE_FILES} ${PROJECT_SOURCE_DIR}/source/shared_lib/sources/platform/sdl/thread.cpp)
		SET(MG_SOURCE_FILES ${MG_SOURCE_FILES} ${PROJECT_SOURCE_DIR}/source/shared_lib/sources/platform/sdl/window.cpp)
		SET(MG_SOURCE_FILES ${MG_SOURCE_FILES} ${PROJECT_SOURCE_DIR}/source/shared_lib/sources/platform/sdl/window_gl.cpp)
		SET(MG_SOURCE_FILES ${MG_SOURCE_FILES} ${PROJECT_SOURCE_DIR}/source/win32_deps/src/glprocs.c)
	ENDIF()
	
	FOREACH(DIR IN LISTS DIRS_WITH_SRC)
		set(INCLUDE_DIR_TO_GLOB ${MG_INCLUDES_ROOT}${DIR})
		INCLUDE_DIRECTORIES( ${INCLUDE_DIR_TO_GLOB} )
		IF(APPLE)
			#MESSAGE(STATUS "INCLUDE_DIR_TO_GLOB: ${INCLUDE_DIR_TO_GLOB}")
			FILE(GLOB INC_FILES_FROM_THIS_DIR ${INCLUDE_DIR_TO_GLOB}/*.h)
			#MESSAGE(STATUS "INCLUDE FILES GLOBBED: ${INC_FILES_FROM_THIS_DIR}")
			SET(MG_INCLUDE_FILES ${MG_INCLUDE_FILES} ${INC_FILES_FROM_THIS_DIR})
		ENDIF(APPLE)

		set(SRC_DIR_TO_GLOB ${MG_SOURCES_ROOT}${DIR})
		#MESSAGE(STATUS "SRC_DIR_TO_GLOB: ${SRC_DIR_TO_GLOB}")
		FILE(GLOB SRC_FILES_FROM_THIS_DIR ${SRC_DIR_TO_GLOB}/*.cpp)
		#MESSAGE(STATUS "SRC_FILES_FROM_THIS_DIR: ${SRC_FILES_FROM_THIS_DIR}")
		SET(MG_SOURCE_FILES ${MG_SOURCE_FILES} ${SRC_FILES_FROM_THIS_DIR})
	
		set(SRC_DIR_TO_GLOB ${MG_SOURCES_ROOT}${DIR})
		#MESSAGE(STATUS "SRC_DIR_TO_GLOB: ${SRC_DIR_TO_GLOB}")
		FILE(GLOB SRC_FILES_FROM_THIS_DIR ${SRC_DIR_TO_GLOB}/*.c)
		#MESSAGE(STATUS "SRC_FILES_FROM_THIS_DIR: ${SRC_FILES_FROM_THIS_DIR}")
		SET(MG_SOURCE_FILES ${MG_SOURCE_FILES} ${SRC_FILES_FROM_THIS_DIR})

		IF(APPLE)
			FILE(GLOB SRC_FILES_FROM_THIS_DIR ${SRC_DIR_TO_GLOB}/*.mm)
			#MESSAGE(STATUS "SRC_FILES_FROM_THIS_DIR: ${SRC_FILES_FROM_THIS_DIR}")
			SET(MG_SOURCE_FILES ${MG_SOURCE_FILES} ${SRC_FILES_FROM_THIS_DIR})
			FILE(GLOB SRC_FILES_FROM_THIS_DIR ${SRC_DIR_TO_GLOB}/*.m)
			#MESSAGE(STATUS "SRC_FILES_FROM_THIS_DIR: ${SRC_FILES_FROM_THIS_DIR}")
			SET(MG_SOURCE_FILES ${MG_SOURCE_FILES} ${SRC_FILES_FROM_THIS_DIR})	
		ENDIF(APPLE)
	ENDFOREACH(DIR)

	#MESSAGE(STATUS "Source files: ${MG_INCLUDE_FILES}")
	#MESSAGE(STATUS "Source files: ${MG_SOURCE_FILES}")
	#MESSAGE(STATUS "Include dirs: ${INCLUDE_DIRECTORIES}")

	# FIXME: hackish, should be tested for
	IF(APPLE)
		SET(PLATFORM_SPECIFIC_DEFINES "-DHAVE_SYS_IOCTL_H")
	ELSEIF(WIN32)
		SET(PLATFORM_SPECIFIC_DEFINES "-DX11_AVAILABLE")
	ELSE()
		SET(PLATFORM_SPECIFIC_DEFINES "-DX11_AVAILABLE -DHAVE_SYS_IOCTL_H")
	ENDIF()

	# FIXME: hackish...
#	IF(WANT_STREFLOP)
#		SET(STREFLOP_PROPERTIES "-DSTREFLOP_SSE -DLIBM_COMPILING_FLT32")
#	ENDIF()

	SET_SOURCE_FILES_PROPERTIES(${MG_SOURCE_FILES} PROPERTIES COMPILE_FLAGS 
		"${PLATFORM_SPECIFIC_DEFINES} ${STREFLOP_PROPERTIES} ${CXXFLAGS}")
	
	SET_SOURCE_FILES_PROPERTIES(${MG_INCLUDE_FILES} PROPERTIES HEADER_FILE_ONLY 1)

	IF(NOT WIN32)
		ADD_LIBRARY(${TARGET_NAME} STATIC ${MG_SOURCE_FILES} ${MG_INCLUDE_FILES})
		MESSAGE(STATUS "Building shared game library as a STATIC library and saving to [${LIBRARY_OUTPUT_PATH}]")
	ELSE()
		SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/data/glest_game)
		ADD_LIBRARY(${TARGET_NAME} STATIC ${MG_SOURCE_FILES} ${MG_INCLUDE_FILES})
		MESSAGE(STATUS "Building shared game library as a SHARED library and saving to [${LIBRARY_OUTPUT_PATH}]")
	ENDIF()

	# ADD_LIBRARY(libmegaglest SHARED ${MG_SOURCE_FILES} ${MG_INCLUDE_FILES})
	# ADD_LIBRARY(libmegaglest STATIC ${MG_SOURCE_FILES} ${MG_INCLUDE_FILES})

	MESSAGE(STATUS "EXTERNAL_LIBS = [${EXTERNAL_LIBS}]")

	OPTION(BUILD_MEGAGLEST_UPNP_DEBUG "Build MegaGlest with UPNP DEBUG enabled" OFF)
	IF(BUILD_MEGAGLEST_UPNP_DEBUG)
		MESSAGE(STATUS "Will try to build MegaGlest shared library with DEBUG info for UPNP")
		ADD_DEFINITIONS("-DDEBUG")
	ENDIF()

	IF(WIN32)
	  TARGET_LINK_LIBRARIES(${TARGET_NAME} stdc++ gcc odbc32 wsock32 winspool winmm shell32 comctl32 ctl3d32  advapi32 wsock32 opengl32 glu32 ole32 oleaut32 uuid mingw32 ddraw dsound dxguid ws2_32 iphlpapi wsock32 libogg libvorbis libvorbisfile zlib jpeg libpng xerces-c2_8_0 OpenAL32 libcurl winmm gdi32 opengl32 glu32 SDL SDLmain lua5.1 stdc++ moldname mingwex msvcrt user32 kernel32)

	  IF(WANT_STREFLOP)
	    TARGET_LINK_LIBRARIES(${TARGET_NAME} streflop)
          ENDIF()
	  TARGET_LINK_LIBRARIES(${TARGET_NAME} ${EXTERNAL_LIBS})
	ELSE()
          IF(WANT_STREFLOP)
	    TARGET_LINK_LIBRARIES(${TARGET_NAME} streflop)
 	  ENDIF()
	  TARGET_LINK_LIBRARIES(${TARGET_NAME} ${EXTERNAL_LIBS})

	ENDIF()

	#IF(BUILD_MEGAGLEST_UPNP_DEBUG)
		# REMOVE_DEFINITIONS("-DDEBUG")
	#ENDIF()

ENDIF()
