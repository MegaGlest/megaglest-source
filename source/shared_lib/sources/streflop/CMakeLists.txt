#########################################################################################
# streflop
#if (NOT $ENV{CXX} MATCHES "icpc")
#	SET(cxxflags "${cxxflags} -mfpmath=sse -msse")
#endif (NOT $ENV{CXX} MATCHES "icpc")

FILE(GLOB_RECURSE STREFLOP_GLOBBED_CPP libm/flt-32/*.cpp)

INCLUDE_DIRECTORIES(
	../../include/streflop/libm/flt-32
	../../include/streflop/libm/headers
	)

SET(STREFLOP_SRC ${STREFLOP_GLOBBED_CPP})
# use SSE unconditionally (FIXME?)
#SET_SOURCE_FILES_PROPERTIES(${STREFLOP_SRC} PROPERTIES COMPILE_FLAGS "-DSTREFLOP_SSE -DLIBM_COMPILING_FLT32 -O3 ${CXXFLAGS}")

ADD_LIBRARY(streflop STATIC EXCLUDE_FROM_ALL
	${STREFLOP_SRC}
)
set_target_properties(streflop PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")
