# Find the Lua 5.1 includes and library
#
# LUA_INCLUDE_DIR - where to find lua.h
# LUA_LIBRARIES - List of fully qualified libraries to link against
# LUA_FOUND - Set to TRUE if found

# Copyright (c) 2007, Pau Garcia i Quiles, <pgquiles@elpauer.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

OPTION(WANT_STATIC_LIBS "builds as many static libs as possible" OFF)
IF(WANT_STATIC_LIBS)
	OPTION(LUA_STATIC "Set to ON to link your project with static library (instead of DLL)." ON)
ENDIF()

IF(LUA_INCLUDE_DIR AND LUA_LIBRARIES)
	#SET(LUA_FIND_QUIETLY TRUE)
ENDIF(LUA_INCLUDE_DIR AND LUA_LIBRARIES)

FIND_PATH(LUA_INCLUDE_DIR NAMES lua.hpp 
		PATHS 	/usr/include
				/usr/include/lua
				/usr/include/lua5.1
		IF(FreeBSD)
                	SET(PATHS "/usr/local/include/lua51")
                ENDIF()
				$ENV{LUA_HOME}
		)

IF (LUA_STATIC AND NOT LUA_LIBRARIES)
	FIND_LIBRARY(LUA_LIBRARIES NAMES liblua5.1.a liblua.a lua5.1 lua
		PATHS 
                IF(FreeBSD)
                       SET(PATHS "/usr/local/lib/lua51")
                ENDIF()
                $ENV{LUA_HOME})

ELSE()
	FIND_LIBRARY(LUA_LIBRARIES NAMES lua5.1 lua
		PATHS 
                IF(FreeBSD)
                       SET(PATHS "/usr/local/lib/lua51")
                ENDIF()
                $ENV{LUA_HOME})
ENDIF()

MESSAGE(STATUS "LUA_INC: ${LUA_INCLUDE_DIR}")
MESSAGE(STATUS "LUA_LIB: ${LUA_LIBRARIES}")

IF(LUA_INCLUDE_DIR AND LUA_LIBRARIES)
	SET(LUA_FOUND TRUE)
	INCLUDE(CheckLibraryExists)
	CHECK_LIBRARY_EXISTS(${LUA_LIBRARIES} lua_close "" LUA_NEED_PREFIX)
ELSE(LUA_INCLUDE_DIR AND LUA_LIBRARIES)
	SET(LUA_FOUND FALSE)
ENDIF (LUA_INCLUDE_DIR AND LUA_LIBRARIES)

IF(LUA_FOUND)
	IF (NOT LUA_FIND_QUIETLY)
		MESSAGE(STATUS "Found Lua library: ${LUA_LIBRARIES}")
		MESSAGE(STATUS "Found Lua headers: ${LUA_INCLUDE_DIR}")
	ENDIF (NOT LUA_FIND_QUIETLY)
ELSE(LUA_FOUND)
	IF(LUA_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "Could NOT find Lua")
	ENDIF(LUA_FIND_REQUIRED)
ENDIF(LUA_FOUND)

MARK_AS_ADVANCED(LUA_INCLUDE_DIR LUA_LIBRARIES)
