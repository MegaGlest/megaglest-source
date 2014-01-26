CMAKE_MINIMUM_REQUIRED( VERSION 2.6.2 )
PROJECT( MegaGlest )

#SET(CMAKE_VERBOSE_MAKEFILE ON)

#
# *NOTE: For now we assume some variation of GCC Compiler (or MingW for Windows binaries)
# VC++ users should not use CMake yet but rather the build-mg.bat file in mk/windoze
#

# build type defaults to release
if(NOT DEFINED CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose build type: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
endif(NOT DEFINED CMAKE_BUILD_TYPE)

# add additional CMake modules
MESSAGE(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/mk/cmake/Modules)

MESSAGE(STATUS "Build type for this compile will be: ${CMAKE_BUILD_TYPE}")

IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        SET(CMAKE_COMPILER_IS_GNUCXX "YES")
        ADD_DEFINITIONS("-Qunused-arguments -Wno-switch")
ENDIF()

OPTION(BUILD_MEGAGLEST_MODEL_IMPORT_EXPORT_TOOLS "Build model import/export tools" ON)
OPTION(BUILD_MEGAGLEST_MODEL_VIEWER "Build model viewer" ON)
OPTION(BUILD_MEGAGLEST_MAP_EDITOR "Build map editor" ON)
OPTION(BUILD_MEGAGLEST "Build MegaGlest" ON)
OPTION(BUILD_MEGAGLEST_TESTS "Build MegaGlest Unit Tests" OFF)
OPTION(WITH_VLC "Use libVLC to play videos" ON)

FIND_PROGRAM(HELP2MAN "help2man")

set(XVFB_EXEC "")
find_program(XVFB_RUN NAMES "xvfb-run")
if (NOT ${XVFB_RUN} MATCHES "XVFB_RUN-NOTFOUND")
	set(XVFB_EXEC ${XVFB_RUN})
	message(STATUS "Using xvfb-run to run man2help.")
endif()

include(${CMAKE_SOURCE_DIR}/mk/cmake/Modules/SpecialMacros.cmake) 

SET(PKG_NAME "megaglest")
# read version
file(READ ${PROJECT_SOURCE_DIR}/source/glest_game/facilities/game_util.cpp MG_VERSION_H_CONTENTS)
# find the version string
#
MESSAGE(STATUS "Determining version number system type to use:")
string (REGEX MATCH "\"v([0-9]).([0-9]).([0-9].*)[^\";]" _threePartMatch ${MG_VERSION_H_CONTENTS})
if(NOT _threePartMatch)
        MESSAGE(STATUS "** Trying 2 part versioning] ")
        string (REGEX MATCH "\"v([0-9]).([0-9].*)[^\";]" _threePartMatch ${MG_VERSION_H_CONTENTS})

        if (_threePartMatch)
                # parse the parts of the version string
                STRING(REGEX REPLACE "\"v([0-9])\\.[0-9].*" "\\1" VER_MAJOR   "${_threePartMatch}")
                STRING(REGEX REPLACE "\"v[0-9]\\.([0-9]).*" "\\1" VER_MINOR "${_threePartMatch}")

                MESSAGE(STATUS "MegaGlest Version is [${VER_MAJOR}.${VER_MINOR}] ")
        endif()
# parse the version string
else()
        MESSAGE(STATUS "** Trying 3 part versioning] ")
        if (_threePartMatch)
                # parse the parts of the version string
                STRING(REGEX REPLACE "\"v([0-9])\\.[0-9]\\.[0-9].*" "\\1" VER_MAJOR   "${_threePartMatch}")
                STRING(REGEX REPLACE "\"v[0-9]\\.([0-9])\\.[0-9].*" "\\1" VER_MINOR   "${_threePartMatch}")
                STRING(REGEX REPLACE "\"v[0-9]\\.[0-9]\\.([0-9]).*" "\\1" VER_RELEASE "${_threePartMatch}")

                #MESSAGE(STATUS "******* TEST3 MegaGlest Version #1 is [${VER_MAJOR}] ")
                #MESSAGE(STATUS "******* TEST3 MegaGlest Version #2 is [${VER_MINOR}] ")
                #MESSAGE(STATUS "******* TEST3 MegaGlest Version #3 is [${VER_RELEASE}] ")

                MESSAGE(STATUS "MegaGlest Version is [${VER_MAJOR}.${VER_MINOR}.${VER_RELEASE}] ")
        endif()
endif()

## Compiler flags
IF(APPLE)
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
ENDIF(APPLE)

IF(CMAKE_COMPILER_IS_GNUCXX OR MINGW)
        IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
                MESSAGE(STATUS "Detected CLANG Compiler...")
	ELSEIF(CMAKE_COMPILER_IS_GNUCXX)
		MESSAGE(STATUS "Detected GNUC Compiler...")
	ELSEIF(MINGW)
		MESSAGE(STATUS "Detected MINGW Compiler...")
	ENDIF()

	if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo)
	endif (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)

	# Default compiler flags
	#SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s")  ## Strip binary

	# For gcc warning options see: http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
	IF(NOT MINGW)
		# For tons of verbose warnings add: -Wall
#		ADD_DEFINITIONS("-Wreturn-type -fno-strict-aliasing -frounding-math  -fsignaling-nans -mfpmath=sse -msse -rdynamic")
		ADD_DEFINITIONS("-Wuninitialized -Wsign-compare -Wunused-function -Wunused-variable -Wreturn-type -fno-strict-aliasing -frounding-math  -fsignaling-nans -rdynamic")
	ELSE()
#		ADD_DEFINITIONS("-Wreturn-type -fno-strict-aliasing -frounding-math  -fsignaling-nans -mfpmath=sse -msse -DUNICODE")
		ADD_DEFINITIONS("-Wuninitialized -Wsign-compare -Wunused-function -Wunused-variable -Wreturn-type -fno-strict-aliasing -frounding-math  -fsignaling-nans -DUNICODE")
	ENDIF()
	#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")	
        
        #MESSAGE(STATUS "*TEST: Checking for max SSE LEVEL [${MAX_SSE_LEVEL_DESIRED}]")

        IF(NOT STREFLOP_SOFTWRAPPER_FORCE)
                SET(STREFLOP_SOFTWRAPPER_FORCE "0" CACHE STRING "Set the streflop library to be forced to use the software emulator")
        ENDIF()

        IF(NOT MAX_SSE_LEVEL_DESIRED AND NOT STREFLOP_SOFTWRAPPER_FORCE)
                SET(MAX_SSE_LEVEL_DESIRED "1" CACHE STRING "Set the max SSE level to use if supported (0-3)")
        ENDIF()
        
        IF(NOT STREFLOP_SOFTWRAPPER_FORCE)
                MESSAGE(STATUS "*NOTE: Checking for max SSE LEVEL [${MAX_SSE_LEVEL_DESIRED}]")
                special_check_for_sse( ${MAX_SSE_LEVEL_DESIRED} )
        ENDIF()

        OPTION(WANT_STREFLOP "use the library streflop" OFF)
	IF(WANT_STREFLOP)
		ADD_DEFINITIONS("-DUSE_STREFLOP -DSTREFLOP_RANDOM_GEN_SIZE=32 -DLIBM_COMPILING_FLT32 -DN_SPECIALIZED=32")

                IF(HAS_SSE_EXTENSIONS AND NOT ${MAX_SSE_LEVEL_DESIRED} MATCHES "0" AND NOT STREFLOP_SOFTWRAPPER_FORCE)
                        ADD_DEFINITIONS("-DSTREFLOP_SSE")
                        MESSAGE(STATUS "*NOTE: using SSE for STREFLOP.")
                ELSE()
                        IF(NOT STREFLOP_SOFTWRAPPER_FORCE)
                                special_check_for_x87()
                        ENDIF()

                        IF(HAS_X87_SUPPORT AND NOT STREFLOP_SOFTWRAPPER_FORCE)
                                ADD_DEFINITIONS("-DSTREFLOP_X87")
                                MESSAGE(STATUS "*NOTE: using X87 for STREFLOP.")
                        ELSE()
                                ADD_DEFINITIONS("-DSTREFLOP_SOFT")
                                MESSAGE(STATUS "*NOTE: using SOFT emulation for STREFLOP.")
                        ENDIF()
                ENDIF()
	ELSE()
		MESSAGE(STATUS "*WARNING: Disabled use of STREFLOP! Out of synchs may occur")
	ENDIF()

        include(CheckCXXSourceRuns)

        check_cxx_source_runs("
        #include <execinfo.h>
        #include <stdio.h>
        int main()
        {
	const size_t max_depth = 6;
	void *stack_addrs[max_depth];
	size_t stack_depth = backtrace(stack_addrs, max_depth);
        }"
        HAS_GCC_BACKTRACE)

        IF(HAS_GCC_BACKTRACE)
                message(STATUS "Found GCC backtrace lib, will support backtraces")
                ADD_DEFINITIONS(-DHAS_GCC_BACKTRACE)
        ELSE()
                message(STATUS "**WARNING DID NOT Find GCC backtrace lib")
        ENDIF()

	# Debug compiler flags
	SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3")

	# Release compiler flags
	SET(CMAKE_CXX_FLAGS_RELEASE "-O3 ${CMAKE_CXX_FLAGS_RELEASE} -O3 ")
	SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s")  ## Strip binary

	# Release with debug info compiler flags
	SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 ${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g -O3 ") 

	# Release minimum size compiler flags
	SET(CMAKE_CXX_FLAGS_MINSIZEREL "-O3 ${CMAKE_CXX_FLAGS_MINSIZEREL} -O3 ")
	SET(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} -s")  ## Strip binary 

        # Get the git revision info for the binary
	SET(HAS_GIT "FALSE") 
        SET(GIT_LIVE_REV_CMD "") 

	OPTION(WANT_GIT_STAMP "use git revision stamp" ON)
	IF(WANT_GIT_STAMP)
		# The stuff below gets GIT Global Revision # but ONLY when calling cmake!
		# the FindGit.cmake module is part of the standard distribution
		include(FindGit)
	
		IF(GIT_FOUND)
                        SET(HAS_GIT "TRUE") 
                        MESSAGE(STATUS "Found GIT and using GIT version stamping...")

                        # Get the current commit SHA1
                        execute_process(
                          COMMAND git log -1 --format=%h
                          WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                          OUTPUT_VARIABLE GIT_SHA1
                          OUTPUT_STRIP_TRAILING_WHITESPACE
                        )

                        # Get the current version counter
                        execute_process(
                          COMMAND git rev-list HEAD --count
                          WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                          OUTPUT_VARIABLE GIT_COMMIT_COUNT
                          OUTPUT_STRIP_TRAILING_WHITESPACE
                        )

                        SET(GIT_LIVE_REV_CMD "`git rev-list HEAD --count`.`git log -1 --format=%h`")

                        MESSAGE(STATUS "Using GIT revision stamp: [${GIT_COMMIT_COUNT}.${GIT_SHA1}] CMD [${GIT_LIVE_REV_CMD}]")
                        
		ENDIF()
	ENDIF()

        IF(APPLE AND NOT CMAKE_COMPILER_IS_GNUCXX)
	        SET(GIT_VERSION_CMD "-DGITVERSION='\\\\'${GIT_LIVE_REV_CMD}\\\\''")
        ELSE()
                SET(GIT_VERSION_CMD "-DGITVERSION='\\\"${GIT_LIVE_REV_CMD}\\\"'")
#                SET(SVN_VERSION_CMD "-DSVNVERSION='\\\"`svnversion -n ${PROJECT_SOURCE_DIR}`\\\"'")
        ENDIF()

	IF(CMAKE_INSTALL_PREFIX STREQUAL "")
		MESSAGE(STATUS "*NOTE: NOT USING a Custom Data Install Path...")
	ELSE()
		IF(NOT MEGAGLEST_BIN_INSTALL_PATH)
			SET(MEGAGLEST_BIN_INSTALL_PATH "bin/" CACHE STRING "The installation path for binaries (this is appended to the CMAKE_INSTALL_PREFIX)")
		ENDIF()
		IF(NOT MEGAGLEST_DATA_INSTALL_PATH)
			SET(MEGAGLEST_DATA_INSTALL_PATH "share/megaglest/" CACHE STRING "The installation path for data files (this is appended to the CMAKE_INSTALL_PREFIX)")
		ENDIF()
		IF(NOT MEGAGLEST_DESKTOP_INSTALL_PATH)
                	SET(MEGAGLEST_DESKTOP_INSTALL_PATH "share/applications/" CACHE STRING "The installation path for desktop files (this is appended to the CMAKE_INSTALL_PREFIX)")
		ENDIF()
		IF(NOT MEGAGLEST_ICON_INSTALL_PATH)
                	SET(MEGAGLEST_ICON_INSTALL_PATH "share/pixmaps/" CACHE STRING "The installation path for icon files (this is appended to the CMAKE_INSTALL_PREFIX)")
		ENDIF()
		IF(NOT MEGAGLEST_MANPAGE_INSTALL_PATH)
                	SET(MEGAGLEST_MANPAGE_INSTALL_PATH "share/man/man6/" CACHE STRING "The installation path for manpage files (this is appended to the CMAKE_INSTALL_PREFIX)")
		ENDIF()

		IF(NOT CUSTOM_DATA_INSTALL_PATH)
                        IF(APPLE)
			        SET(CUSTOM_DATA_INSTALL_PATH "'\\\\'${CMAKE_INSTALL_PREFIX}/${MEGAGLEST_DATA_INSTALL_PATH}\\\\''" CACHE STRING "The FULL installation path for data files (this is build automatically by combining CMAKE_INSTALL_PREFIX and MEGAGLEST_DATA_INSTALL_PATH)")
                        ELSE()
                                SET(CUSTOM_DATA_INSTALL_PATH "'\\\"${CMAKE_INSTALL_PREFIX}/${MEGAGLEST_DATA_INSTALL_PATH}\\\"'" CACHE STRING "The FULL installation path for data files (this is build automatically by combining CMAKE_INSTALL_PREFIX and MEGAGLEST_DATA_INSTALL_PATH)")
                        ENDIF()
		ENDIF()
		SET(CUSTOM_DATA_INSTALL_PATH_VALUE "-DCUSTOM_DATA_INSTALL_PATH=${CUSTOM_DATA_INSTALL_PATH}")

                SET(PKG_DATADIR ${CUSTOM_DATA_INSTALL_PATH_VALUE})
                SET(PKG_BINDIR ${MEGAGLEST_BIN_INSTALL_PATH})
   
		MESSAGE(STATUS "*NOTE: Custom Data Install Path is [${CUSTOM_DATA_INSTALL_PATH}]")
	ENDIF()

	SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${GIT_VERSION_CMD} ${CUSTOM_DATA_INSTALL_PATH_VALUE}")
	SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${GIT_VERSION_CMD} ${CUSTOM_DATA_INSTALL_PATH_VALUE}")
	SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${GIT_VERSION_CMD} ${CUSTOM_DATA_INSTALL_PATH_VALUE}")
	SET(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} ${GIT_VERSION_CMD} ${CUSTOM_DATA_INSTALL_PATH_VALUE}")

	# We do some funky character escaping to get the right stuff written out to 
	# the final Makefile so we get the GIT Global Revsion #
	string(REPLACE "'" "\"" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
	string(REPLACE "'" "\"" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
	string(REPLACE "'" "\"" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
	string(REPLACE "'" "\"" CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}")

	IF( CMAKE_BUILD_TYPE STREQUAL Debug )
		message("CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
	ELSEIF( CMAKE_BUILD_TYPE STREQUAL Release )
		message("CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")
	ELSEIF( CMAKE_BUILD_TYPE STREQUAL RELWITHDEBINFO )
		message("CMAKE_CXX_FLAGS_RELWITHDEBINFO: ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
	ELSEIF( CMAKE_BUILD_TYPE STREQUAL MINSIZEREL )
		message("CMAKE_CXX_FLAGS_MINSIZEREL: ${CMAKE_CXX_FLAGS_MINSIZEREL}")

	ENDIF()

	# Win32 specific Compiler Flags
	IF(WIN32)
		ADD_DEFINITIONS("-D_WINDOWS -D_WIN32 -D_STDCALL_SUPPORTED -D_M_IX86 -DXML_LIBRARY -D_LIB -DCURL_STATICLIB")
	ELSE()
		ADD_DEFINITIONS("-DCURL_STATICLIB")
	ENDIF()
ENDIF()

IF(WIN32)
	SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH}
	   ${PROJECT_SOURCE_DIR}/source/win32_deps/lib
	   ${PROJECT_SOURCE_DIR}/source/win32_deps/xerces-c-src_2_8_0/lib
           ${PROJECT_SOURCE_DIR}/source/win32_deps/wxWidgets-2.8.10/lib)

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
	   ${PROJECT_SOURCE_DIR}/source/win32_deps/curl-7.21.3/include
	   ${PROJECT_SOURCE_DIR}/source/win32_deps/SDL-1.2.x/include
	   ${PROJECT_SOURCE_DIR}/source/win32_deps/wxWidgets-2.8.10/include)

	link_directories(${PROJECT_SOURCE_DIR}/source/win32_deps/lib)
	link_directories(${PROJECT_SOURCE_DIR}/source/win32_deps/xerces-c-src_2_8_0/lib)
	link_directories(${PROJECT_SOURCE_DIR}/source/win32_deps/wxWidgets-2.8.10/lib)
	include_directories("${PROJECT_SOURCE_DIR}/source/win32_deps/Microsoft\ DirectX\ SDK \(November 2007\)/Include")
	include_directories(${PROJECT_SOURCE_DIR}/source/win32_deps/openal-soft-1.12.854/include)
ENDIF()

# Check if sources exist
IF(EXISTS "${PROJECT_SOURCE_DIR}/source/")
	MESSAGE(STATUS "**Found game source code.")

	ADD_SUBDIRECTORY( ${PROJECT_SOURCE_DIR}/source/shared_lib )
	ADD_SUBDIRECTORY( ${PROJECT_SOURCE_DIR}/source/glest_game )
	#if(wxWidgets_FOUND)
	  ADD_SUBDIRECTORY( ${PROJECT_SOURCE_DIR}/source/glest_map_editor )
	  ADD_SUBDIRECTORY( ${PROJECT_SOURCE_DIR}/source/g3d_viewer )
	#else()
	#  MESSAGE(STATUS "WARNING... the following game tools will NOT be built since we cannot find wxWidgets on this machine")
	#  MESSAGE(STATUS "map editor, g3d viewer")
	#endif()
	ADD_SUBDIRECTORY( ${PROJECT_SOURCE_DIR}/source/tools/glexemel )

	IF(APPLE)
		include(${PROJECT_SOURCE_DIR}/mk/macosx/CMakeLists.txt)
	ENDIF()

        ADD_SUBDIRECTORY( ${PROJECT_SOURCE_DIR}/source/tests )
ENDIF()

# Check if data exist
IF(EXISTS "${PROJECT_SOURCE_DIR}/data/glest_game/CMakeLists.txt")
        MESSAGE(STATUS "**Found game data.")
	ADD_SUBDIRECTORY( ${PROJECT_SOURCE_DIR}/data/glest_game )
ENDIF()

SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Mark Vejvoda") #required
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "
 libcurl4-gnutls-dev | libcurl4-nss-dev,
 libfontconfig1-dev,
 libftgl-dev,
 libglew-dev,
 libircclient-dev,
 libjpeg-dev,
 liblua5.1-0-dev,
 libminiupnpc-dev,
 libogg-dev,
 libopenal-dev,
 libpng12-dev,
 libsdl1.2-dev,
 libvlc-dev,
 libvorbis-dev,
 libwxgtk2.8-dev,
 libxerces-c2-dev,
 libxml2-dev,
 libz-dev,
 libfribidi-dev")

#find_program(DPKG_PROGRAM dpkg DOC "dpkg program of Debian-based systems")
#if(DPKG_PROGRAM)
#  execute_process(
#    COMMAND ${DPKG_PROGRAM} --print-architecture
#    OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
#    OUTPUT_STRIP_TRAILING_WHITESPACE
#  )
#endif(DPKG_PROGRAM)

SET(CPACK_PACKAGE_NAME ${PKG_NAME})
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "MegaGlest")
SET(CPACK_PACKAGE_VENDOR "megaglest.org")
#SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
#SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "megaglest")
SET(CPACK_PACKAGE_VERSION_MAJOR ${VER_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${VER_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${VER_RELEASE})
IF(WIN32)
    SET(CPACK_NSIS_DISPLAY_NAME "MegaGlest")
    SET(CPACK_NSIS_MUI_ICON "${PROJECT_SOURCE_DIR}/mk/windoze/glest.ico")
    SET(CPACK_PACKAGE_ICON "${PROJECT_SOURCE_DIR}/mk/windoze/megaglest.ico")
    SET(CPACK_NSIS_URL_INFO_ABOUT "http://megaglest.org")
ENDIF()
INCLUDE(CPack)

get_directory_property( DirDefs DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS )
foreach( d ${DirDefs} )
    message( STATUS "=====> Found Define: " ${d} )
endforeach()
message( STATUS "=====> DirDefs: " ${DirDefs} )
#MESSAGE(STATUS "*** Compiler definitions are [${COMPILE_DEFINITIONS}]")

MARK_AS_ADVANCED(SDLMAIN_LIBRARY)
MARK_AS_ADVANCED(SDL_INCLUDE_DIR)
MARK_AS_ADVANCED(SDL_LIBRARY)



