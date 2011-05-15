# - Find curl
# Find the native CURL headers and libraries.
#
#  CURL_INCLUDE_DIRS - where to find curl/curl.h, etc.
#  CURL_LIBRARIES    - List of libraries when using curl.
#  CURL_FOUND        - True if curl found.

# Look for the header file.
FIND_PATH(CURL_INCLUDE_DIR NAMES curl/curl.h)
MARK_AS_ADVANCED(CURL_INCLUDE_DIR)

# Look for the library.
FIND_LIBRARY(CURL_LIBRARY NAMES curl curl-gnutls
                          PATHS "/usr/local/lib/")
MARK_AS_ADVANCED(CURL_LIBRARY)

# handle the QUIETLY and REQUIRED arguments and set CURL_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CURL DEFAULT_MSG CURL_LIBRARY CURL_INCLUDE_DIR)

IF(CURL_FOUND)

  SET(CURL_LIBRARIES ${CURL_LIBRARY})
  SET(CURL_INCLUDE_DIRS ${CURL_INCLUDE_DIR})

  # IF we are using a system that supports curl-config use it
  # and force using static libs
  IF(UNIX AND NOT APPLE)
    FIND_PROGRAM( CMAKE_CURL_CONFIG curl-config)
    MARK_AS_ADVANCED(CMAKE_CURL_CONFIG)

    IF(CMAKE_CURL_CONFIG)
      OPTION(WANT_STATIC_LIBS "builds as many static libs as possible" OFF)
      IF(WANT_STATIC_LIBS)
	# run the curl-config program to get --static-libs
	EXEC_PROGRAM(sh
		ARGS "${CMAKE_CURL_CONFIG} --static-libs"
	        OUTPUT_VARIABLE CURL_STATIC_LIBS
	        RETURN_VALUE RET)

	MESSAGE(STATUS "CURL RET = ${RET}")
      ELSE()
      	SET(RET 1)
      ENDIF()

      IF(${RET} EQUAL 0)
        MESSAGE(STATUS "USING CURL STATIC LIBS: ${CURL_STATIC_LIBS}")
      	SET(CURL_LIBRARIES "-Bstatic ${CURL_STATIC_LIBS}")
      ELSE()

        EXEC_PROGRAM(sh
	  ARGS "${CMAKE_CURL_CONFIG} --libs"
          OUTPUT_VARIABLE CURL_STATIC_LIBS
          RETURN_VALUE RET)

        MESSAGE(STATUS "#2 CURL RET = ${RET} using CURL dynamic libs: ${CURL_STATIC_LIBS}")
        SET(CURL_LIBRARIES "${CURL_STATIC_LIBS}")

      ENDIF()
    ENDIF()
ENDIF()
ELSE(CURL_FOUND)
  SET(CURL_LIBRARIES)
  SET(CURL_INCLUDE_DIRS)
ENDIF(CURL_FOUND)
