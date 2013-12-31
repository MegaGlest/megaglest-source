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

IF(FORCE_LUA_5_2)
        MESSAGE(STATUS "Trying to FORCE LUA 5.2 ...")

        SET(LUA_FIND_INCLUDE_PATHS  /usr/include/lua5.2
                                    /usr/include
   			            /usr/include/lua )
        SET(LUA_FIND_STATIC_LIB_NAMES liblua5.2.a lua5.2 liblua.a lua )
        SET(LUA_FIND_DYNAMIC_LIB_NAMES lua5.2 lua )

ELSEIF(FORCE_LUA_5_1)
        MESSAGE(STATUS "Trying to FORCE LUA 5.1 ...")

        SET(LUA_FIND_INCLUDE_PATHS /usr/include/lua5.1
                                   /usr/include
                                   /usr/include/lua )
        SET(LUA_FIND_STATIC_LIB_NAMES liblua5.1.a lua5.1 liblua.a lua )
        SET(LUA_FIND_DYNAMIC_LIB_NAMES lua5.1 lua )
ELSE()
        SET(LUA_FIND_INCLUDE_PATHS /usr/include
                                /usr/include/lua5.2
			        /usr/include/lua
			        /usr/include/lua5.1 )
        SET(LUA_FIND_STATIC_LIB_NAMES liblua5.2.a liblua.a liblua5.1.a lua5.2 lua lua5.1 )
        SET(LUA_FIND_DYNAMIC_LIB_NAMES lua5.2 lua lua5.1 )
ENDIF()

FIND_PATH(LUA_INCLUDE_DIR NAMES lua.hpp 
		PATHS 	${LUA_FIND_INCLUDE_PATHS}
		IF(FreeBSD)
                	SET(PATHS "/usr/local/include/lua51")
                ENDIF()
		$ENV{LUA_HOME}
		)

IF (LUA_STATIC AND NOT LUA_LIBRARIES)
	FIND_LIBRARY(LUA_LIBRARIES NAMES ${LUA_FIND_STATIC_LIB_NAMES}
		PATHS 
                IF(FreeBSD)
                       SET(PATHS "/usr/local/lib/lua51")
                ENDIF()
                $ENV{LUA_HOME})

ELSE()
	FIND_LIBRARY(LUA_LIBRARIES NAMES ${LUA_FIND_DYNAMIC_LIB_NAMES}
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

