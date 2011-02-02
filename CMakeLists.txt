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
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/mk/cmake/Modules)

MESSAGE(STATUS "Build type for this compile will be: ${CMAKE_BUILD_TYPE}")

## Compiler flags
IF(CMAKE_COMPILER_IS_GNUCXX OR MINGW)
	IF(CMAKE_COMPILER_IS_GNUCXX)
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
		ADD_DEFINITIONS("-Wreturn-type -fno-strict-aliasing -frounding-math  -fsignaling-nans -mfpmath=sse -msse -rdynamic")
	ELSE()
		ADD_DEFINITIONS("-Wreturn-type -fno-strict-aliasing -frounding-math  -fsignaling-nans -mfpmath=sse -msse")
	ENDIF()
	#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")	

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

	OPTION(WANT_SVN_STAMP "use svn stamp" ON)
	IF(WANT_SVN_STAMP)
		# The stuff below gets SVN Global Revision # but ONLY when calling cmake!
		# the FindSubversion.cmake module is part of the standard distribution
		include(FindSubversion)
		# extract working copy information for SOURCE_DIR into MY_XXX variables
		Subversion_WC_INFO(${PROJECT_SOURCE_DIR} MG)
		#add_definitions(-DSVNVERSION="${MG_WC_REVISION}")
	ENDIF()

	SET(SVN_VERSION_CMD "-DSVNVERSION='\\\"${MG_WC_REVISION}\\\"'")

	IF(UNIX AND NOT APPLE)
		# We do some funky character escaping to get the right stuff written out to 
		# the final Makefile so we get the SVN Global Revsion #
		SET(SVN_VERSION_CMD "-DSVNVERSION='\\\"`svnversion -n ${PROJECT_SOURCE_DIR}`\\\"'")
	ENDIF()

	SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${SVN_VERSION_CMD}")
	SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${SVN_VERSION_CMD}")
	SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${SVN_VERSION_CMD}")
	SET(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} ${SVN_VERSION_CMD}")
	# We do some funky character escaping to get the right stuff written out to 
	# the final Makefile so we get the SVN Global Revsion #
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
		ADD_DEFINITIONS("-D_WINDOWS -D_WIN32 -D_STDCALL_SUPPORTED -D_M_IX86 -DXML_LIBRARY -D_LIB -DUSE_STREFLOP -DSTREFLOP_SSE -DSTREFLOP_RANDOM_GEN_SIZE=32 -DLIBM_COMPILING_FLT32 -DCURL_STATICLIB")
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
	   
ADD_SUBDIRECTORY( source/shared_lib )
ADD_SUBDIRECTORY( source/glest_game )
#if(wxWidgets_FOUND)
  ADD_SUBDIRECTORY( source/glest_map_editor )
  ADD_SUBDIRECTORY( source/g3d_viewer )
  ADD_SUBDIRECTORY( source/configurator )
#else()
#  MESSAGE(STATUS "WARNING... the following game tools will NOT be built since we cannot find wxWidgets on this machine")
#  MESSAGE(STATUS "map editor, g3d viewer, configurator")
#endif()
ADD_SUBDIRECTORY( source/tools/glexemel )

IF(APPLE)
	include(mk/macosx/CMakeLists.txt)
ENDIF(APPLE)
