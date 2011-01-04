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
FIND_LIBRARY(CURL_LIBRARY NAMES curl curl-gnutls)
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

    IF(CMAKE_CURL_CONFIG)
      # run the curl-config program to get --static-libs
      EXEC_PROGRAM(sh
	ARGS "${CMAKE_CURL_CONFIG} --static-libs"
        OUTPUT_VARIABLE CURL_STATIC_LIBS
        RETURN_VALUE RET)

      SET(CURL_LIBRARIES "-static ${CURL_STATIC_LIBS}")
    ENDIF()

ENDIF()
ELSE(CURL_FOUND)
  SET(CURL_LIBRARIES)
  SET(CURL_INCLUDE_DIRS)
ENDIF(CURL_FOUND)
