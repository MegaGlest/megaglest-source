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

ADD_LIBRARY(streflop STATIC EXCLUDE_FROM_ALL
	${STREFLOP_SRC}
)
set_target_properties(streflop PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")
