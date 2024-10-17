IF(WIN32)
	# Windows cmake version needs to be higher - for CMAKE_MSVC_RUNTIME_LIBRARY.
	CMAKE_MINIMUM_REQUIRED( VERSION 3.15 )

	# This must be Set before "PROJECT()" is called.
	SET(VCPKG_TARGET_TRIPLET "x64-windows-static")
ELSE()
	CMAKE_MINIMUM_REQUIRED( VERSION 3.10 )
ENDIF()

PROJECT( MegaGlest )

#SET(CMAKE_VERBOSE_MAKEFILE ON)

#
# *NOTE: This script has been tested and works with with GCC, Clang and MSbuild (VS).
# MSBuild requires using vcpkg and installing required libs.
# Script works with Windows, Linux and Mac.
#

# build type
# This doesn't make sense for windows vcxprojs. All types of build produce the same proj.
IF(NOT MSVC)
	IF(NOT CMAKE_BUILD_TYPE)
		SET(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose build type: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
	ENDIF()

	SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS None Debug Release RelWithDebInfo MinSizeRel)
ENDIF()

IF(NOT CMAKE_VERSION VERSION_LESS "3.1")
    cmake_policy(SET CMP0054 NEW)
ENDIF()

IF(MSVC)
	# Sets the flags MT/ MTd correctly.
	SET(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

	INCLUDE(ProcessorCount)
	ProcessorCount(N)
	IF(N GREATER 0)
		MESSAGE(STATUS "Will build on ${N} cores.")
	ELSE()
		SET(N 1)
		MESSAGE(STATUS "Could not find number of cores. Will attempt to build on ${N} cores.")
	ENDIF()

	# "MP" is build with all cores.
	SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MP${N} /utf-8")
	SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MP${N} /utf-8")
	SET(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MP${N} /utf-8" )
	SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MP${N} /utf-8" )

	SET(MEGAGLEST_DATA_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}/" CACHE STRING "The installation path for data files ${COMMON_INFO_ABOUT_PATH}")
	SET(MEGAGLEST_INI_INSTALL_PATH "${MEGAGLEST_DATA_INSTALL_PATH}" CACHE STRING "Install path for ini files")

	INCLUDE(FindPkgConfig)
ENDIF()

# add additional CMake modules
MESSAGE(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/mk/cmake/Modules)
IF(CMAKE_BUILD_TYPE)
	MESSAGE(STATUS "Build type for this compile will be: ${CMAKE_BUILD_TYPE}")
ENDIF()
IF ("${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?Clang")
    SET(CMAKE_COMPILER_IS_GNUCXX "YES")
#        ADD_DEFINITIONS("-Qunused-arguments -Wno-switch")
    ADD_DEFINITIONS("-Wno-switch")
ENDIF()

OPTION(BUILD_MEGAGLEST_MODEL_IMPORT_EXPORT_TOOLS "Build model import/export tools" ON)
OPTION(BUILD_MEGAGLEST_MODEL_VIEWER "Build model viewer" ON)
OPTION(BUILD_MEGAGLEST_MAP_EDITOR "Build map editor" ON)
OPTION(BUILD_MEGAGLEST "Build MegaGlest" ON)
OPTION(BUILD_MEGAGLEST_TESTS "Build MegaGlest Unit Tests" OFF)
OPTION(INCLUDE_DATA "Include Game Data in build. If true but no data present, data will be skipped." ON)
OPTION(WANT_SINGLE_INSTALL_DIRECTORY "Use single install directory for everything. It is useful for example for MacOS cpack bundles." OFF)
OPTION(WANT_APPIMAGE "Enable AppImage compatibility" ON)
OPTION(WANT_STATIC_LIBS "Builds as many static libs as possible." OFF)
OPTION(WANT_USE_VLC "Use libVLC to play videos." OFF)
OPTION(WANT_USE_OpenSSL "Use libOpenSSL during CURL linking." ON)
OPTION(WANT_USE_FriBiDi "Enable libFriBIDi support." ON)
OPTION(WANT_USE_GoogleBreakpad "Enable GoogleBreakpad support." ON)
OPTION(WANT_USE_STREFLOP "Use the library streflop." ON)
OPTION(WANT_USE_XercesC "Enable libXercesC support." OFF)

FIND_PROGRAM(HELP2MAN "help2man")

set(XVFB_EXEC "")
find_program(XVFB_RUN NAMES "xvfb-run")
if (NOT ${XVFB_RUN} MATCHES "XVFB_RUN-NOTFOUND")
	set(XVFB_EXEC ${XVFB_RUN})
	message(STATUS "Using xvfb-run to run man2help.")
endif()
MARK_AS_ADVANCED(HELP2MAN XVFB_RUN)

include(${CMAKE_SOURCE_DIR}/mk/cmake/Modules/SpecialMacros.cmake)
include(${CMAKE_SOURCE_DIR}/mk/cmake/Modules/ReqVersAndStaticConf.cmake)
MESSAGE(STATUS "=====================================================================")

SET(PKG_NAME "megaglest")
# read version
FILE(READ ${PROJECT_SOURCE_DIR}/source/glest_game/facilities/game_util.cpp MG_VERSION_H_CONTENTS)
MESSAGE(STATUS "Determining version number system type to use:")
SET(Begin_otv_string "string glestVersionString")
STRING(REGEX MATCH "${Begin_otv_string}[ \t]*=[ \t]*\"[ \t]*v[ \t]*([0-9][^\\.]*)(\\.)([^\\.]*)(\\.)([^\"\\.;]*)\";" _threePartMatch "${MG_VERSION_H_CONTENTS}")
STRING(REGEX MATCH "${Begin_otv_string}[ \t]*=[ \t]*\"[ \t]*v[ \t]*([0-9][^\\.]*)(\\.)([^\"\\.;]*)\";" _twoPartMatch "${MG_VERSION_H_CONTENTS}")
STRING(REGEX MATCH "${Begin_otv_string}[ \t]*=[ \t]*\"[ \t]*v[ \t]*([0-9][^\"\\.;]*)\";" _onePartMatch "${MG_VERSION_H_CONTENTS}")
IF(_threePartMatch)
	MESSAGE(STATUS "** Trying 3 part versioning ")
	STRING(REGEX REPLACE "[^=]*=[ \t]*\"[ \t]*v[ \t]*([0-9][^\\.]*)\\.[^\\.]*\\..*" "\\1" VER_MAJOR "${_threePartMatch}")
	STRING(REGEX REPLACE "[^=]*=[ \t]*\"[ \t]*v[ \t]*[0-9][^\\.]*\\.([^\\.]*)\\..*" "\\1" VER_MINOR "${_threePartMatch}")
	STRING(REGEX REPLACE "[^=]*=[ \t]*\"[ \t]*v[ \t]*[0-9][^\\.]*\\.[^\\.]*\\.([^\"\\.;]*).*" "\\1" VER_PATCH "${_threePartMatch}")
	SET(MEGAGLEST_VERSION "${VER_MAJOR}.${VER_MINOR}.${VER_PATCH}")
ELSEIF(_twoPartMatch)
	MESSAGE(STATUS "** Trying 2 part versioning ")
	STRING(REGEX REPLACE "[^=]*=[ \t]*\"[ \t]*v[ \t]*([0-9][^\\.]*)\\..*" "\\1" VER_MAJOR "${_twoPartMatch}")
	STRING(REGEX REPLACE "[^=]*=[ \t]*\"[ \t]*v[ \t]*[0-9][^\\.]*\\.([^\"\\.;]*).*" "\\1" VER_MINOR "${_twoPartMatch}")
	SET(VER_PATCH "0")
	SET(MEGAGLEST_VERSION "${VER_MAJOR}.${VER_MINOR}")
ELSEIF(_onePartMatch)
	MESSAGE(STATUS "** Trying 1 part versioning ")
	STRING(REGEX REPLACE "[^=]*=[ \t]*\"[ \t]*v[ \t]*([0-9][^\"\\.;]*).*" "\\1" VER_MAJOR "${_onePartMatch}")
	SET(VER_MINOR "0")
	SET(VER_PATCH "0")
	SET(MEGAGLEST_VERSION "${VER_MAJOR}")
ENDIF()
FOREACH(ver_var VER_MAJOR;VER_MINOR;VER_PATCH;MEGAGLEST_VERSION)
	STRING(REGEX REPLACE "[ \t]" "" "${ver_var}" "${${ver_var}}")
ENDFOREACH()
MESSAGE(STATUS "Detected MegaGlest Version is [v${MEGAGLEST_VERSION}] ")

IF(NOT MSVC)
	SET(PIC_FLAG "${PIC_FLAG} -fPIC")
	IF(WANT_STATIC_LIBS AND "${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
		# CMAKE_SIZEOF_VOID_P=8 => 64bit
		ADD_DEFINITIONS("${PIC_FLAG}")
	ENDIF()
ENDIF()

## Compiler flags, CPACK configuration and other Apple specific code.
IF(APPLE)
	include(${PROJECT_SOURCE_DIR}/mk/macos/CMakeLists.txt)
ENDIF(APPLE)

IF(CMAKE_COMPILER_IS_GNUCXX OR MINGW OR MSVC)
    IF ("${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?Clang")
        MESSAGE(STATUS "Detected CLANG Compiler...")
	ELSEIF(CMAKE_COMPILER_IS_GNUCXX)
		MESSAGE(STATUS "Detected GNUC Compiler...")
	ELSEIF(MINGW)
		MESSAGE(STATUS "Detected MINGW Compiler...")
	ELSEIF(MSVC)
		MESSAGE(STATUS "Detected VS target for building with msbuild...")
	ENDIF()

	IF(NOT WANT_APPIMAGE)
		MESSAGE(STATUS "**Disable appimage code.")
		ADD_DEFINITIONS(-DNO_APPIMAGE)
	ELSEIF((NOT INCLUDE_DATA OR NOT EXISTS "${PROJECT_SOURCE_DIR}/data/glest_game/CMakeLists.txt") AND "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
		MESSAGE(STATUS "**AppImage compatiblity + nodata wanted. Adding flag \"APPIMAGE_NODATA\".")
		ADD_DEFINITIONS(-DAPPIMAGE_NODATA)
	ENDIF()

    # Common generic GNU type compiler options that work with all generic GCC compatible compilers
	IF(NOT MSVC)
    	ADD_DEFINITIONS("-Wuninitialized -Wsign-compare -Wunused-function -Wunused-variable -Wreturn-type -fno-strict-aliasing")
	ENDIF()

	# For gcc warning options see: http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
	IF(NOT WIN32)
		# For tons of verbose warnings add: -Wall
#		ADD_DEFINITIONS("-Wreturn-type -fno-strict-aliasing -frounding-math  -fsignaling-nans -mfpmath=sse -msse -rdynamic")
#		ADD_DEFINITIONS("-Wuninitialized -Wsign-compare -Wunused-function -Wunused-variable -Wreturn-type -fno-strict-aliasing -frounding-math  -fsignaling-nans -rdynamic")
	ELSE()
#		ADD_DEFINITIONS("-Wreturn-type -fno-strict-aliasing -frounding-math  -fsignaling-nans -mfpmath=sse -msse -DUNICODE")
#		ADD_DEFINITIONS("-Wuninitialized -Wsign-compare -Wunused-function -Wunused-variable -Wreturn-type -fno-strict-aliasing -frounding-math  -fsignaling-nans -DUNICODE")
        ADD_DEFINITIONS("-DUNICODE")
	ENDIF()

    # CLang specific Compiler Options
    IF ("${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?Clang")

	ELSEIF(MSVC)

    # GCC specific Compiler Options
    ELSE()
        ADD_DEFINITIONS("-frounding-math -fsignaling-nans")

#                IF(NOT MINGW)
#                        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -rdynamic")
#                        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic")
#                ENDIF()
    ENDIF()

if(DEFINED ENV{CFLAGS})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} $ENV{CFLAGS}")
endif()

if(DEFINED ENV{CXXFLAGS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{CXXFLAGS}")
endif()

# Include the necessary modules to check compiler flags
include(CheckCXXCompilerFlag)
include(CheckCCompilerFlag)

# Define your extra flags in a list
set(EXTRA_FLAGS
    "-Werror=odr"
    "-Werror=lto-type-mismatch"
    "-Werror=strict-aliasing"
)

# Loop through each flag and check for both C++ and C compilers
foreach(flag ${EXTRA_FLAGS})

    # Check for C++ compiler
    check_cxx_compiler_flag(${flag} CXX_SUPPORTS_${flag})
    if(CXX_SUPPORTS_${flag})
        message(STATUS "C++ compiler supports flag ${flag}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
    else()
        message(STATUS "C++ compiler does not support flag ${flag}")
    endif()

    # Check for C compiler
    check_c_compiler_flag(${flag} C_SUPPORTS_${flag})
    if(C_SUPPORTS_${flag})
        message(STATUS "C compiler supports flag ${flag}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}")
    else()
        message(STATUS "C compiler does not support flag ${flag}")
    endif()

endforeach()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT result OUTPUT output)

    if(result)
        message(STATUS "IPO (LTO) is supported: enabling for Release build.")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS "IPO (LTO) is not supported: ${output}")
    endif()
endif()

	#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    #MESSAGE(STATUS "*TEST: Checking for max SSE LEVEL [${FORCE_MAX_SSE_LEVEL}]")

	OPTION(FORCE_STREFLOP_SOFTWRAPPER "Set the streflop library to be forced to use the software emulator" OFF)
    IF(NOT FORCE_MAX_SSE_LEVEL AND NOT FORCE_STREFLOP_SOFTWRAPPER)
        SET(FORCE_MAX_SSE_LEVEL "1" CACHE STRING "Set the max SSE level to use if supported (0-3)")
    ENDIF()

    IF(NOT FORCE_STREFLOP_SOFTWRAPPER)
        MESSAGE(STATUS "*NOTE: Checking for max SSE LEVEL [${FORCE_MAX_SSE_LEVEL}]")
        special_check_for_sse( ${FORCE_MAX_SSE_LEVEL} )
    ENDIF()

	IF(WANT_USE_STREFLOP)
		ADD_DEFINITIONS("-DUSE_STREFLOP -DSTREFLOP_RANDOM_GEN_SIZE=32 -DLIBM_COMPILING_FLT32 -DN_SPECIALIZED=32")

        IF(HAS_SSE_EXTENSIONS AND NOT ${FORCE_MAX_SSE_LEVEL} MATCHES "0" AND NOT FORCE_STREFLOP_SOFTWRAPPER)
            ADD_DEFINITIONS("-DSTREFLOP_SSE")
            MESSAGE(STATUS "*NOTE: using SSE for STREFLOP.")
        ELSE()
            IF(NOT FORCE_STREFLOP_SOFTWRAPPER)
                special_check_for_x87()
            ENDIF()

            IF(HAS_X87_SUPPORT AND NOT FORCE_STREFLOP_SOFTWRAPPER)
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

	IF(NOT MSVC)
		# Debug compiler flags
		SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3")

		# Release compiler flags
		SET(CMAKE_CXX_FLAGS_RELEASE "-O3 ${CMAKE_CXX_FLAGS_RELEASE} -O3 ")
		IF(NOT CMAKE_GENERATOR STREQUAL Xcode)
			SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s")  ## Strip binary
		ENDIF()

		# Release with debug info compiler flags
		SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 ${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g -O3 ")

		# Release minimum size compiler flags
		IF(NOT CMAKE_GENERATOR STREQUAL Xcode)
			SET(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} -s")  ## Strip binary
		ENDIF()
	ENDIF()

    # see if this fixes compile issue for tomreyn: libpthread.so.0: error adding symbols: DSO missing from command line
    IF(UNIX)
        MESSAGE(STATUS "*NOTE: Apply fix for: libpthread.so.0: error adding symbols: DSO missing from command line")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -pthread")
        SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -pthread")
        SET(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} -pthread")
    ENDIF()

	# Get the git revision info for the binary
	SET(HAS_GIT "FALSE")
	SET(GIT_LIVE_REV_CMD "")

	OPTION(WANT_GIT_STAMP "use git revision stamp" ON)
	IF(WANT_GIT_STAMP)
		# The stuff below gets GIT Global Revision # but ONLY when calling cmake!
		# the FindGit.cmake module is part of the standard distribution
		include(FindGit)

		IF(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git/")
            SET(HAS_GIT "TRUE")
            MESSAGE(STATUS "Found GIT and using GIT version stamping...")

            # Get the current commit SHA1
            execute_process(
              COMMAND git log -1 --format=%h --abbrev=7
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

            SET(GIT_LIVE_REV_CMD "`cd '${PROJECT_SOURCE_DIR}' && git rev-list HEAD --count`.`cd '${PROJECT_SOURCE_DIR}' && git log -1 --format=%h --abbrev=7`")

            MESSAGE(STATUS "Using GIT revision stamp: [${GIT_COMMIT_COUNT}.${GIT_SHA1}] CMD [${GIT_LIVE_REV_CMD}]")

			IF(CMAKE_GENERATOR STREQUAL Xcode)
				SET(GIT_VERSION_CMD "-DGITVERSION='\\\\'${GIT_COMMIT_COUNT}.${GIT_SHA1}\\\\''")
			ELSE()
				SET(GIT_VERSION_CMD "-DGITVERSION='\\\"${GIT_LIVE_REV_CMD}\\\"'")
			ENDIF()
		ENDIF()
	ENDIF()

	SET(COMMON_INFO_ABOUT_PATH "(if the path is relative then is appended to the CMAKE_INSTALL_PREFIX)")
	IF(CMAKE_INSTALL_PREFIX STREQUAL "")
		MESSAGE(STATUS "*NOTE: NOT USING a Custom Data Install Path...")
	ELSE()
		IF(WANT_SINGLE_INSTALL_DIRECTORY AND NOT MEGAGLEST_SINGLE_DIRECTORY_INSTALL_PATH)
			SET(MEGAGLEST_SINGLE_DIRECTORY_INSTALL_PATH "megaglest-game/" CACHE STRING "The single directory installation path for game ${COMMON_INFO_ABOUT_PATH}")
		ENDIF()
		IF(NOT MEGAGLEST_BIN_INSTALL_PATH AND NOT WANT_SINGLE_INSTALL_DIRECTORY)
			SET(MEGAGLEST_BIN_INSTALL_PATH "bin/" CACHE STRING "The installation path for binaries ${COMMON_INFO_ABOUT_PATH}")
		ENDIF()
		IF(NOT MEGAGLEST_DATA_INSTALL_PATH AND NOT WANT_SINGLE_INSTALL_DIRECTORY)
			SET(MEGAGLEST_DATA_INSTALL_PATH "share/megaglest/" CACHE STRING "The installation path for data files ${COMMON_INFO_ABOUT_PATH}")
		ENDIF()
		SET(MEGAGLEST_INI_INSTALL_PATH "${MEGAGLEST_DATA_INSTALL_PATH}")
		IF(NOT MEGAGLEST_DESKTOP_INSTALL_PATH AND NOT WANT_SINGLE_INSTALL_DIRECTORY)
			SET(MEGAGLEST_DESKTOP_INSTALL_PATH "share/applications/" CACHE STRING "The installation path for desktop files ${COMMON_INFO_ABOUT_PATH}")
		ENDIF()
		IF(NOT MEGAGLEST_ICON_INSTALL_PATH AND NOT WANT_SINGLE_INSTALL_DIRECTORY)
			SET(MEGAGLEST_ICON_INSTALL_PATH "share/pixmaps/" CACHE STRING "The installation path for icon files ${COMMON_INFO_ABOUT_PATH}")
		ENDIF()
		IF(NOT MEGAGLEST_MANPAGE_INSTALL_PATH AND NOT WANT_SINGLE_INSTALL_DIRECTORY)
			SET(MEGAGLEST_MANPAGE_INSTALL_PATH "share/man/man6/" CACHE STRING "The installation path for manpage files ${COMMON_INFO_ABOUT_PATH}")
		ENDIF()
		IF(WANT_SINGLE_INSTALL_DIRECTORY)
			FOREACH(MG_PATH BIN DATA INI DESKTOP ICON)
				IF(NOT MEGAGLEST_${MG_PATH}_INSTALL_PATH)
					SET(MEGAGLEST_${MG_PATH}_INSTALL_PATH "${MEGAGLEST_SINGLE_DIRECTORY_INSTALL_PATH}")
				ENDIF()
			ENDFOREACH()
		ENDIF()
		IF(NOT WANT_SINGLE_INSTALL_DIRECTORY)
			FOREACH(MG_PATH DATA INI)
				IF(IS_ABSOLUTE "${MEGAGLEST_${MG_PATH}_INSTALL_PATH}")
					SET(MEGAGLEST_FULL_${MG_PATH}_INSTALL_PATH "${MEGAGLEST_${MG_PATH}_INSTALL_PATH}/")
				ELSE()
					SET(MEGAGLEST_FULL_${MG_PATH}_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}/${MEGAGLEST_${MG_PATH}_INSTALL_PATH}/")
				ENDIF()
				STRING(REGEX REPLACE "//+" "/" MEGAGLEST_FULL_${MG_PATH}_INSTALL_PATH "${MEGAGLEST_FULL_${MG_PATH}_INSTALL_PATH}")
			ENDFOREACH()

			IF(NOT CUSTOM_DATA_INSTALL_PATH)
				SET(CUSTOM_DATA_INSTALL_PATH "${MEGAGLEST_FULL_DATA_INSTALL_PATH}")
				# ^ someday this intermediate step may be removed
			ENDIF()
			SET(CUSTOM_INSTALL_PATHS_VALUES "-DCUSTOM_DATA_INSTALL_PATH=${CUSTOM_DATA_INSTALL_PATH}")
		ENDIF()

		#SET(PKG_DATADIR ${CUSTOM_DATA_INSTALL_PATH_VALUE})
		#SET(PKG_BINDIR ${MEGAGLEST_BIN_INSTALL_PATH})
		# ^ hard to tell for what it is needed and most likely both were set to wrong values, so let's try to not use them
		MESSAGE(STATUS "*NOTE: Custom Data Install Path is [${CUSTOM_DATA_INSTALL_PATH}]")
	ENDIF()

	string(TOUPPER "${CMAKE_BUILD_TYPE}" MG_BUILD_TYPE)
	IF(HAS_GIT STREQUAL "TRUE")
		SET(CMAKE_CXX_FLAGS_${MG_BUILD_TYPE} "${CMAKE_CXX_FLAGS_${MG_BUILD_TYPE}} ${GIT_VERSION_CMD}")
	ENDIF()
	SET(CMAKE_CXX_FLAGS_${MG_BUILD_TYPE} "${CMAKE_CXX_FLAGS_${MG_BUILD_TYPE}} ${CUSTOM_INSTALL_PATHS_VALUES}")

	# We do some funky character escaping to get the right stuff written out to
	# the final Makefile so we get the GIT Global Revsion #
	string(REPLACE "'" "\"" CMAKE_CXX_FLAGS_${MG_BUILD_TYPE} "${CMAKE_CXX_FLAGS_${MG_BUILD_TYPE}}")

	message(STATUS "CMAKE_CXX_FLAGS_${MG_BUILD_TYPE}: ${CMAKE_CXX_FLAGS_${MG_BUILD_TYPE}}")

	# Win32 specific Compiler Flags
	IF(WIN32 AND NOT MSVC)
		ADD_DEFINITIONS("-D_WINDOWS -D_WIN32 -D_STDCALL_SUPPORTED -D_M_IX86 -DXML_LIBRARY -D_LIB -DCURL_STATICLIB")
	ENDIF()
ENDIF()

IF(UNIX AND NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
	OPTION(WANT_DEPRECATION_WARNINGS "Want to see warnings related with deprecated code parts." OFF)
	# We don't wanna see bug reports with deprecation warnings from OSes where game isn't
	# actively developed, and where usually those warnings can make "make log" completely unreadable.
	# Main targets are: bsd and macos

	MARK_AS_ADVANCED(CLEAR wxWidgets_CONFIG_EXECUTABLE)
	# This variable may be sometimes necessary to set manually.
ELSE()
	OPTION(WANT_DEPRECATION_WARNINGS "Want to see warnings related with deprecated code parts." ON)
ENDIF()
IF(NOT WANT_DEPRECATION_WARNINGS)
	ADD_DEFINITIONS(-Wno-deprecated -Wno-deprecated-declarations)
ENDIF()
MARK_AS_ADVANCED(WANT_DEPRECATION_WARNINGS)

IF(NOT MG_CMAKE_INSTALL_PREFIX AND NOT CMAKE_INSTALL_PREFIX STREQUAL "")
	SET(MG_CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
ENDIF()
MESSAGE(STATUS "**NOTE: MG_CMAKE_INSTALL_PREFIX is [${MG_CMAKE_INSTALL_PREFIX}]")

SET(SDL_WINDOWS_DIR_DINC "SDL-2.0.x")
SET(SDL_VERSION_NAME "SDL2")
SET(SDL_VERSION_SNAME "sdl")

OPTION(WANT_DEV_OUTPATH "Use developer friendly output paths." OFF)
IF(UNIX AND "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
	SET(MEGAGLEST_FRIENDLY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/mk/linux/")
ELSEIF(UNIX AND APPLE)
	SET(MEGAGLEST_FRIENDLY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/mk/macos/")
ELSEIF(UNIX AND NOT APPLE)
	SET(MEGAGLEST_FRIENDLY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/mk/other_unix/")
ELSEIF(WIN32)
	SET(MEGAGLEST_FRIENDLY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/mk/windoze/")
ELSE()
	SET(MEGAGLEST_FRIENDLY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/data/glest_game/")
ENDIF()

# Set VS project exe outputs.
IF(MSVC)
	SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${MEGAGLEST_FRIENDLY_OUTPUT_PATH})
	SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${MEGAGLEST_FRIENDLY_OUTPUT_PATH})
	SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${MEGAGLEST_FRIENDLY_OUTPUT_PATH})
	SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${MEGAGLEST_FRIENDLY_OUTPUT_PATH})
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

    ADD_SUBDIRECTORY( ${PROJECT_SOURCE_DIR}/source/tests )
ENDIF()

# Check if data exist
IF(INCLUDE_DATA AND EXISTS "${PROJECT_SOURCE_DIR}/data/glest_game/CMakeLists.txt")
    MESSAGE(STATUS "**Found game data.")
	ADD_SUBDIRECTORY( ${PROJECT_SOURCE_DIR}/data/glest_game )
ENDIF()

#get_directory_property( DirDefs COMPILE_DEFINITIONS )
#MESSAGE(STATUS ">>>> NOTICE Compiler definitions used: ${DirDefs}")
#MESSAGE(STATUS "END of compile defs...")
IF ("${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?Clang")
    MESSAGE(WARNING ">> CLANG is NOT currently compatible as it does not support the following essential GCC compiler settings: -frounding-math -fsignaling-nans")
ENDIF()

IF(NOT DEFINED CPACK_GENERATOR)
	SET(CPACK_GENERATOR "DEB")
ENDIF()
# CPack configuration shared accross platforms
SET(CPACK_PACKAGE_NAME ${PKG_NAME})
IF(INCLUDE_DATA AND EXISTS "${PROJECT_SOURCE_DIR}/data/glest_game/others/megaglest-long-description.txt.in")
	FILE(READ "${PROJECT_SOURCE_DIR}/data/glest_game/others/megaglest-long-description.txt.in" MEGAGLEST_LONG_DESCRIPTION)
	SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${MEGAGLEST_LONG_DESCRIPTION}")
ELSE()
	SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "MegaGlest")
ENDIF()
SET(CPACK_PACKAGE_VENDOR "megaglest.org")
#SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
#SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "megaglest")
SET(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
SET(CPACK_PACKAGE_VERSION "${MEGAGLEST_VERSION}")
SET(CPACK_PACKAGE_VERSION_MAJOR "${VER_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${VER_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${VER_PATCH}")

IF("${CPACK_GENERATOR}" STREQUAL "DEB")
	SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Mark Vejvoda") #required
	SET(CPACK_DEBIAN_PACKAGE_SECTION "games")
	SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
	SET(CPACK_DEBIAN_PACKAGE_SUGGESTS "p7zip-full")
ENDIF()
IF("${CPACK_GENERATOR}" STREQUAL "NSIS")
	SET(CPACK_NSIS_DISPLAY_NAME "MegaGlest")
	SET(CPACK_NSIS_MUI_ICON "${PROJECT_SOURCE_DIR}/mk/windoze/glest.ico")
	SET(CPACK_PACKAGE_ICON "${PROJECT_SOURCE_DIR}/mk/windoze/megaglest.ico")
	SET(CPACK_NSIS_URL_INFO_ABOUT "https://megaglest.org")
ENDIF()

INCLUDE(CPack)

get_directory_property( DirDefs DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS )
foreach( d ${DirDefs} )
    message( STATUS "=====> Found Define: " ${d} )
endforeach()
#MESSAGE( STATUS "=====> DirDefs: " ${DirDefs} )
#MESSAGE(STATUS "*** Compiler definitions are [${COMPILE_DEFINITIONS}]")

MARK_AS_ADVANCED(${SDL_VERSION_NAME}MAIN_LIBRARY)
MARK_AS_ADVANCED(${SDL_VERSION_NAME}_INCLUDE_DIR)
MARK_AS_ADVANCED(${SDL_VERSION_NAME}_LIBRARY)
MARK_AS_ADVANCED(wxWidgets_CONFIG_EXECUTABLE)
MARK_AS_ADVANCED(wxWidgets_wxrc_EXECUTABLE)
MARK_AS_ADVANCED(wxWidgets_USE_DEBUG)
