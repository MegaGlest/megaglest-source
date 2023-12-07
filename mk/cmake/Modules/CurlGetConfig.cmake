#
# Curl Get Config
#
# IF we are using a system that supports curl-config use it.
#

IF(CURL_FOUND)
    IF(UNIX)
	IF(UNIX AND APPLE AND NOT CMAKE_CURL_CONFIG)
	    FIND_PROGRAM(CMAKE_CURL_CONFIG curl-config
			PATHS /opt/local
			PATH_SUFFIXES bin NO_DEFAULT_PATH)
	ENDIF()
	IF(NOT CMAKE_CURL_CONFIG)
	    FIND_PROGRAM(CMAKE_CURL_CONFIG curl-config
			PATHS
			~/Library/Frameworks
			/Library/Frameworks
			/sw # Fink
			/opt/local # DarwinPorts
			/opt/csw # Blastwave
			/opt
			PATH_SUFFIXES bin)
	ENDIF()
	MARK_AS_ADVANCED(CMAKE_CURL_CONFIG)

	IF(CMAKE_CURL_CONFIG)
	    IF(STATIC_CURL)
		# run the curl-config program to get --static-libs
		execute_process(COMMAND ${CMAKE_CURL_CONFIG} --static-libs
				OUTPUT_VARIABLE CURL_STATIC_LIBS
				RESULT_VARIABLE RET
				OUTPUT_STRIP_TRAILING_WHITESPACE)
				set(CURL_STATIC_LIBS "${CURL_STATIC_LIBS} -lldap")
	    ELSE()
		SET(RET 1)
	    ENDIF()

	    IF(RET EQUAL 0 AND CURL_STATIC_LIBS)
		MESSAGE(STATUS "curl-config: ${CMAKE_CURL_CONFIG}, #1 , using CURL static libs: [${CURL_STATIC_LIBS}]")
		SET(CURL_LIBRARIES "-Bstatic ${CURL_STATIC_LIBS}")
	    ELSE()
		execute_process(COMMAND ${CMAKE_CURL_CONFIG} --libs
				OUTPUT_VARIABLE CURL_DYNAMIC_LIBS
				RESULT_VARIABLE RET2
				OUTPUT_STRIP_TRAILING_WHITESPACE)

		IF(RET2 EQUAL 0 AND CURL_DYNAMIC_LIBS)
		    MESSAGE(STATUS "curl-config: ${CMAKE_CURL_CONFIG}, #2 RET = ${RET}, using CURL dynamic libs: ${CURL_DYNAMIC_LIBS}")
		    SET(CURL_LIBRARIES "${CURL_DYNAMIC_LIBS}")
		ELSE()
		    MESSAGE(STATUS "curl-config: ${CMAKE_CURL_CONFIG}, #3 RET = ${RET}/${RET2}, using CURL libs found by cmake: ${CURL_LIBRARIES}")
		ENDIF()
	    ENDIF()
	ENDIF()
    ENDIF()
    IF(CURL_VERSION_STRING AND "${CURL_VERSION_STRING}" VERSION_LESS "${CURL_MIN_VERSION_MG}")
	MESSAGE(STATUS "(please visit http://curl.haxx.se/libcurl/ to find a newer version)")
	MESSAGE(FATAL_ERROR " CURL version = [${CURL_VERSION_STRING}] we require AT LEAST [7.16.4]")
    ENDIF()
ELSE()
    SET(CURL_LIBRARIES)
    SET(CURL_INCLUDE_DIRS)
ENDIF()
