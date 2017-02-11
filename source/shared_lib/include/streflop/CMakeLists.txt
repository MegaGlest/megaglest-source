find_package(PkgConfig REQUIRED)
IF(FORCE_STREFLOP_SOFTWRAPPER)
    pkg_search_module(STREFLOP streflop-soft)
ELSE()
    IF(HAS_SSE_EXTENSIONS AND NOT ${FORCE_MAX_SSE_LEVEL} MATCHES "0")
        pkg_search_module(STREFLOP streflop-sse)
    ELSE()
        IF(HAS_X87_SUPPORT)
            pkg_search_module(STREFLOP streflop-x87)
        ELSE()
            pkg_search_module(STREFLOP streflop-soft)
        ENDIF()
    ENDIF()
ENDIF()
IF(NOT STREFLOP_FOUND)
    pkg_search_module(STREFLOP streflop)
ENDIF()
MESSAGE(STATUS "Search for Library STREFLOP result = ${STREFLOP_FOUND} libs: ${STREFLOP_LIBRARIES} include dirs: ${STREFLOP_INCLUDE_DIRS}")

IF(FORCE_EMBEDDED_LIBS)
    SET(STREFLOP_FOUND OFF)
    MESSAGE(STATUS "FORCING USE of EMBEDDED Libraries...")
ENDIF()

IF(WANT_USE_STREFLOP)
    IF(STREFLOP_FOUND)
        INCLUDE_DIRECTORIES(${STREFLOP_INCLUDE_DIRS} ${STREFLOP_INCLUDE_DIRS}/streflop)
        SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${STREFLOP_LIBRARIES})

	    ADD_DEFINITIONS("-DUSE_STREFLOP_PKG")
    ENDIF()
ENDIF()

IF(NOT STREFLOP_FOUND)
    AUX_SOURCE_DIRECTORY(libm/flt-32 libm_flt32_source)

    SET(cxxflags "-w -O3 -I${CMAKE_CURRENT_SOURCE_DIR}/libm/headers")
    if (NOT $ENV{CXX} MATCHES "icpc")
	    SET(cxxflags "${cxxflags} -mfpmath=sse -msse")
    endif (NOT $ENV{CXX} MATCHES "icpc")
    SET_SOURCE_FILES_PROPERTIES(${libm_flt32_source} PROPERTIES COMPILE_FLAGS "-DLIBM_COMPILING_FLT32 ${cxxflags}")

    ADD_LIBRARY(streflop STATIC EXCLUDE_FROM_ALL
	    SMath.cpp
	    Random.cpp
	    streflopC.cpp
	    ${libm_flt32_source}
    )
    set_target_properties(streflop PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")
ENDIF()
