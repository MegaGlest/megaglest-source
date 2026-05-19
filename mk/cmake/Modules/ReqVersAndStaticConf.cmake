#
# Required Versions And Static Config
#
# Things related with "Static build" and optional versioning.
#
# By assumption in general all should be done in the way that the default dynamic
# compilation should work even without this file.
#

IF(WANT_STATIC_LIBS)
    IF(BUILD_MEGAGLEST_MODEL_VIEWER OR BUILD_MEGAGLEST_MAP_EDITOR OR BUILD_MEGAGLEST)
	    # shared lib
	    FOREACH(STATIC_LIB
		    OpenSSL
		    CURL
		    XercesC
		    LUA
		    JPEG
		    PNG
		    FontConfig
		    FTGL
		    GLEW
		    FriBiDi
		    Miniupnpc
		    Ircclient)
	        LIST(APPEND LIST_OF_STATIC_LIBS_MG "${STATIC_LIB}")
	    ENDFOREACH()
    ENDIF()
    IF(BUILD_MEGAGLEST_MODEL_VIEWER OR BUILD_MEGAGLEST_MAP_EDITOR)
	    # wxWidgets for tools
	    FOREACH(STATIC_LIB
		    wxWidgets)
	        LIST(APPEND LIST_OF_STATIC_LIBS_MG "${STATIC_LIB}")
	    ENDFOREACH()

	    IF(NOT DEFINED STATIC_wxWidgets)
		SET(STATIC_wxWidgets OFF)
		# wxWidgets by default are not available static
	    ENDIF()
    ENDIF()
    IF(BUILD_MEGAGLEST)
	    # only libs not used by shared lib
	    FOREACH(STATIC_LIB
		    OGG)
	        LIST(APPEND LIST_OF_STATIC_LIBS_MG "${STATIC_LIB}")
	    ENDFOREACH()
    ENDIF()
    FOREACH(STATIC_LIB ${LIST_OF_STATIC_LIBS_MG})
	    IF(DEFINED WANT_USE_${STATIC_LIB} AND NOT WANT_USE_${STATIC_LIB})
	        IF(DEFINED STATIC_${STATIC_LIB})
		        UNSET(STATIC_${STATIC_LIB} CACHE)
	        ENDIF()
	    ELSE()
	        IF(DEFINED STATIC_${STATIC_LIB} AND NOT STATIC_${STATIC_LIB})
		    SET(STATIC_OPTION_VALUE OFF)
	        ELSE()
		    SET(STATIC_OPTION_VALUE ON)
	        ENDIF()
	        OPTION("STATIC_${STATIC_LIB}" "Set to ON to link your project with static ${STATIC_LIB} library (instead of DLL)." "${STATIC_OPTION_VALUE}")
	    ENDIF()
    ENDFOREACH()
ENDIF()

IF(STATIC_OpenSSL)
    SET(OPENSSL_USE_STATIC_LIBS ON)
ENDIF()

IF(STATIC_CURL AND UNIX)
    ADD_DEFINITIONS("-DCURL_STATICLIB")
ENDIF()
SET(CURL_MIN_VERSION_MG "7.16.4")

set(FORCE_LUA_VERSION "OFF" CACHE STRING "Lua version to use (e.g. 5.4). Set to OFF for auto-detect.")
set_property(CACHE FORCE_LUA_VERSION PROPERTY STRINGS OFF 5.5 5.4 5.3 5.2 5.1)
if(NOT FORCE_LUA_VERSION STREQUAL "OFF")
    find_package(Lua ${FORCE_LUA_VERSION} EXACT REQUIRED)
else()
    find_package(Lua REQUIRED)
endif()

IF(STATIC_JPEG)
    SET(JPEG_NAMES jpeg.a libjpeg.a ${JPEG_NAMES})
ENDIF()

IF(STATIC_PNG)
    list(APPEND PNG_NAMES png.a libpng.a)
    set(_PNG_VERSION_SUFFIXES 17 16 15 14 12 18 19 20 21 22)
    foreach(v IN LISTS _PNG_VERSION_SUFFIXES)
	list(APPEND PNG_NAMES png${v}.a libpng${v}.a)
	list(APPEND PNG_NAMES_DEBUG png${v}d.a libpng${v}d.a)
    endforeach()
    unset(_PNG_VERSION_SUFFIXES)
ENDIF()

IF(STATIC_wxWidgets)
    SET(wxWidgets_USE_STATIC ON)
    MESSAGE(STATUS "==========> wxWidgets wanting STATIC libs.")
ENDIF()

SET(VLC_MIN_VERSION_MG "1.1.0")
